#include "../src/passes/loop_invariant_subset_hoisting.hpp"
#include "../src/passes/inline_scope_strict.hpp"
#include "../src/passes/cvpipelining/cvpipelining.hpp"
#include "../src/passes/hivm_inline_otf_load_store.hpp"
#include "../src/passes/infer_hivm_data_layout.hpp"
#include "../src/passes/infer_and_set_buffer_size.hpp"
#include "../src/passes/inject_block_sync.hpp"
#include "../src/passes/tile_cube_vector_loop.hpp"
#include "../src/passes/tile_and_bind_sub_block.hpp"
#include "../src/passes/tightly_coupled_buffer_guard.hpp"
#include "../src/passes/mark_multi_buffer.hpp"
#include "../src/passes/plan_memory/operation_index.hpp"
#include "../src/passes/sink_op_to_consumer_in_loop.hpp"
#include "../src/passes/split_mix_kernel.hpp"
#include "../src/pipeline/after_alloc_extra_buffer.hpp"

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
      "ub_overflow_model_cpp/tests/fixtures/tile_vector_loop.mlir", false);
  const cvub::StageResult result =
      cvub::RunTileCubeVectorLoop(std::move(module), 2, 2);
  if (result.precision != cvub::Precision::Exact)
    for (const auto &diagnostic : result.diagnostics)
      std::cerr << "TileCubeVectorLoop diagnostic: " << diagnostic.reason
                << "\n";
  Check(result.precision == cvub::Precision::Exact,
        "default vector tiling must be exact");
  Check(FindOperation(result.module, "hivm.hir.vadd").resultTypes ==
            std::vector<std::string>{"tensor<1x64xf16>"},
        "vector tiling must shrink the UB tile");
  Check(FindOperation(result.module, "memref.alloc").resultTypes ==
            std::vector<std::string>{"memref<1x64xf16>"},
        "vector tiling must shrink the physical local allocation");
}

void TestVectorTilingIgnoresScalarAxisOperands() {
  cvub::GenericModule module = cvub::ParseGenericIR(
      "ub_overflow_model_cpp/tests/fixtures/"
      "tile_vector_loop_scalar_operand.mlir",
      false);
  const cvub::StageResult result =
      cvub::RunTileCubeVectorLoop(std::move(module), 2, 2);
  Check(result.precision == cvub::Precision::Exact,
        "scalar HIVM operands must not reject vector tiling");
  Check(FindOperation(result.module, "hivm.hir.vadd").resultTypes ==
            std::vector<std::string>{"tensor<1x64xi32>"},
        "vector tiling must propagate only the shaped operand axis");
  Check(CountOperation(result.module, "arith.index_cast") == 1,
        "the scalar offset producer must remain outside axis propagation");
}

void TestInferBufferSizeComposesCVPipelineTripCount() {
  cvub::GenericModule module = cvub::ParseGenericIRText(R"mlir(
"builtin.module"() ({
  "func.func"() <{function_type = (index, index, index) -> (), sym_name = "kernel"}> ({
  ^bb0(%upper: index, %iv: index, %step: index):
    %c4 = "arith.constant"() {value = 4 : index} : () -> index
    %remaining = "affine.apply"(%upper, %iv, %step) <{map = affine_map<(d0, d1)[s0] -> ((d0 - d1) ceildiv s0)>}> : (index, index, index) -> index
    %capped = "arith.minui"(%remaining, %c4) : (index, index) -> index
    "func.return"() : () -> ()
  }) {hacc.function_kind = #hacc.function_kind<DEVICE>} : () -> ()
  }) : () -> ()
)mlir", false);
  module = cvub::RunInferAndSetBufferSizePipeline(std::move(module));
  Check(CountOperation(module, "arith.minui") == 0,
        "InferAndSetBufferSize must lower the CVPipelining cap");
  Check(CountOperation(module, "affine.apply") == 0,
        "the composed CVPipelining bound must not retain its affine.apply");
  Check(CountOperation(module, "affine.min") == 1,
        "the composed CVPipelining bound must become one affine.min");
}

void TestExistingAffineMapFoldsConstantOperands() {
  cvub::GenericModule module = cvub::ParseGenericIRText(R"mlir(
"builtin.module"() ({
  "func.func"() <{function_type = (index) -> (), sym_name = "kernel"}> ({
  ^bb0(%upper: index):
    %c32 = "arith.constant"() <{value = 32 : index}> : () -> index
    %bound = "affine.min"(%upper, %c32) <{map = affine_map<()[s0, s1] -> (4, s0 ceildiv s1)>}> : (index, index) -> index
    "test.consume"(%bound) : (index) -> ()
    "func.return"() : () -> ()
  }) {hacc.function_kind = #hacc.function_kind<DEVICE>} : () -> ()
}) : () -> ()
)mlir", false);
  cvub::FoldExistingAffineConstantOperands(module);
  const cvub::GenericOperation &bound = FindOperation(module, "affine.min");
  Check(bound.operands.size() == 1,
        "existing affine.min must fold an index constant into its map");
  Check(bound.properties.find("c(32)") != std::string::npos,
        "existing affine.min must preserve the folded constant expression");
}

void TestSubsetHoisting() {
  cvub::GenericModule module = cvub::ParseGenericIR(
      "ub_overflow_model_cpp/tests/fixtures/subset_hoisting_lifetime.mlir",
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
      "ub_overflow_model_cpp/tests/fixtures/subset_hoisting_lifetime.mlir",
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
      "ub_overflow_model_cpp/tests/fixtures/scope_tensor_empty_review.mlir",
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
      "ub_overflow_model_cpp/tests/fixtures/two_aiv_functions.mlir", false);
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

cvub::BufferizedSemanticIR BuildInferLayoutTestIR(const char *coreType,
                                                  bool storeIntoMmadB) {
  std::string text = R"mlir(
"builtin.module"() ({
  "func.func"() <{function_type = () -> (), sym_name = "layout_test"}> ({
  ^bb0:
    %zero = "arith.constant"() <{value = 0.0 : f32}> : () -> f32
    %idx = "arith.constant"() <{value = 0 : index}> : () -> index
    %cond = "arith.constant"() <{value = true}> : () -> i1
    %a = "memref.alloc"() : () -> memref<16x16xf32, #hivm.address_space<cbuf>>
    %b = "memref.alloc"() : () -> memref<16x16xf32, #hivm.address_space<cbuf>>
    %other = "memref.alloc"() : () -> memref<16x16xf32, #hivm.address_space<cbuf>>
    %c = "memref.alloc"() : () -> memref<16x16xf32, #hivm.address_space<cc>>
    "memref.store"(%zero, DESTINATION, %idx, %idx) : (f32, memref<16x16xf32, #hivm.address_space<cbuf>>, index, index) -> ()
    "hivm.hir.mmadL1"(%a, %b, %cond, %idx, %idx, %idx, %c) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> : (memref<16x16xf32, #hivm.address_space<cbuf>>, memref<16x16xf32, #hivm.address_space<cbuf>>, i1, index, index, index, memref<16x16xf32, #hivm.address_space<cc>>) -> ()
    "func.return"() : () -> ()
  }) {hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = CORE_TYPE} : () -> ()
}) : () -> ()
)mlir";
  const std::string destination = storeIntoMmadB ? "%b" : "%other";
  text.replace(text.find("DESTINATION"), std::string("DESTINATION").size(),
               destination);
  text.replace(text.find("CORE_TYPE"), std::string("CORE_TYPE").size(),
               coreType);
  cvub::GenericModule module = cvub::ParseGenericIRText(text, false);
  cvub::ApplyOperationSemanticsToAll(module.operations);
  return cvub::BuildBufferizedSemanticIR(
      module, cvub::RunOneShotBufferize(module));
}

void TestInferLayoutRejectsRemappedScalarStore() {
  cvub::BufferizedSemanticIR module = BuildInferLayoutTestIR(
      "#hivm.func_core_type<AIC>", /*storeIntoMmadB=*/true);
  bool rejected = false;
  try {
    cvub::ValidateInferHIVMDataLayoutScalarStores(module);
  } catch (const std::runtime_error &error) {
    rejected = std::string(error.what()).find(
                   "store index operand count not equal to memref rank") !=
               std::string::npos;
  }
  Check(rejected,
        "InferHIVMDataLayout must expose the remapped rank-4 store failure");
}

void TestInferLayoutStoreValidationScope() {
  cvub::ValidateInferHIVMDataLayoutScalarStores(BuildInferLayoutTestIR(
      "#hivm.func_core_type<AIV>", /*storeIntoMmadB=*/true));
  cvub::ValidateInferHIVMDataLayoutScalarStores(BuildInferLayoutTestIR(
      "#hivm.func_core_type<AIC>", /*storeIntoMmadB=*/false));
}

void TestSplitMixDeadRegionIgnoresInternalYieldUses() {
  cvub::GenericModule module = cvub::ParseGenericIRText(R"mlir(
"builtin.module"() ({
  "func.func"() <{function_type = () -> (), sym_name = "dead_region"}> ({
    %unused = "scf.execute_region"() ({
      %value = "arith.constant"() <{value = 1 : i32}> : () -> i32
      "scf.yield"(%value) : (i32) -> ()
    }) : () -> i32
    "func.return"() : () -> ()
  }) {hacc.function_kind = #hacc.function_kind<DEVICE>} : () -> ()
}) : () -> ()
)mlir", false);
  cvub::PipelineAnalysisContext uses(module, cvub::kGenericAnalysisUsers);
  std::set<int> active;
  for (const cvub::GenericBlock &block : module.blocks)
    active.insert(block.operations.begin(), block.operations.end());
  const cvub::GenericOperation &region =
      FindOperation(module, "scf.execute_region");
  Check(cvub::IsSplitMixTriviallyDead(module, region, active, uses),
        "SplitMix deadness must ignore uses nested under the erased region");
}

void TestSplitMixCubeFoldsEmptyInsertSlice() {
  cvub::GenericModule module = cvub::ParseGenericIRText(R"mlir(
"builtin.module"() ({
  "func.func"() <{function_type = () -> (), sym_name = "mix_kernel"}> ({
    %source = "tensor.empty"() : () -> tensor<4xf32>
    %destination = "tensor.empty"() : () -> tensor<4xf32>
    %inserted = "tensor.insert_slice"(%source, %destination) : (tensor<4xf32>, tensor<4xf32>) -> tensor<4xf32>
    "test.consume"(%inserted) : (tensor<4xf32>) -> ()
    "func.return"() : () -> ()
  }) {hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<MIX>} : () -> ()
}) : () -> ()
)mlir", false);
  module = cvub::RunSplitMixKernelProjection(
      std::move(module), cvub::SplitMixCoreType::Cube);
  Check(CountOperation(module, "tensor.insert_slice") == 0,
        "SplitMix Cube postprocessing must fold empty insert_slice");
  std::vector<const cvub::GenericOperation *> emptyOperations;
  for (const cvub::GenericOperation &operation : module.operations)
    if (operation.name == "tensor.empty")
      emptyOperations.push_back(&operation);
  const cvub::GenericOperation &consumer =
      FindOperation(module, "test.consume");
  Check(emptyOperations.size() == 2 &&
            emptyOperations.back()->results.size() == 1 &&
            consumer.operands.size() == 1 && consumer.operands.front() ==
                emptyOperations.back()->results.front(),
        "the folded insert_slice must forward its destination");
}

void TestAscend950TightlyCoupledBufferBlocks() {
  cvub::GenericModule module = cvub::ParseGenericIR(
      "ub_overflow_model_cpp/tests/fixtures/two_aiv_functions.mlir", false);
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

void TestA3TightlyCoupledPassesAreUBNoOp() {
  cvub::GenericModule module = cvub::ParseGenericIR(
      "ub_overflow_model_cpp/tests/fixtures/two_aiv_functions.mlir", false);
  module.operations.front().attributes =
      "hacc.target = #hacc.target<\"Ascend910_9382\">";
  const std::string before = cvub::SerializeGenericModule(module);
  const cvub::StageResult result =
      cvub::GuardTightlyCoupledBufferPasses(std::move(module));
  Check(result.precision == cvub::Precision::Exact,
        "A2/A3 tightly-coupled passes must be an exact UB no-op");
  Check(cvub::SerializeGenericModule(result.module) == before,
        "A2/A3 tightly-coupled no-op must not rewrite the module");
}

void TestRawAffineDynamicExtentUpperBound() {
  const cvub::GenericModule module = cvub::ParseGenericIR(
      "ub_overflow_model_cpp/tests/fixtures/constantize_raw_affine_bound.mlir",
      false);
  const cvub::GenericModuleAnalysisIndexes analysis(
      module, cvub::kGenericAnalysisDefinitions);
  const cvub::GenericOperation *extent = nullptr;
  for (const cvub::GenericOperation &operation : module.operations)
    if (operation.name == "affine.apply" && operation.operands.size() == 2)
      extent = &operation;
  Check(extent && !extent->results.empty(),
        "raw affine extent fixture is malformed");
  const std::optional<int64_t> bound = cvub::ConstantizeBufferSizeUpperBound(
      extent->results.front(), module, analysis);
  Check(bound && *bound == 32,
        "raw BiSheng affine chain must have the closed upper bound 32");
}

void TestRawInlineAffineDynamicExtentUpperBound() {
  const cvub::GenericModule module = cvub::ParseGenericIR(
      "ub_overflow_model_cpp/tests/fixtures/constantize_raw_inline_affine_bound.mlir",
      false);
  const cvub::GenericModuleAnalysisIndexes analysis(
      module, cvub::kGenericAnalysisDefinitions);
  const cvub::GenericOperation *extent = nullptr;
  for (const cvub::GenericOperation &operation : module.operations)
    if (operation.name == "affine.apply" && operation.operands.size() == 2)
      extent = &operation;
  Check(extent && !extent->results.empty(),
        "raw inline affine extent fixture is malformed");
  const std::optional<int64_t> bound = cvub::ConstantizeBufferSizeUpperBound(
      extent->results.front(), module, analysis);
  Check(bound && *bound == 32,
        "raw BiSheng min(base + tile, other) map must preserve the tile bound");
}

void TestOTFExtraUsersBlockBeforePlanning() {
  cvub::GenericModule module = cvub::ParseGenericIR(
      "ub_overflow_model_cpp/tests/fixtures/unaligned_concat_store.mlir",
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
      "ub_overflow_model_cpp/tests/fixtures/otf_subbyte_concat_store.mlir",
      false);
  const std::string before = cvub::SerializeGenericModule(module);
  module = cvub::RunHIVMInlineOTFLoadStore(std::move(module));
  Check(cvub::SerializeGenericModule(module) == before,
        "sub-byte concat must follow the real pass's aligned no-op path");
}

void TestTask7SuccessfulTilingChangesUBSemantics() {
  cvub::GenericModule module = cvub::ParseGenericIR(
      "ub_overflow_model_cpp/tests/fixtures/subblock_bind_success.mlir",
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
  Check(CountOperation(module, "scf.if") == 1 &&
            CountOperation(module, "hivm.hir.get_sub_block_idx") == 1,
        "successful Task7 tiling must limit each unsliced store to sub-block "
        "0");
  size_t tiledStores = 0;
  size_t guardedStores = 0;
  for (const cvub::GenericOperation &operation : module.operations) {
    if (operation.name != "hivm.hir.store")
      continue;
    if (cvub::HasSplitMixDictionaryEntry(operation.attributes, "tiled_op"))
      ++tiledStores;
    if (operation.parentId >= 0 &&
        module.operations.at(static_cast<size_t>(operation.parentId)).name ==
            "scf.if")
      ++guardedStores;
  }
  Check(tiledStores == 1 && guardedStores == 1,
        "successful Task7 tiling must not guard stores already marked "
        "tiled_op");
}

void TestTask7DisabledTilingStillLimitsSubBlock() {
  cvub::GenericModule module = cvub::ParseGenericIR(
      "ub_overflow_model_cpp/tests/fixtures/subblock_bind_success.mlir",
      false);
  cvub::ApplyOperationSemanticsToAll(module.operations);
  module = cvub::RunTileAndBindSubBlock(std::move(module), nullptr,
                                        /*enableTile=*/false);
  cvub::ValidateGenericModule(module);
  Check(FindOperation(module, "hivm.hir.vadd").resultTypes.front() ==
            "tensor<16x16xf16>",
        "disabled Task7 tiling must preserve the untiled UB shape");
  Check(CountOperation(module, "hivm.hir.get_sub_block_idx") == 2 &&
            CountOperation(module, "scf.if") == 2,
        "disabled Task7 tiling must still restrict stores to sub-block 0");
  for (const cvub::GenericOperation &operation : module.operations)
    Check(!cvub::HasSplitMixDictionaryEntry(operation.attributes, "tiled_op"),
          "disabled Task7 tiling must not mark operations as tiled");
}

void TestTask7UnresolvedMarkedSliceRollsBack() {
  cvub::GenericModule module = cvub::ParseGenericIR(
      "ub_overflow_model_cpp/tests/fixtures/"
      "subblock_bind_unmodeled_static.mlir",
      false);
  cvub::ApplyOperationSemanticsToAll(module.operations);
  module = cvub::RunTileAndBindSubBlock(std::move(module));
  cvub::ValidateGenericModule(module);
  Check(FindOperation(module, "hivm.hir.vsub").resultTypes.front() ==
            "tensor<16x16xf16>" &&
            CountOperation(module, "hivm.hir.get_sub_block_idx") == 1 &&
            CountOperation(module, "scf.if") == 1,
        "an unresolved marked slice on an unsupported source must make the "
        "strict verifier roll the TileAndBind candidate back");
}

void TestTask7EarlyCanonicalizationIsNotSkipped() {
  cvub::GenericModule module = cvub::ParseGenericIR(
      "ub_overflow_model_cpp/tests/fixtures/subblock_early_alloc_to_tensor.mlir",
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

void TestTask7BubbleUpReachesGreedyFixedPoint() {
  cvub::GenericModule module = cvub::ParseGenericIR(
      "ub_overflow_model_cpp/tests/fixtures/subblock_bubble_fixed_point.mlir",
      false);
  cvub::ApplyOperationSemanticsToAll(module.operations);
  module = cvub::RunTileAndBindSubBlock(std::move(module));
  cvub::ValidateGenericModule(module);
  size_t tiledLoops = 0;
  for (const cvub::GenericOperation &operation : module.operations) {
    if (operation.blockId < 0 || operation.name != "scf.for" ||
        operation.results.size() != 1 || operation.operands.size() < 4)
      continue;
    ++tiledLoops;
    Check(operation.resultTypes.front() == "tensor<32xf32>" &&
              operation.operandTypes.back() == "tensor<32xf32>",
          "greedy bubble-up retry must narrow both the loop result and init");
  }
  Check(tiledLoops == 1,
        "fixed-point fixture must retain exactly one tensor iter-arg loop");
}

void TestTask7NestedSubviewPreservesOtherParentUsers() {
  cvub::GenericModule module = cvub::ParseGenericIR(
      "ub_overflow_model_cpp/tests/fixtures/"
      "subblock_nested_subview_multi_user.mlir",
      false);
  cvub::ApplyOperationSemanticsToAll(module.operations);
  const cvub::GenericOperation &function = FindOperation(module, "func.func");
  int parentId = -1;
  int childId = -1;
  for (const cvub::GenericOperation &operation : module.operations) {
    if (operation.name != "memref.subview" || operation.results.size() != 1)
      continue;
    if (operation.resultTypes.front().find("memref<8x128x") == 0)
      parentId = operation.id;
    if (operation.resultTypes.front().find("memref<4x128x") == 0)
      childId = operation.id;
  }
  Check(parentId >= 0 && childId >= 0,
        "nested subview fixture is malformed");
  const int parentValue =
      module.operations.at(static_cast<size_t>(parentId)).results.front();

  cvub::RunTileAndBindSubviewFromTiling(module, function.id);
  cvub::ApplyOperationSemanticsToAll(module.operations);
  cvub::ValidateGenericModule(module);

  const cvub::GenericOperation &oldParent =
      module.operations.at(static_cast<size_t>(parentId));
  const cvub::GenericOperation &child =
      module.operations.at(static_cast<size_t>(childId));
  Check(oldParent.resultTypes.front().find("memref<8x128x") == 0,
        "a multiply-used tiling parent must retain its original view type");
  Check(!child.operands.empty() && child.operands.front() != parentValue,
        "the marked child must be rewired to a fresh sub-block parent view");
  const std::map<int, const cvub::GenericOperation *> definitions =
      cvub::DefiningOperations(module);
  const auto newParent = definitions.find(child.operands.front());
  Check(newParent != definitions.end() &&
            newParent->second->name == "memref.subview" &&
            newParent->second->resultTypes.front().find("memref<32x128x") ==
                0 &&
            child.resultTypes.front().find("memref<4x128x") == 0,
        "nested view bubble-up must reproduce BiSheng's 64 -> 32 -> 4 tiling");
}

void TestGenericRewriterAssignsRegionOrdinals() {
  cvub::GenericModule module = cvub::ParseGenericIR(
      "ub_overflow_model_cpp/tests/fixtures/two_aiv_functions.mlir", false);
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

void TestGenericRewriterIncrementalReplaceAllUses() {
  cvub::GenericModule module = cvub::ParseGenericIR(
      "ub_overflow_model_cpp/tests/fixtures/two_aiv_functions.mlir", false);
  const cvub::GenericOperation &function = FindOperation(module, "func.func");
  const int functionId = function.id;
  const int region = function.regions.front();
  const int block =
      module.regions.at(static_cast<size_t>(region)).blocks.front();
  cvub::PipelineAnalysisContext analysis(module);
  cvub::GenericRewriter rewriter(module, &analysis);
  const int from = rewriter.newValue();
  const int to = rewriter.newValue();
  const int user = rewriter.createOperation(
      functionId, region, block, "test.incremental_use", {}, {from, from},
      {"index", "index"});
  cvub::GenericOperation &userOperation =
      module.operations.at(static_cast<size_t>(user));
  userOperation.dpsInputs = {from};
  userOperation.dpsInits = {from};
  rewriter.appendToBlock(block, user);
  Check(analysis.useCount(from) == 2,
        "incremental use-list must observe created operands");
  int existingValue = -1;
  for (const cvub::GenericOperation &operation : module.operations)
    if (!operation.results.empty()) {
      existingValue = operation.results.front();
      break;
    }
  Check(existingValue >= 0, "revision test requires an existing definition");
  (void)analysis.definingOperationId(existingValue);
  const uint64_t buildsBeforeReplace =
      analysis.diagnostics().fullIndexBuilds;
  const uint64_t defUseRevisionBefore = analysis.revisions().defUse;
  rewriter.replaceAllUses(from, to);
  Check(analysis.useCount(from) == 0 && analysis.useCount(to) == 2,
        "replaceAllUses must update the incremental use-list");
  Check(userOperation.operands == std::vector<int>({to, to}) &&
            userOperation.dpsInputs == std::vector<int>({to}) &&
            userOperation.dpsInits == std::vector<int>({to}),
        "replaceAllUses must update operands and cached DPS projections");
  (void)analysis.definingOperationId(existingValue);
  Check(analysis.diagnostics().fullIndexBuilds == buildsBeforeReplace,
        "operand replacement must not rebuild definition/type indexes");
  Check(analysis.revisions().defUse > defUseRevisionBefore &&
            analysis.diagnostics().incrementallyUpdatedUses == 2,
        "operand replacement must advance only the incremental def-use path");
  rewriter.removeFromBlock(block, user);
  Check(analysis.useCount(to) == 0,
        "detaching an operation must remove its operands from the use-list");
  rewriter.appendToBlock(block, user);
  Check(analysis.useCount(to) == 2,
        "reattaching an operation must restore its operands in the use-list");
}

void TestGenericRewriterBatchTombstones() {
  cvub::GenericModule module = cvub::ParseGenericIR(
      "ub_overflow_model_cpp/tests/fixtures/batch_tombstones.mlir", false);
  cvub::PipelineAnalysisContext analysis(module, cvub::kGenericAnalysisUsers);
  std::vector<int> additions;
  for (const cvub::GenericOperation &operation : module.operations)
    if (operation.name == "arith.addi")
      additions.push_back(operation.id);
  Check(additions.size() == 3, "expected three batch rewrite operations");
  cvub::GenericRewriter rewriter(module, &analysis);
  const size_t removed =
      rewriter.removeManyFromBlocks({additions[1], additions.back()});
  Check(removed == 2, "batch tombstone count mismatch");
  const cvub::GenericOperation &survivor =
      module.operations.at(static_cast<size_t>(additions.front()));
  Check(survivor.ordinal == 0,
        "batch tombstones must renumber each surviving suffix once");
  Check(!analysis.hasUsers(module.operations
                               .at(static_cast<size_t>(additions[1]))
                               .results.front()),
        "batch tombstones must update incremental use counts");
}

void TestCompactGenericModuleIdentityFastPath() {
  cvub::GenericModule module = cvub::ParseGenericIR(
      "ub_overflow_model_cpp/tests/fixtures/two_aiv_functions.mlir", false);
  const std::string before = cvub::SerializeGenericModule(module);
  module = cvub::CompactGenericModule(std::move(module));
  cvub::ValidateGenericModule(module);
  Check(cvub::SerializeGenericModule(module) == before,
        "identity compaction must preserve the complete generic IR");
}

void TestCVPipelineAttributeScanner() {
  cvub::GenericOperation operation;
  operation.attributes =
      "{pipeline.cubeonly, nested = #test.attr<foo = [1, 2]>, "
      "label = \"x,y\"}";
  Check(cvub::CVPipelineHasAttribute(operation, "pipeline.cubeonly") &&
            cvub::CVPipelineHasAttribute(operation, "nested") &&
            cvub::CVPipelineHasAttribute(operation, "label") &&
            !cvub::CVPipelineHasAttribute(operation, "foo"),
        "CVPipelining attribute lookup must respect nested dictionaries");
}

void TestTypedPlanMemoryIndexRejectsTextFallback() {
  cvub::OperationRecord operation;
  operation.index = 0;
  operation.operationId = 7;
  operation.opName = "arith.addi";
  operation.text = "%sum = arith.addi %lhs, %rhs : index";
  std::vector<cvub::OperationRecord> operations{operation};
  bool rejected = false;
  try {
    (void)cvub::BuildPlanMemoryOperationIndexStorage(operations, true);
  } catch (const std::runtime_error &) {
    rejected = true;
  }
  Check(rejected,
        "typed PlanMemory index must not reconstruct SSA from operation text");
  cvub::MaterializeOperationValueLists(operations);
  const auto index =
      cvub::BuildPlanMemoryOperationIndexStorage(operations, true);
  Check(index->resultsByRecord.front() ==
            std::vector<std::string>{"%sum"} &&
            index->operandsByRecord.front() ==
                std::vector<std::string>({"%lhs", "%rhs"}),
        "typed PlanMemory index must consume materialized SSA lists");
}

void TestMarkMultiBufferExplicitMarksAndFailFast() {
  cvub::GenericModule module = cvub::ParseGenericIR(
      "ub_overflow_model_cpp/tests/fixtures/two_aiv_functions.mlir", false);
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
      "ub_overflow_model_cpp/tests/fixtures/sink_op_use_order.mlir", false);
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
  size_t secondFills = 0;
  size_t ifOnlyFills = 0;
  for (const cvub::GenericOperation &operation : module.operations) {
    if (operation.name != "hivm.hir.vbrc")
      continue;
    secondFills += operation.attributes.find("second_fill") !=
                   std::string::npos;
    ifOnlyFills += operation.attributes.find("if_only_fill") !=
                   std::string::npos;
  }
  Check(CountOperation(module, "hivm.hir.vbrc") == 5 && secondFills == 2,
        "uses in different loops must each receive a clone while repeated "
        "operands receive one clone per OpOperand");
  Check(ifOnlyFills == 1,
        "a consumer nested only in scf.if must not be treated as a loop "
        "consumer");
}

void TestVShLUsesReviewedDestinationStyleSemantics() {
  cvub::GenericOperation operation;
  operation.name = "hivm.hir.vshl";
  operation.operands = {0, 1};
  operation.operandTypes = {"tensor<64xi32>", "tensor<i32>"};
  operation.results = {2};
  operation.resultTypes = {"tensor<64xi32>"};
  operation.properties =
      "operandSegmentSizes = array<i32: 1, 1, 0>";
  cvub::ApplyOperationSemantics(operation);
  Check(operation.dpsInputs == std::vector<int>({0}) &&
            operation.dpsInits == std::vector<int>({1}),
        "VShL must use the destination-style operand segmentation declared "
        "by the real HIVM op");
}

void TestTileCubeVectorLoopLiftsMemRefLoadLikeRealPass() {
  cvub::GenericModule module = cvub::ParseGenericIR(
      "ub_overflow_model_cpp/tests/fixtures/tile_cube_vector_lift_load.mlir",
      false);
  module = cvub::tile_cube_vector_loop_detail::LiftMemRefLoadsInLoop(
      std::move(module));
  Check(CountOperation(module, "memref.alloc") == 0,
        "lift preprocessing must erase the single-use destination alloc");
  Check(CountOperation(module, "tensor.empty") == 1 &&
            CountOperation(module, "bufferization.to_tensor") == 1,
        "lift preprocessing must reproduce the real source conversion and "
        "destination empty canonicalization");
  const cvub::GenericOperation &load = FindOperation(module, "hivm.hir.load");
  Check(load.results.size() == 1 &&
            load.resultTypes == std::vector<std::string>{"tensor<64xf16>"} &&
            load.operandTypes ==
                std::vector<std::string>({"tensor<64xf16>",
                                          "tensor<64xf16>"}),
        "lift preprocessing must rebuild the load in tensor form");
}

cvub::GenericModule InjectBlockSyncFixture() {
  return cvub::ParseGenericIRText(R"mlir(
"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}], function_type = (i64) -> (), sym_name = "mix_kernel"}> ({
  ^bb0(%ffts: i64):
    "hivm.hir.load"() : () -> ()
    "func.return"() : () -> ()
  }) {hacc.function_kind = #hacc.function_kind<DEVICE>, hfusion.fusion_kind = #hfusion.fusion_kind<MIX_CV>, hivm.func_core_type = #hivm.func_core_type<MIX>} : () -> ()
}) : () -> ()
)mlir", false);
}

void TestInjectBlockSyncNativeBranches() {
  cvub::GenericModule disabled =
      cvub::RunInjectBlockSync(InjectBlockSyncFixture(), false, true);
  Check(CountOperation(disabled, "hivm.hir.set_ffts_base_addr") == 1,
        "disabled InjectBlockSync must retain SetFFTSBaseAddr like BiSheng");
  Check(CountOperation(disabled, "hivm.hir.sync_block") == 0 &&
            CountOperation(disabled, "hivm.hir.sync_block_set") == 0 &&
            CountOperation(disabled, "hivm.hir.sync_block_wait") == 0,
        "disabled InjectBlockSync must skip automatic synchronization");

  cvub::GenericModule blockAll =
      cvub::RunInjectBlockSync(InjectBlockSyncFixture(), true, false);
  Check(CountOperation(blockAll, "hivm.hir.set_ffts_base_addr") == 1,
        "block-all InjectBlockSync must set the FFTS base address once");
  Check(CountOperation(blockAll, "hivm.hir.pipe_barrier") == 2 &&
            CountOperation(blockAll, "hivm.hir.sync_block_set") == 4 &&
            CountOperation(blockAll, "hivm.hir.sync_block_wait") == 4,
        "block-all InjectBlockSync must surround each target with the native "
        "five-operation sequence");
}

void TestInjectBlockSyncCVUnrollSelectorPlacement() {
  cvub::GenericModule module = cvub::ParseGenericIRText(R"mlir(
"builtin.module"() ({
  "func.func"() <{function_type = () -> (), sym_name = "selector_fixture"}> ({
    %c0 = "arith.constant"() <{value = 0 : index}> : () -> index
    %c1 = "arith.constant"() <{value = 1 : index}> : () -> index
    %c4 = "arith.constant"() <{value = 4 : index}> : () -> index
    "scf.for"(%c0, %c4, %c1) ({
    ^bb0(%iv: index):
      "test.anchor"() : () -> ()
      "scf.yield"() : () -> ()
    }) <{multibuffer_unroll_factor = 4 : i32}> : (index, index, index) -> ()
    "func.return"() : () -> ()
  }) : () -> ()
}) : () -> ()
)mlir", false);
  const int loop = FindOperation(module, "scf.for").id;
  const int anchor = FindOperation(module, "test.anchor").id;
  cvub::GenericRewriter rewriter(module);
  cvub::CrossCoreDynamicEventBuilder events(
      module, rewriter,
      cvub::CrossCoreDynamicEventPlacement::BeforeSyncAnchor);
  const int selected = events.selectedCVUnrollEvent(loop, 5, anchor);
  cvub::CreateCrossCoreDynamicSync(
      module, rewriter, anchor, "hivm.hir.sync_block_wait", selected,
      cvub::SplitMixCoreType::Vector, false);
  cvub::ValidateGenericModule(module);

  const cvub::GenericOperation &anchorOp =
      module.operations.at(static_cast<size_t>(anchor));
  const cvub::GenericBlock &body =
      module.blocks.at(static_cast<size_t>(anchorOp.blockId));
  const size_t anchorPosition = static_cast<size_t>(
      std::find(body.operations.begin(), body.operations.end(), anchor) -
      body.operations.begin());
  Check(anchorPosition >= 4,
        "InjectBlockSync selector must precede its synchronization anchor");
  std::vector<std::string> prefix;
  for (size_t position = anchorPosition - 4; position < anchorPosition;
       ++position)
    prefix.push_back(module.operations.at(
        static_cast<size_t>(body.operations[position])).name);
  Check(prefix == std::vector<std::string>({
                      "arith.constant", "arith.addi", "arith.index_cast",
                      "hivm.hir.sync_block_wait"}),
        "InjectBlockSync CV-unroll selector must use native endpoint-local "
        "constant/add/index-cast placement");
}

} // namespace

int main() {
  TestVectorTiling();
  std::cout << "[PASS] yy pipeline retains default vector tiling semantics\n";
  TestVectorTilingIgnoresScalarAxisOperands();
  std::cout << "[PASS] TileCubeVectorLoop ignores scalar axis operands like "
               "BiSheng\n";
  TestInferBufferSizeComposesCVPipelineTripCount();
  std::cout << "[PASS] InferAndSetBufferSize composes CVPipelining bounds\n";
  TestExistingAffineMapFoldsConstantOperands();
  std::cout << "[PASS] existing affine maps fold constant operands\n";
  TestSubsetHoisting();
  std::cout << "[PASS] yy pipeline retains subset-hoisting semantics\n";
  TestSubsetMismatchBlocksTransactionally();
  std::cout << "[PASS] subset mismatch blocks transactionally\n";
  TestStrictInlineRejectsUnprovedCall();
  std::cout << "[PASS] strict inline rejects unproved calls\n";
  TestValidatorRejectsCrossFunctionValue();
  std::cout << "[PASS] generic verifier rejects cross-function SSA\n";
  TestInferLayoutRejectsRemappedScalarStore();
  std::cout << "[PASS] InferHIVMDataLayout exposes remapped store failure\n";
  TestInferLayoutStoreValidationScope();
  std::cout << "[PASS] InferHIVMDataLayout store validation respects scope\n";
  TestSplitMixDeadRegionIgnoresInternalYieldUses();
  std::cout << "[PASS] SplitMix dead-region cleanup ignores internal yields\n";
  TestSplitMixCubeFoldsEmptyInsertSlice();
  std::cout << "[PASS] SplitMix Cube folds empty insert_slice like BiSheng\n";
  TestAscend950TightlyCoupledBufferBlocks();
  std::cout << "[PASS] Ascend950 tightly-coupled buffers fail closed\n";
  TestA3TightlyCoupledPassesAreUBNoOp();
  std::cout << "[PASS] A2/A3 tightly-coupled passes are UB no-ops\n";
  TestRawAffineDynamicExtentUpperBound();
  std::cout << "[PASS] raw BiSheng affine dynamic extent has a closed bound\n";
  TestRawInlineAffineDynamicExtentUpperBound();
  std::cout << "[PASS] raw inline BiSheng affine extent has a closed bound\n";
  TestOTFExtraUsersBlockBeforePlanning();
  std::cout << "[PASS] OTF concat extra users fail closed\n";
  TestOTFSubbyteIsAlignedNoOp();
  std::cout << "[PASS] OTF sub-byte concat remains an aligned no-op\n";
  TestTask7SuccessfulTilingChangesUBSemantics();
  std::cout << "[PASS] Task7 successful tiling changes UB semantics\n";
  TestTask7DisabledTilingStillLimitsSubBlock();
  std::cout << "[PASS] disabled Task7 tiling still limits sub-block use\n";
  TestTask7UnresolvedMarkedSliceRollsBack();
  std::cout << "[PASS] unresolved Task7 marked slices roll back strictly\n";
  TestTask7EarlyCanonicalizationIsNotSkipped();
  std::cout << "[PASS] Task7 early canonicalization is modeled\n";
  TestTask7BubbleUpReachesGreedyFixedPoint();
  std::cout << "[PASS] Task7 bubble-up reaches the greedy fixed point\n";
  TestTask7NestedSubviewPreservesOtherParentUsers();
  std::cout << "[PASS] Task7 nested subview preserves other parent users\n";
  TestGenericRewriterAssignsRegionOrdinals();
  std::cout << "[PASS] GenericRewriter assigns region ordinals\n";
  TestGenericRewriterIncrementalReplaceAllUses();
  std::cout << "[PASS] GenericRewriter replaces uses incrementally\n";
  TestGenericRewriterBatchTombstones();
  std::cout << "[PASS] GenericRewriter batches tombstones\n";
  TestCompactGenericModuleIdentityFastPath();
  std::cout << "[PASS] GenericModule identity compaction is exact\n";
  TestCVPipelineAttributeScanner();
  std::cout << "[PASS] CVPipelining attribute lookup avoids reparsing\n";
  TestTypedPlanMemoryIndexRejectsTextFallback();
  std::cout << "[PASS] typed PlanMemory index rejects text fallback\n";
  TestMarkMultiBufferExplicitMarksAndFailFast();
  std::cout << "[PASS] MarkMultiBuffer preserves explicit order and fails closed\n";
  TestSinkOpUseMultiplicityAndGreedyOrder();
  std::cout << "[PASS] SinkOpToConsumer preserves MLIR use and worklist order\n";
  TestVShLUsesReviewedDestinationStyleSemantics();
  std::cout << "[PASS] VShL uses reviewed destination-style semantics\n";
  TestTileCubeVectorLoopLiftsMemRefLoadLikeRealPass();
  std::cout << "[PASS] TileCubeVectorLoop lifts memref loads like BiSheng\n";
  TestInjectBlockSyncNativeBranches();
  std::cout << "[PASS] InjectBlockSync native disabled and block-all branches\n";
  TestInjectBlockSyncCVUnrollSelectorPlacement();
  std::cout << "[PASS] InjectBlockSync CV-unroll selectors use native "
               "endpoint placement\n";
  return 0;
}
