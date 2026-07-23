// UB-focused MemLivenessAnalysis equivalent used by the PlanMemory model.
#ifndef CVPIPELINE_UB_MODEL_CPP_MEM_LIVENESS_ANALYSIS_HPP
#define CVPIPELINE_UB_MODEL_CPP_MEM_LIVENESS_ANALYSIS_HPP

#include "liveness.hpp"

#include <iostream>
#include <list>
#include <memory>
#include <random>

namespace cvub {

inline std::vector<std::string>
extractMemrefTypes(const std::string &groupBody) {
  std::vector<std::string> types;
  size_t pos = 0;
  while ((pos = groupBody.find("memref<", pos)) != std::string::npos) {
    size_t begin = pos;
    pos += 7;
    int depth = 1;
    while (pos < groupBody.size() && depth > 0) {
      if (groupBody[pos] == '<')
        ++depth;
      else if (groupBody[pos] == '>')
        --depth;
      ++pos;
    }
    if (depth == 0)
      types.push_back(groupBody.substr(begin, pos - begin));
  }
  return types;
}

inline std::optional<int64_t>
staticMemrefOffset(const std::string &type) {
  std::optional<MemRefTypeModel> model = ParseMemRefType(type);
  return model ? std::optional<int64_t>(model->offset) : std::nullopt;
}

inline uint64_t elementBitWidthFromType(const std::string &type) {
  std::optional<MemRefTypeModel> model = ParseMemRefType(type);
  return model ? GetElementBitWidth(*model) : 0;
}

inline bool hasIntegerElementType(const std::string &type) {
  std::optional<MemRefTypeModel> model = ParseMemRefType(type);
  return model && HasIntegerElementType(*model);
}

inline std::vector<uint64_t>
operandBitWidths(const std::string &groupBody) {
  std::vector<uint64_t> widths;
  static const std::regex typeRegex(R"((bf16|f16|f32|f64|i[0-9]+))");
  for (std::sregex_iterator it(groupBody.begin(), groupBody.end(), typeRegex),
       end;
       it != end; ++it) {
    std::string type = (*it)[1].str();
    if (type == "bf16" || type == "f16")
      widths.push_back(16);
    else if (type == "f32")
      widths.push_back(32);
    else if (type == "f64")
      widths.push_back(64);
    else
      widths.push_back(static_cast<uint64_t>(std::stoull(type.substr(1))));
  }
  return widths;
}

inline std::vector<std::string>
operationGroupMemrefTypes(const OperationRecord &operation,
                          const std::string &group) {
  const std::vector<std::string> *materialized =
      MaterializedOperationGroupTypes(operation, group);
  if (!materialized)
    return extractMemrefTypes(extractGroupBody(operation.text, group));
  std::vector<std::string> memrefs;
  for (const std::string &type : *materialized)
    if (type.find("memref<") != std::string::npos)
      memrefs.push_back(type);
  return memrefs;
}

inline std::vector<uint64_t>
operationGroupOperandBitWidths(const OperationRecord &operation,
                               const std::string &group) {
  const std::vector<std::string> *materialized =
      MaterializedOperationGroupTypes(operation, group);
  if (!materialized)
    return operandBitWidths(extractGroupBody(operation.text, group));
  std::vector<uint64_t> widths;
  for (const std::string &type : *materialized) {
    std::vector<uint64_t> current = operandBitWidths(type);
    widths.insert(widths.end(), current.begin(), current.end());
  }
  return widths;
}

inline bool lastDimContiguous(const std::string &type) {
  std::optional<MemRefTypeModel> model = ParseMemRefType(type);
  return model && !model->strides.empty() && model->strides.back() &&
         *model->strides.back() == 1;
}

inline std::vector<std::string> memrefShapeTokens(const std::string &type) {
  std::optional<MemRefTypeModel> model = ParseMemRefType(type);
  if (!model)
    return {};
  std::vector<std::string> shape;
  for (const std::optional<int64_t> &dimension : model->shape)
    shape.push_back(dimension ? std::to_string(*dimension) : "?");
  return shape;
}

inline HIVMPipe GetPipe(const OperationRecord &op) {
  HIVMPipe pipe = GetStaticPipe(op.opName);
  if (pipe != HIVMPipe::Unassigned)
    return pipe;
  if (op.opName == "hivm.hir.copy") {
    std::vector<std::string> inputTypes =
        operationGroupMemrefTypes(op, "ins");
    std::vector<std::string> outputTypes =
        operationGroupMemrefTypes(op, "outs");
    if (inputTypes.size() != 1 || outputTypes.size() != 1)
      return HIVMPipe::Unassigned;
    const std::optional<MemRefTypeModel> source =
        ParseMemRefType(inputTypes.front());
    const std::optional<MemRefTypeModel> destination =
        ParseMemRefType(outputTypes.front());
    if (!source || !destination)
      return HIVMPipe::Unassigned;
    if (source->addressSpace == AddressSpace::GM &&
        destination->addressSpace == AddressSpace::L1)
      return HIVMPipe::MTE2;
    if (source->addressSpace == AddressSpace::UB &&
        destination->addressSpace == AddressSpace::L1)
      return HIVMPipe::MTE3;
    if (source->addressSpace == AddressSpace::UB &&
        destination->addressSpace == AddressSpace::UB)
      return HIVMPipe::Other;
    if (source->addressSpace == AddressSpace::L0C &&
        destination->addressSpace == AddressSpace::GM)
      return HIVMPipe::Other;
    return HIVMPipe::Unassigned;
  }
  if (op.opName == "hivm.hir.vbrc") {
    std::vector<std::string> outputTypes =
        operationGroupMemrefTypes(op, "outs");
    if (outputTypes.size() != 1)
      return HIVMPipe::Unassigned;
    const std::optional<MemRefTypeModel> destination =
        ParseMemRefType(outputTypes.front());
    if (!destination)
      return HIVMPipe::Unassigned;
    if (destination->addressSpace == AddressSpace::L1)
      return HIVMPipe::MTE2;
    if (destination->addressSpace == AddressSpace::UB)
      return HIVMPipe::Other;
    return HIVMPipe::Unassigned;
  }
  if (op.opName == "hivm.hir.custom") {
    if (op.text.find("PIPE_MTE2") != std::string::npos)
      return HIVMPipe::MTE2;
    if (op.text.find("PIPE_MTE3") != std::string::npos)
      return HIVMPipe::MTE3;
    if (op.text.find("PIPE_UNASSIGNED") == std::string::npos &&
        op.text.find("#hivm.pipe<") != std::string::npos)
      return HIVMPipe::Other;
  }
  return HIVMPipe::Unassigned;
}

inline std::vector<int64_t> getDenseI64Array(const std::string &text,
                                             const std::string &name) {
  const size_t key = text.find(name);
  if (key == std::string::npos)
    return {};
  const size_t begin = text.find('[', key + name.size());
  const size_t end = text.find(']', begin == std::string::npos ? key : begin);
  if (begin == std::string::npos || end == std::string::npos)
    return {};
  std::vector<int64_t> values;
  static const std::regex integerRegex(R"(-?[0-9]+)");
  const std::string body = text.substr(begin + 1, end - begin - 1);
  for (std::sregex_iterator it(body.begin(), body.end(), integerRegex), last;
       it != last; ++it)
    values.push_back(std::stoll(it->str()));
  return values;
}

inline std::optional<std::string>
getFirstInputMemrefType(const OperationRecord &op) {
  std::vector<std::string> types = operationGroupMemrefTypes(op, "ins");
  if (types.empty())
    return std::nullopt;
  return types.front();
}

inline bool hasPureBufferSemantics(const OperationRecord &op) {
  return op.text.find("tensor<") == std::string::npos;
}

inline bool hasHWUnsupportedScalarOperand(const OperationRecord &op) {
  if (!HasScalarOnlyHWOperand1(op.opName))
    return false;
  return operationGroupMemrefTypes(op, "ins").size() >= 2;
}

using StrideAlignment = std::pair<int64_t, int64_t>;
using StrideAlignmentMap =
    std::map<std::string, std::vector<StrideAlignment>>;

inline std::vector<int64_t> getDenseIntegerArrayAttribute(
    const std::string &text, const std::string &name) {
  const size_t key = text.find(name);
  if (key == std::string::npos)
    return {};
  size_t begin = text.find('[', key + name.size());
  size_t end =
      text.find(']', begin == std::string::npos ? key + name.size() : begin);
  if (begin == std::string::npos || end == std::string::npos) {
    const size_t array = text.find("array<", key + name.size());
    const size_t colon = text.find(':', array);
    const size_t close = text.find('>', colon);
    if (array == std::string::npos || colon == std::string::npos ||
        close == std::string::npos)
      return {};
    begin = colon;
    end = close;
  }
  std::vector<int64_t> values;
  static const std::regex integerRegex(R"(-?[0-9]+)");
  const std::string body = text.substr(begin + 1, end - begin - 1);
  for (std::sregex_iterator it(body.begin(), body.end(), integerRegex), last;
       it != last; ++it)
    values.push_back(std::stoll(it->str()));
  return values;
}

inline StrideAlignmentMap CollectStrideAlignmentMarks(
    const std::vector<OperationRecord> &operations) {
  StrideAlignmentMap result;
  for (const OperationRecord &operation : operations) {
    if (operation.opName != "annotation.mark" ||
        operation.text.find("hivm.stride_align_dims") == std::string::npos)
      continue;
    std::vector<std::string> operands = operationOperandNames(operation);
    if (operands.empty())
      continue;
    std::vector<int64_t> dimensions = getDenseIntegerArrayAttribute(
        operation.text, "hivm.stride_align_dims");
    std::vector<int64_t> alignments = getDenseIntegerArrayAttribute(
        operation.text, "hivm.stride_align_value_in_byte");
    if (dimensions.empty() || alignments.empty() ||
        dimensions.size() != alignments.size())
      continue;
    for (size_t i = 0; i < dimensions.size(); ++i)
      result[operands.front()].push_back({dimensions[i], alignments[i]});
  }
  return result;
}

struct FlattenResultModel {
  int64_t rankAfterFlatten = 0;
  std::vector<int64_t> barrierDims;
};

inline FlattenResultModel getFlattenedUniformReassociation(
    const OperationRecord &op, const std::vector<int64_t> &limitedAxes,
    const StrideAlignmentMap &strideAlignments = {}) {
  struct TypedOperand {
    std::string value;
    MemRefTypeModel type;
  };
  std::vector<TypedOperand> operands;
  for (const std::string group : {"ins", "outs"}) {
    const std::string body = extractGroupBody(op.text, group);
    std::vector<std::string> values = extractSSAs(body);
    std::vector<std::string> types = extractMemrefTypes(body);
    if (values.size() != types.size())
      throw std::runtime_error(
          "FlattenInterface: shaped operand/type count mismatch for " +
          op.opName);
    for (size_t i = 0; i < values.size(); ++i) {
      std::optional<MemRefTypeModel> type = ParseMemRefType(types[i]);
      if (!type)
        throw std::runtime_error("FlattenInterface: expected memref operand");
      auto marks = strideAlignments.find(values[i]);
      if (marks != strideAlignments.end())
        *type = ApplyStrideAlignment(std::move(*type), marks->second);
      operands.push_back({values[i], std::move(*type)});
    }
  }
  if (operands.empty())
    throw std::runtime_error("FlattenInterface: operation has no memrefs");
  const size_t rank = operands.front().type.shape.size();
  for (const TypedOperand &operand : operands)
    if (operand.type.shape.size() != rank)
      throw std::runtime_error("FlattenInterface: operand rank mismatch");
  for (int64_t axis : limitedAxes)
    if (axis < 0 || static_cast<size_t>(axis) >= rank)
      throw std::runtime_error("FlattenInterface: limited axis out of range");
  if (rank == 0)
    return {};

  std::vector<bool> unitMask(rank, true);
  for (const TypedOperand &operand : operands)
    for (size_t axis = 0; axis < rank; ++axis)
      unitMask[axis] = unitMask[axis] && operand.type.shape[axis] &&
                       *operand.type.shape[axis] == 1;
  for (int64_t axis : limitedAxes)
    unitMask[static_cast<size_t>(axis)] = false;

  // getReassociationFromUnitMask from FlattenUnit.cpp.
  std::vector<std::vector<size_t>> unitReassociation(1);
  size_t axis = 0;
  while (axis < rank && unitMask[axis])
    unitReassociation.back().push_back(axis++);
  bool firstNonUnit = true;
  while (axis < rank) {
    if (!firstNonUnit)
      unitReassociation.emplace_back();
    firstNonUnit = false;
    unitReassociation.back().push_back(axis++);
    while (axis < rank && unitMask[axis])
      unitReassociation.back().push_back(axis++);
  }

  std::vector<size_t> unitMapping(rank, 0);
  for (size_t group = 0; group < unitReassociation.size(); ++group)
    for (size_t originalAxis : unitReassociation[group])
      unitMapping[originalAxis] = group;
  std::vector<int64_t> adjustedAxes;
  for (int64_t limitedAxis : limitedAxes)
    adjustedAxes.push_back(static_cast<int64_t>(
        unitMapping[static_cast<size_t>(limitedAxis)]));

  std::vector<MemRefTypeModel> unitTypes;
  for (const TypedOperand &operand : operands)
    unitTypes.push_back(
        CollapseMemRefType(operand.type, unitReassociation));
  const size_t unitRank = unitReassociation.size();
  std::vector<bool> targetMask(unitRank, false);
  for (int64_t adjustedAxis : adjustedAxes)
    targetMask[static_cast<size_t>(adjustedAxis)] = true;
  const std::vector<bool> contiguous = GetContiguousAxes(unitTypes);
  const bool isTargetCollapsible = op.opName == "hivm.hir.vreduce";
  std::vector<size_t> collapseMapping(unitRank, 0);
  size_t collapseGroupCount = 0;
  for (size_t i = 0; i < unitRank; ++i) {
    bool createGroup = i == 0;
    if (i > 0 && targetMask[i - 1] != targetMask[i])
      createGroup = true;
    if (!isTargetCollapsible && targetMask[i])
      createGroup = true;
    if (!contiguous[i])
      createGroup = true;
    if (createGroup)
      ++collapseGroupCount;
    collapseMapping[i] = collapseGroupCount - 1;
  }

  FlattenResultModel result;
  result.rankAfterFlatten = static_cast<int64_t>(collapseGroupCount);
  for (int64_t adjustedAxis : adjustedAxes)
    result.barrierDims.push_back(static_cast<int64_t>(
        collapseMapping[static_cast<size_t>(adjustedAxis)]));
  return result;
}

inline bool shouldCumOpLowerToScalarLoops(
    const OperationRecord &op,
    const StrideAlignmentMap &strideAlignments = {}) {
  if (!hasPureBufferSemantics(op))
    return false;
  std::vector<int64_t> cumDims = getDenseI64Array(op.text, "cum_dims");
  if (cumDims.size() > 1)
    return false;
  if (cumDims.empty())
    throw std::runtime_error("VCum operation has no cumulative dimension");
  std::optional<std::string> inputType = getFirstInputMemrefType(op);
  if (!inputType)
    return false;
  if (elementBitWidthFromType(*inputType) == 64 &&
      hasIntegerElementType(*inputType))
    return true;
  FlattenResultModel flattened =
      getFlattenedUniformReassociation(op, cumDims, strideAlignments);
  if (flattened.barrierDims.size() != 1)
    throw std::runtime_error("VCum flattening lost its cumulative dimension");
  return flattened.barrierDims.front() == flattened.rankAfterFlatten - 1;
}

inline bool shouldVReduceOpLowerToScalarLoops(
    const OperationRecord &op,
    const StrideAlignmentMap &strideAlignments = {}) {
  if (!hasPureBufferSemantics(op))
    return false;
  std::optional<std::string> inputType = getFirstInputMemrefType(op);
  if (!inputType)
    return false;
  const uint64_t bitWidth = elementBitWidthFromType(*inputType);
  const bool integerType = hasIntegerElementType(*inputType);
  const std::optional<MemRefTypeModel> sourceType =
      ParseMemRefType(*inputType);
  if (!sourceType)
    return false;
  const std::vector<int64_t> reduceDims =
      getDenseI64Array(op.text, "reduce_dims");
  static const std::regex vreduceKindRegex(
      R"(vreduce\s*<([a-z_]+)>)");
  std::smatch arithmetic;
  if (!std::regex_search(op.text, arithmetic, vreduceKindRegex))
    return false;
  const std::string kind = arithmetic[1].str();
  if (kind == "min" || kind == "max" || kind == "sum" ||
      kind == "prod" || kind == "xori")
    return integerType && bitWidth == 64;

  const bool indexLeft = kind == "max_with_index_left" ||
                         kind == "min_with_index_left";
  const bool indexRight = kind == "max_with_index_right" ||
                          kind == "min_with_index_right";
  if (!indexLeft && !indexRight)
    return false;
  if (integerType && (bitWidth == 16 || bitWidth == 32 || bitWidth == 64))
    return true;
  if (integerType)
    return false;
  const bool supportedFloat = sourceType->elementType == "f16" ||
                              sourceType->elementType == "f32" ||
                              sourceType->elementType == "bf16";
  if (!supportedFloat)
    return false;
  if (indexRight) {
    if (reduceDims.size() != 1)
      throw std::runtime_error(
          "VReduce index-right requires one reduction dimension");
    if (sourceType->shape.size() == 1 ||
        reduceDims.front() ==
            static_cast<int64_t>(sourceType->shape.size()) - 1)
      return true;
  }
  return getFlattenedUniformReassociation(op, reduceDims, strideAlignments)
             .rankAfterFlatten > 2;
}

inline bool shouldLowerToScalarLoops(
    const OperationRecord &op,
    const StrideAlignmentMap &strideAlignments = {}) {
  if (!HasImplByScalarOpInterface(op.opName) ||
      !hasPureBufferSemantics(op))
    return false;
  if (hasHWUnsupportedScalarOperand(op))
    return true;
  std::optional<std::string> inputType = getFirstInputMemrefType(op);
  if (!inputType)
    return false;
  const uint64_t bitWidth = elementBitWidthFromType(*inputType);
  const bool integerType = hasIntegerElementType(*inputType);
  if (op.opName == "hivm.hir.vcumprod" ||
      op.opName == "hivm.hir.vcumsum")
    return shouldCumOpLowerToScalarLoops(op, strideAlignments);
  if (op.opName == "hivm.hir.vmod")
    return integerType && (bitWidth == 32 || bitWidth == 64);
  if (op.opName == "hivm.hir.vcmp") {
    if (!integerType)
      return false;
    const bool eqOrNe = op.text.find("compare_mode = <eq>") !=
                            std::string::npos ||
                        op.text.find("compare_mode = <ne>") !=
                            std::string::npos ||
                        op.text.find("compare_mode") == std::string::npos;
    return bitWidth != 32 || !eqOrNe;
  }
  if (op.opName == "hivm.hir.vmulext" ||
      op.opName == "hivm.hir.vmulextui")
    return integerType && (bitWidth == 32 || bitWidth == 64);
  if (op.opName == "hivm.hir.vreduce")
    return shouldVReduceOpLowerToScalarLoops(op, strideAlignments);
  return integerType && bitWidth == 64;
}

inline bool IsReuseHIVMOp(
    const OperationRecord &op, const std::string &genBuffer,
    const std::string &killBuffer,
    const std::unordered_map<std::string, std::string> &alias,
    const std::set<std::string> &allocNames,
    bool restrictInplaceAsISA = false) {
  if (!IsElementwiseNaryOp(op.opName) ||
      !IsDestinationStyleOp(op.opName))
    return false;

  std::vector<std::string> outputTypes =
      operationGroupMemrefTypes(op, "outs");
  std::vector<std::string> inputTypes =
      operationGroupMemrefTypes(op, "ins");
  auto typeForBuffer = [&](const std::string &group,
                           const std::string &buffer)
      -> std::optional<std::string> {
    std::string body = extractGroupBody(op.text, group);
    std::vector<std::string> values = extractSSAs(body);
    std::vector<std::string> types = extractMemrefTypes(body);
    for (size_t i = 0; i < values.size(); ++i) {
      auto root = canonicalAlloc(alias, allocNames, values[i]);
      if (!root || *root != buffer)
        continue;
      if (values.size() == types.size())
        return types[i];
      if (i < types.size())
        return types[i];
      if (!types.empty())
        return types.front();
    }
    return std::nullopt;
  };
  if (outputTypes.empty())
    return false;
  std::optional<int64_t> outputOffset = staticMemrefOffset(outputTypes.front());
  if (!outputOffset)
    return false;
  for (const std::string &inputType : inputTypes) {
    std::optional<int64_t> inputOffset = staticMemrefOffset(inputType);
    if (inputOffset && *inputOffset != *outputOffset)
      return false;
  }

  const bool hasBroadcast =
      !getDenseI64Array(op.text, "broadcast").empty();
  const bool hasTranspose =
      !getDenseI64Array(op.text, "transpose").empty();
  static const std::set<std::string> isaReusable = {
      "hivm.hir.vadd", "hivm.hir.vsub", "hivm.hir.vmax",
      "hivm.hir.vmin", "hivm.hir.vor",  "hivm.hir.vand",
      "hivm.hir.vmul"};
  if (isaReusable.count(op.opName) && !hasBroadcast && !hasTranspose)
    return true;

  if (restrictInplaceAsISA)
    return false;

  std::vector<std::string> temps =
      OperationGroupNames(op, "temp_buffer");
  if (temps.size() > 1)
    return false;
  if (temps.size() == 1) {
    bool hasScalarInput =
        operationGroupOperandBitWidths(op, "ins").size() >
        inputTypes.size();
    if (!ShouldAllocExtraBufferForScalarOrOTFBrc(op.opName) ||
        (!hasBroadcast && !hasScalarInput))
      return false;
  }
  if (hasTranspose)
    return false;
  if (hasBroadcast) {
    std::optional<std::string> genType = typeForBuffer("outs", genBuffer);
    std::optional<std::string> killType = typeForBuffer("ins", killBuffer);
    if (!genType || !killType)
      return false;
    std::vector<std::string> outShape = memrefShapeTokens(*genType);
    std::vector<std::string> inShape = memrefShapeTokens(*killType);
    size_t count = std::min(outShape.size(), inShape.size());
    for (size_t i = 0; i < count; ++i)
      if (outShape[i] != inShape[i])
        return false;
    return true;
  }

  if (HasSameOperandsElementType(op.opName))
    return true;

  if (op.opName == "hivm.hir.vsel") {
    std::vector<std::string> inputs = OperationGroupNames(op, "ins");
    std::vector<uint64_t> widths =
        operationGroupOperandBitWidths(op, "ins");
    if (inputs.size() >= 2 && widths.size() >= 2) {
      auto cond = canonicalAlloc(alias, allocNames, inputs[0]);
      if (cond && widths[0] == 1 && widths[1] == 64)
        return killBuffer != *cond;
    }
    return true;
  }

  uint64_t outputBits = elementBitWidthFromType(outputTypes.front());
  std::vector<uint64_t> inputWidths =
      operationGroupOperandBitWidths(op, "ins");
  if (outputBits == 0 || inputWidths.empty())
    return false;
  for (uint64_t inputBits : inputWidths) {
    if (inputBits == outputBits)
      return true;
    if (inputBits % outputBits != 0)
      continue;
    if (op.opName != "hivm.hir.vcast")
      return true;
    std::optional<std::string> genType = typeForBuffer("outs", genBuffer);
    std::optional<std::string> killType = typeForBuffer("ins", killBuffer);
    if (!genType || !killType)
      return false;
    return memrefShapeTokens(*genType).size() <= 1 &&
           memrefShapeTokens(*killType).size() <= 1 &&
           lastDimContiguous(*genType) && lastDimContiguous(*killType);
  }
  return false;
}

// The parsed PlanMemory input is deliberately independent from MLIR text.
// The PlanMemory input builder constructs this object from the pass semantic
// state; the file overload
// below remains only for the PlanMemory-before-IR compatibility interface.
struct PlanMemoryInput {
  std::vector<IRAllocRecord> allocations;
  std::vector<OperationRecord> operations;
  std::vector<std::string> functionArguments;
  std::shared_ptr<const PlanMemoryOperationIndexStorage> operationIndex;
  // True only for the in-memory semantic bridge used by the normal pipeline.
  // Such an input must carry its finalized typed SSA/use-def index; rebuilding
  // it from OperationRecord::text would reintroduce the bridge conversion this
  // path is intended to remove.  ParsePlanMemoryInput remains the compatibility
  // path for a user-supplied before-plan-memory text file.
  bool hasTypedOperationBridge = false;
};

inline std::shared_ptr<const PlanMemoryOperationIndexStorage>
RequirePlanMemoryOperationIndex(const PlanMemoryInput &input) {
  if (input.hasTypedOperationBridge && !input.operationIndex)
    throw std::runtime_error(
        "PlanMemoryInput: typed operation bridge is missing its index");
  return input.operationIndex;
}

// Everything below is a pure function of the finalized PlanMemory input.  A
// retry changes only the random order in which simultaneously-live values are
// considered; rebuilding these indexes for every seed is therefore redundant.
// Keep the context non-movable because Liveness holds a reference to the
// operation index, which in turn holds a reference to the input operation
// stream.
class PreparedMemLivenessAnalysis {
public:
  explicit PreparedMemLivenessAnalysis(const PlanMemoryInput &input,
                                       DebugTrace *trace = nullptr)
      : operations(input.operations),
        operationIndex(operations, RequirePlanMemoryOperationIndex(input)),
        liveness(operations, input.functionArguments, operationIndex, trace) {
    // Bridge-generated operation ids intentionally live in a high numeric
    // range.  A dense cache indexed by that id initialized roughly one
    // million optional vectors for every input even though only O(operations)
    // entries can ever be queried.
    orderedLiveValues.reserve(operations.size());
    for (const IRAllocRecord &record : input.allocations) {
      allocNames.insert(record.name);
      allocOrder.push_back(record.name);
      allocExtentBits[record.name] = record.constBits;
    }
    // Region topology and branch block arguments are immutable across retry
    // seeds. Build the same ordered relationships once instead of repeatedly
    // scanning the complete operation stream from every control-flow op in
    // every attempt.
    for (const OperationRecord &operation : operations)
      blockArguments[{operation.regionPath, operation.blockLabel}] =
          operation.blockArguments;

    // Build direct region ownership in the same linear walk. The previous
    // lazy helpers rescanned the entire operation stream for each parent and
    // each rollback seed even though this topology is immutable.
    std::map<std::vector<int>, int> lastOperationAtPath;
    for (const OperationRecord &operation : operations) {
      if (!operation.regionPath.empty()) {
        std::vector<int> parentPath(operation.regionPath.begin(),
                                    std::prev(operation.regionPath.end()));
        const int region = operation.regionPath.back();
        const auto key = std::make_pair(parentPath, region);
        if (parentOperationByRegionPath.count(key) == 0) {
          const auto owner = lastOperationAtPath.find(parentPath);
          const int ownerId = owner == lastOperationAtPath.end()
                                  ? -1
                                  : owner->second;
          parentOperationByRegionPath.emplace(key, ownerId);
          if (ownerId >= 0) {
            std::vector<int> &children =
                childRegionsByOperation[ownerId];
            if (children.empty() || children.back() != region)
              children.push_back(region);
          }
        }
      }
      lastOperationAtPath[operation.regionPath] = operation.operationId;
    }
  }

  PreparedMemLivenessAnalysis(const PreparedMemLivenessAnalysis &) = delete;
  PreparedMemLivenessAnalysis &
  operator=(const PreparedMemLivenessAnalysis &) = delete;
  PreparedMemLivenessAnalysis(PreparedMemLivenessAnalysis &&) = delete;
  PreparedMemLivenessAnalysis &
  operator=(PreparedMemLivenessAnalysis &&) = delete;

  const std::vector<std::string> &
  currentlyLiveValuesOrdered(int operationId) const {
    static const std::vector<std::string> empty;
    if (operationId < 0)
      return empty;
    auto cached = orderedLiveValues.find(operationId);
    if (cached != orderedLiveValues.end())
      return cached->second;
    return orderedLiveValues
        .emplace(operationId,
                 liveness.currentlyLiveValuesOrdered(operationId))
        .first->second;
  }

  const std::vector<int> &childRegions(int operationId) const {
    static const std::vector<int> empty;
    auto found = childRegionsByOperation.find(operationId);
    if (found != childRegionsByOperation.end())
      return found->second;
    return empty;
  }

  int parentOperation(const std::vector<int> &parentPath, int region) const {
    const auto key = std::make_pair(parentPath, region);
    auto found = parentOperationByRegionPath.find(key);
    if (found != parentOperationByRegionPath.end())
      return found->second;
    return -1;
  }

  const std::vector<std::string> *
  argumentsForBlock(const std::vector<int> &regionPath,
                    const std::string &blockLabel) const {
    auto found = blockArguments.find({regionPath, blockLabel});
    return found == blockArguments.end() ? nullptr : &found->second;
  }

  const std::vector<OperationRecord> &operations;
  std::set<std::string> allocNames;
  std::vector<std::string> allocOrder;
  std::map<std::string, uint64_t> allocExtentBits;
  PlanMemoryOperationIndex operationIndex;
  Liveness liveness;
  mutable std::unordered_map<int, std::vector<std::string>> orderedLiveValues;
  std::map<int, std::vector<int>> childRegionsByOperation;
  std::map<std::pair<std::vector<int>, int>, int>
      parentOperationByRegionPath;
  std::map<std::pair<std::vector<int>, std::string>,
           std::vector<std::string>>
      blockArguments;
};

inline PlanMemoryInput ParsePlanMemoryInput(const fs::path &beforeIR,
                                            const std::string &coreType) {
  (void)RequireUniqueTargetFunction(beforeIR, coreType);
  PlanMemoryInput result;
  result.allocations = qualifyScopedAllocRecords(beforeIR, coreType);
  std::set<std::string> allocNames;
  std::map<std::string, std::string> allocTypes;
  for (const IRAllocRecord &record : result.allocations) {
    allocNames.insert(record.name);
    allocTypes[record.name] = record.memrefType;
  }
  result.operations = ApplyPlanMemoryNormalizePatterns(
      NormalizeIterUseAfterYieldInit(
          qualifyScopedSSAValues(parseFunctionOperations(beforeIR, coreType)),
          allocNames, allocTypes));
  MaterializeOperationValueLists(result.operations);
  result.operationIndex =
      BuildPlanMemoryOperationIndexStorage(result.operations);
  result.functionArguments = parseFunctionArgumentNames(beforeIR, coreType);
  return result;
}

inline LifetimeAnalysis
buildMemLivenessAnalysis(const PlanMemoryInput &input,
                         const PreparedMemLivenessAnalysis &prepared,
                         uint32_t randomSeed = 0,
                         bool restrictInplaceAsISA = false,
                         bool materializeStaticStorageFacts = true) {
  (void)input;
  const std::set<std::string> &allocNames = prepared.allocNames;
  const std::vector<std::string> &allocOrder = prepared.allocOrder;
  const std::map<std::string, uint64_t> &allocExtentBits =
      prepared.allocExtentBits;
  const std::vector<OperationRecord> &operations = prepared.operations;
  const PlanMemoryOperationIndex &operationIndex = prepared.operationIndex;
  const Liveness &liveness = prepared.liveness;
  auto GetAllocExtentBits = [&](const std::string &name) {
    auto found = allocExtentBits.find(name);
    return found == allocExtentBits.end() ? uint64_t{0} : found->second;
  };
  BufferAliasMap buffer2AliasVec(operations.size());
  std::unordered_map<std::string, std::string> traceback;
  for (const std::string &alloc : allocNames)
    traceback[alloc] = alloc;

  std::vector<OpInfo> linearOperation;
  std::unordered_map<std::string, BufferStatus> buffer2status;
  std::unordered_map<std::string, int> allocDefOperation;
  std::unordered_map<std::string, int> buffer2LifeAllocTime;
  std::unordered_map<std::string, int> buffer2LifeFreeTime;
  std::map<std::string, uint32_t> buffer2MultiNum;
  // BufferInfo::ignoreInplace starts false. PlanMemory sets it only while
  // processing concrete extra-buffer/select/alloc-alias relationships.
  std::unordered_set<std::string> ignoreInplaceBuffers;
  traceback.reserve(operations.size() + allocNames.size());
  buffer2status.reserve(operations.size());
  allocDefOperation.reserve(allocNames.size());
  buffer2LifeAllocTime.reserve(allocNames.size());
  buffer2LifeFreeTime.reserve(allocNames.size());
  ignoreInplaceBuffers.reserve(allocNames.size());
  std::mt19937 randomGenerator(randomSeed);
  std::vector<std::string> preloadBuffers;
  std::unordered_set<std::string> preloadBufferSet;
  std::vector<std::string> shuffledLiveValues;
  linearOperation.reserve(operations.size());
  preloadBuffers.reserve(allocNames.size());
  preloadBufferSet.reserve(allocNames.size());
  shuffledLiveValues.reserve(allocNames.size());

  struct LoopContext {
    std::vector<std::pair<std::string, std::string>> iterArgs;
    std::vector<std::string> results;
    std::vector<std::string> yielded;
    int region = -1;
  };
  std::vector<LoopContext> loopStack;
  struct WhileContext {
    std::vector<std::pair<std::string, std::string>> iterArgs;
    std::vector<std::string> results;
    std::vector<std::string> afterArguments;
    std::vector<std::string> yielded;
    int beforeRegion = -1;
    int afterRegion = -1;
  };
  std::vector<WhileContext> whileStack;
  struct IfContext {
    std::vector<std::string> results;
    std::vector<std::string> yielded;
    int region = -1;
  };
  std::vector<IfContext> ifStack;
  struct ScopeContext {
    std::vector<std::string> results;
    std::vector<std::string> returned;
    int region = -1;
  };
  std::vector<ScopeContext> scopeStack;

  auto resolveAlloc = [&](const std::string &value)
      -> std::optional<std::string> {
    if (auto direct = canonicalAlloc(traceback, allocNames, value))
      return direct;
    for (const std::string &alias : buffer2AliasVec.GetAliasBuffers(value))
      if (auto root = canonicalAlloc(traceback, allocNames, alias))
        return root;
    return std::nullopt;
  };
  auto UpdateBufferAlias = [&](const std::string &value,
                         const std::string &aliasValue, bool conditional,
                         bool ignoreAliasInplace = false) {
    buffer2AliasVec.UpdateBufferAlias(value, aliasValue, conditional);
    if (!allocNames.count(value))
      buffer2status[value] = BufferStatus::UNDEFFINED;
    if (ignoreAliasInplace && allocNames.count(aliasValue))
      ignoreInplaceBuffers.insert(aliasValue);
  };
  auto GetAliasBuffers = [&](const std::string &value) {
    std::vector<std::string> aliases = buffer2AliasVec.GetAliasBuffers(value);
    if (std::find(aliases.begin(), aliases.end(), value) == aliases.end())
      aliases.push_back(value);
    return aliases;
  };
  // Most gen/preload walks only consume aliases once. Iterate the stable
  // alias order in place instead of allocating and copying a temporary vector
  // for every operand of every attempt.
  auto ForEachAlias = [&](const std::string &value, auto &&visitor) {
    bool includesValue = false;
    for (const BufferAliasMap::AliasPair &pair :
         buffer2AliasVec.GetAliasBufferCondPairs(value)) {
      includesValue |= pair.first == value;
      if (!visitor(pair.first))
        return;
    }
    if (!includesValue)
      (void)visitor(value);
  };
  auto IsPreloadBuffer = [&](const std::string &value) {
    if (preloadBufferSet.empty())
      return false;
    bool found = false;
    ForEachAlias(value, [&](const std::string &alias) {
      found = preloadBufferSet.count(alias) != 0;
      return !found;
    });
    return found;
  };
  auto UpdateOperandGenInfo = [&](OpInfo &info, const std::string &value) {
    auto it = buffer2status.find(value);
    if (it == buffer2status.end())
      return;
    if (it->second == BufferStatus::DEFFINED) {
      if (IsPreloadBuffer(value))
        return;
      info.gen.push_back(value);
      it->second = BufferStatus::GENED;
    } else if (it->second == BufferStatus::KILLED) {
      throw std::runtime_error("PlanMemory: killed buffer used again: " +
                               value + " at operation " +
                               std::to_string(info.index));
    }
  };
  auto UpdateOpGenInfo = [&](OpInfo &info, const std::string &value) {
    ForEachAlias(value, [&](const std::string &alias) {
      UpdateOperandGenInfo(info, alias);
      return true;
    });
  };
  auto AllDeadAfter = [&](const std::vector<std::string> &aliases,
                          int operationId) {
    for (const std::string &alias : aliases)
      if (!liveness.isDeadAfter(alias, operationId) ||
          !liveness.isDeadAfterOp(alias, operationId))
        return false;
    return true;
  };
  auto UpdateOpKillInfo = [&](OpInfo &info, const std::string &operand) {
    std::vector<std::string> aliases = GetAliasBuffers(operand);
    for (const std::string &alias : aliases) {
      auto it = buffer2status.find(alias);
      if (it == buffer2status.end())
        return;
      if (it->second != BufferStatus::GENED)
        continue;
      auto def = allocDefOperation.find(alias);
      if (def == allocDefOperation.end() ||
          !liveness.definingParentDominatedBy(def->second,
                                              info.operation->operationId) ||
          !AllDeadAfter(aliases, info.operation->operationId))
        continue;
      info.kill.push_back(alias);
      it->second = BufferStatus::KILLED;
    }
  };
  auto OpKillHandle = [&](OpInfo &info) {
    const std::vector<std::string> &orderedLive =
        prepared.currentlyLiveValuesOrdered(info.operation->operationId);
    if (orderedLive.empty())
      return;
    shuffledLiveValues.assign(orderedLive.begin(), orderedLive.end());
    std::shuffle(shuffledLiveValues.begin(), shuffledLiveValues.end(),
                 randomGenerator);
    for (const std::string &value : shuffledLiveValues)
      UpdateOpKillInfo(info, value);
  };
  auto GetLiveBuffersInLoop = [&](const OperationRecord &op) {
    std::vector<std::string> result;
    const std::vector<std::string> &orderedLive =
        prepared.currentlyLiveValuesOrdered(op.operationId);
    if (orderedLive.empty())
      return result;
    shuffledLiveValues.assign(orderedLive.begin(), orderedLive.end());
    std::shuffle(shuffledLiveValues.begin(), shuffledLiveValues.end(),
                 randomGenerator);
    for (const std::string &value : shuffledLiveValues)
      ForEachAlias(value, [&](const std::string &alias) {
        if (buffer2status.count(alias))
          result.push_back(alias);
        return true;
      });
    return result;
  };
  auto UpdateLinearOperation = [&](const OperationRecord &op) -> OpInfo & {
    linearOperation.push_back(
        {&op, static_cast<int>(linearOperation.size()), {}, {}});
    return linearOperation.back();
  };
  auto GetChildRegion = [&](const OperationRecord &parent) {
    const std::vector<int> &regions =
        prepared.childRegions(parent.operationId);
    return regions.empty() ? -1 : regions.front();
  };
  auto GetChildRegions = [&](const OperationRecord &parent) {
    return prepared.childRegions(parent.operationId);
  };
  auto GetRegionArguments = [&](int region) {
    return operationIndex.argumentsForRegion(region);
  };
  auto GetRegionYieldedValues = [&](int region) {
    return operationIndex.yieldedValuesForRegion(region);
  };
  auto GetParentOperationId = [&](const OperationRecord &child) {
    if (child.regionPath.empty())
      return -1;
    std::vector<int> parentPath(child.regionPath.begin(),
                                child.regionPath.end() - 1);
    return prepared.parentOperation(parentPath, child.regionPath.back());
  };
  auto IsInRegion = [](const OperationRecord &op, int region) {
    return region >= 0 && !op.regionPath.empty() &&
           op.regionPath.back() == region;
  };
  auto CheckIfUnknownOpTouchBuffer = [&](const OperationRecord &op) {
    static const std::set<std::string> skippableOps = {
        "func.return", "return", "scf.yield", "memref.dim",
        "hivm.hir.dcci", "scope.return"};
    if (skippableOps.count(op.opName))
      return false;
    for (const std::string &operand : operationIndex.operands(op))
      if (resolveAlloc(operand))
        return true;
    for (const std::string &resultValue : operationIndex.results(op))
      if (resolveAlloc(resultValue))
        return true;
    return op.text.find("#hivm.address_space<ub>") != std::string::npos;
  };

  for (const OperationRecord &op : operations) {
    const std::string &line = op.text;
    std::string result = resultNameBeforeEqual(line);
    const std::vector<std::string> &opResults = operationIndex.results(op);
    if (!opResults.empty())
      result = opResults.front();
    if (op.opName == "scf.while") {
      OpInfo &info = UpdateLinearOperation(op);
      for (const std::string &live : GetLiveBuffersInLoop(op))
        UpdateOpGenInfo(info, live);
      WhileContext context;
      context.iterArgs = OperationWhileInitArgPairs(op);
      context.results = opResults;
      std::vector<int> regions = GetChildRegions(op);
      if (!regions.empty())
        context.beforeRegion = regions[0];
      if (regions.size() > 1) {
        context.afterRegion = regions[1];
        context.afterArguments = GetRegionArguments(context.afterRegion);
      }
      for (const auto &pair : context.iterArgs) {
        traceback[pair.first] = pair.second;
        UpdateBufferAlias(pair.first, pair.second, false);
      }
      whileStack.push_back(std::move(context));
      continue;
    }
    if (op.opName == "scope.scope") {
      (void)UpdateLinearOperation(op);
      scopeStack.push_back(
          {opResults, {}, GetChildRegion(op)});
      continue;
    }
    if (op.opName == "scf.for") {
      OpInfo &info = UpdateLinearOperation(op);
      for (const std::string &live : GetLiveBuffersInLoop(op))
        UpdateOpGenInfo(info, live);
      LoopContext context;
      context.iterArgs = OperationIterArgPairs(op);
      context.results = opResults;
      context.region = GetChildRegion(op);
      context.yielded = GetRegionYieldedValues(context.region);
      for (const auto &pair : context.iterArgs) {
        traceback[pair.first] = pair.second;
        UpdateBufferAlias(pair.first, pair.second, false);
      }
      size_t count = std::min(context.yielded.size(),
                              context.iterArgs.size());
      for (size_t i = 0; i < count; ++i)
        UpdateBufferAlias(context.yielded[i], context.iterArgs[i].first,
                          false);
      count = std::min(context.results.size(), context.yielded.size());
      for (size_t i = 0; i < count; ++i) {
        traceback[context.results[i]] = context.yielded[i];
        UpdateBufferAlias(context.results[i], context.yielded[i], false);
      }
      loopStack.push_back(std::move(context));
      continue;
    }
    if (op.opName == "scf.condition") {
      (void)UpdateLinearOperation(op);
      if (!whileStack.empty() &&
          IsInRegion(op, whileStack.back().beforeRegion)) {
        std::vector<std::string> forwarded =
            extractConditionForwardedValues(line);
        size_t count =
            std::min(forwarded.size(), whileStack.back().afterArguments.size());
        for (size_t i = 0; i < count; ++i)
          UpdateBufferAlias(whileStack.back().afterArguments[i], forwarded[i],
                            false);
      }
      continue;
    }
    if (op.opName == "scope.return" && !scopeStack.empty() &&
        IsInRegion(op, scopeStack.back().region)) {
      scopeStack.back().returned = operationIndex.operands(op);
      (void)UpdateLinearOperation(op);
      continue;
    }
    if ((op.opName == "scf.yield" ||
         op.opName == "scf.if.implicit_yield") &&
        !ifStack.empty() && IsInRegion(op, ifStack.back().region)) {
      ifStack.back().yielded = op.opName == "scf.yield"
                                   ? operationIndex.operands(op)
                                   : std::vector<std::string>{};
      (void)UpdateLinearOperation(op);
      continue;
    }
    if (op.opName == "scf.yield" && !whileStack.empty() &&
        IsInRegion(op, whileStack.back().afterRegion)) {
      whileStack.back().yielded = operationIndex.operands(op);
      (void)UpdateLinearOperation(op);
      continue;
    }
    if ((op.opName == "scf.yield" ||
         op.opName == "scf.for.implicit_yield") &&
        !loopStack.empty() && IsInRegion(op, loopStack.back().region)) {
      (void)UpdateLinearOperation(op);
      continue;
    }
    if (op.opName == "scf.while.end") {
      if (whileStack.empty())
        throw std::runtime_error("PlanMemory: unmatched scf.while.end");
      WhileContext context = whileStack.back();
      whileStack.pop_back();
      size_t count = std::min(context.yielded.size(), context.iterArgs.size());
      for (size_t i = 0; i < count; ++i)
        UpdateBufferAlias(context.yielded[i], context.iterArgs[i].first,
                          false);
      count = std::min(context.results.size(), context.afterArguments.size());
      for (size_t i = 0; i < count; ++i) {
        traceback[context.results[i]] = context.afterArguments[i];
        UpdateBufferAlias(context.results[i], context.afterArguments[i],
                          false);
      }
      OpInfo &info = UpdateLinearOperation(op);
      OpKillHandle(info);
      continue;
    }
    if (op.opName == "scope.scope.end") {
      if (scopeStack.empty())
        throw std::runtime_error("PlanMemory: unmatched scope.scope.end");
      ScopeContext context = scopeStack.back();
      scopeStack.pop_back();
      size_t count = std::min(context.results.size(), context.returned.size());
      for (size_t i = 0; i < count; ++i) {
        traceback[context.results[i]] = context.returned[i];
        UpdateBufferAlias(context.results[i], context.returned[i], false);
      }
      OpInfo &info = UpdateLinearOperation(op);
      OpKillHandle(info);
      continue;
    }
    if (op.opName == "scf.for.end") {
      if (loopStack.empty())
        throw std::runtime_error("PlanMemory: unmatched scf.for.end");
      LoopContext context = loopStack.back();
      loopStack.pop_back();
      OpInfo &info = UpdateLinearOperation(op);
      OpKillHandle(info);
      continue;
    }
    if (op.opName == "scf.if") {
      (void)UpdateLinearOperation(op);
      ifStack.push_back(
          {opResults, {}, GetChildRegion(op)});
      continue;
    }
    if (op.opName == "scf.if.else") {
      if (ifStack.empty())
        throw std::runtime_error("PlanMemory: unmatched scf.if.else");
      IfContext &context = ifStack.back();
      size_t count = std::min(context.results.size(), context.yielded.size());
      for (size_t i = 0; i < count; ++i)
        UpdateBufferAlias(context.results[i], context.yielded[i], true);
      context.yielded.clear();
      context.region = GetChildRegion(op);
      (void)UpdateLinearOperation(op);
      continue;
    }
    if (op.opName == "scf.if.end") {
      if (ifStack.empty())
        throw std::runtime_error("PlanMemory: unmatched scf.if.end");
      IfContext context = ifStack.back();
      ifStack.pop_back();
      size_t count = std::min(context.results.size(), context.yielded.size());
      for (size_t i = 0; i < count; ++i)
        UpdateBufferAlias(context.results[i], context.yielded[i], true);
      OpInfo &info = UpdateLinearOperation(op);
      OpKillHandle(info);
      continue;
    }

    OpInfo &info = UpdateLinearOperation(op);
    if (op.opName == "memref.alloc" && allocNames.count(result)) {
      buffer2status[result] = BufferStatus::DEFFINED;
      allocDefOperation[result] = op.operationId;
      continue;
    }
    if (isViewLikeMemrefOp(op.opName)) {
      for (const std::string &operand : operationIndex.operands(op)) {
        if (!resolveAlloc(operand))
          continue;
        traceback[result] = operand;
        UpdateBufferAlias(result, operand, false);
        break;
      }
      continue;
    }
    if (op.opName == "arith.select") {
      const std::vector<std::string> &operands = operationIndex.operands(op);
      if (operands.size() >= 3) {
        UpdateBufferAlias(result, operands[1], true, true);
        UpdateBufferAlias(result, operands[2], true, true);
      }
      OpKillHandle(info);
      continue;
    }
    if (op.opName == "memref.store") {
      const std::vector<std::string> &operands = operationIndex.operands(op);
      if (operands.size() >= 2)
        UpdateOpGenInfo(info, operands[1]);
      OpKillHandle(info);
      continue;
    }
    if (IsDestinationStyleOp(op.opName)) {
      for (const std::string &out : extractGroupSSAs(line, "outs"))
        UpdateOpGenInfo(info, out);
      for (const std::string group : {"temp_buffer", "tmps"}) {
        for (const std::string &temp : extractGroupSSAs(line, group)) {
          if (auto alloc = resolveAlloc(temp))
            ignoreInplaceBuffers.insert(*alloc);
          UpdateOpGenInfo(info, temp);
        }
      }
      OpKillHandle(info);
      continue;
    }
    if (op.opName == "annotation.mark") {
      const std::vector<std::string> &operands = operationIndex.operands(op);
      if (!operands.empty()) {
        if (auto alloc = resolveAlloc(operands.front())) {
          static const std::regex multiBufferRegex(
              R"(hivm\.multi_buffer\s*=\s*([0-9]+))");
          std::smatch multiBuffer;
          if (std::regex_search(line, multiBuffer, multiBufferRegex)) {
            uint32_t count =
                static_cast<uint32_t>(std::stoul(multiBuffer[1].str()));
            if (count > 1)
              buffer2MultiNum[*alloc] = count;
          }
          if (line.find("hivm.preload_local_buffer") !=
                  std::string::npos &&
              preloadBufferSet.insert(*alloc).second)
            preloadBuffers.push_back(*alloc);
          UpdateOpKillInfo(info, *alloc);
        }
      }
      continue;
    }
    if (op.opName == "cf.br" || op.opName == "cf.cond_br") {
      for (const BranchDestination &destination : op.branchDestinations) {
        const std::vector<std::string> *arguments =
            prepared.argumentsForBlock(op.regionPath, destination.label);
        if (!arguments)
          throw std::runtime_error("PlanMemory: branch destination not found: " +
                                   destination.label);
        if (arguments->size() != destination.operands.size())
          throw std::runtime_error(
              "PlanMemory: branch destination operand count mismatch: " +
              destination.label);
        for (size_t i = 0; i < destination.operands.size(); ++i)
          UpdateBufferAlias(arguments->at(i), destination.operands[i],
                            true);
      }
      continue;
    }
    if (op.opName == "memref.load" || op.opName == "hivm.hir.debug") {
      OpKillHandle(info);
      continue;
    }
    if (CheckIfUnknownOpTouchBuffer(op))
      throw std::runtime_error(
          "PlanMemory exact blocker: unrecognized operation touches local "
          "buffer: " +
          op.opName);
  }

  auto UpdatePreloadBuffersGenInfo = [&](OpInfo &info) {
    for (const std::string &preloadBuffer : preloadBuffers)
      ForEachAlias(preloadBuffer, [&](const std::string &buffer) {
        auto status = buffer2status.find(buffer);
        if (status == buffer2status.end() ||
            status->second != BufferStatus::DEFFINED)
          return true;
        info.gen.push_back(buffer);
        status->second = BufferStatus::GENED;
        return true;
      });
  };
  auto UpdatePreloadBuffersKillInfo = [&](OpInfo &info) {
    for (const std::string &preloadBuffer : preloadBuffers)
      ForEachAlias(preloadBuffer, [&](const std::string &buffer) {
        auto status = buffer2status.find(buffer);
        if (status == buffer2status.end() ||
            status->second != BufferStatus::GENED)
          return true;
        info.kill.push_back(buffer);
        status->second = BufferStatus::KILLED;
        return true;
      });
  };
  auto UpdatePreloadBuffersGenKillMap = [&]() {
    int parentForOperationId = -1;
    for (const OpInfo &info : linearOperation) {
      if (info.operation->opName == "scope.scope") {
        parentForOperationId = GetParentOperationId(*info.operation);
        break;
      }
    }
    if (parentForOperationId < 0)
      return;
    size_t count = 0;
    for (OpInfo &info : linearOperation) {
      if (info.operation->operationId != parentForOperationId)
        continue;
      if (count == 0) {
        UpdatePreloadBuffersGenInfo(info);
        ++count;
      } else {
        UpdatePreloadBuffersKillInfo(info);
        break;
      }
    }
  };
  UpdatePreloadBuffersGenKillMap();

  auto GenerateBufferLife = [&]() {
    for (size_t i = 0; i < linearOperation.size(); ++i) {
      for (const std::string &gen : linearOperation[i].gen)
        buffer2LifeAllocTime[gen] = static_cast<int>(i);
      for (const std::string &kill : linearOperation[i].kill)
        buffer2LifeFreeTime[kill] = static_cast<int>(i);
    }
  };
  GenerateBufferLife();

  std::vector<std::pair<std::string, std::string>> inplacePairList;
  auto InitializeInplacePairList = [&]() {
    for (const std::string &buffer : allocOrder) {
      for (const BufferAliasMap::AliasPair &pair :
           buffer2AliasVec.GetAliasBufferCondPairs(buffer)) {
        if (!allocNames.count(pair.first) || pair.second)
          continue;
        bool exists = false;
        for (const auto &current : inplacePairList)
          if ((current.first == buffer && current.second == pair.first) ||
              (current.first == pair.first && current.second == buffer))
            exists = true;
        if (exists)
          continue;
        inplacePairList.push_back({buffer, pair.first});
        ignoreInplaceBuffers.insert(buffer);
        ignoreInplaceBuffers.insert(pair.first);
      }
    }
  };
  InitializeInplacePairList();

  std::set<int> inplaceOpSet;
  std::vector<std::pair<std::string, std::string>> inplacableBufferPairs;

  auto IsInplaceMultiBuffer = [&](
                                  const std::string &src,
                                  const std::vector<std::pair<std::string,
                                                              std::string>>
                                      &inplaceList) {
    auto srcMultiBuffer = buffer2MultiNum.find(src);
    if (srcMultiBuffer != buffer2MultiNum.end() &&
        srcMultiBuffer->second > 1)
      return true;
    for (const auto &pair : inplaceList) {
      std::string pairedBuffer;
      if (pair.first == src)
        pairedBuffer = pair.second;
      else if (pair.second == src)
        pairedBuffer = pair.first;
      else
        continue;
      auto multiBuffer = buffer2MultiNum.find(pairedBuffer);
      if (multiBuffer != buffer2MultiNum.end() && multiBuffer->second > 1)
        return true;
    }
    return false;
  };

  auto VisitDstOpTypeReachable = [&resolveAlloc, &operations,
                                  &operationIndex](
                                     const std::string &src,
                                     const std::string &dstOpName,
                                     const std::vector<std::pair<std::string,
                                                                 std::string>>
                                         &inplaceList) {
    std::set<std::string> visited;
    std::function<bool(const std::string &)> visit =
        [&](const std::string &current) {
          if (!visited.insert(current).second)
            return false;

          // Reproduce the order observed through MLIR Value::getUsers().
          // PlanMemory deliberately returns after the first view-like user,
          // even when that branch does not reach dst, so this order affects
          // whether a later view branch is considered.
          for (size_t position : operationIndex.users(current)) {
            const OperationRecord &operation = operations[position];
            if (operation.opName == dstOpName)
              return true;
            if (isViewLikeMemrefOp(operation.opName)) {
              const std::vector<std::string> &results =
                  operationIndex.results(operation);
              return !results.empty() && visit(results.front());
            }
          }

          const std::optional<std::string> root = resolveAlloc(current);
          if (!root)
            return false;
          for (const auto &pair : inplaceList) {
            if (pair.first == pair.second)
              continue;
            if (pair.first == *root && visit(pair.second))
              return true;
            if (pair.second == *root && visit(pair.first))
              return true;
          }
          return false;
        };
    return visit(src);
  };

  auto HasUser = [&](const std::string &src, const std::string &dstOpName,
                     const std::vector<std::pair<std::string, std::string>>
                         &inplaceList) {
    return VisitDstOpTypeReachable(src, dstOpName, inplaceList);
  };

  auto InplaceStallPipeline = [&](
                                  const std::string &gen,
                                  const std::string &kill,
                                  const std::vector<std::pair<std::string,
                                                              std::string>>
                                      &inplaceList) {
    if (!IsInplaceMultiBuffer(gen, inplaceList) ||
        !IsInplaceMultiBuffer(kill, inplaceList))
      return false;
    if (HasUser(gen, "hivm.hir.store", inplaceList) &&
        HasUser(kill, "hivm.hir.load", inplaceList)) {
      inplacableBufferPairs.push_back({gen, kill});
      return true;
    }
    return false;
  };

  auto GenerateInplaceList = [&]() {
    for (const OpInfo &info : linearOperation) {
      if ((info.gen.empty() && info.kill.empty()) ||
          inplaceOpSet.count(info.operation->operationId))
        continue;
      for (const std::string &gen : info.gen) {
        if (ignoreInplaceBuffers.count(gen))
          continue;
        for (const std::string &kill : info.kill) {
          if (ignoreInplaceBuffers.count(kill) ||
              GetAllocExtentBits(kill) < GetAllocExtentBits(gen))
            continue;
          if (!IsReuseHIVMOp(*info.operation, gen, kill, traceback,
                             allocNames, restrictInplaceAsISA))
            continue;
          if (InplaceStallPipeline(gen, kill, inplacePairList))
            continue;
          inplacePairList.push_back({gen, kill});
          break;
        }
      }
      inplaceOpSet.insert(info.operation->operationId);
    }
  };
  GenerateInplaceList();

  UnionFind storage(allocNames);
  for (const auto &pair : inplacePairList)
    storage.unite(pair.first, pair.second);
  std::map<std::string, std::vector<std::string>> groups = storage.groups();
  std::map<std::string, std::vector<std::string>> valueToGroup;
  for (const auto &group : groups)
    for (const std::string &value : group.second)
      valueToGroup[value] = group.second;

  LifetimeAnalysis analysis;
  for (const OpInfo &info : linearOperation) {
    if (materializeStaticStorageFacts)
      analysis.operations.push_back(*info.operation);
    for (const std::string &gen : info.gen)
      analysis.bufferGenOrder.push_back(gen);
  }
  analysis.inplacePairList = inplacePairList;
  analysis.inplacableBufferPairs = inplacableBufferPairs;
  analysis.buffer2MultiNum = buffer2MultiNum;
  if (materializeStaticStorageFacts)
    for (const OperationRecord &operation : operations) {
      std::vector<std::string> values = operationIndex.operands(operation);
      const std::vector<std::string> &results =
          operationIndex.results(operation);
      values.insert(values.end(), results.begin(), results.end());
      for (const std::string &value : values)
        if (auto alloc = resolveAlloc(value))
          analysis.canonicalAllocByValue[value] = *alloc;
    }
  std::vector<std::string> sortedNames = allocOrder;
  std::sort(sortedNames.begin(), sortedNames.end(), ssaLess);
  for (const std::string &name : sortedNames) {
    LifetimeRecord record;
    record.name = name;
    record.directAllocTime =
        buffer2LifeAllocTime.count(name) ? buffer2LifeAllocTime[name] : -1;
    record.directFreeTime =
        buffer2LifeFreeTime.count(name) ? buffer2LifeFreeTime[name] : -1;
    record.group = valueToGroup[name];
    record.allocTime = record.directAllocTime;
    record.freeTime = record.directFreeTime;
    bool initialized = false;
    for (const std::string &member : record.group) {
      if (!buffer2LifeAllocTime.count(member) ||
          !buffer2LifeFreeTime.count(member))
        continue;
      if (!initialized) {
        record.allocTime = buffer2LifeAllocTime[member];
        record.freeTime = buffer2LifeFreeTime[member];
        initialized = true;
      } else {
        record.allocTime =
            std::min(record.allocTime, buffer2LifeAllocTime[member]);
        record.freeTime =
            std::max(record.freeTime, buffer2LifeFreeTime[member]);
      }
    }
    analysis.records.push_back(std::move(record));
  }
  return analysis;
}

inline LifetimeAnalysis
buildMemLivenessAnalysis(const PlanMemoryInput &input,
                         uint32_t randomSeed = 0,
                         bool restrictInplaceAsISA = false) {
  PreparedMemLivenessAnalysis prepared(input);
  return buildMemLivenessAnalysis(input, prepared, randomSeed,
                                  restrictInplaceAsISA);
}

inline LifetimeAnalysis analyzeLifetimes(const fs::path &beforeIR,
                                         const std::string &coreType = "AIV",
                                         uint32_t randomSeed = 0,
                                         bool restrictInplaceAsISA = false) {
  return buildMemLivenessAnalysis(ParsePlanMemoryInput(beforeIR, coreType),
                                  randomSeed, restrictInplaceAsISA);
}

inline LifetimeAnalysis analyzeLifetimes(const PlanMemoryInput &input,
                                         uint32_t randomSeed = 0,
                                         bool restrictInplaceAsISA = false) {
  return buildMemLivenessAnalysis(input, randomSeed, restrictInplaceAsISA);
}

inline LifetimeAnalysis
analyzeLifetimes(const PlanMemoryInput &input,
                 const PreparedMemLivenessAnalysis &prepared,
                 uint32_t randomSeed = 0,
                 bool restrictInplaceAsISA = false,
                 bool materializeStaticStorageFacts = true) {
  return buildMemLivenessAnalysis(input, prepared, randomSeed,
                                  restrictInplaceAsISA,
                                  materializeStaticStorageFacts);
}

inline std::vector<BufferInfoRecord>
BuildBufferInfos(const PlanMemoryInput &input) {
  std::vector<BufferInfoRecord> result;
  for (const IRAllocRecord &alloc : input.allocations) {
    result.push_back({alloc.name, alloc.constBits, alloc.extentBits,
                      false});
  }
  return result;
}

inline std::vector<BufferInfoRecord> BuildBufferInfos(const fs::path &beforeIR) {
  return BuildBufferInfos(ParsePlanMemoryInput(beforeIR, "AIV"));
}



} // namespace cvub

#endif
