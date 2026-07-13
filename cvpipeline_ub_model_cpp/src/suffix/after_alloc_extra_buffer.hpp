#ifndef CVPIPELINE_UB_MODEL_CPP_AFTER_ALLOC_EXTRA_BUFFER_HPP
#define CVPIPELINE_UB_MODEL_CPP_AFTER_ALLOC_EXTRA_BUFFER_HPP

#include "alloc_extra_buffer.hpp"
#include "align_storage.hpp"

namespace cvub {

struct AfterAllocExtraBufferState {
  PostBufferizationRewriteState postBufferization;
  AlignStorageResult alignStorage;
  std::vector<LocalBufferRecord> buffers;
  std::map<std::string, int> bufferOwnerOperations;
};

inline uint64_t StaticBufferBits(const std::string &type) {
  const std::optional<MemRefTypeModel> parsed = ParseMemRefType(type);
  if (!parsed)
    throw std::runtime_error("AllocExtraBuffer: expected memref type");
  uint64_t elements = 1;
  for (const std::optional<int64_t> &extent : parsed->shape) {
    if (!extent)
      throw std::runtime_error(
          "AllocExtraBuffer: dynamic allocation requires buffer-size model: " + type);
    elements = CheckedMul(elements, static_cast<uint64_t>(*extent),
                          "AllocExtraBuffer allocation element count");
  }
  return CheckedMul(elements, GetElementBitWidth(*parsed),
                    "AllocExtraBuffer allocation bits");
}

inline int AllocationSourceOrder(const BufferAllocation &allocation) {
  return AllocationSourceOperation(allocation.source);
}

inline std::optional<uint64_t> TryStaticBufferBits(const std::string &type) {
  const std::optional<MemRefTypeModel> parsed = ParseMemRefType(type);
  if (!parsed)
    return std::nullopt;
  uint64_t elements = 1;
  for (const std::optional<int64_t> &extent : parsed->shape) {
    if (!extent)
      return std::nullopt;
    elements = CheckedMul(elements, static_cast<uint64_t>(*extent),
                          "AllocExtraBuffer allocation element count");
  }
  return CheckedMul(elements, GetElementBitWidth(*parsed),
                    "AllocExtraBuffer allocation bits");
}

inline std::vector<std::optional<int64_t>> AllocationUpperBounds(
    const PostBufferizationRewriteState &postBufferization, const BufferAllocation &allocation) {
  const std::optional<MemRefTypeModel> allocationType =
      ParseMemRefType(allocation.type);
  if (!allocationType)
    throw std::runtime_error("AllocExtraBuffer: expected allocation memref type");
  std::vector<std::optional<int64_t>> bounds = allocationType->shape;
  if (std::all_of(bounds.begin(), bounds.end(),
                  [](const std::optional<int64_t> &bound) {
                    return bound.has_value();
                  }))
    return bounds;

  const size_t first = allocation.source.find(':');
  const size_t second = allocation.source.find(':', first + 1);
  if (!startsWith(allocation.source, "operand:") ||
      first == std::string::npos || second == std::string::npos)
    throw std::runtime_error("AllocExtraBuffer: dynamic allocation source is unsupported: " +
                             allocation.source);
  const int operationId =
      std::stoi(allocation.source.substr(first + 1, second - first - 1));
  const size_t operand =
      static_cast<size_t>(std::stoull(allocation.source.substr(second + 1)));
  const GenericModule &module = postBufferization.bufferized.logicalModule;
  const GenericOperation &owner =
      module.operations.at(static_cast<size_t>(operationId));
  if (operand >= owner.operands.size())
    throw std::runtime_error("AllocExtraBuffer: dynamic allocation operand is missing");
  const std::map<int, const GenericOperation *> definitions =
      DefiningOperations(module);
  auto definition = definitions.find(owner.operands[operand]);
  if (definition == definitions.end() ||
      (definition->second->name != "tensor.extract_slice" &&
       definition->second->name != "memref.subview") ||
      definition->second->operandTypes.empty())
    throw std::runtime_error(
        "AllocExtraBuffer: dynamic allocation upper bound is not an extract slice");
  const std::string sourceTypeText =
      IsTensorType(definition->second->operandTypes.front())
          ? ConvertTensorToMemRefType(definition->second->operandTypes.front())
          : definition->second->operandTypes.front();
  const std::optional<MemRefTypeModel> sourceType =
      ParseMemRefType(sourceTypeText);
  if (!sourceType || sourceType->shape.size() != bounds.size())
    throw std::runtime_error("AllocExtraBuffer: dynamic slice source rank mismatch");
  for (size_t axis = 0; axis < bounds.size(); ++axis)
    if (!bounds[axis])
      bounds[axis] = sourceType->shape[axis];
  if (std::any_of(bounds.begin(), bounds.end(),
                  [](const std::optional<int64_t> &bound) {
                    return !bound.has_value();
                  }))
    throw std::runtime_error("AllocExtraBuffer: dynamic allocation upper bound is unknown");
  return bounds;
}

inline uint64_t BufferBitsFromUpperBounds(
    const std::string &type,
    const std::vector<std::optional<int64_t>> &upperBounds) {
  const std::optional<MemRefTypeModel> parsed = ParseMemRefType(type);
  if (!parsed || parsed->shape.size() != upperBounds.size())
    throw std::runtime_error("AllocExtraBuffer: allocation upper-bound rank mismatch");
  uint64_t elements = 1;
  for (const std::optional<int64_t> &bound : upperBounds) {
    if (!bound)
      throw std::runtime_error("AllocExtraBuffer: missing allocation upper bound");
    elements = CheckedMul(elements, static_cast<uint64_t>(*bound),
                          "AllocExtraBuffer upper-bound element count");
  }
  return CheckedMul(elements, GetElementBitWidth(*parsed),
                    "AllocExtraBuffer upper-bound allocation bits");
}

inline uint64_t DecomposeBufferBits(
    const PostBufferizationRewriteState &postBufferization, const DecomposeBufferAllocation &allocation,
    const std::vector<LocalBufferRecord> &baseBuffers) {
  if (const std::optional<uint64_t> bits =
          TryStaticBufferBits(allocation.type))
    return *bits;

  for (const BufferizedOperandAccess &access : postBufferization.bufferized.accesses) {
    if (access.operationId != allocation.ownerOperation ||
        access.operandNumber != 0)
      continue;
    const std::string identity = MappedBufferIdentity(
        access.bufferId, postBufferization.singlePoint.bufferMapping);
    if (const LocalBufferRecord *source =
            FindSourceBuffer(baseBuffers, identity))
      return source->constBits;
  }
  throw std::runtime_error(
      "AllocExtraBuffer: dynamic decomposed allocation has no source buffer size: " +
      allocation.ownerName);
}

inline std::string GeneratedBufferType(
    const PostBufferizationRewriteState &postBufferization, const std::string &buffer,
    const std::vector<LocalBufferRecord> &buffers) {
  if (startsWith(buffer, "choice(") && buffer.back() == ')') {
    for (const std::string &alternative :
         splitTopLevel(buffer.substr(7, buffer.size() - 8))) {
      const std::string type = GeneratedBufferType(postBufferization, alternative, buffers);
      if (!type.empty())
        return type;
    }
    return "";
  }
  std::string identity = buffer;
  if (startsWith(identity, "local:")) {
    auto mapped = postBufferization.singlePoint.bufferMapping.find(identity);
    if (mapped != postBufferization.singlePoint.bufferMapping.end())
      identity = "base:" + mapped->second.substr(6);
    else
      identity = "base:" + identity.substr(6);
  }
  if (const LocalBufferRecord *record = FindSourceBuffer(buffers, identity))
    return record->type;
  for (const BufferizedValueBinding &binding : postBufferization.bufferized.values)
    if (binding.bufferId == buffer)
      return IsTensorType(binding.type) ? ConvertTensorToMemRefType(binding.type)
                                        : binding.type;
  return "";
}

inline std::optional<ExtraBufferAllocation> ModelExtraBufferForOperation(
    GenericOperation operation,
    std::vector<std::string> physicalTypes) {
  if (operation.name == "hivm.hir.vinterleave") {
    for (size_t index = 0; index < operation.operandTypes.size(); ++index) {
      const std::optional<MemRefTypeModel> type =
          ParseMemRefType(operation.operandTypes[index]);
      if (!type)
        continue;
      const int64_t elements = StaticElementCount(*type);
      operation.operandTypes[index] =
          "memref<" + std::to_string(elements) + "x" + type->elementType +
          ">";
      physicalTypes[index] = operation.operandTypes[index];
    }
  }
  GenericModule fake;
  GenericOperation function;
  function.id = 0;
  function.name = "func.func";
  function.properties = "{sym_name = \"kernel\"}";
  function.attributes =
      "{hacc.function_kind = #hacc.function_kind<DEVICE>}";
  fake.operations.push_back(function);

  int nextValue = 1000;
  for (size_t index = 0; index < operation.operandTypes.size(); ++index) {
    if (!IsMemRefType(operation.operandTypes[index]))
      continue;
    GenericOperation allocation;
    allocation.id = static_cast<int>(fake.operations.size());
    allocation.parentId = 0;
    allocation.name = "memref.alloc";
    allocation.results = {nextValue};
    allocation.resultTypes = {physicalTypes.at(index)};
    fake.operations.push_back(allocation);
    operation.operands[index] = nextValue++;
  }
  operation.id = static_cast<int>(fake.operations.size());
  operation.parentId = 0;
  operation.results.clear();
  operation.resultTypes.clear();
  operation.dpsInputs.clear();
  operation.dpsInits.clear();
  if (HasModeledOperationSemantics(operation.name))
    ApplyOperationSemantics(operation);
  fake.operations.push_back(operation);
  const std::vector<ExtraBufferAllocation> extra =
      ModelAllocExtraBuffer(fake, "kernel");
  if (extra.empty())
    return std::nullopt;
  if (extra.size() != 1)
    throw std::runtime_error("AllocExtraBuffer: one operation produced multiple temp buffers");
  return extra.front();
}

inline std::string AllocationTypeForTrace(const LocalBufferRecord &record) {
  const std::optional<MemRefTypeModel> type = ParseMemRefType(record.type);
  if (!type)
    throw std::runtime_error("AllocExtraBuffer: malformed traced allocation type");
  const uint64_t bitWidth = GetElementBitWidth(*type);
  if (bitWidth == 0 || record.constBits % bitWidth != 0)
    throw std::runtime_error("AllocExtraBuffer: invalid traced allocation size");
  return "memref<" + std::to_string(record.constBits / bitWidth) + "x" +
         type->elementType + ">";
}

inline std::string FormatMemRefType(const MemRefTypeModel &type) {
  std::string result = "memref<";
  for (const std::optional<int64_t> &extent : type.shape)
    result += (extent ? std::to_string(*extent) : "?") + "x";
  result += type.elementType;
  if (!IsIdentityStrides(type)) {
    result += ", strided<[";
    for (size_t index = 0; index < type.strides.size(); ++index) {
      if (index != 0)
        result += ", ";
      result +=
          type.strides[index] ? std::to_string(*type.strides[index]) : "?";
    }
    result += "]>";
  }
  return result + ">";
}

inline std::string SetDictionaryValue(const std::string &dictionary,
                                        const std::string &name,
                                        const std::string &value) {
  std::vector<std::string> entries;
  bool replaced = false;
  if (dictionary.size() >= 2) {
    entries = splitTopLevel(dictionary.substr(1, dictionary.size() - 2));
    for (std::string &entry : entries) {
      const size_t equal = entry.find('=');
      if (equal != std::string::npos &&
          trim(entry.substr(0, equal)) == name) {
        entry = name + " = " + value;
        replaced = true;
      }
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

inline MemRefTypeModel AlignedOperandType(
    const LocalBufferRecord &record, MemRefTypeModel type,
    const AlignStorageResult &alignStorage) {
  auto found = alignStorage.strideAlignments.find(record.sourceIdentity);
  if (found == alignStorage.strideAlignments.end())
    return type;
  std::set<size_t> dimensions;
  for (size_t rootDimension : found->second)
    if (const std::optional<size_t> dimension =
            RootDimToOperandDim(record, type, rootDimension))
      dimensions.insert(*dimension);
  const uint64_t bitWidth = GetElementBitWidth(type);
  if (bitWidth == 0 || 256 % bitWidth != 0)
    throw std::runtime_error("AllocExtraBuffer: invalid aligned operand element width");
  std::vector<uint64_t> targets(type.shape.size(), 1);
  for (size_t dimension : dimensions)
    targets.at(dimension) = 256 / bitWidth;
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
    if (type.shape[axis]) {
      shapeAccumulation = CheckedMul(
          shapeAccumulation,
          std::lcm(static_cast<uint64_t>(*type.shape[axis]),
                   units[axis + 1]),
          "AllocExtraBuffer aligned operand shape");
    }
  }
  std::vector<std::optional<uint64_t>> physicalShape;
  for (size_t axis = 0; axis < type.shape.size(); ++axis) {
    if (!type.shape[axis]) {
      physicalShape.push_back(std::nullopt);
      continue;
    }
    const uint64_t extent = static_cast<uint64_t>(*type.shape[axis]);
    physicalShape.push_back(units[axis] == 1
                                ? extent
                                : AlignUp(extent, units[axis]));
  }
  physicalShape.push_back(units.back());
  std::optional<uint64_t> stride = 1;
  type.strides.assign(type.shape.size(), std::nullopt);
  for (size_t reverse = physicalShape.size(); reverse > 1; --reverse) {
    if (!stride || !physicalShape[reverse - 1]) {
      stride = std::nullopt;
    } else {
      stride = CheckedMul(*stride, *physicalShape[reverse - 1],
                          "AllocExtraBuffer aligned operand stride");
    }
    if (stride)
      type.strides[reverse - 2] = static_cast<int64_t>(*stride);
  }
  if (!type.strides.empty() && type.strides.back() != 1) {
    type.shape.push_back(1);
    type.strides.push_back(1);
  }
  type.hasStridedLayout = true;
  return type;
}

inline void FlattenLimitedOperationForAllocExtraBuffer(GenericOperation &operation,
                                      const std::string &dimensionName,
                                      const std::vector<int64_t> &dimensions) {
  std::vector<MemRefTypeModel> types;
  std::vector<size_t> indices;
  for (size_t index = 0; index < operation.operandTypes.size(); ++index) {
    if (std::optional<MemRefTypeModel> type =
            ParseMemRefType(operation.operandTypes[index])) {
      indices.push_back(index);
      types.push_back(*type);
    }
  }
  if (types.empty())
    return;
  size_t rank = 0;
  for (const MemRefTypeModel &type : types)
    rank = std::max(rank, type.shape.size());
  for (MemRefTypeModel &type : types)
    while (type.shape.size() < rank) {
      type.shape.push_back(1);
      type.strides.push_back(1);
      type.hasStridedLayout = true;
    }
  const std::vector<std::vector<size_t>> reassociation =
      ReassociationWithBarriers(types, dimensions);
  for (size_t index = 0; index < types.size(); ++index)
    operation.operandTypes[indices[index]] =
        FormatMemRefType(CollapseMemRefType(types[index], reassociation));
  std::vector<int64_t> adjusted;
  for (int64_t dimension : dimensions) {
    if (dimension < 0)
      throw std::runtime_error("AllocExtraBuffer: negative limited-operation dimension");
    const std::optional<size_t> mapped = ReassociatedDimension(
        reassociation, static_cast<size_t>(dimension));
    if (!mapped)
      throw std::runtime_error("AllocExtraBuffer: unmapped limited-operation dimension");
    adjusted.push_back(static_cast<int64_t>(*mapped));
  }
  std::string value = "array<i64";
  if (!adjusted.empty()) {
    value += ": ";
    for (size_t index = 0; index < adjusted.size(); ++index) {
      if (index != 0)
        value += ", ";
      value += std::to_string(adjusted[index]);
    }
  }
  value += ">";
  operation.properties =
      SetDictionaryValue(operation.properties, dimensionName, value);
  operation.attributes =
      SetDictionaryValue(operation.attributes, dimensionName, value);
}

inline void FlattenForAllocExtraBuffer(GenericOperation &operation) {
  if (operation.name == "hivm.hir.vreduce") {
    FlattenLimitedOperationForAllocExtraBuffer(operation, "reduce_dims",
                              GetLimitedAxes(operation));
    return;
  }
  if (operation.name == "hivm.hir.vbrc") {
    std::string value =
        FindDictionaryValue(operation.properties, "broadcast_dims");
    if (value.empty())
      value = FindDictionaryValue(operation.attributes, "broadcast_dims");
    FlattenLimitedOperationForAllocExtraBuffer(operation, "broadcast_dims",
                              DecomposeI64Array(value));
  }
}

inline std::vector<std::pair<int, ExtraBufferAllocation>>
ModelConnectedAllocExtraBuffer(const PostBufferizationRewriteState &postBufferization,
                               const std::vector<LocalBufferRecord> &buffers,
                               const AlignStorageResult &alignStorage) {
  std::map<int, const OperationRewriteDelta *> rewrites;
  for (const OperationRewriteDelta &rewrite : postBufferization.operationRewrites)
    rewrites[rewrite.sourceOperation] = &rewrite;
  std::vector<std::pair<int, ExtraBufferAllocation>> result;
  const std::map<int, const GenericOperation *> definitions =
      DefiningOperations(postBufferization.bufferized.logicalModule);
  for (const GenericOperation &source :
       postBufferization.bufferized.logicalModule.operations) {
    if (postBufferization.singlePoint.scalarizedOperations.count(source.id) != 0)
      continue;
    std::map<size_t, std::string> operandBuffers;
    for (const BufferizedOperandAccess &access : postBufferization.bufferized.accesses)
      if (access.operationId == source.id)
        operandBuffers[static_cast<size_t>(access.operandNumber)] =
            MappedBufferIdentity(access.bufferId,
                                   postBufferization.singlePoint.bufferMapping);

    auto buildOriginal = [&]() {
      GenericOperation operation = source;
      std::vector<std::string> physical(operation.operandTypes.size());
      for (size_t index = 0; index < operation.operandTypes.size(); ++index) {
        auto buffer = operandBuffers.find(index);
        const std::string identity =
            buffer == operandBuffers.end() ? std::string() : buffer->second;
        operation.operandTypes[index] = BufferTypeAfterMapping(
            identity, buffers, operation.operandTypes[index]);
        auto definition = index < source.operands.size()
                              ? definitions.find(source.operands[index])
                              : definitions.end();
        if (definition != definitions.end() &&
            definition->second->name == "hivm.hir.bitcast")
          operation.operandTypes[index] =
              IsTensorType(source.operandTypes[index])
                  ? ConvertTensorToMemRefType(source.operandTypes[index])
                  : source.operandTypes[index];
        physical[index] = operation.operandTypes[index];
        if (const LocalBufferRecord *record =
                FindSourceBuffer(buffers, identity)) {
          if (operation.name == "hivm.hir.vreduce" ||
              operation.name == "hivm.hir.vbrc")
            if (std::optional<MemRefTypeModel> type =
                    ParseMemRefType(operation.operandTypes[index]))
              operation.operandTypes[index] = FormatMemRefType(
                  AlignedOperandType(*record, *type, alignStorage));
          physical[index] = record->type;
          if (operation.name == "hivm.hir.vreduce" ||
              operation.name == "hivm.hir.vbrc") {
            physical[index] = AllocationTypeForTrace(*record);
            if (definition != definitions.end() &&
                definition->second->name == "hivm.hir.bitcast") {
              const std::string logical =
                  IsTensorType(source.operandTypes[index])
                      ? ConvertTensorToMemRefType(source.operandTypes[index])
                      : source.operandTypes[index];
              const std::optional<MemRefTypeModel> type =
                  ParseMemRefType(logical);
              if (!type)
                throw std::runtime_error("AllocExtraBuffer: malformed bitcast trace type");
              physical[index] =
                  "memref<" + std::to_string(StaticElementCount(*type)) +
                  "x" + type->elementType + ">";
            }
          }
        }
      }
      FlattenForAllocExtraBuffer(operation);
      operation.operands.resize(operation.operandTypes.size());
      return std::pair<GenericOperation, std::vector<std::string>>{
          std::move(operation), std::move(physical)};
    };

    auto rewrite = rewrites.find(source.id);
    if (rewrite == rewrites.end()) {
      auto [operation, physical] = buildOriginal();
      if (std::optional<ExtraBufferAllocation> extra =
              ModelExtraBufferForOperation(std::move(operation), physical))
        result.push_back({source.id, std::move(*extra)});
      continue;
    }
    for (const GeneratedOperationRewrite &generated : rewrite->second->replacement) {
      GenericOperation operation;
      operation.name = generated.name;
      operation.properties = source.properties;
      operation.attributes = source.attributes;
      std::vector<std::string> physical;
      for (const std::string &buffer : generated.buffers) {
        const std::string type = GeneratedBufferType(postBufferization, buffer, buffers);
        if (type.empty())
          throw std::runtime_error("AllocExtraBuffer: generated operation buffer is missing: " +
                                   buffer);
        operation.operandTypes.push_back(type);
        physical.push_back(type);
      }
      operation.operands.resize(operation.operandTypes.size());
      if (std::optional<ExtraBufferAllocation> extra =
              ModelExtraBufferForOperation(std::move(operation), physical))
        result.push_back({source.id, std::move(*extra)});
    }
  }
  return result;
}

inline AfterAllocExtraBufferState BuildAfterAllocExtraBufferState(PostBufferizationRewriteState postBufferization) {
  AfterAllocExtraBufferState result;
  const std::map<std::string, AddressSpace> scopes = InferHIVMMemScope(postBufferization);
  struct Pending {
    int operation = -1;
    int phase = 0;
    size_t ordinal = 0;
    LocalBufferRecord buffer;
  };
  std::vector<Pending> pending;
  for (size_t index = 0; index < postBufferization.singlePoint.allocations.size(); ++index) {
    const BufferAllocation &allocation = postBufferization.singlePoint.allocations[index];
    const std::string identity = "base:" + std::to_string(index);
    const std::vector<std::optional<int64_t>> upperBounds =
        AllocationUpperBounds(postBufferization, allocation);
    pending.push_back({AllocationSourceOrder(allocation), 0, index,
                       {identity, identity, allocation.source, allocation.type,
                        scopes.at(identity),
                        BufferBitsFromUpperBounds(allocation.type,
                                                    upperBounds),
                        false, upperBounds}});
  }
  std::vector<LocalBufferRecord> baseBuffers;
  for (const Pending &item : pending)
    baseBuffers.push_back(item.buffer);
  std::map<int, size_t> decomposeOrdinals;
  for (const DecomposeBufferAllocation &allocation : postBufferization.decomposeAllocations) {
    const size_t ordinal = decomposeOrdinals[allocation.ownerOperation]++;
    const std::string identity =
        "decompose:" + std::to_string(allocation.ownerOperation) + ":" +
        std::to_string(ordinal);
    pending.push_back({allocation.ownerOperation, 1, ordinal,
                       {identity, identity, allocation.ownerName, allocation.type,
                        scopes.at(identity),
                        DecomposeBufferBits(postBufferization, allocation, baseBuffers),
                        false, {}}});
  }
  std::vector<LocalBufferRecord> sourceBuffers;
  for (const Pending &item : pending)
    sourceBuffers.push_back(item.buffer);
  result.alignStorage = ModelAlignStorage(postBufferization, sourceBuffers);
  for (size_t index = 0; index < pending.size(); ++index)
    pending[index].buffer = sourceBuffers[index];
  size_t extraOrdinal = 0;
  for (auto &[operation, allocation] :
       ModelConnectedAllocExtraBuffer(postBufferization, sourceBuffers,
                                      result.alignStorage)) {
    const GenericOperation &owner = postBufferization.bufferized.logicalModule.operations.at(
        static_cast<size_t>(operation));
    const GenericOperation *function =
        EnclosingFunction(postBufferization.bufferized.logicalModule, owner);
    if (!function)
      throw std::runtime_error("AllocExtraBuffer: extra buffer outside function");
    const std::string identity = "extra-source:" + std::to_string(extraOrdinal++);
    const AddressSpace space = FunctionDefaultAddressSpace(*function);
    pending.push_back({operation, 2, extraOrdinal,
                       {identity, identity, allocation.ownerName,
                        allocation.type, space,
                        StaticBufferBits(allocation.type), true, {}}});
  }
  std::sort(pending.begin(), pending.end(), [](const Pending &lhs,
                                                const Pending &rhs) {
    return std::tie(lhs.operation, lhs.phase, lhs.ordinal) <
           std::tie(rhs.operation, rhs.phase, rhs.ordinal);
  });
  size_t baseOrdinal = 0;
  std::map<std::string, size_t> ownerOrdinals;
  for (Pending &item : pending) {
    if (item.buffer.extraBuffer) {
      const size_t ordinal = ownerOrdinals[item.buffer.ownerName]++;
      item.buffer.identity = "extra:" + item.buffer.ownerName + ":" +
                             std::to_string(ordinal);
    } else {
      item.buffer.identity = "base:" + std::to_string(baseOrdinal++);
    }
    result.bufferOwnerOperations[item.buffer.sourceIdentity] = item.operation;
    result.buffers.push_back(std::move(item.buffer));
  }
  result.postBufferization = std::move(postBufferization);
  return result;
}

inline std::string
SerializeLocalBufferRecords(const std::vector<LocalBufferRecord> &buffers) {
  std::ostringstream output;
  output << "ALLOC_EXTRA_BUFFERS\t1\n";
  for (size_t index = 0; index < buffers.size(); ++index) {
    const LocalBufferRecord &buffer = buffers[index];
    output << "BUFFER\t" << index << '\t' << HexEncode(buffer.identity) << '\t'
           << HexEncode(buffer.sourceIdentity) << '\t'
           << HexEncode(buffer.ownerName) << '\t'
           << AddressSpaceName(buffer.addressSpace) << '\t'
           << buffer.constBits << '\t' << HexEncode(buffer.type) << '\t'
           << (buffer.extraBuffer ? 1 : 0) << '\n';
  }
  return output.str();
}

inline std::string SerializeAfterAllocExtraBufferState(const AfterAllocExtraBufferState &module) {
  return "AFTER_ALLOC_EXTRA_BUFFER_STATE\t1\n" + SerializePostBufferizationRewriteState(module.postBufferization) +
         SerializeAlignStorageResult(module.alignStorage) +
         SerializeLocalBufferRecords(module.buffers);
}

} // namespace cvub

#endif
