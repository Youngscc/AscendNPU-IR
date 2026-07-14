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
#include <utility>
#include <vector>

namespace cvub {
namespace tile_cube_vector_loop_detail {

enum class LoopKind { Vector, Cube };

struct ShapedType {
  std::string kind;
  std::vector<int64_t> shape;
  std::string tail;
};

inline std::optional<ShapedType> ParseStaticShapedType(const std::string &type) {
  static const std::regex shaped(
      R"(^(tensor|memref)<([0-9]+(?:x[0-9]+)*)x(.+)>$)");
  std::smatch match;
  if (!std::regex_match(type, match, shaped))
    return std::nullopt;
  ShapedType result;
  result.kind = match[1].str();
  result.tail = match[3].str();
  for (const std::string &dimension : splitTopLevel(match[2].str(), 'x'))
    result.shape.push_back(std::stoll(dimension));
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
  return static_cast<unsigned>(std::stoul(match[1].str()));
}

inline bool CheckedMultiply(int64_t &value, int64_t factor) {
  if (factor < 0 ||
      (factor != 0 && value > std::numeric_limits<int64_t>::max() / factor))
    return false;
  value *= factor;
  return true;
}

inline int64_t CeilDiv(int64_t extent, unsigned divisor) {
  const int64_t signedDivisor = static_cast<int64_t>(divisor);
  return extent / signedDivisor + (extent % signedDivisor == 0 ? 0 : 1);
}

inline std::vector<int64_t> ParseIntegerArray(const std::string &text) {
  if (text.size() < 2 || text.front() != '[' || text.back() != ']')
    return {};
  std::vector<int64_t> result;
  for (const std::string &item :
       splitTopLevel(text.substr(1, text.size() - 2)))
    result.push_back(std::stoll(item));
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

inline std::optional<LoopKind>
CoreLoopKind(const GenericOperation &operation) {
  const std::string core =
      IRDictionaryValue(operation.attributes, "hivm.loop_core_type");
  if (core == "#hivm.tcore_type<VECTOR>")
    return LoopKind::Vector;
  if (core == "#hivm.tcore_type<CUBE>")
    return LoopKind::Cube;
  return std::nullopt;
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

inline std::optional<int64_t>
CandidateExtent(const GenericModule &module, const GenericOperation &loop,
                LoopKind kind) {
  const std::string anchor =
      kind == LoopKind::Vector ? "hivm.hir.store" : "hivm.hir.fixpipe";
  for (int child : DirectChildren(module, loop)) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(child));
    if (operation.name != anchor || operation.operandTypes.empty())
      continue;
    const auto type = ParseStaticShapedType(operation.operandTypes.front());
    if (!type || type->shape.empty())
      return std::nullopt;
    return type->shape.back();
  }
  return int64_t{0};
}

inline bool IsKnownVectorRollback(const GenericModule &module,
                                  const GenericOperation &loop,
                                  int64_t extent, unsigned tripCount) {
  const int64_t tile = CeilDiv(extent, tripCount);
  for (int child : DirectChildren(module, loop)) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(child));
    for (const std::string &typeText : operation.resultTypes) {
      const auto type = ParseStaticShapedType(typeText);
      if (!type || std::find(type->shape.begin(), type->shape.end(), extent) ==
                       type->shape.end())
        continue;
      const unsigned bitWidth = ElementBitWidth(type->tail);
      if (bitWidth == 0)
        return false;
      int64_t bits = static_cast<int64_t>(bitWidth);
      for (int64_t dimension : type->shape)
        if (!CheckedMultiply(bits, dimension == extent ? tile : dimension))
          return false;
      const int64_t alignment = bitWidth == 1 ? 8 : 256;
      if (bits % alignment != 0)
        return true;
    }
  }
  return false;
}

inline bool PhysicalSizesAreChecked(const GenericModule &module,
                                    const GenericOperation &loop,
                                    int64_t extent, unsigned tripCount) {
  const int64_t tile = CeilDiv(extent, tripCount);
  for (int child : DirectChildren(module, loop)) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(child));
    for (const std::string &typeText : operation.resultTypes) {
      const auto type = ParseStaticShapedType(typeText);
      if (!type || std::find(type->shape.begin(), type->shape.end(), extent) ==
                       type->shape.end())
        continue;
      int64_t bits = static_cast<int64_t>(ElementBitWidth(type->tail));
      if (bits == 0)
        return false;
      for (int64_t dimension : type->shape)
        if (!CheckedMultiply(bits, dimension == extent ? tile : dimension))
          return false;
    }
  }
  return true;
}

inline bool IsKnownCubeSkip(const GenericModule &module,
                            const GenericOperation &loop, int64_t extent) {
  constexpr int64_t l0cSizeBits = 1048576LL * 8LL;
  for (int child : DirectChildren(module, loop)) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(child));
    if (operation.name == "hivm.hir.mmadL1" && operation.operands.size() > 2) {
      const GenericOperation *flag = Definition(module, operation.operands[2]);
      if (flag == nullptr || flag->name != "arith.constant")
        return true;
    }
    if (operation.name != "hivm.hir.fixpipe" ||
        operation.operandTypes.empty())
      continue;
    const auto type = ParseStaticShapedType(operation.operandTypes.front());
    if (!type)
      return false;
    const unsigned bitWidth = ElementBitWidth(type->tail);
    if (bitWidth == 0)
      return false;
    int64_t bits = static_cast<int64_t>(bitWidth);
    for (int64_t dimension : type->shape)
      bits *= dimension;
    return bits <= l0cSizeBits && extent > 0;
  }
  return true;
}

inline bool IsSupportedBodyOperation(const std::string &name,
                                     LoopKind kind) {
  static const std::set<std::string> common = {
      "tensor.empty", "tensor.extract_slice", "memref.subview",
      "hivm.hir.load", "hivm.hir.store"};
  if (common.count(name) != 0)
    return true;
  if (kind == LoopKind::Vector)
    return name == "hivm.hir.vadd";
  return name == "hivm.hir.mmadL1" || name == "hivm.hir.fixpipe";
}

inline std::optional<std::string>
UnsupportedReason(const GenericModule &module, const GenericOperation &loop,
                  LoopKind kind, int64_t extent) {
  if (loop.name != "scf.for")
    return "scope.scope tiling is a real successful pattern but is not modeled";
  const std::vector<int> children = DirectChildren(module, loop);
  if (children.empty())
    return "loop does not have one supported region and block";
  const GenericOperation &terminator =
      module.operations.at(static_cast<size_t>(children.back()));
  if (terminator.name != "scf.yield" || !terminator.operands.empty() ||
      !loop.results.empty())
    return "result-carrying tiled loops are not modeled";
  if (extent <= 0)
    return "no statically shaped tiling anchor was found";
  for (size_t index = 0; index + 1 < children.size(); ++index) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(children[index]));
    if (!operation.regions.empty())
      return "nested region producer tiling is not modeled";
    if (!IsSupportedBodyOperation(operation.name, kind))
      return "successful producer pattern is not modeled: " + operation.name;
  }
  return std::nullopt;
}

inline bool RewriteType(std::string &typeText, int64_t extent,
                        int64_t tileSize) {
  auto type = ParseStaticShapedType(typeText);
  if (!type)
    return false;
  bool changed = false;
  for (int64_t &dimension : type->shape)
    if (dimension == extent) {
      dimension = tileSize;
      changed = true;
    }
  if (changed)
    typeText = PrintShapedType(*type);
  return changed;
}

inline bool HasSingleEligibleSubviewUser(const GenericModule &module,
                                         int allocationValue) {
  const GenericOperation *user = nullptr;
  for (const GenericOperation &operation : module.operations) {
    const auto uses = static_cast<size_t>(std::count(
        operation.operands.begin(), operation.operands.end(), allocationValue));
    if (uses == 0)
      continue;
    if (uses != 1 || user != nullptr)
      return false;
    user = &operation;
  }
  if (user == nullptr || user->name != "memref.subview")
    return false;
  const std::vector<int64_t> offsets = ParseIntegerArray(
      IRDictionaryValue(user->attributes, "static_offsets"));
  const std::vector<int64_t> strides = ParseIntegerArray(
      IRDictionaryValue(user->attributes, "static_strides"));
  return !offsets.empty() && !strides.empty() &&
         std::all_of(offsets.begin(), offsets.end(),
                     [](int64_t value) { return value == 0; }) &&
         std::all_of(strides.begin(), strides.end(),
                     [](int64_t value) { return value == 1; });
}

inline std::set<int> AffectedOperations(const GenericModule &module,
                                        const std::vector<int> &children) {
  std::set<int> affected(children.begin(), children.end() - 1);
  std::vector<int> worklist(affected.begin(), affected.end());
  while (!worklist.empty()) {
    const int operationId = worklist.back();
    worklist.pop_back();
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    for (int operand : operation.operands) {
      const GenericOperation *definition = Definition(module, operand);
      if (definition == nullptr ||
          (definition->name != "tensor.empty" &&
           definition->name != "memref.alloc" &&
           definition->name != "memref.subview"))
        continue;
      if (affected.insert(definition->id).second)
        worklist.push_back(definition->id);
    }
  }
  return affected;
}

inline void RewriteTypes(GenericModule &module, const std::set<int> &affected,
                         int64_t extent, int64_t tileSize) {
  for (GenericOperation &operation : module.operations) {
    if (affected.count(operation.id) == 0)
      continue;
    const bool eligibleAllocation =
        operation.name != "memref.alloc" || operation.results.size() != 1 ||
        HasSingleEligibleSubviewUser(module, operation.results.front());
    if (eligibleAllocation)
      for (std::string &type : operation.resultTypes)
        RewriteType(type, extent, tileSize);
    for (std::string &type : operation.operandTypes)
      RewriteType(type, extent, tileSize);
  }
}

inline void RewriteSlices(GenericModule &module, const std::set<int> &affected,
                          int64_t extent, int64_t tileSize,
                          int dynamicOffset) {
  constexpr int64_t dynamic = std::numeric_limits<int64_t>::min();
  for (GenericOperation &operation : module.operations) {
    if (affected.count(operation.id) == 0)
      continue;
    if (operation.name != "tensor.extract_slice" &&
        operation.name != "memref.subview")
      continue;
    std::vector<int64_t> sizes = ParseIntegerArray(
        IRDictionaryValue(operation.attributes, "static_sizes"));
    std::vector<int64_t> offsets = ParseIntegerArray(
        IRDictionaryValue(operation.attributes, "static_offsets"));
    if (sizes.empty() || offsets.size() != sizes.size())
      continue;
    for (size_t index = 0; index < sizes.size(); ++index) {
      if (sizes[index] != extent)
        continue;
      sizes[index] = tileSize;
      if (operation.name == "tensor.extract_slice") {
        offsets[index] = dynamic;
        operation.operands.push_back(dynamicOffset);
        operation.operandTypes.push_back("index");
      }
    }
    operation.attributes = SetDictionaryValue(
        operation.attributes, "static_sizes", PrintIntegerArray(sizes));
    operation.attributes = SetDictionaryValue(
        operation.attributes, "static_offsets", PrintIntegerArray(offsets));
  }
}

inline void ApplyTiling(GenericModule &module, int loopId, int64_t extent,
                        unsigned tripCount) {
  const GenericOperation loopSnapshot =
      module.operations.at(static_cast<size_t>(loopId));
  const int64_t tileSize = CeilDiv(extent, tripCount);
  const int outerRegion = loopSnapshot.regions.front();
  const int outerBlock =
      module.regions.at(static_cast<size_t>(outerRegion)).blocks.front();
  const std::vector<int> children =
      module.blocks.at(static_cast<size_t>(outerBlock)).operations;
  const std::set<int> affected = AffectedOperations(module, children);
  const int terminator = children.back();

  GenericRewriter rewriter(module);
  const int zero = rewriter.createOperation(
      loopId, outerRegion, outerBlock, "arith.constant", {"index"}, {}, {},
      "", "{value = 0 : index}");
  const int upper = rewriter.createOperation(
      loopId, outerRegion, outerBlock, "arith.constant", {"index"}, {}, {},
      "", "{value = " + std::to_string(tripCount) + " : index}");
  const int step = rewriter.createOperation(
      loopId, outerRegion, outerBlock, "arith.constant", {"index"}, {}, {},
      "", "{value = 1 : index}");
  const int innerLoop = rewriter.createOperation(
      loopId, outerRegion, outerBlock, "scf.for", {},
      {module.operations.at(static_cast<size_t>(zero)).results.front(),
       module.operations.at(static_cast<size_t>(upper)).results.front(),
       module.operations.at(static_cast<size_t>(step)).results.front()},
      {"index", "index", "index"});
  const int innerRegion = rewriter.createRegion(innerLoop);
  const int innerBlock = rewriter.createBlock(innerRegion, {"index"});
  const int innerIV =
      module.blocks.at(static_cast<size_t>(innerBlock)).arguments.front();
  const int offset = rewriter.createOperation(
      innerLoop, innerRegion, innerBlock, "affine.apply", {"index"},
      {innerIV}, {"index"}, "",
      "{map = affine_map<(d0) -> (d0 * " + std::to_string(tileSize) + ")>}");
  rewriter.appendToBlock(innerBlock, offset);

  for (size_t index = 0; index + 1 < children.size(); ++index) {
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

  const auto terminatorPosition = [&]() {
    const auto &operations =
        module.blocks.at(static_cast<size_t>(outerBlock)).operations;
    return static_cast<size_t>(std::distance(
        operations.begin(),
        std::find(operations.begin(), operations.end(), terminator)));
  };
  size_t position = terminatorPosition();
  rewriter.insertToBlock(outerBlock, position++, zero);
  rewriter.insertToBlock(outerBlock, position++, upper);
  rewriter.insertToBlock(outerBlock, position++, step);
  rewriter.insertToBlock(outerBlock, position, innerLoop);

  RewriteTypes(module, affected, extent, tileSize);
  RewriteSlices(
      module, affected, extent, tileSize,
      module.operations.at(static_cast<size_t>(offset)).results.front());
}

inline void AddDiagnostic(StageResult &result, const GenericOperation &loop,
                          const std::string &reason) {
  result.precision = Precision::Incomplete;
  result.diagnostics.push_back({"TileCubeVectorLoop", "", loop.id,
                                loop.name, reason});
}

} // namespace tile_cube_vector_loop_detail

inline StageResult RunTileCubeVectorLoop(GenericModule module,
                                         unsigned vectorTripCount,
                                         unsigned cubeTripCount) {
  using namespace tile_cube_vector_loop_detail;
  StageResult result;
  result.module = std::move(module);
  if (vectorTripCount == 0 || cubeTripCount == 0) {
    result.precision = Precision::Incomplete;
    result.diagnostics.push_back({"TileCubeVectorLoop", "", -1, "",
                                  "trip count must be greater than zero"});
    return result;
  }

  const size_t originalOperationCount = result.module.operations.size();
  bool transformed = false;
  for (size_t index = 0; index < originalOperationCount; ++index) {
    const GenericOperation snapshot = result.module.operations[index];
    const auto kind = CoreLoopKind(snapshot);
    if (!kind)
      continue;
    const unsigned tripCount = *kind == LoopKind::Vector ? vectorTripCount
                                                         : cubeTripCount;
    if (tripCount == 1)
      continue;
    try {
      const auto extent = CandidateExtent(result.module, snapshot, *kind);
      if (!extent) {
        AddDiagnostic(result, snapshot,
                      "dynamic or non-static tiling anchor is not modeled");
        continue;
      }
      if (*extent > 0 && !PhysicalSizesAreChecked(
                             result.module, snapshot, *extent, tripCount)) {
        AddDiagnostic(result, snapshot,
                      "physical size is unresolved or overflows int64");
        continue;
      }
      if (*kind == LoopKind::Vector && *extent > 0 &&
          IsKnownVectorRollback(result.module, snapshot, *extent, tripCount))
        continue;
      if (*kind == LoopKind::Cube &&
          IsKnownCubeSkip(result.module, snapshot, *extent))
        continue;
      if (const auto reason =
              UnsupportedReason(result.module, snapshot, *kind, *extent)) {
        AddDiagnostic(result, snapshot, *reason);
        continue;
      }

      GenericModule candidate = result.module;
      ApplyTiling(candidate, snapshot.id, *extent, tripCount);
      ValidateGenericModule(candidate);
      result.module = std::move(candidate);
      transformed = true;
    } catch (const std::exception &error) {
      AddDiagnostic(result, snapshot,
                    "analysis or transactional rewrite was not modeled exactly: " +
                        std::string(error.what()));
    }
  }
  if (transformed) {
    try {
      GenericModule compacted = CompactGenericModule(result.module);
      ValidateGenericModule(compacted);
      result.module = std::move(compacted);
    } catch (const std::exception &error) {
      result.precision = Precision::Incomplete;
      result.diagnostics.push_back(
          {"TileCubeVectorLoop", "", -1, "",
           "final compaction was not modeled exactly: " +
               std::string(error.what())});
    }
  }
  return result;
}

} // namespace cvub

#endif
