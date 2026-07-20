#ifndef CVPIPELINE_UB_MODEL_CPP_ALIGN_STORAGE_HPP
#define CVPIPELINE_UB_MODEL_CPP_ALIGN_STORAGE_HPP

#include "../pipeline/local_buffer_record.hpp"

namespace cvub {

struct AlignedOperand {
  std::string buffer;
  MemRefTypeModel type;
  size_t operandNumber = 0;
  bool isDpsInit = false;
};

struct AlignStorageResult {
  std::map<std::string, std::map<size_t, uint64_t>> allocAlignments;
  std::map<std::string, std::set<size_t>> strideAlignments;
};

inline std::optional<size_t> RootDimToOperandDim(
    const LocalBufferRecord &record, const MemRefTypeModel &operandType,
    size_t rootDimension);

inline bool IsLastDimContiguous(const MemRefTypeModel &type) {
  return type.shape.empty() ||
         (type.strides.back() && *type.strides.back() == 1) ||
         (type.shape.back() && *type.shape.back() == 1);
}

inline std::vector<std::vector<size_t>> ContinuousReassociation(
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

inline std::vector<std::vector<size_t>> UniformReassociationPipeline(
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
      ContinuousReassociation(unitCollapsed);
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

inline std::optional<size_t> LastNotUnitAxis(
    const std::vector<MemRefTypeModel> &types,
    const std::vector<size_t> &group) {
  for (auto iterator = group.rbegin(); iterator != group.rend(); ++iterator)
    if (std::any_of(types.begin(), types.end(), [&](const MemRefTypeModel &type) {
          return !type.shape[*iterator] || *type.shape[*iterator] != 1;
        }))
      return *iterator;
  return std::nullopt;
}

inline std::optional<size_t> GetLastUnContinuousDim(
    const std::vector<MemRefTypeModel> &types) {
  const std::vector<std::vector<size_t>> reassociation =
      ContinuousReassociation(types);
  if (reassociation.empty())
    return std::nullopt;
  bool lastUnitStride = true;
  for (const MemRefTypeModel &type : types) {
    const MemRefTypeModel flattened = CollapseMemRefType(type, reassociation);
    lastUnitStride = lastUnitStride && IsLastDimContiguous(flattened);
  }
  if (!lastUnitStride)
    return LastNotUnitAxis(types, reassociation.back());
  if (reassociation.size() == 1)
    return std::nullopt;
  return LastNotUnitAxis(types, reassociation[reassociation.size() - 2]);
}

inline std::optional<size_t> GetLastUnContinuousDimWithBarriers(
    const std::vector<MemRefTypeModel> &types,
    const std::vector<int64_t> &barrierDims) {
  std::vector<std::vector<size_t>> reassociation =
      ContinuousReassociation(types);
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
        lastUnitStride && IsLastDimContiguous(
                              CollapseMemRefType(type, reassociation));
  if (!lastUnitStride)
    return LastNotUnitAxis(types, reassociation.back());
  if (reassociation.size() == 1)
    return std::nullopt;
  return LastNotUnitAxis(types, reassociation[reassociation.size() - 2]);
}

inline std::vector<std::vector<size_t>> ReassociationWithBarriers(
    const std::vector<MemRefTypeModel> &types,
    const std::vector<int64_t> &barrierDims) {
  std::vector<std::vector<size_t>> reassociation =
      ContinuousReassociation(types);
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
AllocExtraBufferReassociationWithBarriers(
    const std::vector<MemRefTypeModel> &types,
    const std::vector<int64_t> &barrierDims) {
  if (types.empty())
    return {};
  const size_t rank = types.front().shape.size();
  const std::vector<bool> contiguous = GetContiguousAxes(types);
  std::set<size_t> barriers;
  for (int64_t dimension : barrierDims)
    if (dimension >= 0 && static_cast<size_t>(dimension) < rank)
      barriers.insert(static_cast<size_t>(dimension));
  if (barriers.empty())
    return ContinuousReassociation(types);

  std::vector<bool> uniformUnit(rank, true);
  for (const MemRefTypeModel &type : types) {
    if (type.shape.size() != rank)
      throw std::runtime_error("flatten operand rank mismatch");
    for (size_t axis = 0; axis < rank; ++axis)
      uniformUnit[axis] = uniformUnit[axis] && type.shape[axis] &&
                          *type.shape[axis] == 1;
  }

  std::vector<std::vector<size_t>> reassociation;
  for (size_t axis = 0; axis < rank; ++axis) {
    bool barrierBoundary = false;
    if (axis > 0 &&
        (barriers.count(axis - 1) != barriers.count(axis))) {
      const bool nonBarrierIsRight = barriers.count(axis - 1) != 0;
      const size_t nonBarrierAxis = nonBarrierIsRight ? axis : axis - 1;
      bool adjacentComponentIsUnit = uniformUnit[nonBarrierAxis];
      if (nonBarrierIsRight) {
        for (size_t next = nonBarrierAxis + 1;
             adjacentComponentIsUnit && next < rank && contiguous[next] &&
             barriers.count(next) == 0;
             ++next)
          adjacentComponentIsUnit = uniformUnit[next];
      } else {
        size_t current = nonBarrierAxis;
        while (adjacentComponentIsUnit && current > 0 &&
               contiguous[current] && barriers.count(current - 1) == 0) {
          --current;
          adjacentComponentIsUnit = uniformUnit[current];
        }
      }
      // FlattenInterface may absorb an adjacent dimension that is one for
      // every operand into the limited axis. The entire contiguous component
      // on that side must be unit; otherwise removing this boundary would
      // transitively merge a non-unit dimension across the limited axis.
      barrierBoundary = !adjacentComponentIsUnit;
    }
    if (axis == 0 || !contiguous[axis] || barrierBoundary)
      reassociation.push_back({});
    reassociation.back().push_back(axis);
  }
  return reassociation;
}

inline std::vector<std::vector<size_t>>
UniformReassociationPipelineWithBarriers(
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
      ReassociationWithBarriers(unitCollapsed, adjustedBarriers);
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

inline std::optional<size_t> ReassociatedDimension(
    const std::vector<std::vector<size_t>> &reassociation,
    size_t dimension) {
  for (size_t group = 0; group < reassociation.size(); ++group)
    if (std::find(reassociation[group].begin(), reassociation[group].end(),
                  dimension) != reassociation[group].end())
      return group;
  return std::nullopt;
}

inline std::optional<size_t> PreviousUncontinuousDimension(
    const std::vector<MemRefTypeModel> &types,
    const std::vector<std::vector<size_t>> &reassociation,
    size_t flattenedDimension) {
  if (flattenedDimension == 0)
    return std::nullopt;
  for (size_t reverse = flattenedDimension; reverse > 0; --reverse)
    if (const std::optional<size_t> dimension =
            LastNotUnitAxis(types, reassociation[reverse - 1]))
      return dimension;
  return std::nullopt;
}

inline bool AlreadyAligned(const MemRefTypeModel &type, size_t alignDim) {
  const uint64_t bitWidth = GetElementBitWidth(type);
  if (alignDim >= type.shape.size() || bitWidth == 0)
    return true;
  if (type.hasStridedLayout && type.strides[alignDim] &&
      CheckedMul(static_cast<uint64_t>(*type.strides[alignDim]), bitWidth,
                 "AlignStorage explicit stride bits") %
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
                               "AlignStorage stride inner elements");
  }
  return CheckedMul(innerElements, bitWidth, "AlignStorage stride inner bits") % 256 ==
         0;
}

inline uint64_t AlignedBufferBits(const MemRefTypeModel &type,
                                    const std::set<size_t> &alignDims,
                                    const std::vector<std::optional<int64_t>>
                                        *upperBounds = nullptr) {
  if (alignDims.empty()) {
    std::string plain = "memref<";
    for (const std::optional<int64_t> &extent : type.shape) {
      if (!extent)
        throw std::runtime_error("AllocExtraBuffer: dynamic aligned buffer");
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
        throw std::runtime_error("AllocExtraBuffer: missing dynamic allocation upper bound");
      elements = CheckedMul(elements, static_cast<uint64_t>(*extent),
                            "AllocExtraBuffer allocation element count");
    }
    return CheckedMul(elements, GetElementBitWidth(type),
                      "AllocExtraBuffer allocation bits");
  }
  const uint64_t bitWidth = GetElementBitWidth(type);
  if (bitWidth == 0 || 256 % bitWidth != 0)
    throw std::runtime_error("AllocExtraBuffer: invalid stride alignment element width");
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
          "AllocExtraBuffer aligned shape accumulation");
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
        throw std::runtime_error("AllocExtraBuffer: missing dynamic allocation upper bound");
      const uint64_t bound =
          static_cast<uint64_t>(*(*upperBounds)[axis]);
      aligned = unit == 1
                    ? bound
                    : CheckedAdd(bound, unit - 1,
                                 "AlignStorage dynamic aligned closed upper bound");
    }
    elements = CheckedMul(elements, aligned, "AllocExtraBuffer aligned allocation shape");
  }
  elements = CheckedMul(elements, units.back(), "AllocExtraBuffer aligned trailing shape");
  return CheckedMul(elements, bitWidth, "AllocExtraBuffer aligned allocation bits");
}

inline uint64_t SizeAlignedBufferBits(
    const MemRefTypeModel &type,
    const std::map<size_t, uint64_t> &alignBytesByDim) {
  const uint64_t bitWidth = GetElementBitWidth(type);
  if (bitWidth == 0 || bitWidth % 8 != 0)
    throw std::runtime_error("AllocExtraBuffer: invalid size alignment element width");
  const uint64_t elementBytes = bitWidth / 8;
  uint64_t elements = 1;
  for (size_t axis = 0; axis < type.shape.size(); ++axis) {
    if (!type.shape[axis])
      throw std::runtime_error("AllocExtraBuffer: dynamic size-aligned allocation");
    uint64_t extent = static_cast<uint64_t>(*type.shape[axis]);
    auto alignment = alignBytesByDim.find(axis);
    if (alignment != alignBytesByDim.end()) {
      if (alignment->second == 0 || alignment->second % elementBytes != 0)
        throw std::runtime_error("AllocExtraBuffer: invalid allocation alignment bytes");
      extent = AlignUp(extent, alignment->second / elementBytes);
    }
    elements = CheckedMul(elements, extent, "AlignStorage size-aligned shape");
  }
  return CheckedMul(elements, bitWidth, "AlignStorage size-aligned bits");
}

inline bool HasBooleanProperty(const GenericOperation &operation,
                                 const std::string &name) {
  return FindDictionaryValue(operation.properties, name) == "true" ||
         FindDictionaryValue(operation.attributes, name) == "true";
}

inline std::vector<int64_t>
GetLimitedAxes(const GenericOperation &operation) {
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

inline void MarkSizeAlignment(
    const AlignedOperand &operand, size_t axis, uint64_t alignmentBytes,
    const std::vector<LocalBufferRecord> &buffers,
    std::map<std::string, std::map<size_t, uint64_t>> &marks) {
  if (axis >= operand.type.shape.size() || !operand.type.shape[axis]) {
    const LocalBufferRecord *record =
        FindSourceBuffer(buffers, operand.buffer);
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
  const LocalBufferRecord *record = FindSourceBuffer(buffers, operand.buffer);
  if (record && record->addressSpace == AddressSpace::UB)
    marks[record->sourceIdentity][axis] =
        std::max(marks[record->sourceIdentity][axis], alignmentBytes);
}

inline std::vector<size_t>
TransposeLoopDims(const GenericOperation &operation, size_t rank) {
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

inline void MarkVCastAllocSize(
    const GenericOperation &operation,
    const std::vector<AlignedOperand> &operands,
    const std::vector<LocalBufferRecord> &buffers,
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
    MarkSizeAlignment(operands.front(), 0, sourceAlignment, buffers, marks);
    MarkSizeAlignment(operands.back(), 0, 32 * 32, buffers, marks);
    return;
  }
  MarkSizeAlignment(operands.front(), rank - 1, 32, buffers, marks);
  MarkSizeAlignment(operands.front(), rank - 2, 32 * bytesFactor, buffers,
                      marks);
  MarkSizeAlignment(operands.back(), rank - 1, 32, buffers, marks);
  MarkSizeAlignment(operands.back(), rank - 2, 32, buffers, marks);
  (void)sourceBytes;
  (void)operation;
}

inline void MarkVTransposeAllocSize(
    const GenericOperation &operation,
    const std::vector<AlignedOperand> &operands,
    const std::vector<LocalBufferRecord> &buffers,
    std::map<std::string, std::map<size_t, uint64_t>> &marks) {
  if (operands.size() < 2 || operands.front().type.shape.empty() ||
      HasBooleanProperty(operation, "disable_align"))
    return;
  const size_t rank = operands.front().type.shape.size();
  const std::vector<size_t> dimensions = TransposeLoopDims(operation, rank);
  if (dimensions.empty() ||
      std::find(dimensions.begin(), dimensions.end(), rank - 1) ==
          dimensions.end())
    return;
  for (const AlignedOperand &operand : operands)
    for (size_t axis : dimensions)
      MarkSizeAlignment(operand, axis, 32, buffers, marks);
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
  MarkSizeAlignment(operands.front(), dimensions[1], 64, buffers, marks);
  MarkSizeAlignment(operands.back(), dimensions[0], 64, buffers, marks);
}

inline void MarkVSortAllocSize(
    const std::vector<AlignedOperand> &operands,
    const std::vector<LocalBufferRecord> &buffers,
    std::map<std::string, std::map<size_t, uint64_t>> &marks) {
  for (const AlignedOperand &operand : operands) {
    if (operand.type.shape.empty())
      continue;
    const uint64_t elementBytes = GetElementBitWidth(operand.type) / 8;
    if (elementBytes == 0)
      continue;
    const uint64_t elementsPerBlock = 32 / elementBytes;
    MarkSizeAlignment(operand, operand.type.shape.size() - 1,
                        32 * (32 / elementsPerBlock), buffers, marks);
  }
}

inline void MarkBufferizationConflictCopies(
    const PostBufferizationRewriteState &postBufferization, const std::vector<LocalBufferRecord> &buffers,
    std::map<std::string, std::set<size_t>> &marks) {
  const GenericModule &module = postBufferization.bufferized.logicalModule;
  for (size_t ordinal = 0; ordinal < postBufferization.singlePoint.allocations.size();
       ++ordinal) {
    const BufferAllocation &allocation = postBufferization.singlePoint.allocations[ordinal];
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
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    if (operand >= operation.operands.size())
      continue;
    const BufferizedValueBinding *original = nullptr;
    for (const BufferizedValueBinding &binding : postBufferization.bufferized.values)
      if (binding.valueId == operation.operands[operand]) {
        original = &binding;
        break;
      }
    if (!original)
      continue;
    const std::string sourceIdentity = MappedBufferIdentity(
        original->bufferId, postBufferization.singlePoint.bufferMapping);
    const std::string destinationIdentity =
        "base:" + std::to_string(ordinal);
    const LocalBufferRecord *source =
        FindSourceBuffer(buffers, sourceIdentity);
    const LocalBufferRecord *destination =
        FindSourceBuffer(buffers, destinationIdentity);
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
    const std::optional<size_t> alignDim = GetLastUnContinuousDim(types);
    if (!alignDim)
      continue;
    if (source->addressSpace == AddressSpace::UB &&
        !AlreadyAligned(*sourceType, *alignDim))
      marks[source->sourceIdentity].insert(*alignDim);
    if (destination->addressSpace == AddressSpace::UB &&
        !AlreadyAligned(*destinationType, *alignDim))
      marks[destination->sourceIdentity].insert(*alignDim);
  }
}

inline std::vector<std::vector<size_t>> StaticShapePartition(
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

inline std::optional<size_t> RootDimToOperandDim(
    const LocalBufferRecord &record, const MemRefTypeModel &operandType,
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
        StaticShapePartition(rootType->shape, operandType.shape);
    for (size_t axis = 0; axis < groups.size(); ++axis)
      if (std::find(groups[axis].begin(), groups[axis].end(), rootDimension) !=
          groups[axis].end())
        return axis;
  }
  if (rootType->shape.size() < operandType.shape.size()) {
    const std::vector<std::vector<size_t>> groups =
        StaticShapePartition(operandType.shape, rootType->shape);
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

inline std::optional<size_t> OperandDimToRootDim(
    const LocalBufferRecord &record, const MemRefTypeModel &operandType,
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
        StaticShapePartition(rootType->shape, operandType.shape);
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
        StaticShapePartition(operandType.shape, rootType->shape);
    for (size_t rootAxis = 0; rootAxis < groups.size(); ++rootAxis)
      if (std::find(groups[rootAxis].begin(), groups[rootAxis].end(),
                    operandDimension) != groups[rootAxis].end())
        return rootAxis;
  }
  return std::nullopt;
}

inline std::vector<AlignedOperand> OperationOperandTypes(
    const PostBufferizationRewriteState &postBufferization, const GenericOperation &operation,
    const std::vector<LocalBufferRecord> &buffers,
    const std::map<std::string, std::set<size_t>> *strideMarks = nullptr) {
  std::map<size_t, std::string> accesses;
  for (const BufferizedOperandAccess &access : postBufferization.bufferized.accesses)
    if (access.operationId == operation.id)
      accesses[static_cast<size_t>(access.operandNumber)] =
          MappedBufferIdentity(access.bufferId, postBufferization.singlePoint.bufferMapping);
  std::vector<AlignedOperand> result;
  const std::vector<size_t> initIndices = DpsInitOperandIndices(
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
      const std::map<int, const GenericOperation *> definitions =
          DefiningOperations(postBufferization.bufferized.logicalModule);
      auto definition = definitions.find(operation.operands[index]);
      if (definition != definitions.end() &&
          definition->second->name == "tensor.extract_slice") {
        const BufferizedValueBinding *binding = nullptr;
        for (const BufferizedValueBinding &candidate : postBufferization.bufferized.values)
          if (candidate.valueId == operation.operands[index]) {
            binding = &candidate;
            break;
          }
        if (binding) {
          const std::string rootIdentity = MappedBufferIdentity(
              binding->bufferId, postBufferization.singlePoint.bufferMapping);
          const LocalBufferRecord *root =
              FindSourceBuffer(buffers, rootIdentity);
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
        const LocalBufferRecord *record = FindSourceBuffer(buffers, buffer);
        for (size_t rootAxis : marked->second) {
          const std::optional<size_t> axis =
              record ? RootDimToOperandDim(*record, *type, rootAxis)
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
ModelAlignStorage(const PostBufferizationRewriteState &postBufferization,
                  std::vector<LocalBufferRecord> &buffers,
                  bool alignAllocSize = true,
                  bool enableStrideAlign = true) {
  std::map<std::string, std::map<size_t, uint64_t>> sizeMarks;
  if (alignAllocSize) {
    for (const GenericOperation &operation :
         postBufferization.bufferized.logicalModule.operations) {
      if (!IsHIVMStructuredOp(operation.name))
        continue;
      const std::vector<AlignedOperand> operands =
          OperationOperandTypes(postBufferization, operation, buffers);
      if (operation.name == "hivm.hir.vcast") {
        const GenericOperation *function = EnclosingFunction(
            postBufferization.bufferized.logicalModule, operation);
        if (function && !HasDictionaryEntry(
                            function->attributes,
                            "hivm.disable_size_align_for_cast"))
          MarkVCastAllocSize(operation, operands, buffers, sizeMarks);
      } else if (operation.name == "hivm.hir.vtranspose") {
        MarkVTransposeAllocSize(operation, operands, buffers, sizeMarks);
      } else if (operation.name == "hivm.hir.vsort") {
        MarkVSortAllocSize(operands, buffers, sizeMarks);
      }
    }
  }

  std::map<std::string, std::set<size_t>> marks;
  if (enableStrideAlign)
    MarkBufferizationConflictCopies(postBufferization, buffers, marks);
  for (const GenericOperation &operation :
       postBufferization.bufferized.logicalModule.operations) {
      if (!enableStrideAlign)
        break;
      if (!IsHIVMStructuredOp(operation.name))
        continue;
      const std::vector<AlignedOperand> operands =
          OperationOperandTypes(postBufferization, operation, buffers, &marks);
      if (operands.empty())
        continue;
      const size_t rank = operands.front().type.shape.size();
      if (rank == 0 || std::any_of(operands.begin(), operands.end(),
                                   [&](const AlignedOperand &operand) {
                                     return operand.type.shape.size() != rank;
                                   }))
        continue;
      if (operation.name == "hivm.hir.vtranspose") {
        const std::vector<size_t> dimensions =
            TransposeLoopDims(operation, rank);
        if (std::find(dimensions.begin(), dimensions.end(), rank - 1) !=
            dimensions.end())
          continue;
        for (const AlignedOperand &operand : operands) {
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
          const LocalBufferRecord *record =
              FindSourceBuffer(buffers, operand.buffer);
          if (operandAlignDim && record &&
              record->addressSpace == AddressSpace::UB &&
              !AlreadyAligned(operand.type, *operandAlignDim)) {
            const std::optional<size_t> rootDimension =
                OperandDimToRootDim(*record, operand.type,
                                      *operandAlignDim);
            if (rootDimension)
              marks[record->sourceIdentity].insert(*rootDimension);
          }
        }
        continue;
      }
      if ((operation.name == "hivm.hir.load" ||
           operation.name == "hivm.hir.store") &&
          HasBooleanProperty(
              operation, "may_implicit_transpose_with_last_axis"))
        continue;
      std::vector<MemRefTypeModel> types;
      for (const AlignedOperand &operand : operands)
        types.push_back(operand.type);
      const auto alignmentLimitedAxes = [&]() {
        // Without AlignAllocSize, the real alignment pipeline does not carry
        // VConcat's concatenation dimension into the stride-mark barrier
        // analysis. Other limited operations keep their normal barriers.
        if (!alignAllocSize && operation.name == "hivm.hir.vconcat")
          return std::vector<int64_t>{};
        return GetLimitedAxes(operation);
      };
      std::optional<size_t> alignDim;
      if (operation.name == "hivm.hir.vcumsum") {
        const std::vector<int64_t> dimensions =
            alignmentLimitedAxes();
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
                       : GetLastUnContinuousDimWithBarriers(types,
                                                              dimensions);
      } else if (const std::vector<int64_t> dimensions =
                     alignmentLimitedAxes();
                 !dimensions.empty()) {
        alignDim = GetLastUnContinuousDimWithBarriers(types, dimensions);
      } else {
        alignDim = GetLastUnContinuousDim(types);
      }
      if (!alignDim)
        continue;
      for (const AlignedOperand &operand : operands) {
        const LocalBufferRecord *record =
            FindSourceBuffer(buffers, operand.buffer);
        std::optional<size_t> adjustedAlignDim = alignDim;
        if (operation.name == "hivm.hir.vreduce") {
          const std::vector<int64_t> reduceDims =
              GetLimitedAxes(operation);
          const std::vector<std::vector<size_t>> reassociation =
              ReassociationWithBarriers(types, reduceDims);
          const std::optional<size_t> flattenedAlignDim =
              ReassociatedDimension(reassociation, *alignDim);
          const std::optional<size_t> flattenedReduceDim =
              reduceDims.empty() || reduceDims.back() < 0
                  ? std::nullopt
                  : ReassociatedDimension(
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
                    : PreviousUncontinuousDimension(
                          types, reassociation, *flattenedAlignDim - 1);
          }
        }
        if (!record || record->addressSpace != AddressSpace::UB ||
            !adjustedAlignDim ||
            AlreadyAligned(operand.type, *adjustedAlignDim))
          continue;
        const std::optional<size_t> rootDimension =
            OperandDimToRootDim(*record, operand.type, *adjustedAlignDim);
        if (rootDimension)
          marks[record->sourceIdentity].insert(*rootDimension);
      }
  }

  bool propagated = enableStrideAlign;
  for (size_t iteration = 0; propagated && iteration < 10; ++iteration) {
    propagated = false;
    for (const GenericOperation &operation :
         postBufferization.bufferized.logicalModule.operations) {
      if (!IsHIVMStructuredOp(operation.name) ||
          operation.name == "hivm.hir.load" ||
          operation.name == "hivm.hir.store")
        continue;
      const std::vector<AlignedOperand> operands =
          OperationOperandTypes(postBufferization, operation, buffers);
      std::set<size_t> unionDimensions;
      for (const AlignedOperand &operand : operands) {
        const LocalBufferRecord *record =
            FindSourceBuffer(buffers, operand.buffer);
        if (!record)
          continue;
        auto found = marks.find(record->sourceIdentity);
        if (found != marks.end())
          for (size_t rootDimension : found->second)
            if (const std::optional<size_t> dimension =
                    RootDimToOperandDim(*record, operand.type,
                                          rootDimension))
              unionDimensions.insert(*dimension);
      }
      if (unionDimensions.empty())
        continue;
      for (const AlignedOperand &operand : operands) {
        const LocalBufferRecord *record =
            FindSourceBuffer(buffers, operand.buffer);
        if (!record || record->addressSpace != AddressSpace::UB)
          continue;
        for (size_t dimension : unionDimensions) {
          if (dimension >= operand.type.shape.size() ||
              AlreadyAligned(operand.type, dimension))
            continue;
          const std::optional<size_t> rootDimension =
              OperandDimToRootDim(*record, operand.type, dimension);
          if (!rootDimension)
            continue;
          propagated |=
              marks[record->sourceIdentity].insert(*rootDimension).second;
        }
      }
    }
  }
  for (LocalBufferRecord &buffer : buffers) {
    const std::optional<MemRefTypeModel> type = ParseMemRefType(buffer.type);
    auto size = sizeMarks.find(buffer.sourceIdentity);
    if (size != sizeMarks.end()) {
      if (!type)
        throw std::runtime_error("AllocExtraBuffer: malformed size-aligned buffer type");
      buffer.constBits = SizeAlignedBufferBits(*type, size->second);
    }
    auto found = marks.find(buffer.sourceIdentity);
    if (found == marks.end())
      continue;
    if (!type)
      throw std::runtime_error("AllocExtraBuffer: malformed aligned buffer type");
    buffer.constBits =
        std::max(buffer.constBits,
                 AlignedBufferBits(*type, found->second,
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
