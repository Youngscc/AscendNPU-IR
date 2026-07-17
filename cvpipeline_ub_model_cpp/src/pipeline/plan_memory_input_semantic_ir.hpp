#ifndef CVPIPELINE_UB_MODEL_CPP_PLANMEMORY_INPUT_SEMANTIC_IR_HPP
#define CVPIPELINE_UB_MODEL_CPP_PLANMEMORY_INPUT_SEMANTIC_IR_HPP

#include "after_mark_multi_buffer.hpp"

namespace cvub {

struct PlanMemoryInputBufferRecord {
  size_t globalOrdinal = 0;
  std::string identity;
  std::string sourceIdentity;
  std::string functionName;
  std::string physicalType;
  uint64_t constBits = 0;
  bool extraBuffer = false;
  uint32_t multiBufferNum = 1;
  bool preloadLocalBuffer = false;
};

struct PlanMemoryInputAccessRecord {
  std::string functionName;
  size_t ordinal = 0;
  int sourceOperation = -1;
  std::string sourceOperationName;
  std::string operationName;
  bool generatedOperation = false;
  size_t generatedOrdinal = 0;
  bool preserveSourceProperties = false;
  bool outOfPlaceCopy = false;
  int sourceOperandNumber = -1;
  bool decomposedScalarVSub = false;
  std::vector<std::pair<std::string, std::string>> accesses;
  std::set<std::string> temporaryBuffers;
};

struct PlanMemoryInputSemanticIR {
  AfterMarkMultiBufferState afterMarkMultiBuffer;
  std::vector<PlanMemoryInputBufferRecord> buffers;
  std::vector<PlanMemoryInputAccessRecord> accesses;
  std::vector<std::string> accessBlockers;
};

inline bool IsControlAliasOperation(const std::string &name) {
  return name == "scf.for" || name == "scf.while" || name == "scf.if" ||
         name == "scf.yield" || name == "scf.condition" ||
         name == "scope.scope" || name == "scope.return" || name == "cf.br" ||
         name == "cf.cond_br";
}

inline std::string AccessEffect(const std::string &name, size_t operand,
                                  size_t operandCount,
                                  const std::string &properties = "") {
  if (IsControlAliasOperation(name))
    return "alias";
  GenericOperation operation;
  operation.name = name;
  operation.operands.resize(operandCount);
  operation.properties = properties;
  if (operation.properties.empty() && IsDestinationStyleOp(name) &&
      name != "hivm.hir.varange" && name != "hivm.hir.vconcat" &&
      operandCount != 0)
    operation.properties =
        "operandSegmentSizes = array<i32: " +
        std::to_string(operandCount - 1) + ", 1>";
  const InlineLoadCopyMemoryEffect effect = OperandMemoryEffect(operation, operand);
  const bool read = HasMemoryEffect(effect, InlineLoadCopyMemoryEffect::Read);
  const bool write = HasMemoryEffect(effect, InlineLoadCopyMemoryEffect::Write);
  return read && write ? "rw" : read ? "r" : write ? "w" : "none";
}

inline bool IsElidedOperation(const std::string &name) {
  return name == "memref.alloc" || name == "memref.alloca" ||
         name == "tensor.empty" || name == "bufferization.alloc_tensor" ||
         name == "bufferization.to_tensor" || name == "annotation.mark" ||
         name == "memref.dim" ||
         IsControlAliasOperation(name) || IsViewLikeOperation(name) ||
         name == "memref.transpose";
}

inline std::string BufferizedOperationName(const std::string &name) {
  if (name == "tensor.extract")
    return "memref.load";
  if (name == "tensor.insert")
    return "memref.store";
  return name;
}

inline bool SinglePointBufferSurvives(
    const std::string &buffer,
    const std::map<std::string, std::string> &bufferMapping) {
  for (const std::string &alternative : BufferAlternatives(buffer))
    if (bufferMapping.count(alternative) != 0)
      return true;
  return false;
}

inline std::string OriginalOperationBuffer(const PostBufferizationRewriteState &module,
                                             int operation,
                                             size_t operand) {
  for (const BufferizedOperandAccess &access : module.bufferized.accesses)
    if (access.operationId == operation &&
        access.operandNumber == static_cast<int>(operand))
      return access.bufferId;
  return "";
}

inline std::string ValueBuffer(const BufferizedSemanticIR &module,
                                 int value) {
  for (const BufferizedValueBinding &binding : module.values)
    if (binding.valueId == value)
      return binding.bufferId;
  return "";
}

inline bool ValueHasUser(const GenericModule &module, int value) {
  return std::any_of(module.operations.begin(), module.operations.end(),
                     [&](const GenericOperation &operation) {
                       return std::find(operation.operands.begin(),
                                        operation.operands.end(), value) !=
                              operation.operands.end();
                     });
}

inline const GenericOperation *DefiningOperation(
    const GenericModule &module, int value) {
  for (const GenericOperation &operation : module.operations)
    if (std::find(operation.results.begin(), operation.results.end(), value) !=
        operation.results.end())
      return &operation;
  return nullptr;
}

inline bool IsOutOfPlaceOperand(const PostBufferizationRewriteState &module, int operation,
                                  size_t operand) {
  const std::string source = "operand:" + std::to_string(operation) + ":" +
                             std::to_string(operand);
  return std::any_of(module.bufferized.allocations.begin(),
                     module.bufferized.allocations.end(),
                     [&](const BufferAllocation &allocation) {
                       return allocation.source == source;
                     });
}

inline bool TensorInsertMustMaterialize(const PostBufferizationRewriteState &module,
                                          const GenericOperation &insert) {
  if (insert.results.empty())
    return false;
  const int result = insert.results.front();
  for (const GenericOperation &user : module.bufferized.logicalModule.operations) {
    if (std::find(user.operands.begin(), user.operands.end(), result) ==
        user.operands.end())
      continue;
    if (module.singlePoint.scalarizedOperations.count(user.id) == 0)
      return true;
  }
  return false;
}

inline std::string PhysicalTypeSignature(const std::string &type) {
  const std::optional<MemRefTypeModel> parsed = ParseMemRefType(type);
  if (!parsed)
    throw std::runtime_error("PlanMemoryInput: expected memref type: " + type);
  std::ostringstream output;
  output << parsed->elementType << "|shape=";
  for (size_t index = 0; index < parsed->shape.size(); ++index) {
    if (index)
      output << ',';
    if (parsed->shape[index])
      output << *parsed->shape[index];
    else
      output << '?';
  }
  output << "|strides=";
  for (size_t index = 0; index < parsed->strides.size(); ++index) {
    if (index)
      output << ',';
    if (parsed->strides[index])
      output << *parsed->strides[index];
    else
      output << '?';
  }
  output << "|offset=";
  if (parsed->offset == std::numeric_limits<int64_t>::min())
    output << '?';
  else
    output << parsed->offset;
  return output.str();
}

inline std::string PhysicalBufferTypeSignature(
    const LocalBufferRecord &buffer, const AlignStorageResult &alignStorage) {
  const std::optional<MemRefTypeModel> parsed = ParseMemRefType(buffer.type);
  if (!parsed)
    throw std::runtime_error("PlanMemoryInput: expected buffer memref type");
  const uint64_t bitWidth = GetElementBitWidth(*parsed);
  if (bitWidth == 0)
    throw std::runtime_error("PlanMemoryInput: invalid allocation element width");
  if (std::any_of(parsed->shape.begin(), parsed->shape.end(),
                  [](const std::optional<int64_t> &extent) {
                    return !extent.has_value();
                  })) {
    if (buffer.constBits % 8 != 0)
      throw std::runtime_error("PlanMemoryInput: dynamic byte allocation is not integral");
    return PhysicalTypeSignature(
        "memref<" + std::to_string(buffer.constBits / 8) + "xi8>");
  }
  std::vector<uint64_t> shape;
  for (size_t axis = 0; axis < parsed->shape.size(); ++axis) {
    const std::optional<int64_t> extent = parsed->shape[axis];
    if (!extent || *extent < 0)
      throw std::runtime_error("PlanMemoryInput: dynamic physical allocation shape: " +
                               buffer.sourceIdentity);
    shape.push_back(static_cast<uint64_t>(*extent));
  }
  auto sizeAlign = alignStorage.allocAlignments.find(buffer.sourceIdentity);
  if (sizeAlign != alignStorage.allocAlignments.end()) {
    if (bitWidth % 8 != 0)
      throw std::runtime_error("PlanMemoryInput: sub-byte size-aligned allocation");
    const uint64_t elementBytes = bitWidth / 8;
    for (const auto &[axis, alignmentBytes] : sizeAlign->second) {
      if (axis >= shape.size() || alignmentBytes % elementBytes != 0)
        throw std::runtime_error("PlanMemoryInput: invalid allocation size alignment");
      shape[axis] = AlignUp(shape[axis], alignmentBytes / elementBytes);
    }
  }

  auto strideAlign = alignStorage.strideAlignments.find(buffer.sourceIdentity);
  if (strideAlign != alignStorage.strideAlignments.end()) {
    if (256 % bitWidth != 0)
      throw std::runtime_error("PlanMemoryInput: invalid stride alignment width");
    std::vector<uint64_t> targets(shape.size(), 1);
    for (size_t axis : strideAlign->second)
      targets.at(axis) = std::lcm(targets.at(axis), 256 / bitWidth);
    std::vector<uint64_t> units(shape.size() + 1, 1);
    uint64_t innerAlignedUnits = 1;
    uint64_t shapeAccumulation = 1;
    for (size_t reverse = shape.size(); reverse > 0; --reverse) {
      const size_t axis = reverse - 1;
      const uint64_t next = std::lcm(innerAlignedUnits, targets[axis]);
      units[axis + 1] = shapeAccumulation % next == 0
                            ? 1
                            : next / innerAlignedUnits;
      innerAlignedUnits = next;
      shapeAccumulation = CheckedMul(
          shapeAccumulation, std::lcm(shape[axis], units[axis + 1]),
          "PlanMemoryInput aligned allocation shape");
    }
    bool changed = false;
    for (size_t axis = 0; axis < shape.size(); ++axis)
      if (units[axis] != 1)
        shape[axis] = AlignUp(shape[axis], units[axis]), changed = true;
    if (units.back() != 1)
      changed = true;
    if (changed)
      shape.push_back(units.back());
  }

  uint64_t elements = 1;
  for (uint64_t extent : shape)
    elements = CheckedMul(elements, extent, "PlanMemoryInput physical element count");
  const uint64_t physicalBits =
      CheckedMul(elements, bitWidth, "PlanMemoryInput physical allocation bits");
  if (physicalBits != buffer.constBits)
    throw std::runtime_error(
        "PlanMemoryInput: physical shape does not match constBits: " +
        buffer.sourceIdentity + " calculated=" + std::to_string(physicalBits) +
        " expected=" + std::to_string(buffer.constBits));
  std::ostringstream type;
  type << "memref<";
  for (uint64_t extent : shape)
    type << extent << 'x';
  type << parsed->elementType << '>';
  return PhysicalTypeSignature(type.str());
}

inline bool IsAIVFunction(const GenericOperation &function) {
  return DecomposeEnumValue(
             FindDictionaryValue(function.attributes, "hivm.func_core_type")) ==
         "AIV";
}

inline PlanMemoryInputSemanticIR
BuildPlanMemoryInputSemanticIR(AfterMarkMultiBufferState afterMarkMultiBuffer) {
  PlanMemoryInputSemanticIR result;
  const GenericModule &module = afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.bufferized.logicalModule;
  for (size_t ordinal = 0; ordinal < afterMarkMultiBuffer.afterInlineLoadCopy.buffers.size(); ++ordinal) {
    const LocalBufferRecord &buffer = afterMarkMultiBuffer.afterInlineLoadCopy.buffers[ordinal];
    if (buffer.addressSpace != AddressSpace::UB)
      continue;
    const int ownerId = BufferOwnerOperation(afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer, buffer);
    if (ownerId < 0 || static_cast<size_t>(ownerId) >= module.operations.size())
      throw std::runtime_error("PlanMemoryInput: buffer owner operation is missing");
    const GenericOperation *function = EnclosingFunction(
        module, module.operations.at(static_cast<size_t>(ownerId)));
    if (!function || !IsAIVFunction(*function))
      continue;
    auto multi = afterMarkMultiBuffer.markMultiBuffer.buffer2MultiNum.find(buffer.sourceIdentity);
    result.buffers.push_back(
        {ordinal, buffer.identity, buffer.sourceIdentity,
         GenericFunctionName(*function),
         PhysicalBufferTypeSignature(buffer, afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.alignStorage),
         buffer.constBits, buffer.extraBuffer,
         multi == afterMarkMultiBuffer.markMultiBuffer.buffer2MultiNum.end() ? 1U
                                                           : multi->second,
         afterMarkMultiBuffer.markMultiBuffer.preloadLocalBuffers.count(buffer.sourceIdentity) !=
             0});
  }
  std::map<std::string, std::string> finalIdentity;
  std::set<std::string> targetBuffers;
  for (const LocalBufferRecord &buffer : afterMarkMultiBuffer.afterInlineLoadCopy.buffers)
    finalIdentity[buffer.sourceIdentity] = buffer.identity;
  for (const PlanMemoryInputBufferRecord &buffer : result.buffers)
    targetBuffers.insert(buffer.sourceIdentity);
  std::map<int, const OperationRewriteDelta *> rewrites;
  for (const OperationRewriteDelta &rewrite : afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.operationRewrites)
    rewrites[rewrite.sourceOperation] = &rewrite;
  std::map<int, const InlineLoadCopyRewrite *> inlineLoads;
  for (const InlineLoadCopyRewrite &rewrite : afterMarkMultiBuffer.afterInlineLoadCopy.inlineLoadCopy.rewrites)
    inlineLoads[rewrite.copyOperation] = &rewrite;
  std::map<std::pair<int, std::string>, std::vector<std::string>> extraBuffers;
  for (const LocalBufferRecord &buffer : afterMarkMultiBuffer.afterInlineLoadCopy.buffers) {
    if (!buffer.extraBuffer)
      continue;
    const int owner = BufferOwnerOperation(afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer, buffer);
    extraBuffers[{owner, buffer.ownerName}].push_back(buffer.sourceIdentity);
  }
  std::map<std::pair<int, std::string>, size_t> nextExtraBuffer;
  std::map<std::string, size_t> functionOrdinals;
  auto append = [&](const GenericOperation &source, const std::string &name,
                    std::vector<std::string> buffers,
                    const std::string &properties = "",
                    bool buffersArePostSinglePoint = false,
                    size_t generatedOrdinal = 0,
                    bool outOfPlaceCopy = false,
                    bool preserveSourceProperties = false,
                    bool decomposedScalarVSub = false,
                    int sourceOperandNumber = -1) {
    const GenericOperation *function = EnclosingFunction(module, source);
    if (!function || !IsAIVFunction(*function) || IsElidedOperation(name))
      return;
    const auto extraKey = std::make_pair(source.id, name);
    auto extra = extraBuffers.find(extraKey);
    bool appendedExtraBuffer = false;
    if (extra != extraBuffers.end()) {
      size_t &next = nextExtraBuffer[extraKey];
      if (next < extra->second.size()) {
        buffers.push_back(extra->second[next++]);
        appendedExtraBuffer = true;
      }
    }
    PlanMemoryInputAccessRecord event;
    event.functionName = GenericFunctionName(*function);
    event.ordinal = functionOrdinals[event.functionName]++;
    event.sourceOperation = source.id;
    event.sourceOperationName = source.name;
    event.operationName = BufferizedOperationName(name);
    event.generatedOperation = buffersArePostSinglePoint;
    event.generatedOrdinal = generatedOrdinal;
    event.outOfPlaceCopy = outOfPlaceCopy;
    event.sourceOperandNumber = sourceOperandNumber;
    event.preserveSourceProperties = preserveSourceProperties;
    event.decomposedScalarVSub = decomposedScalarVSub;
    for (size_t operand = 0; operand < buffers.size(); ++operand) {
      std::set<std::string> candidates;
      for (const std::string &alternative :
           BufferAlternatives(buffers[operand])) {
        const std::string mapped =
            buffersArePostSinglePoint && startsWith(alternative, "local:")
                ? "base:" + alternative.substr(6)
                : MappedBufferIdentity(
                      alternative,
                      afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.singlePoint.bufferMapping);
        if (targetBuffers.count(mapped) != 0)
          candidates.insert(mapped);
      }
      if (candidates.empty())
        continue;
      if (candidates.size() != 1)
        continue;
      const std::string &identity = *candidates.begin();
      auto final = finalIdentity.find(identity);
      if (final == finalIdentity.end())
        throw std::runtime_error("PlanMemoryInput: access references missing buffer");
      std::string effectProperties = properties;
      if (effectProperties.empty() && appendedExtraBuffer &&
          IsDestinationStyleOp(event.operationName) && buffers.size() >= 2)
        effectProperties =
            "operandSegmentSizes = array<i32: " +
            std::to_string(buffers.size() - 2) + ", 1, 1>";
      event.accesses.push_back(
          {final->second, AccessEffect(event.operationName, operand,
                                         buffers.size(), effectProperties)});
      if (appendedExtraBuffer && operand + 1 == buffers.size())
        event.temporaryBuffers.insert(final->second);
    }
    if (!event.accesses.empty())
      result.accesses.push_back(std::move(event));
    else
      --functionOrdinals[event.functionName];
  };

  for (const GenericOperation &operation : module.operations) {
    if (afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.bufferized.preBufferizationCSE.erasedOperations.count(
            operation.id) != 0)
      continue;
    const bool tensorSinglePointBufferization =
        operation.name == "tensor.insert" || operation.name == "tensor.extract";
    if (operation.name == "tensor.extract" && !operation.results.empty() &&
        !ValueHasUser(module, operation.results.front()))
      continue;
    if (operation.name == "tensor.extract" && !operation.operands.empty()) {
      const GenericOperation *definition =
          DefiningOperation(module, operation.operands.front());
      if (definition &&
          afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.singlePoint.scalarizedOperations.count(definition->id) !=
              0)
        continue;
    }
    if (afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.singlePoint.scalarizedOperations.count(operation.id) != 0 &&
        !tensorSinglePointBufferization) {
      const std::map<size_t, std::string> scalarBuffers =
          OperationBufferMap(afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization, operation.id);
      const std::vector<size_t> destinations = DpsInitOperandIndices(
          operation.name, operation.operands.size(), operation.properties);
      for (size_t input = 0; input < operation.operandTypes.size(); ++input) {
        if ((!IsTensorType(operation.operandTypes[input]) &&
             !IsMemRefType(operation.operandTypes[input])) ||
            std::find(destinations.begin(), destinations.end(), input) !=
                destinations.end())
          continue;
        auto buffer = scalarBuffers.find(input);
        if (buffer == scalarBuffers.end())
          continue;
        const std::string original = OriginalOperationBuffer(
            afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization, operation.id, input);
        if (SinglePointBufferSurvives(
                original, afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.singlePoint.bufferMapping))
          append(operation, "memref.load", {buffer->second});
      }
      for (size_t destination : destinations) {
        auto buffer = scalarBuffers.find(destination);
        if (buffer == scalarBuffers.end())
          continue;
        const std::string original = OriginalOperationBuffer(
            afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization, operation.id, destination);
        if (SinglePointBufferSurvives(
                original, afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.singlePoint.bufferMapping))
          append(operation, "memref.store", {"", buffer->second});
      }
      continue;
    }
    auto inlineLoad = inlineLoads.find(operation.id);
    if (inlineLoad == inlineLoads.end() &&
        afterMarkMultiBuffer.afterInlineLoadCopy.inlineLoadCopy
                .erasedOperations.count(operation.id) != 0)
      continue;
    auto rewritten = rewrites.find(operation.id);
    if (rewritten != rewrites.end()) {
      for (size_t generatedOrdinal = 0;
           generatedOrdinal < rewritten->second->replacement.size();
           ++generatedOrdinal) {
        const GeneratedOperationRewrite &generated =
            rewritten->second->replacement[generatedOrdinal];
        if (generated.name == "hivm.hir.vbrc" &&
            generated.buffers.size() == 1)
          append(operation, generated.name, {"", generated.buffers.front()},
                 "", true, generatedOrdinal, false,
                 generated.preserveSourceProperties,
                 generated.decomposedScalarVSub);
        else
          append(operation, generated.name, generated.buffers, "", true,
                 generatedOrdinal, false,
                 generated.preserveSourceProperties,
                 generated.decomposedScalarVSub);
      }
      continue;
    }
    const std::map<size_t, std::string> operationBuffers =
        OperationBufferMap(afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization, operation.id);
    std::vector<std::string> buffers(operation.operands.size());
    for (const auto &[operand, buffer] : operationBuffers)
      if (operand < buffers.size() &&
          afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.singlePoint.canonicalizedIterArgAccesses.count(
              {operation.id, static_cast<int>(operand)}) == 0)
        buffers[operand] = buffer;
    if (operation.name == "hivm.hir.vconcat") {
      const std::vector<size_t> destinations = DpsInitOperandIndices(
          operation.name, operation.operands.size(), operation.properties);
      if (destinations.size() != 1 || destinations.front() >= buffers.size())
        throw std::runtime_error("PlanMemoryInput: vconcat must have one destination");
      const size_t destination = destinations.front();
      for (size_t input = 0; input < destination; ++input)
        if (!buffers[input].empty())
          append(operation, "hivm.hir.copy",
                 {buffers[input], buffers[destination]}, "", false, input);
      continue;
    }
    if (operation.name == "tensor.from_elements" &&
        !operation.results.empty()) {
      const std::string destination =
          ValueBuffer(afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.bufferized, operation.results.front());
      for (size_t element = 0; element < operation.operands.size(); ++element)
        append(operation, "memref.store", {"", destination}, "", false,
               element);
      continue;
    }
    const bool materializedTensorInsert =
        operation.name == "tensor.insert" &&
        TensorInsertMustMaterialize(afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization, operation);
    if (materializedTensorInsert && buffers.size() > 1 &&
        !operation.results.empty()) {
      const std::string resultBuffer = ValueBuffer(
          afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.bufferized, operation.results.front());
      if (!resultBuffer.empty())
        buffers[1] = resultBuffer;
    }
    for (size_t operand = 0; operand < operation.operandTypes.size(); ++operand) {
      if (!IsTensorType(operation.operandTypes[operand]) ||
          operand >= buffers.size() || buffers[operand].empty() ||
          !IsOutOfPlaceOperand(afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization, operation.id, operand))
        continue;
      const std::vector<size_t> initOperands = DpsInitOperandIndices(
          operation.name, operation.operands.size(), operation.properties);
      if (startsWith(operation.name, "hivm.hir.v") &&
          std::find(initOperands.begin(), initOperands.end(), operand) !=
              initOperands.end())
        continue;
      const std::string destination = MappedBufferIdentity(
          buffers[operand], afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.singlePoint.bufferMapping);
      if (targetBuffers.count(destination) == 0)
        continue;
      const std::string source =
          ValueBuffer(afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.bufferized, operation.operands[operand]);
      if (!source.empty())
        append(operation, "hivm.hir.copy", {source, buffers[operand]},
               "", false, 0, true, false, false,
               static_cast<int>(operand));
    }
    if (inlineLoad != inlineLoads.end()) {
      append(operation, "hivm.hir.load",
             {inlineLoad->second->loadSource,
              inlineLoad->second->copyDestination});
      continue;
    }
    if (tensorSinglePointBufferization) {
      const size_t bufferOperand = operation.name == "tensor.insert" ? 1 : 0;
      if (bufferOperand >= buffers.size() ||
          (!SinglePointBufferSurvives(
               OriginalOperationBuffer(afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization, operation.id,
                                         bufferOperand),
               afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.singlePoint.bufferMapping) &&
           !materializedTensorInsert))
        continue;
    }
    if (operation.name == "tensor.insert_slice") {
      if (buffers.size() > 1)
        append(operation, "hivm.hir.copy", {buffers[0], buffers[1]});
      continue;
    }
    std::string name = BufferizedOperationName(operation.name);
    if (name == "hivm.hir.load" &&
        HasBooleanProperty(operation, "init_out_buffer") &&
        buffers.size() > 1)
      append(operation, "hivm.hir.vbrc", {"", buffers[1]});
    if (name == "hivm.hir.load" && buffers.size() > 1) {
      const std::string destination = MappedBufferIdentity(
          buffers[1], afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.singlePoint.bufferMapping);
      const LocalBufferRecord *record =
          FindSourceBuffer(afterMarkMultiBuffer.afterInlineLoadCopy.buffers, destination);
      if (record && record->addressSpace == AddressSpace::L1)
        name = "hivm.hir.nd2nz";
    }
    append(operation, name, buffers, operation.properties);
  }
  result.afterMarkMultiBuffer = std::move(afterMarkMultiBuffer);
  return result;
}

inline std::string SerializePlanMemoryInputBuffers(
    const PlanMemoryInputSemanticIR &module) {
  std::ostringstream output;
  output << "PLANMEMORY_INPUT_BUFFERS\t1\n";
  for (const PlanMemoryInputBufferRecord &buffer : module.buffers)
    output << "BUFFER\t" << buffer.globalOrdinal << '\t'
           << HexEncode(buffer.identity) << '\t'
           << HexEncode(buffer.functionName) << '\t'
           << HexEncode(buffer.physicalType) << '\t' << buffer.constBits << '\t'
           << (buffer.extraBuffer ? 1 : 0) << '\t' << buffer.multiBufferNum
           << '\t' << (buffer.preloadLocalBuffer ? 1 : 0) << '\n';
  return output.str();
}

inline std::string SerializePlanMemoryInputAccesses(
    const PlanMemoryInputSemanticIR &module) {
  std::ostringstream output;
  output << "PLANMEMORY_INPUT_ACCESSES\t1\n";
  for (const std::string &blocker : module.accessBlockers)
    output << "ACCESS_BLOCKER\t" << HexEncode(blocker) << '\n';
  for (const PlanMemoryInputAccessRecord &event : module.accesses) {
    output << "EVENT\t" << HexEncode(event.functionName) << '\t'
           << event.ordinal << '\t' << HexEncode(event.operationName);
    for (const auto &[buffer, effect] : event.accesses)
      output << '\t' << HexEncode(buffer) << ':' << effect;
    output << '\n';
  }
  return output.str();
}

inline std::string SerializePlanMemoryInputAccessDebug(
    const PlanMemoryInputSemanticIR &module) {
  std::ostringstream output;
  output << "PLANMEMORY_INPUT_ACCESS_DEBUG\t1\n";
  for (const std::string &blocker : module.accessBlockers)
    output << "ACCESS_BLOCKER\t" << HexEncode(blocker) << '\n';
  for (const PlanMemoryInputAccessRecord &event : module.accesses) {
    output << "EVENT\t" << HexEncode(event.functionName) << '\t'
           << event.ordinal << '\t' << event.sourceOperation << '\t'
           << HexEncode(event.sourceOperationName) << '\t'
           << HexEncode(event.operationName);
    for (const auto &[buffer, effect] : event.accesses)
      output << '\t' << HexEncode(buffer) << ':' << effect;
    output << '\n';
  }
  return output.str();
}

inline std::string SerializePlanMemoryInputSemanticIR(
    const PlanMemoryInputSemanticIR &module) {
  return "PLANMEMORY_INPUT_SEMANTIC_IR\t1\n" +
         SerializeAfterMarkMultiBufferState(module.afterMarkMultiBuffer) +
         SerializePlanMemoryInputBuffers(module) +
         SerializePlanMemoryInputAccesses(module);
}

} // namespace cvub

#endif
