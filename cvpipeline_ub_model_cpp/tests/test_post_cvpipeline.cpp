#include "test_support.hpp"
#include "../src/post_cvpipeline/ir_utils.hpp"
#include "../src/post_cvpipeline/pipeline.hpp"
#include "../src/post_cvpipeline/split_mix_aiv.hpp"
#include "../src/post_cvpipeline/tile_cube_vector_loop.hpp"

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

int FunctionId(const cvub::GenericModule &module, const std::string &name) {
  for (const cvub::GenericOperation &operation : module.operations)
    if (operation.name == "func.func" &&
        cvub::FunctionSymbolName(operation) == name)
      return operation.id;
  throw std::runtime_error("missing function: " + name);
}

const cvub::GenericOperation &DefinitionOf(const cvub::GenericModule &module,
                                           int value) {
  for (const cvub::GenericOperation &operation : module.operations)
    if (std::find(operation.results.begin(), operation.results.end(), value) !=
        operation.results.end())
      return operation;
  throw std::runtime_error("missing value definition");
}

bool HasOperation(const cvub::GenericModule &module,
                  const std::string &operation) {
  return std::any_of(module.operations.begin(), module.operations.end(),
                     [&](const cvub::GenericOperation &candidate) {
                       return candidate.name == operation;
                     });
}

bool HasOperationCase(const cvub::GenericModule &module,
                      const std::string &caseName) {
  return std::any_of(module.operations.begin(), module.operations.end(),
                     [&](const cvub::GenericOperation &operation) {
                       return cvub::UnquoteIRString(cvub::IRDictionaryValue(
                                  operation.attributes, "case")) == caseName;
                     });
}

bool HasUnitAttribute(const std::string &dictionary,
                      const std::string &name) {
  if (dictionary.size() < 2)
    return false;
  for (const std::string &entry : cvub::splitTopLevel(
           dictionary.substr(1, dictionary.size() - 2)))
    if (cvub::trim(entry) == name)
      return true;
  return false;
}

const cvub::GenericOperation &OperationWithCase(
    const cvub::GenericModule &module, const std::string &caseName) {
  for (const cvub::GenericOperation &operation : module.operations)
    if (cvub::UnquoteIRString(
            cvub::IRDictionaryValue(operation.attributes, "case")) == caseName)
      return operation;
  throw std::runtime_error("missing operation case: " + caseName);
}

const cvub::GenericOperation &OperationNamed(
    const cvub::GenericModule &module, const std::string &name) {
  for (const cvub::GenericOperation &operation : module.operations)
    if (operation.name == name)
      return operation;
  throw std::runtime_error("missing operation: " + name);
}

size_t OperationCount(const cvub::GenericModule &module,
                      const std::string &name) {
  return static_cast<size_t>(std::count_if(
      module.operations.begin(), module.operations.end(),
      [&](const cvub::GenericOperation &operation) {
        return operation.name == name;
      }));
}

cvub::GenericOperation &MutableOperationNamed(cvub::GenericModule &module,
                                              const std::string &name,
                                              size_t occurrence = 0) {
  for (cvub::GenericOperation &operation : module.operations)
    if (operation.name == name && occurrence-- == 0)
      return operation;
  throw std::runtime_error("missing mutable operation: " + name);
}

void ExpectTileIncompleteAndUnchanged(const cvub::GenericModule &module,
                                      unsigned vectorTrips = 2,
                                      unsigned cubeTrips = 2) {
  const std::string before = cvub::SerializeGenericModule(module);
  const auto result =
      cvub::RunTileCubeVectorLoop(module, vectorTrips, cubeTrips);
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Incomplete);
  CVUB_CHECK_EQ(cvub::SerializeGenericModule(result.module), before);
  CVUB_CHECK(!result.diagnostics.empty());
}

void MakeCubeDimensionsSharedAndSquare(cvub::GenericModule &module) {
  for (cvub::GenericOperation &operation : module.operations) {
    const auto rewriteTypes = [](std::vector<std::string> &types) {
      for (std::string &type : types) {
        size_t position = std::string::npos;
        while ((position = type.find("128x4096")) != std::string::npos)
          type.replace(position, 8, "256x256");
        while ((position = type.find("64x4096")) != std::string::npos)
          type.replace(position, 7, "64x256");
        while ((position = type.find("128x64")) != std::string::npos)
          type.replace(position, 6, "256x64");
      }
    };
    rewriteTypes(operation.resultTypes);
    rewriteTypes(operation.operandTypes);
  }
  auto &subview = MutableOperationNamed(module, "memref.subview");
  subview.attributes = cvub::SetDictionaryValue(
      subview.attributes, "static_sizes", "[256, 256]");
  auto &mmad = MutableOperationNamed(module, "hivm.hir.mmadL1");
  auto &sharedDimension = module.operations.at(
      static_cast<size_t>(DefinitionOf(module, mmad.operands[5]).id));
  sharedDimension.attributes = cvub::SetDictionaryValue(
      sharedDimension.attributes, "value", "256 : index");
  sharedDimension.resultTypes = {"index"};
  const int sharedValue = sharedDimension.results.front();
  mmad.operands[3] = sharedValue;
  mmad.operands[5] = sharedValue;
  mmad.dpsInputs.clear();
  mmad.dpsInits.clear();
  cvub::ApplyOperationSemantics(mmad);
  cvub::ValidateGenericModule(module);
}

void DuplicateAnchorBeforeYield(cvub::GenericModule &module,
                                const std::string &anchorName) {
  const cvub::GenericOperation anchor =
      MutableOperationNamed(module, anchorName);
  const cvub::GenericOperation yield = MutableOperationNamed(module, "scf.yield");
  cvub::GenericRewriter rewriter(module);
  const int duplicate = rewriter.createOperation(
      anchor.parentId, anchor.regionId, anchor.blockId, anchor.name, {},
      anchor.operands, anchor.operandTypes, anchor.properties,
      anchor.attributes);
  const auto &operations =
      module.blocks.at(static_cast<size_t>(anchor.blockId)).operations;
  const auto position = static_cast<size_t>(std::distance(
      operations.begin(),
      std::find(operations.begin(), operations.end(), yield.id)));
  rewriter.insertToBlock(anchor.blockId, position, duplicate);
  cvub::ValidateGenericModule(module);
}

bool HasMarkOfCase(const cvub::GenericModule &module,
                   const std::string &caseName) {
  const int source = OperationWithCase(module, caseName).results.front();
  return std::any_of(module.operations.begin(), module.operations.end(),
                     [&](const cvub::GenericOperation &operation) {
                       return operation.name == "annotation.mark" &&
                              !operation.operands.empty() &&
                              operation.operands.front() == source;
                     });
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

CVUB_TEST(default_vector_tiling_uses_ceil_div_and_shrinks_local_alloc) {
  const auto result = cvub::RunTileCubeVectorLoop(
      cvub::test::ParseFixture("tile_vector_loop.mlir"), 2, 2);
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
  CVUB_CHECK_EQ(OperationNamed(result.module, "hivm.hir.vadd").resultTypes,
                std::vector<std::string>({"tensor<1x64xf16>"}));
  CVUB_CHECK_EQ(OperationNamed(result.module, "memref.alloc").resultTypes,
                std::vector<std::string>({"memref<1x64xf16>"}));
  const auto &slice = OperationNamed(result.module, "tensor.extract_slice");
  CVUB_CHECK_EQ(cvub::IRDictionaryValue(slice.attributes, "static_offsets"),
                "[0, -9223372036854775808]");
  CVUB_CHECK_EQ(cvub::IRDictionaryValue(slice.attributes, "static_sizes"),
                "[1, 64]");
  CVUB_CHECK_EQ(cvub::IRDictionaryValue(slice.attributes, "static_strides"),
                "[1, 1]");
  CVUB_CHECK_EQ(OperationCount(result.module, "scf.for"), 2U);
  CVUB_CHECK_EQ(OperationWithCase(result.module, "unrelated").resultTypes,
                std::vector<std::string>({"tensor<1x128xf16>"}));
  const auto &source = DefinitionOf(
      result.module, OperationNamed(result.module, "tensor.extract_slice")
                         .operands.front());
  CVUB_CHECK_EQ(source.resultTypes,
                std::vector<std::string>({"tensor<1x128xf16>"}));
  CVUB_CHECK_EQ(result.module.operations.at(static_cast<size_t>(source.parentId))
                    .name,
                "func.func");
  CVUB_CHECK_EQ(OperationCount(result.module, "memref.subview"), 0U);
  CVUB_CHECK_EQ(DefinitionOf(result.module,
      OperationNamed(result.module, "hivm.hir.store").operands[1]).name,
                "memref.alloc");
  const auto &vadd = OperationNamed(result.module, "hivm.hir.vadd");
  const auto &inner = result.module.operations.at(
      static_cast<size_t>(vadd.parentId));
  CVUB_CHECK_EQ(inner.name, "scf.for");
  cvub::ValidateGenericModule(result.module);
}

CVUB_TEST(vector_axis_is_proven_from_store_dependency_not_shape_position) {
  auto mAxis = cvub::test::ParseFixture("tile_vector_loop.mlir");
  for (auto &operation : mAxis.operations) {
    for (std::string &type : operation.resultTypes) {
      if (type == "tensor<1x128xf16>") type = "tensor<128x1xf16>";
      if (type == "memref<1x128xf16>") type = "memref<128x1xf16>";
    }
    for (std::string &type : operation.operandTypes) {
      if (type == "tensor<1x128xf16>") type = "tensor<128x1xf16>";
      if (type == "memref<1x128xf16>") type = "memref<128x1xf16>";
    }
    if (operation.name == "tensor.extract_slice" ||
        operation.name == "memref.subview")
      operation.attributes = cvub::SetDictionaryValue(
          operation.attributes, "static_sizes", "[128, 1]");
  }
  const auto exact = cvub::RunTileCubeVectorLoop(mAxis, 2, 2);
  CVUB_CHECK_EQ(exact.precision, cvub::Precision::Exact);
  CVUB_CHECK_EQ(OperationNamed(exact.module, "hivm.hir.vadd").resultTypes,
                std::vector<std::string>({"tensor<64x1xf16>"}));

  auto ambiguous = cvub::test::ParseFixture("tile_vector_loop.mlir");
  for (auto &operation : ambiguous.operations) {
    for (std::string &type : operation.resultTypes)
      if (type == "tensor<1x128xf16>") type = "tensor<64x128xf16>";
    for (std::string &type : operation.operandTypes)
      if (type == "tensor<1x128xf16>") type = "tensor<64x128xf16>";
  }
  ExpectTileIncompleteAndUnchanged(ambiguous);
}

CVUB_TEST(tiling_moves_only_anchor_producer_closure) {
  auto module = cvub::test::ParseFixture("tile_vector_loop.mlir");
  const auto &yield = OperationNamed(module, "scf.yield");
  cvub::GenericRewriter rewriter(module);
  const int unrelated = rewriter.createOperation(
      yield.parentId, yield.regionId, yield.blockId, "tensor.empty",
      {"tensor<1x128xf16>"}, {}, {}, "", "{case = \"inner_unrelated\"}");
  const auto &ops = module.blocks.at(static_cast<size_t>(yield.blockId)).operations;
  const size_t position = static_cast<size_t>(std::distance(
      ops.begin(), std::find(ops.begin(), ops.end(), yield.id)));
  rewriter.insertToBlock(yield.blockId, position, unrelated);
  cvub::ValidateGenericModule(module);

  const auto result = cvub::RunTileCubeVectorLoop(module, 2, 2);
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
  const auto &kept = OperationWithCase(result.module, "inner_unrelated");
  CVUB_CHECK_EQ(kept.resultTypes,
                std::vector<std::string>({"tensor<1x128xf16>"}));
  const auto &parent = result.module.operations.at(static_cast<size_t>(kept.parentId));
  CVUB_CHECK_EQ(parent.name, "scf.for");
  CVUB_CHECK(!cvub::IRDictionaryValue(parent.attributes,
                                      "hivm.loop_core_type").empty());
}

CVUB_TEST(dynamic_alloc_and_dynamic_gm_boundaries_fail_closed) {
  auto dynamicAlloc = cvub::test::ParseFixture("tile_vector_loop.mlir");
  auto &alloc = MutableOperationNamed(dynamicAlloc, "memref.alloc");
  alloc.resultTypes.front() = "memref<?x128xf16>";
  MutableOperationNamed(dynamicAlloc, "memref.subview").operandTypes.front() =
      "memref<?x128xf16>";
  ExpectTileIncompleteAndUnchanged(dynamicAlloc);

  auto dynamicSubview = cvub::test::ParseFixture("tile_vector_loop.mlir");
  auto &view = MutableOperationNamed(dynamicSubview, "memref.subview");
  view.attributes = cvub::SetDictionaryValue(
      view.attributes, "static_sizes", "[1, -9223372036854775808]");
  ExpectTileIncompleteAndUnchanged(dynamicSubview);
}

CVUB_TEST(cube_fit_uses_fixpipe_destination_element_type) {
  auto module = cvub::test::ParseFixture("tile_cube_loop.mlir");
  auto &specAttributes = module.operations.front().attributes;
  specAttributes.replace(specAttributes.find("1048576"), 7, "10000000");
  auto &fixpipe = MutableOperationNamed(module, "hivm.hir.fixpipe");
  fixpipe.operandTypes[1] = "memref<128x4096xf16>";
  auto &view = MutableOperationNamed(module, "memref.subview");
  view.resultTypes.front() = "memref<128x4096xf16>";
  auto &alloc = MutableOperationNamed(module, "memref.alloc");
  alloc.resultTypes.front() = "memref<128x4096xf16>";
  view.operandTypes.front() = "memref<128x4096xf16>";
  const auto result = cvub::RunTileCubeVectorLoop(module, 2, 2);
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
  CVUB_CHECK_EQ(OperationCount(result.module, "scf.for"), 1U);
}

CVUB_TEST(default_cube_tiling_uses_ceil_div_and_fuses_mmad) {
  auto input = cvub::test::ParseFixture("tile_cube_loop.mlir");
  auto &inputMmad = MutableOperationNamed(input, "hivm.hir.mmadL1");
  cvub::ApplyOperationSemantics(inputMmad);
  CVUB_CHECK_EQ(inputMmad.operands.size(), 7U);
  CVUB_CHECK_EQ(cvub::DpsInitOperandIndices(
                    inputMmad.name, inputMmad.operands.size(),
                    inputMmad.properties),
                std::vector<size_t>({6U}));
  CVUB_CHECK_EQ(inputMmad.dpsInits,
                std::vector<int>({inputMmad.operands[6]}));
  const auto result = cvub::RunTileCubeVectorLoop(
      input, 2, 2);
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
  CVUB_CHECK_EQ(OperationNamed(result.module, "hivm.hir.mmadL1").resultTypes,
                std::vector<std::string>({"tensor<128x2048xf32>"}));
  const auto &matrix = OperationNamed(result.module, "hivm.hir.mmadL1");
  CVUB_CHECK_EQ(matrix.operands.size(), 7U);
  CVUB_CHECK_EQ(cvub::DpsInitOperandIndices(
                    matrix.name, matrix.operands.size(), matrix.properties),
                std::vector<size_t>({6U}));
  CVUB_CHECK_EQ(matrix.dpsInits, std::vector<int>({matrix.operands[6]}));
  CVUB_CHECK_EQ(DefinitionOf(result.module, matrix.operands[5])
                    .name,
                "affine.min");
  CVUB_CHECK_EQ(cvub::IRDictionaryValue(
                    DefinitionOf(result.module, matrix.operands[3])
                        .attributes,
                    "value"),
                "128 : index");
  CVUB_CHECK_EQ(cvub::IRDictionaryValue(
                    DefinitionOf(result.module, matrix.operands[4])
                        .attributes,
                    "value"),
                "64 : index");
  const auto &rhs = DefinitionOf(result.module, matrix.operands[1]);
  CVUB_CHECK_EQ(rhs.resultTypes,
                std::vector<std::string>({"tensor<64x2048xf16>"}));
  CVUB_CHECK_EQ(result.module.operations.at(static_cast<size_t>(matrix.parentId))
                    .name,
                "scf.for");
  cvub::ValidateGenericModule(result.module);
}

CVUB_TEST(cube_rejects_legacy_four_operand_synthetic_mmad) {
  auto module = cvub::test::ParseFixture("tile_cube_loop.mlir");
  auto &legacy = MutableOperationNamed(module, "hivm.hir.mmadL1");
  const std::vector<int> originalOperands = legacy.operands;
  const std::vector<std::string> originalTypes = legacy.operandTypes;
  legacy.operands = {originalOperands[0], originalOperands[1],
                     originalOperands[2], originalOperands[6]};
  legacy.operandTypes = {originalTypes[0], originalTypes[1], originalTypes[2],
                         originalTypes[6]};
  legacy.dpsInputs = {legacy.operands[0], legacy.operands[1],
                      legacy.operands[2]};
  legacy.dpsInits = {legacy.operands[3]};
  legacy.properties = cvub::SetDictionaryValue(
      legacy.properties, "operandSegmentSizes", "array<i32: 1, 1, 1, 1>");
  legacy.attributes = cvub::SetDictionaryValue(
      legacy.attributes, "operandSegmentSizes", "array<i32: 1, 1, 1, 1>");
  cvub::ValidateGenericModule(module);
  ExpectTileIncompleteAndUnchanged(module);

  auto dynamicInit = module;
  auto &dynamicLegacy = MutableOperationNamed(dynamicInit, "hivm.hir.mmadL1");
  const auto &wrapper = OperationNamed(dynamicInit, "scf.for");
  const int body = dynamicInit.regions.at(
      static_cast<size_t>(wrapper.regions.front())).blocks.front();
  dynamicLegacy.operands[2] =
      dynamicInit.blocks.at(static_cast<size_t>(body)).arguments.front();
  dynamicLegacy.operandTypes[2] = "index";
  dynamicLegacy.dpsInputs[2] = dynamicLegacy.operands[2];
  cvub::ValidateGenericModule(dynamicInit);
  ExpectTileIncompleteAndUnchanged(dynamicInit);

  ExpectTileIncompleteAndUnchanged(module, 2, 1);
}

CVUB_TEST(cube_shared_real_dimensions_update_only_the_n_operand_and_metadata) {
  auto module = cvub::test::ParseFixture("tile_cube_loop.mlir");
  MakeCubeDimensionsSharedAndSquare(module);
  const auto &before = OperationNamed(module, "hivm.hir.mmadL1");
  const int shared = before.operands[3];
  CVUB_CHECK_EQ(before.operands[5], shared);
  CVUB_CHECK_EQ(before.dpsInputs[3], shared);
  CVUB_CHECK_EQ(before.dpsInputs[5], shared);

  const auto result = cvub::RunTileCubeVectorLoop(module, 2, 2);
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
  const auto &matrix = OperationNamed(result.module, "hivm.hir.mmadL1");
  CVUB_CHECK_EQ(matrix.operands[3], shared);
  CVUB_CHECK(matrix.operands[5] != shared);
  CVUB_CHECK_EQ(matrix.dpsInputs[3], shared);
  CVUB_CHECK_EQ(matrix.dpsInputs[5], matrix.operands[5]);
  const auto &minimum = DefinitionOf(result.module, matrix.operands[5]);
  CVUB_CHECK_EQ(cvub::IRDictionaryValue(minimum.properties, "map"),
                "affine_map<(d0) -> (-d0 + 256, 128)>");
  CVUB_CHECK(cvub::IRDictionaryValue(minimum.attributes, "map").empty());
  CVUB_CHECK_EQ(minimum.operands.size(), 1U);
  CVUB_CHECK_EQ(minimum.operandTypes, std::vector<std::string>({"index"}));
  CVUB_CHECK_EQ(minimum.resultTypes, std::vector<std::string>({"index"}));
}

CVUB_TEST(cube_real_dimensions_must_match_the_static_matrix_shape) {
  auto stale = cvub::test::ParseFixture("tile_cube_loop.mlir");
  const auto &staleMmad = OperationNamed(stale, "hivm.hir.mmadL1");
  auto &staleRealN = stale.operations.at(
      static_cast<size_t>(DefinitionOf(stale, staleMmad.operands[5]).id));
  staleRealN.attributes = cvub::SetDictionaryValue(
      staleRealN.attributes, "value", "4095 : index");
  ExpectTileIncompleteAndUnchanged(stale);

  auto dynamic = cvub::test::ParseFixture("tile_cube_loop.mlir");
  auto &dynamicMmad = MutableOperationNamed(dynamic, "hivm.hir.mmadL1");
  const auto &wrapper = OperationNamed(dynamic, "scf.for");
  const int body = dynamic.regions.at(
      static_cast<size_t>(wrapper.regions.front())).blocks.front();
  dynamicMmad.operands[5] =
      dynamic.blocks.at(static_cast<size_t>(body)).arguments.front();
  cvub::ValidateGenericModule(dynamic);
  ExpectTileIncompleteAndUnchanged(dynamic);
}

CVUB_TEST(cube_m_axis_is_forced_by_inside_a_load_dependency) {
  auto module = cvub::test::ParseFixture("tile_cube_loop.mlir");
  auto &mmad = MutableOperationNamed(module, "hivm.hir.mmadL1");
  auto &load = MutableOperationNamed(module, "hivm.hir.load");
  const int outsideA = mmad.operands[0];
  const int outsideB = load.operands[0];
  const int loaded = mmad.operands[1];
  auto &loadInit = module.operations.at(
      static_cast<size_t>(DefinitionOf(module, load.operands[1]).id));
  loadInit.resultTypes.front() = "tensor<128x64xf16>";
  load.operands[0] = outsideA;
  load.operandTypes = {"tensor<128x64xf16>", "tensor<128x64xf16>"};
  load.resultTypes.front() = "tensor<128x64xf16>";
  mmad.operands[0] = loaded;
  mmad.operandTypes[0] = "tensor<128x64xf16>";
  mmad.operands[1] = outsideB;
  mmad.operandTypes[1] = "tensor<64x4096xf16>";
  cvub::ValidateGenericModule(module);

  const auto result = cvub::RunTileCubeVectorLoop(module, 2, 2);
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
  const auto &tiledMmad = OperationNamed(result.module, "hivm.hir.mmadL1");
  CVUB_CHECK_EQ(tiledMmad.resultTypes,
                std::vector<std::string>({"tensor<64x4096xf32>"}));
  CVUB_CHECK_EQ(tiledMmad.operandTypes[0], "tensor<64x64xf16>");
  CVUB_CHECK_EQ(tiledMmad.operandTypes[1], "tensor<64x4096xf16>");
  CVUB_CHECK_EQ(DefinitionOf(result.module, tiledMmad.operands[3]).name,
                "affine.min");
  CVUB_CHECK_EQ(cvub::IRDictionaryValue(
                    DefinitionOf(result.module, tiledMmad.operands[4])
                        .attributes,
                    "value"),
                "64 : index");
  CVUB_CHECK_EQ(cvub::IRDictionaryValue(
                    DefinitionOf(result.module, tiledMmad.operands[5])
                        .attributes,
                    "value"),
                "4096 : index");
  cvub::ValidateGenericModule(result.module);
}

CVUB_TEST(cube_shared_real_dimensions_update_only_the_m_operand_and_metadata) {
  auto module = cvub::test::ParseFixture("tile_cube_loop.mlir");
  MakeCubeDimensionsSharedAndSquare(module);
  auto &mmad = MutableOperationNamed(module, "hivm.hir.mmadL1");
  auto &load = MutableOperationNamed(module, "hivm.hir.load");
  const int outsideA = mmad.operands[0];
  const int outsideB = load.operands[0];
  const int loaded = mmad.operands[1];
  auto &loadInit = module.operations.at(
      static_cast<size_t>(DefinitionOf(module, load.operands[1]).id));
  loadInit.resultTypes.front() = "tensor<256x64xf16>";
  load.operands[0] = outsideA;
  load.operandTypes = {"tensor<256x64xf16>", "tensor<256x64xf16>"};
  load.resultTypes.front() = "tensor<256x64xf16>";
  load.dpsInputs.clear();
  load.dpsInits.clear();
  load.effects.clear();
  cvub::ApplyOperationSemantics(load);
  mmad.operands[0] = loaded;
  mmad.operandTypes[0] = "tensor<256x64xf16>";
  mmad.operands[1] = outsideB;
  mmad.operandTypes[1] = "tensor<64x256xf16>";
  mmad.dpsInputs.clear();
  mmad.dpsInits.clear();
  cvub::ApplyOperationSemantics(mmad);
  const int shared = mmad.operands[3];
  CVUB_CHECK_EQ(mmad.operands[5], shared);
  cvub::ValidateGenericModule(module);

  const auto result = cvub::RunTileCubeVectorLoop(module, 2, 2);
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
  const auto &matrix = OperationNamed(result.module, "hivm.hir.mmadL1");
  CVUB_CHECK(matrix.operands[3] != shared);
  CVUB_CHECK_EQ(matrix.operands[5], shared);
  CVUB_CHECK_EQ(matrix.dpsInputs[3], matrix.operands[3]);
  CVUB_CHECK_EQ(matrix.dpsInputs[5], shared);
  const auto &minimum = DefinitionOf(result.module, matrix.operands[3]);
  CVUB_CHECK_EQ(cvub::IRDictionaryValue(minimum.properties, "map"),
                "affine_map<(d0) -> (-d0 + 256, 128)>");
  CVUB_CHECK(cvub::IRDictionaryValue(minimum.attributes, "map").empty());
  CVUB_CHECK_EQ(minimum.operands.size(), 1U);
  CVUB_CHECK_EQ(minimum.operandTypes, std::vector<std::string>({"index"}));
  CVUB_CHECK_EQ(minimum.resultTypes, std::vector<std::string>({"index"}));
}

CVUB_TEST(cube_fixpipe_nz2nd_is_supported_but_other_dma_modes_fail_closed) {
  const auto supported = cvub::RunTileCubeVectorLoop(
      cvub::test::ParseFixture("tile_cube_loop.mlir"), 2, 2);
  CVUB_CHECK_EQ(supported.precision, cvub::Precision::Exact);
  CVUB_CHECK_EQ(cvub::IRDictionaryValue(
                    OperationNamed(supported.module, "hivm.hir.fixpipe")
                        .attributes,
                    "dma_mode"),
                "#hivm.dma_mode<nz2nd>");

  auto unsupported = cvub::test::ParseFixture("tile_cube_loop.mlir");
  auto &fixpipe = MutableOperationNamed(unsupported, "hivm.hir.fixpipe");
  fixpipe.attributes = cvub::SetDictionaryValue(
      fixpipe.attributes, "dma_mode", "#hivm.dma_mode<nz2dn>");
  fixpipe.properties = cvub::SetDictionaryValue(
      fixpipe.properties, "dma_mode", "#hivm.dma_mode<nz2dn>");
  ExpectTileIncompleteAndUnchanged(unsupported);
}

CVUB_TEST(cube_inside_load_must_prove_the_group_axis) {
  auto module = cvub::test::ParseFixture("tile_cube_loop.mlir");
  auto &load = MutableOperationNamed(module, "hivm.hir.load");
  load.attributes = cvub::SetDictionaryValue(load.attributes, "transpose",
                                               "[1, 0]");
  ExpectTileIncompleteAndUnchanged(module);
}

CVUB_TEST(identity_dependencies_reject_axis_changes_anywhere_in_chain) {
  auto upstreamTranspose = cvub::test::ParseFixture("tile_vector_loop.mlir");
  auto &load = MutableOperationNamed(upstreamTranspose, "hivm.hir.load");
  load.attributes = cvub::SetDictionaryValue(load.attributes, "transpose",
                                              "[1, 0]");
  ExpectTileIncompleteAndUnchanged(upstreamTranspose);

  auto upstreamImplicit = cvub::test::ParseFixture("tile_vector_loop.mlir");
  auto &implicitLoad = MutableOperationNamed(upstreamImplicit, "hivm.hir.load");
  implicitLoad.attributes = cvub::SetDictionaryValue(
      implicitLoad.attributes, "may_implicit_transpose_with_last_axis", "true");
  ExpectTileIncompleteAndUnchanged(upstreamImplicit);

  auto storeImplicit = cvub::test::ParseFixture("tile_vector_loop.mlir");
  auto &store = MutableOperationNamed(storeImplicit, "hivm.hir.store");
  store.attributes = cvub::SetDictionaryValue(
      store.attributes, "may_implicit_transpose_with_last_axis", "true");
  ExpectTileIncompleteAndUnchanged(storeImplicit);

  auto cubeImplicit = cvub::test::ParseFixture("tile_cube_loop.mlir");
  auto &cubeLoad = MutableOperationNamed(cubeImplicit, "hivm.hir.load");
  cubeLoad.attributes = cvub::SetDictionaryValue(
      cubeLoad.attributes, "may_implicit_transpose_with_last_axis", "true");
  ExpectTileIncompleteAndUnchanged(cubeImplicit);

  auto unknownAxis = cvub::test::ParseFixture("tile_vector_loop.mlir");
  auto &sum = MutableOperationNamed(unknownAxis, "hivm.hir.vadd");
  sum.attributes = cvub::SetDictionaryValue(sum.attributes,
                                             "test.axis_mapping", "identity");
  ExpectTileIncompleteAndUnchanged(unknownAxis);
}

CVUB_TEST(cube_global_axis_ambiguity_fails_closed) {
  auto module = cvub::test::ParseFixture("tile_cube_loop.mlir");
  auto &mmad = MutableOperationNamed(module, "hivm.hir.mmadL1");
  const auto rhsLoad = MutableOperationNamed(module, "hivm.hir.load");
  const auto &yield = MutableOperationNamed(module, "scf.yield");
  cvub::GenericRewriter rewriter(module);
  const int lhsInit = rewriter.createOperation(
      yield.parentId, yield.regionId, yield.blockId, "tensor.empty",
      {"tensor<128x64xf16>"});
  const int lhsLoad = rewriter.createOperation(
      yield.parentId, yield.regionId, yield.blockId, rhsLoad.name,
      {"tensor<128x64xf16>"}, {mmad.operands[0],
                                module.operations.at(static_cast<size_t>(lhsInit))
                                    .results.front()},
      {"tensor<128x64xf16>", "tensor<128x64xf16>"}, "",
      "{may_implicit_transpose_with_last_axis = false}");
  const auto &ops = module.blocks.at(static_cast<size_t>(yield.blockId)).operations;
  const size_t position = static_cast<size_t>(std::distance(
      ops.begin(), std::find(ops.begin(), ops.end(), rhsLoad.id)));
  rewriter.insertToBlock(yield.blockId, position, lhsInit);
  rewriter.insertToBlock(yield.blockId, position + 1, lhsLoad);
  MutableOperationNamed(module, "hivm.hir.mmadL1").operands[0] =
      module.operations.at(static_cast<size_t>(lhsLoad)).results.front();
  cvub::ValidateGenericModule(module);
  ExpectTileIncompleteAndUnchanged(module);
}

CVUB_TEST(direct_local_destination_moves_once_and_shrinks) {
  auto module = cvub::test::ParseFixture("tile_vector_loop.mlir");
  auto &alloc = MutableOperationNamed(module, "memref.alloc");
  auto &view = MutableOperationNamed(module, "memref.subview");
  auto &store = MutableOperationNamed(module, "hivm.hir.store");
  store.operands[1] = alloc.results.front();
  store.operandTypes[1] = alloc.resultTypes.front();
  cvub::EraseOperationTree(module, view.id);
  module = cvub::CompactGenericModule(std::move(module));
  cvub::ValidateGenericModule(module);

  const auto result = cvub::RunTileCubeVectorLoop(module, 2, 2);
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
  CVUB_CHECK_EQ(OperationCount(result.module, "memref.alloc"), 1U);
  const auto &movedAlloc = OperationNamed(result.module, "memref.alloc");
  CVUB_CHECK_EQ(movedAlloc.resultTypes,
                std::vector<std::string>({"memref<1x64xf16>"}));
  CVUB_CHECK_EQ(result.module.operations.at(
                    static_cast<size_t>(movedAlloc.parentId)).name,
                "scf.for");
  const auto &parentBlock = result.module.blocks.at(
      static_cast<size_t>(movedAlloc.blockId)).operations;
  CVUB_CHECK_EQ(static_cast<size_t>(std::count(parentBlock.begin(),
                                               parentBlock.end(),
                                               movedAlloc.id)),
                1U);
  cvub::ValidateGenericModule(result.module);
}

CVUB_TEST(cube_moves_only_the_selected_axis_branch) {
  auto module = cvub::test::ParseFixture("tile_cube_loop.mlir");
  auto &loop = MutableOperationNamed(module, "scf.for");
  auto &lhs = MutableOperationNamed(module, "tensor.empty", 0);
  auto &matmul = MutableOperationNamed(module, "hivm.hir.mmadL1");
  cvub::GenericRewriter rewriter(module);
  rewriter.removeFromBlock(lhs.blockId, lhs.id);
  const auto &ops = module.blocks.at(static_cast<size_t>(matmul.blockId)).operations;
  const size_t position = static_cast<size_t>(std::distance(
      ops.begin(), std::find(ops.begin(), ops.end(), matmul.id)));
  rewriter.insertToBlock(matmul.blockId, position, lhs.id);
  lhs.parentId = loop.id;
  lhs.regionId = matmul.regionId;
  lhs.blockId = matmul.blockId;
  cvub::ValidateGenericModule(module);

  const auto result = cvub::RunTileCubeVectorLoop(module, 2, 2);
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
  const auto &kept = DefinitionOf(result.module,
      OperationNamed(result.module, "hivm.hir.mmadL1").operands[0]);
  CVUB_CHECK_EQ(result.module.operations.at(static_cast<size_t>(kept.parentId)).id,
                OperationId(result.module, "scf.for"));
}

CVUB_TEST(tile_walk_rejects_nested_fixpipe_and_external_structured_ops) {
  auto nested = cvub::test::ParseFixture("tile_cube_loop.mlir");
  auto &fixpipe = MutableOperationNamed(nested, "hivm.hir.fixpipe");
  fixpipe.parentId = MutableOperationNamed(nested, "hivm.hir.load").id;
  ExpectTileIncompleteAndUnchanged(nested);

  auto external = cvub::test::ParseFixture("tile_vector_loop.mlir");
  const auto load = MutableOperationNamed(external, "hivm.hir.load");
  const auto yield = MutableOperationNamed(external, "scf.yield");
  cvub::GenericRewriter rewriter(external);
  const int unused = rewriter.createOperation(
      yield.parentId, yield.regionId, yield.blockId, load.name,
      load.resultTypes, load.operands, load.operandTypes, load.properties,
      load.attributes);
  const auto &ops = external.blocks.at(static_cast<size_t>(yield.blockId)).operations;
  rewriter.insertToBlock(yield.blockId,
      static_cast<size_t>(std::distance(ops.begin(),
          std::find(ops.begin(), ops.end(), yield.id))), unused);
  cvub::ValidateGenericModule(external);
  ExpectTileIncompleteAndUnchanged(external);
}

CVUB_TEST(vector_local_destination_shrinks_but_gm_boundary_is_sliced) {
  auto local = cvub::RunTileCubeVectorLoop(
      cvub::test::ParseFixture("tile_vector_loop.mlir"), 2, 2);
  CVUB_CHECK_EQ(local.precision, cvub::Precision::Exact);
  CVUB_CHECK_EQ(OperationNamed(local.module, "memref.alloc").resultTypes,
                std::vector<std::string>({"memref<1x64xf16>"}));

  auto gm = cvub::test::ParseFixture("tile_vector_loop.mlir");
  auto &function = MutableOperationNamed(gm, "func.func");
  auto &entry = gm.blocks.at(static_cast<size_t>(
      gm.regions.at(static_cast<size_t>(function.regions.front())).blocks.front()));
  int nextValue = 0;
  for (const auto &operation : gm.operations)
    for (int value : operation.results) nextValue = std::max(nextValue, value + 1);
  for (const auto &block : gm.blocks)
    for (int value : block.arguments) nextValue = std::max(nextValue, value + 1);
  entry.arguments.push_back(nextValue);
  entry.argumentTypes.push_back("memref<1x128xf16>");
  auto &store = MutableOperationNamed(gm, "hivm.hir.store");
  store.operands[1] = nextValue;
  store.operandTypes[1] = "memref<1x128xf16>";
  cvub::ValidateGenericModule(gm);
  const auto boundary = cvub::RunTileCubeVectorLoop(gm, 2, 2);
  CVUB_CHECK_EQ(boundary.precision, cvub::Precision::Exact);
  const auto &tile = OperationNamed(boundary.module, "memref.subview");
  CVUB_CHECK_EQ(cvub::IRDictionaryValue(tile.attributes, "static_offsets"),
                "[0, -9223372036854775808]");
}

CVUB_TEST(vector_alignment_checks_boundary_operands_and_rejects_shape_only_axis) {
  auto operand = cvub::test::ParseFixture("tile_vector_loop.mlir");
  auto &source = MutableOperationNamed(operand, "tensor.empty", 0);
  source.resultTypes.front() = "tensor<1x128xf8>";
  auto &slice = MutableOperationNamed(operand, "tensor.extract_slice");
  slice.operandTypes.front() = "tensor<1x128xf8>";
  auto &spec = operand.operations.front().attributes;
  const size_t align = spec.find("256 : i32", spec.find("UB_ALIGN_SIZE"));
  spec.replace(align, 9, "1024 : i32");
  const auto operandRollback = cvub::RunTileCubeVectorLoop(operand, 2, 2);
  CVUB_CHECK_EQ(operandRollback.precision, cvub::Precision::Exact);
  CVUB_CHECK_EQ(OperationCount(operandRollback.module, "scf.for"), 1U);
  CVUB_CHECK_EQ(OperationCount(operandRollback.module, "memref.subview"), 0U);
  CVUB_CHECK_EQ(OperationNamed(operandRollback.module, "tensor.empty").resultTypes,
                std::vector<std::string>({"tensor<1x128xf8>"}));

  auto shapeOnly = cvub::test::ParseFixture("tile_vector_loop.mlir");
  auto &store = MutableOperationNamed(shapeOnly, "hivm.hir.store");
  auto &empty = MutableOperationNamed(shapeOnly, "tensor.empty", 2);
  store.operands[0] = empty.results.front();
  store.operandTypes[0] = empty.resultTypes.front();
  cvub::ValidateGenericModule(shapeOnly);
  ExpectTileIncompleteAndUnchanged(shapeOnly);

  auto cubeShapeOnly = cvub::test::ParseFixture("tile_cube_loop.mlir");
  auto &cubeLoad = MutableOperationNamed(cubeShapeOnly, "hivm.hir.load");
  auto &cubeMmad = MutableOperationNamed(cubeShapeOnly, "hivm.hir.mmadL1");
  cubeMmad.operands[1] = cubeLoad.operands[0];
  cubeMmad.operandTypes[1] = cubeLoad.operandTypes[0];
  cvub::ValidateGenericModule(cubeShapeOnly);
  ExpectTileIncompleteAndUnchanged(cubeShapeOnly);
}

CVUB_TEST(vector_alignment_rollback_still_runs_global_shrink) {
  auto module = cvub::test::ParseFixture("tile_vector_loop.mlir");
  for (cvub::GenericOperation &operation : module.operations) {
    for (std::string &type : operation.resultTypes)
      if (type == "tensor<1x128xf16>")
        type = "tensor<1x130xf16>";
    for (std::string &type : operation.operandTypes)
      if (type == "tensor<1x128xf16>")
        type = "tensor<1x130xf16>";
      else if (type == "memref<1x128xf16>")
        type = "memref<1x130xf16>";
    for (std::string &type : operation.resultTypes)
      if (type == "memref<1x128xf16>")
        type = "memref<1x130xf16>";
  }
  const auto result = cvub::RunTileCubeVectorLoop(module, 2, 2);
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
  CVUB_CHECK_EQ(OperationCount(result.module, "scf.for"), 1U);
  CVUB_CHECK_EQ(OperationCount(result.module, "memref.subview"), 0U);
}

CVUB_TEST(unimplemented_successful_loop_pattern_is_incomplete_and_unchanged) {
  auto module = cvub::test::ParseFixture("tile_vector_loop.mlir");
  auto &loop = module.operations.at(
      static_cast<size_t>(OperationId(module, "scf.for")));
  loop.name = "scope.scope";
  loop.operands.clear();
  loop.operandTypes.clear();
  auto &scopeBody = module.blocks.at(static_cast<size_t>(
      module.regions.at(static_cast<size_t>(loop.regions.front()))
          .blocks.front()));
  scopeBody.arguments.clear();
  scopeBody.argumentTypes.clear();
  module.operations.at(static_cast<size_t>(OperationId(module, "scf.yield")))
      .name = "scope.return";
  const std::string before = cvub::SerializeGenericModule(module);
  const auto result = cvub::RunTileCubeVectorLoop(module, 2, 2);
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Incomplete);
  CVUB_CHECK_EQ(cvub::SerializeGenericModule(result.module), before);
  CVUB_CHECK(!result.diagnostics.empty());
}

CVUB_TEST(tile_model_fails_closed_for_ambiguous_axis_and_branch_shapes) {
  auto nonLast = cvub::test::ParseFixture("tile_vector_loop.mlir");
  for (auto &operation : nonLast.operations) {
    for (std::string &type : operation.resultTypes)
      if (type == "tensor<1x128xf16>") type = "tensor<128x1xf16>";
    for (std::string &type : operation.operandTypes)
      if (type == "tensor<1x128xf16>") type = "tensor<128x1xf16>";
  }
  ExpectTileIncompleteAndUnchanged(nonLast);

  auto repeated = cvub::test::ParseFixture("tile_vector_loop.mlir");
  for (auto &operation : repeated.operations) {
    for (std::string &type : operation.resultTypes)
      if (type == "tensor<1x128xf16>") type = "tensor<128x128xf16>";
    for (std::string &type : operation.operandTypes)
      if (type == "tensor<1x128xf16>") type = "tensor<128x128xf16>";
  }
  ExpectTileIncompleteAndUnchanged(repeated);
}

CVUB_TEST(tile_model_fails_closed_for_multiple_anchors_and_unknown_body_ops) {
  auto multiStore = cvub::test::ParseFixture("tile_vector_loop.mlir");
  DuplicateAnchorBeforeYield(multiStore, "hivm.hir.store");
  ExpectTileIncompleteAndUnchanged(multiStore);

  auto unknown = cvub::test::ParseFixture("tile_vector_loop.mlir");
  MutableOperationNamed(unknown, "hivm.hir.vadd").name = "test.unrelated";
  ExpectTileIncompleteAndUnchanged(unknown);

  auto multiFixpipe = cvub::test::ParseFixture("tile_cube_loop.mlir");
  DuplicateAnchorBeforeYield(multiFixpipe, "hivm.hir.fixpipe");
  ExpectTileIncompleteAndUnchanged(multiFixpipe);
}

CVUB_TEST(tile_model_fails_closed_for_remainder_alignment_and_overflow) {
  auto remainder = cvub::test::ParseFixture("tile_vector_loop.mlir");
  for (auto &operation : remainder.operations) {
    for (std::string &type : operation.resultTypes)
      if (type == "tensor<1x128xf16>") type = "tensor<1x129xf16>";
    for (std::string &type : operation.operandTypes)
      if (type == "tensor<1x128xf16>") type = "tensor<1x129xf16>";
  }
  ExpectTileIncompleteAndUnchanged(remainder);

  auto operandAlignment = cvub::test::ParseFixture("tile_vector_loop.mlir");
  auto &vadd = MutableOperationNamed(operandAlignment, "hivm.hir.vadd");
  vadd.operandTypes.front() = "tensor<1x2xi8>";
  ExpectTileIncompleteAndUnchanged(operandAlignment);

  auto overflow = cvub::test::ParseFixture("tile_vector_loop.mlir");
  auto &sum = MutableOperationNamed(overflow, "hivm.hir.vadd");
  sum.resultTypes.front() = "tensor<9223372036854775807x128xf16>";
  ExpectTileIncompleteAndUnchanged(overflow);
}

CVUB_TEST(tile_model_handles_trip_one_and_global_shrink_after_skip) {
  auto module = cvub::test::ParseFixture("tile_vector_loop.mlir");
  const auto result = cvub::RunTileCubeVectorLoop(module, 1, 1);
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
  CVUB_CHECK_EQ(OperationCount(result.module, "memref.subview"), 0U);
  CVUB_CHECK_EQ(OperationNamed(result.module, "memref.alloc").resultTypes,
                std::vector<std::string>({"memref<1x128xf16>"}));
}

CVUB_TEST(tile_model_without_candidates_matches_real_early_return) {
  auto module = cvub::test::ParseFixture("tile_vector_loop.mlir");
  auto &loop = MutableOperationNamed(module, "scf.for");
  loop.attributes = "{}";
  const std::string before = cvub::SerializeGenericModule(module);
  const auto result = cvub::RunTileCubeVectorLoop(module, 2, 2);
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
  CVUB_CHECK_EQ(cvub::SerializeGenericModule(result.module), before);
}

CVUB_TEST(tile_model_fails_closed_for_lift_nested_and_missing_spec) {
  auto lift = cvub::test::ParseFixture("tile_vector_loop.mlir");
  auto &load = MutableOperationNamed(lift, "hivm.hir.load");
  load.operandTypes.back() = "memref<1x128xf16>";
  ExpectTileIncompleteAndUnchanged(lift);

  auto nested = cvub::test::ParseFixture("tile_vector_loop.mlir");
  auto &innerCandidate = MutableOperationNamed(nested, "hivm.hir.vadd");
  innerCandidate.attributes = cvub::SetDictionaryValue(
      innerCandidate.attributes, "hivm.loop_core_type",
      "#hivm.tcore_type<VECTOR>");
  ExpectTileIncompleteAndUnchanged(nested);

  auto missingSpec = cvub::test::ParseFixture("tile_vector_loop.mlir");
  missingSpec.operations.front().attributes = "{}";
  ExpectTileIncompleteAndUnchanged(missingSpec);
}

CVUB_TEST(tile_model_isolates_and_tiles_multiple_functions) {
  const auto result = cvub::RunTileCubeVectorLoop(
      cvub::test::ParseFixture("tile_two_functions.mlir"), 2, 2);
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
  CVUB_CHECK_EQ(OperationCount(result.module, "scf.for"), 4U);
  CVUB_CHECK_EQ(cvub::FunctionSymbolNames(result.module),
                std::vector<std::string>({"first", "second"}));
  cvub::ValidateGenericModule(result.module);
}

CVUB_TEST(tile_model_handles_cube_no_branch_l0c_skip_and_override) {
  auto noBranch = cvub::test::ParseFixture("tile_cube_no_branch.mlir");
  const auto noBranchResult = cvub::RunTileCubeVectorLoop(noBranch, 2, 2);
  CVUB_CHECK_EQ(noBranchResult.precision, cvub::Precision::Exact);
  CVUB_CHECK_EQ(OperationCount(noBranchResult.module, "scf.for"), 1U);

  auto l0cSkip = cvub::test::ParseFixture("tile_cube_loop.mlir");
  for (auto &operation : l0cSkip.operations) {
    for (std::string &type : operation.resultTypes) {
      if (type == "tensor<128x64xf16>") type = "tensor<64x64xf16>";
      if (type == "tensor<128x4096xf32>") type = "tensor<64x128xf32>";
      if (type == "memref<128x4096xf32>") type = "memref<64x128xf32>";
      if (type == "tensor<64x4096xf16>") type = "tensor<64x128xf16>";
    }
    for (std::string &type : operation.operandTypes) {
      if (type == "tensor<128x64xf16>") type = "tensor<64x64xf16>";
      if (type == "tensor<128x4096xf32>") type = "tensor<64x128xf32>";
      if (type == "memref<128x4096xf32>") type = "memref<64x128xf32>";
      if (type == "tensor<64x4096xf16>") type = "tensor<64x128xf16>";
    }
  }
  auto &smallSubview = MutableOperationNamed(l0cSkip, "memref.subview");
  smallSubview.attributes = cvub::SetDictionaryValue(
      smallSubview.attributes, "static_sizes", "[64, 128]");
  const auto &smallMmad = OperationNamed(l0cSkip, "hivm.hir.mmadL1");
  auto &smallRealM = l0cSkip.operations.at(
      static_cast<size_t>(DefinitionOf(l0cSkip, smallMmad.operands[3]).id));
  auto &smallRealN = l0cSkip.operations.at(
      static_cast<size_t>(DefinitionOf(l0cSkip, smallMmad.operands[5]).id));
  smallRealM.attributes =
      cvub::SetDictionaryValue(smallRealM.attributes, "value", "64 : index");
  smallRealN.attributes =
      cvub::SetDictionaryValue(smallRealN.attributes, "value", "128 : index");
  const auto skipResult = cvub::RunTileCubeVectorLoop(l0cSkip, 2, 2);
  CVUB_CHECK_EQ(skipResult.precision, cvub::Precision::Exact);
  CVUB_CHECK_EQ(OperationCount(skipResult.module, "scf.for"), 1U);

  auto overrideModule = l0cSkip;
  auto &overrideMmad = MutableOperationNamed(overrideModule, "hivm.hir.mmadL1");
  overrideMmad.attributes = cvub::SetDictionaryValue(
      overrideMmad.attributes, "hivm.tile_mix_cube_num", "2 : i64");
  const auto overrideResult =
      cvub::RunTileCubeVectorLoop(overrideModule, 2, 7);
  CVUB_CHECK_EQ(overrideResult.precision, cvub::Precision::Exact);
  CVUB_CHECK_EQ(OperationCount(overrideResult.module, "scf.for"), 2U);
}

CVUB_TEST(tile_model_rejects_dynamic_subview_transactionally) {
  auto module = cvub::test::ParseFixture("tile_vector_loop.mlir");
  auto &subview = MutableOperationNamed(module, "memref.subview");
  subview.attributes = cvub::SetDictionaryValue(
      subview.attributes, "static_offsets", "[0, -9223372036854775808]");
  ExpectTileIncompleteAndUnchanged(module);
}

CVUB_TEST(tile_model_fails_closed_for_bad_cube_override_and_misplaced_marker) {
  auto badOverride = cvub::test::ParseFixture("tile_cube_loop.mlir");
  auto &mmad = MutableOperationNamed(badOverride, "hivm.hir.mmadL1");
  mmad.attributes = cvub::SetDictionaryValue(
      mmad.attributes, "hivm.tile_mix_cube_num", "0 : i64");
  ExpectTileIncompleteAndUnchanged(badOverride);

  auto misplacedMarker = cvub::test::ParseFixture("tile_vector_loop.mlir");
  MutableOperationNamed(misplacedMarker, "func.return").attributes =
      cvub::SetDictionaryValue(MutableOperationNamed(misplacedMarker,
                                                     "func.return").attributes,
                               "test.second_function_boundary", "true");
  MutableOperationNamed(misplacedMarker, "func.return").attributes =
      cvub::SetDictionaryValue(MutableOperationNamed(misplacedMarker,
                                                     "func.return").attributes,
                               "hivm.loop_core_type",
                               "#hivm.tcore_type<VECTOR>");
  ExpectTileIncompleteAndUnchanged(misplacedMarker);
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

CVUB_TEST(core_classification_requires_exact_tcore_type_spelling) {
  auto module = cvub::test::ParseFixture("control_flow_mix_before_split.mlir");
  auto &custom = module.operations.at(
      static_cast<size_t>(OperationId(module, "hivm.hir.custom")));
  for (const std::string invalid : {"#bogus<CUBE>", "AIV",
                                    "#hivm.foo<AIC>",
                                    "#hivm.tcore_type<AIC>"}) {
    custom.attributes =
        cvub::SetDictionaryValue(custom.attributes, "tcoretype", invalid);
    CVUB_CHECK_EQ(cvub::ClassifyCore(module, custom), cvub::CoreKind::Unknown);
  }
  custom.attributes = cvub::SetDictionaryValue(
      custom.attributes, "tcoretype", "#hivm.tcore_type<VECTOR>");
  CVUB_CHECK_EQ(cvub::ClassifyCore(module, custom), cvub::CoreKind::Vector);
}

CVUB_TEST(split_mix_models_contextual_core_rules) {
  auto module = cvub::test::ParseFixture("task3_review_cases.mlir");
  const auto classifyCase = [&](const std::string &caseName) {
    return cvub::ClassifyCore(module, OperationWithCase(module, caseName));
  };
  CVUB_CHECK_EQ(classifyCase("load_vector"), cvub::CoreKind::Vector);
  CVUB_CHECK_EQ(classifyCase("load_cube"), cvub::CoreKind::Cube);
  CVUB_CHECK_EQ(classifyCase("copy_vector"), cvub::CoreKind::Vector);
  CVUB_CHECK_EQ(classifyCase("copy_cube"), cvub::CoreKind::Cube);
  CVUB_CHECK_EQ(classifyCase("vbrc_vector"), cvub::CoreKind::Vector);
  CVUB_CHECK_EQ(classifyCase("vbrc_cube"), cvub::CoreKind::Cube);
  CVUB_CHECK_EQ(cvub::ClassifyCore(
                    module, module.operations.at(static_cast<size_t>(
                                OperationId(module, "hivm.hir.matmul")))),
                cvub::CoreKind::Cube);
  CVUB_CHECK_EQ(cvub::ClassifyCore(module,
                                  OperationWithCase(module, "context_unknown")),
                cvub::CoreKind::Unknown);
}

CVUB_TEST(split_mix_scope_stubs_multi_results_and_preserves_unsafe_if) {
  auto module = cvub::test::ParseFixture("task3_review_cases.mlir");
  auto &dps = const_cast<cvub::GenericOperation &>(
      OperationWithCase(module, "dps_multi"));
  dps.dpsInits = {dps.operands[1], dps.operands[2]};
  const auto result = cvub::ProjectMixFunctionsToAIV(std::move(module));
  const auto &projected = ProjectedFunction(result, "review").module;
  CVUB_CHECK(!HasOperation(projected, "scope.scope"));
  CVUB_CHECK(HasOperation(projected, "scf.if"));
  CVUB_CHECK_EQ(OperationWithCase(projected, "loop0").operands.front(),
                ResultOf(projected, "tensor.empty"));
  CVUB_CHECK(HasOperationCase(projected, "scope_tensor"));
  CVUB_CHECK(HasOperationCase(projected, "scope_int"));
  CVUB_CHECK(HasOperationCase(projected, "scope_external"));
  CVUB_CHECK_EQ(DefinitionOf(projected,
                            OperationWithCase(projected, "scope_tensor")
                                .operands.front())
                    .name,
                "tensor.empty");
  const auto &integerStub = DefinitionOf(
      projected, OperationWithCase(projected, "scope_int").operands.front());
  CVUB_CHECK_EQ(integerStub.name, "arith.constant");
  CVUB_CHECK_EQ(cvub::IRDictionaryValue(integerStub.attributes, "value"),
                "0 : i32");
  CVUB_CHECK_EQ(OperationWithCase(projected, "scope_external").operands.front(),
                OperationWithCase(projected, "scope_external").operands[1]);
  CVUB_CHECK_EQ(OperationWithCase(projected, "dps0").operands.front(),
                OperationWithCase(projected, "dps0").operands[1]);
  CVUB_CHECK_EQ(OperationWithCase(projected, "dps1").operands.front(),
                OperationWithCase(projected, "dps1").operands[1]);
  CVUB_CHECK(HasDiagnostic(result, "SplitMixKernelAIVProjection",
                           "branch yields"));
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Incomplete);
  cvub::ValidateGenericModule(projected);
}

CVUB_TEST(split_mix_preserves_multi_result_dps_on_count_mismatch) {
  auto module = cvub::test::ParseFixture("task3_review_cases.mlir");
  auto &dps = const_cast<cvub::GenericOperation &>(
      OperationWithCase(module, "dps_multi"));
  dps.dpsInits = {dps.operands[1]};
  const auto result = cvub::ProjectMixFunctionsToAIV(std::move(module));
  const auto &projected = ProjectedFunction(result, "review").module;
  CVUB_CHECK(HasOperationCase(projected, "dps_multi"));
  CVUB_CHECK(HasDiagnostic(result, "SplitMixKernelAIVProjection",
                           "DPS init/result count mismatch"));
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Incomplete);
  cvub::ValidateGenericModule(projected);
}

CVUB_TEST(split_mix_uses_real_vector_cleanup_labels_and_function_attrs) {
  auto module = cvub::test::ParseFixture("control_flow_mix_before_split.mlir");
  auto &cube = module.operations.at(
      static_cast<size_t>(OperationId(module, "hivm.hir.mmadL1")));
  cube.dpsInits = {cube.operands.back()};
  const auto result = cvub::ProjectMixFunctionsToAIV(std::move(module));
  const auto &projected = ProjectedFunction(result, "control").module;
  CVUB_CHECK(!HasOperation(projected, "annotation.mark"));
  CVUB_CHECK(!HasOperation(projected, "tensor.extract"));
  const auto &function = projected.operations.at(1);
  CVUB_CHECK(HasUnitAttribute(function.attributes, "hivm.part_of_mix"));
  CVUB_CHECK_EQ(cvub::IRDictionaryValue(function.attributes, "mix_mode"),
                "\"mix\"");
}

CVUB_TEST(split_mix_vector_cleanup_mirrors_remove_useless_mark_conditions) {
  auto module = cvub::test::ParseFixture("split_mix_cleanup.mlir");
  cvub::PostProcessVectorFunction(module, FunctionId(module, "cleanup"));
  module = cvub::CompactGenericModule(std::move(module));

  CVUB_CHECK(!HasMarkOfCase(module, "removable_source"));
  CVUB_CHECK(HasMarkOfCase(module, "only_mark_source"));
  CVUB_CHECK(HasMarkOfCase(module, "fixpipe_source"));
  CVUB_CHECK(HasMarkOfCase(module, "store_source"));
  CVUB_CHECK(HasMarkOfCase(module, "attributed_source"));
  cvub::ValidateGenericModule(module);
}

CVUB_TEST(split_mix_vector_cleanup_matches_extract_duplication_topology) {
  auto module = cvub::test::ParseFixture("split_mix_cleanup.mlir");
  cvub::PostProcessVectorFunction(module, FunctionId(module, "cleanup"));
  module = cvub::CompactGenericModule(std::move(module));

  CVUB_CHECK(HasOperationCase(module, "original_extract"));
  CVUB_CHECK(!HasOperationCase(module, "dead_new_extract"));
  CVUB_CHECK(HasOperationCase(module, "live_new_extract"));
  cvub::ValidateGenericModule(module);
}

CVUB_TEST(split_mix_best_effort_cube_keeps_live_labeled_extract) {
  const auto result = cvub::ProjectMixFunctionsToAIV(
      cvub::test::ParseFixture("split_mix_cleanup.mlir"));
  const auto &projected = ProjectedFunction(result, "cleanup").module;

  CVUB_CHECK_EQ(result.precision, cvub::Precision::Incomplete);
  CVUB_CHECK(HasDiagnostic(result, "SplitMixKernelAIVProjection",
                           "no safe result replacement rule"));
  CVUB_CHECK(HasOperationCase(projected, "live_new_extract"));
  cvub::ValidateGenericModule(projected);
}

CVUB_TEST(split_mix_already_aiv_unknown_is_incomplete) {
  auto module = cvub::test::ParseFixture("control_flow_mix_before_split.mlir");
  auto &function = module.operations.at(1);
  function.attributes = cvub::SetDictionaryValue(
      function.attributes, "hivm.func_core_type", "#hivm.func_core_type<AIV>");
  const auto result = cvub::ProjectMixFunctionsToAIV(std::move(module));
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Incomplete);
  CVUB_CHECK(HasDiagnostic(result, "SplitMixKernelAIVProjection",
                           "no compiler core classification"));
  CVUB_CHECK(HasDiagnostic(result, "SplitMixKernelAIVProjection",
                           "Cube-classified"));
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
                  stage.stage == "TileCubeVectorLoop" ||
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
  CVUB_CHECK_EQ(result.coverage.front().disposition,
                cvub::CoverageDisposition::Modeled);
  CVUB_CHECK(!HasDiagnostic(result, "TileCubeVectorLoop", "unsupported"));
  CVUB_CHECK(HasDiagnostic(result, "InferAndSetBufferSize", "unsupported"));
}

CVUB_TEST(post_pipeline_returns_tile_stage_output_to_downstream) {
  const auto input = cvub::test::ParseFixture("tile_vector_loop.mlir");
  const auto result = cvub::RunPostCVPipelineAIVProjection(input, {});
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Incomplete);
  CVUB_CHECK_EQ(OperationCount(result.module, "scf.for"), 2U);
  CVUB_CHECK_EQ(OperationNamed(result.module, "hivm.hir.vadd").resultTypes,
                std::vector<std::string>({"tensor<1x64xf16>"}));
  CVUB_CHECK(cvub::SerializeGenericModule(result.module) !=
             cvub::SerializeGenericModule(input));
  cvub::ValidateGenericModule(result.module);
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
