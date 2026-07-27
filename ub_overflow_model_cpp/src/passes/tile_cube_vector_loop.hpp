#ifndef CVPIPELINE_UB_MODEL_CPP_PASSES_TILE_CUBE_VECTOR_LOOP_HPP
#define CVPIPELINE_UB_MODEL_CPP_PASSES_TILE_CUBE_VECTOR_LOOP_HPP

#include "../analysis/hivm_dimension_analyzer.hpp"
#include "canonicalization_hivm_pipeline.hpp"
#include "../ir/post_pipeline_ir_utils.hpp"
#include "../pipeline/modeling_result.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <regex>
#include <set>
#include <string>
#include <vector>

namespace cvub {
namespace tile_cube_vector_loop_detail {

enum class LoopKind { Vector, Cube };

struct ShapedType {
  std::string kind;
  std::vector<int64_t> shape;
  std::string tail;
};

struct TargetSpec {
  int64_t l0cSizeBits = 0;
  int64_t ubAlignBits = 0;
};

struct LoopPlan {
  struct YieldedTensor {
    int yieldedValue = -1;
    int producerId = -1;
    int originalInit = -1;
    size_t axis = 0;
    std::string type;
  };
  int loopId = -1;
  LoopKind kind = LoopKind::Vector;
  int64_t extent = 0;
  unsigned tripCount = 1;
  bool skip = false;
  std::map<int, size_t> valueAxes;
  std::set<int> producerClosure;
  std::set<int> preservedProducerClosure;
  int cubeMmadId = -1;
  size_t cubeRealDimensionOperand = 0;
  size_t cubeRealDimensionDpsInput = 0;
  int localDestinationSubview = -1;
  int localDestinationAlloc = -1;
  std::vector<int> vectorStoreAnchors;
  std::vector<YieldedTensor> yieldedTensors;
};

inline std::optional<ShapedType> ParseStaticShapedType(const std::string &type) {
  static const std::regex shaped(
      R"(^(tensor|memref)<([0-9]+(?:x[0-9]+)*)x(.+)>$)");
  std::smatch match;
  if (!std::regex_match(type, match, shaped))
    return std::nullopt;
  ShapedType result{match[1].str(), {}, match[3].str()};
  try {
    for (const std::string &dimension : splitTopLevel(match[2].str(), 'x'))
      result.shape.push_back(std::stoll(dimension));
  } catch (const std::exception &) {
    return std::nullopt;
  }
  return result;
}

inline std::string PrintShapedType(const ShapedType &type) {
  std::string result = type.kind + "<";
  for (int64_t dimension : type.shape)
    result += std::to_string(dimension) + "x";
  return result + type.tail + ">";
}

inline unsigned ElementBitWidth(const std::string &tail) {
  if (tail == "bf16" || startsWith(tail, "bf16,"))
    return 16;
  static const std::regex element(R"(^[fiu]([0-9]+)(?:,.*)?$)");
  std::smatch match;
  if (!std::regex_match(tail, match, element))
    return 0;
  try {
    const unsigned long width = std::stoul(match[1].str());
    return width <= std::numeric_limits<unsigned>::max()
               ? static_cast<unsigned>(width)
               : 0U;
  } catch (const std::exception &) {
    return 0;
  }
}

inline bool CheckedMultiply(int64_t &value, int64_t factor) {
  if (value < 0 || factor < 0 ||
      (factor != 0 && value > std::numeric_limits<int64_t>::max() / factor))
    return false;
  value *= factor;
  return true;
}

inline std::optional<int64_t> StaticSizeBits(const ShapedType &type) {
  int64_t bits = static_cast<int64_t>(ElementBitWidth(type.tail));
  if (bits == 0)
    return std::nullopt;
  for (int64_t dimension : type.shape)
    if (!CheckedMultiply(bits, dimension))
      return std::nullopt;
  return bits;
}

inline std::vector<int64_t> ParseIntegerArray(const std::string &text) {
  std::string payload = trim(text);
  if (startsWith(payload, "array<i64:") ||
      startsWith(payload, "array<i32:")) {
    const size_t colon = payload.find(':');
    if (colon == std::string::npos || payload.back() != '>')
      return {};
    payload = "[" + payload.substr(colon + 1,
                                    payload.size() - colon - 2) + "]";
  }
  if (payload.size() < 2 || payload.front() != '[' || payload.back() != ']')
    return {};
  std::vector<int64_t> result;
  try {
    for (const std::string &item :
         splitTopLevel(payload.substr(1, payload.size() - 2)))
      result.push_back(std::stoll(item));
  } catch (const std::exception &) {
    return {};
  }
  return result;
}

inline std::string PrintIntegerArray(const std::vector<int64_t> &values) {
  std::string result = "array<i64: ";
  for (size_t index = 0; index < values.size(); ++index) {
    if (index != 0)
      result += ", ";
    result += std::to_string(values[index]);
  }
  return result + ">";
}

inline std::optional<int64_t> ParseSpecEntry(const std::string &text,
                                             const std::string &name) {
  const std::regex entry("\\\"" + name +
                         "\\\"[[:space:]]*,[[:space:]]*([0-9]+)");
  std::smatch match;
  if (!std::regex_search(text, match, entry))
    return std::nullopt;
  try {
    return std::stoll(match[1].str());
  } catch (const std::exception &) {
    return std::nullopt;
  }
}

inline std::optional<TargetSpec> ReadTargetSpec(const GenericModule &module) {
  if (module.operations.empty())
    return std::nullopt;
  const std::string &attributes = module.operations.front().attributes;
  const auto l0c = ParseSpecEntry(attributes, "L0C_SIZE");
  const auto ubAlign = ParseSpecEntry(attributes, "UB_ALIGN_SIZE");
  if (!l0c || !ubAlign || *l0c <= 0 || *ubAlign <= 0)
    return std::nullopt;
  return TargetSpec{*l0c, *ubAlign};
}

inline std::optional<LoopKind> CoreLoopKind(const GenericOperation &operation) {
  const std::string core =
      IRDictionaryValue(operation.attributes, "hivm.loop_core_type");
  if (core == "#hivm.tcore_type<VECTOR>")
    return LoopKind::Vector;
  if (core == "#hivm.tcore_type<CUBE>")
    return LoopKind::Cube;
  return std::nullopt;
}

inline bool HasLoopCoreAttribute(const GenericOperation &operation) {
  return !IRDictionaryValue(operation.attributes, "hivm.loop_core_type")
              .empty();
}

inline std::vector<int> DirectChildren(const GenericModule &module,
                                       const GenericOperation &operation) {
  if (operation.regions.size() != 1)
    return {};
  const GenericRegion &region =
      module.regions.at(static_cast<size_t>(operation.regions.front()));
  if (region.blocks.size() != 1)
    return {};
  return module.blocks.at(static_cast<size_t>(region.blocks.front())).operations;
}

inline const GenericOperation *Definition(const GenericModule &module,
                                          int value) {
  for (const GenericOperation &operation : module.operations)
    if (std::find(operation.results.begin(), operation.results.end(), value) !=
        operation.results.end())
      return &operation;
  return nullptr;
}

inline std::optional<int64_t> ConstantIndexValue(
    const GenericModule &module, int value) {
  const GenericOperation *definition = Definition(module, value);
  if (definition == nullptr || definition->name != "arith.constant" ||
      definition->resultTypes != std::vector<std::string>{"index"})
    return std::nullopt;
  const std::string attribute =
      IRDictionaryValue(definition->attributes, "value");
  static const std::regex indexValue(
      R"(^([0-9]+)[[:space:]]*:[[:space:]]*index$)");
  std::smatch match;
  if (!std::regex_match(attribute, match, indexValue))
    return std::nullopt;
  try {
    return std::stoll(match[1].str());
  } catch (const std::exception &) {
    return std::nullopt;
  }
}

inline std::vector<const GenericOperation *>
Users(const GenericModule &module, int value) {
  std::vector<const GenericOperation *> users;
  for (const GenericOperation &operation : module.operations)
    if (std::find(operation.operands.begin(), operation.operands.end(), value) !=
        operation.operands.end())
      users.push_back(&operation);
  return users;
}

inline std::optional<std::string> TensorTypeForMemRef(
    const std::string &type) {
  if (!startsWith(type, "memref<") || type.size() < 9 || type.back() != '>')
    return std::nullopt;
  const std::vector<std::string> fields =
      splitTopLevel(type.substr(7, type.size() - 8));
  if (fields.empty())
    return std::nullopt;
  return "tensor<" + trim(fields.front()) + ">";
}

// Exact translation of TileCubeVectorLoop.cpp::LiftToTensor and
// CanonicalizeAllocToTensor. The real pass applies both patterns greedily to
// the whole module before collecting loop information.
inline GenericModule LiftMemRefLoadsInLoop(GenericModule module) {
  std::vector<int> loadIds;
  for (const GenericOperation &operation : module.operations)
    if (operation.name == "hivm.hir.load")
      loadIds.push_back(operation.id);

  GenericRewriter rewriter(module);
  for (int loadId : loadIds) {
    const GenericOperation loadSnapshot =
        module.operations.at(static_cast<size_t>(loadId));
    std::vector<size_t> initIndices;
    try {
      initIndices = DpsInitOperandIndices(
          loadSnapshot.name, loadSnapshot.operands.size(),
          loadSnapshot.properties);
    } catch (const std::exception &) {
      continue;
    }
    if (initIndices.size() != 1 || initIndices.front() >= loadSnapshot.operands.size())
      continue;
    const size_t destinationIndex = initIndices.front();
    if (destinationIndex >= loadSnapshot.operandTypes.size() ||
        !startsWith(loadSnapshot.operandTypes[destinationIndex], "memref<"))
      continue;

    const int destination = loadSnapshot.operands[destinationIndex];
    const std::vector<const GenericOperation *> destinationUsers =
        Users(module, destination);
    int toTensorId = -1;
    bool multipleToTensorUsers = false;
    for (const GenericOperation *user : destinationUsers) {
      if (user->id == loadId || user->name != "bufferization.to_tensor")
        continue;
      if (toTensorId >= 0) {
        multipleToTensorUsers = true;
        break;
      }
      toTensorId = user->id;
    }
    if (toTensorId < 0 || multipleToTensorUsers)
      continue;
    const GenericOperation toTensorSnapshot =
        module.operations.at(static_cast<size_t>(toTensorId));
    if (toTensorSnapshot.results.size() != 1 ||
        toTensorSnapshot.resultTypes.size() != 1 || loadSnapshot.blockId < 0 ||
        toTensorSnapshot.blockId < 0)
      continue;

    std::vector<int> operands = loadSnapshot.operands;
    std::vector<std::string> operandTypes = loadSnapshot.operandTypes;
    if (!operands.empty() && !operandTypes.empty() &&
        startsWith(operandTypes.front(), "memref<")) {
      const std::optional<std::string> tensorType =
          TensorTypeForMemRef(operandTypes.front());
      if (!tensorType)
        continue;
      const int sourceToTensor = rewriter.createOperation(
          loadSnapshot.parentId, loadSnapshot.regionId, loadSnapshot.blockId,
          "bufferization.to_tensor", {*tensorType}, {operands.front()},
          {operandTypes.front()}, "restrict, writable");
      const GenericBlock &loadBlock =
          module.blocks.at(static_cast<size_t>(loadSnapshot.blockId));
      const auto loadPosition = std::find(loadBlock.operations.begin(),
                                          loadBlock.operations.end(), loadId);
      if (loadPosition == loadBlock.operations.end())
        throw std::runtime_error(
            "TileCubeVectorLoop: attached load is absent from its block");
      rewriter.insertToBlock(
          loadSnapshot.blockId,
          static_cast<size_t>(std::distance(loadBlock.operations.begin(),
                                            loadPosition)),
          sourceToTensor);
      operands.front() = module.operations.at(
          static_cast<size_t>(sourceToTensor)).results.front();
      operandTypes.front() = *tensorType;
    }
    operands[destinationIndex] = toTensorSnapshot.results.front();
    operandTypes[destinationIndex] = toTensorSnapshot.resultTypes.front();

    const int tensorLoad = rewriter.createOperation(
        loadSnapshot.parentId, loadSnapshot.regionId, loadSnapshot.blockId,
        loadSnapshot.name, {toTensorSnapshot.resultTypes.front()}, operands,
        operandTypes, loadSnapshot.properties, loadSnapshot.attributes);
    const GenericBlock &destinationBlock =
        module.blocks.at(static_cast<size_t>(toTensorSnapshot.blockId));
    const auto toTensorPosition =
        std::find(destinationBlock.operations.begin(),
                  destinationBlock.operations.end(), toTensorId);
    if (toTensorPosition == destinationBlock.operations.end())
      throw std::runtime_error(
          "TileCubeVectorLoop: attached to_tensor is absent from its block");
    rewriter.insertToBlock(
        toTensorSnapshot.blockId,
        static_cast<size_t>(std::distance(destinationBlock.operations.begin(),
                                          toTensorPosition)) +
            1,
        tensorLoad);

    const int oldTensor = toTensorSnapshot.results.front();
    const int newTensor =
        module.operations.at(static_cast<size_t>(tensorLoad)).results.front();
    for (GenericOperation &operation : module.operations) {
      if (operation.id == tensorLoad)
        continue;
      for (size_t operand = 0; operand < operation.operands.size(); ++operand)
        if (operation.operands[operand] == oldTensor)
          rewriter.replaceOperand(operation.id, operand, newTensor);
      for (int &value : operation.dpsInputs)
        if (value == oldTensor)
          value = newTensor;
      for (int &value : operation.dpsInits)
        if (value == oldTensor)
          value = newTensor;
    }
    rewriter.removeFromBlock(loadSnapshot.blockId, loadId);
  }
  rewriter.applyDirtyOperationSemantics();
  module = CompactGenericModule(std::move(module));

  // This is the second pattern in the same real greedy rewrite set. It is
  // intentionally evaluated after the lifted loads have released their
  // destination memrefs, which is the fixed point reached by MLIR's greedy
  // driver.
  std::vector<int> allocationIds;
  for (const GenericOperation &operation : module.operations)
    if (operation.name == "memref.alloc" && operation.results.size() == 1 &&
        operation.blockId >= 0)
      allocationIds.push_back(operation.id);
  GenericRewriter canonicalizer(module);
  for (int allocationId : allocationIds) {
    const GenericOperation allocation =
        module.operations.at(static_cast<size_t>(allocationId));
    const std::vector<const GenericOperation *> allocationUsers =
        Users(module, allocation.results.front());
    if (allocationUsers.size() != 1 ||
        allocationUsers.front()->name != "bufferization.to_tensor")
      continue;
    const int toTensorId = allocationUsers.front()->id;
    GenericOperation &toTensor = canonicalizer.modifyOperation(toTensorId);
    toTensor.name = "tensor.empty";
    toTensor.operands.clear();
    toTensor.operandTypes.clear();
    toTensor.properties.clear();
    toTensor.attributes.clear();
    toTensor.effects.clear();
    toTensor.dpsInputs.clear();
    toTensor.dpsInits.clear();
    canonicalizer.removeFromBlock(allocation.blockId, allocationId);
  }
  canonicalizer.applyDirtyOperationSemantics();
  module = CompactGenericModule(std::move(module));
  ValidateGenericModule(module);
  return module;
}

inline void SetTileOperationDictionaryValue(GenericOperation &operation,
                                            const std::string &name,
                                            const std::string &value) {
  if (!FindDictionaryValue(operation.properties, name).empty())
    operation.properties =
        SetDictionaryValue(operation.properties, name, value);
  else
    operation.attributes =
        SetDictionaryValue(operation.attributes, name, value);
}

inline std::string PrintTileI32Array(const std::vector<size_t> &values) {
  std::string result = "array<i32: ";
  for (size_t index = 0; index < values.size(); ++index) {
    if (index != 0)
      result += ", ";
    result += std::to_string(values[index]);
  }
  return result + ">";
}

// applyPatternsGreedily uses an OperationFolder while running LiftToTensor and
// the transform cleanup patterns.  In particular it folds constant dynamic
// slice operands into the static offset/size/stride arrays.  This is observable
// in the real TileCubeVectorLoop output and also removes the otherwise-local
// index constants before bufferization.
inline void FoldTileConstantSliceOperands(GenericModule &module) {
  constexpr int64_t kDynamic = std::numeric_limits<int64_t>::min();
  for (GenericOperation &operation : module.operations) {
    if (operation.name != "tensor.extract_slice" &&
        operation.name != "memref.subview")
      continue;
    std::vector<size_t> segments = OperandSegmentSizes(operation.properties);
    if (segments.empty())
      segments = OperandSegmentSizes(operation.attributes);
    if (segments.size() != 4 || operation.operands.size() < segments.front())
      continue;

    std::array<std::vector<int64_t>, 3> staticValues = {
        ParseIntegerArray(IRDictionaryValue(operation.properties,
                                            "static_offsets")
                              .empty()
                          ? IRDictionaryValue(operation.attributes,
                                              "static_offsets")
                          : IRDictionaryValue(operation.properties,
                                              "static_offsets")),
        ParseIntegerArray(IRDictionaryValue(operation.properties,
                                            "static_sizes")
                              .empty()
                          ? IRDictionaryValue(operation.attributes,
                                              "static_sizes")
                          : IRDictionaryValue(operation.properties,
                                              "static_sizes")),
        ParseIntegerArray(IRDictionaryValue(operation.properties,
                                            "static_strides")
                              .empty()
                          ? IRDictionaryValue(operation.attributes,
                                              "static_strides")
                          : IRDictionaryValue(operation.properties,
                                              "static_strides"))};
    const std::array<std::string, 3> names = {
        "static_offsets", "static_sizes", "static_strides"};
    std::vector<size_t> eraseOperands;
    size_t operandBegin = segments.front();
    for (size_t segment = 1; segment < segments.size(); ++segment) {
      std::vector<size_t> dynamicPositions;
      for (size_t position = 0; position < staticValues[segment - 1].size();
           ++position)
        if (staticValues[segment - 1][position] == kDynamic)
          dynamicPositions.push_back(position);
      size_t folded = 0;
      for (size_t ordinal = 0;
           ordinal < segments[segment] && ordinal < dynamicPositions.size();
           ++ordinal) {
        const size_t operandIndex = operandBegin + ordinal;
        if (operandIndex >= operation.operands.size() ||
            operandIndex >= operation.operandTypes.size() ||
            operation.operandTypes[operandIndex] != "index")
          continue;
        const GenericOperation *definition =
            Definition(module, operation.operands[operandIndex]);
        if (definition == nullptr)
          continue;
        const std::optional<ArithIntegerConstant> constant =
            ParseArithIntegerConstant(*definition);
        if (!constant)
          continue;
        staticValues[segment - 1][dynamicPositions[ordinal]] =
            SignedArithInteger(*constant);
        eraseOperands.push_back(operandIndex);
        ++folded;
      }
      segments[segment] -= folded;
      operandBegin += segments[segment] + folded;
    }
    std::sort(eraseOperands.rbegin(), eraseOperands.rend());
    for (size_t operandIndex : eraseOperands) {
      operation.operands.erase(operation.operands.begin() +
                               static_cast<std::ptrdiff_t>(operandIndex));
      operation.operandTypes.erase(
          operation.operandTypes.begin() +
          static_cast<std::ptrdiff_t>(operandIndex));
    }
    if (eraseOperands.empty())
      continue;
    for (size_t index = 0; index < names.size(); ++index)
      SetTileOperationDictionaryValue(
          operation, names[index], PrintIntegerArray(staticValues[index]));
    SetTileOperationDictionaryValue(operation, "operandSegmentSizes",
                                    PrintTileI32Array(segments));
  }
}

inline std::string TileStripDynamicOffset(std::string type) {
  const std::string marker = ", offset: ?";
  const size_t offset = type.find(marker);
  if (offset != std::string::npos)
    type.erase(offset, marker.size());
  return type;
}

inline std::string TileAddDynamicOffset(std::string type) {
  if (type.find("offset: ?") != std::string::npos)
    return type;
  const size_t layoutEnd = type.rfind("]>>");
  if (layoutEnd != std::string::npos)
    type.insert(layoutEnd + 1, ", offset: ?");
  return type;
}

inline void PropagateTileValueType(GenericModule &module, int value,
                                   const std::string &type) {
  for (GenericOperation &operation : module.operations)
    for (size_t index = 0; index < operation.operands.size(); ++index)
      if (operation.operands[index] == value &&
          index < operation.operandTypes.size())
        operation.operandTypes[index] = type;
}

inline void InferTileStaticSubviewOffsets(GenericModule &module) {
  for (GenericOperation &operation : module.operations) {
    if ((operation.name != "memref.subview" &&
         operation.name != "memref.collapse_shape") ||
        operation.operands.empty() || operation.operandTypes.empty() ||
        operation.results.size() != 1 || operation.resultTypes.size() != 1)
      continue;
    bool staticOffset =
        operation.operandTypes.front().find("offset: ?") == std::string::npos;
    if (operation.name == "memref.subview") {
      std::string offsets =
          IRDictionaryValue(operation.properties, "static_offsets");
      if (offsets.empty())
        offsets = IRDictionaryValue(operation.attributes, "static_offsets");
      const std::vector<int64_t> values = ParseIntegerArray(offsets);
      staticOffset = staticOffset && !values.empty() &&
                     std::none_of(values.begin(), values.end(), [](int64_t value) {
                       return value == std::numeric_limits<int64_t>::min();
                     });
    }
    if (!staticOffset ||
        operation.resultTypes.front().find("offset: ?") == std::string::npos)
      continue;
    operation.resultTypes.front() =
        TileStripDynamicOffset(operation.resultTypes.front());
    PropagateTileValueType(module, operation.results.front(),
                           operation.resultTypes.front());
  }
}

// applyCleanUpPatterns canonicalizes the affine.apply created by
// CVPipelining after static loop bounds become visible.  Compose its constant
// upper bound and step into the map, leaving only the IV operand, exactly as
// makeComposedAffineApply does in the real TileCubeVectorLoop transform.
inline void ComposeTileCVPipelineTripCountConstants(GenericModule &module) {
  const std::map<int, const GenericOperation *> definitions =
      DefiningOperations(module);
  for (GenericOperation &operation : module.operations) {
    if (operation.name != "affine.apply" || operation.operands.size() != 3 ||
        operation.operandTypes !=
            std::vector<std::string>{"index", "index", "index"})
      continue;
    std::string map = FindDictionaryValue(operation.properties, "map");
    if (map.empty())
      map = FindDictionaryValue(operation.attributes, "map");
    std::string compact;
    for (char character : map)
      if (!std::isspace(static_cast<unsigned char>(character)))
        compact.push_back(character);
    if (compact !=
        "affine_map<(d0,d1)[s0]->((d0-d1)ceildivs0)>")
      continue;

    const auto upperDefinition = definitions.find(operation.operands[0]);
    const auto stepDefinition = definitions.find(operation.operands[2]);
    if (upperDefinition == definitions.end() ||
        stepDefinition == definitions.end())
      continue;
    const std::optional<ArithIntegerConstant> upper =
        ParseArithIntegerConstant(*upperDefinition->second);
    const std::optional<ArithIntegerConstant> step =
        ParseArithIntegerConstant(*stepDefinition->second);
    if (!upper || !step || SignedArithInteger(*upper) < 0 ||
        SignedArithInteger(*step) <= 0)
      continue;

    operation.operands = {operation.operands[1]};
    operation.operandTypes = {"index"};
    operation.properties =
        "{map = affine_map<(d0) -> ((-d0 + " +
        std::to_string(SignedArithInteger(*upper)) + ") ceildiv " +
        std::to_string(SignedArithInteger(*step)) + ")>}";
    operation.attributes = operation.properties;
  }
}

inline void RemoveTileTriviallyDeadOperations(GenericModule &module) {
  const std::set<std::string> removable = {
      "bufferization.to_tensor", "memref.alloc",
      "memref_ext.alloc_workspace", "tensor.empty"};
  while (true) {
    std::set<int> attached;
    for (const GenericBlock &block : module.blocks)
      attached.insert(block.operations.begin(), block.operations.end());
    std::set<int> usedValues;
    for (int operationId : attached) {
      const GenericOperation &operation =
          module.operations.at(static_cast<size_t>(operationId));
      usedValues.insert(operation.operands.begin(), operation.operands.end());
      usedValues.insert(operation.dpsInputs.begin(), operation.dpsInputs.end());
      usedValues.insert(operation.dpsInits.begin(), operation.dpsInits.end());
    }
    std::vector<int> dead;
    for (int operationId : attached) {
      const GenericOperation &operation =
          module.operations.at(static_cast<size_t>(operationId));
      if (operationId <= 0 || !operation.regions.empty() ||
          removable.count(operation.name) == 0 || operation.results.empty())
        continue;
      if (std::none_of(operation.results.begin(), operation.results.end(),
                       [&](int value) { return usedValues.count(value) != 0; }))
        dead.push_back(operationId);
    }
    if (dead.empty())
      break;
    for (int operationId : dead)
      EraseOperationTree(module, operationId);
  }
}

inline void CSETileTensorEmpty(GenericModule &module) {
  auto dominates = [&](const GenericOperation &definition,
                       const GenericOperation &use) {
    int nestedOperation = use.id;
    while (nestedOperation >= 0) {
      const GenericOperation &nested =
          module.operations.at(static_cast<size_t>(nestedOperation));
      if (nested.blockId == definition.blockId)
        return definition.ordinal < nested.ordinal;
      if (nested.blockId < 0)
        break;
      const GenericBlock &block =
          module.blocks.at(static_cast<size_t>(nested.blockId));
      nestedOperation =
          module.regions.at(static_cast<size_t>(block.regionId))
              .parentOperation;
    }
    return false;
  };
  GenericRewriter rewriter(module);
  std::map<std::string, std::vector<int>> available;
  for (const GenericBlock &blockSnapshot : module.blocks) {
    for (int operationId : blockSnapshot.operations) {
      const GenericOperation &operation =
          module.operations.at(static_cast<size_t>(operationId));
      if (operation.name != "tensor.empty" || !operation.operands.empty() ||
          operation.results.size() != 1 || operation.resultTypes.size() != 1)
        continue;
      int replacement = -1;
      for (int candidateId : available[operation.resultTypes.front()]) {
        const GenericOperation &candidate =
            module.operations.at(static_cast<size_t>(candidateId));
        if (dominates(candidate, operation)) {
          replacement = candidate.results.front();
          break;
        }
      }
      if (replacement < 0) {
        available[operation.resultTypes.front()].push_back(operation.id);
        continue;
      }
      ReplaceAllUses(module, operation.results.front(), replacement);
      rewriter.removeFromBlock(operation.blockId, operation.id);
    }
  }
}

inline void OrderLiftedLoadInitializers(GenericModule &module) {
  std::vector<std::pair<int, int>> moves;
  for (const GenericOperation &load : module.operations) {
    if (load.name != "hivm.hir.load" || load.operands.size() < 2)
      continue;
    const GenericOperation *source = Definition(module, load.operands.front());
    const GenericOperation *destination = Definition(module, load.operands[1]);
    if (source == nullptr || destination == nullptr ||
        source->name != "bufferization.to_tensor" ||
        destination->name != "tensor.empty" ||
        source->blockId != destination->blockId)
      continue;
    const GenericBlock &block =
        module.blocks.at(static_cast<size_t>(source->blockId));
    const auto sourcePosition =
        std::find(block.operations.begin(), block.operations.end(), source->id);
    const auto destinationPosition = std::find(
        block.operations.begin(), block.operations.end(), destination->id);
    if (sourcePosition != block.operations.end() &&
        destinationPosition != block.operations.end() &&
        sourcePosition < destinationPosition)
      moves.push_back({destination->id, source->id});
  }
  for (const auto &[operation, before] : moves)
    MoveOperationBefore(module, operation, before);
}

inline GenericModule CanonicalizeTileGreedyArtifacts(
    GenericModule module, bool preserveLoadOwnedDestinationTiles = false) {
  FoldTileConstantSliceOperands(module);
  InferTileStaticSubviewOffsets(module);
  ComposeTileCVPipelineTripCountConstants(module);
  CSETileTensorEmpty(module);
  RemoveTileTriviallyDeadOperations(module);
  module = CompactGenericModule(std::move(module));
  OrderLiftedLoadInitializers(module);
  for (const GenericOperation &operation : module.operations)
    if (operation.name == "func.func")
      RunGreedyOperationFolder(module, operation.id);
  PipelineAnalysisContext useLists(module, kGenericAnalysisUsers);
  std::set<int> loadOwnedDestinationTiles;
  if (preserveLoadOwnedDestinationTiles) {
    for (const GenericOperation &operation : module.operations) {
      if (operation.name != "hivm.hir.load")
        continue;
      for (int destination : operation.dpsInits) {
        const GenericOperation *definition = Definition(module, destination);
        if (definition != nullptr &&
            (definition->name == "tensor.extract_slice" ||
             definition->name == "memref.subview"))
          loadOwnedDestinationTiles.insert(definition->id);
      }
    }
  }
  RunCanonicalizationCommonSubexpressionElimination(
      module, useLists,
      preserveLoadOwnedDestinationTiles ? &loadOwnedDestinationTiles
                                        : nullptr);
  while (EliminateCanonicalizationDeadCode(module, useLists)) {
  }
  module = CompactGenericModule(std::move(module));
  ValidateGenericModule(module);
  return module;
}

inline bool IsAncestor(const GenericModule &module, int possibleAncestor,
                       int operationId) {
  int parent = module.operations.at(static_cast<size_t>(operationId)).parentId;
  while (parent >= 0) {
    if (parent == possibleAncestor)
      return true;
    parent = module.operations.at(static_cast<size_t>(parent)).parentId;
  }
  return false;
}

inline const GenericOperation *TraceVectorProducer(const GenericModule &module,
                                                   const GenericOperation &loop,
                                                   int value) {
  std::set<int> visited;
  std::vector<int> worklist{value};
  while (!worklist.empty()) {
    const GenericOperation *definition = Definition(module, worklist.back());
    worklist.pop_back();
    if (definition == nullptr || !IsAncestor(module, loop.id, definition->id) ||
        !visited.insert(definition->id).second)
      continue;
    if (definition->name.rfind("hivm.hir.", 0) == 0 &&
        !definition->results.empty())
      return definition;
    for (size_t index = 0; index < definition->operands.size(); ++index)
      if (ParseStaticShapedType(definition->operandTypes[index]))
        worklist.push_back(definition->operands[index]);
  }
  return nullptr;
}

inline std::optional<unsigned> ParseCubeOverride(const GenericModule &module,
                                                 const GenericOperation &loop,
                                                 std::string &reason) {
  std::optional<unsigned> result;
  static const std::regex integer(R"(^([0-9]+)(?:[[:space:]]*:[[:space:]]*i[0-9]+)?$)");
  for (int child : DirectChildren(module, loop)) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(child));
    if (operation.name != "hivm.hir.mmadL1")
      continue;
    const std::string value =
        IRDictionaryValue(operation.attributes, "hivm.tile_mix_cube_num");
    if (value.empty())
      continue;
    std::smatch match;
    if (!std::regex_match(value, match, integer)) {
      reason = "tile_mix_cube_num is not a positive integer";
      return 0U;
    }
    unsigned long parsed = 0;
    try {
      parsed = std::stoul(match[1].str());
    } catch (const std::exception &) {
      reason = "tile_mix_cube_num overflows";
      return 0U;
    }
    if (parsed == 0 || parsed > std::numeric_limits<unsigned>::max()) {
      reason = "tile_mix_cube_num is outside the supported range";
      return 0U;
    }
    const unsigned current = static_cast<unsigned>(parsed);
    if (result && *result != current) {
      reason = "conflicting tile_mix_cube_num values";
      return 0U;
    }
    result = current;
  }
  return result;
}

inline bool IsAllowedBodyOperation(const GenericOperation &operation,
                                   LoopKind kind) {
  static const std::set<std::string> common = {
      "tensor.empty", "tensor.extract_slice", "memref.subview",
      "scf.yield"};
  if (!operation.regions.empty() || common.count(operation.name) != 0)
    return operation.regions.empty();
  if (kind == LoopKind::Vector)
    return operation.name == "hivm.hir.load" ||
           operation.name == "hivm.hir.vadd" ||
           operation.name == "hivm.hir.store";
  return operation.name == "hivm.hir.mmadL1" ||
         operation.name == "hivm.hir.load" ||
         operation.name == "hivm.hir.fixpipe";
}

inline bool IsSupportedProducer(const GenericOperation &operation,
                                LoopKind kind) {
  if (!operation.regions.empty())
    return false;
  if (operation.name == "tensor.empty" ||
      operation.name == "tensor.extract_slice" ||
      operation.name == "memref.subview" || operation.name == "hivm.hir.load")
    return true;
  if (kind == LoopKind::Vector)
    return operation.name.rfind("hivm.hir.", 0) == 0 ||
           operation.name == "tensor.expand_shape" ||
           operation.name == "tensor.collapse_shape";
  return operation.name == "hivm.hir.mmadL1";
}

inline bool CollectProducerClosure(const GenericModule &module,
                                   const GenericOperation &loop, int value,
                                   LoopKind kind, std::set<int> &closure,
                                   std::string &reason) {
  const GenericOperation *definition = Definition(module, value);
  if (definition == nullptr || !IsAncestor(module, loop.id, definition->id))
    return true;
  if (definition->name == "bufferization.to_tensor" ||
      definition->name == "tensor.empty" ||
      definition->name == "memref.collapse_shape" ||
      definition->name == "memref.reinterpret_cast" ||
      definition->name == "tensor.extract_slice" ||
      definition->name == "memref.subview")
    return true;
  if (!IsSupportedProducer(*definition, kind)) {
    reason = "anchor dependency contains an operation whose axis semantics "
             "are not modeled: " + definition->name;
    return false;
  }
  if (!closure.insert(definition->id).second)
    return true;
  for (size_t index = 0; index < definition->operands.size(); ++index) {
    // DimensionAnalyzerBase::processPreOrderWalk only starts dimension
    // propagation from allowed shaped values.  Scalar operands (for example
    // the i32 offset of an elementwise HIVM op) have rank zero and are not a
    // producer that fuse_into moves into the tiled loop.
    if (!ParseStaticShapedType(definition->operandTypes[index]))
      continue;
    if (!CollectProducerClosure(module, loop, definition->operands[index],
                                kind, closure, reason))
      return false;
  }
  return true;
}

inline std::optional<size_t> UniqueParallelAxis(const ShapedType &type) {
  // DimensionAnalyzer::getTilingDim orders parallel candidates by their
  // original dimension number and computeTilingDim selects the lower ordered
  // candidate when every Store in the value group supports it.  On the
  // non-regbased Ascend target, size-1 and odd dimensions are not candidates.
  // The old model incorrectly required there to be only one non-unit axis,
  // rejecting ordinary 64x128 vector stores for which the real analyzer picks
  // axis 0.
  for (size_t index = 0; index < type.shape.size(); ++index) {
    if (type.shape[index] <= 1 || type.shape[index] % 2 != 0)
      continue;
    return index;
  }
  return std::nullopt;
}

inline bool IsHIVMStructuredName(const std::string &name) {
  return name.rfind("hivm.hir.", 0) == 0;
}

inline std::vector<int> Descendants(const GenericModule &module,
                                    const GenericOperation &root) {
  std::vector<int> result;
  for (const GenericOperation &operation : module.operations)
    if (operation.id != root.id && IsAncestor(module, root.id, operation.id))
      result.push_back(operation.id);
  return result;
}

inline bool IsDirectChild(const GenericOperation &root,
                          const GenericOperation &operation) {
  return operation.parentId == root.id;
}

inline bool IsZeroUnitSubview(const GenericOperation &subview);
inline bool HasExactIdentitySemantics(const GenericOperation &operation,
                                      size_t rank, std::string &reason);

inline std::vector<const GenericOperation *> FindInLoopLoadDependencies(
    const GenericModule &module, const GenericOperation &loop, int value) {
  std::vector<const GenericOperation *> loads;
  std::set<int> visited;
  std::vector<int> worklist{value};
  while (!worklist.empty()) {
    const GenericOperation *definition = Definition(module, worklist.back());
    worklist.pop_back();
    if (definition == nullptr || !IsAncestor(module, loop.id, definition->id) ||
        !visited.insert(definition->id).second)
      continue;
    if (definition->name == "hivm.hir.load")
      loads.push_back(definition);
    worklist.insert(worklist.end(), definition->operands.begin(),
                    definition->operands.end());
  }
  return loads;
}

inline const GenericOperation *FindInLoopLoadDependency(
    const GenericModule &module, const GenericOperation &loop, int value) {
  const auto loads = FindInLoopLoadDependencies(module, loop, value);
  return loads.empty() ? nullptr : loads.front();
}

inline bool ProveLoadAxisEvidence(const GenericModule &module,
                                  const GenericOperation &loop, int value,
                                  size_t localAxis, int64_t groupExtent,
                                  std::string &reason) {
  const auto loads = FindInLoopLoadDependencies(module, loop, value);
  if (loads.empty())
    return false;
  for (const GenericOperation *load : loads) {
    if (load->results.size() != 1 || load->resultTypes.size() != 1) {
      reason = "in-loop load axis is not an identity mapping to the group axis";
      return false;
    }
    const auto type = ParseStaticShapedType(load->resultTypes.front());
    if (!type || !HasExactIdentitySemantics(*load, type->shape.size(), reason))
      return false;
    if (localAxis >= type->shape.size() ||
        type->shape[localAxis] != groupExtent) {
      reason = "in-loop load axis does not match the selected group axis";
      return false;
    }
  }
  return true;
}

inline bool ClassifyDestination(const GenericModule &module,
                                const GenericOperation &anchor,
                                LoopPlan &plan, std::string &reason) {
  if (anchor.operands.size() < 2 || anchor.operandTypes.size() < 2) {
    reason = "tiling anchor has no destination";
    return false;
  }
  const int destination = anchor.operands[1];
  const GenericOperation *definition = Definition(module, destination);
  if (definition == nullptr)
    return true; // A block argument is a GM boundary in the supported form.
  if (definition->name == "memref.alloc" && definition->results.size() == 1 &&
      definition->resultTypes.size() == 1 && definition->operands.empty() &&
      definition->resultTypes.front().find("address_space<gm>") ==
          std::string::npos) {
    const auto users = Users(module, definition->results.front());
    if (users.size() != 1 || users.front()->id != anchor.id) {
      reason = "local destination alloc is not exclusively reusable";
      return false;
    }
    const auto axis = plan.valueAxes.find(destination);
    if (axis == plan.valueAxes.end()) {
      reason = "local destination has no proven axis";
      return false;
    }
    plan.localDestinationAlloc = definition->id;
    plan.producerClosure.insert(definition->id);
    return true;
  }
  if (definition->name != "memref.subview" || definition->operands.empty()) {
    if (definition->name == "memref.collapse_shape" ||
        definition->name == "memref.reinterpret_cast")
      return true;
    reason = "destination scope cannot be classified as local or GM boundary";
    return false;
  }
  const auto destinationType =
      ParseStaticShapedType(anchor.operandTypes[1]);
  if (!destinationType ||
      !HasExactIdentitySemantics(*definition, destinationType->shape.size(),
                                 reason))
    return false;
  const GenericOperation *base = Definition(module, definition->operands[0]);
  if (base == nullptr)
    return true; // A subview of a block argument is a GM boundary.
  if (base->name != "memref.alloc" || base->results.size() != 1 ||
      base->resultTypes.size() != 1 || !base->operands.empty() ||
      base->resultTypes.front().find("address_space<gm>") != std::string::npos ||
      !IsZeroUnitSubview(*definition)) {
    reason = "destination scope cannot be classified as a reusable local alloc";
    return false;
  }
  const auto users = Users(module, base->results.front());
  if (users.size() != 1 || users.front()->id != definition->id) {
    reason = "local destination alloc is not exclusively reusable";
    return false;
  }
  plan.localDestinationSubview = definition->id;
  plan.localDestinationAlloc = base->id;
  const auto axis = plan.valueAxes.find(destination);
  if (axis == plan.valueAxes.end()) {
    reason = "local destination has no proven axis";
    return false;
  }
  plan.valueAxes[base->results.front()] = axis->second;
  plan.producerClosure.insert(base->id);
  return true;
}

inline bool HasStaticBoundaryView(const GenericModule &module, int value) {
  const GenericOperation *definition = Definition(module, value);
  if (definition == nullptr || definition->name != "memref.subview")
    return true;
  const auto offsets = ParseIntegerArray(
      IRDictionaryValue(definition->attributes, "static_offsets"));
  const auto sizes = ParseIntegerArray(
      IRDictionaryValue(definition->attributes, "static_sizes"));
  const auto strides = ParseIntegerArray(
      IRDictionaryValue(definition->attributes, "static_strides"));
  return !offsets.empty() && offsets.size() == sizes.size() &&
         sizes.size() == strides.size() &&
         std::none_of(offsets.begin(), offsets.end(),
                      [](int64_t dimension) { return dimension < 0; }) &&
         std::none_of(sizes.begin(), sizes.end(),
                      [](int64_t dimension) { return dimension < 0; }) &&
         std::all_of(strides.begin(), strides.end(),
                     [](int64_t dimension) { return dimension == 1; });
}

inline bool HasUnitIterationDomain(const GenericModule &module,
                                   const GenericOperation &loop) {
  if (loop.operands.size() != 3)
    return false;
  const GenericOperation *lower = Definition(module, loop.operands[0]);
  const GenericOperation *upper = Definition(module, loop.operands[1]);
  const GenericOperation *step = Definition(module, loop.operands[2]);
  return lower != nullptr && upper != nullptr && step != nullptr &&
         lower->name == "arith.constant" && upper->name == "arith.constant" &&
         step->name == "arith.constant" &&
         IRDictionaryValue(lower->attributes, "value") == "0 : index" &&
         IRDictionaryValue(upper->attributes, "value") == "1 : index" &&
         IRDictionaryValue(step->attributes, "value") == "1 : index";
}

inline bool RecordValueAxis(const GenericModule &module, LoopPlan &plan,
                            int value, const std::string &typeText, size_t axis,
                            int64_t extent, std::string &reason,
                            bool allowRepeatedExtent = false) {
  const auto type = ParseStaticShapedType(typeText);
  if (type && (axis >= type->shape.size() || type->shape[axis] != extent)) {
    std::optional<size_t> remapped;
    for (size_t candidate = 0; candidate < type->shape.size(); ++candidate) {
      if (type->shape[candidate] != extent)
        continue;
      if (remapped) {
        remapped.reset();
        break;
      }
      remapped = candidate;
    }
    if (remapped)
      axis = *remapped;
  }
  if (!type || axis >= type->shape.size() || type->shape[axis] != extent) {
    reason = "producer dependency has no provable tiling dimension for value " +
             std::to_string(value) + " type " + typeText + " axis " +
             std::to_string(axis);
    return false;
  }
  for (size_t index = 0; index < type->shape.size(); ++index)
    if (!allowRepeatedExtent && index != axis &&
        type->shape[index] == extent) {
      reason = "producer dependency has repeated equal-size dimensions";
      return false;
    }
  const auto existing = plan.valueAxes.find(value);
  if (existing != plan.valueAxes.end() && existing->second != axis) {
    reason = "producer dependency maps one value to conflicting axes";
    return false;
  }
  plan.valueAxes[value] = axis;
  const GenericOperation *definition = Definition(module, value);
  if (definition != nullptr && definition->results.size() == 1 &&
      definition->resultTypes.size() == 1) {
    const auto defType = ParseStaticShapedType(definition->resultTypes.front());
    if (!defType || axis >= defType->shape.size() ||
        defType->shape[axis] != extent) {
      reason = "producer result type disagrees with its consumer axis";
      return false;
    }
    plan.valueAxes[definition->results.front()] = axis;
  }
  return true;
}

inline std::vector<std::string>
DictionaryKeys(const std::string &dictionary) {
  std::vector<std::string> keys;
  if (dictionary.size() < 2 || dictionary.front() != '{' ||
      dictionary.back() != '}')
    return keys;
  for (const std::string &entry :
       splitTopLevel(dictionary.substr(1, dictionary.size() - 2))) {
    const size_t equal = entry.find('=');
    const std::string key = trim(entry.substr(0, equal));
    if (!key.empty())
      keys.push_back(key);
  }
  return keys;
}

inline bool IsIdentityPermutation(const std::string &text, size_t rank) {
  const auto permutation = ParseIntegerArray(text);
  if (permutation.size() != rank)
    return false;
  for (size_t index = 0; index < rank; ++index)
    if (permutation[index] != static_cast<int64_t>(index))
      return false;
  return true;
}

inline bool HasExactIdentitySemantics(const GenericOperation &operation,
                                      size_t rank, std::string &reason) {
  std::set<std::string> allowed;
  if (operation.name == "tensor.empty") {
    allowed = {};
  } else if (operation.name == "tensor.extract_slice" ||
             operation.name == "memref.subview") {
    allowed = {"static_offsets", "static_sizes", "static_strides"};
  } else if (operation.name == "hivm.hir.load") {
    allowed = {"init_out_buffer", "may_implicit_transpose_with_last_axis",
               "operandSegmentSizes", "tcoretype"};
    if (operation.operands.size() != 2) {
      reason = "load padding/condition operands are outside the identity model";
      return false;
    }
  } else if (operation.name == "hivm.hir.vadd") {
    allowed = {"operandSegmentSizes", "tcoretype", "transpose"};
  } else if (operation.name == "hivm.hir.store") {
    allowed = {"atomic_kind", "may_implicit_transpose_with_last_axis",
               "tcoretype"};
  } else if (operation.name == "hivm.hir.mmadL1") {
    allowed = {"hivm.tile_mix_cube_num", "operandSegmentSizes", "tcoretype"};
  } else if (operation.name == "hivm.hir.fixpipe") {
    allowed = {"dma_mode", "operandSegmentSizes", "tcoretype"};
  } else {
    reason = "operation has no exact identity-axis attribute model";
    return false;
  }

  for (const std::string &key : DictionaryKeys(operation.attributes)) {
    // Scheduling/provenance unit attributes are deliberately ignored by the
    // real DimensionAnalyzer; they do not participate in any dimension
    // relation. Keep the same behavior instead of treating them as an
    // unknown axis transform.
    if (key == "\"inserted-load\"" || key == "\"inserted-store\"")
      continue;
    if (allowed.count(key) == 0) {
      reason = "operation attribute '" + key +
               "' has unknown axis semantics";
      return false;
    }
  }
  const std::string implicit = IRDictionaryValue(
      operation.attributes, "may_implicit_transpose_with_last_axis");
  if (!implicit.empty() && implicit != "false") {
    reason = "implicit transpose breaks identity axis equivalence";
    return false;
  }
  const std::string transpose =
      IRDictionaryValue(operation.attributes, "transpose");
  if (!transpose.empty() && !IsIdentityPermutation(transpose, rank)) {
    reason = "transpose attribute breaks identity axis equivalence";
    return false;
  }
  if (operation.name == "hivm.hir.fixpipe") {
    const std::string dmaMode =
        IRDictionaryValue(operation.attributes, "dma_mode");
    if (!dmaMode.empty() && dmaMode != "#hivm.dma_mode<nz2nd>") {
      reason = "fixpipe dma_mode is outside the proven nz2nd axis model";
      return false;
    }
  }
  return true;
}

inline bool RecordIdentityDependencyAxis(const GenericModule &module,
                                         const GenericOperation &loop,
                                         LoopPlan &plan, int value,
                                         const std::string &typeText,
                                         size_t axis, int64_t extent,
                                         std::string &reason) {
  if (!RecordValueAxis(module, plan, value, typeText, axis, extent, reason,
                       /*allowRepeatedExtent=*/true))
    return false;
  const GenericOperation *definition = Definition(module, value);
  if (definition == nullptr || !IsAncestor(module, loop.id, definition->id) ||
      definition->name == "tensor.empty" ||
      definition->name == "bufferization.to_tensor" ||
      definition->name == "tensor.extract_slice" ||
      definition->name == "memref.subview")
    return true;
  if (!IsSupportedProducer(*definition, LoopKind::Vector)) {
    reason = "dependency axis equivalence is not explicitly modeled: " +
             definition->name;
    return false;
  }
  const auto resultType = ParseStaticShapedType(typeText);
  if (!resultType)
    return false;
  for (size_t index = 0; index < definition->operands.size(); ++index) {
    const auto operandType =
        ParseStaticShapedType(definition->operandTypes[index]);
    if (!operandType)
      continue;
    std::optional<size_t> operandAxis;
    if (axis < operandType->shape.size() &&
        operandType->shape[axis] == extent) {
      // Elementwise/DPS operations and the leftmost-non-unit reshape rule in
      // the real analyzer preserve the ordered axis whenever possible.
      operandAxis = axis;
    } else {
      for (size_t candidate = 0; candidate < operandType->shape.size();
           ++candidate) {
        if (operandType->shape[candidate] != extent)
          continue;
        if (!operandAxis)
          operandAxis = candidate;
      }
    }
    // Broadcast/reduction auxiliaries that do not carry the selected
    // parallel axis are not fused for that axis by DimensionAnalyzer.
    if (!operandAxis)
      continue;
    if (!RecordIdentityDependencyAxis(module, loop, plan,
                                      definition->operands[index],
                                      definition->operandTypes[index],
                                      *operandAxis,
                                      extent, reason))
      return false;
  }
  return true;
}

inline bool ValidateIdentityDependencyChain(
    const GenericModule &module, const GenericOperation &loop, int value,
    const std::string &typeText, size_t axis, int64_t extent,
    std::set<int> &visited, std::string &reason) {
  const auto type = ParseStaticShapedType(typeText);
  if (!type || axis >= type->shape.size() || type->shape[axis] != extent) {
    reason = "identity dependency type does not carry the semantic axis";
    return false;
  }
  const GenericOperation *definition = Definition(module, value);
  if (definition == nullptr || !IsAncestor(module, loop.id, definition->id) ||
      definition->name == "tensor.empty" ||
      !visited.insert(definition->id).second)
    return true;
  if (definition->name != "tensor.extract_slice" &&
      definition->name != "memref.subview" &&
      definition->name != "hivm.hir.load" &&
      definition->name != "hivm.hir.vadd") {
    reason = "dependency axis equivalence is not explicitly modeled";
    return false;
  }
  if (!HasExactIdentitySemantics(*definition, type->shape.size(), reason))
    return false;
  for (size_t index = 0; index < definition->operands.size(); ++index) {
    if (!ParseStaticShapedType(definition->operandTypes[index]))
      continue;
    if (!ValidateIdentityDependencyChain(
            module, loop, definition->operands[index],
            definition->operandTypes[index], axis, extent, visited, reason))
      return false;
  }
  return true;
}

inline bool CheckProvenVectorAlignment(const GenericModule &module,
                                       const LoopPlan &plan,
                                       unsigned tripCount, int64_t alignBits,
                                       std::string &reason,
                                       bool &alignmentRollback) {
  alignmentRollback = false;
  for (int child : plan.producerClosure) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(child));
    const auto checkValues = [&](const std::vector<int> &values,
                                 const std::vector<std::string> &types) {
      for (size_t index = 0; index < values.size(); ++index) {
        const auto axis = plan.valueAxes.find(values[index]);
        if (axis == plan.valueAxes.end())
          continue;
        const auto type = ParseStaticShapedType(types[index]);
        if (!type || axis->second >= type->shape.size()) {
          reason = "alignment value " + std::to_string(values[index]) +
                   " has incompatible type " + types[index] + " in " +
                   operation.name;
          return false;
        }
        ShapedType tiled = *type;
        tiled.shape[axis->second] /= static_cast<int64_t>(tripCount);
        const auto bits = StaticSizeBits(tiled);
        if (!bits) {
          reason = "physical size overflows or has unknown element width";
          return false;
        }
        const int64_t actualAlign =
            ElementBitWidth(tiled.tail) == 1 ? 8 : alignBits;
        if (*bits % actualAlign != 0)
          alignmentRollback = true;
      }
      return true;
    };
    if (!checkValues(operation.operands, operation.operandTypes) ||
        !checkValues(operation.results, operation.resultTypes)) {
      if (reason.empty())
        reason = "aligned dependency operand/result has no proven tiled type";
      return false;
    }
  }
  return true;
}

inline bool ValidateCubeMmadODS(const GenericOperation &mmad,
                                size_t &realDimensionDpsInput,
                                size_t realDimensionOperand,
                                std::string &reason) {
  std::vector<size_t> operandSegments;
  std::vector<size_t> dpsInitIndices;
  try {
    operandSegments = OperandSegmentSizes(mmad.properties);
    dpsInitIndices =
        DpsInitOperandIndices(mmad.name, mmad.operands.size(), mmad.properties);
  } catch (const std::exception &) {
    reason = "MmadL1 has malformed operand segment metadata";
    return false;
  }
  const std::vector<size_t> coreSegments = {1, 1, 1, 1, 1,
                                             1, 1, 0, 0, 0};
  std::vector<int> expectedInputs;
  if (mmad.operands.size() >= 6)
    expectedInputs.assign(mmad.operands.begin(), mmad.operands.begin() + 6);
  const bool hasMaterializedDps =
      !mmad.dpsInputs.empty() || !mmad.dpsInits.empty();
  if (mmad.results.size() != 1 || mmad.resultTypes.size() != 1 ||
      mmad.operands.size() != 7 || mmad.operandTypes.size() != 7 ||
      operandSegments != coreSegments ||
      dpsInitIndices != std::vector<size_t>{6} ||
      (hasMaterializedDps &&
       (mmad.dpsInputs != expectedInputs ||
        mmad.dpsInits != std::vector<int>{mmad.operands[6]})) ||
      mmad.operandTypes[2] != "i1" || mmad.operandTypes[3] != "index" ||
      mmad.operandTypes[4] != "index" || mmad.operandTypes[5] != "index" ||
      realDimensionOperand >= 6) {
    reason = "MmadL1 does not match the exact ODS core operand layout";
    return false;
  }
  realDimensionDpsInput = realDimensionOperand;
  return true;
}

inline bool AnalyzeLoop(const GenericModule &module,
                        const GenericOperation &loop, const TargetSpec &spec,
                        unsigned vectorTripCount, unsigned cubeTripCount,
                        LoopPlan &plan, std::string &reason) {
  const auto kind = CoreLoopKind(loop);
  if (!kind) {
    reason = "unrecognized hivm.loop_core_type";
    return false;
  }
  const bool isForLoop = loop.name == "scf.for";
  const bool isScope = loop.name == "scope.scope";
  if (!isForLoop && !isScope) {
    reason = "candidate is neither scf.for nor scope.scope";
    return false;
  }
  const std::vector<int> children = DirectChildren(module, loop);
  if (children.empty()) {
    reason = "candidate loop must have one region and one block";
    return false;
  }
  const GenericOperation &terminator =
      module.operations.at(static_cast<size_t>(children.back()));
  const std::string expectedTerminator =
      isForLoop ? "scf.yield" : "scope.return";
  if (terminator.name != expectedTerminator) {
    reason = "candidate loop/scope has a malformed terminator";
    return false;
  }
  // collectCubeLoopInfo never inspects the enclosing loop's yielded values:
  // it only discovers Fixpipe/Mmad branches and tiles those branches in place.
  // Consequently a result-carrying scf.for is not a special case for Cube.
  // Vector is different: collectVectorLoopInfo deliberately turns every
  // yielded tensor into a dummy Store and therefore needs the result-aware
  // transformation implemented below.
  if (*kind == LoopKind::Vector &&
      (terminator.operands.size() != loop.results.size() ||
       (isForLoop && loop.operands.size() < 3 + loop.results.size()))) {
    reason = "result-carrying vector loop/scope has malformed results";
    return false;
  }
  plan.loopId = loop.id;
  plan.kind = *kind;
  plan.tripCount = *kind == LoopKind::Vector ? vectorTripCount : cubeTripCount;
  bool hasCubeOverride = false;
  if (*kind == LoopKind::Cube) {
    const auto override = ParseCubeOverride(module, loop, reason);
    if (!reason.empty())
      return false;
    if (override) {
      plan.tripCount = *override;
      hasCubeOverride = true;
    }
  }
  const std::string anchorName =
      *kind == LoopKind::Vector ? "hivm.hir.store" : "hivm.hir.fixpipe";
  for (int descendant : Descendants(module, loop)) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(descendant));
    if (!IsDirectChild(loop, operation) && IsHIVMStructuredName(operation.name)) {
      reason = "nested HIVM structured operations are outside the exact body model";
      return false;
    }
  }
  std::vector<const GenericOperation *> anchors;
  for (int child : children) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(child));
    if (operation.name == anchorName)
      anchors.push_back(&operation);
  }
  if (*kind == LoopKind::Vector) {
    // DimensionAnalyzer::computeTilingDim scans scf.yield only when the
    // candidate itself is scf.for.  For scope.scope it derives candidates
    // exclusively from Store/Copy-like operations.  A result-only scope with
    // no Store therefore leaves every returned value at tilingDim=-1 and
    // tryCollectTilingInfoForTerminate rolls the transaction back.
    if (isScope && anchors.empty()) {
      plan.skip = true;
      return true;
    }
    for (const GenericOperation *anchor : anchors)
      plan.vectorStoreAnchors.push_back(anchor->id);
    for (size_t index = 0; index < terminator.operands.size(); ++index) {
      const auto yieldedType =
          ParseStaticShapedType(terminator.operandTypes[index]);
      if (!yieldedType || yieldedType->kind != "tensor") {
        // This is collectVectorLoopInfo's transactional failure path:
        // tryCollectTilingInfoForTerminate rejects the value, rolls back all
        // dummy stores, and processCandidateLoop deliberately ignores the
        // failure.
        plan.skip = true;
        return true;
      }
      const GenericOperation *producer = TraceVectorProducer(
          module, loop, terminator.operands[index]);
      if (producer == nullptr || producer->results.size() != 1 ||
          producer->resultTypes.size() != 1) {
        plan.skip = true;
        return true;
      }
      std::vector<size_t> initIndices;
      try {
        initIndices = DpsInitOperandIndices(
            producer->name, producer->operands.size(), producer->properties);
      } catch (const std::exception &) {
        plan.skip = true;
        return true;
      }
      if (initIndices.size() != 1 ||
          initIndices.front() >= producer->operands.size()) {
        plan.skip = true;
        return true;
      }
      plan.yieldedTensors.push_back(
          {terminator.operands[index], producer->id,
           producer->operands[initIndices.front()], 0,
           producer->resultTypes.front()});
    }
    // VectorLoopInfo records one dummy Store immediately after every traced
    // producer and sorts all tiled operations by payload topological order
    // before fuseLoops. Consequently, the fused loop's iter args/results are
    // ordered by producer position, not by the original scf.yield operand
    // number. Keep the original yielded value in each record so the outer
    // yield is remapped back to its original position after tiling.
    std::stable_sort(
        plan.yieldedTensors.begin(), plan.yieldedTensors.end(),
        [&](const LoopPlan::YieldedTensor &lhs,
            const LoopPlan::YieldedTensor &rhs) {
          const GenericOperation &lhsProducer = module.operations.at(
              static_cast<size_t>(lhs.producerId));
          const GenericOperation &rhsProducer = module.operations.at(
              static_cast<size_t>(rhs.producerId));
          if (lhsProducer.blockId != rhsProducer.blockId)
            return lhs.producerId < rhs.producerId;
          return lhsProducer.ordinal < rhsProducer.ordinal;
        });
  }
  if (anchors.empty()) {
    if (*kind == LoopKind::Cube) {
      plan.skip = true;
      return true;
    }
    if (plan.yieldedTensors.empty()) {
      plan.skip = true;
      return true;
    }
  }
  if (*kind == LoopKind::Cube && !hasCubeOverride) {
    // TileCubeVectorLoop.cpp::collectCubeLoopInfo calls
    // canFitBranchInBuffer before grouping branches or calculating their
    // tiling axes. Preserve that ordering: small result-carrying and
    // multi-Fixpipe loops are successful no-ops even when DimensionAnalyzer
    // would not later be able to form a group.
    int64_t totalDestinationBits = 0;
    bool allStatic = true;
    for (const GenericOperation *anchor : anchors) {
      if (anchor->operandTypes.size() < 2) {
        allStatic = false;
        break;
      }
      const auto destination = ParseStaticShapedType(anchor->operandTypes[1]);
      const auto bits = destination ? StaticSizeBits(*destination)
                                    : std::nullopt;
      if (!bits || totalDestinationBits >
                       std::numeric_limits<int64_t>::max() - *bits) {
        allStatic = false;
        break;
      }
      totalDestinationBits += *bits;
    }
    if (allStatic && totalDestinationBits <= spec.l0cSizeBits) {
      plan.skip = true;
      return true;
    }
  }
  if (*kind == LoopKind::Cube && anchors.size() != 1) {
    reason = "multiple Cube branches require real axis grouping";
    return false;
  }
  if (!anchors.empty() && anchors.front()->operandTypes.empty()) {
    reason = "tiling anchor has no shaped source";
    return false;
  }
  const auto sourceType = ParseStaticShapedType(
      !anchors.empty() ? anchors.front()->operandTypes.front()
                       : plan.yieldedTensors.front().type);
  if (!sourceType ||
      (*kind == LoopKind::Cube && sourceType->shape.size() != 2)) {
    reason = "DimensionAnalyzer tiling axis is not uniquely provable";
    return false;
  }

  if (*kind == LoopKind::Vector) {
    if (plan.tripCount == 1) {
      plan.skip = true;
      return true;
    }
    const auto axis = UniqueParallelAxis(*sourceType);
    if (!axis) {
      reason = "DimensionAnalyzer has no unique parallel store-source axis";
      return false;
    }
    plan.extent = sourceType->shape[*axis];
    if (plan.extent % static_cast<int64_t>(plan.tripCount) != 0) {
      reason = "non-divisible tiling requires a dynamic remainder model";
      return false;
    }
    for (LoopPlan::YieldedTensor &yielded : plan.yieldedTensors) {
      const auto type = ParseStaticShapedType(yielded.type);
      if (!type) {
        plan.skip = true;
        return true;
      }
      std::optional<size_t> yieldedAxis;
      if (*axis < type->shape.size() && type->shape[*axis] == plan.extent) {
        yieldedAxis = *axis;
      } else {
        for (size_t candidate = 0; candidate < type->shape.size(); ++candidate)
          if (type->shape[candidate] == plan.extent) {
            if (yieldedAxis) {
              plan.skip = true;
              return true;
            }
            yieldedAxis = candidate;
          }
      }
      if (!yieldedAxis) {
        plan.skip = true;
        return true;
      }
      yielded.axis = *yieldedAxis;
      const GenericOperation &producer =
          module.operations.at(static_cast<size_t>(yielded.producerId));
      if (!CollectProducerClosure(module, loop, producer.results.front(),
                                  *kind, plan.producerClosure, reason) ||
          !RecordIdentityDependencyAxis(
              module, loop, plan, producer.results.front(),
              producer.resultTypes.front(), yielded.axis, plan.extent,
              reason))
        return false;
      plan.producerClosure.insert(producer.id);
    }
    for (const GenericOperation *anchor : anchors) {
      if (anchor->operandTypes.empty()) {
        reason = "tiling anchor has no shaped source";
        return false;
      }
      const auto currentType = ParseStaticShapedType(anchor->operandTypes[0]);
      const auto currentAxis =
          currentType ? UniqueParallelAxis(*currentType) : std::nullopt;
      if (!currentType || !currentAxis || *currentAxis != *axis ||
          currentType->shape[*currentAxis] != plan.extent) {
        reason = "Vector stores do not share DimensionAnalyzer's selected axis";
        return false;
      }
      if (!HasExactIdentitySemantics(*anchor, currentType->shape.size(),
                                     reason) ||
          !CollectProducerClosure(module, loop, anchor->operands.front(),
                                  *kind, plan.producerClosure, reason))
        return false;
      const GenericOperation *vectorProducer =
          Definition(module, anchor->operands.front());
      if (vectorProducer == nullptr ||
          !IsSupportedProducer(*vectorProducer, LoopKind::Vector)) {
        reason =
            "shape-only Vector axis lacks HIVM structured semantic evidence";
        return false;
      }
      plan.producerClosure.insert(anchor->id);
      if (!RecordIdentityDependencyAxis(
              module, loop, plan, anchor->operands.front(),
              anchor->operandTypes.front(), *axis, plan.extent, reason))
        return false;
      if (anchor->operands.size() < 2 || anchor->operandTypes.size() < 2 ||
          !HasStaticBoundaryView(module, anchor->operands[1]) ||
          !RecordValueAxis(module, plan, anchor->operands[1],
                           anchor->operandTypes[1], *axis, plan.extent,
                           reason, /*allowRepeatedExtent=*/true)) {
        if (reason.empty())
          reason = "store destination axis is not equivalent";
        return false;
      }
      const int previousAlloc = plan.localDestinationAlloc;
      if (!ClassifyDestination(module, *anchor, plan, reason))
        return false;
      if (previousAlloc >= 0 && plan.localDestinationAlloc != previousAlloc) {
        reason = "multiple reusable local Store destinations are not modeled";
        return false;
      }
    }
    // The real collector tags every HIVM structured op in the candidate (and
    // reshape ops feeding one), not merely the backwards slice of a Store.
    // Those tags are subsequently matched in reverse order by fuse_into.
    for (int descendant : Descendants(module, loop)) {
      const GenericOperation &operation =
          module.operations.at(static_cast<size_t>(descendant));
      if (!IsDirectChild(loop, operation))
        continue;
      const bool reshape = operation.name == "tensor.expand_shape" ||
                           operation.name == "tensor.collapse_shape";
      if (!IsHIVMStructuredName(operation.name) && !reshape)
        continue;
      plan.producerClosure.insert(operation.id);
      const auto recordValues = [&](const std::vector<int> &values,
                                    const std::vector<std::string> &types) {
        for (size_t index = 0; index < values.size(); ++index) {
          const auto type = ParseStaticShapedType(types[index]);
          if (!type)
            continue;
          std::optional<size_t> carriedAxis;
          if (*axis < type->shape.size() &&
              type->shape[*axis] == plan.extent) {
            carriedAxis = *axis;
          } else {
            for (size_t candidate = 0; candidate < type->shape.size();
                 ++candidate)
              if (type->shape[candidate] == plan.extent) {
                if (!carriedAxis)
                  carriedAxis = candidate;
              }
          }
          if (carriedAxis &&
              !RecordValueAxis(module, plan, values[index], types[index],
                               *carriedAxis, plan.extent, reason,
                               /*allowRepeatedExtent=*/true))
            return false;
        }
        return true;
      };
      if (!recordValues(operation.operands, operation.operandTypes) ||
          !recordValues(operation.results, operation.resultTypes))
        return false;
      for (size_t index = 0; index < operation.operands.size(); ++index) {
        if (!ParseStaticShapedType(operation.operandTypes[index]))
          continue;
        if (!CollectProducerClosure(module, loop, operation.operands[index],
                                    *kind, plan.producerClosure, reason))
          return false;
      }
    }
    bool alignmentRollback = false;
    if (!CheckProvenVectorAlignment(module, plan, plan.tripCount,
                                    spec.ubAlignBits, reason,
                                    alignmentRollback))
      return false;
    if (alignmentRollback) {
      plan.skip = true;
      return true;
    }
  } else {
    const GenericOperation *mmad =
        Definition(module, anchors.front()->operands.front());
    if (mmad == nullptr || !IsAncestor(module, loop.id, mmad->id)) {
      reason = "fixpipe branch has no in-loop MmadL1 producer";
      return false;
    }
    if (mmad->name != "hivm.hir.mmadL1" || mmad->operands.size() <= 2) {
      reason = "fixpipe branch has no supported MmadL1 producer";
      return false;
    }
    if (anchors.front()->operands.size() < 2 ||
        anchors.front()->operandTypes.size() < 2) {
      reason = "fixpipe does not match the exact ODS core operand layout";
      return false;
    }
    plan.cubeRealDimensionOperand = 3U;
    if (!ValidateCubeMmadODS(*mmad, plan.cubeRealDimensionDpsInput,
                             plan.cubeRealDimensionOperand, reason))
      return false;
    if (plan.tripCount == 1) {
      plan.skip = true;
      return true;
    }
    const GenericOperation *accumulate = Definition(module, mmad->operands[2]);
    if (accumulate == nullptr || accumulate->name != "arith.constant") {
      plan.skip = true;
      return true;
    }
    const auto destinationType =
        ParseStaticShapedType(anchors.front()->operandTypes[1]);
    const auto aType = ParseStaticShapedType(mmad->operandTypes[0]);
    const auto bType = ParseStaticShapedType(mmad->operandTypes[1]);
    const auto accumulateType = ParseStaticShapedType(mmad->operandTypes[6]);
    const auto realM = ConstantIndexValue(module, mmad->operands[3]);
    const auto realK = ConstantIndexValue(module, mmad->operands[4]);
    const auto realN = ConstantIndexValue(module, mmad->operands[5]);
    if (!aType || !bType || !accumulateType || aType->shape.size() != 2 ||
        bType->shape.size() != 2 || accumulateType->shape != sourceType->shape ||
        aType->shape[1] != bType->shape[0] ||
        sourceType->shape[0] != aType->shape[0] ||
        sourceType->shape[1] != bType->shape[1] ||
        !realM || !realK || !realN || *realM != aType->shape[0] ||
        *realK != aType->shape[1] || *realN != bType->shape[1] ||
        !HasExactIdentitySemantics(*mmad, 2, reason) ||
        !HasExactIdentitySemantics(*anchors.front(), 2, reason)) {
      if (reason.empty())
        reason = "MmadL1/fixpipe lacks exact MxK.KxN-to-MxN axis evidence";
      return false;
    }
    if (!destinationType || destinationType->shape.size() != 2 ||
        destinationType->shape != sourceType->shape ||
        !HasStaticBoundaryView(module, anchors.front()->operands[1]) ||
        mmad->attributes.find("transpose") != std::string::npos) {
      reason = "fixpipe source and destination axes are not equivalent";
      return false;
    }
    const bool aInside =
        FindInLoopLoadDependency(module, loop, mmad->operands[0]) != nullptr;
    const bool bInside =
        FindInLoopLoadDependency(module, loop, mmad->operands[1]) != nullptr;
    std::optional<size_t> axis;
    if (aInside != bInside)
      axis = aInside ? 0U : 1U;
    if (!axis) {
      reason = "Cube M/N axis lacks exact DimensionAnalyzer load evidence";
      return false;
    }
    plan.extent = sourceType->shape[*axis];
    plan.cubeMmadId = mmad->id;
    plan.cubeRealDimensionOperand = *axis == 0 ? 3U : 5U;
    plan.cubeRealDimensionDpsInput = plan.cubeRealDimensionOperand;
    if ((aInside != bInside) &&
        !ProveLoadAxisEvidence(module, loop, mmad->operands[*axis], *axis,
                               plan.extent, reason)) {
      if (reason.empty())
        reason = "inside-load evidence does not prove the selected group axis";
      return false;
    }
    if (plan.extent % static_cast<int64_t>(plan.tripCount) != 0) {
      reason = "non-divisible tiling requires a dynamic remainder model";
      return false;
    }
    plan.producerClosure.insert(mmad->id);
    if (!CollectProducerClosure(module, loop, mmad->operands[*axis], *kind,
                                plan.producerClosure, reason) ||
        !CollectProducerClosure(module, loop, mmad->operands[6], *kind,
                                plan.producerClosure, reason))
      return false;
    const size_t otherAxis = *axis == 0 ? 1 : 0;
    if (!CollectProducerClosure(module, loop, mmad->operands[otherAxis], *kind,
                                plan.preservedProducerClosure, reason))
      return false;
    plan.producerClosure.insert(anchors.front()->id);
    if (!RecordValueAxis(module, plan, mmad->results[0], mmad->resultTypes[0],
                         *axis, plan.extent, reason, true) ||
        !RecordValueAxis(module, plan, mmad->operands[6],
                         mmad->operandTypes[6], *axis, plan.extent, reason,
                         true) ||
        !RecordValueAxis(module, plan, anchors.front()->operands[1],
                         anchors.front()->operandTypes[1], *axis, plan.extent,
                         reason, true))
      return false;
    const size_t matrixOperand = *axis == 0 ? 0 : 1;
    if (!RecordIdentityDependencyAxis(
            module, loop, plan, mmad->operands[matrixOperand],
            mmad->operandTypes[matrixOperand], *axis, plan.extent, reason))
      return false;
    if (!ClassifyDestination(module, *anchors.front(), plan, reason))
      return false;
    std::set<int> preservedEvidence;
    const GenericOperation *matrix =
        Definition(module, anchors.front()->operands.front());
    const int64_t preservedExtent = sourceType->shape[otherAxis];
    if (!ValidateIdentityDependencyChain(
            module, loop, matrix->operands[otherAxis],
            matrix->operandTypes[otherAxis],
            otherAxis, preservedExtent, preservedEvidence, reason))
      return false;
    const auto bits = StaticSizeBits(*destinationType);
    if (!bits) {
      reason = "L0C branch size overflows or has unknown element width";
      return false;
    }
    if (!hasCubeOverride && *bits <= spec.l0cSizeBits) {
      plan.skip = true;
      return true;
    }
  }
  for (int descendant : Descendants(module, loop)) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(descendant));
    if (!IsDirectChild(loop, operation))
      continue;
    if (IsHIVMStructuredName(operation.name) &&
        plan.producerClosure.count(operation.id) == 0 &&
        plan.preservedProducerClosure.count(operation.id) == 0) {
      reason = "HIVM structured operation lies outside the proven producer closure";
      return false;
    }
  }
  return true;
}

inline bool ValuesAlignedAfterTiling(
    const std::vector<int> &values, const std::vector<std::string> &types,
    const DimensionAnalyzer &analyzer, unsigned tripCount,
    int64_t alignmentBits) {
  for (size_t index = 0; index < values.size() && index < types.size();
       ++index) {
    const int64_t tilingDim = analyzer.getTilingDim(values[index]);
    // Preserve TileCubeVectorLoop.cpp::areValuesAlignedAfterTiling exactly:
    // the first un-tiled value makes the whole ValueRange acceptable.
    if (tilingDim == -1)
      return true;
    const auto type = ParseStaticShapedType(types[index]);
    if (!type || type->kind != "tensor" || tilingDim < 0 ||
        static_cast<size_t>(tilingDim) >= type->shape.size())
      continue;
    ShapedType tiled = *type;
    tiled.shape[static_cast<size_t>(tilingDim)] /=
        static_cast<int64_t>(tripCount);
    const auto bits = StaticSizeBits(tiled);
    if (!bits)
      continue;
    const int64_t actualAlignment =
        ElementBitWidth(tiled.tail) == 1 ? 8 : alignmentBits;
    if (*bits % actualAlignment != 0)
      return false;
  }
  return true;
}

inline bool VectorCollectionEmitsDiagnostic(const GenericModule &module,
                                            const GenericOperation &loop,
                                            unsigned tripCount,
                                            int64_t alignmentBits) {
  if (tripCount == 1)
    return false;
  DimensionAnalyzer analyzer(module);
  if (!analyzer.initialize())
    return true;
  analyzer.computeTilingDim(loop);
  for (int operationId : Descendants(module, loop)) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    if (operation.name == "hivm.hir.store") {
      if (operation.operands.empty() ||
          analyzer.getTilingDim(operation.operands.front()) == -1)
        return true;
      continue;
    }
    if (operation.name == "scf.for" &&
        (operation.properties.find("ExtractedLoadOrStore") !=
             std::string::npos ||
         operation.attributes.find("ExtractedLoadOrStore") !=
             std::string::npos))
      return true;
    if (!ValuesAlignedAfterTiling(operation.results, operation.resultTypes,
                                  analyzer, tripCount, alignmentBits) ||
        !ValuesAlignedAfterTiling(operation.operands, operation.operandTypes,
                                  analyzer, tripCount, alignmentBits))
      return true;
  }
  return false;
}

inline bool RewriteType(std::string &typeText, int64_t extent,
                        int64_t tileSize, size_t axis) {
  auto type = ParseStaticShapedType(typeText);
  if (!type || axis >= type->shape.size() || type->shape[axis] != extent)
    return false;
  type->shape[axis] = tileSize;
  typeText = PrintShapedType(*type);
  return true;
}

inline void UpdateMaterializedDpsOperand(GenericOperation &operation,
                                         size_t operandIndex, int oldValue,
                                         int newValue) {
  if (operation.dpsInputs.empty() && operation.dpsInits.empty())
    return;
  const std::vector<size_t> initIndices = DpsInitOperandIndices(
      operation.name, operation.operands.size(), operation.properties);
  const std::set<size_t> initSet(initIndices.begin(), initIndices.end());
  const bool isInit = initSet.count(operandIndex) != 0;
  size_t dpsPosition = 0;
  for (size_t index = 0; index < operandIndex; ++index)
    if ((initSet.count(index) != 0) == isInit)
      ++dpsPosition;
  std::vector<int> &values = isInit ? operation.dpsInits : operation.dpsInputs;
  if (dpsPosition >= values.size() || values[dpsPosition] != oldValue)
    throw std::runtime_error(
        "DPS bookkeeping disagrees with the rewritten operand position");
  values[dpsPosition] = newValue;
}

// VectorLoopInfo creates one tiled loop for every real Store and for every
// dummy Store inserted immediately after a yielded HIVM producer. OpToTile's
// ordering compares the payload positions of all of those roots together
// before fuseLoops runs. The resulting payload is therefore a dependency-
// topological schedule rooted by that combined source order; placing all real
// Stores before all yielded producers incorrectly reorders independent
// branches and changes the later fuse_into/CSE placement of boundary tiles.
inline std::vector<int> VectorFusedProducerOrder(const GenericModule &module,
                                                 const LoopPlan &plan,
                                                 int boundaryBlock = -1) {
  std::vector<int> ordered;
  std::set<int> visited;
  std::set<int> visitedBoundaryTiles;
  std::function<void(int)> visit = [&](int operationId) {
    if (plan.producerClosure.count(operationId) == 0 ||
        !visited.insert(operationId).second)
      return;
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    for (int operand : operation.operands) {
      const GenericOperation *definition = Definition(module, operand);
      if (definition == nullptr)
        continue;
      if (plan.producerClosure.count(definition->id) != 0) {
        visit(definition->id);
      } else if (boundaryBlock >= 0 &&
                 definition->blockId == boundaryBlock &&
                 (definition->name == "tensor.extract_slice" ||
                  definition->name == "memref.subview") &&
                 visitedBoundaryTiles.insert(definition->id).second) {
        ordered.push_back(definition->id);
      }
    }
    ordered.push_back(operationId);
  };
  std::vector<int> tiledRoots = plan.vectorStoreAnchors;
  for (const LoopPlan::YieldedTensor &yielded : plan.yieldedTensors)
    tiledRoots.push_back(yielded.producerId);
  std::stable_sort(tiledRoots.begin(), tiledRoots.end(),
                   [&](int lhs, int rhs) {
                     const GenericOperation &lhsOperation =
                         module.operations.at(static_cast<size_t>(lhs));
                     const GenericOperation &rhsOperation =
                         module.operations.at(static_cast<size_t>(rhs));
                     if (lhsOperation.blockId != rhsOperation.blockId)
                       return lhsOperation.id < rhsOperation.id;
                     return lhsOperation.ordinal < rhsOperation.ordinal;
                   });
  for (int root : tiledRoots)
    visit(root);
  for (int operationId : plan.producerClosure)
    visit(operationId);
  return ordered;
}

inline void ApplyTiling(GenericModule &module, const LoopPlan &plan) {
  const GenericOperation snapshot =
      module.operations.at(static_cast<size_t>(plan.loopId));
  const int64_t tileSize =
      plan.extent / static_cast<int64_t>(plan.tripCount);
  const int outerRegion = snapshot.regions.front();
  const int outerBlock =
      module.regions.at(static_cast<size_t>(outerRegion)).blocks.front();
  const std::vector<int> children =
      module.blocks.at(static_cast<size_t>(outerBlock)).operations;
  const std::set<int> &affected = plan.producerClosure;
  const int terminator = children.back();

  GenericRewriter rewriter(module);
  std::vector<int> createdConstants;
  auto getOrCreateIndexConstant = [&](int64_t literal) {
    const std::string text = std::to_string(literal);
    if (const std::optional<int> existing =
            FindArithConstantValue(module, outerBlock, "index", text))
      return *existing;
    const int operation = rewriter.createOperation(
        plan.loopId, outerRegion, outerBlock, "arith.constant", {"index"}, {},
        {}, "", "{value = " + text + " : index}");
    createdConstants.push_back(operation);
    return module.operations.at(static_cast<size_t>(operation)).results.front();
  };
  // transform.structured.tile_using_for emits the actual tile offset as the
  // induction variable: [0, extent) with step=tileSize.  The old bridge used
  // [0, tripCount) step 1 plus an affine multiply, which introduced an extra
  // SSA operation and changed slice/insert liveness.
  const int zeroValue = getOrCreateIndexConstant(0);
  const int upperValue = getOrCreateIndexConstant(plan.extent);
  const int stepValue = getOrCreateIndexConstant(tileSize);
  std::vector<int> innerOperands = {zeroValue, upperValue, stepValue};
  std::vector<std::string> innerOperandTypes = {"index", "index", "index"};
  std::vector<std::string> innerResultTypes;
  for (const LoopPlan::YieldedTensor &yielded : plan.yieldedTensors) {
    innerOperands.push_back(yielded.originalInit);
    innerOperandTypes.push_back(yielded.type);
    innerResultTypes.push_back(yielded.type);
  }
  const int innerLoop = rewriter.createOperation(
      plan.loopId, outerRegion, outerBlock, "scf.for", innerResultTypes,
      innerOperands, innerOperandTypes);
  const int innerRegion = rewriter.createRegion(innerLoop);
  std::vector<std::string> innerBlockTypes = {"index"};
  innerBlockTypes.insert(innerBlockTypes.end(), innerResultTypes.begin(),
                         innerResultTypes.end());
  const int innerBlock = rewriter.createBlock(innerRegion, innerBlockTypes);
  const int innerIV =
      module.blocks.at(static_cast<size_t>(innerBlock)).arguments.front();
  const std::vector<int> &innerBlockArguments =
      module.blocks.at(static_cast<size_t>(innerBlock)).arguments;
  const int offsetValue = innerIV;

  if (plan.kind == LoopKind::Cube) {
    if (plan.cubeMmadId < 0)
      throw std::runtime_error("Cube plan has no MmadL1 operation");
    GenericOperation &matrix = module.operations.at(
        static_cast<size_t>(plan.cubeMmadId));
    const bool hasMaterializedDps =
        !matrix.dpsInputs.empty() || !matrix.dpsInits.empty();
    if (plan.cubeRealDimensionOperand >= matrix.operands.size() ||
        plan.cubeRealDimensionOperand >= matrix.operandTypes.size() ||
        (hasMaterializedDps &&
         plan.cubeRealDimensionDpsInput >= matrix.dpsInputs.size()))
      throw std::runtime_error("Cube MmadL1 real dimension operand is missing");
    const int oldDimension = matrix.operands[plan.cubeRealDimensionOperand];
    if (hasMaterializedDps &&
        matrix.dpsInputs[plan.cubeRealDimensionDpsInput] != oldDimension)
      throw std::runtime_error(
          "Cube MmadL1 DPS input position disagrees with its operand segment");
    const int remaining = rewriter.createOperation(
        innerLoop, innerRegion, innerBlock, "affine.min", {"index"},
        {innerIV},
        {"index"},
        "{map = affine_map<(d0) -> (-d0 + " +
            std::to_string(plan.extent) + ", " +
            std::to_string(tileSize) + ")>}",
        "");
    rewriter.appendToBlock(innerBlock, remaining);
    const int tiledDimension =
        module.operations.at(static_cast<size_t>(remaining)).results.front();
    matrix.operands[plan.cubeRealDimensionOperand] = tiledDimension;
    if (hasMaterializedDps)
      matrix.dpsInputs[plan.cubeRealDimensionDpsInput] = tiledDimension;
  }

  if (plan.localDestinationAlloc >= 0) {
    GenericOperation &alloc = module.operations.at(
        static_cast<size_t>(plan.localDestinationAlloc));
    const GenericOperation *subview = plan.localDestinationSubview >= 0
        ? &module.operations.at(static_cast<size_t>(plan.localDestinationSubview))
        : nullptr;
    if (alloc.results.size() != 1 ||
        (subview != nullptr && subview->results.size() != 1))
      throw std::runtime_error("malformed reusable local destination");
    rewriter.removeFromBlock(alloc.blockId, alloc.id);
    rewriter.appendToBlock(innerBlock, alloc.id);
    alloc.parentId = innerLoop;
    alloc.regionId = innerRegion;
    alloc.blockId = innerBlock;
    if (!RewriteType(alloc.resultTypes.front(), plan.extent, tileSize,
                     plan.valueAxes.at(alloc.results.front())))
      throw std::runtime_error("cannot shrink reusable local destination");
    if (subview != nullptr) {
      ReplaceAllUses(module, subview->results.front(), alloc.results.front());
      EraseOperationTree(module, subview->id);
    }
  }

  for (size_t index = 0; index + 1 < children.size(); ++index) {
    if (children[index] == plan.localDestinationSubview)
      continue;
    if (children[index] == plan.localDestinationAlloc)
      continue;
    if (affected.count(children[index]) == 0)
      continue;
    rewriter.removeFromBlock(outerBlock, children[index]);
    rewriter.appendToBlock(innerBlock, children[index]);
    GenericOperation &moved =
        module.operations.at(static_cast<size_t>(children[index]));
    moved.parentId = innerLoop;
    moved.regionId = innerRegion;
    moved.blockId = innerBlock;
  }

  if (plan.kind == LoopKind::Vector) {
    const std::vector<int> fusedOrder =
        VectorFusedProducerOrder(module, plan);
    GenericBlock &body = module.blocks.at(static_cast<size_t>(innerBlock));
    std::vector<int> reordered;
    reordered.reserve(body.operations.size());
    for (int operationId : body.operations)
      if (plan.producerClosure.count(operationId) == 0)
        reordered.push_back(operationId);
    reordered.insert(reordered.end(), fusedOrder.begin(), fusedOrder.end());
    body.operations = std::move(reordered);
    for (size_t ordinal = 0; ordinal < body.operations.size(); ++ordinal)
      module.operations.at(static_cast<size_t>(body.operations[ordinal]))
          .ordinal = static_cast<int>(ordinal);
  }

  const auto &outerOperations =
      module.blocks.at(static_cast<size_t>(outerBlock)).operations;
  const auto found =
      std::find(outerOperations.begin(), outerOperations.end(), terminator);
  size_t position =
      static_cast<size_t>(std::distance(outerOperations.begin(), found));
  for (int constant : createdConstants)
    rewriter.insertToBlock(outerBlock, position++, constant);
  rewriter.insertToBlock(outerBlock, position, innerLoop);

  std::map<std::pair<int, int>, int> boundaryTiles;
  // Keep boundary construction in the already validated fused payload order.
  // The real reverse producer order is used below only to assign placement
  // ownership; changing construction order also changes OperationFolder/CSE
  // and therefore the resulting UB buffers.
  const std::vector<int> scheduledInnerOperations =
      module.blocks.at(static_cast<size_t>(innerBlock)).operations;
  auto firstConsumerSharesLoadDestination =
      [&](const GenericOperation &load, int destination) {
        if (load.results.empty())
          return false;
        const auto loadPosition = std::find(scheduledInnerOperations.begin(),
                                            scheduledInnerOperations.end(),
                                            load.id);
        if (loadPosition == scheduledInnerOperations.end())
          return false;
        for (auto iterator = std::next(loadPosition);
             iterator != scheduledInnerOperations.end(); ++iterator) {
          const GenericOperation &consumer =
              module.operations.at(static_cast<size_t>(*iterator));
          if (affected.count(consumer.id) == 0 ||
              std::find(consumer.operands.begin(), consumer.operands.end(),
                        load.results.front()) == consumer.operands.end())
            continue;
          const std::vector<size_t> initIndices = DpsInitOperandIndices(
              consumer.name, consumer.operands.size(), consumer.properties);
          return std::any_of(
              initIndices.begin(), initIndices.end(), [&](size_t index) {
                return index < consumer.operands.size() &&
                       consumer.operands[index] == destination;
              });
        }
        return false;
      };
  for (int operationId : scheduledInnerOperations) {
    if (affected.count(operationId) == 0)
      continue;
    GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    if (operation.blockId != innerBlock ||
        operation.name == "tensor.extract_slice" ||
        operation.name == "memref.subview")
      continue;
    const std::vector<int> originalOperands = operation.operands;
    const std::vector<std::string> originalOperandTypes = operation.operandTypes;
    for (size_t index = 0; index < originalOperands.size(); ++index) {
      const int original = originalOperands[index];
      const std::string originalType = originalOperandTypes[index];
      const auto axisEntry = plan.valueAxes.find(original);
      if (axisEntry == plan.valueAxes.end())
        continue;
      const GenericOperation *definition = Definition(module, original);
      if (definition != nullptr && affected.count(definition->id) != 0)
        continue;
      const std::vector<size_t> initIndices = DpsInitOperandIndices(
          operation.name, operation.operands.size(), operation.properties);
      const bool isDpsInit =
          std::find(initIndices.begin(), initIndices.end(), index) !=
          initIndices.end();
      int yieldedIterArg = -1;
      for (size_t yieldedIndex = 0;
           yieldedIndex < plan.yieldedTensors.size(); ++yieldedIndex) {
        const LoopPlan::YieldedTensor &yielded =
            plan.yieldedTensors[yieldedIndex];
        if (yielded.producerId == operation.id &&
            yielded.originalInit == original) {
          yieldedIterArg = innerBlockArguments[yieldedIndex + 1];
          break;
        }
      }
      // OpBuilder::clone does not rewrite arbitrary captures of a loop init.
      // In particular, intermediate DPS destinations keep using the captured
      // tensor.empty. The final yielded producer is the explicit loop-carried
      // destination and therefore uses the corresponding iter_arg.
      const int sliceBase =
          isDpsInit && yieldedIterArg >= 0 ? yieldedIterArg : original;
      int tiledValue = -1;
      const auto cacheKey = std::make_pair(original, sliceBase);
      const bool distinctLoadDestinationTile =
          operation.name == "hivm.hir.load" && isDpsInit &&
          firstConsumerSharesLoadDestination(operation, original);
      const auto cached = boundaryTiles.find(cacheKey);
      if (!distinctLoadDestinationTile && cached != boundaryTiles.end()) {
        tiledValue = cached->second;
      } else {
        auto type = ParseStaticShapedType(originalType);
        if (!type || axisEntry->second >= type->shape.size())
          throw std::runtime_error("boundary value has no proven static axis");
        std::vector<int64_t> offsets(type->shape.size(), 0);
        std::vector<int64_t> sizes = type->shape;
        std::vector<int64_t> strides(type->shape.size(), 1);
        offsets[axisEntry->second] = std::numeric_limits<int64_t>::min();
        sizes[axisEntry->second] = tileSize;
        ShapedType tiledType = *type;
        tiledType.shape[axisEntry->second] = tileSize;
        std::string resultType = PrintShapedType(tiledType);
        if (type->kind == "memref")
          resultType = TileAddDynamicOffset(std::move(resultType));
        const std::string sliceName =
            type->kind == "memref" ? "memref.subview" : "tensor.extract_slice";
        const int slice = rewriter.createOperation(
            innerLoop, innerRegion, innerBlock, sliceName, {resultType},
            {sliceBase, offsetValue},
            {originalType, "index"},
            "{operandSegmentSizes = array<i32: 1, 1, 0, 0>, "
            "static_offsets = " + PrintIntegerArray(offsets) +
                ", static_sizes = " + PrintIntegerArray(sizes) +
                ", static_strides = " + PrintIntegerArray(strides) + "}",
            "{}");
        const auto &innerOperations =
            module.blocks.at(static_cast<size_t>(innerBlock)).operations;
        const auto before =
            std::find(innerOperations.begin(), innerOperations.end(), operationId);
        rewriter.insertToBlock(
            innerBlock,
            static_cast<size_t>(std::distance(innerOperations.begin(), before)),
            slice);
        tiledValue =
            module.operations.at(static_cast<size_t>(slice)).results.front();
        if (!distinctLoadDestinationTile)
          boundaryTiles[cacheKey] = tiledValue;
      }
      GenericOperation &updated =
          module.operations.at(static_cast<size_t>(operationId));
      UpdateMaterializedDpsOperand(updated, index, original, tiledValue);
      updated.operands[index] = tiledValue;
      RewriteType(updated.operandTypes[index], plan.extent, tileSize,
                  axisEntry->second);
      if (const GenericOperation *slice = Definition(module, tiledValue);
          slice != nullptr && !slice->resultTypes.empty())
        updated.operandTypes[index] = slice->resultTypes.front();
    }
  }

  if (plan.kind == LoopKind::Vector) {
    // The first producer ordering establishes the fused payload.  Boundary
    // tiles do not exist yet at that point.  Rebuild the same order after
    // materialization so the external operand tiles occupy the positions
    // chosen by ExtendedFuseIntoContainingOp instead of being grouped ahead
    // of every producer merely because they are outside producerClosure.
    const std::vector<int> fusedOrder =
        VectorFusedProducerOrder(module, plan, innerBlock);
    const std::set<int> scheduled(fusedOrder.begin(), fusedOrder.end());
    GenericBlock &body = module.blocks.at(static_cast<size_t>(innerBlock));
    std::vector<int> reordered;
    reordered.reserve(body.operations.size());
    for (int operationId : body.operations)
      if (plan.producerClosure.count(operationId) == 0 &&
          scheduled.count(operationId) == 0)
        reordered.push_back(operationId);
    reordered.insert(reordered.end(), fusedOrder.begin(), fusedOrder.end());
    body.operations = std::move(reordered);
    for (size_t ordinal = 0; ordinal < body.operations.size(); ++ordinal)
      module.operations.at(static_cast<size_t>(body.operations[ordinal]))
          .ordinal = static_cast<int>(ordinal);
  }

  for (GenericOperation &operation : module.operations) {
    if (affected.count(operation.id) == 0)
      continue;
    for (size_t index = 0; index < operation.resultTypes.size(); ++index) {
      const auto axis = plan.valueAxes.find(operation.results[index]);
      if (axis != plan.valueAxes.end())
        RewriteType(operation.resultTypes[index], plan.extent, tileSize,
                    axis->second);
    }
    for (size_t index = 0; index < operation.operandTypes.size(); ++index) {
      if (index == 0 && (operation.name == "tensor.extract_slice" ||
                         operation.name == "memref.subview"))
        continue;
      const auto axis = plan.valueAxes.find(operation.operands[index]);
      if (axis != plan.valueAxes.end())
        RewriteType(operation.operandTypes[index], plan.extent, tileSize,
                    axis->second);
    }
    if (operation.name != "tensor.extract_slice" &&
        operation.name != "memref.subview") {
      if (operation.name == "tensor.expand_shape" &&
          !operation.resultTypes.empty()) {
        const auto expanded = ParseStaticShapedType(operation.resultTypes.front());
        if (expanded)
          SetTileOperationDictionaryValue(
              operation, "static_output_shape",
              PrintIntegerArray(expanded->shape));
      }
      continue;
    }
    auto sizes = ParseIntegerArray(
        IRDictionaryValue(operation.attributes, "static_sizes"));
    auto offsets = ParseIntegerArray(
        IRDictionaryValue(operation.attributes, "static_offsets"));
    auto strides = ParseIntegerArray(
        IRDictionaryValue(operation.attributes, "static_strides"));
    if (operation.results.empty())
      throw std::runtime_error("slice/subview has no result");
    const auto axisEntry = plan.valueAxes.find(operation.results.front());
    if (axisEntry == plan.valueAxes.end())
      continue;
    const size_t axis = axisEntry->second;
    if (sizes.size() != offsets.size() || offsets.size() != strides.size() ||
        axis >= sizes.size() || sizes[axis] != plan.extent ||
        offsets[axis] != 0 || strides[axis] != 1)
      throw std::runtime_error("slice/subview does not match proven static form");
    sizes[axis] = tileSize;
    offsets[axis] = std::numeric_limits<int64_t>::min();
    operation.operands.push_back(offsetValue);
    operation.operandTypes.push_back("index");
    operation.attributes = SetDictionaryValue(
        operation.attributes, "static_sizes", PrintIntegerArray(sizes));
    operation.attributes = SetDictionaryValue(
        operation.attributes, "static_offsets", PrintIntegerArray(offsets));
  }

  std::vector<int> innerYieldValues;
  for (size_t index = 0; index < plan.yieldedTensors.size(); ++index) {
    const LoopPlan::YieldedTensor &yielded = plan.yieldedTensors[index];
    const GenericOperation producer =
        module.operations.at(static_cast<size_t>(yielded.producerId));
    if (producer.results.size() != 1)
      throw std::runtime_error("yielded HIVM producer no longer has one result");
    const auto fullType = ParseStaticShapedType(yielded.type);
    if (!fullType || yielded.axis >= fullType->shape.size())
      throw std::runtime_error("yielded tensor has no tiled axis");
    std::vector<int64_t> offsets(fullType->shape.size(), 0);
    std::vector<int64_t> sizes = fullType->shape;
    std::vector<int64_t> strides(fullType->shape.size(), 1);
    offsets[yielded.axis] = std::numeric_limits<int64_t>::min();
    sizes[yielded.axis] = tileSize;
    const int insert = rewriter.createOperation(
        innerLoop, innerRegion, innerBlock, "tensor.insert_slice",
        {yielded.type},
        {producer.results.front(), innerBlockArguments[index + 1],
         offsetValue},
        {producer.resultTypes.front(), yielded.type, "index"},
        "{operandSegmentSizes = array<i32: 1, 1, 1, 0, 0>, "
        "static_offsets = " + PrintIntegerArray(offsets) +
            ", static_sizes = " + PrintIntegerArray(sizes) +
            ", static_strides = " + PrintIntegerArray(strides) + "}",
        "{}");
    ApplyOperationSemantics(
        module.operations.at(static_cast<size_t>(insert)));
    // Each dummy Store's tiled loop carries this insert immediately after its
    // HIVM producer.  Fusing sibling loops preserves those relative positions;
    // RemoveDummyStore removes only the dummy Store itself.
    const auto &innerOperations =
        module.blocks.at(static_cast<size_t>(innerBlock)).operations;
    const auto producerPosition =
        std::find(innerOperations.begin(), innerOperations.end(), producer.id);
    if (producerPosition == innerOperations.end())
      throw std::runtime_error("yielded producer is absent from tiled loop");
    rewriter.insertToBlock(
        innerBlock,
        static_cast<size_t>(std::distance(innerOperations.begin(),
                                          producerPosition)) +
            1,
        insert);
    innerYieldValues.push_back(
        module.operations.at(static_cast<size_t>(insert)).results.front());
  }
  const int innerYield = rewriter.createOperation(
      innerLoop, innerRegion, innerBlock, "scf.yield", {}, innerYieldValues,
      innerResultTypes);
  rewriter.appendToBlock(innerBlock, innerYield);

  const GenericOperation &finishedInnerLoop =
      module.operations.at(static_cast<size_t>(innerLoop));
  std::set<int> externalConsumers;
  for (size_t resultIndex = 0; resultIndex < plan.yieldedTensors.size();
       ++resultIndex) {
    const GenericOperation &producer = module.operations.at(
        static_cast<size_t>(plan.yieldedTensors[resultIndex].producerId));
    const int oldResult = producer.results.front();
    const int newResult = finishedInnerLoop.results[resultIndex];
    for (GenericOperation &operation : module.operations) {
      if (operation.blockId == innerBlock)
        continue;
      for (size_t operandIndex = 0; operandIndex < operation.operands.size();
           ++operandIndex) {
        if (operation.operands[operandIndex] != oldResult)
          continue;
        rewriter.replaceOperand(operation.id, operandIndex, newResult);
        for (int &value : operation.dpsInputs)
          if (value == oldResult)
            value = newResult;
        for (int &value : operation.dpsInits)
          if (value == oldResult)
            value = newResult;
        externalConsumers.insert(operation.id);
      }
    }
  }
  for (int consumer : externalConsumers) {
    GenericOperation &operation =
        module.operations.at(static_cast<size_t>(consumer));
    if (operation.blockId == outerBlock && operation.id != terminator)
      MoveOperationBefore(module, operation.id, terminator);
  }
}

inline bool IsZeroUnitSubview(const GenericOperation &subview) {
  const auto offsets =
      ParseIntegerArray(IRDictionaryValue(subview.attributes, "static_offsets"));
  const auto strides =
      ParseIntegerArray(IRDictionaryValue(subview.attributes, "static_strides"));
  return !offsets.empty() && offsets.size() == strides.size() &&
         std::all_of(offsets.begin(), offsets.end(),
                     [](int64_t value) { return value == 0; }) &&
         std::all_of(strides.begin(), strides.end(),
                     [](int64_t value) { return value == 1; });
}

inline bool HasUnmodeledDynamicShrink(const GenericModule &module) {
  for (const GenericOperation &alloc : module.operations) {
    if (alloc.name != "memref.alloc" || alloc.results.size() != 1 ||
        alloc.resultTypes.size() != 1)
      continue;
    const auto users = Users(module, alloc.results.front());
    if (users.size() != 1 || users.front()->name != "memref.subview" ||
        !IsZeroUnitSubview(*users.front()))
      continue;
    const auto sizes = ParseIntegerArray(
        IRDictionaryValue(users.front()->attributes, "static_sizes"));
    if (!alloc.operands.empty() || !ParseStaticShapedType(alloc.resultTypes[0]) ||
        sizes.empty() ||
        std::any_of(sizes.begin(), sizes.end(),
                    [](int64_t value) { return value < 0; }))
      return true;
  }
  return false;
}

inline void ShrinkAllocWholeModule(GenericModule &module) {
  std::vector<int> erase;
  for (GenericOperation &alloc : module.operations) {
    if (alloc.name != "memref.alloc" || alloc.results.size() != 1 ||
        alloc.resultTypes.size() != 1)
      continue;
    const auto users = Users(module, alloc.results.front());
    if (users.size() != 1 || users.front()->name != "memref.subview" ||
        users.front()->results.size() != 1 ||
        users.front()->resultTypes.size() != 1 ||
        !alloc.operands.empty() || !IsZeroUnitSubview(*users.front()))
      continue;
    const auto sizes = ParseIntegerArray(
        IRDictionaryValue(users.front()->attributes, "static_sizes"));
    if (sizes.empty() ||
        std::any_of(sizes.begin(), sizes.end(),
                    [](int64_t value) { return value < 0; }))
      continue;
    const int replacement = alloc.results.front();
    const int oldSubview = users.front()->results.front();
    const int subviewId = users.front()->id;
    const int parentId = users.front()->parentId;
    const int regionId = users.front()->regionId;
    const int blockId = users.front()->blockId;
    alloc.resultTypes.front() = users.front()->resultTypes.front();
    ReplaceAllUses(module, oldSubview, replacement);
    MoveOperationBefore(module, alloc.id, subviewId);
    alloc.parentId = parentId;
    alloc.regionId = regionId;
    alloc.blockId = blockId;
    erase.push_back(subviewId);
  }
  for (int operation : erase)
    EraseOperationTree(module, operation);
}

inline void AddDiagnostic(StageResult &result, int operationId,
                          const std::string &operation,
                          const std::string &reason) {
  result.precision = Precision::Incomplete;
  result.diagnostics.push_back(
      {"TileCubeVectorLoop", "", operationId, operation, reason});
}

} // namespace tile_cube_vector_loop_detail

inline StageResult RunTileCubeVectorLoop(GenericModule module,
                                         unsigned vectorTripCount,
                                         unsigned cubeTripCount) {
  using namespace tile_cube_vector_loop_detail;
  StageResult result;
  result.module = module;
  if (vectorTripCount == 0 || cubeTripCount == 0) {
    AddDiagnostic(result, -1, "", "trip count must be greater than zero");
    return result;
  }
  // TileCubeVectorLoopPass returns before its LiftToTensor greedy rewrite
  // unless the module contains an scf.for/scope.scope carrying
  // hivm.loop_core_type.  Running the preprocessing unconditionally rewrites
  // ordinary memref loads even when CV/workspace management is disabled.
  const bool hasPipelinedLoops = std::any_of(
      module.operations.begin(), module.operations.end(),
      [](const GenericOperation &operation) {
        return (operation.name == "scf.for" ||
                operation.name == "scope.scope") &&
               HasLoopCoreAttribute(operation);
      });
  if (!hasPipelinedLoops)
    return result;
  try {
    module = LiftMemRefLoadsInLoop(std::move(module));
    module = CanonicalizeTileGreedyArtifacts(std::move(module));
    result.module = module;
  } catch (const std::exception &error) {
    AddDiagnostic(result, -1, "",
                  std::string("liftMemRefLoadsInLoop failed: ") + error.what());
    return result;
  }
  std::vector<int> candidates;
  for (const GenericOperation &operation : module.operations)
    if (HasLoopCoreAttribute(operation))
      candidates.push_back(operation.id);
  if (candidates.empty())
    return result;
  const auto spec = ReadTargetSpec(module);
  if (!spec) {
    AddDiagnostic(result, -1, "", "required NPU target spec is missing");
    return result;
  }
  if (HasUnmodeledDynamicShrink(module)) {
    AddDiagnostic(result, -1, "",
                  "dynamic mixed-size shrinkAlloc is not modeled exactly");
    return result;
  }
  for (size_t left = 0; left < candidates.size(); ++left)
    for (size_t right = left + 1; right < candidates.size(); ++right)
      if (IsAncestor(module, candidates[left], candidates[right]) ||
          IsAncestor(module, candidates[right], candidates[left])) {
        AddDiagnostic(result, candidates[right],
                      module.operations.at(static_cast<size_t>(candidates[right]))
                          .name,
                      "nested marked candidate loops are not modeled");
        return result;
      }

  std::vector<LoopPlan> plans;
  for (int candidate : candidates) {
    const GenericOperation &loop =
        module.operations.at(static_cast<size_t>(candidate));
    // Preload scopes use the real DimensionAnalyzer collector directly.  The
    // scf.for path below already carries a stricter, transformation-oriented
    // proof model; replacing that proof with this collection-only check would
    // incorrectly turn otherwise exact loop plans into diagnostics.
    if (loop.name == "scope.scope" &&
        CoreLoopKind(loop) == LoopKind::Vector &&
        VectorCollectionEmitsDiagnostic(module, loop, vectorTripCount,
                                        spec->ubAlignBits)) {
      AddDiagnostic(result, candidate, loop.name,
                    "Failed to collect vector loop tiling info");
      return result;
    }
    LoopPlan plan;
    std::string reason;
    if (!AnalyzeLoop(module, loop, *spec, vectorTripCount, cubeTripCount, plan,
                     reason)) {
      // collectLoopInfo deliberately ignores the LogicalResult returned by
      // collectVectorLoopInfo/collectCubeLoopInfo.  A rejected candidate may
      // emit an MLIR diagnostic, but it is not added to loopsToTile and the
      // pass continues successfully with the original loop.  Mirror that
      // behavior here instead of turning an unsupported/rejected candidate
      // into a model-wide blocker (notably scope.scope in preload mode).
      continue;
    }
    plans.push_back(plan);
  }

  try {
    GenericModule transformed = module;
    for (const LoopPlan &plan : plans)
      if (!plan.skip) {
        ApplyTiling(transformed, plan);
        // ApplyTiling intentionally leaves erased rewrite intermediates as
        // detached tombstones until the pass-wide canonicalize/compact step
        // below.  Validate only the compacted result: the strict validator's
        // every-operation-must-have-one-parent invariant is not meaningful in
        // the middle of that transaction.
      }
    // Vector fusion runs cleanup before each producer is fused, not after the
    // final producer. Producers are fused in reverse order, so the final Load
    // owns a fresh destination tile while earlier fused producers have already
    // shared their equivalent tiles. Do not let this pass-wide cleanup perform
    // an extra, source-absent CSE of that final Load destination.
    transformed = CanonicalizeTileGreedyArtifacts(
        std::move(transformed), /*preserveLoadOwnedDestinationTiles=*/true);
    ShrinkAllocWholeModule(transformed);
    transformed = CompactGenericModule(std::move(transformed));
    ValidateGenericModule(transformed);
    result.module = std::move(transformed);
  } catch (const std::exception &error) {
    result.module = std::move(module);
    AddDiagnostic(result, -1, "",
                  "transactional modeling failed: " + std::string(error.what()));
  }
  return result;
}

} // namespace cvub

#endif
