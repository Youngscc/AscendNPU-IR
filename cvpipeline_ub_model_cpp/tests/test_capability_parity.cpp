#include "../src/passes/loop_invariant_subset_hoisting.hpp"
#include "../src/passes/inline_scope_strict.hpp"
#include "../src/passes/hivm_inline_otf_load_store.hpp"
#include "../src/passes/tile_cube_vector_loop.hpp"
#include "../src/passes/tile_and_bind_sub_block.hpp"
#include "../src/passes/tightly_coupled_buffer_guard.hpp"
#include "../src/passes/mark_multi_buffer.hpp"
#include "../src/passes/sink_op_to_consumer_in_loop.hpp"

#include <iostream>
#include <stdexcept>

namespace {

void Check(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}

const cvub::GenericOperation &FindOperation(
    const cvub::GenericModule &module, const char *name) {
  for (const cvub::GenericOperation &operation : module.operations)
    if (operation.name == name)
      return operation;
  throw std::runtime_error(std::string("missing operation: ") + name);
}

const cvub::GenericOperation &FindCase(
    const cvub::GenericModule &module, const char *caseName) {
  const std::string marker = std::string("case = \"") + caseName + "\"";
  for (const cvub::GenericOperation &operation : module.operations)
    if (operation.attributes.find(marker) != std::string::npos ||
        operation.properties.find(marker) != std::string::npos)
      return operation;
  throw std::runtime_error(std::string("missing case: ") + caseName);
}

size_t CountOperation(const cvub::GenericModule &module,
                      const std::string &name) {
  return static_cast<size_t>(std::count_if(
      module.operations.begin(), module.operations.end(),
      [&](const cvub::GenericOperation &operation) {
        return operation.name == name;
      }));
}

void TestVectorTiling() {
  cvub::GenericModule module = cvub::ParseGenericIR(
      "cvpipeline_ub_model_cpp/tests/fixtures/tile_vector_loop.mlir", false);
  const cvub::StageResult result =
      cvub::RunTileCubeVectorLoop(std::move(module), 2, 2);
  Check(result.precision == cvub::Precision::Exact,
        "default vector tiling must be exact");
  Check(FindOperation(result.module, "hivm.hir.vadd").resultTypes ==
            std::vector<std::string>{"tensor<1x64xf16>"},
        "vector tiling must shrink the UB tile");
  Check(FindOperation(result.module, "memref.alloc").resultTypes ==
            std::vector<std::string>{"memref<1x64xf16>"},
        "vector tiling must shrink the physical local allocation");
}

void TestSubsetHoisting() {
  cvub::GenericModule module = cvub::ParseGenericIR(
      "cvpipeline_ub_model_cpp/tests/fixtures/subset_hoisting_lifetime.mlir",
      false);
  const cvub::StageResult result =
      cvub::RunLoopInvariantSubsetHoisting(std::move(module), true);
  Check(result.precision == cvub::Precision::Exact,
        "proven static subset pair must be exact");
  const cvub::GenericOperation &loop = FindCase(result.module, "subset_loop");
  const cvub::GenericOperation &extract =
      FindCase(result.module, "subset_extract");
  const cvub::GenericOperation &insert =
      FindCase(result.module, "subset_insert");
  Check(extract.blockId == loop.blockId && extract.ordinal < loop.ordinal,
        "subset extraction must move before the loop");
  Check(insert.blockId == loop.blockId && loop.ordinal < insert.ordinal,
        "subset insertion must move after the loop");
  Check(loop.results.size() == 2,
        "subset hoisting must carry an independent subset result");
}

void TestSubsetMismatchBlocksTransactionally() {
  cvub::GenericModule module = cvub::ParseGenericIR(
      "cvpipeline_ub_model_cpp/tests/fixtures/subset_hoisting_lifetime.mlir",
      false);
  cvub::GenericOperation &insert = const_cast<cvub::GenericOperation &>(
      FindCase(module, "subset_insert"));
  insert.properties = cvub::SetDictionaryValue(
      insert.properties, "static_sizes", "array<i64: 3>");
  const std::string before = cvub::SerializeGenericModule(module);
  const cvub::StageResult result =
      cvub::RunLoopInvariantSubsetHoisting(std::move(module), true);
  Check(result.precision == cvub::Precision::Incomplete,
        "mismatched subset descriptors must block");
  Check(cvub::SerializeGenericModule(result.module) == before,
        "subset blocker must not leave a partial rewrite");
}

void TestStrictInlineRejectsUnprovedCall() {
  cvub::GenericModule module = cvub::ParseGenericIR(
      "cvpipeline_ub_model_cpp/tests/fixtures/scope_tensor_empty_review.mlir",
      false);
  const cvub::StageResult result = cvub::RunStrictInlineScope(std::move(module));
  Check(result.precision == cvub::Precision::Incomplete,
        "unproved public call must block exact inline modeling");
  Check(std::any_of(result.diagnostics.begin(), result.diagnostics.end(),
                    [](const cvub::PostCVPipelineDiagnostic &diagnostic) {
                      return diagnostic.reason.find("private or internal") !=
                             std::string::npos;
                    }),
        "inline blocker must explain the callee visibility requirement");
}

void TestValidatorRejectsCrossFunctionValue() {
  cvub::GenericModule module = cvub::ParseGenericIR(
      "cvpipeline_ub_model_cpp/tests/fixtures/two_aiv_functions.mlir", false);
  std::vector<const cvub::GenericOperation *> functions;
  for (const cvub::GenericOperation &operation : module.operations)
    if (operation.name == "func.func")
      functions.push_back(&operation);
  Check(functions.size() == 2, "fixture must contain two functions");
  const auto body = [&](const cvub::GenericOperation &function) -> int {
    return module.regions.at(static_cast<size_t>(function.regions.front()))
        .blocks.front();
  };
  int foreignIndex = -1;
  for (int operationId :
       module.blocks.at(static_cast<size_t>(body(*functions[0]))).operations) {
    const cvub::GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    if (operation.name == "arith.constant" &&
        operation.resultTypes == std::vector<std::string>{"index"})
      foreignIndex = operation.results.front();
  }
  Check(foreignIndex >= 0, "first function must define an index constant");
  for (int operationId :
       module.blocks.at(static_cast<size_t>(body(*functions[1]))).operations) {
    cvub::GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    if (operation.name == "memref.store")
      operation.operands.back() = foreignIndex;
  }
  bool rejected = false;
  try {
    cvub::ValidateGenericModule(module);
  } catch (const std::runtime_error &error) {
    rejected = std::string(error.what()).find("cross-function") !=
               std::string::npos;
  }
  Check(rejected, "validator must reject cross-function SSA references");
}

void TestAscend950TightlyCoupledBufferBlocks() {
  cvub::GenericModule module = cvub::ParseGenericIR(
      "cvpipeline_ub_model_cpp/tests/fixtures/two_aiv_functions.mlir", false);
  module.operations.front().attributes =
      "hacc.target = #hacc.target<\"Ascend950PR_9579\">";
  const cvub::StageResult result =
      cvub::GuardTightlyCoupledBufferPasses(std::move(module));
  Check(result.precision == cvub::Precision::Incomplete,
        "Ascend950 UB allocs must block until tightly-coupled anchors are "
        "modeled");
  Check(!result.diagnostics.empty() &&
            result.diagnostics.front().pipelineStage.find(
                "MarkTightlyCoupledBuffer") != std::string::npos,
        "Ascend950 blocker must name the missing real passes");
}

void TestOTFExtraUsersBlockBeforePlanning() {
  cvub::GenericModule module = cvub::ParseGenericIR(
      "cvpipeline_ub_model_cpp/tests/fixtures/unaligned_concat_store.mlir",
      false);
  module = cvub::ExtractFunctionModule(module, "extra_users_concat_store_aiv");
  bool rejected = false;
  try {
    (void)cvub::RunHIVMInlineOTFLoadStore(std::move(module));
  } catch (const std::runtime_error &error) {
    rejected = std::string(error.what()).find("users beyond the store") !=
               std::string::npos;
  }
  Check(rejected, "OTF concat with extra users must fail closed");
}

void TestOTFSubbyteIsAlignedNoOp() {
  cvub::GenericModule module = cvub::ParseGenericIR(
      "cvpipeline_ub_model_cpp/tests/fixtures/otf_subbyte_concat_store.mlir",
      false);
  const std::string before = cvub::SerializeGenericModule(module);
  module = cvub::RunHIVMInlineOTFLoadStore(std::move(module));
  Check(cvub::SerializeGenericModule(module) == before,
        "sub-byte concat must follow the real pass's aligned no-op path");
}

void TestTask7SuccessfulTilingChangesUBSemantics() {
  cvub::GenericModule module = cvub::ParseGenericIR(
      "cvpipeline_ub_model_cpp/tests/fixtures/subblock_bind_success.mlir",
      false);
  cvub::ApplyOperationSemanticsToAll(module.operations);
  module = cvub::RunTileAndBindSubBlock(std::move(module));
  cvub::ValidateGenericModule(module);
  Check(CountOperation(module, "scf.for") > 0 &&
            CountOperation(module, "memref.subview") > 0,
        "successful Task7 tiling must materialize the sub-block loop and "
        "destination slice");
  Check(FindOperation(module, "hivm.hir.vadd").resultTypes.front() !=
            "tensor<16x16xf16>",
        "successful Task7 tiling must shrink the vector UB tile");
}

void TestTask7EarlyCanonicalizationIsNotSkipped() {
  cvub::GenericModule module = cvub::ParseGenericIR(
      "cvpipeline_ub_model_cpp/tests/fixtures/subblock_early_alloc_to_tensor.mlir",
      false);
  for (cvub::GenericOperation &operation : module.operations)
    if (operation.name == "test.consume") {
      operation.name = "annotation.mark";
      operation.attributes = "{effects = [\"read\"]}";
    }
  const size_t allocsBefore = CountOperation(module, "memref.alloc");
  module = cvub::RunTileAndBindSubBlock(std::move(module));
  cvub::ValidateGenericModule(module);
  Check(CountOperation(module, "memref.alloc") < allocsBefore,
        "Task7 early alloc-to-tensor canonicalization must affect UB "
        "ownership before Exact planning");
}

void TestGenericRewriterAssignsRegionOrdinals() {
  cvub::GenericModule module = cvub::ParseGenericIR(
      "cvpipeline_ub_model_cpp/tests/fixtures/two_aiv_functions.mlir", false);
  const cvub::GenericOperation &function = FindOperation(module, "func.func");
  const int block = module.regions
                        .at(static_cast<size_t>(function.regions.front()))
                        .blocks.front();
  cvub::GenericRewriter rewriter(module);
  const int branch = rewriter.createOperation(
      function.id, function.regions.front(), block, "scf.if", {});
  const int firstRegion = rewriter.createRegion(branch);
  const int firstBlock = rewriter.createBlock(firstRegion, {});
  const int firstYield = rewriter.createOperation(
      branch, firstRegion, firstBlock, "scf.yield", {});
  rewriter.appendToBlock(firstBlock, firstYield);
  const int secondRegion = rewriter.createRegion(branch);
  const int secondBlock = rewriter.createBlock(secondRegion, {});
  const int secondYield = rewriter.createOperation(
      branch, secondRegion, secondBlock, "scf.yield", {});
  rewriter.appendToBlock(secondBlock, secondYield);
  rewriter.insertToBlock(block, 0, branch);
  cvub::ValidateGenericModule(module);
  Check(module.regions.at(static_cast<size_t>(firstRegion)).ordinal == 0 &&
            module.regions.at(static_cast<size_t>(secondRegion)).ordinal == 1,
        "GenericRewriter must preserve multi-region ownership ordinals");
}

void TestMarkMultiBufferExplicitMarksAndFailFast() {
  cvub::GenericModule module = cvub::ParseGenericIR(
      "cvpipeline_ub_model_cpp/tests/fixtures/two_aiv_functions.mlir", false);
  const cvub::GenericOperation &function = FindOperation(module, "func.func");
  const cvub::GenericOperation &allocation =
      FindOperation(module, "memref.alloc");
  const int functionId = function.id;
  const int functionRegion = function.regions.front();
  const int block = allocation.blockId;
  const int allocationResult = allocation.results.front();
  const std::string allocationType = allocation.resultTypes.front();
  cvub::GenericRewriter rewriter(module);
  const auto appendMark = [&](const std::string &attributes) {
    const int mark = rewriter.createOperation(
        functionId, functionRegion, block, "annotation.mark", {},
        {allocationResult}, {allocationType}, "", attributes);
    rewriter.appendToBlock(block, mark);
    return mark;
  };
  const int firstMark =
      appendMark("{hivm.multi_buffer = 2 : i32}");
  const int secondMark = appendMark(
      "{hivm.multi_buffer = 3 : i32, hivm.preload_local_buffer = 1 : i32}");
  const int singleBufferMark =
      appendMark("{hivm.multi_buffer = 1 : i32}");

  cvub::AfterInlineLoadCopyState state;
  state.afterAllocExtraBuffer.postBufferization.bufferized.logicalModule =
      module;
  state.afterAllocExtraBuffer.postBufferization.bufferized.accesses = {
      {firstMark, 0, "local:0"}, {secondMark, 0, "local:0"},
      {singleBufferMark, 0, "local:0"}};
  state.buffers.push_back({"%base_0", "base:0", "memref.alloc",
                           allocationType, cvub::AddressSpace::UB, 65536,
                           false, {2048}});

  const cvub::MarkMultiBufferResult result =
      cvub::ModelMarkMultiBuffer(state, {});
  Check(result.buffer2MultiNum.at("base:0") == 3,
        "the last non-one explicit multi-buffer mark must win");
  Check(result.preloadLocalBuffers.count("base:0") == 1,
        "explicit preload marks must be preserved");

  state.afterAllocExtraBuffer.postBufferization.bufferized.accesses.clear();
  bool blocked = false;
  try {
    (void)cvub::ModelMarkMultiBuffer(state, {});
  } catch (const std::runtime_error &error) {
    blocked = std::string(error.what()).find(
                  "explicit multi-buffer mark has no modeled buffer") !=
              std::string::npos;
  }
  Check(blocked, "unresolved explicit multi-buffer marks must fail closed");
}

void TestSinkOpUseMultiplicityAndGreedyOrder() {
  cvub::GenericModule module = cvub::ParseGenericIR(
      "cvpipeline_ub_model_cpp/tests/fixtures/sink_op_use_order.mlir", false);
  module = cvub::RunSinkOpToConsumerInLoop(std::move(module));

  const cvub::GenericOperation &consumer = FindOperation(module, "hivm.hir.vadd");
  Check(consumer.operands[0] != consumer.operands[1],
        "two operands on one user must receive distinct sunk clones");

  const cvub::GenericOperation &loop = FindOperation(module, "scf.for");
  const int loopBlock =
      module.regions.at(static_cast<size_t>(loop.regions.front())).blocks.front();
  std::vector<std::string> fillOrder;
  for (int operationId :
       module.blocks.at(static_cast<size_t>(loopBlock)).operations) {
    const cvub::GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    if (operation.name != "hivm.hir.vbrc")
      continue;
    fillOrder.push_back(operation.attributes.find("second_fill") !=
                                std::string::npos
                            ? "second"
                            : "first");
  }
  Check(fillOrder == std::vector<std::string>({"second", "first", "first"}),
        "sink pass must reproduce MLIR use-list and greedy worklist order");
  Check(CountOperation(module, "hivm.hir.vbrc") == 3,
        "two uses on one consumer must create one clone per OpOperand");
}

} // namespace

int main() {
  TestVectorTiling();
  std::cout << "[PASS] yy pipeline retains default vector tiling semantics\n";
  TestSubsetHoisting();
  std::cout << "[PASS] yy pipeline retains subset-hoisting semantics\n";
  TestSubsetMismatchBlocksTransactionally();
  std::cout << "[PASS] subset mismatch blocks transactionally\n";
  TestStrictInlineRejectsUnprovedCall();
  std::cout << "[PASS] strict inline rejects unproved calls\n";
  TestValidatorRejectsCrossFunctionValue();
  std::cout << "[PASS] generic verifier rejects cross-function SSA\n";
  TestAscend950TightlyCoupledBufferBlocks();
  std::cout << "[PASS] Ascend950 tightly-coupled buffers fail closed\n";
  TestOTFExtraUsersBlockBeforePlanning();
  std::cout << "[PASS] OTF concat extra users fail closed\n";
  TestOTFSubbyteIsAlignedNoOp();
  std::cout << "[PASS] OTF sub-byte concat remains an aligned no-op\n";
  TestTask7SuccessfulTilingChangesUBSemantics();
  std::cout << "[PASS] Task7 successful tiling changes UB semantics\n";
  TestTask7EarlyCanonicalizationIsNotSkipped();
  std::cout << "[PASS] Task7 early canonicalization is modeled\n";
  TestGenericRewriterAssignsRegionOrdinals();
  std::cout << "[PASS] GenericRewriter assigns region ordinals\n";
  TestMarkMultiBufferExplicitMarksAndFailFast();
  std::cout << "[PASS] MarkMultiBuffer preserves explicit order and fails closed\n";
  TestSinkOpUseMultiplicityAndGreedyOrder();
  std::cout << "[PASS] SinkOpToConsumer preserves MLIR use and worklist order\n";
  return 0;
}
