#ifndef CVPIPELINE_UB_MODEL_CPP_POST_CVPIPELINE_TILE_CUBE_VECTOR_LOOP_HPP
#define CVPIPELINE_UB_MODEL_CPP_POST_CVPIPELINE_TILE_CUBE_VECTOR_LOOP_HPP

#include "ir_utils.hpp"
#include "types.hpp"

#include <cstdint>
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
  int loopId = -1;
  LoopKind kind = LoopKind::Vector;
  int64_t extent = 0;
  unsigned tripCount = 1;
  bool skip = false;
  std::map<int, size_t> valueAxes;
  std::set<int> producerClosure;
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
  if (text.size() < 2 || text.front() != '[' || text.back() != ']')
    return {};
  std::vector<int64_t> result;
  try {
    for (const std::string &item :
         splitTopLevel(text.substr(1, text.size() - 2)))
      result.push_back(std::stoll(item));
  } catch (const std::exception &) {
    return {};
  }
  return result;
}

inline std::string PrintIntegerArray(const std::vector<int64_t> &values) {
  std::string result = "[";
  for (size_t index = 0; index < values.size(); ++index) {
    if (index != 0)
      result += ", ";
    result += std::to_string(values[index]);
  }
  return result + "]";
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

inline std::vector<const GenericOperation *>
Users(const GenericModule &module, int value) {
  std::vector<const GenericOperation *> users;
  for (const GenericOperation &operation : module.operations)
    if (std::find(operation.operands.begin(), operation.operands.end(), value) !=
        operation.operands.end())
      users.push_back(&operation);
  return users;
}

inline bool HasUnmodeledLiftPattern(const GenericModule &module) {
  for (const GenericOperation &operation : module.operations) {
    if (operation.name == "hivm.hir.load" && operation.operandTypes.size() > 1 &&
        operation.operandTypes.back().rfind("memref<", 0) == 0)
      return true;
    if (operation.name != "memref.alloc" || operation.results.size() != 1)
      continue;
    const auto users = Users(module, operation.results.front());
    if (users.size() == 1 && users.front()->name == "bufferization.to_tensor")
      return true;
  }
  return false;
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
    return operation.name == "hivm.hir.vadd";
  return operation.name == "hivm.hir.mmadL1";
}

inline bool CollectProducerClosure(const GenericModule &module,
                                   const GenericOperation &loop, int value,
                                   LoopKind kind, std::set<int> &closure,
                                   std::string &reason) {
  const GenericOperation *definition = Definition(module, value);
  if (definition == nullptr || !IsAncestor(module, loop.id, definition->id))
    return true;
  if (!IsSupportedProducer(*definition, kind)) {
    reason = "anchor dependency contains an operation whose axis semantics "
             "are not modeled";
    return false;
  }
  if (!closure.insert(definition->id).second)
    return true;
  for (int operand : definition->operands)
    if (!CollectProducerClosure(module, loop, operand, kind, closure, reason))
      return false;
  return true;
}

inline std::optional<size_t> UniqueParallelAxis(const ShapedType &type) {
  std::optional<size_t> axis;
  for (size_t index = 0; index < type.shape.size(); ++index) {
    if (type.shape[index] <= 1)
      continue;
    if (axis)
      return std::nullopt;
    axis = index;
  }
  return axis;
}

inline bool HasInLoopLoadDependency(const GenericModule &module,
                                    const GenericOperation &loop, int value) {
  std::set<int> visited;
  std::vector<int> worklist{value};
  while (!worklist.empty()) {
    const GenericOperation *definition = Definition(module, worklist.back());
    worklist.pop_back();
    if (definition == nullptr || !IsAncestor(module, loop.id, definition->id) ||
        !visited.insert(definition->id).second)
      continue;
    if (definition->name == "hivm.hir.load")
      return true;
    worklist.insert(worklist.end(), definition->operands.begin(),
                    definition->operands.end());
  }
  return false;
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
                      [](int64_t value) { return value < 0; }) &&
         std::none_of(sizes.begin(), sizes.end(),
                      [](int64_t value) { return value < 0; }) &&
         std::all_of(strides.begin(), strides.end(),
                     [](int64_t value) { return value == 1; });
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
                            int64_t extent, std::string &reason) {
  const auto type = ParseStaticShapedType(typeText);
  if (!type || axis >= type->shape.size() || type->shape[axis] != extent) {
    reason = "producer dependency has no provable tiling dimension for value " +
             std::to_string(value) + " type " + typeText + " axis " +
             std::to_string(axis);
    return false;
  }
  for (size_t index = 0; index < type->shape.size(); ++index)
    if (index != axis && type->shape[index] == extent) {
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

inline bool TypeUsesAxisExactly(const ShapedType &type, int64_t extent,
                                size_t axis) {
  size_t occurrences = 0;
  for (size_t index = 0; index < type.shape.size(); ++index)
    if (type.shape[index] == extent) {
      ++occurrences;
      if (index != axis)
        return false;
    }
  return occurrences == 1;
}

inline bool RecordIdentityDependencyAxis(const GenericModule &module,
                                         const GenericOperation &loop,
                                         LoopPlan &plan, int value,
                                         const std::string &typeText,
                                         size_t axis, int64_t extent,
                                         std::string &reason) {
  if (!RecordValueAxis(module, plan, value, typeText, axis, extent, reason))
    return false;
  const GenericOperation *definition = Definition(module, value);
  if (definition == nullptr || !IsAncestor(module, loop.id, definition->id) ||
      definition->name == "tensor.empty")
    return true;
  if (definition->name != "tensor.extract_slice" &&
      definition->name != "memref.subview" &&
      definition->name != "hivm.hir.load" &&
      definition->name != "hivm.hir.vadd") {
    reason = "dependency axis equivalence is not explicitly modeled";
    return false;
  }
  for (size_t index = 0; index < definition->operands.size(); ++index) {
    if (!ParseStaticShapedType(definition->operandTypes[index]))
      continue;
    if (!RecordIdentityDependencyAxis(module, loop, plan,
                                      definition->operands[index],
                                      definition->operandTypes[index], axis,
                                      extent, reason))
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
    for (size_t index = 0; index < operation.results.size(); ++index) {
      const auto axis = plan.valueAxes.find(operation.results[index]);
      if (axis == plan.valueAxes.end())
        continue;
      const std::string *typeText = &operation.resultTypes[index];
      const auto type = ParseStaticShapedType(*typeText);
      if (!type || axis->second >= type->shape.size())
        return false;
      ShapedType tiled = *type;
      tiled.shape[axis->second] /= static_cast<int64_t>(tripCount);
      const auto bits = StaticSizeBits(tiled);
      if (!bits) {
        reason = "physical size overflows or has unknown element width";
        return false;
      }
      const int64_t actualAlign = ElementBitWidth(tiled.tail) == 1 ? 8 : alignBits;
      if (*bits % actualAlign != 0)
        alignmentRollback = true;
    }
  }
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
  if (loop.name != "scf.for") {
    reason = "only the proven scf.for tiling form is modeled";
    return false;
  }
  if (!HasUnitIterationDomain(module, loop)) {
    reason = "the narrow exact model requires the compiler unit-trip wrapper";
    return false;
  }
  const std::vector<int> children = DirectChildren(module, loop);
  if (children.empty()) {
    reason = "candidate loop must have one region and one block";
    return false;
  }
  const GenericOperation &terminator =
      module.operations.at(static_cast<size_t>(children.back()));
  if (terminator.name != "scf.yield" || !terminator.operands.empty() ||
      !loop.results.empty()) {
    reason = "result-carrying or malformed candidate loops are not modeled";
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
  if (plan.tripCount == 1) {
    plan.skip = true;
    return true;
  }

  const std::string anchorName =
      *kind == LoopKind::Vector ? "hivm.hir.store" : "hivm.hir.fixpipe";
  std::vector<const GenericOperation *> anchors;
  for (int child : children) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(child));
    if (operation.name == anchorName)
      anchors.push_back(&operation);
  }
  if (anchors.size() != 1) {
    if (*kind == LoopKind::Cube && anchors.empty()) {
      plan.skip = true;
      return true;
    }
    reason = "exactly one tiling anchor is required by the narrow exact model";
    return false;
  }
  if (anchors.front()->operandTypes.empty()) {
    reason = "tiling anchor has no shaped source";
    return false;
  }
  const auto sourceType =
      ParseStaticShapedType(anchors.front()->operandTypes.front());
  if (!sourceType || sourceType->shape.size() != 2) {
    reason = "DimensionAnalyzer tiling axis is not uniquely provable";
    return false;
  }

  if (*kind == LoopKind::Vector) {
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
    if (!CollectProducerClosure(module, loop, anchors.front()->operands.front(),
                                *kind, plan.producerClosure, reason))
      return false;
    plan.producerClosure.insert(anchors.front()->id);
    if (!RecordIdentityDependencyAxis(
            module, loop, plan, anchors.front()->operands.front(),
            anchors.front()->operandTypes.front(), *axis, plan.extent, reason))
      return false;
    if (anchors.front()->operands.size() < 2 ||
        anchors.front()->operandTypes.size() < 2 ||
        !HasStaticBoundaryView(module, anchors.front()->operands[1]) ||
        !RecordValueAxis(module, plan, anchors.front()->operands[1],
                         anchors.front()->operandTypes[1], *axis, plan.extent,
                         reason)) {
      if (reason.empty())
        reason = "store destination axis is not equivalent";
      return false;
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
    const GenericOperation *accumulate = Definition(module, mmad->operands[2]);
    if (accumulate == nullptr || accumulate->name != "arith.constant") {
      plan.skip = true;
      return true;
    }
    if (mmad->results.size() != 1 || mmad->resultTypes.size() != 1 ||
        mmad->operands.size() != 4 || mmad->operandTypes.size() != 4 ||
        anchors.front()->operands.size() < 2 ||
        anchors.front()->operandTypes.size() < 2) {
      reason = "MmadL1/fixpipe branch arity is outside the exact model";
      return false;
    }
    const auto destinationType =
        ParseStaticShapedType(anchors.front()->operandTypes[1]);
    if (!destinationType || destinationType->shape.size() != 2 ||
        destinationType->shape != sourceType->shape ||
        !HasStaticBoundaryView(module, anchors.front()->operands[1]) ||
        mmad->attributes.find("transpose") != std::string::npos) {
      reason = "fixpipe source and destination axes are not equivalent";
      return false;
    }
    const bool aInside =
        HasInLoopLoadDependency(module, loop, mmad->operands[0]);
    const bool bInside =
        HasInLoopLoadDependency(module, loop, mmad->operands[1]);
    std::optional<size_t> axis;
    if (aInside != bInside)
      axis = aInside ? 0U : 1U;
    else if (!aInside)
      axis = UniqueParallelAxis(*sourceType);
    if (!axis) {
      reason = "Cube M/N axis lacks unique DimensionAnalyzer load evidence";
      return false;
    }
    plan.extent = sourceType->shape[*axis];
    if (plan.extent % static_cast<int64_t>(plan.tripCount) != 0) {
      reason = "non-divisible tiling requires a dynamic remainder model";
      return false;
    }
    if (!CollectProducerClosure(module, loop, anchors.front()->operands.front(),
                                *kind, plan.producerClosure, reason))
      return false;
    plan.producerClosure.insert(anchors.front()->id);
    if (!RecordValueAxis(module, plan, mmad->results[0], mmad->resultTypes[0],
                         *axis, plan.extent, reason) ||
        !RecordValueAxis(module, plan, mmad->operands[3],
                         mmad->operandTypes[3], *axis, plan.extent, reason) ||
        !RecordValueAxis(module, plan, anchors.front()->operands[1],
                         anchors.front()->operandTypes[1], *axis, plan.extent,
                         reason))
      return false;
    const size_t matrixOperand = *axis == 0 ? 0 : 1;
    if (!RecordIdentityDependencyAxis(
            module, loop, plan, mmad->operands[matrixOperand],
            mmad->operandTypes[matrixOperand], *axis, plan.extent, reason))
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
  return true;
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
  const int zero = rewriter.createOperation(
      plan.loopId, outerRegion, outerBlock, "arith.constant", {"index"}, {}, {},
      "", "{value = 0 : index}");
  const int upper = rewriter.createOperation(
      plan.loopId, outerRegion, outerBlock, "arith.constant", {"index"}, {}, {},
      "", "{value = " + std::to_string(plan.tripCount) + " : index}");
  const int step = rewriter.createOperation(
      plan.loopId, outerRegion, outerBlock, "arith.constant", {"index"}, {}, {},
      "", "{value = 1 : index}");
  const int innerLoop = rewriter.createOperation(
      plan.loopId, outerRegion, outerBlock, "scf.for", {},
      {module.operations.at(static_cast<size_t>(zero)).results.front(),
       module.operations.at(static_cast<size_t>(upper)).results.front(),
       module.operations.at(static_cast<size_t>(step)).results.front()},
      {"index", "index", "index"});
  const int innerRegion = rewriter.createRegion(innerLoop);
  const int innerBlock = rewriter.createBlock(innerRegion, {"index"});
  const int innerIV =
      module.blocks.at(static_cast<size_t>(innerBlock)).arguments.front();
  const int offset = rewriter.createOperation(
      innerLoop, innerRegion, innerBlock, "affine.apply", {"index"}, {innerIV},
      {"index"}, "", "{map = affine_map<(d0) -> (d0 * " +
                              std::to_string(tileSize) + ")>}");
  rewriter.appendToBlock(innerBlock, offset);

  for (size_t index = 0; index + 1 < children.size(); ++index) {
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
  const int innerYield = rewriter.createOperation(
      innerLoop, innerRegion, innerBlock, "scf.yield", {});
  rewriter.appendToBlock(innerBlock, innerYield);

  const auto &outerOperations =
      module.blocks.at(static_cast<size_t>(outerBlock)).operations;
  const auto found =
      std::find(outerOperations.begin(), outerOperations.end(), terminator);
  size_t position =
      static_cast<size_t>(std::distance(outerOperations.begin(), found));
  rewriter.insertToBlock(outerBlock, position++, zero);
  rewriter.insertToBlock(outerBlock, position++, upper);
  rewriter.insertToBlock(outerBlock, position++, step);
  rewriter.insertToBlock(outerBlock, position, innerLoop);

  std::map<int, int> boundaryTiles;
  for (int operationId : affected) {
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
      int tiledValue = -1;
      const auto cached = boundaryTiles.find(original);
      if (cached != boundaryTiles.end()) {
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
        const std::string resultType = PrintShapedType(tiledType);
        const std::string sliceName =
            type->kind == "memref" ? "memref.subview" : "tensor.extract_slice";
        const int slice = rewriter.createOperation(
            innerLoop, innerRegion, innerBlock, sliceName, {resultType},
            {original,
             module.operations.at(static_cast<size_t>(offset)).results.front()},
            {originalType, "index"}, "",
            "{static_offsets = " + PrintIntegerArray(offsets) +
                ", static_sizes = " + PrintIntegerArray(sizes) +
                ", static_strides = " + PrintIntegerArray(strides) + "}");
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
        boundaryTiles[original] = tiledValue;
      }
      GenericOperation &updated =
          module.operations.at(static_cast<size_t>(operationId));
      updated.operands[index] = tiledValue;
      RewriteType(updated.operandTypes[index], plan.extent, tileSize,
                  axisEntry->second);
    }
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
        operation.name != "memref.subview")
      continue;
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
    operation.operands.push_back(
        module.operations.at(static_cast<size_t>(offset)).results.front());
    operation.operandTypes.push_back("index");
    operation.attributes = SetDictionaryValue(
        operation.attributes, "static_sizes", PrintIntegerArray(sizes));
    operation.attributes = SetDictionaryValue(
        operation.attributes, "static_offsets", PrintIntegerArray(offsets));
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
  const auto spec = ReadTargetSpec(module);
  if (!spec) {
    AddDiagnostic(result, -1, "", "required NPU target spec is missing");
    return result;
  }
  std::vector<int> candidates;
  for (const GenericOperation &operation : module.operations)
    if (HasLoopCoreAttribute(operation))
      candidates.push_back(operation.id);
  if (candidates.empty())
    return result;
  if (HasUnmodeledLiftPattern(module)) {
    AddDiagnostic(result, -1, "",
                  "liftMemRefLoadsInLoop preprocessing would change the module");
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
    LoopPlan plan;
    std::string reason;
    if (!AnalyzeLoop(module, loop, *spec, vectorTripCount, cubeTripCount, plan,
                     reason)) {
      AddDiagnostic(result, loop.id, loop.name, reason);
      return result;
    }
    plans.push_back(plan);
  }

  try {
    GenericModule transformed = module;
    for (const LoopPlan &plan : plans)
      if (!plan.skip)
        ApplyTiling(transformed, plan);
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
