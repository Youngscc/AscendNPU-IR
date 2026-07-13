#ifndef CVPIPELINE_UB_MODEL_CPP_ALIGN_STORAGE_HPP
#define CVPIPELINE_UB_MODEL_CPP_ALIGN_STORAGE_HPP

#include "c4_buffer_record.hpp"

namespace cvub {

struct C4OperandType {
  std::string buffer;
  MemRefTypeModel type;
  size_t operandNumber = 0;
  bool isDpsInit = false;
};

struct AlignStorageResult {
  std::map<std::string, std::map<size_t, uint64_t>> allocAlignments;
  std::map<std::string, std::set<size_t>> strideAlignments;
};

inline std::optional<size_t> C4RootDimToOperandDim(
    const C4BufferRecord &record, const MemRefTypeModel &operandType,
    size_t rootDimension);

inline bool C4IsLastDimContiguous(const MemRefTypeModel &type) {
  return type.shape.empty() ||
         (type.strides.back() && *type.strides.back() == 1) ||
         (type.shape.back() && *type.shape.back() == 1);
}

inline std::vector<std::vector<size_t>> C4ContinuousReassociation(
    const std::vector<MemRefTypeModel> &types) {
  const std::vector<bool> contiguous = GetContiguousAxes(types);
  std::vector<std::vector<size_t>> result;
  for (size_t axis = 0; axis < contiguous.size(); ++axis) {
    if (axis == 0 || !contiguous[axis])
      result.push_back({});
    result.back().push_back(axis);
  }
  return result;
}

inline std::vector<std::vector<size_t>> C4UniformReassociationPipeline(
    const std::vector<MemRefTypeModel> &types) {
  if (types.empty() || types.front().shape.empty())
    return {};
  const size_t rank = types.front().shape.size();
  std::vector<bool> unit(rank, true);
  for (const MemRefTypeModel &type : types) {
    if (type.shape.size() != rank)
      throw std::runtime_error("flatten operand rank mismatch");
    for (size_t axis = 0; axis < rank; ++axis)
      unit[axis] = unit[axis] && type.shape[axis] &&
                   *type.shape[axis] == 1;
  }

  std::vector<std::vector<size_t>> unitReassociation(1);
  size_t axis = 0;
  while (axis < rank && unit[axis])
    unitReassociation.back().push_back(axis++);
  bool firstNonUnit = true;
  while (axis < rank) {
    if (!firstNonUnit)
      unitReassociation.push_back({});
    firstNonUnit = false;
    unitReassociation.back().push_back(axis++);
    while (axis < rank && unit[axis])
      unitReassociation.back().push_back(axis++);
  }
  if (unitReassociation.front().empty())
    unitReassociation.erase(unitReassociation.begin());

  std::vector<MemRefTypeModel> unitCollapsed;
  unitCollapsed.reserve(types.size());
  for (const MemRefTypeModel &type : types)
    unitCollapsed.push_back(CollapseMemRefType(type, unitReassociation));
  const std::vector<std::vector<size_t>> uniform =
      C4ContinuousReassociation(unitCollapsed);
  std::vector<std::vector<size_t>> composed;
  composed.reserve(uniform.size());
  for (const std::vector<size_t> &group : uniform) {
    composed.push_back({});
    for (size_t intermediateAxis : group) {
      if (intermediateAxis >= unitReassociation.size())
        throw std::runtime_error("flatten reassociation composition failed");
      composed.back().insert(composed.back().end(),
                             unitReassociation[intermediateAxis].begin(),
                             unitReassociation[intermediateAxis].end());
    }
  }
  return composed;
}

inline std::optional<size_t> C4LastNotUnitAxis(
    const std::vector<MemRefTypeModel> &types,
    const std::vector<size_t> &group) {
  for (auto iterator = group.rbegin(); iterator != group.rend(); ++iterator)
    if (std::any_of(types.begin(), types.end(), [&](const MemRefTypeModel &type) {
          return !type.shape[*iterator] || *type.shape[*iterator] != 1;
        }))
      return *iterator;
  return std::nullopt;
}

inline std::optional<size_t> C4GetLastUnContinuousDim(
    const std::vector<MemRefTypeModel> &types) {
  const std::vector<std::vector<size_t>> reassociation =
      C4ContinuousReassociation(types);
  if (reassociation.empty())
    return std::nullopt;
  bool lastUnitStride = true;
  for (const MemRefTypeModel &type : types) {
    const MemRefTypeModel flattened = CollapseMemRefType(type, reassociation);
    lastUnitStride = lastUnitStride && C4IsLastDimContiguous(flattened);
  }
  if (!lastUnitStride)
    return C4LastNotUnitAxis(types, reassociation.back());
  if (reassociation.size() == 1)
    return std::nullopt;
  return C4LastNotUnitAxis(types, reassociation[reassociation.size() - 2]);
}

inline std::optional<size_t> C4GetLastUnContinuousDimWithBarriers(
    const std::vector<MemRefTypeModel> &types,
    const std::vector<int64_t> &barrierDims) {
  std::vector<std::vector<size_t>> reassociation =
      C4ContinuousReassociation(types);
  std::set<size_t> barriers;
  for (int64_t dimension : barrierDims)
    if (dimension >= 0)
      barriers.insert(static_cast<size_t>(dimension));
  if (!barriers.empty()) {
    reassociation.clear();
    const std::vector<bool> contiguous = GetContiguousAxes(types);
    for (size_t axis = 0; axis < contiguous.size(); ++axis) {
      const bool boundary =
          axis > 0 && (barriers.count(axis - 1) != barriers.count(axis));
      if (axis == 0 || !contiguous[axis] || boundary)
        reassociation.push_back({});
      reassociation.back().push_back(axis);
    }
  }
  if (reassociation.empty())
    return std::nullopt;
  bool lastUnitStride = true;
  for (const MemRefTypeModel &type : types)
    lastUnitStride =
        lastUnitStride && C4IsLastDimContiguous(
                              CollapseMemRefType(type, reassociation));
  if (!lastUnitStride)
    return C4LastNotUnitAxis(types, reassociation.back());
  if (reassociation.size() == 1)
    return std::nullopt;
  return C4LastNotUnitAxis(types, reassociation[reassociation.size() - 2]);
}

inline std::vector<std::vector<size_t>> C4ReassociationWithBarriers(
    const std::vector<MemRefTypeModel> &types,
    const std::vector<int64_t> &barrierDims) {
  std::vector<std::vector<size_t>> reassociation =
      C4ContinuousReassociation(types);
  std::set<size_t> barriers;
  for (int64_t dimension : barrierDims)
    if (dimension >= 0)
      barriers.insert(static_cast<size_t>(dimension));
  if (barriers.empty())
    return reassociation;
  reassociation.clear();
  const std::vector<bool> contiguous = GetContiguousAxes(types);
  for (size_t axis = 0; axis < contiguous.size(); ++axis) {
    const bool boundary =
        axis > 0 && (barriers.count(axis - 1) != barriers.count(axis));
    if (axis == 0 || !contiguous[axis] || boundary)
      reassociation.push_back({});
    reassociation.back().push_back(axis);
  }
  return reassociation;
}

inline std::vector<std::vector<size_t>>
C4UniformReassociationPipelineWithBarriers(
    const std::vector<MemRefTypeModel> &types,
    const std::vector<int64_t> &barrierDims) {
  if (types.empty() || types.front().shape.empty())
    return {};
  const size_t rank = types.front().shape.size();
  std::set<size_t> barriers;
  for (int64_t dimension : barrierDims)
    if (dimension >= 0 && static_cast<size_t>(dimension) < rank)
      barriers.insert(static_cast<size_t>(dimension));
  std::vector<bool> unit(rank, true);
  for (const MemRefTypeModel &type : types)
    for (size_t axis = 0; axis < rank; ++axis)
      unit[axis] = unit[axis] && type.shape[axis] &&
                   *type.shape[axis] == 1 && barriers.count(axis) == 0;

  std::vector<std::vector<size_t>> unitReassociation(1);
  size_t axis = 0;
  while (axis < rank && unit[axis])
    unitReassociation.back().push_back(axis++);
  bool firstNonUnit = true;
  while (axis < rank) {
    if (!firstNonUnit)
      unitReassociation.push_back({});
    firstNonUnit = false;
    unitReassociation.back().push_back(axis++);
    while (axis < rank && unit[axis])
      unitReassociation.back().push_back(axis++);
  }
  if (unitReassociation.front().empty())
    unitReassociation.erase(unitReassociation.begin());

  std::vector<int64_t> adjustedBarriers;
  for (size_t group = 0; group < unitReassociation.size(); ++group)
    for (size_t originalAxis : unitReassociation[group])
      if (barriers.count(originalAxis) != 0) {
        adjustedBarriers.push_back(static_cast<int64_t>(group));
        break;
      }
  std::vector<MemRefTypeModel> unitCollapsed;
  for (const MemRefTypeModel &type : types)
    unitCollapsed.push_back(CollapseMemRefType(type, unitReassociation));
  const std::vector<std::vector<size_t>> uniform =
      C4ReassociationWithBarriers(unitCollapsed, adjustedBarriers);
  std::vector<std::vector<size_t>> composed;
  for (const std::vector<size_t> &group : uniform) {
    composed.push_back({});
    for (size_t intermediateAxis : group)
      composed.back().insert(composed.back().end(),
                             unitReassociation.at(intermediateAxis).begin(),
                             unitReassociation.at(intermediateAxis).end());
  }
  return composed;
}

inline std::optional<size_t> C4ReassociatedDimension(
    const std::vector<std::vector<size_t>> &reassociation,
    size_t dimension) {
  for (size_t group = 0; group < reassociation.size(); ++group)
    if (std::find(reassociation[group].begin(), reassociation[group].end(),
                  dimension) != reassociation[group].end())
      return group;
  return std::nullopt;
}

inline std::optional<size_t> C4PreviousUncontinuousDimension(
    const std::vector<MemRefTypeModel> &types,
    const std::vector<std::vector<size_t>> &reassociation,
    size_t flattenedDimension) {
  if (flattenedDimension == 0)
    return std::nullopt;
  for (size_t reverse = flattenedDimension; reverse > 0; --reverse)
    if (const std::optional<size_t> dimension =
            C4LastNotUnitAxis(types, reassociation[reverse - 1]))
      return dimension;
  return std::nullopt;
}

inline bool C4AlreadyAligned(const MemRefTypeModel &type, size_t alignDim) {
  const uint64_t bitWidth = GetElementBitWidth(type);
  if (alignDim >= type.shape.size() || bitWidth == 0)
    return true;
  if (type.hasStridedLayout && type.strides[alignDim] &&
      CheckedMul(static_cast<uint64_t>(*type.strides[alignDim]), bitWidth,
                 "C4 explicit stride bits") %
              256 ==
          0)
    return true;
  bool allOuterUnit = true;
  for (size_t axis = 0; axis <= alignDim; ++axis)
    allOuterUnit = allOuterUnit && type.shape[axis] && *type.shape[axis] == 1;
  if (allOuterUnit)
    return true;
  uint64_t innerElements = 1;
  for (size_t axis = alignDim + 1; axis < type.shape.size(); ++axis) {
    if (!type.shape[axis])
      return false;
    innerElements = CheckedMul(innerElements,
                               static_cast<uint64_t>(*type.shape[axis]),
                               "C4 stride inner elements");
  }
  return CheckedMul(innerElements, bitWidth, "C4 stride inner bits") % 256 ==
         0;
}

inline uint64_t C4AlignedBufferBits(const MemRefTypeModel &type,
                                    const std::set<size_t> &alignDims,
                                    const std::vector<std::optional<int64_t>>
                                        *upperBounds = nullptr) {
  if (alignDims.empty()) {
    std::string plain = "memref<";
    for (const std::optional<int64_t> &extent : type.shape) {
      if (!extent)
        throw std::runtime_error("C4: dynamic aligned buffer");
      plain += std::to_string(*extent) + "x";
    }
    uint64_t elements = 1;
    for (size_t axis = 0; axis < type.shape.size(); ++axis) {
      const std::optional<int64_t> extent =
          type.shape[axis] ? type.shape[axis]
                           : upperBounds && axis < upperBounds->size()
                                 ? (*upperBounds)[axis]
                                 : std::nullopt;
      if (!extent)
        throw std::runtime_error("C4: missing dynamic allocation upper bound");
      elements = CheckedMul(elements, static_cast<uint64_t>(*extent),
                            "C4 allocation element count");
    }
    return CheckedMul(elements, GetElementBitWidth(type),
                      "C4 allocation bits");
  }
  const uint64_t bitWidth = GetElementBitWidth(type);
  if (bitWidth == 0 || 256 % bitWidth != 0)
    throw std::runtime_error("C4: invalid stride alignment element width");
  std::vector<uint64_t> targets(type.shape.size(), 1);
  for (size_t axis : alignDims)
    targets.at(axis) = std::lcm(targets.at(axis), 256 / bitWidth);
  std::vector<uint64_t> units(type.shape.size() + 1, 1);
  uint64_t innerAlignedUnits = 1;
  uint64_t shapeAccumulation = 1;
  for (size_t reverse = type.shape.size(); reverse > 0; --reverse) {
    const size_t axis = reverse - 1;
    const uint64_t next = std::lcm(innerAlignedUnits, targets[axis]);
    units[axis + 1] = shapeAccumulation % next == 0
                          ? 1
                          : next / innerAlignedUnits;
    innerAlignedUnits = next;
    if (type.shape[axis])
      shapeAccumulation = CheckedMul(
          shapeAccumulation,
          std::lcm(static_cast<uint64_t>(*type.shape[axis]), units[axis + 1]),
          "C4 aligned shape accumulation");
  }
  uint64_t elements = 1;
  for (size_t axis = 0; axis < type.shape.size(); ++axis) {
    const uint64_t unit = units[axis];
    uint64_t aligned = 0;
    if (type.shape[axis]) {
      const uint64_t extent = static_cast<uint64_t>(*type.shape[axis]);
      aligned = unit == 1 ? extent : AlignUp(extent, unit);
    } else {
      if (!upperBounds || axis >= upperBounds->size() ||
          !(*upperBounds)[axis])
        throw std::runtime_error("C4: missing dynamic allocation upper bound");
      const uint64_t bound =
          static_cast<uint64_t>(*(*upperBounds)[axis]);
      aligned = unit == 1
                    ? bound
                    : CheckedAdd(bound, unit - 1,
                                 "C4 dynamic aligned closed upper bound");
    }
    elements = CheckedMul(elements, aligned, "C4 aligned allocation shape");
  }
  elements = CheckedMul(elements, units.back(), "C4 aligned trailing shape");
  return CheckedMul(elements, bitWidth, "C4 aligned allocation bits");
}

inline uint64_t C4SizeAlignedBufferBits(
    const MemRefTypeModel &type,
    const std::map<size_t, uint64_t> &alignBytesByDim) {
  const uint64_t bitWidth = GetElementBitWidth(type);
  if (bitWidth == 0 || bitWidth % 8 != 0)
    throw std::runtime_error("C4: invalid size alignment element width");
  const uint64_t elementBytes = bitWidth / 8;
  uint64_t elements = 1;
  for (size_t axis = 0; axis < type.shape.size(); ++axis) {
    if (!type.shape[axis])
      throw std::runtime_error("C4: dynamic size-aligned allocation");
    uint64_t extent = static_cast<uint64_t>(*type.shape[axis]);
    auto alignment = alignBytesByDim.find(axis);
    if (alignment != alignBytesByDim.end()) {
      if (alignment->second == 0 || alignment->second % elementBytes != 0)
        throw std::runtime_error("C4: invalid allocation alignment bytes");
      extent = AlignUp(extent, alignment->second / elementBytes);
    }
    elements = CheckedMul(elements, extent, "C4 size-aligned shape");
  }
  return CheckedMul(elements, bitWidth, "C4 size-aligned bits");
}

inline bool C4HasBooleanProperty(const C1OperationRecord &operation,
                                 const std::string &name) {
  return FindDictionaryValue(operation.properties, name) == "true" ||
         FindDictionaryValue(operation.attributes, name) == "true";
}

inline std::vector<int64_t>
C4GetLimitedAxes(const C1OperationRecord &operation) {
  const auto arrayProperty = [&](const std::string &name) {
    std::string value = FindDictionaryValue(operation.properties, name);
    if (value.empty())
      value = FindDictionaryValue(operation.attributes, name);
    return DecomposeI64Array(value);
  };
  if (operation.name == "hivm.hir.vreduce")
    return arrayProperty("reduce_dims");
  if (operation.name == "hivm.hir.vcumsum")
    return arrayProperty("cum_dims");
  if (operation.name == "hivm.hir.vconcat") {
    std::string value = FindDictionaryValue(operation.properties, "dim");
    if (value.empty())
      value = FindDictionaryValue(operation.attributes, "dim");
    return value.empty() ? std::vector<int64_t>{}
                         : std::vector<int64_t>{std::stoll(value)};
  }
  return {};
}

inline void C4MarkSizeAlignment(
    const C4OperandType &operand, size_t axis, uint64_t alignmentBytes,
    const std::vector<C4BufferRecord> &buffers,
    std::map<std::string, std::map<size_t, uint64_t>> &marks) {
  if (axis >= operand.type.shape.size() || !operand.type.shape[axis]) {
    const C4BufferRecord *record =
        C4FindSourceBuffer(buffers, operand.buffer);
    if (record && record->addressSpace == AddressSpace::UB)
      marks[record->sourceIdentity][axis] = std::max(
          marks[record->sourceIdentity][axis], alignmentBytes);
    return;
  }
  const uint64_t elementBytes = GetElementBitWidth(operand.type) / 8;
  if (elementBytes == 0 ||
      (static_cast<uint64_t>(*operand.type.shape[axis]) * elementBytes) %
              alignmentBytes ==
          0)
    return;
  const C4BufferRecord *record = C4FindSourceBuffer(buffers, operand.buffer);
  if (record && record->addressSpace == AddressSpace::UB)
    marks[record->sourceIdentity][axis] =
        std::max(marks[record->sourceIdentity][axis], alignmentBytes);
}

inline std::vector<size_t>
C4TransposeLoopDims(const C1OperationRecord &operation, size_t rank) {
  std::vector<int64_t> permutation = DecomposeI64Array(
      FindDictionaryValue(operation.properties, "permutation"));
  if (permutation.empty())
    permutation = DecomposeI64Array(
        FindDictionaryValue(operation.attributes, "permutation"));
  std::vector<size_t> result;
  for (size_t axis = 0; axis < permutation.size() && axis < rank; ++axis)
    if (permutation[axis] != static_cast<int64_t>(axis))
      result.push_back(axis);
  return result;
}

inline void C4MarkVCastAllocSize(
    const C1OperationRecord &operation,
    const std::vector<C4OperandType> &operands,
    const std::vector<C4BufferRecord> &buffers,
    std::map<std::string, std::map<size_t, uint64_t>> &marks) {
  if (operands.size() < 2)
    return;
  const uint64_t sourceBits = GetElementBitWidth(operands.front().type);
  const uint64_t destinationBits = GetElementBitWidth(operands.back().type);
  if (!((sourceBits == 32 || sourceBits == 16) && destinationBits == 8))
    return;
  const size_t rank = operands.front().type.shape.size();
  if (rank == 0 || operands.back().type.shape.size() != rank)
    return;
  const uint64_t sourceBytes = sourceBits / 8;
  const uint64_t bytesFactor = sourceBits / destinationBits;
  if (rank == 1) {
    const std::optional<int64_t> sourceExtent = operands.front().type.shape[0];
    uint64_t sourceAlignment = 32 * 32 * bytesFactor;
    if (sourceExtent && *sourceExtent <= 32)
      sourceAlignment =
          AlignUp(static_cast<uint64_t>(*sourceExtent) * bytesFactor, 32);
    C4MarkSizeAlignment(operands.front(), 0, sourceAlignment, buffers, marks);
    C4MarkSizeAlignment(operands.back(), 0, 32 * 32, buffers, marks);
    return;
  }
  C4MarkSizeAlignment(operands.front(), rank - 1, 32, buffers, marks);
  C4MarkSizeAlignment(operands.front(), rank - 2, 32 * bytesFactor, buffers,
                      marks);
  C4MarkSizeAlignment(operands.back(), rank - 1, 32, buffers, marks);
  C4MarkSizeAlignment(operands.back(), rank - 2, 32, buffers, marks);
  (void)sourceBytes;
  (void)operation;
}

inline void C4MarkVTransposeAllocSize(
    const C1OperationRecord &operation,
    const std::vector<C4OperandType> &operands,
    const std::vector<C4BufferRecord> &buffers,
    std::map<std::string, std::map<size_t, uint64_t>> &marks) {
  if (operands.size() < 2 || operands.front().type.shape.empty() ||
      C4HasBooleanProperty(operation, "disable_align"))
    return;
  const size_t rank = operands.front().type.shape.size();
  const std::vector<size_t> dimensions = C4TransposeLoopDims(operation, rank);
  if (dimensions.empty() ||
      std::find(dimensions.begin(), dimensions.end(), rank - 1) ==
          dimensions.end())
    return;
  for (const C4OperandType &operand : operands)
    for (size_t axis : dimensions)
      C4MarkSizeAlignment(operand, axis, 32, buffers, marks);
  if (GetElementBitWidth(operands.front().type) != 32 ||
      dimensions.size() != 2)
    return;
  bool doubleAligned = false;
  for (size_t axis : dimensions) {
    const std::optional<int64_t> extent = operands.front().type.shape[axis];
    if (extent && AlignUp(static_cast<uint64_t>(*extent) * 4, 32) % 64 == 0)
      doubleAligned = true;
  }
  if (doubleAligned)
    return;
  C4MarkSizeAlignment(operands.front(), dimensions[1], 64, buffers, marks);
  C4MarkSizeAlignment(operands.back(), dimensions[0], 64, buffers, marks);
}

inline void C4MarkVSortAllocSize(
    const std::vector<C4OperandType> &operands,
    const std::vector<C4BufferRecord> &buffers,
    std::map<std::string, std::map<size_t, uint64_t>> &marks) {
  for (const C4OperandType &operand : operands) {
    if (operand.type.shape.empty())
      continue;
    const uint64_t elementBytes = GetElementBitWidth(operand.type) / 8;
    if (elementBytes == 0)
      continue;
    const uint64_t elementsPerBlock = 32 / elementBytes;
    C4MarkSizeAlignment(operand, operand.type.shape.size() - 1,
                        32 * (32 / elementsPerBlock), buffers, marks);
  }
}

inline void C4MarkBufferizationConflictCopies(
    const C3SemanticIR &c3, const std::vector<C4BufferRecord> &buffers,
    std::map<std::string, std::set<size_t>> &marks) {
  const C1SemanticModule &module = c3.bufferized.logicalModule;
  for (size_t ordinal = 0; ordinal < c3.singlePoint.allocations.size();
       ++ordinal) {
    const BufferAllocation &allocation = c3.singlePoint.allocations[ordinal];
    if (!startsWith(allocation.source, "operand:"))
      continue;
    const size_t first = allocation.source.find(':');
    const size_t second = allocation.source.find(':', first + 1);
    if (first == std::string::npos || second == std::string::npos)
      continue;
    const int operationId =
        std::stoi(allocation.source.substr(first + 1, second - first - 1));
    const size_t operand =
        static_cast<size_t>(std::stoull(allocation.source.substr(second + 1)));
    const C1OperationRecord &operation =
        module.operations.at(static_cast<size_t>(operationId));
    if (operand >= operation.operands.size())
      continue;
    const BufferizedValueBinding *original = nullptr;
    for (const BufferizedValueBinding &binding : c3.bufferized.values)
      if (binding.valueId == operation.operands[operand]) {
        original = &binding;
        break;
      }
    if (!original)
      continue;
    const std::string sourceIdentity = C4MappedBufferIdentity(
        original->bufferId, c3.singlePoint.bufferMapping);
    const std::string destinationIdentity =
        "base:" + std::to_string(ordinal);
    const C4BufferRecord *source =
        C4FindSourceBuffer(buffers, sourceIdentity);
    const C4BufferRecord *destination =
        C4FindSourceBuffer(buffers, destinationIdentity);
    std::optional<MemRefTypeModel> sourceType =
        ParseMemRefType(allocation.type);
    const std::optional<MemRefTypeModel> rootType =
        source ? ParseMemRefType(source->type) : std::nullopt;
    const std::optional<MemRefTypeModel> destinationType =
        destination ? ParseMemRefType(destination->type) : std::nullopt;
    if (!source || !destination || !sourceType || !rootType ||
        !destinationType || sourceType->shape.size() != rootType->shape.size())
      continue;
    sourceType->hasStridedLayout = true;
    sourceType->strides = rootType->strides;
    const std::vector<MemRefTypeModel> types = {*sourceType, *destinationType};
    const std::optional<size_t> alignDim = C4GetLastUnContinuousDim(types);
    if (!alignDim)
      continue;
    if (source->addressSpace == AddressSpace::UB &&
        !C4AlreadyAligned(*sourceType, *alignDim))
      marks[source->sourceIdentity].insert(*alignDim);
    if (destination->addressSpace == AddressSpace::UB &&
        !C4AlreadyAligned(*destinationType, *alignDim))
      marks[destination->sourceIdentity].insert(*alignDim);
  }
}

inline std::vector<std::vector<size_t>> C4StaticShapePartition(
    const std::vector<std::optional<int64_t>> &source,
    const std::vector<std::optional<int64_t>> &target) {
  std::vector<std::vector<size_t>> result;
  size_t sourceAxis = 0;
  for (size_t targetAxis = 0; targetAxis < target.size(); ++targetAxis) {
    const std::optional<int64_t> &targetExtent = target[targetAxis];
    result.push_back({});
    std::optional<int64_t> product = 1;
    while (sourceAxis < source.size()) {
      result.back().push_back(sourceAxis);
      if (!product || !source[sourceAxis])
        product = std::nullopt;
      else
        product = detail::CheckedShapeProduct(*product, *source[sourceAxis]);
      ++sourceAxis;
      if (product == targetExtent) {
        const size_t remainingTargets = target.size() - targetAxis - 1;
        while (sourceAxis < source.size() && source[sourceAxis] == 1 &&
               source.size() - sourceAxis > remainingTargets) {
          result.back().push_back(sourceAxis++);
        }
        break;
      }
      if (targetExtent && product && *product > *targetExtent)
        return {};
    }
  }
  return sourceAxis == source.size() ? result
                                     : std::vector<std::vector<size_t>>{};
}

inline std::optional<size_t> C4RootDimToOperandDim(
    const C4BufferRecord &record, const MemRefTypeModel &operandType,
    size_t rootDimension) {
  const std::optional<MemRefTypeModel> rootType = ParseMemRefType(record.type);
  if (!rootType)
    return std::nullopt;
  if (rootType->shape.size() == operandType.shape.size())
    return rootDimension < operandType.shape.size()
               ? std::optional<size_t>(rootDimension)
               : std::nullopt;
  if (rootType->shape.size() > operandType.shape.size()) {
    const std::vector<std::vector<size_t>> groups =
        C4StaticShapePartition(rootType->shape, operandType.shape);
    for (size_t axis = 0; axis < groups.size(); ++axis)
      if (std::find(groups[axis].begin(), groups[axis].end(), rootDimension) !=
          groups[axis].end())
        return axis;
  }
  if (rootType->shape.size() < operandType.shape.size()) {
    const std::vector<std::vector<size_t>> groups =
        C4StaticShapePartition(operandType.shape, rootType->shape);
    if (rootDimension >= groups.size())
      return std::nullopt;
    for (auto iterator = groups[rootDimension].rbegin();
         iterator != groups[rootDimension].rend(); ++iterator)
      if (!operandType.shape[*iterator] || operandType.shape[*iterator] != 1)
        return *iterator;
    return groups[rootDimension].empty()
               ? std::nullopt
               : std::optional<size_t>(groups[rootDimension].back());
  }
  return std::nullopt;
}

inline std::optional<size_t> C4OperandDimToRootDim(
    const C4BufferRecord &record, const MemRefTypeModel &operandType,
    size_t operandDimension) {
  const std::optional<MemRefTypeModel> rootType = ParseMemRefType(record.type);
  if (!rootType)
    return std::nullopt;
  if (rootType->shape.size() == operandType.shape.size())
    return operandDimension < rootType->shape.size()
               ? std::optional<size_t>(operandDimension)
               : std::nullopt;
  if (rootType->shape.size() > operandType.shape.size()) {
    const std::vector<std::vector<size_t>> groups =
        C4StaticShapePartition(rootType->shape, operandType.shape);
    if (operandDimension >= groups.size())
      return std::nullopt;
    for (auto iterator = groups[operandDimension].rbegin();
         iterator != groups[operandDimension].rend(); ++iterator)
      if (!rootType->shape[*iterator] || *rootType->shape[*iterator] != 1)
        return *iterator;
    return groups[operandDimension].empty()
               ? std::nullopt
               : std::optional<size_t>(groups[operandDimension].back());
  }
  if (rootType->shape.size() < operandType.shape.size()) {
    const std::vector<std::vector<size_t>> groups =
        C4StaticShapePartition(operandType.shape, rootType->shape);
    for (size_t rootAxis = 0; rootAxis < groups.size(); ++rootAxis)
      if (std::find(groups[rootAxis].begin(), groups[rootAxis].end(),
                    operandDimension) != groups[rootAxis].end())
        return rootAxis;
  }
  return std::nullopt;
}

inline std::vector<C4OperandType> C4OperationOperandTypes(
    const C3SemanticIR &c3, const C1OperationRecord &operation,
    const std::vector<C4BufferRecord> &buffers,
    const std::map<std::string, std::set<size_t>> *strideMarks = nullptr) {
  std::map<size_t, std::string> accesses;
  for (const BufferizedOperandAccess &access : c3.bufferized.accesses)
    if (access.operationId == operation.id)
      accesses[static_cast<size_t>(access.operandNumber)] =
          C4MappedBufferIdentity(access.bufferId, c3.singlePoint.bufferMapping);
  std::vector<C4OperandType> result;
  const std::vector<size_t> initIndices = C1DpsInitOperandIndices(
      operation.name, operation.operands.size(), operation.properties);
  const std::set<size_t> initSet(initIndices.begin(), initIndices.end());
  for (size_t index = 0; index < operation.operandTypes.size(); ++index) {
    if (!IsMemRefType(operation.operandTypes[index]) &&
        !IsTensorType(operation.operandTypes[index]))
      continue;
    const std::string buffer = accesses[index];
    const std::string typeText =
        IsTensorType(operation.operandTypes[index])
            ? ConvertTensorToMemRefType(operation.operandTypes[index])
            : operation.operandTypes[index];
    std::optional<MemRefTypeModel> type = ParseMemRefType(typeText);
    if (type && IsTensorType(operation.operandTypes[index])) {
      const std::map<int, const C1OperationRecord *> definitions =
          C1DefiningOperations(c3.bufferized.logicalModule);
      auto definition = definitions.find(operation.operands[index]);
      if (definition != definitions.end() &&
          definition->second->name == "tensor.extract_slice") {
        const BufferizedValueBinding *binding = nullptr;
        for (const BufferizedValueBinding &candidate : c3.bufferized.values)
          if (candidate.valueId == operation.operands[index]) {
            binding = &candidate;
            break;
          }
        if (binding) {
          const std::string rootIdentity = C4MappedBufferIdentity(
              binding->bufferId, c3.singlePoint.bufferMapping);
          const C4BufferRecord *root =
              C4FindSourceBuffer(buffers, rootIdentity);
          const std::optional<MemRefTypeModel> rootType =
              root ? ParseMemRefType(root->type) : std::nullopt;
          if (rootType && rootType->shape.size() == type->shape.size()) {
            type->hasStridedLayout = true;
            type->strides = rootType->strides;
          }
        }
      }
    }
    if (type && strideMarks) {
      auto marked = strideMarks->find(buffer);
      if (marked != strideMarks->end()) {
        std::vector<std::pair<int64_t, int64_t>> alignments;
        const C4BufferRecord *record = C4FindSourceBuffer(buffers, buffer);
        for (size_t rootAxis : marked->second) {
          const std::optional<size_t> axis =
              record ? C4RootDimToOperandDim(*record, *type, rootAxis)
                     : std::nullopt;
          if (axis)
            alignments.push_back(
                {static_cast<int64_t>(*axis), static_cast<int64_t>(32)});
        }
        *type = ApplyStrideAlignment(*type, alignments);
      }
    }
    if (type)
      result.push_back({buffer, *type, index, initSet.count(index) != 0});
  }
  return result;
}

inline AlignStorageResult
ModelAlignStorage(const C3SemanticIR &c3,
                  std::vector<C4BufferRecord> &buffers) {
  std::map<std::string, std::map<size_t, uint64_t>> sizeMarks;
  for (const C1OperationRecord &operation :
       c3.bufferized.logicalModule.operations) {
    if (!IsHIVMStructuredOp(operation.name))
      continue;
    const std::vector<C4OperandType> operands =
        C4OperationOperandTypes(c3, operation, buffers);
    if (operation.name == "hivm.hir.vcast") {
      const C1OperationRecord *function = C4EnclosingFunction(
          c3.bufferized.logicalModule, operation);
      if (function && !HasDictionaryEntry(
                          function->attributes,
                          "hivm.disable_size_align_for_cast"))
        C4MarkVCastAllocSize(operation, operands, buffers, sizeMarks);
    } else if (operation.name == "hivm.hir.vtranspose") {
      C4MarkVTransposeAllocSize(operation, operands, buffers, sizeMarks);
    } else if (operation.name == "hivm.hir.vsort") {
      C4MarkVSortAllocSize(operands, buffers, sizeMarks);
    }
  }

  std::map<std::string, std::set<size_t>> marks;
  C4MarkBufferizationConflictCopies(c3, buffers, marks);
  for (const C1OperationRecord &operation :
       c3.bufferized.logicalModule.operations) {
      if (!IsHIVMStructuredOp(operation.name))
        continue;
      const std::vector<C4OperandType> operands =
          C4OperationOperandTypes(c3, operation, buffers, &marks);
      if (operands.empty())
        continue;
      const size_t rank = operands.front().type.shape.size();
      if (rank == 0 || std::any_of(operands.begin(), operands.end(),
                                   [&](const C4OperandType &operand) {
                                     return operand.type.shape.size() != rank;
                                   }))
        continue;
      if (operation.name == "hivm.hir.vtranspose") {
        const std::vector<size_t> dimensions =
            C4TransposeLoopDims(operation, rank);
        if (std::find(dimensions.begin(), dimensions.end(), rank - 1) !=
            dimensions.end())
          continue;
        for (const C4OperandType &operand : operands) {
          std::optional<size_t> operandAlignDim;
          for (auto iterator = dimensions.rbegin();
               iterator != dimensions.rend(); ++iterator) {
            const std::optional<int64_t> extent =
                operand.type.shape[*iterator];
            if (!extent || *extent != 1) {
              operandAlignDim = *iterator;
              break;
            }
          }
          const C4BufferRecord *record =
              C4FindSourceBuffer(buffers, operand.buffer);
          if (operandAlignDim && record &&
              record->addressSpace == AddressSpace::UB &&
              !C4AlreadyAligned(operand.type, *operandAlignDim)) {
            const std::optional<size_t> rootDimension =
                C4OperandDimToRootDim(*record, operand.type,
                                      *operandAlignDim);
            if (rootDimension)
              marks[record->sourceIdentity].insert(*rootDimension);
          }
        }
        continue;
      }
      if ((operation.name == "hivm.hir.load" ||
           operation.name == "hivm.hir.store") &&
          C4HasBooleanProperty(
              operation, "may_implicit_transpose_with_last_axis"))
        continue;
      std::vector<MemRefTypeModel> types;
      for (const C4OperandType &operand : operands)
        types.push_back(operand.type);
      std::optional<size_t> alignDim;
      if (operation.name == "hivm.hir.vcumsum") {
        const std::vector<int64_t> dimensions =
            C4GetLimitedAxes(operation);
        const size_t next = dimensions.empty()
                                ? rank
                                : static_cast<size_t>(dimensions.back() + 1);
        const bool nextIsUnit =
            next < rank &&
            std::all_of(types.begin(), types.end(), [&](const auto &type) {
              return type.shape[next] && *type.shape[next] == 1;
            });
        alignDim = nextIsUnit
                       ? std::optional<size_t>(next)
                       : C4GetLastUnContinuousDimWithBarriers(types,
                                                              dimensions);
      } else if (const std::vector<int64_t> dimensions =
                     C4GetLimitedAxes(operation);
                 !dimensions.empty()) {
        alignDim = C4GetLastUnContinuousDimWithBarriers(types, dimensions);
      } else {
        alignDim = C4GetLastUnContinuousDim(types);
      }
      if (!alignDim)
        continue;
      for (const C4OperandType &operand : operands) {
        const C4BufferRecord *record =
            C4FindSourceBuffer(buffers, operand.buffer);
        std::optional<size_t> adjustedAlignDim = alignDim;
        if (operation.name == "hivm.hir.vreduce") {
          const std::vector<int64_t> reduceDims =
              C4GetLimitedAxes(operation);
          const std::vector<std::vector<size_t>> reassociation =
              C4ReassociationWithBarriers(types, reduceDims);
          const std::optional<size_t> flattenedAlignDim =
              C4ReassociatedDimension(reassociation, *alignDim);
          const std::optional<size_t> flattenedReduceDim =
              reduceDims.empty() || reduceDims.back() < 0
                  ? std::nullopt
                  : C4ReassociatedDimension(
                        reassociation,
                        static_cast<size_t>(reduceDims.back()));
          const bool isLastReduce =
              flattenedReduceDim &&
              *flattenedReduceDim + 1 == reassociation.size();
          if (isLastReduce && flattenedAlignDim &&
              *flattenedAlignDim + 2 == reassociation.size() &&
              operand.isDpsInit) {
            adjustedAlignDim =
                *flattenedAlignDim == 0
                    ? std::nullopt
                    : C4PreviousUncontinuousDimension(
                          types, reassociation, *flattenedAlignDim - 1);
          }
        }
        if (!record || record->addressSpace != AddressSpace::UB ||
            !adjustedAlignDim ||
            C4AlreadyAligned(operand.type, *adjustedAlignDim))
          continue;
        const std::optional<size_t> rootDimension =
            C4OperandDimToRootDim(*record, operand.type, *adjustedAlignDim);
        if (rootDimension)
          marks[record->sourceIdentity].insert(*rootDimension);
      }
  }

  bool propagated = true;
  for (size_t iteration = 0; propagated && iteration < 10; ++iteration) {
    propagated = false;
    for (const C1OperationRecord &operation :
         c3.bufferized.logicalModule.operations) {
      if (!IsHIVMStructuredOp(operation.name) ||
          operation.name == "hivm.hir.load" ||
          operation.name == "hivm.hir.store" ||
          operation.name == "hivm.hir.vtranspose")
        continue;
      const std::vector<C4OperandType> operands =
          C4OperationOperandTypes(c3, operation, buffers);
      std::set<size_t> unionDimensions;
      for (const C4OperandType &operand : operands) {
        const C4BufferRecord *record =
            C4FindSourceBuffer(buffers, operand.buffer);
        if (!record)
          continue;
        auto found = marks.find(record->sourceIdentity);
        if (found != marks.end())
          for (size_t rootDimension : found->second)
            if (const std::optional<size_t> dimension =
                    C4RootDimToOperandDim(*record, operand.type,
                                          rootDimension))
              unionDimensions.insert(*dimension);
      }
      if (unionDimensions.empty())
        continue;
      for (const C4OperandType &operand : operands) {
        const C4BufferRecord *record =
            C4FindSourceBuffer(buffers, operand.buffer);
        if (!record || record->addressSpace != AddressSpace::UB)
          continue;
        for (size_t dimension : unionDimensions) {
          if (dimension >= operand.type.shape.size() ||
              C4AlreadyAligned(operand.type, dimension))
            continue;
          const std::optional<size_t> rootDimension =
              C4OperandDimToRootDim(*record, operand.type, dimension);
          if (!rootDimension)
            continue;
          propagated |=
              marks[record->sourceIdentity].insert(*rootDimension).second;
        }
      }
    }
  }
  for (C4BufferRecord &buffer : buffers) {
    const std::optional<MemRefTypeModel> type = ParseMemRefType(buffer.type);
    auto size = sizeMarks.find(buffer.sourceIdentity);
    if (size != sizeMarks.end()) {
      if (!type)
        throw std::runtime_error("C4: malformed size-aligned buffer type");
      buffer.constBits = C4SizeAlignedBufferBits(*type, size->second);
    }
    auto found = marks.find(buffer.sourceIdentity);
    if (found == marks.end())
      continue;
    if (!type)
      throw std::runtime_error("C4: malformed aligned buffer type");
    buffer.constBits =
        std::max(buffer.constBits,
                 C4AlignedBufferBits(*type, found->second,
                                     &buffer.upperBounds));
  }
  return {std::move(sizeMarks), std::move(marks)};
}

inline std::string SerializeAlignStorageResult(const AlignStorageResult &result) {
  std::ostringstream output;
  output << "ALIGN_STORAGE\t1\n";
  for (const auto &[buffer, dimensions] : result.allocAlignments)
    for (const auto &[dimension, bytes] : dimensions)
      output << "ALLOC_ALIGN\t" << HexEncode(buffer) << '\t' << dimension
             << '\t' << bytes << '\n';
  for (const auto &[buffer, dimensions] : result.strideAlignments)
    for (size_t dimension : dimensions)
      output << "STRIDE_ALIGN\t" << HexEncode(buffer) << '\t' << dimension
             << "\t32\n";
  return output.str();
}

} // namespace cvub

#endif
