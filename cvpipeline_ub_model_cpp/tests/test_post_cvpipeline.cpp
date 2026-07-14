#include "test_support.hpp"
#include "../src/post_cvpipeline/ir_utils.hpp"
#include "../src/post_cvpipeline/pipeline.hpp"
#include "../src/post_cvpipeline/split_mix_aiv.hpp"

#include <fstream>
#include <string>
#include <vector>

namespace {

bool HasDiagnostic(const cvub::PostCVPipelineResult &result,
                   const std::string &stage, const std::string &reason) {
  for (const cvub::PostCVPipelineDiagnostic &diagnostic : result.diagnostics) {
    if (diagnostic.pipelineStage == stage &&
        diagnostic.reason.find(reason) != std::string::npos)
      return true;
  }
  return false;
}

cvub::fs::path WriteManifest(const std::string &name,
                             const std::string &contents) {
  const cvub::fs::path path =
      cvub::fs::path("cvpipeline_ub_model_cpp/output/tests") / name;
  std::ofstream output(path);
  if (!output)
    throw std::runtime_error("cannot write test manifest: " + path.string());
  output << contents;
  return path;
}

int ResultOf(const cvub::GenericModule &module, const std::string &operation) {
  for (const cvub::GenericOperation &candidate : module.operations)
    if (candidate.name == operation && !candidate.results.empty())
      return candidate.results.front();
  throw std::runtime_error("missing result-producing operation: " + operation);
}

int OperationId(const cvub::GenericModule &module,
                const std::string &operation) {
  for (const cvub::GenericOperation &candidate : module.operations)
    if (candidate.name == operation)
      return candidate.id;
  throw std::runtime_error("missing operation: " + operation);
}

bool HasOperation(const cvub::GenericModule &module,
                  const std::string &operation) {
  return std::any_of(module.operations.begin(), module.operations.end(),
                     [&](const cvub::GenericOperation &candidate) {
                       return candidate.name == operation;
                     });
}

const cvub::GenericOperation &OperationWithCase(
    const cvub::GenericModule &module, const std::string &caseName) {
  for (const cvub::GenericOperation &operation : module.operations)
    if (cvub::UnquoteIRString(
            cvub::IRDictionaryValue(operation.attributes, "case")) == caseName)
      return operation;
  throw std::runtime_error("missing operation case: " + caseName);
}

const cvub::ProjectedAIVModule &ProjectedFunction(
    const cvub::PostCVPipelineResult &result, const std::string &source) {
  for (const auto &function : result.functions)
    if (function.sourceFunction == source)
      return function;
  throw std::runtime_error("missing projected function: " + source);
}

CVUB_TEST(post_pipeline_defaults_match_compiler) {
  const cvub::PostCVPipelineOptions options;
  CVUB_CHECK_EQ(options.tileMixVectorLoop, 2U);
  CVUB_CHECK_EQ(options.tileMixCubeLoop, 2U);
  CVUB_CHECK(options.enableAutoBindSubBlock);
  CVUB_CHECK(options.enableCodeMotion);
  CVUB_CHECK(!options.enableUbufSaving);
}

CVUB_TEST(split_mix_keeps_vector_and_workspace_boundary) {
  auto module = cvub::test::ParseFixture("simple_mix_before_split.mlir");
  auto &cube = module.operations.at(
      static_cast<size_t>(OperationId(module, "hivm.hir.mmadL1")));
  cube.dpsInits = {cube.operands.back()};

  auto result = cvub::ProjectMixFunctionsToAIV(std::move(module));

  CVUB_CHECK_EQ(result.functions.size(), 1U);
  const auto &projected = result.functions.front();
  CVUB_CHECK_EQ(projected.projectedFunction, "kernel_mix_aiv");
  CVUB_CHECK(!HasOperation(projected.module, "hivm.hir.mmadL1"));
  CVUB_CHECK(!HasOperation(projected.module, "hivm.hir.fixpipe"));
  CVUB_CHECK(HasOperation(projected.module, "memref_ext.alloc_workspace"));
  CVUB_CHECK(HasOperation(projected.module, "hivm.hir.load"));
  CVUB_CHECK(HasOperation(projected.module, "hivm.hir.vadd"));
  cvub::ValidateGenericModule(projected.module);
}

CVUB_TEST(core_classification_honors_precedence_and_known_registries) {
  const auto module =
      cvub::test::ParseFixture("control_flow_mix_before_split.mlir");
  CVUB_CHECK_EQ(cvub::ClassifyCore(
                    module, module.operations.at(static_cast<size_t>(
                                OperationId(module, "scf.for")))),
                cvub::CoreKind::Cube);
  CVUB_CHECK_EQ(cvub::ClassifyCore(
                    module, module.operations.at(static_cast<size_t>(
                                OperationId(module, "hivm.hir.mmadL1")))),
                cvub::CoreKind::Cube);
  CVUB_CHECK_EQ(cvub::ClassifyCore(
                    module, module.operations.at(static_cast<size_t>(
                                OperationId(module, "hivm.hir.vsub")))),
                cvub::CoreKind::Vector);
  CVUB_CHECK_EQ(cvub::ClassifyCore(
                    module, module.operations.at(static_cast<size_t>(
                                OperationId(module, "tensor.empty")))),
                cvub::CoreKind::Neutral);
  CVUB_CHECK_EQ(cvub::ClassifyCore(
                    module, module.operations.at(static_cast<size_t>(
                                OperationId(module, "hivm.hir.custom")))),
                cvub::CoreKind::Unknown);
}

CVUB_TEST(core_classification_rejects_non_enum_substring_matches) {
  auto module =
      cvub::test::ParseFixture("control_flow_mix_before_split.mlir");
  auto &custom = module.operations.at(
      static_cast<size_t>(OperationId(module, "hivm.hir.custom")));
  custom.attributes = cvub::SetDictionaryValue(
      custom.attributes, "tcoretype", "#hivm.tcore_type<NOT_VECTOR>");
  CVUB_CHECK_EQ(cvub::ClassifyCore(module, custom), cvub::CoreKind::Unknown);
}

CVUB_TEST(split_mix_replaces_dps_loop_if_and_scope_results) {
  auto module =
      cvub::test::ParseFixture("control_flow_mix_before_split.mlir");
  auto &cube = module.operations.at(
      static_cast<size_t>(OperationId(module, "hivm.hir.mmadL1")));
  const int base = cube.operands.back();
  cube.dpsInits = {base};

  const auto result = cvub::ProjectMixFunctionsToAIV(std::move(module));
  const auto &projected = ProjectedFunction(result, "control").module;
  for (const std::string caseName : {"dps", "loop", "if", "scope"})
    CVUB_CHECK_EQ(OperationWithCase(projected, caseName).operands.front(),
                  ResultOf(projected, "tensor.empty"));
  CVUB_CHECK(!HasOperation(projected, "hivm.hir.mmadL1"));
  CVUB_CHECK(!HasOperation(projected, "scf.for"));
  CVUB_CHECK(!HasOperation(projected, "scf.if"));
  CVUB_CHECK(!HasOperation(projected, "scope.scope"));
  CVUB_CHECK(!HasOperation(projected, "annotation.mark"));
  cvub::ValidateGenericModule(projected);
}

CVUB_TEST(split_mix_unknown_op_preserves_closure_and_reports_incomplete) {
  auto module =
      cvub::test::ParseFixture("control_flow_mix_before_split.mlir");
  auto &cube = module.operations.at(
      static_cast<size_t>(OperationId(module, "hivm.hir.mmadL1")));
  cube.dpsInits = {cube.operands.back()};

  const auto result = cvub::ProjectMixFunctionsToAIV(std::move(module));
  const auto &projected = ProjectedFunction(result, "control").module;
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Incomplete);
  CVUB_CHECK(HasDiagnostic(result, "SplitMixKernelAIVProjection",
                           "no compiler core classification"));
  CVUB_CHECK(HasOperation(projected, "hivm.hir.custom"));
  CVUB_CHECK(HasOperation(projected, "hivm.hir.vadd"));
  cvub::ValidateGenericModule(projected);
}

CVUB_TEST(split_mix_isolates_each_mix_function) {
  auto module =
      cvub::test::ParseFixture("control_flow_mix_before_split.mlir");
  auto &cube = module.operations.at(
      static_cast<size_t>(OperationId(module, "hivm.hir.mmadL1")));
  cube.dpsInits = {cube.operands.back()};
  const auto result = cvub::ProjectMixFunctionsToAIV(std::move(module));
  CVUB_CHECK_EQ(result.functions.size(), 2U);
  CVUB_CHECK_EQ(cvub::DeviceFunctionNames(ProjectedFunction(result, "control").module),
                std::vector<std::string>({"control_mix_aiv"}));
  CVUB_CHECK_EQ(cvub::DeviceFunctionNames(ProjectedFunction(result, "second").module),
                std::vector<std::string>({"second_mix_aiv"}));
}

CVUB_TEST(split_mix_passes_already_aiv_function_through_in_isolation) {
  auto result = cvub::ProjectMixFunctionsToAIV(
      cvub::test::ParseFixture("minimal_aiv.mlir"));
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
  CVUB_CHECK_EQ(result.functions.size(), 1U);
  CVUB_CHECK_EQ(result.functions.front().sourceFunction, "minimal_aiv");
  CVUB_CHECK_EQ(result.functions.front().projectedFunction, "minimal_aiv");
  CVUB_CHECK_EQ(cvub::DeviceFunctionNames(result.functions.front().module),
                std::vector<std::string>({"minimal_aiv"}));
  cvub::ValidateGenericModule(result.functions.front().module);
}

CVUB_TEST(post_pipeline_manifest_has_real_order) {
  const auto stages = cvub::LoadPostCVPipelineManifest(
      "cvpipeline_ub_model_cpp/config/post_cvpipeline_manifest.tsv");
  CVUB_CHECK_EQ(cvub::StageNames(stages),
                std::vector<std::string>({
                    "TileCubeVectorLoop", "InferAndSetBufferSize",
                    "WorkspaceSemanticProjection",
                    "CanonicalizationBeforeSplit", "CrossCoreSyncInvariant",
                    "SplitMixKernelAIVProjection", "InlineScope",
                    "TileAndBindSubBlock", "FoldTensorEmpty",
                    "CanonicalizationAfterSplit", "LoopInvariantCodeMotion",
                    "LoopInvariantSubsetHoisting", "CloneTensorEmpty",
                    "InlineOTFLoadStore"}));
  for (const cvub::StageCoverage &stage : stages)
    CVUB_CHECK_EQ(stage.disposition,
                  stage.stage == "SplitMixKernelAIVProjection"
                      ? cvub::CoverageDisposition::Modeled
                      : cvub::CoverageDisposition::Unsupported);
}

CVUB_TEST(ub_saving_configuration_is_reported_incomplete) {
  cvub::PostCVPipelineOptions options;
  options.enableUbufSaving = true;
  auto result = cvub::RunPostCVPipelineAIVProjection(
      cvub::test::ParseFixture("minimal_aiv.mlir"), options);
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Incomplete);
  CVUB_CHECK(HasDiagnostic(result, "configuration", "enableUbufSaving"));
}

CVUB_TEST(post_pipeline_manifest_rejects_duplicate_stages) {
  const auto path = WriteManifest(
      "duplicate_post_cvpipeline_manifest.tsv",
      "order\tstage\tdisposition\n"
      "1\tTileCubeVectorLoop\tunsupported\n"
      "2\tTileCubeVectorLoop\tunsupported\n");
  CVUB_CHECK_THROWS_CONTAINS(cvub::LoadPostCVPipelineManifest(path),
                             "duplicate stage");
}

CVUB_TEST(post_pipeline_manifest_rejects_missing_columns) {
  const auto path = WriteManifest(
      "missing_columns_post_cvpipeline_manifest.tsv",
      "order\tstage\tdisposition\n1\tTileCubeVectorLoop\n");
  CVUB_CHECK_THROWS_CONTAINS(cvub::LoadPostCVPipelineManifest(path),
                             "missing columns");
}

CVUB_TEST(post_pipeline_manifest_rejects_order_mismatch) {
  const auto path = WriteManifest(
      "wrong_order_post_cvpipeline_manifest.tsv",
      "order\tstage\tdisposition\n"
      "1\tInferAndSetBufferSize\tunsupported\n");
  CVUB_CHECK_THROWS_CONTAINS(cvub::LoadPostCVPipelineManifest(path),
                             "stage order mismatch");
}

CVUB_TEST(unsupported_stages_make_projection_incomplete) {
  const auto result = cvub::RunPostCVPipelineAIVProjection(
      cvub::test::ParseFixture("minimal_aiv.mlir"), {});
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Incomplete);
  CVUB_CHECK_EQ(result.coverage.size(), 14U);
  CVUB_CHECK(HasDiagnostic(result, "TileCubeVectorLoop", "unsupported"));
}

CVUB_TEST(replace_all_uses_updates_dps_and_block_edges) {
  auto module = cvub::test::ParseFixture("generic_rewrite.mlir");
  const int oldValue = ResultOf(module, "test.old");
  const int newValue = ResultOf(module, "test.new");
  cvub::GenericOperation &dps =
      module.operations.at(static_cast<size_t>(OperationId(module, "test.dps")));
  dps.dpsInputs = {oldValue};
  dps.dpsInits = {oldValue};

  cvub::ReplaceAllUses(module, oldValue, newValue);

  CVUB_CHECK(!cvub::ModuleReferencesValue(module, oldValue));
  cvub::ValidateGenericModule(module);
}

CVUB_TEST(compaction_remaps_dps_values) {
  auto module = cvub::test::ParseFixture("generic_rewrite.mlir");
  const int oldValue = ResultOf(module, "test.old");
  cvub::GenericOperation &dps =
      module.operations.at(static_cast<size_t>(OperationId(module, "test.dps")));
  dps.dpsInputs = {oldValue};
  dps.dpsInits = {oldValue};

  cvub::EraseOperationTree(module, OperationId(module, "test.dead"));
  module = cvub::CompactGenericModule(std::move(module));

  cvub::ValidateGenericModule(module);
  const cvub::GenericOperation &compactedDps = module.operations.at(
      static_cast<size_t>(OperationId(module, "test.dps")));
  CVUB_CHECK_EQ(compactedDps.dpsInputs, compactedDps.dpsInits);
  CVUB_CHECK_EQ(compactedDps.dpsInputs.size(), 1U);
  CVUB_CHECK(std::find(compactedDps.operands.begin(), compactedDps.operands.end(),
                       compactedDps.dpsInputs.front()) !=
             compactedDps.operands.end());
}

CVUB_TEST(compaction_rejects_references_to_erased_values) {
  auto module = cvub::test::ParseFixture("generic_rewrite.mlir");
  const int oldOperation = OperationId(module, "test.old");
  cvub::EraseOperationTree(module, oldOperation);
  CVUB_CHECK_THROWS_CONTAINS(cvub::CompactGenericModule(std::move(module)),
                             "unmapped value");
}

CVUB_TEST(erase_tree_and_move_before_keep_structure_consistent) {
  auto module = cvub::test::ParseFixture("two_mix_functions.mlir");
  cvub::EraseOperationTree(module, OperationId(module, "test.container"));
  module = cvub::CompactGenericModule(std::move(module));
  CVUB_CHECK_EQ(cvub::DeviceFunctionNames(module),
                std::vector<std::string>({"first_mix_aiv",
                                          "second_mix_aiv"}));

  const int first = OperationId(module, "test.first");
  const int second = OperationId(module, "test.second");
  cvub::MoveOperationBefore(module, second, first);
  const cvub::GenericBlock &block = module.blocks.at(
      static_cast<size_t>(module.operations.at(static_cast<size_t>(first)).blockId));
  CVUB_CHECK(std::find(block.operations.begin(), block.operations.end(), second) <
             std::find(block.operations.begin(), block.operations.end(), first));
  cvub::ValidateGenericModule(module);
}

CVUB_TEST(set_dictionary_value_replaces_or_appends) {
  CVUB_CHECK_EQ(cvub::SetDictionaryValue("{alpha = 1, beta = [2, 3]}",
                                         "alpha", "4"),
                "{alpha = 4, beta = [2, 3]}");
  CVUB_CHECK_EQ(cvub::SetDictionaryValue("{}", "core", "\"AIV\""),
                "{core = \"AIV\"}");
}

CVUB_TEST(extract_function_keeps_required_declarations_only) {
  auto module = cvub::test::ParseFixture("two_mix_functions.mlir");
  auto isolated = cvub::ExtractFunctionModule(module, "first_mix_aiv");
  CVUB_CHECK_EQ(cvub::DeviceFunctionNames(isolated),
                std::vector<std::string>({"first_mix_aiv"}));
  auto symbols = cvub::FunctionSymbolNames(isolated);
  std::sort(symbols.begin(), symbols.end());
  CVUB_CHECK_EQ(symbols,
                std::vector<std::string>({"first_mix_aiv", "used_decl"}));
  for (const cvub::GenericOperation &operation : isolated.operations)
    if (cvub::FunctionSymbolName(operation) == "used_decl")
      CVUB_CHECK(operation.regions.empty());
  cvub::ValidateGenericModule(isolated);
}

CVUB_TEST(validation_rejects_use_before_definition) {
  auto module = cvub::test::ParseFixture("generic_rewrite.mlir");
  auto &dead = module.operations.at(
      static_cast<size_t>(OperationId(module, "test.dead")));
  dead.operands = {ResultOf(module, "test.old")};
  dead.operandTypes = {"tensor<4xf32>"};
  CVUB_CHECK_THROWS_CONTAINS(cvub::ValidateGenericModule(module),
                             "does not dominate use");
}

CVUB_TEST(validation_rejects_cross_function_value_use) {
  auto module = cvub::test::ParseFixture("two_mix_functions.mlir");
  auto &first = module.operations.at(
      static_cast<size_t>(OperationId(module, "test.first")));
  first.results = {1000};
  first.resultTypes = {"i32"};
  const int secondFunction = [&] {
    for (const cvub::GenericOperation &operation : module.operations)
      if (cvub::FunctionSymbolName(operation) == "second_mix_aiv")
        return operation.id;
    throw std::runtime_error("missing second function");
  }();
  const int secondBlock = module.regions.at(static_cast<size_t>(
      module.operations.at(static_cast<size_t>(secondFunction)).regions.front()))
                              .blocks.front();
  auto &secondReturn = module.operations.at(static_cast<size_t>(
      module.blocks.at(static_cast<size_t>(secondBlock)).operations.front()));
  secondReturn.operands = {1000};
  secondReturn.operandTypes = {"i32"};
  CVUB_CHECK_THROWS_CONTAINS(cvub::ValidateGenericModule(module),
                             "not visible at use");
}

CVUB_TEST(validation_rejects_value_use_across_sibling_regions) {
  auto module = cvub::test::ParseFixture("two_mix_functions.mlir");
  const int containerId = OperationId(module, "test.container");
  auto &child = module.operations.at(
      static_cast<size_t>(OperationId(module, "test.child")));
  child.results = {1001};
  child.resultTypes = {"i32"};

  cvub::GenericRewriter rewriter(module);
  const int siblingRegion = rewriter.createRegion(containerId);
  const int siblingBlock = rewriter.createBlock(siblingRegion, {});
  const int siblingUse = rewriter.createOperation(
      containerId, siblingRegion, siblingBlock, "test.sibling_use", {}, {1001},
      {"i32"});
  rewriter.appendToBlock(siblingBlock, siblingUse);
  CVUB_CHECK_THROWS_CONTAINS(cvub::ValidateGenericModule(module),
                             "not visible at use");
}

CVUB_TEST(validation_rejects_operand_count_and_type_mismatch) {
  auto module = cvub::test::ParseFixture("generic_rewrite.mlir");
  auto &use = module.operations.at(
      static_cast<size_t>(OperationId(module, "test.use")));
  use.operandTypes.clear();
  CVUB_CHECK_THROWS_CONTAINS(cvub::ValidateGenericModule(module),
                             "operand/type count mismatch");

  module = cvub::test::ParseFixture("generic_rewrite.mlir");
  auto &typedUse = module.operations.at(
      static_cast<size_t>(OperationId(module, "test.use")));
  typedUse.operandTypes.front() = "i32";
  CVUB_CHECK_THROWS_CONTAINS(cvub::ValidateGenericModule(module),
                             "operand type mismatch");
}

CVUB_TEST(validation_rejects_bad_branch_forwarding) {
  auto module = cvub::test::ParseFixture("generic_rewrite.mlir");
  auto &branch = module.operations.at(
      static_cast<size_t>(OperationId(module, "cf.br")));
  branch.operands.clear();
  branch.operandTypes.clear();
  CVUB_CHECK_THROWS_CONTAINS(cvub::ValidateGenericModule(module),
                             "successor operand count mismatch");

  module = cvub::test::ParseFixture("generic_rewrite.mlir");
  auto &typedBranch = module.operations.at(
      static_cast<size_t>(OperationId(module, "cf.br")));
  typedBranch.operandTypes.front() = "i32";
  CVUB_CHECK_THROWS_CONTAINS(cvub::ValidateGenericModule(module),
                             "successor operand type mismatch");
}

CVUB_TEST(validation_rejects_bad_cond_branch_forwarding) {
  auto module = cvub::test::ParseFixture("generic_rewrite.mlir");
  auto &condition = module.operations.at(
      static_cast<size_t>(OperationId(module, "test.dead")));
  condition.resultTypes.front() = "i1";
  auto &branch = module.operations.at(
      static_cast<size_t>(OperationId(module, "cf.br")));
  const int target = branch.successors.front();
  branch.name = "cf.cond_br";
  branch.successors = {target, target};
  branch.operands = {condition.results.front(), branch.operands.front()};
  branch.operandTypes = {"i1", "tensor<4xf32>"};
  CVUB_CHECK_THROWS_CONTAINS(cvub::ValidateGenericModule(module),
                             "successor operand count mismatch");

  branch.operands.push_back(branch.operands.back());
  branch.operandTypes.push_back("i32");
  CVUB_CHECK_THROWS_CONTAINS(cvub::ValidateGenericModule(module),
                             "successor operand type mismatch");
}

CVUB_TEST(validation_rejects_duplicate_parent_links) {
  auto module = cvub::test::ParseFixture("generic_rewrite.mlir");
  auto &function = module.operations.at(static_cast<size_t>(1));
  function.regions.push_back(function.regions.front());
  CVUB_CHECK_THROWS_CONTAINS(cvub::ValidateGenericModule(module),
                             "region must have exactly one parent link");

  module = cvub::test::ParseFixture("generic_rewrite.mlir");
  auto &region = module.regions.at(static_cast<size_t>(
      module.operations.at(static_cast<size_t>(1)).regions.front()));
  region.blocks.push_back(region.blocks.front());
  CVUB_CHECK_THROWS_CONTAINS(cvub::ValidateGenericModule(module),
                             "block must have exactly one parent link");

  module = cvub::test::ParseFixture("generic_rewrite.mlir");
  auto &block = module.blocks.at(static_cast<size_t>(
      module.regions.at(static_cast<size_t>(
          module.operations.at(static_cast<size_t>(1)).regions.front()))
          .blocks.front()));
  block.operations.push_back(block.operations.front());
  CVUB_CHECK_THROWS_CONTAINS(cvub::ValidateGenericModule(module),
                             "operation must have exactly one parent link");
}

CVUB_TEST(device_enums_require_exact_payloads) {
  auto module = cvub::test::ParseFixture("generic_rewrite.mlir");
  auto &function = module.operations.at(static_cast<size_t>(1));
  function.attributes = cvub::SetDictionaryValue(
      function.attributes, "hacc.function_kind",
      "#hacc.function_kind<NOT_DEVICE>");
  CVUB_CHECK(cvub::DeviceFunctionNames(module).empty());
  CVUB_CHECK_THROWS_CONTAINS(cvub::ValidateBeforeCVPipelineBoundary(module),
                             "expected non-host MIX or AIV device function");

  module = cvub::test::ParseFixture("generic_rewrite.mlir");
  auto &wrongCore = module.operations.at(static_cast<size_t>(1));
  wrongCore.attributes = cvub::SetDictionaryValue(
      wrongCore.attributes, "hivm.func_core_type",
      "#hivm.func_core_type<NOT_AIV>");
  CVUB_CHECK_THROWS_CONTAINS(cvub::ValidateBeforeCVPipelineBoundary(module),
                             "expected non-host MIX or AIV device function");
}

CVUB_TEST(boundary_validation_rejects_post_bufferization_only_input) {
  auto module =
      cvub::test::ParseFixture("after_bufferization_only.mlir");
  CVUB_CHECK_THROWS_CONTAINS(
      cvub::ValidateBeforeCVPipelineBoundary(module),
      "expected tensor-level before-CVPipeline device function");
}

} // namespace

int main() { return cvub::test::RunAllTests(); }
