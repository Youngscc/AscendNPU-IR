#ifndef CVPIPELINE_UB_MODEL_CPP_CANONICALIZATION_HIVM_PIPELINE_HPP
#define CVPIPELINE_UB_MODEL_CPP_CANONICALIZATION_HIVM_PIPELINE_HPP

#include "../ir/generic_analysis.hpp"
#include "../ir/generic_rewriter.hpp"
#include "affine_min_max_canonicalization.hpp"
#include "fold_tensor_empty.hpp"
#include "../analysis/hivm_dimension_analyzer.hpp"
#include "../ir/operation_folder.hpp"

namespace cvub {

constexpr int64_t kCanonicalizationDynamicIndex =
    std::numeric_limits<int64_t>::min();

inline std::string CanonicalizationI64Array(
    const std::vector<int64_t> &values) {
  std::ostringstream output;
  output << "array<i64:";
  for (size_t index = 0; index < values.size(); ++index)
    output << (index == 0 ? " " : ", ") << values[index];
  return output.str() + ">";
}

inline std::string SetCanonicalizationDictionaryValue(
    const std::string &dictionary, const std::string &name,
    const std::string &value) {
  if (dictionary.size() < 2 || dictionary.front() != '{' ||
      dictionary.back() != '}')
    throw std::runtime_error(
        "canonicalize-ext: malformed operation properties");
  std::vector<std::string> entries =
      splitTopLevel(dictionary.substr(1, dictionary.size() - 2));
  bool replaced = false;
  for (std::string &entry : entries) {
    const size_t equal = entry.find('=');
    if (equal != std::string::npos && trim(entry.substr(0, equal)) == name) {
      entry = name + " = " + value;
      replaced = true;
    }
  }
  if (!replaced)
    entries.push_back(name + " = " + value);
  std::string result = "{";
  for (size_t index = 0; index < entries.size(); ++index) {
    if (index != 0)
      result += ", ";
    result += trim(entries[index]);
  }
  return result + "}";
}

inline void SetCanonicalizationProperty(GenericOperation &operation,
                                        const std::string &name,
                                        const std::string &value) {
  operation.properties = SetCanonicalizationDictionaryValue(
      operation.properties, name, value);
  // GenericOperation::attributes is the parser's merged property/attribute
  // view. Keep its inherent-property entries in sync with MLIR's single
  // operation state after a rewrite updates a property.
  operation.attributes = SetCanonicalizationDictionaryValue(
      operation.attributes, name, value);
}

inline std::string ReplaceCanonicalizationShape(
    const std::string &type,
    const std::vector<std::optional<int64_t>> &shape) {
  const size_t prefix =
      startsWith(type, "tensor<") || startsWith(type, "memref<")
          ? 7
          : std::string::npos;
  if (prefix == std::string::npos)
    return type;
  const size_t close = findBalancedClose(type, prefix - 1);
  if (close == std::string::npos)
    throw std::runtime_error("canonicalize-ext: malformed shaped type");
  std::vector<std::string> fields =
      splitTopLevel(type.substr(prefix, close - prefix));
  if (fields.empty())
    return type;
  std::vector<std::string> shapeAndElement =
      detail::SplitTypeTextAtTopLevel(fields.front(), 'x');
  if (shapeAndElement.size() != shape.size() + 1)
    throw std::runtime_error("canonicalize-ext: shaped type rank mismatch");
  std::string rebuilt;
  for (const std::optional<int64_t> &extent : shape) {
    if (!rebuilt.empty())
      rebuilt += 'x';
    rebuilt += extent ? std::to_string(*extent) : "?";
  }
  if (!rebuilt.empty())
    rebuilt += 'x';
  rebuilt += shapeAndElement.back();
  fields.front() = rebuilt;
  std::string result = type.substr(0, prefix);
  for (size_t index = 0; index < fields.size(); ++index) {
    if (index != 0)
      result += ", ";
    result += fields[index];
  }
  return result + type.substr(close);
}

inline std::optional<int64_t> CanonicalizationIndexConstant(
    const GenericOperation &operation) {
  if (operation.name != "arith.constant" || operation.results.size() != 1 ||
      operation.resultTypes.size() != 1 ||
      operation.resultTypes.front() != "index")
    return std::nullopt;
  std::string literal = FindDictionaryValue(operation.properties, "value");
  if (literal.empty())
    literal = FindDictionaryValue(operation.attributes, "value");
  const size_t typed = literal.find(':');
  if (typed != std::string::npos)
    literal.resize(typed);
  try {
    return std::stoll(trim(literal));
  } catch (const std::exception &) {
    return std::nullopt;
  }
}

inline bool CanonicalizationExtentCompatible(
    const std::optional<int64_t> &oldExtent,
    const std::optional<int64_t> &newExtent) {
  return !oldExtent || !newExtent || *oldExtent == *newExtent;
}

// Mirrors computeRankReductionMask used by the tensor/memref return type
// canonicalizers. A dimension can only disappear when its folded size is one.
inline std::optional<std::vector<size_t>> CanonicalizationKeptDimensions(
    const std::vector<std::optional<int64_t>> &oldShape,
    const std::vector<std::optional<int64_t>> &fullShape) {
  std::vector<size_t> kept;
  std::function<bool(size_t, size_t)> match = [&](size_t full, size_t old) {
    if (full == fullShape.size())
      return old == oldShape.size();
    if (old < oldShape.size() &&
        CanonicalizationExtentCompatible(oldShape[old], fullShape[full])) {
      kept.push_back(full);
      if (match(full + 1, old + 1))
        return true;
      kept.pop_back();
    }
    if (fullShape[full] && *fullShape[full] == 1 &&
        match(full + 1, old))
      return true;
    return false;
  };
  if (!match(0, 0))
    return std::nullopt;
  return kept;
}

inline void ReplaceCanonicalizationResultType(
    GenericModule &module, const GenericModuleAnalysisIndexes &indexes,
    int value, const std::string &type) {
  for (int userId : indexes.users(value)) {
    GenericOperation &operation =
        module.operations.at(static_cast<size_t>(userId));
    for (size_t index = 0; index < operation.operands.size(); ++index)
      if (operation.operands[index] == value &&
          index < operation.operandTypes.size())
        operation.operandTypes[index] = type;
  }
}

// Projection of OpWithOffsetSizesAndStridesConstantArgumentFolder used by
// tensor.extract_slice and memref.subview canonicalization.
inline bool FoldConstantOffsetSizeAndStrideOperands(
    GenericModule &module, const GenericModuleAnalysisIndexes &indexes) {
  bool changed = false;
  for (GenericOperation &operation : module.operations) {
    if ((operation.name != "tensor.extract_slice" &&
         operation.name != "memref.subview") ||
        operation.results.size() != 1 || operation.resultTypes.size() != 1)
      continue;
    const std::vector<size_t> segments =
        OperandSegmentSizes(operation.properties);
    if (segments.size() != 4 || segments.front() != 1)
      continue;
    std::vector<int64_t> staticOffsets =
        ParseDimensionI64Array(operation.properties, "static_offsets");
    std::vector<int64_t> staticSizes =
        ParseDimensionI64Array(operation.properties, "static_sizes");
    std::vector<int64_t> staticStrides =
        ParseDimensionI64Array(operation.properties, "static_strides");
    if (staticOffsets.size() != staticSizes.size() ||
        staticSizes.size() != staticStrides.size())
      continue;

    std::vector<int> operands{operation.operands.front()};
    std::vector<std::string> operandTypes{operation.operandTypes.front()};
    std::vector<size_t> newSegments{1, 0, 0, 0};
    size_t dynamicIndex = 1;
    bool foldedAny = false;
    auto foldList = [&](std::vector<int64_t> &staticValues, size_t segment,
                        bool onlyNonNegative, bool onlyNonZero) {
      for (int64_t &staticValue : staticValues) {
        if (staticValue != kCanonicalizationDynamicIndex)
          continue;
        const int value = operation.operands.at(dynamicIndex);
        const std::string &type = operation.operandTypes.at(dynamicIndex);
        ++dynamicIndex;
        std::optional<int64_t> constant;
        const int definition = indexes.definingOperationId(value);
        if (definition >= 0)
          constant = CanonicalizationIndexConstant(
              module.operations.at(static_cast<size_t>(definition)));
        if (constant && (!onlyNonNegative || *constant >= 0) &&
            (!onlyNonZero || *constant != 0)) {
          staticValue = *constant;
          foldedAny = true;
          continue;
        }
        operands.push_back(value);
        operandTypes.push_back(type);
        ++newSegments[segment];
      }
    };
    foldList(staticOffsets, 1, true, false);
    foldList(staticSizes, 2, true, false);
    foldList(staticStrides, 3, false, true);
    if (!foldedAny)
      continue;

    operation.operands = std::move(operands);
    operation.operandTypes = std::move(operandTypes);
    SetCanonicalizationProperty(
        operation, "operandSegmentSizes",
        "array<i32: " + std::to_string(newSegments[0]) + ", " +
            std::to_string(newSegments[1]) + ", " +
            std::to_string(newSegments[2]) + ", " +
            std::to_string(newSegments[3]) + ">");
    SetCanonicalizationProperty(operation, "static_offsets",
                                CanonicalizationI64Array(staticOffsets));
    SetCanonicalizationProperty(operation, "static_sizes",
                                CanonicalizationI64Array(staticSizes));
    SetCanonicalizationProperty(operation, "static_strides",
                                CanonicalizationI64Array(staticStrides));

    const std::optional<ShapedTypeModel> oldType =
        ParseShapedTypeForDimensionAnalysis(operation.resultTypes.front());
    if (!oldType)
      throw std::runtime_error(
          "canonicalize-ext: slice result is not a shaped type");
    std::vector<std::optional<int64_t>> fullShape;
    fullShape.reserve(staticSizes.size());
    for (int64_t size : staticSizes)
      fullShape.push_back(size == kCanonicalizationDynamicIndex
                              ? std::nullopt
                              : std::optional<int64_t>(size));
    const std::optional<std::vector<size_t>> kept =
        CanonicalizationKeptDimensions(oldType->shape, fullShape);
    if (!kept)
      throw std::runtime_error(
          "canonicalize-ext: cannot preserve slice rank reduction");
    std::vector<std::optional<int64_t>> resultShape;
    for (size_t dimension : *kept)
      resultShape.push_back(fullShape[dimension]);
    const std::string resultType = ReplaceCanonicalizationShape(
        operation.resultTypes.front(), resultShape);
    operation.resultTypes.front() = resultType;
    ReplaceCanonicalizationResultType(module, indexes,
                                      operation.results.front(), resultType);
    changed = true;
  }
  return changed;
}

inline std::string CanonicalizationOperationKey(
    const GenericOperation &operation) {
  std::ostringstream key;
  key << operation.name << '\n' << JoinDelimited(operation.resultTypes, ",")
      << '\n' << JoinDelimited(operation.operandTypes, ",") << '\n'
      << joinIds(operation.operands) << '\n';
  if (operation.name == "arith.constant") {
    if (const std::optional<ArithIntegerConstant> integer =
            ParseArithIntegerConstant(operation)) {
      key << "integer:" << integer->width << ':' << integer->bits;
    } else {
      std::string value =
          FindDictionaryValue(operation.properties, "value");
      if (value.empty())
        value = FindDictionaryValue(operation.attributes, "value");
      key << "value:" << trim(std::move(value));
    }
  } else {
    key << operation.properties << '\n' << operation.attributes;
  }
  return key.str();
}

inline void ReplaceCanonicalizedValue(GenericModule &module, int from,
                                      int to) {
  for (GenericOperation &operation : module.operations) {
    for (int &operand : operation.operands)
      if (operand == from)
        operand = to;
    for (int &operand : operation.dpsInputs)
      if (operand == from)
        operand = to;
    for (int &operand : operation.dpsInits)
      if (operand == from)
        operand = to;
  }
}

inline std::map<int, size_t>
CanonicalizationValueUseCounts(const GenericModule &module) {
  std::map<int, size_t> result;
  for (const GenericOperation &operation : module.operations) {
    if (operation.blockId >= 0) {
      const std::vector<int> &blockOperations =
          module.blocks.at(static_cast<size_t>(operation.blockId)).operations;
      if (std::find(blockOperations.begin(), blockOperations.end(),
                    operation.id) == blockOperations.end())
        continue;
    }
    for (int operand : operation.operands)
      ++result[operand];
  }
  return result;
}

inline bool IsCanonicalizationTerminator(
    const GenericOperation &operation) {
  static const std::set<std::string> terminators = {
      "affine.yield", "cf.br",       "cf.cond_br", "func.return",
      "scf.condition", "scf.yield"};
  return terminators.count(operation.name) != 0;
}

// Dense, stage-local topology used by the fixed-point canonicalizer. The
// Generic IR retains detached operation records, so operation.blockId alone
// cannot distinguish a live operation from a tombstone. Building this once
// per fixed-point iteration replaces the former repeated searches through
// every block operation list and the per-channel std::map reconstruction.
struct CanonicalizationTopology {
  explicit CanonicalizationTopology(const GenericModule &module)
      : activeOperations(module.operations.size(), uint8_t{0}),
        operationOrdinals(module.operations.size(), -1) {
    int maximumValue = -1;
    for (const GenericOperation &operation : module.operations) {
      if (operation.blockId < 0)
        activeOperations[static_cast<size_t>(operation.id)] = 1;
      for (int result : operation.results)
        maximumValue = std::max(maximumValue, result);
      for (int operand : operation.operands)
        maximumValue = std::max(maximumValue, operand);
    }
    for (const GenericBlock &block : module.blocks)
      for (int argument : block.arguments)
        maximumValue = std::max(maximumValue, argument);
    blockArgumentOwners.assign(
        maximumValue < 0 ? 0 : static_cast<size_t>(maximumValue) + 1, -1);
    for (const GenericBlock &block : module.blocks) {
      for (int argument : block.arguments)
        blockArgumentOwners[static_cast<size_t>(argument)] = block.id;
      for (size_t ordinal = 0; ordinal < block.operations.size(); ++ordinal) {
        const int operationId = block.operations[ordinal];
        activeOperations.at(static_cast<size_t>(operationId)) = 1;
        operationOrdinals.at(static_cast<size_t>(operationId)) =
            static_cast<int>(ordinal);
      }
    }
  }

  bool isActive(int operationId) const {
    return operationId >= 0 &&
           static_cast<size_t>(operationId) < activeOperations.size() &&
           activeOperations[static_cast<size_t>(operationId)] != 0;
  }

  int blockArgumentOwner(int value) const {
    return value < 0 || static_cast<size_t>(value) >= blockArgumentOwners.size()
               ? -1
               : blockArgumentOwners[static_cast<size_t>(value)];
  }

  void detach(int operationId) {
    if (operationId >= 0 &&
        static_cast<size_t>(operationId) < activeOperations.size())
      activeOperations[static_cast<size_t>(operationId)] = 0;
  }

  size_t valueCount() const { return blockArgumentOwners.size(); }

  std::vector<uint8_t> activeOperations;
  std::vector<int> operationOrdinals;
  std::vector<int> blockArgumentOwners;
};

inline bool IsCanonicalizationValueInBlock(
    int value, int blockId, PipelineAnalysisContext &analysis,
    const CanonicalizationTopology &topology, const GenericModule &module) {
  const int argumentOwner = topology.blockArgumentOwner(value);
  if (argumentOwner >= 0)
    return argumentOwner == blockId;
  const int definition = analysis.definingOperationId(value);
  return topology.isActive(definition) &&
         module.operations.at(static_cast<size_t>(definition)).blockId ==
             blockId;
}

// Mirrors CanonicalizeIterArg.cpp::isIterationIndependent. A dead scf.for
// result may be removed even when its yielded value is computed from the tied
// iter arg, provided that the entire chain is side-effect free and has no uses
// outside that dead channel.
inline std::optional<std::set<int>>
CanonicalizationIterationIndependentOps(const GenericModule &module,
                                         const GenericBlock &body,
                                         int allowedIterArg, int yielded,
                                         size_t channelIndex,
                                         PipelineAnalysisContext &analysis,
                                         const CanonicalizationTopology
                                             &topology) {
  std::vector<int> worklist = {yielded};
  std::vector<uint8_t> visitedValues(topology.valueCount(), uint8_t{0});
  if (yielded >= 0 && static_cast<size_t>(yielded) >= visitedValues.size())
    visitedValues.resize(static_cast<size_t>(yielded) + 1, uint8_t{0});
  if (yielded >= 0)
    visitedValues[static_cast<size_t>(yielded)] = 1;
  std::vector<uint8_t> visitedOperationFlags(module.operations.size(),
                                             uint8_t{0});

  while (!worklist.empty()) {
    const int current = worklist.back();
    worklist.pop_back();
    if (current == allowedIterArg)
      continue;

    const int argumentOwner = topology.blockArgumentOwner(current);
    if (argumentOwner >= 0) {
      if (argumentOwner == body.id)
        continue;
      return std::nullopt;
    }

    const int definition = analysis.definingOperationId(current);
    if (!topology.isActive(definition))
      return std::nullopt;
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(definition));
    // CanonicalizeIterArg.cpp::isNoEffect deliberately treats operations
    // without MemoryEffectOpInterface as effecting.  In GenericOperation,
    // an empty effect string means a reviewed operation whose interface
    // reported no effects; "none" is the conservative/default marker used
    // for operations such as scf.for that do not expose that interface.
    // Treating both spellings alike lets an outer dead iter-arg delete a
    // nested loop that still contains fixpipe/store side effects.
    if (operation.blockId != body.id || !operation.effects.empty())
      return std::nullopt;
    visitedOperationFlags[static_cast<size_t>(operation.id)] = 1;
    for (int operand : operation.operands)
      if (IsCanonicalizationValueInBlock(
              operand, body.id, analysis, topology, module)) {
        if (operand >= 0 &&
            static_cast<size_t>(operand) >= visitedValues.size())
          visitedValues.resize(static_cast<size_t>(operand) + 1, uint8_t{0});
        if (operand >= 0 &&
            visitedValues[static_cast<size_t>(operand)] == 0) {
          visitedValues[static_cast<size_t>(operand)] = 1;
          worklist.push_back(operand);
        }
      }
  }

  std::set<int> mustDelete;
  for (int userId : analysis.users(allowedIterArg)) {
    if (!topology.isActive(userId))
      continue;
    const GenericOperation &user =
        module.operations.at(static_cast<size_t>(userId));
    for (size_t operandIndex = 0; operandIndex < user.operands.size();
         ++operandIndex) {
      if (user.operands[operandIndex] != allowedIterArg)
        continue;
      if (user.name == "scf.yield") {
        if (user.blockId != body.id || operandIndex != channelIndex)
          return std::nullopt;
        continue;
      }
      if (visitedOperationFlags[static_cast<size_t>(user.id)] == 0)
        return std::nullopt;
      mustDelete.insert(user.id);
    }
  }

  for (int operationId : mustDelete) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    for (int result : operation.results) {
      for (int userId : analysis.users(result)) {
        if (!topology.isActive(userId))
          continue;
        const GenericOperation &user =
            module.operations.at(static_cast<size_t>(userId));
        for (size_t operandIndex = 0; operandIndex < user.operands.size();
             ++operandIndex) {
          if (user.operands[operandIndex] != result ||
              mustDelete.count(user.id) != 0)
            continue;
          if (user.name == "scf.yield" && user.blockId == body.id &&
              operandIndex == channelIndex)
            continue;
          return std::nullopt;
        }
      }
    }
  }
  return mustDelete;
}

// Projection of CanonicalizeIterArgPattern and RemoveDeadIterArgPattern from
// CanonicalizeIterArg.cpp. The lightweight IR rewrites the existing scf.for
// record instead of cloning it, but preserves the same result/init/iter/yield
// channels.
inline bool CanonicalizeIterArgs(GenericModule &module,
                                 PipelineAnalysisContext &useLists) {
  bool changed = false;
  GenericRewriter rewriter(module, &useLists);
  CanonicalizationTopology topology(module);
  for (GenericOperation &loop : module.operations) {
    if (!topology.isActive(loop.id) || loop.name != "scf.for" ||
        loop.regions.size() != 1 ||
        loop.operands.size() < 3 || loop.results.empty())
      continue;
    const GenericRegion &region =
        module.regions.at(static_cast<size_t>(loop.regions.front()));
    if (region.blocks.size() != 1)
      continue;
    GenericBlock &body =
        module.blocks.at(static_cast<size_t>(region.blocks.front()));
    if (body.arguments.size() < loop.results.size() + 1 ||
        body.operations.empty())
      continue;
    GenericOperation &yield = module.operations.at(
        static_cast<size_t>(body.operations.back()));
    if (yield.name != "scf.yield" ||
        yield.operands.size() < loop.results.size())
      continue;

    for (size_t index = 0; index < loop.results.size(); ++index) {
      const int init = loop.operands.at(index + 3);
      const int iterArg = body.arguments.at(index + 1);
      const int yielded = yield.operands.at(index);
      if ((yielded == init || yielded == iterArg) &&
          useLists.hasUsers(loop.results[index])) {
        rewriter.replaceAllUses(loop.results[index], init);
        changed = true;
      }
    }

    std::vector<size_t> removable;
    std::set<int> operationsToErase;
    for (size_t index = 0; index < loop.results.size(); ++index) {
      if (useLists.hasUsers(loop.results[index]))
        continue;
      const int iterArg = body.arguments.at(index + 1);
      const int yielded = yield.operands.at(index);
      const std::optional<std::set<int>> deadOperations =
          CanonicalizationIterationIndependentOps(module, body, iterArg,
                                                   yielded, index, useLists,
                                                   topology);
      if (!deadOperations)
        continue;
      removable.push_back(index);
      operationsToErase.insert(deadOperations->begin(), deadOperations->end());
    }
    if (!removable.empty()) {
      useLists.operationWillModify(loop);
      useLists.operationWillModify(yield);
    }
    for (auto iterator = removable.rbegin(); iterator != removable.rend();
         ++iterator) {
      const size_t index = *iterator;
      loop.results.erase(loop.results.begin() +
                         static_cast<std::ptrdiff_t>(index));
      loop.resultTypes.erase(loop.resultTypes.begin() +
                             static_cast<std::ptrdiff_t>(index));
      loop.operands.erase(loop.operands.begin() +
                          static_cast<std::ptrdiff_t>(index + 3));
      loop.operandTypes.erase(loop.operandTypes.begin() +
                              static_cast<std::ptrdiff_t>(index + 3));
      body.arguments.erase(body.arguments.begin() +
                           static_cast<std::ptrdiff_t>(index + 1));
      body.argumentTypes.erase(body.argumentTypes.begin() +
                               static_cast<std::ptrdiff_t>(index + 1));
      yield.operands.erase(yield.operands.begin() +
                           static_cast<std::ptrdiff_t>(index));
      if (index < yield.operandTypes.size())
        yield.operandTypes.erase(yield.operandTypes.begin() +
                                 static_cast<std::ptrdiff_t>(index));
      changed = true;
    }
    if (!operationsToErase.empty()) {
      for (int operationId : operationsToErase)
        if (topology.isActive(operationId)) {
          rewriter.removeFromBlock(body.id, operationId);
          topology.detach(operationId);
        }
      changed = true;
    }
  }
  return changed;
}

inline std::optional<bool>
CanonicalizationKnownBoolean(
    const GenericOperation &operation,
    const std::map<int, ArithIntegerConstant> &constants) {
  if (operation.results.size() != 1 || operation.resultTypes.size() != 1 ||
      operation.resultTypes.front() != "i1")
    return std::nullopt;
  if (operation.name == "arith.constant") {
    std::string value =
        FindDictionaryValue(operation.properties, "value");
    if (value.empty())
      value = FindDictionaryValue(operation.attributes, "value");
    const size_t typeSeparator = value.find(':');
    if (typeSeparator != std::string::npos)
      value.resize(typeSeparator);
    value = trim(std::move(value));
    if (value == "true" || value == "1")
      return true;
    if (value == "false" || value == "0")
      return false;
  }
  return FoldArithCmpI(operation, constants);
}

// Arith's canonical folds reduce boolean identities after cmpi(x, x) has
// become a known value. Preserve an existing SSA value as the replacement so
// no synthetic constant is needed in the lightweight representation.
inline bool FoldCanonicalizationBooleanOps(
    GenericModule &module, PipelineAnalysisContext &useLists) {
  const CanonicalizationTopology topology(module);
  std::map<int, ArithIntegerConstant> integerConstants;
  for (const GenericOperation &operation : module.operations)
    if (const std::optional<ArithIntegerConstant> value =
            ParseArithIntegerConstant(operation))
      integerConstants[operation.results.front()] = *value;
  std::map<int, bool> knownValues;
  for (const GenericOperation &operation : module.operations) {
    const std::optional<bool> known =
        CanonicalizationKnownBoolean(operation, integerConstants);
    if (known)
      knownValues[operation.results.front()] = *known;
  }

  GenericRewriter rewriter(module, &useLists);
  for (const GenericOperation &operation : module.operations) {
    if ((operation.name != "arith.ori" &&
         operation.name != "arith.andi") ||
        operation.operands.size() != 2 || operation.results.size() != 1 ||
        operation.blockId < 0)
      continue;
    if (!topology.isActive(operation.id))
      continue;
    const int lhs = operation.operands.front();
    const int rhs = operation.operands.back();
    const auto lhsKnown = knownValues.find(lhs);
    const auto rhsKnown = knownValues.find(rhs);
    int replacement = -1;
    if (operation.name == "arith.ori") {
      if (lhsKnown != knownValues.end() && !lhsKnown->second)
        replacement = rhs;
      else if (rhsKnown != knownValues.end() && !rhsKnown->second)
        replacement = lhs;
      else if (lhsKnown != knownValues.end() && lhsKnown->second)
        replacement = lhs;
      else if (rhsKnown != knownValues.end() && rhsKnown->second)
        replacement = rhs;
    } else {
      if (lhsKnown != knownValues.end() && lhsKnown->second)
        replacement = rhs;
      else if (rhsKnown != knownValues.end() && rhsKnown->second)
        replacement = lhs;
      else if (lhsKnown != knownValues.end() && !lhsKnown->second)
        replacement = lhs;
      else if (rhsKnown != knownValues.end() && !rhsKnown->second)
        replacement = rhs;
    }
    if (replacement < 0)
      continue;
    rewriter.replaceAllUses(operation.results.front(), replacement);
    rewriter.removeFromBlock(operation.blockId, operation.id);
    return true;
  }
  return false;
}

inline bool EliminateCanonicalizationDeadCode(
    GenericModule &module, PipelineAnalysisContext &useLists) {
  GenericRewriter rewriter(module, &useLists);
  CanonicalizationTopology topology(module);
  bool changed = false;
  for (auto iterator = module.operations.rbegin();
       iterator != module.operations.rend(); ++iterator) {
    const GenericOperation &operation = *iterator;
    if (operation.blockId < 0 || operation.results.empty() ||
        !operation.regions.empty() || IsCanonicalizationTerminator(operation))
      continue;
    if (!topology.isActive(operation.id))
      continue;
    if (std::any_of(operation.results.begin(), operation.results.end(),
                    [&](int result) { return useLists.hasUsers(result); }))
      continue;
    const bool noEffects = operation.effects.empty() ||
                           operation.effects == "none" ||
                           operation.name == "tensor.empty";
    if (!noEffects)
      continue;
    rewriter.removeFromBlock(operation.blockId, operation.id);
    topology.detach(operation.id);
    changed = true;
  }
  return changed;
}

inline std::vector<int>
CanonicalizationOperationPreOrder(const GenericModule &module) {
  std::vector<int> result;
  std::function<void(int)> visit = [&](int operationId) {
    result.push_back(operationId);
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    for (int regionId : operation.regions) {
      const GenericRegion &region =
          module.regions.at(static_cast<size_t>(regionId));
      for (int blockId : region.blocks) {
        const GenericBlock &block =
            module.blocks.at(static_cast<size_t>(blockId));
        for (int childId : block.operations)
          visit(childId);
      }
    }
  };
  if (!module.operations.empty())
    visit(0);
  return result;
}

inline bool CanonicalizationOperationDominates(
    const GenericModule &module, const CanonicalizationTopology &topology,
    const GenericOperation &candidate, const GenericOperation &operation) {
  const GenericOperation *cursor = &operation;
  while (cursor) {
    if (candidate.blockId == cursor->blockId && candidate.blockId >= 0 &&
        topology.isActive(candidate.id) && topology.isActive(cursor->id)) {
      const int candidateOrdinal =
          topology.operationOrdinals.at(static_cast<size_t>(candidate.id));
      const int cursorOrdinal =
          topology.operationOrdinals.at(static_cast<size_t>(cursor->id));
      if (candidateOrdinal >= 0 && cursorOrdinal >= 0 &&
          candidateOrdinal < cursorOrdinal)
        return true;
    }
    if (cursor->regionId < 0)
      break;
    const int parent = module.regions.at(
        static_cast<size_t>(cursor->regionId)).parentOperation;
    if (parent < 0)
      break;
    cursor = &module.operations.at(static_cast<size_t>(parent));
  }
  return false;
}

// CSE never crosses an isolated function boundary. Partition candidates by
// enclosing function so equivalent AIC/AIV operations do not trigger a
// quadratic sequence of dominance failures in combined projections.
inline void RunCanonicalizationCommonSubexpressionElimination(
    GenericModule &module, PipelineAnalysisContext &useLists,
    const std::set<int> *protectedOperationIds = nullptr) {
  const GenericModuleAnalysisIndexes enclosingFunctions(
      module, kGenericAnalysisEnclosingFunctions);
  CanonicalizationTopology topology(module);
  std::map<std::pair<int, std::string>, std::vector<int>>
      availableOperations;
  GenericRewriter rewriter(module, &useLists);
  for (int operationId : CanonicalizationOperationPreOrder(module)) {
    const GenericOperation snapshot =
        module.operations.at(static_cast<size_t>(operationId));
    if (snapshot.results.empty() || snapshot.blockId < 0 ||
        !snapshot.regions.empty() ||
        (!snapshot.effects.empty() && snapshot.effects != "none" &&
         snapshot.name != "tensor.empty"))
      continue;
    const auto key = std::make_pair(
        enclosingFunctions.enclosingFunctionId(snapshot.id),
        CanonicalizationOperationKey(snapshot));
    const bool protectedFromReplacement =
        protectedOperationIds != nullptr &&
        protectedOperationIds->count(operationId) != 0;
    int dominating = -1;
    for (auto candidate = availableOperations[key].rbegin();
         candidate != availableOperations[key].rend(); ++candidate) {
      if (CanonicalizationOperationDominates(
              module, topology,
              module.operations.at(static_cast<size_t>(*candidate)),
              snapshot)) {
        dominating = *candidate;
        break;
      }
    }
    // A transform-created destination tile must remain distinct from an
    // equivalent value created earlier in the block: the real transform
    // cleanup runs before that final producer is fused.  It is nevertheless
    // available to CSE operations created after it.  Recording it here also
    // makes it the most recent dominating candidate for those later values.
    if (dominating < 0 || protectedFromReplacement) {
      availableOperations[key].push_back(snapshot.id);
      continue;
    }
    const GenericOperation &candidate =
        module.operations.at(static_cast<size_t>(dominating));
    if (candidate.results.size() != snapshot.results.size())
      continue;
    for (size_t index = 0; index < snapshot.results.size(); ++index)
      rewriter.replaceAllUses(snapshot.results[index],
                              candidate.results[index]);
    rewriter.removeFromBlock(snapshot.blockId, snapshot.id);
    topology.detach(snapshot.id);
  }
}

// ExtendedCanonicalizer and CSE reuse equivalent dominating operations. Only
// side-effect-free, regionless operations participate, matching MLIR CSE's
// eligibility rule.
inline GenericModule
RunCanonicalizationHIVMAfterArithToAffine(GenericModule module) {
  // scf::createCanonicalizeIterArgPass registers
  // tensor::populateFoldTensorEmptyPatterns before its iter-arg patterns.
  // This is part of the production canonicalization pipeline, not just the
  // later standalone FoldTensorEmpty pass after TileAndBindSubBlock.
  module = RunFoldTensorEmpty(std::move(module));
  const GenericModuleAnalysisIndexes foldIndexes(
      module, kGenericAnalysisDefinitions | kGenericAnalysisUsers);
  while (FoldConstantOffsetSizeAndStrideOperands(module, foldIndexes)) {
  }
  PipelineAnalysisContext useLists(
      module, kGenericAnalysisDefinitions | kGenericAnalysisUsers);
  while (CanonicalizeIterArgs(module, useLists)) {
  }
  while (FoldCanonicalizationBooleanOps(module, useLists)) {
  }
  RunCanonicalizationCommonSubexpressionElimination(module, useLists);
  while (FoldCanonicalizationBooleanOps(module, useLists)) {
  }
  while (EliminateCanonicalizationDeadCode(module, useLists)) {
  }
  return CompactGenericModule(std::move(module));
}

inline GenericModule RunCanonicalizationHIVMPipeline(GenericModule module) {
  return RunCanonicalizationHIVMAfterArithToAffine(
      RunArithToAffineConversion(std::move(module)));
}

inline GenericModule
RunCanonicalizationHIVMPipelineSourceAligned(GenericModule module) {
  return RunCanonicalizationHIVMAfterArithToAffine(
      RunArithToAffineConversionWithAffineCanonicalization(
          std::move(module)));
}

} // namespace cvub

#endif
