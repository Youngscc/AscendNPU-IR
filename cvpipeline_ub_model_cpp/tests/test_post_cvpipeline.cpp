#include "test_support.hpp"
#include "../src/post_cvpipeline/code_motion.hpp"
#include "../src/post_cvpipeline/inline_otf_load_store.hpp"
#include "../src/post_cvpipeline/ir_utils.hpp"
#include "../src/post_cvpipeline/buffer_size.hpp"
#include "../src/post_cvpipeline/canonicalization.hpp"
#include "../src/post_cvpipeline/inline_scope.hpp"
#include "../src/post_cvpipeline/pipeline.hpp"
#include "../src/post_cvpipeline/planning.hpp"
#include "../src/post_cvpipeline/split_mix_aiv.hpp"
#include "../src/post_cvpipeline/tensor_empty.hpp"
#include "../src/post_cvpipeline/tile_bind_subblock.hpp"
#include "../src/post_cvpipeline/tile_cube_vector_loop.hpp"
#include "../src/cvpipeline/cvpipelining_pass.hpp"
#include "../src/semantic_ir/generic_op_semantics.hpp"
#include "../src/suffix/suffix_pipeline.hpp"

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

bool HasDiagnostic(const cvub::StageResult &result, const std::string &stage,
                   const std::string &reason) {
  for (const cvub::PostCVPipelineDiagnostic &diagnostic : result.diagnostics)
    if (diagnostic.pipelineStage == stage &&
        diagnostic.reason.find(reason) != std::string::npos)
      return true;
  return false;
}

bool HasDiagnostic(const cvub::ModulePlanResult &result,
                   const std::string &stage, const std::string &reason) {
  for (const cvub::PostCVPipelineDiagnostic &diagnostic : result.diagnostics)
    if (diagnostic.pipelineStage == stage &&
        diagnostic.reason.find(reason) != std::string::npos)
      return true;
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

std::string ReadFixtureText(const std::string &name) {
  std::ifstream input(cvub::fs::path("cvpipeline_ub_model_cpp/tests/fixtures") /
                      name);
  if (!input)
    throw std::runtime_error("cannot read fixture: " + name);
  return std::string(std::istreambuf_iterator<char>(input),
                     std::istreambuf_iterator<char>());
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

std::vector<const cvub::GenericOperation *> OperationsWithCase(
    const cvub::GenericModule &module, const std::string &caseName) {
  std::vector<const cvub::GenericOperation *> operations;
  for (const cvub::GenericOperation &operation : module.operations)
    if (cvub::UnquoteIRString(
            cvub::IRDictionaryValue(operation.attributes, "case")) == caseName)
      operations.push_back(&operation);
  return operations;
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
  const auto &minimum = DefinitionOf(result.module, matrix.operands[5]);
  const auto &innerLoop = result.module.operations.at(
      static_cast<size_t>(matrix.parentId));
  const int innerBlock = result.module.regions.at(
      static_cast<size_t>(innerLoop.regions.front())).blocks.front();
  CVUB_CHECK_EQ(minimum.operands,
                std::vector<int>({result.module.blocks.at(
                    static_cast<size_t>(innerBlock)).arguments.front()}));
  CVUB_CHECK_EQ(minimum.operandTypes, std::vector<std::string>({"index"}));
  CVUB_CHECK_EQ(cvub::IRDictionaryValue(minimum.properties, "map"),
                "affine_map<(d0) -> (-2048*d0 + 4096, 2048)>");
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

CVUB_TEST(cube_affine_min_is_consumed_by_the_real_suffix_bridge) {
  const auto prepareForSuffix = [](cvub::GenericModule &module) {
    auto &minimum = MutableOperationNamed(module, "affine.min");
    std::smatch tileMatch;
    const std::string minimumMap = cvub::IRDictionaryValue(
        minimum.properties, "map");
    CVUB_CHECK(std::regex_search(
        minimumMap, tileMatch,
        std::regex(R"(,[[:space:]]*([0-9]+)\)>$)")));
    const int64_t tileSize = std::stoll(tileMatch[1].str());
    auto &innerLoop = module.operations.at(
        static_cast<size_t>(minimum.parentId));
    innerLoop.properties = cvub::SetDictionaryValue(
        innerLoop.properties, "map_for_to_forall", "unit");

    bool madeMinimumObservable = false;
    for (cvub::GenericOperation &operation : module.operations) {
      if (operation.name != "tensor.extract_slice" &&
          operation.name != "memref.subview")
        continue;
      const std::string offsets = cvub::IRDictionaryValue(
          operation.attributes, "static_offsets");
      const std::string sizes = cvub::IRDictionaryValue(
          operation.attributes, "static_sizes");
      const std::string strides = cvub::IRDictionaryValue(
          operation.attributes, "static_strides");
      if (offsets.empty() || sizes.empty() || strides.empty())
        continue;
      const auto denseI64 = [](const std::string &array) {
        return "array<i64: " + array.substr(1, array.size() - 2) + ">";
      };
      operation.properties = cvub::SetDictionaryValue(
          operation.properties, "static_offsets", denseI64(offsets));
      operation.properties = cvub::SetDictionaryValue(
          operation.properties, "static_sizes", denseI64(sizes));
      operation.properties = cvub::SetDictionaryValue(
          operation.properties, "static_strides", denseI64(strides));
      if (madeMinimumObservable || operation.operands.size() != 2)
        continue;
      std::vector<int64_t> dynamicSizes =
          cvub::tile_cube_vector_loop_detail::ParseIntegerArray(sizes);
      const auto resultType =
          cvub::tile_cube_vector_loop_detail::ParseStaticShapedType(
          operation.resultTypes.front());
      if (!resultType || dynamicSizes.size() != resultType->shape.size())
        continue;
      size_t tiledAxis = dynamicSizes.size();
      for (size_t axis = 0; axis < dynamicSizes.size(); ++axis)
        if (dynamicSizes[axis] == resultType->shape[axis] &&
            dynamicSizes[axis] == tileSize) {
          tiledAxis = axis;
          break;
        }
      if (tiledAxis == dynamicSizes.size())
        continue;
      dynamicSizes[tiledAxis] = std::numeric_limits<int64_t>::min();
      operation.properties = cvub::SetDictionaryValue(
          operation.properties, "static_sizes",
          denseI64(cvub::tile_cube_vector_loop_detail::PrintIntegerArray(
              dynamicSizes)));
      operation.operands.push_back(minimum.results.front());
      operation.operandTypes.push_back("index");
      operation.properties = cvub::SetDictionaryValue(
          operation.properties, "operandSegmentSizes",
          "array<i32: 1, 1, 1, 0>");
      madeMinimumObservable = true;
    }
    CVUB_CHECK(madeMinimumObservable);
  };

  auto nTiled = cvub::RunTileCubeVectorLoop(
      cvub::test::ParseFixture("tile_cube_loop.mlir"), 2, 2);
  CVUB_CHECK_EQ(nTiled.precision, cvub::Precision::Exact);
  auto &nFunction = MutableOperationNamed(nTiled.module, "func.func");
  nFunction.attributes = cvub::SetDictionaryValue(
      nFunction.attributes, "hivm.func_core_type",
      "#hivm.func_core_type<AIV>");
  prepareForSuffix(nTiled.module);
  const cvub::PlanMemoryInput nPlan =
      cvub::BuildSuffixPlanMemoryInput(nTiled.module);
  CVUB_CHECK(std::none_of(
      nPlan.operations.begin(), nPlan.operations.end(),
      [](const cvub::OperationRecord &operation) {
        return operation.opName == "affine.min";
      }));

  auto mInput = cvub::test::ParseFixture("tile_cube_loop.mlir");
  auto &mmad = MutableOperationNamed(mInput, "hivm.hir.mmadL1");
  auto &load = MutableOperationNamed(mInput, "hivm.hir.load");
  const int outsideA = mmad.operands[0];
  const int outsideB = load.operands[0];
  const int loaded = load.results.front();
  auto &loadInit = mInput.operations.at(
      static_cast<size_t>(DefinitionOf(mInput, load.operands[1]).id));
  loadInit.resultTypes.front() = "tensor<128x64xf16>";
  load.operands[0] = outsideA;
  load.operandTypes = {"tensor<128x64xf16>", "tensor<128x64xf16>"};
  load.resultTypes.front() = "tensor<128x64xf16>";
  load.dpsInputs.clear();
  load.dpsInits.clear();
  load.effects.clear();
  cvub::ApplyOperationSemantics(load);
  mmad.operands[0] = loaded;
  mmad.operandTypes[0] = "tensor<128x64xf16>";
  mmad.operands[1] = outsideB;
  mmad.operandTypes[1] = "tensor<64x4096xf16>";
  mmad.dpsInputs.clear();
  mmad.dpsInits.clear();
  cvub::ApplyOperationSemantics(mmad);
  cvub::ValidateGenericModule(mInput);
  auto mTiled = cvub::RunTileCubeVectorLoop(mInput, 2, 2);
  CVUB_CHECK_EQ(mTiled.precision, cvub::Precision::Exact);
  auto &mFunction = MutableOperationNamed(mTiled.module, "func.func");
  mFunction.attributes = cvub::SetDictionaryValue(
      mFunction.attributes, "hivm.func_core_type",
      "#hivm.func_core_type<AIV>");
  prepareForSuffix(mTiled.module);
  const cvub::PlanMemoryInput mPlan =
      cvub::BuildSuffixPlanMemoryInput(mTiled.module);
  CVUB_CHECK(std::none_of(
      mPlan.operations.begin(), mPlan.operations.end(),
      [](const cvub::OperationRecord &operation) {
        return operation.opName == "affine.min";
      }));
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
                "affine_map<(d0) -> (-128*d0 + 256, 128)>");
  CVUB_CHECK(cvub::IRDictionaryValue(minimum.attributes, "map").empty());
  CVUB_CHECK_EQ(minimum.operands.size(), 1U);
  CVUB_CHECK_EQ(minimum.operandTypes, std::vector<std::string>({"index"}));
  CVUB_CHECK_EQ(minimum.resultTypes, std::vector<std::string>({"index"}));
  const auto &innerLoop = result.module.operations.at(
      static_cast<size_t>(minimum.parentId));
  const int innerBlock = result.module.regions.at(
      static_cast<size_t>(innerLoop.regions.front())).blocks.front();
  CVUB_CHECK_EQ(minimum.operands.front(), result.module.blocks.at(
      static_cast<size_t>(innerBlock)).arguments.front());
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
                "affine_map<(d0) -> (-128*d0 + 256, 128)>");
  CVUB_CHECK(cvub::IRDictionaryValue(minimum.attributes, "map").empty());
  CVUB_CHECK_EQ(minimum.operands.size(), 1U);
  CVUB_CHECK_EQ(minimum.operandTypes, std::vector<std::string>({"index"}));
  CVUB_CHECK_EQ(minimum.resultTypes, std::vector<std::string>({"index"}));
  const auto &innerLoop = result.module.operations.at(
      static_cast<size_t>(minimum.parentId));
  const int innerBlock = result.module.regions.at(
      static_cast<size_t>(innerLoop.regions.front())).blocks.front();
  CVUB_CHECK_EQ(minimum.operands.front(), result.module.blocks.at(
      static_cast<size_t>(innerBlock)).arguments.front());
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

CVUB_TEST(core_classification_treats_index_mask_utilities_as_neutral) {
  // set_mask_norm and get_block_idx are index/mask utility ops that the real
  // compiler retains on the AIV side (get_block_idx carries CubeVectorCoreTrait
  // = CUBE_OR_VECTOR; set_mask_norm has no core trait and defaults to
  // CUBE_OR_VECTOR).  SplitMixKernel's filterMixFunc only erases ops whose
  // coreType is exactly CUBE, so CUBE_OR_VECTOR utility ops stay on the AIV
  // projection.  They allocate no UB buffer and compute no tensor data, so the
  // UB model must classify them Neutral (retained, not flagged Incomplete).
  // Regression for the demo_vector_add/demo_randn precision=incomplete gap.
  const auto module = cvub::ParseGenericIR(
      "cvpipeline_ub_model_cpp/examples/inputs/"
      "demo_vector_add_before_cvpipeline.mlir",
      false);
  CVUB_CHECK_EQ(cvub::ClassifyCore(
                    module, module.operations.at(static_cast<size_t>(
                                OperationId(module, "hivm.hir.set_mask_norm")))),
                cvub::CoreKind::Neutral);
  CVUB_CHECK_EQ(cvub::ClassifyCore(
                    module, module.operations.at(static_cast<size_t>(
                                OperationId(module, "hivm.hir.get_block_idx")))),
                cvub::CoreKind::Neutral);
}

CVUB_TEST(mix_fixture_projects_to_single_exact_aiv_kernel) {
  // End-to-end MIX golden: the compiler-boundary MIX function `kernel` (Cube
  // mmadL1 + a Vector load + a flat scf.for) is projected to exactly one
  // `kernel_mix_aiv` function on a fully reproducible path.  The Cube mmadL1 is
  // dropped; the Vector load survives (its output tensor materializes the UB
  // buffer); TileAndBindSubBlock runs the recognized no-store isFailed revert
  // (Exact); the flat loop's invariant tensor.empty is hoisted by LICM; and no
  // stage reports Incomplete.  Mirrors the plan-before-cvpipeline CLI flow.
  auto module = cvub::ParseGenericIR(
      "cvpipeline_ub_model_cpp/examples/inputs/"
      "demo_mix_before_cvpipeline.mlir",
      false);
  for (cvub::GenericOperation &operation : module.operations)
    if (cvub::HasModeledOperationSemantics(operation.name))
      cvub::ApplyOperationSemantics(operation);
  module = cvub::RunCVPipeliningPass(std::move(module),
                                     cvub::CVPipeliningOptions{});
  const cvub::PostCVPipelineResult projected =
      cvub::RunPostCVPipelineAIVProjection(std::move(module),
                                           cvub::PostCVPipelineOptions{});
  CVUB_CHECK_EQ(projected.precision, cvub::Precision::Exact);
  CVUB_CHECK(projected.diagnostics.empty());
  CVUB_CHECK_EQ(projected.functions.size(), 1U);
  CVUB_CHECK_EQ(projected.functions.front().projectedFunction,
                "kernel_mix_aiv");
  const cvub::GenericModule &aiv = projected.functions.front().module;
  CVUB_CHECK(!HasOperation(aiv, "hivm.hir.mmadL1"));  // Cube side dropped.
  CVUB_CHECK(HasOperation(aiv, "hivm.hir.load"));     // Vector side retained.
  CVUB_CHECK(HasOperation(aiv, "scf.for")); // flat loop survives for LICM.
  cvub::ValidateGenericModule(aiv);
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
    if (stage.stage == "WorkspaceSemanticProjection" ||
        stage.stage == "CrossCoreSyncInvariant")
      CVUB_CHECK_EQ(stage.disposition,
                    cvub::CoverageDisposition::UBInvariant);
    else
      CVUB_CHECK_EQ(stage.disposition,
                    stage.stage == "TileCubeVectorLoop" ||
                            stage.stage == "InferAndSetBufferSize" ||
                            stage.stage == "CanonicalizationBeforeSplit" ||
                            stage.stage == "SplitMixKernelAIVProjection" ||
                            stage.stage == "InlineScope" ||
                            stage.stage == "TileAndBindSubBlock" ||
                            stage.stage == "FoldTensorEmpty" ||
                            stage.stage == "CanonicalizationAfterSplit" ||
                            stage.stage == "LoopInvariantCodeMotion" ||
                            stage.stage == "LoopInvariantSubsetHoisting" ||
                            stage.stage == "CloneTensorEmpty" ||
                            stage.stage == "InlineOTFLoadStore"
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

CVUB_TEST(post_pipeline_manifest_has_full_coverage_after_task8) {
  // After Task 8 every manifest stage is modeled or UB-invariant, so the
  // compiled coverage table reports no Unsupported stage and the code-motion /
  // OTF stages never emit an "unsupported" diagnostic.  (The overall precision
  // may still be Incomplete for input- or stage-specific reasons.)
  const auto result = cvub::RunPostCVPipelineAIVProjection(
      cvub::test::ParseFixture("minimal_aiv.mlir"), {});
  CVUB_CHECK_EQ(result.coverage.size(), 14U);
  for (const cvub::StageCoverage &stage : result.coverage)
    CVUB_CHECK(stage.disposition != cvub::CoverageDisposition::Unsupported);
  CVUB_CHECK(!HasDiagnostic(result, "LoopInvariantCodeMotion", "unsupported"));
  CVUB_CHECK(!HasDiagnostic(result, "LoopInvariantSubsetHoisting",
                            "unsupported"));
  CVUB_CHECK(!HasDiagnostic(result, "InlineOTFLoadStore", "unsupported"));
}

CVUB_TEST(post_pipeline_passes_size_and_canonicalization_output_to_split) {
  auto input = cvub::ExtractFunctionModule(
      cvub::test::ParseFixture("dynamic_buffer_size.mlir"), "dynamic_sizes");
  const auto result =
      cvub::RunPostCVPipelineAIVProjection(std::move(input), {});
  CVUB_CHECK_EQ(result.functions.size(), 1U);
  const auto &projected = result.functions.front().module;
  CVUB_CHECK_EQ(OperationCount(projected, "memref.view"), 2U);
  CVUB_CHECK(!HasOperation(projected, "annotation.mark"));
  cvub::ValidateGenericModule(projected);
}

CVUB_TEST(post_pipeline_returns_tile_stage_output_to_downstream) {
  const auto input = cvub::test::ParseFixture("tile_vector_loop.mlir");
  const auto expected = cvub::RunTileCubeVectorLoop(input, 2, 2);
  CVUB_CHECK_EQ(expected.precision, cvub::Precision::Exact);
  const auto result = cvub::RunPostCVPipelineAIVProjection(input, {});
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Incomplete);
  CVUB_CHECK(OperationCount(result.module, "scf.for") <=
             OperationCount(expected.module, "scf.for"));
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

CVUB_TEST(buffer_size_inference_materializes_physical_views) {
  auto input = cvub::ExtractFunctionModule(
      cvub::test::ParseFixture("dynamic_buffer_size.mlir"), "dynamic_sizes");
  const auto result = cvub::RunInferAndSetBufferSize(std::move(input));
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
  CVUB_CHECK_EQ(OperationCount(result.module, "memref.view"), 2U);
  CVUB_CHECK(!HasOperation(result.module, "annotation.mark"));
  std::vector<std::string> allocTypes;
  for (const auto &operation : result.module.operations)
    if (operation.name == "memref.alloc")
      allocTypes.push_back(operation.resultTypes.front());
  CVUB_CHECK(std::find(allocTypes.begin(), allocTypes.end(),
                       "memref<64xi8, #hivm.address_space<UB>>") !=
             allocTypes.end());
  CVUB_CHECK(std::find(allocTypes.begin(), allocTypes.end(),
                       "memref<128xi8, #hivm.address_space<UB>>") !=
             allocTypes.end());
  CVUB_CHECK(std::find(allocTypes.begin(), allocTypes.end(),
                       "memref<8xf16, #hivm.address_space<UB>>") !=
             allocTypes.end());
  cvub::ValidateGenericModule(result.module);
}

CVUB_TEST(buffer_size_pipeline_constantizes_bounds_before_setting_annotations) {
  auto input = cvub::ExtractFunctionModule(
      cvub::test::ParseFixture("dynamic_buffer_size.mlir"),
      "constant_bounds");
  const auto result = cvub::RunInferAndSetBufferSize(std::move(input));
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
  CVUB_CHECK_EQ(OperationCount(result.module, "memref.view"), 2U);
  std::vector<std::string> backingTypes;
  for (const auto &operation : result.module.operations)
    if (operation.name == "memref.alloc" || operation.name == "memref.alloca")
      backingTypes.push_back(operation.resultTypes.front());
  CVUB_CHECK(std::find(backingTypes.begin(), backingTypes.end(),
                       "memref<32xi8, #hivm.address_space<UB>>") !=
             backingTypes.end());
  CVUB_CHECK(std::find(backingTypes.begin(), backingTypes.end(),
                       "memref<80xi8, #hivm.address_space<UB>>") !=
             backingTypes.end());
  const auto &mark = OperationNamed(result.module, "annotation.mark");
  CVUB_CHECK(cvub::IRDictionaryValue(mark.attributes,
                                     "buffer_size_in_byte").empty());
  CVUB_CHECK_EQ(cvub::IRDictionaryValue(mark.attributes, "multi_buffer"),
                "2 : i64");
  cvub::ValidateGenericModule(result.module);
}

CVUB_TEST(auto_infer_buffer_size_does_not_mark_dynamic_alloca) {
  auto input = cvub::ExtractFunctionModule(
      cvub::test::ParseFixture("dynamic_buffer_size.mlir"), "dynamic_sizes");
  for (auto &operation : input.operations)
    if (operation.name == "memref.alloc" &&
        operation.resultTypes.front().find("f32") != std::string::npos)
      operation.name = "memref.alloca";
  const auto result = cvub::RunInferAndSetBufferSize(std::move(input));
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
  bool constantizedAllocaWithoutAutoMark = false;
  for (const auto &operation : result.module.operations)
    if (operation.name == "memref.alloca" &&
        operation.resultTypes.front() ==
            "memref<32xi8, #hivm.address_space<UB>>")
      constantizedAllocaWithoutAutoMark = true;
  CVUB_CHECK(constantizedAllocaWithoutAutoMark);
}

CVUB_TEST(buffer_size_conflict_is_operation_level_incomplete) {
  auto input = cvub::ExtractFunctionModule(
      cvub::test::ParseFixture("dynamic_buffer_size.mlir"),
      "conflicting_sizes");
  // Keep Constantize from erasing both marks before Set observes them: the
  // logical 100xf16 allocation is larger than either requested backing.
  auto &dimension = MutableOperationNamed(input, "arith.constant");
  dimension.attributes = cvub::SetDictionaryValue(
      dimension.attributes, "value", "100 : index");
  const auto before = cvub::SerializeGenericModule(input);
  const auto result = cvub::RunInferAndSetBufferSize(std::move(input));
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Incomplete);
  CVUB_CHECK(HasDiagnostic(result, "InferAndSetBufferSize", "conflicting"));
  CVUB_CHECK_EQ(cvub::SerializeGenericModule(result.module), before);
}

CVUB_TEST(buffer_size_constantize_erases_direct_alias_conflict_before_set) {
  auto input = cvub::ExtractFunctionModule(
      cvub::test::ParseFixture("dynamic_buffer_size.mlir"),
      "constant_bounds");
  auto &function = MutableOperationNamed(input, "func.func");
  function.attributes = cvub::RemoveDictionaryAttribute(
      function.attributes, "enable_auto_mark_buffer_size");
  auto &allocation = MutableOperationNamed(input, "memref.alloc");
  const int allocationId = allocation.id;
  const int allocationValue = allocation.results.front();
  const std::string logicalType = allocation.resultTypes.front();
  const int blockId = allocation.blockId;
  const int parentId = allocation.parentId;
  const int regionId = allocation.regionId;
  const int ordinal = allocation.ordinal;
  auto &directMark = MutableOperationNamed(input, "annotation.mark");
  directMark.attributes = "{buffer_size_in_byte = 512 : i64}";

  cvub::GenericRewriter rewriter(input);
  const int alias = rewriter.createOperation(
      parentId, regionId, blockId, "memref.subview",
      {"memref<20xf16, strided<[2], offset: 1>, #hivm.address_space<UB>>"},
      {allocationValue}, {logicalType},
      "{static_offsets = array<i64: 0>, static_sizes = array<i64: 20>, "
      "static_strides = array<i64: 1>}");
  rewriter.insertToBlock(blockId, static_cast<size_t>(ordinal + 1), alias);
  const auto &aliasOp = input.operations.at(static_cast<size_t>(alias));
  const int aliasMark = rewriter.createOperation(
      parentId, regionId, blockId, "annotation.mark", {},
      {aliasOp.results.front()}, {aliasOp.resultTypes.front()}, "",
      "{buffer_size_in_byte = 640 : i64, hivm.multi_buffer = 2 : i64}");
  cvub::ApplyOperationSemantics(
      input.operations.at(static_cast<size_t>(aliasMark)));
  rewriter.insertToBlock(blockId, static_cast<size_t>(ordinal + 2), aliasMark);
  cvub::ValidateGenericModule(input);

  auto result = cvub::RunInferAndSetBufferSize(std::move(input));
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
  CVUB_CHECK_EQ(result.module.operations.at(static_cast<size_t>(allocationId))
                    .resultTypes.front(),
                "memref<40xi8, #hivm.address_space<UB>>");
  CVUB_CHECK_EQ(OperationCount(result.module, "annotation.mark"), 1U);
  const auto &remainingMark = OperationNamed(result.module, "annotation.mark");
  CVUB_CHECK(cvub::IRDictionaryValue(remainingMark.attributes,
                                     "buffer_size_in_byte").empty());
  CVUB_CHECK_EQ(cvub::IRDictionaryValue(remainingMark.attributes,
                                        "hivm.multi_buffer"),
                "2 : i64");
  auto &consume = MutableOperationNamed(result.module, "test.consume");
  consume.name = "hivm.hir.store";
  const auto &aliasAfter = OperationNamed(result.module, "memref.subview");
  consume.operands[1] = aliasAfter.results.front();
  consume.operandTypes[1] = aliasAfter.resultTypes.front();
  cvub::ApplyOperationSemantics(consume);
  const auto plan = cvub::PlanLocalMemoryForSeed(
      cvub::BuildSuffixPlanMemoryInput(result.module), 0);
  CVUB_CHECK(plan.success);
  CVUB_CHECK(plan.peakBits >= 640U);
}

CVUB_TEST(unresolved_dynamic_local_size_is_incomplete) {
  auto input = cvub::ExtractFunctionModule(
      cvub::test::ParseFixture("dynamic_buffer_size.mlir"),
      "conflicting_sizes");
  for (auto &operation : input.operations)
    if (operation.name == "annotation.mark")
      cvub::GenericRewriter(input).removeFromBlock(operation.blockId,
                                                   operation.id);
  input = cvub::CompactGenericModule(std::move(input));
  MutableOperationNamed(input, "arith.constant").name = "test.dynamic_dim";
  const auto result = cvub::RunInferAndSetBufferSize(std::move(input));
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Incomplete);
  CVUB_CHECK(HasDiagnostic(result, "InferAndSetBufferSize",
                           "unsupported ValueBounds"));
}

CVUB_TEST(buffer_size_constantize_accepts_conservative_min_upper_bound) {
  auto input = cvub::ExtractFunctionModule(
      cvub::test::ParseFixture("dynamic_buffer_size.mlir"),
      "constant_bounds");
  MutableOperationNamed(input, "arith.ceildivui").name = "test.dynamic_dim";
  MutableOperationNamed(input, "arith.maxui").name = "arith.minui";
  for (auto &operation : input.operations)
    if (operation.name == "annotation.mark")
      operation.attributes = cvub::RemoveDictionaryAttribute(
          operation.attributes, "buffer_size_in_byte");
  const auto result = cvub::RunInferAndSetBufferSize(std::move(input));
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
  bool sawUpperBoundBacking = false;
  for (const auto &operation : result.module.operations)
    if (operation.name == "memref.alloca" &&
        operation.resultTypes.front() ==
            "memref<80xi8, #hivm.address_space<UB>>")
      sawUpperBoundBacking = true;
  CVUB_CHECK(sawUpperBoundBacking);
}

CVUB_TEST(buffer_size_constantize_supports_affine_division_expression) {
  auto input = cvub::ExtractFunctionModule(
      cvub::test::ParseFixture("dynamic_buffer_size.mlir"),
      "constant_bounds");
  auto &maximum = MutableOperationNamed(input, "arith.maxui");
  maximum.name = "affine.apply";
  maximum.properties =
      "{map = affine_map<(d0, d1) -> (d0 ceildiv d1)>}";
  const auto result = cvub::RunInferAndSetBufferSize(std::move(input));
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
  CVUB_CHECK_EQ(OperationCount(result.module, "memref.view"), 2U);
}

CVUB_TEST(buffer_size_unknown_value_bounds_pattern_fails_closed) {
  auto input = cvub::ExtractFunctionModule(
      cvub::test::ParseFixture("dynamic_buffer_size.mlir"),
      "constant_bounds");
  MutableOperationNamed(input, "arith.maxui").name = "arith.shli";
  for (auto &operation : input.operations)
    if (operation.name == "annotation.mark")
      operation.attributes = cvub::RemoveDictionaryAttribute(
          operation.attributes, "buffer_size_in_byte");
  const auto result = cvub::RunInferAndSetBufferSize(std::move(input));
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Incomplete);
  CVUB_CHECK(HasDiagnostic(result, "InferAndSetBufferSize",
                           "unsupported ValueBounds"));
}

CVUB_TEST(buffer_size_constantize_erases_size_mark_and_drops_alignment) {
  auto input = cvub::ExtractFunctionModule(
      cvub::test::ParseFixture("dynamic_buffer_size.mlir"),
      "constant_bounds");
  auto &allocation = MutableOperationNamed(input, "memref.alloc");
  allocation.attributes = cvub::SetDictionaryValue(
      allocation.attributes, "alignment", "64 : i64");
  auto &mark = MutableOperationNamed(input, "annotation.mark");
  mark.attributes = cvub::SetDictionaryValue(
      mark.attributes, "buffer_size_in_byte", "512 : i64");
  mark.attributes = cvub::SetDictionaryValue(
      mark.attributes, "hivm.multi_buffer", "3 : i64");

  const auto result = cvub::RunInferAndSetBufferSize(std::move(input));
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
  CVUB_CHECK(!HasOperation(result.module, "annotation.mark"));
  const auto &backing = OperationNamed(result.module, "memref.alloc");
  CVUB_CHECK(cvub::IRDictionaryValue(backing.attributes, "alignment").empty());
}

CVUB_TEST(buffer_size_set_keeps_non_size_mark_alignment_and_multibuffer_peak) {
  auto input = cvub::ExtractFunctionModule(
      cvub::test::ParseFixture("dynamic_buffer_size.mlir"),
      "constant_bounds");
  auto &allocation = MutableOperationNamed(input, "memref.alloc");
  allocation.attributes = cvub::SetDictionaryValue(
      allocation.attributes, "alignment", "64 : i64");
  auto &mark = MutableOperationNamed(input, "annotation.mark");
  mark.attributes = cvub::SetDictionaryValue(
      mark.attributes, "hivm.multi_buffer", "2 : i64");

  auto result = cvub::RunInferAndSetBufferSize(std::move(input));
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
  const auto &remainingMark = OperationNamed(result.module, "annotation.mark");
  CVUB_CHECK(cvub::IRDictionaryValue(remainingMark.attributes,
                                     "buffer_size_in_byte").empty());
  CVUB_CHECK_EQ(cvub::IRDictionaryValue(remainingMark.attributes,
                                        "hivm.multi_buffer"),
                "2 : i64");
  const auto &backing = OperationNamed(result.module, "memref.alloc");
  CVUB_CHECK_EQ(cvub::IRDictionaryValue(backing.attributes, "alignment"),
                "64 : i64");

  auto &consume = MutableOperationNamed(result.module, "test.consume");
  consume.name = "hivm.hir.store";
  consume.operands[1] = backing.results.front();
  consume.operandTypes[1] = backing.resultTypes.front();
  cvub::ApplyOperationSemantics(consume);
  const cvub::PlanMemoryInput planInput =
      cvub::BuildSuffixPlanMemoryInput(result.module);
  CVUB_CHECK(std::any_of(
      planInput.operations.begin(), planInput.operations.end(),
      [](const cvub::OperationRecord &operation) {
        return operation.opName == "annotation.mark" &&
               operation.text.find("hivm.multi_buffer = 2") !=
                   std::string::npos;
      }));
  const auto plan = cvub::PlanLocalMemoryForSeed(planInput, 0);
  CVUB_CHECK(plan.success);
  CVUB_CHECK(plan.peakBits >= 512U);
}

CVUB_TEST(buffer_size_alias_mark_is_set_only_after_constantize) {
  auto input = cvub::ExtractFunctionModule(
      cvub::test::ParseFixture("dynamic_buffer_size.mlir"),
      "constant_bounds");
  auto &function = MutableOperationNamed(input, "func.func");
  function.attributes = cvub::RemoveDictionaryAttribute(
      function.attributes, "enable_auto_mark_buffer_size");
  auto &allocation = MutableOperationNamed(input, "memref.alloc");
  allocation.attributes =
      "{alignment = 64 : i64, discard_me = true}";
  const int allocationId = allocation.id;
  const int allocationValue = allocation.results.front();
  const std::string logicalType = allocation.resultTypes.front();
  const std::string aliasType =
      "memref<20xf16, strided<[2], offset: 1>, #hivm.address_space<UB>>";
  const int parentId = allocation.parentId;
  const int regionId = allocation.regionId;
  const int blockId = allocation.blockId;
  const int ordinal = allocation.ordinal;
  cvub::GenericRewriter rewriter(input);
  const int alias = rewriter.createOperation(
      parentId, regionId, blockId,
      "memref.subview", {aliasType}, {allocationValue}, {logicalType},
      "{static_offsets = array<i64: 0>, static_sizes = array<i64: 20>, "
          "static_strides = array<i64: 1>}", "{}");
  rewriter.insertToBlock(blockId, static_cast<size_t>(ordinal + 1), alias);
  auto &mark = MutableOperationNamed(input, "annotation.mark");
  mark.operands.front() =
      input.operations.at(static_cast<size_t>(alias)).results.front();
  mark.operandTypes.front() = aliasType;
  mark.attributes = cvub::SetDictionaryValue(
      mark.attributes, "hivm.multi_buffer", "2 : i64");

  auto result = cvub::RunInferAndSetBufferSize(std::move(input));
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
  const auto &backing = result.module.operations.at(
      static_cast<size_t>(allocationId));
  CVUB_CHECK_EQ(backing.resultTypes.front(),
                "memref<40xi8, #hivm.address_space<UB>>");
  CVUB_CHECK_EQ(backing.attributes, "{}");
  const auto &remainingMark = OperationNamed(result.module, "annotation.mark");
  CVUB_CHECK(cvub::IRDictionaryValue(remainingMark.attributes,
                                     "buffer_size_in_byte").empty());
  CVUB_CHECK_EQ(cvub::IRDictionaryValue(remainingMark.attributes,
                                        "hivm.multi_buffer"),
                "2 : i64");
  auto &consume = MutableOperationNamed(result.module, "test.consume");
  consume.name = "hivm.hir.store";
  const auto &aliasAfter = OperationNamed(result.module, "memref.subview");
  consume.operands[1] = aliasAfter.results.front();
  consume.operandTypes[1] = aliasAfter.resultTypes.front();
  cvub::ApplyOperationSemantics(consume);
  const cvub::PlanMemoryInput planInput =
      cvub::BuildSuffixPlanMemoryInput(result.module);
  const auto plan = cvub::PlanLocalMemoryForSeed(planInput, 0);
  CVUB_CHECK(plan.success);
  CVUB_CHECK(plan.peakBits >= 640U);
}

CVUB_TEST(buffer_size_set_preserves_only_explicit_alignment) {
  auto input = cvub::ExtractFunctionModule(
      cvub::test::ParseFixture("dynamic_buffer_size.mlir"),
      "constant_bounds");
  auto &allocation = MutableOperationNamed(input, "memref.alloc");
  allocation.attributes =
      "{alignment = 64 : i64, discard_me = true}";
  auto &stack = MutableOperationNamed(input, "memref.alloca");
  const int stackValue = stack.results.front();
  stack.operands.clear();
  stack.operandTypes.clear();
  stack.resultTypes.front() =
      "memref<20xf32, #hivm.address_space<UB>>";
  for (auto &operation : input.operations)
    for (size_t index = 0; index < operation.operands.size(); ++index)
      if (operation.operands[index] == stackValue)
        operation.operandTypes[index] = stack.resultTypes.front();
  const auto result = cvub::RunInferAndSetBufferSize(std::move(input));
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
  const auto &backing = OperationNamed(result.module, "memref.alloc");
  CVUB_CHECK_EQ(backing.attributes, "{alignment = 64 : i64}");
}

CVUB_TEST(buffer_size_alias_view_annotation_cannot_hide_unknown_value_bounds) {
  auto input = cvub::ExtractFunctionModule(
      cvub::test::ParseFixture("dynamic_buffer_size.mlir"),
      "conflicting_sizes");
  auto &allocation = MutableOperationNamed(input, "memref.alloc");
  const int allocationValue = allocation.results.front();
  const std::string logicalType = allocation.resultTypes.front();
  const int blockId = allocation.blockId;
  const int ordinal = allocation.ordinal;
  const int parentId = allocation.parentId;
  const int regionId = allocation.regionId;
  MutableOperationNamed(input, "arith.constant").name = "test.dynamic_dim";
  cvub::GenericRewriter rewriter(input);
  const int view = rewriter.createOperation(
      parentId, regionId, blockId, "memref.view", {logicalType},
      {allocationValue}, {logicalType});
  rewriter.insertToBlock(blockId, static_cast<size_t>(ordinal + 1), view);
  auto &firstMark = MutableOperationNamed(input, "annotation.mark", 0);
  firstMark.operands.front() =
      input.operations.at(static_cast<size_t>(view)).results.front();
  auto &secondMark = MutableOperationNamed(input, "annotation.mark", 1);
  rewriter.removeFromBlock(secondMark.blockId, secondMark.id);
  input = cvub::CompactGenericModule(std::move(input));

  const auto result = cvub::RunInferAndSetBufferSize(std::move(input));
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Incomplete);
  CVUB_CHECK(HasDiagnostic(result, "InferAndSetBufferSize",
                           "unsupported ValueBounds"));
}

CVUB_TEST(buffer_size_unsupported_affine_map_cannot_fall_back_to_set) {
  auto input = cvub::ExtractFunctionModule(
      cvub::test::ParseFixture("dynamic_buffer_size.mlir"),
      "constant_bounds");
  auto &maximum = MutableOperationNamed(input, "arith.maxui");
  maximum.name = "affine.apply";
  maximum.properties =
      "{map = affine_map<(d0, d1) -> (d0 * 2 + d1)>}";
  auto &stack = MutableOperationNamed(input, "memref.alloca");
  const int stackValue = stack.results.front();
  stack.operands.clear();
  stack.operandTypes.clear();
  stack.resultTypes.front() =
      "memref<20xf32, #hivm.address_space<UB>>";
  for (auto &operation : input.operations)
    for (size_t index = 0; index < operation.operands.size(); ++index)
      if (operation.operands[index] == stackValue)
        operation.operandTypes[index] = stack.resultTypes.front();
  const auto result = cvub::RunInferAndSetBufferSize(std::move(input));
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Incomplete);
  CVUB_CHECK(HasDiagnostic(result, "InferAndSetBufferSize",
                           "unsupported ValueBounds"));
}

CVUB_TEST(buffer_size_unsupported_affine_hidden_by_cast_and_add_fails_closed) {
  auto input = cvub::ExtractFunctionModule(
      cvub::test::ParseFixture("dynamic_buffer_size.mlir"),
      "constant_bounds");
  auto &apply = MutableOperationNamed(input, "arith.maxui");
  apply.name = "affine.apply";
  apply.properties =
      "{map = affine_map<(d0, d1) -> (d0 * 2 + d1)>}";
  auto &allocation = MutableOperationNamed(input, "memref.alloc");
  auto &stack = MutableOperationNamed(input, "memref.alloca");
  const int stackValue = stack.results.front();
  stack.operands.clear();
  stack.operandTypes.clear();
  stack.resultTypes.front() =
      "memref<20xf32, #hivm.address_space<UB>>";
  for (auto &operation : input.operations)
    for (size_t index = 0; index < operation.operands.size(); ++index)
      if (operation.operands[index] == stackValue)
        operation.operandTypes[index] = stack.resultTypes.front();
  const int blockId = allocation.blockId;
  const int parentId = allocation.parentId;
  const int regionId = allocation.regionId;
  const int ordinal = allocation.ordinal;
  const auto &zero = MutableOperationNamed(input, "arith.constant", 1);
  cvub::GenericRewriter rewriter(input);
  const int cast = rewriter.createOperation(
      parentId, regionId, blockId, "arith.index_cast", {"index"},
      {apply.results.front()}, {"index"});
  rewriter.insertToBlock(blockId, static_cast<size_t>(ordinal), cast);
  const int add = rewriter.createOperation(
      parentId, regionId, blockId, "arith.addi", {"index"},
      {input.operations.at(static_cast<size_t>(cast)).results.front(),
       zero.results.front()},
      {"index", "index"});
  rewriter.insertToBlock(blockId, static_cast<size_t>(ordinal + 1), add);
  allocation.operands.front() =
      input.operations.at(static_cast<size_t>(add)).results.front();
  cvub::ValidateGenericModule(input);

  const auto result = cvub::RunInferAndSetBufferSize(std::move(input));
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Incomplete);
  CVUB_CHECK(HasDiagnostic(result, "InferAndSetBufferSize",
                           "unsupported ValueBounds"));
}

CVUB_TEST(buffer_size_affine_negative_division_uses_mlir_semantics) {
  auto input = cvub::ExtractFunctionModule(
      cvub::test::ParseFixture("dynamic_buffer_size.mlir"),
      "constant_bounds");
  auto &seven = MutableOperationNamed(input, "arith.constant", 0);
  seven.attributes = cvub::SetDictionaryValue(seven.attributes, "value",
                                               "-7 : index");
  auto &apply = MutableOperationNamed(input, "arith.maxui");
  apply.name = "affine.apply";
  apply.operands = {seven.results.front(),
                    MutableOperationNamed(input, "arith.constant", 1)
                        .results.front()};
  for (const auto &[spelling, expected] :
       std::vector<std::pair<std::string, int64_t>>{
           {"ceildiv", -1}, {"floordiv", -2}, {"mod", 1}}) {
    apply.properties = "{map = affine_map<(d0, d1) -> (d0 " + spelling +
                       " d1)>}";
    const auto bound = cvub::ResolveBufferSizeBound(input,
                                                     apply.results.front());
    CVUB_CHECK(bound.has_value());
    CVUB_CHECK_EQ(bound->value, expected);
  }
}

CVUB_TEST(buffer_size_remsi_uses_affine_apply_operand_bound_semantics) {
  auto input = cvub::ExtractFunctionModule(
      cvub::test::ParseFixture("dynamic_buffer_size.mlir"),
      "constant_bounds");
  auto &ceil = MutableOperationNamed(input, "arith.ceildivui");
  ceil.name = "test.dynamic_dim";
  auto &maximum = MutableOperationNamed(input, "arith.maxui");
  maximum.name = "arith.minsi";
  auto &four = input.operations.at(
      static_cast<size_t>(DefinitionOf(input, ceil.operands[1]).id));
  four.attributes = cvub::SetDictionaryValue(four.attributes, "value",
                                              "8 : index");
  auto &allocation = MutableOperationNamed(input, "memref.alloc");
  const int parentId = allocation.parentId;
  const int regionId = allocation.regionId;
  const int blockId = allocation.blockId;
  cvub::GenericRewriter rewriter(input);
  const int rem = rewriter.createOperation(
      parentId, regionId, blockId, "arith.remsi", {"index"},
      {maximum.results.front(), four.results.front()}, {"index", "index"});
  rewriter.insertToBlock(blockId, static_cast<size_t>(allocation.ordinal), rem);
  allocation.operands.front() =
      input.operations.at(static_cast<size_t>(rem)).results.front();
  for (auto &operation : input.operations)
    if (operation.name == "annotation.mark")
      operation.attributes = cvub::RemoveDictionaryAttribute(
          operation.attributes, "buffer_size_in_byte");

  const auto result = cvub::RunInferAndSetBufferSize(std::move(input));
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
  bool sawFourElementBacking = false;
  for (const auto &operation : result.module.operations)
    if (operation.name == "memref.alloc" &&
        operation.resultTypes.front() ==
            "memref<8xi8, #hivm.address_space<UB>>")
      sawFourElementBacking = true;
  CVUB_CHECK(sawFourElementBacking);
}

CVUB_TEST(buffer_size_auto_infer_signed_overflow_is_incomplete) {
  auto input = cvub::ExtractFunctionModule(
      cvub::test::ParseFixture("dynamic_buffer_size.mlir"), "dynamic_sizes");
  auto &mark = MutableOperationNamed(input, "annotation.mark");
  mark.attributes = cvub::SetDictionaryValue(
      mark.attributes, "buffer_size_in_byte",
      "9223372036854775807 : i64");
  const auto before = cvub::SerializeGenericModule(input);
  const auto result = cvub::RunInferAndSetBufferSize(std::move(input));
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Incomplete);
  CVUB_CHECK(HasDiagnostic(result, "InferAndSetBufferSize", "overflows"));
  CVUB_CHECK_EQ(cvub::SerializeGenericModule(result.module), before);
}

CVUB_TEST(pre_split_canonicalization_reaches_fixed_point) {
  auto module = cvub::ExtractFunctionModule(
      cvub::test::ParseFixture("dynamic_buffer_size.mlir"), "canonicalize");
  const auto result = cvub::RunPreSplitCanonicalization(std::move(module));
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
  CVUB_CHECK(!HasOperation(result.module, "tensor.empty"));
  CVUB_CHECK(!HasOperation(result.module, "annotation.mark"));
  CVUB_CHECK(!HasOperation(result.module, "scf.for"));
  CVUB_CHECK(!HasOperation(result.module, "arith.addi"));
  CVUB_CHECK_EQ(OperationCount(result.module, "arith.constant"), 1U);
  const auto &consume = OperationNamed(result.module, "test.consume");
  CVUB_CHECK_EQ(consume.operands.front(), consume.operands.back());
  cvub::ValidateGenericModule(result.module);
}

CVUB_TEST(pre_split_canonicalization_blocks_unmodeled_real_pass_patterns) {
  auto module = cvub::ExtractFunctionModule(
      cvub::test::ParseFixture("workspace_sync_invariant.mlir"),
      "workspace_sync");
  auto &load = MutableOperationNamed(module, "hivm.hir.load");
  load.name = "scf.if";
  const auto result = cvub::RunPreSplitCanonicalization(std::move(module));
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Incomplete);
  CVUB_CHECK(HasDiagnostic(result, "CanonicalizationBeforeSplit",
                           "unmodeled applicable"));
}

CVUB_TEST(pre_split_canonicalization_blocks_legal_zero_trip_loop) {
  auto module = cvub::ExtractFunctionModule(
      cvub::test::ParseFixture("dynamic_buffer_size.mlir"), "canonicalize");
  auto &loop = MutableOperationNamed(module, "scf.for");
  loop.operands[1] = loop.operands[0];
  const auto before = cvub::SerializeGenericModule(module);
  const auto result = cvub::RunPreSplitCanonicalization(std::move(module));
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Incomplete);
  CVUB_CHECK(HasDiagnostic(result, "CanonicalizationBeforeSplit",
                           "zero-trip"));
  CVUB_CHECK_EQ(cvub::SerializeGenericModule(result.module), before);
}

CVUB_TEST(pre_split_canonicalization_blocks_legal_unused_loop_result_fold) {
  auto module = cvub::ExtractFunctionModule(
      cvub::test::ParseFixture("dynamic_buffer_size.mlir"), "canonicalize");
  auto &consume = MutableOperationNamed(module, "test.consume");
  consume.operands.pop_back();
  consume.operandTypes.pop_back();
  auto &yield = MutableOperationNamed(module, "scf.yield");
  yield.operands.front() = consume.operands.front();
  yield.operandTypes.front() = consume.operandTypes.front();
  cvub::ValidateGenericModule(module);
  const auto result = cvub::RunPreSplitCanonicalization(std::move(module));
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Incomplete);
  CVUB_CHECK(HasDiagnostic(result, "CanonicalizationBeforeSplit",
                           "unused-result fold"));
}

CVUB_TEST(pre_split_canonicalization_allows_nonfoldable_live_tensor_empty) {
  auto module = cvub::ExtractFunctionModule(
      cvub::test::ParseFixture("dynamic_buffer_size.mlir"), "canonicalize");
  auto &mark = MutableOperationNamed(module, "annotation.mark");
  mark.attributes = cvub::SetDictionaryValue(mark.attributes,
                                              "hivm.multi_buffer",
                                              "2 : i64");
  const auto result = cvub::RunPreSplitCanonicalization(std::move(module));
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
}

CVUB_TEST(pre_split_canonicalization_does_not_block_inapplicable_slice) {
  auto module = cvub::ExtractFunctionModule(
      cvub::test::ParseFixture("tile_vector_loop.mlir"), "tile_vector");
  const auto result = cvub::RunPreSplitCanonicalization(std::move(module));
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
}

CVUB_TEST(pre_split_canonicalization_blocks_applicable_hivm_patterns) {
  const std::vector<std::string> names = {
      "hivm.hir.vpow",      "hivm.hir.vreduce", "hivm.hir.vcumsum",
      "hivm.hir.vcumprod",  "hivm.hir.vtranspose",
      "hivm.hir.vpad",      "hivm.hir.debug", "hivm.hir.vbrc"};
  for (const std::string &name : names) {
    auto module = cvub::ExtractFunctionModule(
        cvub::test::ParseFixture("dynamic_buffer_size.mlir"), "canonicalize");
    auto &candidate = MutableOperationNamed(module, "annotation.mark");
    candidate.name = name;
    candidate.attributes = "{}";
    if (name == "hivm.hir.vpow") {
      auto &exponent = MutableOperationNamed(module, "arith.constant", 2);
      exponent.resultTypes.front() = "f32";
      exponent.attributes = cvub::SetDictionaryValue(
          exponent.attributes, "value", "3.0 : f32");
      candidate.operands.push_back(exponent.results.front());
      candidate.operandTypes.push_back(exponent.resultTypes.front());
    } else if (name == "hivm.hir.vreduce" ||
               name == "hivm.hir.vcumsum" ||
               name == "hivm.hir.vcumprod") {
      auto &empty = MutableOperationNamed(module, "tensor.empty");
      empty.resultTypes.front() = "tensor<4x1xf32>";
      candidate.operandTypes.front() = empty.resultTypes.front();
      candidate.attributes = name == "hivm.hir.vreduce"
                                 ? "{reduce_dims = [1]}"
                                 : "{cum_dims = [1]}";
    } else if (name == "hivm.hir.vtranspose") {
      candidate.attributes = "{permutation = [0]}";
    } else if (name == "hivm.hir.vpad") {
      auto &empty = MutableOperationNamed(module, "tensor.empty");
      auto &load = MutableOperationNamed(module, "arith.addi");
      load.name = "hivm.hir.load";
      load.resultTypes.front() = "tensor<4xf32>";
      empty.name = "hivm.hir.vpad";
      empty.operands = {load.results.front()};
      empty.operandTypes = {load.resultTypes.front()};
      candidate.name = "annotation.mark";
    } else if (name == "hivm.hir.debug") {
      const auto &condition = MutableOperationNamed(module, "arith.constant", 1);
      candidate.operands.front() = condition.results.front();
      candidate.operandTypes.front() = "i1";
      MutableOperationNamed(module, "arith.constant", 1).resultTypes.front() =
          "i1";
      candidate.attributes = "{debugtype = \"assert\"}";
    } else if (name == "hivm.hir.vbrc") {
      auto &empty = MutableOperationNamed(module, "tensor.empty");
      empty.resultTypes.front() = "tensor<4x1xf32>";
      const auto &scalar = MutableOperationNamed(module, "arith.constant", 2);
      candidate.operands = {scalar.results.front(), empty.results.front()};
      candidate.operandTypes = {scalar.resultTypes.front(),
                                empty.resultTypes.front()};
      candidate.attributes = "{broadcast_dims = [1]}";
      cvub::ApplyOperationSemantics(candidate);
    }
    const auto result = cvub::RunPreSplitCanonicalization(std::move(module));
    CVUB_CHECK_EQ(result.precision, cvub::Precision::Incomplete);
    CVUB_CHECK(HasDiagnostic(result, "CanonicalizationBeforeSplit",
                             "HIVM canonicalization"));
  }
}

CVUB_TEST(pre_split_canonicalization_allows_proven_hivm_noops) {
  const std::vector<std::string> names = {
      "hivm.hir.vpow",      "hivm.hir.vreduce", "hivm.hir.vcumsum",
      "hivm.hir.vcumprod",  "hivm.hir.vtranspose",
      "hivm.hir.vpad",      "hivm.hir.debug", "hivm.hir.vbrc"};
  for (const std::string &name : names) {
    auto module = cvub::ExtractFunctionModule(
        cvub::test::ParseFixture("dynamic_buffer_size.mlir"), "canonicalize");
    auto &candidate = MutableOperationNamed(module, "annotation.mark");
    candidate.name = name;
    candidate.attributes = "{}";
    if (name == "hivm.hir.vpow") {
      const int nonConstant = ResultOf(module, "scf.for");
      candidate.operands.push_back(nonConstant);
      candidate.operandTypes.push_back("i32");
    } else if (name == "hivm.hir.vreduce" ||
               name == "hivm.hir.vcumsum" ||
               name == "hivm.hir.vcumprod") {
      candidate.attributes = name == "hivm.hir.vreduce"
                                 ? "{reduce_dims = [0]}"
                                 : "{cum_dims = [0]}";
    } else if (name == "hivm.hir.vtranspose") {
      candidate.attributes = "{permutation = [1, 0]}";
    } else if (name == "hivm.hir.debug") {
      candidate.attributes = "{debugtype = \"print\"}";
    } else if (name == "hivm.hir.vbrc") {
      auto &empty = MutableOperationNamed(module, "tensor.empty");
      empty.resultTypes.front() = "tensor<?xf32>";
      candidate.operandTypes.front() = empty.resultTypes.front();
      candidate.attributes = "{broadcast_dims = [0]}";
    }
    const auto result = cvub::RunPreSplitCanonicalization(std::move(module));
    CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
  }
}

CVUB_TEST(pre_split_canonicalization_blocks_registered_hivm_aux_patterns) {
  for (const std::string &sourceName :
       {std::string("tensor.empty"), std::string("tensor.cast")}) {
    auto module = cvub::ExtractFunctionModule(
        cvub::test::ParseFixture("dynamic_buffer_size.mlir"), "canonicalize");
    auto &source = MutableOperationNamed(module, "tensor.empty");
    source.name = sourceName;
    auto &mark = MutableOperationNamed(module, "annotation.mark");
    mark.name = "hivm.hir.convert_layout";
    const auto result = cvub::RunPreSplitCanonicalization(std::move(module));
    CVUB_CHECK_EQ(result.precision, cvub::Precision::Incomplete);
    CVUB_CHECK(HasDiagnostic(result, "CanonicalizationBeforeSplit",
                             "convert-layout"));
  }

  auto module = cvub::ExtractFunctionModule(
      cvub::test::ParseFixture("dynamic_buffer_size.mlir"), "canonicalize");
  auto &empty = MutableOperationNamed(module, "tensor.empty");
  empty.name = "memref.cast";
  auto &mark = MutableOperationNamed(module, "annotation.mark");
  mark.name = "hivm.hir.copy";
  const auto result = cvub::RunPreSplitCanonicalization(std::move(module));
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Incomplete);
  CVUB_CHECK(HasDiagnostic(result, "CanonicalizationBeforeSplit",
                           "memref-cast"));
}

CVUB_TEST(pre_split_canonicalization_blocks_unmodeled_scf_iterarg_chain) {
  auto module = cvub::ExtractFunctionModule(
      cvub::test::ParseFixture("dynamic_buffer_size.mlir"), "canonicalize");
  auto &loop = MutableOperationNamed(module, "scf.for");
  auto &yield = MutableOperationNamed(module, "scf.yield");
  const int yieldId = yield.id;
  const int blockId = yield.blockId;
  const int regionId = yield.regionId;
  cvub::GenericRewriter rewriter(module);
  const int nested = rewriter.createOperation(
      loop.id, regionId, blockId, "scf.if", {"i32"});
  rewriter.insertToBlock(blockId, static_cast<size_t>(yield.ordinal), nested);
  module.operations.at(static_cast<size_t>(yieldId)).operands.front() =
      module.operations.at(static_cast<size_t>(nested)).results.front();
  module.operations.at(static_cast<size_t>(yieldId)).operandTypes.front() =
      "i32";
  const auto result = cvub::RunPreSplitCanonicalization(std::move(module));
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Incomplete);
  CVUB_CHECK(HasDiagnostic(result, "CanonicalizationBeforeSplit",
                           "iter-arg"));
}

CVUB_TEST(pre_split_canonicalization_replaces_live_external_tensor_yield) {
  auto module = cvub::ExtractFunctionModule(
      cvub::test::ParseFixture("dynamic_buffer_size.mlir"), "canonicalize");
  auto &loop = MutableOperationNamed(module, "scf.for");
  auto &yield = MutableOperationNamed(module, "scf.yield");
  auto &consume = MutableOperationNamed(module, "test.consume");
  const auto &external = MutableOperationNamed(module, "tensor.empty");
  const int externalValue = external.results.front();
  const int blockId = loop.blockId;
  const int parentId = loop.parentId;
  const int regionId = loop.regionId;
  const int ordinal = loop.ordinal;
  cvub::GenericRewriter rewriter(module);
  const int upper = rewriter.createOperation(
      parentId, regionId, blockId, "arith.constant", {"index"}, {}, {}, "",
      "{value = 2 : index}");
  rewriter.insertToBlock(blockId, static_cast<size_t>(ordinal), upper);
  const int init = rewriter.createOperation(
      parentId, regionId, blockId, "tensor.empty", {"tensor<4xf32>"}, {}, {},
      "", "{case = \"external_init\"}");
  rewriter.insertToBlock(blockId, static_cast<size_t>(ordinal + 1), init);
  loop.operands[1] =
      module.operations.at(static_cast<size_t>(upper)).results.front();
  loop.operands[3] =
      module.operations.at(static_cast<size_t>(init)).results.front();
  loop.operandTypes[3] = "tensor<4xf32>";
  loop.resultTypes.front() = "tensor<4xf32>";
  const int bodyId = module.regions.at(static_cast<size_t>(loop.regions.front()))
                         .blocks.front();
  auto &body = module.blocks.at(static_cast<size_t>(bodyId));
  body.argumentTypes[1] = "tensor<4xf32>";
  yield.operands.front() = externalValue;
  yield.operandTypes.front() = "tensor<4xf32>";
  consume.operands.back() = loop.results.front();
  consume.operandTypes.back() = "tensor<4xf32>";
  cvub::ValidateGenericModule(module);

  const auto result = cvub::RunPreSplitCanonicalization(std::move(module));
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
  const auto &updatedConsume = OperationNamed(result.module, "test.consume");
  const auto &replacement = DefinitionOf(result.module,
                                          updatedConsume.operands.back());
  CVUB_CHECK_EQ(replacement.name, "tensor.empty");
  CVUB_CHECK(replacement.id != OperationNamed(result.module, "scf.for").id);
}

CVUB_TEST(pre_split_canonicalization_uses_shaped_vbrc_unit_destination) {
  auto module = cvub::ExtractFunctionModule(
      cvub::test::ParseFixture("dynamic_buffer_size.mlir"), "canonicalize");
  auto &source = MutableOperationNamed(module, "tensor.empty");
  source.resultTypes.front() = "tensor<4x1xi32>";
  const int sourceValue = source.results.front();
  const auto &mark = MutableOperationNamed(module, "annotation.mark");
  const int markId = mark.id;
  const int markBlockId = mark.blockId;
  const int blockId = mark.blockId;
  const int parentId = mark.parentId;
  const int regionId = mark.regionId;
  const int ordinal = mark.ordinal;
  cvub::GenericRewriter rewriter(module);
  const int destination = rewriter.createOperation(
      parentId, regionId, blockId, "tensor.empty", {"tensor<4x1xi32>"});
  rewriter.insertToBlock(blockId, static_cast<size_t>(ordinal), destination);
  const int destinationValue =
      module.operations.at(static_cast<size_t>(destination)).results.front();
  const int vbrc = rewriter.createOperation(
      parentId, regionId, blockId, "hivm.hir.vbrc", {"tensor<4x1xi32>"},
      {sourceValue, destinationValue},
      {"tensor<4x1xi32>", "tensor<4x1xi32>"},
      "{operandSegmentSizes = array<i32: 1, 1, 0>, "
      "broadcast_dims = array<i64: 1>}");
  cvub::ApplyOperationSemantics(
      module.operations.at(static_cast<size_t>(vbrc)));
  rewriter.insertToBlock(blockId, static_cast<size_t>(ordinal + 1), vbrc);
  rewriter.removeFromBlock(markBlockId, markId);
  module = cvub::CompactGenericModule(std::move(module));
  cvub::ValidateGenericModule(module);

  const auto result = cvub::RunPreSplitCanonicalization(std::move(module));
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Incomplete);
  CVUB_CHECK(HasDiagnostic(result, "CanonicalizationBeforeSplit",
                           "HIVM canonicalization"));
}

CVUB_TEST(pre_split_canonicalization_traces_vreduce_init_through_affine_for) {
  auto module = cvub::ExtractFunctionModule(
      cvub::test::ParseFixture("dynamic_buffer_size.mlir"),
      "affine_loop_vreduce");
  cvub::ValidateGenericModule(module);
  auto &reduce = MutableOperationNamed(module, "hivm.hir.vreduce");
  cvub::ApplyOperationSemantics(reduce);
  CVUB_CHECK(!reduce.dpsInits.empty());
  CVUB_CHECK(cvub::TracesToVReduceConstantFill(
      module, reduce.dpsInits.front(), reduce, true));
  const auto result = cvub::RunPreSplitCanonicalization(std::move(module));
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Incomplete);
  CVUB_CHECK(HasDiagnostic(result, "CanonicalizationBeforeSplit",
                           "HIVM canonicalization"));
}

CVUB_TEST(pre_split_canonicalization_proves_scf_forall_output_init_schema) {
  auto module = cvub::ExtractFunctionModule(
      cvub::test::ParseFixture("dynamic_buffer_size.mlir"),
      "affine_loop_vreduce");
  auto &loop = MutableOperationNamed(module, "affine.for");
  loop.name = "scf.forall";
  loop.properties =
      "{operandSegmentSizes = array<i32: 1, 1, 0, 1>, "
      "staticLowerBound = array<i64: -1>, "
      "staticUpperBound = array<i64: -1>, staticStep = array<i64: 1>}";
  auto &reduce = MutableOperationNamed(module, "hivm.hir.vreduce");
  cvub::ApplyOperationSemantics(reduce);
  CVUB_CHECK(!reduce.dpsInits.empty());
  CVUB_CHECK(cvub::TracesToVReduceConstantFill(
      module, reduce.dpsInits.front(), reduce, true));
}

CVUB_TEST(pre_split_canonicalization_blocks_unproven_loop_init_trace) {
  auto module = cvub::ExtractFunctionModule(
      cvub::test::ParseFixture("dynamic_buffer_size.mlir"),
      "affine_loop_vreduce");
  MutableOperationNamed(module, "affine.for").name = "scf.parallel";
  cvub::ApplyOperationSemantics(
      MutableOperationNamed(module, "hivm.hir.vreduce"));
  const auto result = cvub::RunPreSplitCanonicalization(std::move(module));
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Incomplete);
  CVUB_CHECK(HasDiagnostic(result, "CanonicalizationBeforeSplit",
                           "cannot prove loop init mapping"));
}

CVUB_TEST(pre_split_canonicalization_traces_vreduce_init_through_reshape) {
  auto module = cvub::ExtractFunctionModule(
      cvub::test::ParseFixture("dynamic_buffer_size.mlir"), "canonicalize");
  auto &source = MutableOperationNamed(module, "tensor.empty");
  source.resultTypes.front() = "tensor<4xf32>";
  auto &mark = MutableOperationNamed(module, "annotation.mark");
  const int sourceValue = source.results.front();
  const int markId = mark.id;
  const int markBlockId = mark.blockId;
  const int blockId = mark.blockId;
  const int parentId = mark.parentId;
  const int regionId = mark.regionId;
  const int ordinal = mark.ordinal;
  cvub::GenericRewriter rewriter(module);
  const int zero = rewriter.createOperation(
      parentId, regionId, blockId, "arith.constant", {"f32"}, {}, {}, "",
      "{value = 0.0 : f32}");
  rewriter.insertToBlock(blockId, static_cast<size_t>(ordinal), zero);
  const int zeroValue =
      module.operations.at(static_cast<size_t>(zero)).results.front();
  const int initEmpty = rewriter.createOperation(
      parentId, regionId, blockId, "tensor.empty", {"tensor<1xf32>"});
  rewriter.insertToBlock(blockId, static_cast<size_t>(ordinal + 1), initEmpty);
  const int fill = rewriter.createOperation(
      parentId, regionId, blockId, "hivm.hir.vbrc", {"tensor<1xf32>"},
      {zeroValue,
       module.operations.at(static_cast<size_t>(initEmpty)).results.front()},
      {"f32", "tensor<1xf32>"}, "{broadcast_dims = array<i64>} ");
  cvub::ApplyOperationSemantics(
      module.operations.at(static_cast<size_t>(fill)));
  rewriter.insertToBlock(blockId, static_cast<size_t>(ordinal + 2), fill);
  const int expand = rewriter.createOperation(
      parentId, regionId, blockId, "tensor.expand_shape", {"tensor<1x1xf32>"},
      {module.operations.at(static_cast<size_t>(fill)).results.front()},
      {"tensor<1xf32>"}, "{reassociation = [[0, 1]]}");
  rewriter.insertToBlock(blockId, static_cast<size_t>(ordinal + 3), expand);
  const int collapse = rewriter.createOperation(
      parentId, regionId, blockId, "tensor.collapse_shape", {"tensor<1xf32>"},
      {module.operations.at(static_cast<size_t>(expand)).results.front()},
      {"tensor<1x1xf32>"}, "{reassociation = [[0, 1]]}");
  rewriter.insertToBlock(blockId, static_cast<size_t>(ordinal + 4), collapse);
  const int reduce = rewriter.createOperation(
      parentId, regionId, blockId, "hivm.hir.vreduce", {"tensor<1xf32>"},
      {sourceValue,
       module.operations.at(static_cast<size_t>(collapse)).results.front()},
      {"tensor<4xf32>", "tensor<1xf32>"},
      "{operandSegmentSizes = array<i32: 1, 1, 0, 0>, "
      "reduce_dims = array<i64: 0>, arith = #hivm.reduce_op<sum>}");
  cvub::ApplyOperationSemantics(
      module.operations.at(static_cast<size_t>(reduce)));
  rewriter.insertToBlock(blockId, static_cast<size_t>(ordinal + 5), reduce);
  // Exercise the real recursive chain through a loop region iter argument.
  auto &loop = MutableOperationNamed(module, "scf.for");
  const int loopResult = loop.results.front();
  const int loopBodyId =
      module.regions.at(static_cast<size_t>(loop.regions.front())).blocks.front();
  auto &loopBody = module.blocks.at(static_cast<size_t>(loopBodyId));
  const int iterArg = loopBody.arguments[1];
  const int collapseValue =
      module.operations.at(static_cast<size_t>(collapse)).results.front();
  loop.operands[3] = collapseValue;
  loop.operandTypes[3] = "tensor<1xf32>";
  loop.resultTypes.front() = "tensor<1xf32>";
  loopBody.argumentTypes[1] = "tensor<1xf32>";
  auto &yield = MutableOperationNamed(module, "scf.yield");
  const int yieldId = yield.id;
  yield.operands.front() = iterArg;
  yield.operandTypes.front() = "tensor<1xf32>";
  auto &reduceOpBeforeMove =
      module.operations.at(static_cast<size_t>(reduce));
  reduceOpBeforeMove.operands[1] = iterArg;
  reduceOpBeforeMove.operandTypes[1] = "tensor<1xf32>";
  reduceOpBeforeMove.dpsInputs.clear();
  reduceOpBeforeMove.dpsInits.clear();
  cvub::ApplyOperationSemantics(reduceOpBeforeMove);
  cvub::MoveOperationBefore(module, reduce, yieldId);
  for (auto &operation : module.operations)
    for (size_t index = 0; index < operation.operands.size(); ++index)
      if (operation.operands[index] == loopResult)
        operation.operandTypes[index] = "tensor<1xf32>";
  rewriter.removeFromBlock(markBlockId, markId);
  module = cvub::CompactGenericModule(std::move(module));
  cvub::ValidateGenericModule(module);
  const auto &reduceOp = OperationNamed(module, "hivm.hir.vreduce");
  CVUB_CHECK(!reduceOp.dpsInits.empty());
  CVUB_CHECK(cvub::TracesToVReduceConstantFill(
      module, reduceOp.dpsInits.front(), reduceOp, true));

  const auto result = cvub::RunPreSplitCanonicalization(std::move(module));
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Incomplete);
  CVUB_CHECK(HasDiagnostic(result, "CanonicalizationBeforeSplit",
                           "unmodeled applicable"));
}

CVUB_TEST(workspace_offsets_and_operandless_sync_are_ub_invariant) {
  const auto baseline =
      cvub::test::ParseFixture("workspace_sync_invariant.mlir");
  auto changed = baseline;
  auto &workspace = MutableOperationNamed(changed,
                                          "memref_ext.alloc_workspace");
  workspace.attributes = cvub::SetDictionaryValue(
      workspace.attributes, "workspace_offset", "4096 : i64");
  auto workspaceResult =
      cvub::VerifyWorkspaceUBInvariant(baseline, std::move(changed));
  CVUB_CHECK_EQ(workspaceResult.precision, cvub::Precision::Exact);
  auto syncResult =
      cvub::VerifyCrossCoreSyncUBInvariant(std::move(workspaceResult.module));
  CVUB_CHECK_EQ(syncResult.precision, cvub::Precision::Exact);
}

CVUB_TEST(workspace_pass_uses_structural_proof_not_self_comparison) {
  auto module = cvub::test::ParseFixture("workspace_sync_invariant.mlir");
  auto exact = cvub::ProveWorkspacePlanningUBInvariant(module);
  CVUB_CHECK_EQ(exact.precision, cvub::Precision::Exact);
  MutableOperationNamed(module, "memref_ext.alloc_workspace")
      .resultTypes.front() = "memref<256xi8, #hivm.address_space<UB>>";
  auto blocked = cvub::ProveWorkspacePlanningUBInvariant(std::move(module));
  CVUB_CHECK_EQ(blocked.precision, cvub::Precision::Incomplete);
  CVUB_CHECK(HasDiagnostic(blocked, "WorkspaceSemanticProjection",
                           "cannot prove"));
}

CVUB_TEST(workspace_and_sync_invariants_fail_closed) {
  const auto baseline =
      cvub::test::ParseFixture("workspace_sync_invariant.mlir");
  auto changed = baseline;
  MutableOperationNamed(changed, "hivm.hir.load").resultTypes.front() =
      "tensor<32xf32>";
  auto workspace = cvub::VerifyWorkspaceUBInvariant(baseline, std::move(changed));
  CVUB_CHECK_EQ(workspace.precision, cvub::Precision::Incomplete);
  CVUB_CHECK(HasDiagnostic(workspace, "WorkspaceSemanticProjection",
                           "AIV load result shape"));

  auto syncModule = baseline;
  auto &sync = MutableOperationNamed(syncModule, "hivm.hir.sync_block");
  const auto &load = OperationNamed(syncModule, "hivm.hir.load");
  sync.operands = {load.operands.front()};
  sync.operandTypes = {"memref<256xi8, #hivm.address_space<UB>>"};
  auto syncResult =
      cvub::VerifyCrossCoreSyncUBInvariant(std::move(syncModule));
  CVUB_CHECK_EQ(syncResult.precision, cvub::Precision::Incomplete);
  CVUB_CHECK(HasDiagnostic(syncResult, "CrossCoreSyncInvariant",
                           "local buffer"));
}

CVUB_TEST(workspace_invariant_matching_is_independent_of_ids_cases_and_order) {
  auto baseline = cvub::test::ParseFixture("workspace_sync_invariant.mlir");
  auto changed = baseline;
  for (auto &operation : changed.operations)
    if (!cvub::IRDictionaryValue(operation.attributes, "case").empty())
      operation.attributes = cvub::SetDictionaryValue(
          operation.attributes, "case", "\"renamed_case_" +
                                                std::to_string(operation.id) +
                                                "\"");
  auto &block = changed.blocks.at(1);
  std::swap(block.operations[0], block.operations[1]);
  for (size_t index = 0; index < block.operations.size(); ++index)
    changed.operations.at(static_cast<size_t>(block.operations[index])).ordinal =
        static_cast<int>(index);
  const auto &anchor = changed.operations.at(
      static_cast<size_t>(changed.blocks.at(1).operations.front()));
  const int anchorParent = anchor.parentId;
  const int anchorRegion = anchor.regionId;
  const int anchorBlock = anchor.blockId;
  cvub::GenericRewriter rewriter(changed);
  const int extra = rewriter.createOperation(
      anchorParent, anchorRegion, anchorBlock, "arith.constant",
      {"index"}, {}, {}, "", "{value = 0 : index}");
  rewriter.insertToBlock(anchorBlock, 0, extra);
  changed = cvub::CompactGenericModule(std::move(changed));
  auto result = cvub::VerifyWorkspaceUBInvariant(baseline, std::move(changed));
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
}

CVUB_TEST(workspace_invariant_detects_add_remove_alias_and_local_shape_changes) {
  const auto baseline = cvub::test::ParseFixture("workspace_sync_invariant.mlir");
  auto aliasChanged = baseline;
  auto &workspace = MutableOperationNamed(aliasChanged,
                                          "memref_ext.alloc_workspace");
  workspace.attributes = cvub::SetDictionaryValue(workspace.attributes,
                                                   "alias_source", "\"arg1\"");
  auto alias = cvub::VerifyWorkspaceUBInvariant(baseline,
                                                std::move(aliasChanged));
  CVUB_CHECK_EQ(alias.precision, cvub::Precision::Incomplete);
  CVUB_CHECK(HasDiagnostic(alias, "WorkspaceSemanticProjection",
                           "alias source"));

  auto localChanged = baseline;
  auto &load = MutableOperationNamed(localChanged, "hivm.hir.load");
  load.resultTypes.front() = "tensor<32xf32>";
  auto local = cvub::VerifyWorkspaceUBInvariant(baseline,
                                                std::move(localChanged));
  CVUB_CHECK_EQ(local.precision, cvub::Precision::Incomplete);
  CVUB_CHECK(HasDiagnostic(local, "WorkspaceSemanticProjection",
                           "AIV load result shape"));

  auto removedModule = baseline;
  MutableOperationNamed(removedModule, "hivm.hir.load").name = "test.removed";
  auto removed = cvub::VerifyWorkspaceUBInvariant(baseline,
                                                  std::move(removedModule));
  CVUB_CHECK_EQ(removed.precision, cvub::Precision::Incomplete);
  CVUB_CHECK(HasDiagnostic(removed, "WorkspaceSemanticProjection", "removed"));

  auto addedModule = baseline;
  const auto &loadAnchor = OperationNamed(addedModule, "hivm.hir.load");
  const int loadParent = loadAnchor.parentId;
  const int loadRegion = loadAnchor.regionId;
  const int loadBlock = loadAnchor.blockId;
  const int loadOrdinal = loadAnchor.ordinal;
  cvub::GenericRewriter addedRewriter(addedModule);
  const int addedAlloc = addedRewriter.createOperation(
      loadParent, loadRegion, loadBlock,
      "memref.alloc", {"memref<16xi8, #hivm.address_space<UB>>"});
  addedRewriter.insertToBlock(loadBlock,
                              static_cast<size_t>(loadOrdinal),
                              addedAlloc);
  auto added = cvub::VerifyWorkspaceUBInvariant(baseline,
                                                std::move(addedModule));
  CVUB_CHECK_EQ(added.precision, cvub::Precision::Incomplete);
  CVUB_CHECK(HasDiagnostic(added, "WorkspaceSemanticProjection", "added"));
}

CVUB_TEST(inline_scope_moves_one_block_body_in_order_and_maps_all_results) {
  const auto result = cvub::RunInlineScope(
      cvub::test::ParseFixture("scope_tensor_empty.mlir"));
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
  CVUB_CHECK(!HasOperationCase(result.module, "inline_scope"));
  CVUB_CHECK_EQ(OperationsWithCase(result.module, "kept_scope").size(), 1U);
  const auto &first = OperationWithCase(result.module, "scope_first");
  const auto &second = OperationWithCase(result.module, "scope_second");
  const auto &user = OperationWithCase(result.module, "scope_user");
  CVUB_CHECK_EQ(first.blockId, user.blockId);
  CVUB_CHECK_EQ(second.blockId, user.blockId);
  CVUB_CHECK(first.ordinal < second.ordinal);
  CVUB_CHECK(second.ordinal < user.ordinal);
  CVUB_CHECK_EQ(user.operands[0], first.results.front());
  CVUB_CHECK_EQ(user.operands[1], second.results.front());
  CVUB_CHECK_EQ(user.operands[2], first.results.front());
  cvub::ValidateGenericModule(result.module);
}

CVUB_TEST(inline_scope_fail_closes_and_preserves_unsupported_region_shape) {
  auto module = cvub::test::ParseFixture("scope_tensor_empty.mlir");
  const cvub::GenericOperation scope =
      MutableOperationNamed(module, "scope.scope");
  const int region = scope.regions.front();
  cvub::GenericRewriter rewriter(module);
  const int extraBlock = rewriter.createBlock(region, {});
  const int firstEmpty = rewriter.createOperation(
      scope.id, region, extraBlock, "tensor.empty", {scope.resultTypes[0]});
  rewriter.appendToBlock(extraBlock, firstEmpty);
  const int secondEmpty = rewriter.createOperation(
      scope.id, region, extraBlock, "tensor.empty", {scope.resultTypes[1]});
  rewriter.appendToBlock(extraBlock, secondEmpty);
  const std::vector<int> yielded = {
      module.operations.at(static_cast<size_t>(firstEmpty)).results.front(),
      module.operations.at(static_cast<size_t>(secondEmpty)).results.front()};
  const int terminator = rewriter.createOperation(
      scope.id, region, extraBlock, "scope.return", {},
      yielded, scope.resultTypes);
  rewriter.appendToBlock(extraBlock, terminator);
  cvub::ValidateGenericModule(module);
  const std::string before = cvub::SerializeGenericModule(module);
  const auto result = cvub::RunInlineScope(module);
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Incomplete);
  CVUB_CHECK(HasDiagnostic(result, "InlineScope", "single-block"));
  CVUB_CHECK_EQ(cvub::SerializeGenericModule(result.module), before);
}

CVUB_TEST(inline_scope_inlines_private_multi_result_function_and_nested_scope) {
  auto module = cvub::test::ParseFixture("scope_tensor_empty_review.mlir");
  // Remove calls which deliberately exercise the fail-closed paths below.
  for (auto &operation : module.operations)
    if (HasOperationCase(module, "public_call") &&
        (cvub::UnquoteIRString(cvub::IRDictionaryValue(operation.attributes,
                                                       "case")) ==
             "public_call" ||
         cvub::UnquoteIRString(cvub::IRDictionaryValue(operation.attributes,
                                                       "case")) ==
             "noinline_call"))
      cvub::GenericRewriter(module).removeFromBlock(operation.blockId,
                                                    operation.id);
  module = cvub::CompactGenericModule(std::move(module));
  const auto result = cvub::RunInlineScope(std::move(module));
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
  CVUB_CHECK(!HasOperationCase(result.module, "inline_call"));
  CVUB_CHECK(!HasOperationCase(result.module, "nested_callee_scope"));
  CVUB_CHECK(!HasOperation(result.module, "private_pair"));
  const auto &first = OperationWithCase(result.module, "nested_scope_first");
  const auto &user = OperationWithCase(result.module, "inline_user");
  CVUB_CHECK_EQ(user.operands[0], first.results.front());
  CVUB_CHECK_EQ(user.operands[1],
                OperationWithCase(result.module, "inline_input").results.front());
  cvub::ValidateGenericModule(result.module);
}

CVUB_TEST(inline_scope_fail_closes_unproven_call_but_honors_noinline) {
  const auto input = cvub::test::ParseFixture("scope_tensor_empty_review.mlir");
  const auto result = cvub::RunInlineScope(input);
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Incomplete);
  CVUB_CHECK(HasDiagnostic(result, "InlineScope", "private or internal"));
  CVUB_CHECK(HasOperationCase(result.module, "public_call"));
  CVUB_CHECK(HasOperationCase(result.module, "noinline_call"));
}

CVUB_TEST(inline_scope_fail_closes_recursive_private_call_graph) {
  auto module = cvub::test::ParseFixture("scope_tensor_empty_review.mlir");
  auto &recursive = MutableOperationNamed(module, "tensor.expand_shape");
  recursive.name = "func.call";
  recursive.properties = "{callee = @private_pair}";
  recursive.attributes = "{callee = @private_pair}";
  const auto result = cvub::RunInlineScope(std::move(module));
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Incomplete);
  CVUB_CHECK(HasDiagnostic(result, "InlineScope", "recursive"));
}

CVUB_TEST(fold_tensor_empty_replaces_empty_source_insert_slice_with_destination) {
  const auto result = cvub::RunFoldTensorEmpty(
      cvub::test::ParseFixture("scope_tensor_empty.mlir"));
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
  CVUB_CHECK(!HasOperationCase(result.module, "fold_insert_slice"));
  CVUB_CHECK(!HasOperationCase(result.module, "fold_source"));
  const auto &destination =
      OperationWithCase(result.module, "fold_destination");
  const auto &user = OperationWithCase(result.module, "fold_user");
  CVUB_CHECK_EQ(user.operands.front(), destination.results.front());
  cvub::ValidateGenericModule(result.module);
}

CVUB_TEST(fold_tensor_empty_handles_static_extract_and_reshape_to_fixedpoint) {
  auto module = cvub::ExtractFunctionModule(
      cvub::test::ParseFixture("minimal_aiv.mlir"), "minimal_aiv");
  const auto ret = OperationNamed(module, "func.return");
  cvub::GenericRewriter rewriter(module);
  const int empty = rewriter.createOperation(
      ret.parentId, ret.regionId, ret.blockId, "tensor.empty",
      {"tensor<4xf32>"}, {}, {}, "", "{case = \"fold_chain_empty\"}");
  rewriter.insertToBlock(ret.blockId, static_cast<size_t>(ret.ordinal), empty);
  const int extract = rewriter.createOperation(
      ret.parentId, ret.regionId, ret.blockId, "tensor.extract_slice",
      {"tensor<4xf32>"},
      {module.operations.at(static_cast<size_t>(empty)).results.front()},
      {"tensor<4xf32>"},
      "{static_offsets = array<i64: 0>, static_sizes = array<i64: 4>, "
      "static_strides = array<i64: 1>}", "{case = \"fold_chain_extract\"}");
  rewriter.insertToBlock(ret.blockId, static_cast<size_t>(ret.ordinal + 1),
                         extract);
  const int expand = rewriter.createOperation(
      ret.parentId, ret.regionId, ret.blockId, "tensor.expand_shape",
      {"tensor<1x4xf32>"},
      {module.operations.at(static_cast<size_t>(extract)).results.front()},
      {"tensor<4xf32>"}, "", "{case = \"fold_chain_expand\"}");
  rewriter.insertToBlock(ret.blockId, static_cast<size_t>(ret.ordinal + 2),
                         expand);
  cvub::ValidateGenericModule(module);
  const auto result = cvub::RunFoldTensorEmpty(std::move(module));
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
  CVUB_CHECK(!HasOperationCase(result.module, "fold_chain_extract"));
  CVUB_CHECK(!HasOperationCase(result.module, "fold_chain_expand"));
  CVUB_CHECK_EQ(OperationCount(result.module, "tensor.empty"), 1U);
}

CVUB_TEST(fold_tensor_empty_blocks_dynamic_or_version_patterns_not_modeled) {
  auto module = cvub::test::ParseFixture("scope_tensor_empty.mlir");
  auto &insert = MutableOperationNamed(module, "tensor.insert_slice");
  insert.name = "tensor.pack";
  const auto result = cvub::RunFoldTensorEmpty(std::move(module));
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Incomplete);
  CVUB_CHECK(HasDiagnostic(result, "FoldTensorEmpty", "tensor.pack"));
}

CVUB_TEST(post_split_canonicalization_relabels_blocker_diagnostics) {
  auto module = cvub::ExtractFunctionModule(
      cvub::test::ParseFixture("workspace_sync_invariant.mlir"),
      "workspace_sync");
  MutableOperationNamed(module, "hivm.hir.load").name = "scf.if";
  const auto result = cvub::RunPostSplitCanonicalization(std::move(module));
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Incomplete);
  CVUB_CHECK(HasDiagnostic(result, "CanonicalizationAfterSplit",
                           "unmodeled applicable"));
  CVUB_CHECK(!HasDiagnostic(result, "CanonicalizationBeforeSplit",
                            "unmodeled applicable"));
}

CVUB_TEST(clone_tensor_empty_gives_every_destination_owner_distinct_storage) {
  auto module = cvub::test::ParseFixture("scope_tensor_empty.mlir");
  // Exercise the semantic metadata path used by product parsing as well as
  // the raw generic fixture's operand-segment path.
  auto &metadataOwner = MutableOperationNamed(module, "hivm.hir.vadd", 1);
  cvub::ApplyOperationSemantics(metadataOwner);
  const auto result = cvub::RunCloneTensorEmpty(std::move(module));
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
  const auto &original =
      OperationWithCase(result.module, "shared_destination");
  std::vector<int> destinations;
  for (const std::string ownerCase : {"owner_zero", "owner_one"}) {
    const auto &owner = OperationWithCase(result.module, ownerCase);
    CVUB_CHECK_EQ(owner.operands.size(), 3U);
    destinations.push_back(owner.operands[2]);
    const auto &clone = DefinitionOf(result.module, owner.operands[2]);
    CVUB_CHECK_EQ(clone.name, "tensor.empty");
    CVUB_CHECK_EQ(clone.blockId, owner.blockId);
    CVUB_CHECK(clone.ordinal < owner.ordinal);
  }
  const auto &forOwner = OperationWithCase(result.module, "for_owner");
  const auto &whileOwner = OperationWithCase(result.module, "while_owner");
  const auto &insertOwner = OperationWithCase(result.module, "insert_owner");
  destinations.push_back(forOwner.operands[3]);
  destinations.push_back(whileOwner.operands[0]);
  destinations.push_back(insertOwner.operands[1]);
  CVUB_CHECK_EQ(std::set<int>(destinations.begin(), destinations.end()).size(),
                destinations.size());
  for (int destination : destinations) {
    CVUB_CHECK(destination != original.results.front());
    CVUB_CHECK_EQ(DefinitionOf(result.module, destination).name,
                  "tensor.empty");
  }
  CVUB_CHECK_EQ(OperationsWithCase(result.module, "size_mark").size(), 3U);
  CVUB_CHECK_EQ(OperationsWithCase(result.module, "other_mark").size(), 1U);
  CVUB_CHECK_EQ(OperationsWithCase(result.module, "scope_first").size(), 1U);
  CVUB_CHECK_EQ(OperationWithCase(result.module, "owner_zero").dpsInits,
                std::vector<int>{destinations.front()});
  cvub::ValidateGenericModule(result.module);
}

CVUB_TEST(clone_tensor_empty_replaces_all_owner_uses_once_and_erases_dead_empty) {
  auto module = cvub::test::ParseFixture("scope_tensor_empty.mlir");
  auto &owner = MutableOperationNamed(module, "hivm.hir.vadd", 1);
  const int oldValue = owner.operands.back();
  owner.operands[0] = oldValue;
  owner.operandTypes[0] = owner.operandTypes.back();
  owner.dpsInputs = {oldValue, oldValue};
  owner.dpsInits = {oldValue};
  // Remove all other users and marks so this source becomes dead after clone.
  for (auto &operation : module.operations)
    if (operation.id != owner.id && operation.id !=
            DefinitionOf(module, oldValue).id &&
        (std::find(operation.operands.begin(), operation.operands.end(),
                   oldValue) != operation.operands.end()))
      cvub::GenericRewriter(module).removeFromBlock(operation.blockId,
                                                    operation.id);
  module = cvub::CompactGenericModule(std::move(module));
  const auto result = cvub::RunCloneTensorEmpty(std::move(module));
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
  const auto &rewritten = OperationWithCase(result.module, "owner_zero");
  CVUB_CHECK_EQ(rewritten.operands[0], rewritten.operands[2]);
  CVUB_CHECK_EQ(rewritten.dpsInputs,
                std::vector<int>({rewritten.operands[0], rewritten.operands[0]}));
  CVUB_CHECK_EQ(rewritten.dpsInits,
                std::vector<int>({rewritten.operands[0]}));
  // The clone copies the case attribute; exactly one such operation proves
  // that the old source was erased instead of becoming a phantom allocation.
  CVUB_CHECK_EQ(OperationsWithCase(result.module, "shared_destination").size(),
                1U);
  const auto plan = cvub::PlanLocalMemoryForSeed(
      cvub::BuildSuffixPlanMemoryInput(result.module), 0);
  CVUB_CHECK(plan.success);
  CVUB_CHECK(plan.peakBits > 0U);
}

CVUB_TEST(clone_tensor_empty_uses_exact_structured_registry) {
  auto module = cvub::test::ParseFixture("scope_tensor_empty.mlir");
  MutableOperationNamed(module, "hivm.hir.vadd", 1).name =
      "hivm.hir.vector_but_not_registered";
  const auto result = cvub::RunCloneTensorEmpty(std::move(module));
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
  const auto &owner = OperationWithCase(result.module, "owner_zero");
  CVUB_CHECK_EQ(owner.operands.back(),
                OperationWithCase(result.module, "shared_destination")
                    .results.front());
}

CVUB_TEST(post_split_scope_empty_stages_are_ordered_and_propagated) {
  const auto result = cvub::RunPostCVPipelineAIVProjection(
      cvub::test::ParseFixture("minimal_aiv.mlir"), {});
  CVUB_CHECK_EQ(result.coverage[6].disposition,
                cvub::CoverageDisposition::Modeled);
  CVUB_CHECK_EQ(result.coverage[7].disposition,
                cvub::CoverageDisposition::Modeled);
  CVUB_CHECK_EQ(result.coverage[8].disposition,
                cvub::CoverageDisposition::Modeled);
  CVUB_CHECK_EQ(result.coverage[9].disposition,
                cvub::CoverageDisposition::Modeled);
  CVUB_CHECK_EQ(result.coverage[12].disposition,
                cvub::CoverageDisposition::Modeled);
  CVUB_CHECK(!result.functions.empty());
  cvub::ValidateGenericModule(result.functions.front().module);
}

CVUB_TEST(scope_tensor_empty_full_rewrite_matches_checked_golden) {
  auto inlineResult = cvub::RunInlineScope(
      cvub::test::ParseFixture("scope_tensor_empty.mlir"));
  auto foldResult =
      cvub::RunFoldTensorEmpty(std::move(inlineResult.module));
  auto canonicalResult =
      cvub::RunPostSplitCanonicalization(std::move(foldResult.module));
  auto cloneResult =
      cvub::RunCloneTensorEmpty(std::move(canonicalResult.module));
  CVUB_CHECK_EQ(inlineResult.precision, cvub::Precision::Exact);
  CVUB_CHECK_EQ(foldResult.precision, cvub::Precision::Exact);
  CVUB_CHECK_EQ(canonicalResult.precision, cvub::Precision::Incomplete);
  CVUB_CHECK(HasDiagnostic(canonicalResult, "CanonicalizationAfterSplit",
                           "unmodeled applicable"));
  CVUB_CHECK_EQ(cloneResult.precision, cvub::Precision::Exact);
  CVUB_CHECK_EQ(cvub::SerializeGenericModule(cloneResult.module),
                ReadFixtureText("scope_tensor_empty.golden.generic-ir"));
}

CVUB_TEST(tile_bind_subblock_success_form_is_reported_incomplete) {
  auto module = cvub::test::ParseFixture("subblock_bind_success.mlir");
  const std::string before = cvub::SerializeGenericModule(module);
  const auto result = cvub::RunTileAndBindSubBlock(module, /*enableTile=*/true);
  // The real pass would derive a tiling dimension for the static-shaped store
  // and wrap the body in a sub-block loop.  The lightweight model cannot
  // reproduce the bubble-up tiling exactly, so it must fail closed: keep the
  // legal IR untouched and report incomplete instead of a silent exact result.
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Incomplete);
  CVUB_CHECK(!result.diagnostics.empty());
  bool namedAivFunction = false;
  for (const cvub::PostCVPipelineDiagnostic &diagnostic : result.diagnostics)
    if (diagnostic.pipelineStage == "TileAndBindSubBlock" &&
        diagnostic.reason.find("sub-block") != std::string::npos)
      namedAivFunction = true;
  CVUB_CHECK(namedAivFunction);
  CVUB_CHECK_EQ(cvub::SerializeGenericModule(result.module), before);
  CVUB_CHECK_EQ(OperationCount(result.module, "scf.if"), 0U);
  CVUB_CHECK_EQ(OperationCount(result.module, "hivm.hir.get_sub_block_idx"), 0U);
  cvub::ValidateGenericModule(result.module);
}

CVUB_TEST(tile_bind_subblock_same_address_hazard_applies_store_limit) {
  auto module = cvub::test::ParseFixture("subblock_bind_fallback.mlir");
  const auto result = cvub::RunTileAndBindSubBlock(module, /*enableTile=*/true);
  // Both the load source and the store destination trace through
  // reinterpret_cast to the same block argument, so the real pass detects the
  // same-address hazard and falls back to limitUniqueSubBlockToStore.  The
  // model mirrors that recognized fallback exactly.
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
  CVUB_CHECK(result.diagnostics.empty());
  CVUB_CHECK_EQ(OperationCount(result.module, "hivm.hir.get_sub_block_idx"), 1U);
  const cvub::GenericOperation &limitIf = [&] {
    for (const cvub::GenericOperation &operation : result.module.operations)
      if (operation.name == "scf.if" &&
          HasUnitAttribute(operation.attributes, "limit_sub_block_id0"))
        return operation;
    throw std::runtime_error("missing limit_sub_block_id0 scf.if");
  }();
  const cvub::GenericOperation &store =
      OperationNamed(result.module, "hivm.hir.store");
  CVUB_CHECK_EQ(store.parentId, limitIf.id);
  // Exactly one store is restricted and it lives inside the guard block.
  CVUB_CHECK_EQ(OperationCount(result.module, "hivm.hir.store"), 1U);
  CVUB_CHECK_EQ(OperationCount(result.module, "scf.if"), 1U);
  // The condition plumbing mirrors the real pass.
  CVUB_CHECK_EQ(OperationCount(result.module, "arith.index_cast"), 1U);
  CVUB_CHECK_EQ(OperationCount(result.module, "arith.cmpi"), 1U);
  cvub::ValidateGenericModule(result.module);
}

CVUB_TEST(tile_bind_subblock_implicit_transpose_hazard_applies_store_limit) {
  auto module =
      cvub::test::ParseFixture("subblock_bind_implicit_transpose.mlir");
  const auto result = cvub::RunTileAndBindSubBlock(module, /*enableTile=*/true);
  // The annotation.mark carries MayImplicitTransposeWithLastAxis = false.  The
  // real pass's isAnnotatedBy reduces to hasAttr(key) (value-agnostic), so even
  // an explicit = false spelling is a hazard that forces a fallback to
  // limitUniqueSubBlockToStore.  The model mirrors that recognized fallback
  // exactly.
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
  CVUB_CHECK(result.diagnostics.empty());
  CVUB_CHECK_EQ(OperationCount(result.module, "hivm.hir.get_sub_block_idx"), 1U);
  const cvub::GenericOperation &limitIf = [&] {
    for (const cvub::GenericOperation &operation : result.module.operations)
      if (operation.name == "scf.if" &&
          HasUnitAttribute(operation.attributes, "limit_sub_block_id0"))
        return operation;
    throw std::runtime_error("missing limit_sub_block_id0 scf.if");
  }();
  const cvub::GenericOperation &store =
      OperationWithCase(result.module, "hazard_store");
  CVUB_CHECK_EQ(store.parentId, limitIf.id);
  cvub::ValidateGenericModule(result.module);
}

CVUB_TEST(tile_bind_subblock_disable_attribute_is_recognized_noop) {
  auto module = cvub::test::ParseFixture("subblock_bind_success.mlir");
  module.operations.front().attributes = cvub::SetDictionaryValue(
      module.operations.front().attributes,
      "hivm.disable_auto_tile_and_bind_subblock", "");
  // A bare key with no value is the unit-attr spelling the real pass checks.
  module.operations.front().attributes =
      "{hivm.disable_auto_tile_and_bind_subblock}";
  const std::string before = cvub::SerializeGenericModule(module);
  const auto result = cvub::RunTileAndBindSubBlock(module, /*enableTile=*/true);
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
  CVUB_CHECK(result.diagnostics.empty());
  CVUB_CHECK_EQ(cvub::SerializeGenericModule(result.module), before);
}

CVUB_TEST(tile_bind_subblock_enable_tile_false_applies_store_limit) {
  auto module = cvub::test::ParseFixture("subblock_bind_success.mlir");
  const auto result = cvub::RunTileAndBindSubBlock(module, /*enableTile=*/false);
  // enableTile=false still applies limitUniqueSubBlockToStore in the real pass.
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
  CVUB_CHECK(result.diagnostics.empty());
  CVUB_CHECK_EQ(OperationCount(result.module, "scf.if"), 1U);
  const cvub::GenericOperation &store =
      OperationNamed(result.module, "hivm.hir.store");
  const cvub::GenericOperation &limitIf = [&] {
    for (const cvub::GenericOperation &operation : result.module.operations)
      if (operation.name == "scf.if" &&
          HasUnitAttribute(operation.attributes, "limit_sub_block_id0"))
        return operation;
    throw std::runtime_error("missing limit_sub_block_id0 scf.if");
  }();
  CVUB_CHECK_EQ(store.parentId, limitIf.id);
  cvub::ValidateGenericModule(result.module);
}

CVUB_TEST(tile_bind_subblock_function_without_store_is_recognized_noop) {
  auto module = cvub::test::ParseFixture("minimal_aiv.mlir");
  const std::string before = cvub::SerializeGenericModule(module);
  // With no store/copy/indirect_store, attemptBindSubBlock's isFailed check
  // stays true and the real pass reverts to limitUniqueSubBlockToStore (a
  // no-op).  The model mirrors that recognized rollback exactly.
  const auto result = cvub::RunTileAndBindSubBlock(module, /*enableTile=*/true);
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
  CVUB_CHECK(result.diagnostics.empty());
  CVUB_CHECK_EQ(cvub::SerializeGenericModule(result.module), before);
}

CVUB_TEST(tile_bind_subblock_dynamic_store_is_recognized_rollback) {
  auto module = cvub::test::ParseFixture("subblock_bind_success.mlir");
  // Drive the whole store-source chain to a dynamic shape consistently so the
  // module stays valid.  A dynamic-shaped store source makes tileAndSliceOp
  // abort in the real pass, which then reverts to limitUniqueSubBlockToStore.
  for (cvub::GenericOperation &operation : module.operations) {
    for (std::string &type : operation.resultTypes)
      if (type == "tensor<16x16xf16>")
        type = "tensor<?x16xf16>";
    for (std::string &type : operation.operandTypes)
      if (type == "tensor<16x16xf16>")
        type = "tensor<?x16xf16>";
  }
  cvub::ValidateGenericModule(module);
  const auto result = cvub::RunTileAndBindSubBlock(module, /*enableTile=*/true);
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
  CVUB_CHECK(result.diagnostics.empty());
  CVUB_CHECK_EQ(OperationCount(result.module, "scf.if"), 1U);
  cvub::ValidateGenericModule(result.module);
}

CVUB_TEST(tile_bind_subblock_slice_wrapped_dynamic_store_is_incomplete) {
  auto module = cvub::test::ParseFixture("subblock_bind_slice_wrapped.mlir");
  const std::string before = cvub::SerializeGenericModule(module);
  const auto result = cvub::RunTileAndBindSubBlock(module, /*enableTile=*/true);
  // The store source has a dynamic-looking direct type (tensor<?x16xf16>) but
  // is produced by a tensor.extract_slice of a static tensor.  The real pass
  // traces through the extract_slice to the underlying static type and proceeds
  // to tile instead of aborting.  The model must fail closed: it must NOT claim
  // the dynamic-store Exact rollback (which would wrongly wrap the store), so
  // the function is reported incomplete with the legal IR untouched.
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Incomplete);
  CVUB_CHECK(!result.diagnostics.empty());
  bool sliceDiagnostic = false;
  for (const cvub::PostCVPipelineDiagnostic &diagnostic : result.diagnostics)
    if (diagnostic.pipelineStage == "TileAndBindSubBlock" &&
        diagnostic.reason.find("slice") != std::string::npos)
      sliceDiagnostic = true;
  CVUB_CHECK(sliceDiagnostic);
  CVUB_CHECK_EQ(cvub::SerializeGenericModule(result.module), before);
  CVUB_CHECK_EQ(OperationCount(result.module, "scf.if"), 0U);
  CVUB_CHECK_EQ(OperationCount(result.module, "hivm.hir.get_sub_block_idx"), 0U);
  cvub::ValidateGenericModule(result.module);
}

CVUB_TEST(tile_bind_subblock_pipeline_marks_stage_modeled) {
  const auto result = cvub::RunPostCVPipelineAIVProjection(
      cvub::test::ParseFixture("minimal_aiv.mlir"), {});
  CVUB_CHECK_EQ(result.coverage[7].stage, "TileAndBindSubBlock");
  CVUB_CHECK_EQ(result.coverage[7].disposition,
                cvub::CoverageDisposition::Modeled);
  CVUB_CHECK(!HasDiagnostic(result, "TileAndBindSubBlock", "unsupported"));
  CVUB_CHECK(!result.functions.empty());
  cvub::ValidateGenericModule(result.functions.front().module);
}

CVUB_TEST(licm_hoists_invariant_pure_ops_and_keeps_variant_and_effectful) {
  auto module = cvub::test::ParseFixture("code_motion_lifetime.mlir");
  const auto result =
      cvub::RunLoopInvariantCodeMotion(module, /*enableCodeMotion=*/true);
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
  const int loopId = OperationWithCase(result.module, "lifetime_loop").id;
  const int funcId = FunctionId(result.module, "code_motion_lifetime_aiv");
  // The two invariant pure definitions move before the loop, sharing the
  // function entry block with (and preceding) the loop.  Their gen-kill
  // interval therefore starts before the loop instead of living across it.
  for (const std::string hoistedCase : {"hoist_target", "hoist_dependent"}) {
    const auto &hoisted = OperationWithCase(result.module, hoistedCase);
    CVUB_CHECK_EQ(hoisted.parentId, funcId);
    CVUB_CHECK_EQ(hoisted.blockId,
                  OperationWithCase(result.module, "lifetime_loop").blockId);
    CVUB_CHECK(hoisted.ordinal <
               OperationWithCase(result.module, "lifetime_loop").ordinal);
  }
  // hoist_dependent is only invariant once hoist_target has moved: confirm the
  // fixed-point iteration placed hoist_target before hoist_dependent.
  CVUB_CHECK(OperationWithCase(result.module, "hoist_target").ordinal <
             OperationWithCase(result.module, "hoist_dependent").ordinal);
  // The loop-variant arithmetic (operand is the induction variable) and the
  // side-effecting store remain inside the loop.
  CVUB_CHECK_EQ(OperationWithCase(result.module, "loop_variant").parentId,
                loopId);
  CVUB_CHECK_EQ(OperationWithCase(result.module, "stays_store").parentId,
                loopId);
  cvub::ValidateGenericModule(result.module);
}

CVUB_TEST(licm_enable_code_motion_false_is_recognized_noop) {
  auto module = cvub::test::ParseFixture("code_motion_lifetime.mlir");
  const std::string before = cvub::SerializeGenericModule(module);
  const auto result =
      cvub::RunLoopInvariantCodeMotion(module, /*enableCodeMotion=*/false);
  // enableCodeMotion=false gates both code-motion passes in the real compiler;
  // the model mirrors that as a recognized no-op (Exact, IR untouched).
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
  CVUB_CHECK(result.diagnostics.empty());
  CVUB_CHECK_EQ(cvub::SerializeGenericModule(result.module), before);
}

CVUB_TEST(licm_function_without_loop_is_recognized_noop) {
  auto module = cvub::test::ParseFixture("minimal_aiv.mlir");
  const std::string before = cvub::SerializeGenericModule(module);
  const auto result =
      cvub::RunLoopInvariantCodeMotion(module, /*enableCodeMotion=*/true);
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
  CVUB_CHECK(result.diagnostics.empty());
  CVUB_CHECK_EQ(cvub::SerializeGenericModule(result.module), before);
}

CVUB_TEST(licm_loop_invariant_memory_op_is_flagged_as_scope_gap) {
  // A loop-invariant hivm.hir.load may be hoisted by the real pass's advanced
  // memory analysis, which the lightweight model does not reproduce.  The model
  // must fail closed: leave the legal IR untouched and report incomplete rather
  // than silently claiming an exact (and possibly wrong-lifetime) result.
  auto module = cvub::test::ParseFixture("code_motion_lifetime.mlir");
  auto &store = MutableOperationNamed(module, "hivm.hir.store");
  // Turn the store into a load carrying a fresh result so the op remains a
  // loop-invariant (operands defined outside the loop once the pure vabs
  // producers hoist) inherent DMA, without colliding on an existing SSA value.
  cvub::GenericRewriter rewriter(module);
  const int loadResult = rewriter.newValue();
  store.name = "hivm.hir.load";
  store.resultTypes = {"tensor<16xf16>"};
  store.results = {loadResult};
  store.dpsInputs.clear();
  store.dpsInits.clear();
  store.effects.clear();
  cvub::ApplyOperationSemantics(store);
  cvub::ValidateGenericModule(module);
  const std::string before = cvub::SerializeGenericModule(module);
  const auto result =
      cvub::RunLoopInvariantCodeMotion(module, /*enableCodeMotion=*/true);
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Incomplete);
  CVUB_CHECK(HasDiagnostic(result, "LoopInvariantCodeMotion", "loop-invariant"));
  CVUB_CHECK_EQ(cvub::SerializeGenericModule(result.module), before);
}

CVUB_TEST(subset_hoisting_loop_without_iter_args_is_recognized_noop) {
  // The lifetime fixture's scf.for carries only the induction variable (no loop
  // iter args), so upstream subset hoisting has no extraction/insertion pair to
  // rewrite and the pass is a recognized no-op.  The model mirrors that exactly.
  auto module = cvub::test::ParseFixture("code_motion_lifetime.mlir");
  const std::string before = cvub::SerializeGenericModule(module);
  const auto result =
      cvub::RunLoopInvariantSubsetHoisting(module, /*enableCodeMotion=*/true);
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
  CVUB_CHECK(result.diagnostics.empty());
  CVUB_CHECK_EQ(cvub::SerializeGenericModule(result.module), before);
}

CVUB_TEST(subset_hoisting_loop_iter_arg_subset_pair_is_flagged_as_scope_gap) {
  // A loop carrying an iter arg through a tensor.extract_slice /
  // tensor.insert_slice pair is rewritten by the real pass (extraction before
  // the loop, insertion after, plus a new carried iter arg/result).  That loop
  // rebuild is not reproduced here, so the model fails closed and flags the
  // function as a scope candidate rather than silently dropping the hoist.
  auto module = cvub::test::ParseFixture("subset_hoisting_lifetime.mlir");
  const std::string before = cvub::SerializeGenericModule(module);
  const auto result =
      cvub::RunLoopInvariantSubsetHoisting(module, /*enableCodeMotion=*/true);
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Incomplete);
  CVUB_CHECK(HasDiagnostic(result, "LoopInvariantSubsetHoisting", "subset"));
  CVUB_CHECK_EQ(cvub::SerializeGenericModule(result.module), before);
}

CVUB_TEST(subset_hoisting_enable_code_motion_false_is_recognized_noop) {
  auto module = cvub::test::ParseFixture("subset_hoisting_lifetime.mlir");
  const std::string before = cvub::SerializeGenericModule(module);
  const auto result =
      cvub::RunLoopInvariantSubsetHoisting(module, /*enableCodeMotion=*/false);
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
  CVUB_CHECK(result.diagnostics.empty());
  CVUB_CHECK_EQ(cvub::SerializeGenericModule(result.module), before);
}

CVUB_TEST(otf_unaligned_concat_store_rebuilds_insert_slice_chain) {
  auto module = cvub::test::ParseFixture("unaligned_concat_store.mlir");
  auto unaligned =
      cvub::ExtractFunctionModule(module, "unaligned_concat_store_aiv");
  const auto result = cvub::RunInlineOTFLoadStore(std::move(unaligned));
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
  // The vconcat result is dead after the store source is repointed, so the
  // buffer_size_in_byte mark and the vconcat itself are removed by dead-op
  // cleanup (this is what prevents the dead concat destination from being
  // counted as a UB allocation).
  CVUB_CHECK(!HasOperationCase(result.module, "unaligned_concat"));
  CVUB_CHECK(!HasOperationCase(result.module, "unaligned_size_mark"));
  CVUB_CHECK_EQ(OperationCount(result.module, "tensor.insert_slice"), 2U);
  const auto &store = OperationWithCase(result.module, "unaligned_store");
  const auto &finalAccumulator = DefinitionOf(result.module, store.operands[0]);
  CVUB_CHECK_EQ(finalAccumulator.name, "tensor.insert_slice");
  // The accumulator is seeded from the vconcat destination and threads the two
  // inputs at their prefix-sum offsets along the (last) concat dimension.  The
  // first insert_slice places unaligned_a at [0, 0]; the second (the store
  // source) places unaligned_b at the running offset [0, 4] (unaligned_a's
  // extent along dim 1 == 4).  Asserting both pins the running-offset update.
  const auto &firstInsertSlice = result.module.operations.at(
      static_cast<size_t>(OperationId(result.module, "tensor.insert_slice")));
  CVUB_CHECK_EQ(
      cvub::IRDictionaryValue(firstInsertSlice.attributes, "static_offsets"),
      "[0, 0]");
  CVUB_CHECK_EQ(
      cvub::IRDictionaryValue(finalAccumulator.attributes, "static_offsets"),
      "[0, 4]");
  // The destination buffer is reused as the accumulator seed, not dropped.
  CVUB_CHECK(HasOperationCase(result.module, "unaligned_dst"));
  CVUB_CHECK(HasOperationCase(result.module, "unaligned_a"));
  CVUB_CHECK(HasOperationCase(result.module, "unaligned_b"));
  cvub::ValidateGenericModule(result.module);
}

CVUB_TEST(otf_aligned_concat_store_is_recognized_noop) {
  auto module = cvub::test::ParseFixture("unaligned_concat_store.mlir");
  auto aligned =
      cvub::ExtractFunctionModule(module, "aligned_concat_store_aiv");
  const std::string before = cvub::SerializeGenericModule(aligned);
  const auto result = cvub::RunInlineOTFLoadStore(std::move(aligned));
  // Every cumulative last-dim input extent in bytes (32, 64) is divisible by the
  // real pass's 32-byte block, so the pattern returns failure and leaves the IR
  // unchanged.  The model mirrors that recognized no-op exactly.
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
  CVUB_CHECK(result.diagnostics.empty());
  CVUB_CHECK_EQ(cvub::SerializeGenericModule(result.module), before);
  CVUB_CHECK(HasOperationCase(result.module, "aligned_concat"));
  CVUB_CHECK(HasOperationCase(result.module, "aligned_size_mark"));
  CVUB_CHECK_EQ(OperationCount(result.module, "tensor.insert_slice"), 0U);
}

CVUB_TEST(licm_loop_invariant_non_speculatable_divsi_fails_closed) {
  // A loop-invariant arith.divsi whose divisor is a (non-constant) function
  // argument is NotSpeculatable upstream, so real LICM leaves it in the loop.
  // The model has no integer-range analysis to prove speculatability, so it must
  // fail closed: report Incomplete and leave the legal IR untouched rather than
  // hoisting the op and claiming a wrong Exact.
  auto module = cvub::test::ParseFixture("code_motion_nonspeculatable.mlir");
  const std::string before = cvub::SerializeGenericModule(module);
  const auto result =
      cvub::RunLoopInvariantCodeMotion(module, /*enableCodeMotion=*/true);
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Incomplete);
  CVUB_CHECK(
      HasDiagnostic(result, "LoopInvariantCodeMotion", "speculatable"));
  CVUB_CHECK_EQ(cvub::SerializeGenericModule(result.module), before);
}

CVUB_TEST(otf_extra_users_concat_store_fails_closed_before_any_mutation) {
  // Two unaligned concat-stores: xu_store1 is a clean candidate the pattern
  // would rewrite, while xu_store2's vconcat feeds an extra user (xu_store2_extra).
  // The model must classify the extra-user store up front and fail closed BEFORE
  // the rewrite loop mutates xu_store1, leaving the whole module byte-for-byte
  // unchanged on Incomplete (no partial rewrite).
  auto module = cvub::test::ParseFixture("unaligned_concat_store.mlir");
  auto extra =
      cvub::ExtractFunctionModule(module, "extra_users_concat_store_aiv");
  const std::string before = cvub::SerializeGenericModule(extra);
  const auto result = cvub::RunInlineOTFLoadStore(std::move(extra));
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Incomplete);
  CVUB_CHECK(
      HasDiagnostic(result, "InlineOTFLoadStore",
                    "vconcat result has users beyond the store and size mark"));
  // No store was rewritten: xu_store1 is still a hivm.hir.store of the original
  // vconcat result, and no insert_slice accumulator was introduced anywhere.
  CVUB_CHECK(HasOperationCase(result.module, "xu_store1"));
  CVUB_CHECK(HasOperationCase(result.module, "xu_concat1"));
  CVUB_CHECK_EQ(OperationCount(result.module, "tensor.insert_slice"), 0U);
  CVUB_CHECK_EQ(cvub::SerializeGenericModule(result.module), before);
}

CVUB_TEST(otf_subbyte_concat_store_is_aligned_noop) {
  // A sub-byte element type (i4 -> elemSizeInBytes = 4/8 = 0) makes every
  // cumulative-extent alignment check (accumOffset * 0) % 32 == 0 trivially
  // true, so the real pattern treats the store as an aligned no-op (returns
  // failure, IR unchanged).  The model mirrors that exactly instead of
  // spuriously routing the zero byte width to Incomplete.
  auto module = cvub::test::ParseFixture("otf_subbyte_concat_store.mlir");
  const std::string before = cvub::SerializeGenericModule(module);
  const auto result = cvub::RunInlineOTFLoadStore(std::move(module));
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Exact);
  CVUB_CHECK(result.diagnostics.empty());
  CVUB_CHECK_EQ(cvub::SerializeGenericModule(result.module), before);
  CVUB_CHECK(HasOperationCase(result.module, "sb_concat"));
  CVUB_CHECK_EQ(OperationCount(result.module, "tensor.insert_slice"), 0U);
}

// ---------------------------------------------------------------------------
// Task 9: per-function suffix planning + module aggregation helpers and tests.
// ---------------------------------------------------------------------------

namespace {

// Writes `ir` to a temporary file under output/tests and parses it back as a
// generic module.  The generic IR parser only reads files, so test modules
// built from string must round-trip through disk.
cvub::GenericModule ParseBuiltIR(const std::string &name, const std::string &ir) {
  const cvub::fs::path path =
      cvub::fs::path("cvpipeline_ub_model_cpp/output/tests") / name;
  {
    std::ofstream output(path);
    if (!output)
      throw std::runtime_error("cannot write built IR: " + path.string());
    output << ir;
  } // close/flush before parsing so the reader sees the full file
  return cvub::ParseGenericIR(path, false);
}

// Builds an isolated AIV module with exactly one func.func holding a single UB
// allocation of `elements` x f32 kept live by a memref.store.  After the suffix
// pipeline + PlanMemory this yields a deterministic peak of elements*32 bits.
// elements == 0 produces a function with no allocation (an "empty" AIV module
// whose only function has zero UB footprint).
cvub::GenericModule BuildAIVModuleWithAlloc(const std::string &functionName,
                                             unsigned elements) {
  std::string body;
  if (elements > 0) {
    const std::string ty =
        "memref<" + std::to_string(elements) + "xf32, #hivm.address_space<UB>>";
    body =
        "    %c0 = \"arith.constant\"() <{value = 0 : f32}> : () -> f32\n"
        "    %idx = \"arith.constant\"() <{value = 0 : index}> : () -> index\n"
        "    %b = \"memref.alloc\"() : () -> " +
        ty +
        "\n"
        "    \"memref.store\"(%c0, %b, %idx) : (f32, " +
        ty + ", index) -> ()\n";
  }
  const std::string ir =
      "\"builtin.module\"() ({\n"
      "  \"func.func\"() <{function_type = () -> (), sym_name = \"" +
      functionName + "\"}> ({\n"
      "  ^bb0:\n" +
      body +
      "    \"func.return\"() : () -> ()\n"
      "  }) {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, "
      "hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = \"aiv\"} : "
      "() -> ()\n"
      "}) : () -> ()\n";
  return ParseBuiltIR("built_aiv_" + functionName + ".mlir", ir);
}

// peak bits -> f32 element count (32 bits per element), used so a test can name
// a desired post-planning peak directly.
unsigned ElementsForPeakBits(uint64_t peakBits) {
  return static_cast<unsigned>(peakBits / 32U);
}

cvub::ProjectedAIVModule MakeProjectedModule(const std::string &sourceFunction,
                                              const std::string &projectedFunction,
                                              unsigned elements) {
  return {sourceFunction, projectedFunction,
          BuildAIVModuleWithAlloc(projectedFunction, elements)};
}

// Builds a PostCVPipelineResult with two independently plannable AIV modules
// whose suffix+PlanMemory peaks equal `peakA` and `peakB`.
cvub::PostCVPipelineResult MakeTwoAIVModulesWithPeaks(uint64_t peakA,
                                                       uint64_t peakB) {
  cvub::PostCVPipelineResult result;
  result.functions.push_back(MakeProjectedModule(
      "source_a", "kernel_a", ElementsForPeakBits(peakA)));
  result.functions.push_back(MakeProjectedModule(
      "source_b", "kernel_b", ElementsForPeakBits(peakB)));
  return result;
}

// 196608 f32 = 6291456 bits, comfortably over the 192 KiB UB capacity, so the
// function overflows under every retry seed.
constexpr unsigned kOverflowElements = 196608U;

} // namespace

CVUB_TEST(module_peak_is_max_not_sum) {
  auto projected = MakeTwoAIVModulesWithPeaks(65536, 98304);
  auto result = cvub::PlanProjectedAIVFunctions(
      projected, {}, std::nullopt, false);
  CVUB_CHECK_EQ(result.functions.size(), 2U);
  CVUB_CHECK_EQ(result.peakBits, 98304U);
  CVUB_CHECK_EQ(result.requiredBits, 98304U);
  CVUB_CHECK(result.success);
  CVUB_CHECK(!result.overflow);
  CVUB_CHECK_EQ(result.capacityBits, cvub::kUBCapacityBits);
  CVUB_CHECK_EQ(result.functions[0].plan.peakBits, 65536U);
  CVUB_CHECK_EQ(result.functions[1].plan.peakBits, 98304U);
}

CVUB_TEST(module_aggregates_one_overflow_plus_one_success) {
  cvub::PostCVPipelineResult projected;
  projected.functions.push_back(
      MakeProjectedModule("overflow_src", "overflow_aiv", kOverflowElements));
  projected.functions.push_back(
      MakeProjectedModule("ok_src", "ok_aiv", ElementsForPeakBits(65536)));
  auto result = cvub::PlanProjectedAIVFunctions(
      projected, {}, std::nullopt, false);
  CVUB_CHECK_EQ(result.functions.size(), 2U);
  CVUB_CHECK(!result.success);
  CVUB_CHECK(result.overflow);
  CVUB_CHECK(result.functions[0].plan.overflow);
  CVUB_CHECK(!result.functions[0].plan.success);
  CVUB_CHECK(result.functions[1].plan.success);
  CVUB_CHECK(!result.functions[1].plan.overflow);
  // Overflowing effective requirement is plan.requiredBits; the successful
  // function's is max(peak, required).  The module takes the max of both, which
  // is dominated by the overflowing function's requiredBits.
  const uint64_t overflowRequired = result.functions[0].plan.requiredBits;
  CVUB_CHECK(overflowRequired > cvub::kUBCapacityBits);
  CVUB_CHECK_EQ(result.peakBits, overflowRequired);
  CVUB_CHECK_EQ(result.requiredBits, overflowRequired);
  CVUB_CHECK(result.peakBits > result.functions[1].plan.peakBits);
}

CVUB_TEST(module_preserves_different_selected_retry_seeds) {
  // Planned with no fixed seed: the successful function settles on seed 0
  // immediately, while the overflowing function exhausts all 20 retry seeds and
  // reports seed 19.  The module must preserve each function's own seed rather
  // than inventing a single module-level seed.
  cvub::PostCVPipelineResult projected;
  projected.functions.push_back(
      MakeProjectedModule("ok_src", "ok_aiv", ElementsForPeakBits(65536)));
  projected.functions.push_back(
      MakeProjectedModule("overflow_src", "overflow_aiv", kOverflowElements));
  auto result = cvub::PlanProjectedAIVFunctions(
      projected, {}, std::nullopt, false);
  CVUB_CHECK_EQ(result.functions.size(), 2U);
  CVUB_CHECK_EQ(result.functions[0].plan.selectedSeed, 0U);
  CVUB_CHECK_EQ(result.functions[1].plan.selectedSeed, 19U);
  CVUB_CHECK(result.functions[0].plan.selectedSeed !=
             result.functions[1].plan.selectedSeed);
}

CVUB_TEST(module_fixed_seed_propagates_to_every_function) {
  auto projected = MakeTwoAIVModulesWithPeaks(65536, 98304);
  auto result = cvub::PlanProjectedAIVFunctions(
      projected, {}, static_cast<uint32_t>(7), false);
  CVUB_CHECK_EQ(result.functions.size(), 2U);
  CVUB_CHECK_EQ(result.functions[0].plan.selectedSeed, 7U);
  CVUB_CHECK_EQ(result.functions[1].plan.selectedSeed, 7U);
  CVUB_CHECK(result.functions[0].plan.success);
  CVUB_CHECK(result.functions[1].plan.success);
  CVUB_CHECK_EQ(result.peakBits, 98304U);
}

CVUB_TEST(module_tolerates_duplicate_buffer_names_across_functions) {
  // Each isolated module independently names its single UB allocation %base_0;
  // planning every function in isolation must not collide on that name, and the
  // module peak is still the max (not a confused merge) of the two.
  auto projected = MakeTwoAIVModulesWithPeaks(65536, 98304);
  auto result = cvub::PlanProjectedAIVFunctions(
      projected, {}, std::nullopt, false);
  CVUB_CHECK_EQ(result.functions.size(), 2U);
  for (const cvub::FunctionPlanResult &function : result.functions) {
    CVUB_CHECK(!function.plan.buffers.empty());
    CVUB_CHECK_EQ(function.plan.buffers.front().name, "%base_0");
  }
  CVUB_CHECK_EQ(result.peakBits, 98304U);
}

CVUB_TEST(module_handles_empty_aiv_modules) {
  cvub::PostCVPipelineResult projected;
  projected.functions.push_back(MakeProjectedModule("empty_src", "empty_aiv", 0));
  projected.functions.push_back(
      MakeProjectedModule("ok_src", "ok_aiv", ElementsForPeakBits(65536)));
  auto result = cvub::PlanProjectedAIVFunctions(
      projected, {}, std::nullopt, false);
  CVUB_CHECK_EQ(result.functions.size(), 2U);
  CVUB_CHECK(result.functions[0].plan.buffers.empty());
  CVUB_CHECK_EQ(result.functions[0].plan.peakBits, 0U);
  CVUB_CHECK_EQ(result.functions[0].plan.requiredBits, 0U);
  CVUB_CHECK(result.functions[0].plan.success);
  CVUB_CHECK(result.functions[1].plan.success);
  CVUB_CHECK(result.success);
  CVUB_CHECK(!result.overflow);
  CVUB_CHECK_EQ(result.peakBits, 65536U);
  CVUB_CHECK_EQ(result.requiredBits, 65536U);
}

CVUB_TEST(planning_two_function_fixture_plans_each_independently) {
  auto module = cvub::test::ParseFixture("two_aiv_functions.mlir");
  cvub::PostCVPipelineResult projected;
  projected.functions.push_back(
      {std::string("small_src"), std::string("small_aiv"),
       cvub::ExtractFunctionModule(module, "small_aiv")});
  projected.functions.push_back(
      {std::string("large_src"), std::string("large_aiv"),
       cvub::ExtractFunctionModule(module, "large_aiv")});
  auto result = cvub::PlanProjectedAIVFunctions(
      projected, {}, std::nullopt, false);
  CVUB_CHECK_EQ(result.functions.size(), 2U);
  CVUB_CHECK_EQ(result.functions[0].plan.peakBits, 65536U);
  CVUB_CHECK_EQ(result.functions[1].plan.peakBits, 98304U);
  CVUB_CHECK_EQ(result.peakBits, 98304U);
  CVUB_CHECK_EQ(result.requiredBits, 98304U);
  CVUB_CHECK(result.success);
}

CVUB_TEST(planning_rejects_module_without_exactly_one_aiv_function) {
  // A module with two AIV functions violates the per-module invariant that
  // replaced the bridge's old multiple-AIV blocker.  Planning reports a
  // diagnostic and marks the result incomplete instead of silently merging.
  auto module = cvub::test::ParseFixture("two_aiv_functions.mlir");
  cvub::PostCVPipelineResult projected;
  projected.functions.push_back(
      {std::string("both_src"), std::string("both_aiv"), std::move(module)});
  auto result = cvub::PlanProjectedAIVFunctions(
      projected, {}, std::nullopt, false);
  CVUB_CHECK_EQ(result.precision, cvub::Precision::Incomplete);
  CVUB_CHECK(!result.success);
  CVUB_CHECK(HasDiagnostic(result, "PlanProjectedAIVFunctions",
                           "exactly one AIV function"));
}

CVUB_TEST(suffix_bridge_throws_on_multiple_aiv_functions) {
  // A module with two AIV functions fed DIRECTLY to the suffix bridge (not via
  // PlanProjectedAIVFunctions, which pre-isolates exactly one AIV per module)
  // must throw fail-closed.  This locks in the bridge's own guard so a legacy or
  // direct caller (fromSuffix CLI, bare-file PlanLocalMemory, tests, future code)
  // can never silently plan only the first function and under-report UB as the
  // module peak.  Removing the guard reverts to fail-open behavior.
  auto module = cvub::test::ParseFixture("two_aiv_functions.mlir");
  CVUB_CHECK_THROWS_CONTAINS(cvub::BuildSuffixPlanMemoryInput(module),
                             "multiple AIV functions are not supported");
}

} // namespace

int main() { return cvub::test::RunAllTests(); }
