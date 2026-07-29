#include "../src/pipeline/cvpipelining_ub_pipeline.hpp"
#include "../src/passes/auto_blockify_parallel_loop.hpp"

#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <tuple>

namespace {

void Check(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}

cvub::GenericOperation MakeOperation(
    int id, int parent, int region, int block, int ordinal,
    std::string name, std::vector<int> results = {},
    std::vector<int> operands = {},
    std::vector<std::string> resultTypes = {},
    std::vector<std::string> operandTypes = {},
    std::string properties = {}, std::vector<int> regions = {}) {
  cvub::GenericOperation operation;
  operation.id = id;
  operation.parentId = parent;
  operation.regionId = region;
  operation.blockId = block;
  operation.ordinal = ordinal;
  operation.name = std::move(name);
  operation.results = std::move(results);
  operation.operands = std::move(operands);
  operation.resultTypes = std::move(resultTypes);
  operation.operandTypes = std::move(operandTypes);
  operation.properties = std::move(properties);
  operation.regions = std::move(regions);
  return operation;
}

cvub::GenericModule NestedReadBeforeWriteModule() {
  constexpr const char *tensor16 = "tensor<16x16xf32>";
  constexpr const char *tensor32 = "tensor<32x32xf32>";
  cvub::GenericModule module;
  module.operations = {
      MakeOperation(0, -1, -1, -1, 0, "builtin.module", {}, {}, {}, {},
                    {}, {0}),
      MakeOperation(1, 0, 0, 0, 0, "func.func", {}, {}, {}, {}, {}, {1}),
      MakeOperation(2, 1, 1, 1, 0, "arith.constant", {0}, {}, {"i1"}),
      MakeOperation(3, 1, 1, 1, 1, "tensor.empty", {1}, {}, {tensor32}),
      MakeOperation(
          4, 1, 1, 1, 2, "tensor.extract_slice", {2}, {1}, {tensor16},
          {tensor32},
          "{operandSegmentSizes = array<i32: 1, 0, 0, 0>, "
          "static_offsets = array<i64: 0, 0>, "
          "static_sizes = array<i64: 16, 16>, "
          "static_strides = array<i64: 1, 1>}"),
      MakeOperation(5, 1, 1, 1, 3, "tensor.empty", {3}, {}, {tensor16}),
      MakeOperation(
          6, 1, 1, 1, 4, "hivm.hir.vcast", {4}, {3, 2}, {tensor16},
          {tensor16, tensor16},
          "{operandSegmentSizes = array<i32: 1, 1, 0>}"),
      MakeOperation(7, 1, 1, 1, 5, "scf.if", {}, {0}, {}, {"i1"}, {},
                    {2}),
      MakeOperation(8, 7, 2, 2, 0, "test.read", {}, {4}, {}, {tensor16}),
      MakeOperation(9, 7, 2, 2, 1, "scf.yield"),
      MakeOperation(10, 1, 1, 1, 6, "tensor.empty", {5}, {}, {tensor16}),
      MakeOperation(
          11, 1, 1, 1, 7, "tensor.insert_slice", {6}, {5, 1}, {tensor32},
          {tensor16, tensor32},
          "{operandSegmentSizes = array<i32: 1, 1, 0, 0, 0>, "
          "static_offsets = array<i64: 0, 0>, "
          "static_sizes = array<i64: 16, 16>, "
          "static_strides = array<i64: 1, 1>}")};
  module.regions = {{0, 0, 0, {0}}, {1, 1, 0, {1}}, {2, 7, 0, {2}}};
  module.blocks = {{0, 0, 0, {}, {}, {1}},
                   {1, 1, 0, {}, {}, {2, 3, 4, 5, 6, 7, 10, 11}},
                   {2, 2, 0, {}, {}, {8, 9}}};
  return module;
}

cvub::OneShotBufferizationDecision DecisionFor(
    const std::vector<cvub::OneShotOpOperandDecision> &decisions,
    int operation, int operand) {
  const auto found = std::find_if(
      decisions.begin(), decisions.end(), [&](const auto &decision) {
        return decision.operationId == operation &&
               decision.operandNumber == operand;
      });
  if (found == decisions.end())
    throw std::runtime_error("missing OneShot decision in test module");
  return found->decision;
}

cvub::OperationRecord MakePlanOperation(
    int index, std::string name, std::vector<int> path, int block,
    std::vector<std::string> results = {},
    std::vector<std::string> operands = {}, std::string semanticKey = {}) {
  cvub::OperationRecord operation;
  operation.index = index;
  operation.line = index + 1;
  operation.operationId = index;
  operation.opName = std::move(name);
  operation.regionPath = std::move(path);
  operation.blockId = block;
  operation.blockLabel = "^bb" + std::to_string(block);
  operation.materializedValueLists = true;
  operation.materializedResults = std::move(results);
  operation.materializedOperands = std::move(operands);
  operation.normalizationKey = std::move(semanticKey);
  std::ostringstream text;
  if (!operation.materializedResults.empty())
    text << operation.materializedResults.front() << " = ";
  text << operation.opName;
  for (const std::string &operand : operation.materializedOperands)
    text << ' ' << operand;
  operation.text = text.str();
  return operation;
}

void TestNormalizeDominatingCSE() {
  const std::string collapseKey =
      "memref.collapse_shape %base : memref<128x32xf32> -> "
      "memref<4096xf32>\nreassociation=0,1";
  std::vector<cvub::OperationRecord> operations = {
      MakePlanOperation(0, "memref.alloc", {}, 0, {"%base"}),
      MakePlanOperation(1, "memref.collapse_shape", {}, 0, {"%outer"},
                        {"%base"}, collapseKey),
      MakePlanOperation(2, "scf.for", {}, 0),
      MakePlanOperation(3, "memref.collapse_shape", {7}, 1, {"%inner"},
                        {"%base"}, collapseKey),
      MakePlanOperation(4, "hivm.hir.copy", {7}, 1, {}, {"%inner"}),
      MakePlanOperation(5, "scf.yield", {7}, 1),
      MakePlanOperation(6, "scf.for.end", {}, 0),
  };
  operations = cvub::ApplyPlanMemoryNormalizePatterns(std::move(operations));
  Check(std::count_if(operations.begin(), operations.end(), [](const auto &op) {
          return op.opName == "memref.collapse_shape";
        }) == 1,
        "CSE must reuse an equivalent collapse from a dominating outer block");
  const auto copy = std::find_if(operations.begin(), operations.end(),
                                 [](const auto &op) {
                                   return op.opName == "hivm.hir.copy";
                                 });
  Check(copy != operations.end() &&
            cvub::operationOperandNames(*copy) ==
                std::vector<std::string>{"%outer"},
        "CSE must rewrite the nested use to the dominating collapse result");

  std::vector<cvub::OperationRecord> siblings = {
      MakePlanOperation(0, "memref.alloc", {}, 0, {"%base"}),
      MakePlanOperation(1, "scf.if", {}, 0),
      MakePlanOperation(2, "memref.collapse_shape", {10}, 1, {"%then"},
                        {"%base"}, collapseKey),
      MakePlanOperation(3, "hivm.hir.copy", {10}, 1, {}, {"%then"}),
      MakePlanOperation(4, "scf.if.end", {}, 0),
      MakePlanOperation(5, "scf.if", {}, 0),
      MakePlanOperation(6, "memref.collapse_shape", {11}, 2, {"%else"},
                        {"%base"}, collapseKey),
      MakePlanOperation(7, "hivm.hir.copy", {11}, 2, {}, {"%else"}),
      MakePlanOperation(8, "scf.if.end", {}, 0),
  };
  siblings = cvub::ApplyPlanMemoryNormalizePatterns(std::move(siblings));
  Check(std::count_if(siblings.begin(), siblings.end(), [](const auto &op) {
          return op.opName == "memref.collapse_shape";
        }) == 2,
        "CSE must not reuse a collapse defined in a sibling region");
}

void TestFlattenCollapseCompositionCSE() {
  cvub::UBAffectingPassOptions options;
  options.enableTritonKernelCompile = true;
  const cvub::PlanMemoryInput input =
      cvub::BuildPlanMemoryInputFromAfterCVPipelining(
          "ub_overflow_model_cpp/data/before_cvpipelining/"
          "kernels_vllm_solve_tril_16x16_kernel.ttadapter/"
          "before_cvpipelining_func_func_solve_tril_16x16_kernel_32.mlir",
          options);

  std::set<std::tuple<int, std::vector<int>, std::string>> collapses;
  for (const cvub::OperationRecord &operation : input.operations) {
    if (operation.opName != "memref.collapse_shape")
      continue;
    const size_t equal = operation.text.find('=');
    const std::string rhs =
        equal == std::string::npos
            ? operation.text
            : cvub::trim(operation.text.substr(equal + 1));
    Check(collapses.emplace(operation.blockId, operation.regionPath, rhs)
              .second,
          "composed collapse_shape must CSE an equivalent view in its block");
  }
}

void TestCVPipeliningDoesNotExtractOverwrittenWorkspaceDestination() {
  cvub::GenericModule module = cvub::ParseGenericIR(
      "ub_overflow_model_cpp/data/before_cvpipelining/"
      "python_tutorial_06-fused-attention.ttadapter/"
      "before_cvpipelining_func_func_attn_fwd_32.mlir",
      false);
  cvub::ApplyOperationSemanticsToAll(module.operations);
  module = cvub::RunCVPipeliningPass(std::move(module), {});

  std::set<int> usedValues;
  for (const cvub::GenericOperation &operation : module.operations)
    usedValues.insert(operation.operands.begin(), operation.operands.end());
  for (const cvub::GenericOperation &operation : module.operations) {
    if (operation.name != "tensor.extract_slice")
      continue;
    Check(std::any_of(operation.results.begin(), operation.results.end(),
                      [&](int value) { return usedValues.count(value) != 0; }),
          "CVPipelining must not extract a workspace destination that real "
          "processWorkspaceOutputs immediately overwrites");
  }
}

void TestRepeatedDestinationValueKeepsDistinctBufferizedOutputs() {
  cvub::UBAffectingPassOptions options;
  options.enableTritonKernelCompile = true;
  const cvub::PlanMemoryInput input =
      cvub::BuildPlanMemoryInputFromAfterCVPipelining(
          "ub_overflow_model_cpp/data/before_cvpipelining/"
          "triton.language.umulhi.ttadapter/"
          "before_cvpipelining_func_func_umulhi_kernel_32.mlir",
          options);
  const auto operation = std::find_if(
      input.operations.begin(), input.operations.end(), [](const auto &op) {
        return op.opName == "hivm.hir.vmulextui";
      });
  Check(operation != input.operations.end(),
        "umulhi regression input must retain vmulextui");
  Check(operation->materializedOutputs.size() == 2 &&
            operation->materializedOutputs[0] !=
                operation->materializedOutputs[1],
        "repeated tensor init SSA values must use the distinct memrefs "
        "created for their tied results");
}

void TestEquivalentMarkedSlicesAreCSEdBeforeBubbleUp() {
  cvub::GenericModule module = cvub::ParseGenericIR(
      "ub_overflow_model_cpp/data/before_cvpipelining/"
      "s5_exp15_hivm_auto_cv_balance_4235us.ttadapter/"
      "before_cvpipelining_func_func_"
      "sparse_flash_attention_prefill_kernel_cvpipe_32.mlir",
      false);
  module = cvub::RunCVPipeliningPass(std::move(module), {});
  cvub::UBAffectingPassOptions options;
  options.enableTritonKernelCompile = true;
  module = cvub::RunPassesBeforeLoopInvariantCodeMotion(
      std::move(module), options);
  cvub::ValidateGenericModule(module);

  const size_t subBlockGuards = static_cast<size_t>(std::count_if(
      module.operations.begin(), module.operations.end(),
      [](const cvub::GenericOperation &operation) {
        return operation.name == "hivm.hir.get_sub_block_idx";
      }));
  const size_t tiledLoops = static_cast<size_t>(std::count_if(
      module.operations.begin(), module.operations.end(),
      [](const cvub::GenericOperation &operation) {
        return operation.name == "scf.for" &&
               cvub::HasSplitMixDictionaryEntry(operation.attributes,
                                                 "map_for_to_forall");
      }));
  Check(subBlockGuards == 0 && tiledLoops == 1,
        "equivalent marked extract slices must be CSE'd so the transactional "
        "TileAndBind candidate does not roll back");
}

void TestPostOneShotScalarCSEProjection() {
  cvub::GenericModule module = cvub::ParseGenericIRText(R"mlir(
"builtin.module"() ({
  "func.func"() <{function_type = () -> (), sym_name = "scalar_cse"}> ({
  ^bb0:
    %zero = "arith.constant"() <{value = 0 : index}> : () -> index
    %sub0 = "hivm.hir.get_sub_block_idx"() : () -> i64
    %idx0 = "arith.index_cast"(%sub0) : (i64) -> index
    %cond0 = "arith.cmpi"(%idx0, %zero) <{predicate = 0 : i64}> : (index, index) -> i1
    "scf.if"(%cond0) ({
      "scf.yield"() : () -> ()
    }, {
    }) : (i1) -> ()
    %sub1 = "hivm.hir.get_sub_block_idx"() : () -> i64
    %idx1 = "arith.index_cast"(%sub1) : (i64) -> index
    %cond1 = "arith.cmpi"(%idx1, %zero) <{predicate = 0 : i64}> : (index, index) -> i1
    "scf.if"(%cond1) ({
      "scf.yield"() : () -> ()
    }, {
    }) : (i1) -> ()
    "func.return"() : () -> ()
  }) : () -> ()
}) : () -> ()
)mlir", false);
  cvub::ApplyOperationSemanticsToAll(module.operations);
  module = cvub::RunPostOneShotScalarCSEProjection(std::move(module));
  cvub::ValidateGenericModule(module);
  const auto count = [&](const char *name) {
    return static_cast<size_t>(std::count_if(
        module.operations.begin(), module.operations.end(),
        [&](const cvub::GenericOperation &operation) {
          return operation.name == name;
        }));
  };
  Check(count("hivm.hir.get_sub_block_idx") == 1 &&
            count("arith.index_cast") == 1 && count("arith.cmpi") == 1 &&
            count("scf.if") == 2,
        "post-OneShot scalar CSE must share the dominating guard chain");
}

void TestErasedSinglePointLocalBufferDetection() {
  const std::map<std::string, std::string> survivors = {
      {"local:0", "local:0"}, {"local:2", "local:1"}};
  Check(cvub::IsErasedSinglePointLocalBuffer("local:1", survivors),
        "a removed local allocation must be recognized before a compacted "
        "survivor reuses its ordinal");
  Check(!cvub::IsErasedSinglePointLocalBuffer("local:2", survivors),
        "a surviving local allocation must remain materialized");
  Check(!cvub::IsErasedSinglePointLocalBuffer("local:1", {}),
        "an empty survivor map represents the identity construction");
}

void TestGeneratedRewriteBufferMappingIsAppliedOnce() {
  cvub::PostBufferizationRewriteState state;
  state.singlePoint.bufferMapping = {
      {"local:24", "local:20"}, {"local:20", "local:16"}};
  const std::vector<cvub::LocalBufferRecord> buffers = {
      {"base:16", "base:16", "old", "memref<128xf16>",
       cvub::AddressSpace::UB, 2048, false, {}},
      {"base:20", "base:20", "rewritten", "memref<128xf32>",
       cvub::AddressSpace::UB, 4096, false, {}}};
  const cvub::LocalBufferIndex index(buffers);
  cvub::PipelineMetadataCache metadata;
  const std::map<std::string, std::string> valueTypes;

  Check(cvub::GeneratedBufferType(state, "local:20", index, valueTypes) ==
            "memref<128xf32>",
        "post-SinglePoint generated buffers must not be remapped twice");
  Check(cvub::GeneratedBufferAllocationTypeForTrace(
            state, "local:20", index, "", metadata) ==
            "memref<128xf32>",
        "generated allocation tracing must use the post-SinglePoint ordinal");
}

void TestPlanMemoryParentLoopFollowsYieldedBuffer() {
  cvub::LifetimeAnalysis liveness;
  liveness.operations = {
      MakePlanOperation(0, "scf.for", {}, 0),
      MakePlanOperation(1, "scf.for", {10}, 1, {"%inner_result"}),
      MakePlanOperation(2, "memref.alloc", {10, 11}, 2, {"%inner"}),
      MakePlanOperation(3, "test.consume", {10, 11}, 2, {}, {"%inner"}),
      MakePlanOperation(4, "scf.yield", {10, 11}, 2, {}, {"%inner"}),
      MakePlanOperation(5, "test.consume", {10}, 1, {},
                        {"%inner_result"}),
      MakePlanOperation(6, "memref.alloc", {10}, 1, {"%outer"}),
      MakePlanOperation(7, "test.consume", {10}, 1, {}, {"%outer"}),
      MakePlanOperation(8, "scf.for.implicit_yield", {10}, 1),
  };
  liveness.canonicalAllocByValue = {
      {"%inner", "%inner"}, {"%inner_result", "%inner"},
      {"%outer", "%outer"}};
  std::vector<cvub::BufferInfoRecord> buffers = {
      {"%inner", 256, 256, false}, {"%outer", 256, 256, false}};
  const cvub::PreparedStorageEntryAnalysis prepared(buffers, liveness);
  Check(prepared.parentLoopByBuffer.at("%inner") ==
            prepared.parentLoopByBuffer.at("%outer"),
        "a buffer yielded from an inner loop and consumed by the outer loop "
        "must use the native outer consumer-loop anchor");
}

void TestAtomicSyncBlockLockIsHoistedAroundOutermostLoop() {
  cvub::GenericModule module = cvub::ParseGenericIR(
      "ub_overflow_model_cpp/data/before_cvpipelining/"
      "triton.language.atomic_and.ttadapter/"
      "before_cvpipelining_func_func_atomic_and_32.mlir",
      false);
  cvub::ApplyOperationSemanticsToAll(module.operations);
  module = cvub::RunAutoBlockifyPrefixStage(std::move(module), {});
  module = cvub::RunCVPipeliningPass(std::move(module), {});
  cvub::UBAffectingPassOptions options;
  options.enableTritonKernelCompile = true;
  const cvub::PlanMemoryInput input =
      cvub::BuildPlanMemoryInputFromAfterCVPipelining(std::move(module),
                                                       options);

  const auto position = [&](const char *name) {
    const auto found = std::find_if(
        input.operations.begin(), input.operations.end(),
        [&](const cvub::OperationRecord &operation) {
          return operation.opName == name;
        });
    Check(found != input.operations.end(),
          "atomic hoisting regression is missing an expected operation");
    return static_cast<size_t>(
        std::distance(input.operations.begin(), found));
  };
  const size_t loop = position("scf.for");
  const size_t loopEnd = position("scf.for.end");
  const size_t lock = position("hivm.hir.sync_block_lock");
  const size_t unlock = position("hivm.hir.sync_block_unlock");
  Check(lock < loop && loopEnd < unlock,
        "SyncBlockHoisting must place one lock pair around the outermost "
        "atomic loop");
  Check(std::count_if(input.operations.begin(), input.operations.end(),
                      [](const cvub::OperationRecord &operation) {
                        return operation.opName ==
                                   "hivm.hir.sync_block_lock" ||
                               operation.opName ==
                                   "hivm.hir.sync_block_unlock";
                      }) == 2,
        "one outermost loop must retain exactly one lock/unlock pair");
}

void TestSavingUBUnitAttributeDetection() {
  Check(cvub::HasUnitAttribute("{hivm.enable_saving_ub}",
                               "hivm.enable_saving_ub"),
        "the model must recognize the native namespaced saving-UB unit "
        "attribute");
  Check(!cvub::HasUnitAttribute("{hivm.func_core_type = "
                                 "#hivm.func_core_type<AIV>}",
                                 "hivm.enable_saving_ub"),
        "an unrelated function attribute must not enable saving-UB sizing");

  cvub::GenericOperation reduction;
  reduction.name = "hivm.hir.vreduce";
  reduction.operands = {-1, -1};
  reduction.operandTypes = {"memref<1x4096xf32>", "memref<1x1xf32>"};
  reduction.attributes =
      "{reduce_dims = array<i64: 1>, "
      "arith = #hivm.hir.reduce_arith<sum>}";
  cvub::PipelineMetadataCache metadata;
  const auto regular = cvub::ModelExtraBufferForOperation(
      reduction, reduction.operandTypes, metadata, false);
  const auto saving = cvub::ModelExtraBufferForOperation(
      reduction, reduction.operandTypes, metadata, true);
  Check(regular && regular->type == "memref<2048xf32>",
        "the regular vreduce query must preserve the native temp size");
  Check(saving && saving->type == "memref<512xf32>",
        "the synthetic interface query must inherit saving-UB semantics");
}

} // namespace

int main() {
  const cvub::GenericModule module = cvub::ParseGenericIR(
      "ub_overflow_model_cpp/tests/fixtures/two_aiv_functions.mlir", false);
  const cvub::ModulePlanResult result =
      cvub::RunUBModuleFromAfterCVPipelining(module, {});

  Check(result.precision == cvub::ModulePlanPrecision::Exact,
        "two independently supported AIV functions must remain exact");
  Check(result.functions.size() == 2,
        "each AIV function must have an independent plan");
  Check(result.peakBits == 98304,
        "module peak must be the maximum function peak");
  Check(result.requiredBits == 98304,
        "module required bits must be the maximum function requirement");
  Check(result.functions[0].function != result.functions[1].function,
        "function plans must retain distinct identities");

  const cvub::ModulePlanResult fastResult =
      cvub::RunUBModuleFromAfterCVPipelining(
          module, {}, std::nullopt, false, nullptr, cvub::kUBCapacityBits,
          true, false);
  Check(fastResult.decisionOnlyNonOverflow &&
            fastResult.conservativeUpperBoundBits &&
            *fastResult.conservativeUpperBoundBits <=
                fastResult.capacityBits &&
            fastResult.functions.empty(),
        "the conservative upper bound must prove a safe module without "
        "materializing PlanMemory");

  const cvub::ModulePlanResult smallCapacity =
      cvub::RunUBModuleFromAfterCVPipelining(
          module, {}, std::nullopt, false, nullptr, 1, true, false);
  Check(!smallCapacity.decisionOnlyNonOverflow && smallCapacity.overflow &&
            !smallCapacity.functions.empty(),
        "an upper bound above capacity must fall through to full overflow "
        "planning");

  const cvub::ModulePlanResult observed =
      cvub::RunUBModuleFromAfterCVPipelining(
          module, {}, std::nullopt, false, nullptr, cvub::kUBCapacityBits,
          false, true);
  Check(!observed.decisionOnlyNonOverflow &&
            observed.conservativeUpperBoundBits &&
            observed.functions.size() == result.functions.size(),
        "validation must observe the same proof and retain full plans");

  cvub::UBAffectingPassOptions ubSavingOptions;
  ubSavingOptions.enableUbufSaving = true;
  ubSavingOptions.enableTritonKernelCompile = true;
  std::ostringstream timingOutput;
  cvub::DebugTrace trace(timingOutput, {}, false, true, false);
  (void)cvub::RunPassesBeforeOneShotBufferize(module, ubSavingOptions, &trace);
  std::vector<std::string> stageNames;
  for (const cvub::DebugTrace::RuntimeTimingRecord &record :
       trace.RuntimeTimings())
    stageNames.push_back(record.name);
  const auto stage = [&](const char *name) {
    const auto found = std::find(stageNames.begin(), stageNames.end(), name);
    Check(found != stageNames.end(), "missing expected UB-saving stage");
    return static_cast<size_t>(std::distance(stageNames.begin(), found));
  };
  Check(stage("CloneTensorEmptyAfterCVPipelining") <
            stage("SinkOpToConsumerInLoopAfterCVPipelining"),
        "BiSheng must clone before the early UB-saving sink");
  Check(stage("CloneTensorEmptyBeforeUbufSavingSink") <
            stage("SinkOpToConsumerInLoop"),
        "BiSheng must clone again immediately before the bufferization sink");

  const std::vector<cvub::OneShotOpOperandDecision> nestedDecisions =
      cvub::ModelOneShotAnalysis(NestedReadBeforeWriteModule());
  Check(DecisionFor(nestedDecisions, 11, 1) ==
            cvub::OneShotBufferizationDecision::InPlace,
        "the later insert_slice destination must bufferize in-place");
  Check(DecisionFor(nestedDecisions, 6, 1) ==
            cvub::OneShotBufferizationDecision::InPlace,
        "the destination-style definition must bufferize in-place");
  Check(DecisionFor(nestedDecisions, 4, 0) ==
            cvub::OneShotBufferizationDecision::InPlace,
        "a nested read before a later write must not create a false RaW "
        "conflict");

  TestNormalizeDominatingCSE();
  TestFlattenCollapseCompositionCSE();
  TestCVPipeliningDoesNotExtractOverwrittenWorkspaceDestination();
  TestRepeatedDestinationValueKeepsDistinctBufferizedOutputs();
  TestEquivalentMarkedSlicesAreCSEdBeforeBubbleUp();
  TestPostOneShotScalarCSEProjection();
  TestErasedSinglePointLocalBufferDetection();
  TestGeneratedRewriteBufferMappingIsAppliedOnce();
  TestPlanMemoryParentLoopFollowsYieldedBuffer();
  TestAtomicSyncBlockLockIsHoistedAroundOutermostLoop();
  TestSavingUBUnitAttributeDetection();

  std::cout << "[PASS] module plans AIV functions independently\n";
  std::cout << "[PASS] conservative non-overflow proof is decision-only\n";
  std::cout << "[PASS] BiSheng UB-saving clone/sink order is preserved\n";
  std::cout << "[PASS] OneShot nested read ordering mirrors DominanceInfo\n";
  std::cout << "[PASS] PlanMemory normalization mirrors dominance-aware CSE\n";
  std::cout << "[PASS] FlattenOps collapse composition reuses equivalent views\n";
  std::cout << "[PASS] CVPipelining omits overwritten workspace slices\n";
  std::cout << "[PASS] repeated DPS init values keep distinct memrefs\n";
  std::cout << "[PASS] equivalent marked slices CSE before bubble-up\n";
  std::cout << "[PASS] post-OneShot scalar CSE shares dominating guards\n";
  std::cout << "[PASS] erased SinglePoint locals are detected before remap\n";
  std::cout << "[PASS] generated rewrite buffers are mapped exactly once\n";
  std::cout << "[PASS] PlanMemory parent loops follow yielded buffers\n";
  std::cout << "[PASS] atomic sync-block locks hoist around outermost loops\n";
  std::cout << "[PASS] saving-UB unit attributes are detected exactly\n";
  return 0;
}
