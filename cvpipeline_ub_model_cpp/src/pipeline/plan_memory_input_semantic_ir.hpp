#ifndef CVPIPELINE_UB_MODEL_CPP_PLANMEMORY_INPUT_SEMANTIC_IR_HPP
#define CVPIPELINE_UB_MODEL_CPP_PLANMEMORY_INPUT_SEMANTIC_IR_HPP

#include "../ir/generic_analysis.hpp"
#include "after_mark_multi_buffer.hpp"

#include <string_view>

namespace cvub {

struct PlanMemoryInputBufferRecord {
  size_t globalOrdinal = 0;
  std::string identity;
  std::string sourceIdentity;
  std::string functionName;
  // Final PlanMemory allocation type.  Earlier revisions stored an
  // intermediate "element|shape|strides|offset" string here and immediately
  // parsed it again in PlanMemoryInputBuilder.  Keep the typed projection in
  // its final textual form so the normal path performs the conversion once.
  std::string memrefType;
  uint64_t constBits = 0;
  bool extraBuffer = false;
  uint32_t multiBufferNum = 1;
  bool preloadLocalBuffer = false;
};

enum class PlanMemoryAccessEffect : uint8_t {
  None,
  Read,
  Write,
  ReadWrite,
};

inline const char *PlanMemoryAccessEffectName(PlanMemoryAccessEffect effect) {
  switch (effect) {
  case PlanMemoryAccessEffect::None:
    return "none";
  case PlanMemoryAccessEffect::Read:
    return "r";
  case PlanMemoryAccessEffect::Write:
    return "w";
  case PlanMemoryAccessEffect::ReadWrite:
    return "rw";
  }
  throw std::runtime_error("PlanMemoryInput: unknown access effect");
}

inline bool operator==(PlanMemoryAccessEffect effect,
                       std::string_view name) {
  return name == PlanMemoryAccessEffectName(effect);
}

inline bool operator!=(PlanMemoryAccessEffect effect,
                       std::string_view name) {
  return !(effect == name);
}

inline std::ostream &operator<<(std::ostream &output,
                                PlanMemoryAccessEffect effect) {
  return output << PlanMemoryAccessEffectName(effect);
}

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
  using Access = std::pair<std::string, PlanMemoryAccessEffect>;
  std::vector<Access> accesses;
  std::set<std::string> temporaryBuffers;
};

// Shared, serialization-independent indexes for the immutable planning
// state.  Stages keep their existing semantic records and debug formats, but
// downstream consumers no longer rebuild the same string/owner/rewrite maps
// from the nested OneShot-to-MultiBuffer state.
struct UBPlanningContext {
  std::vector<std::vector<bool>> outOfPlaceOperands;
  std::map<std::string, size_t> afterAllocBufferBySource;
  std::map<std::string, size_t> liveBufferBySource;
  std::vector<int> liveBufferOwners;
  // Generic operation IDs are dense.  Dense ordinal tables avoid allocating
  // tree nodes for indexes that are queried once per operation while
  // projecting the final PlanMemory event stream.
  std::vector<size_t> operationRewriteBySource;
  std::vector<size_t> inlineLoadByCopyOperation;
  std::vector<bool> erasedInlineLoadOperations;
};

struct PlanMemoryInputSemanticIR {
  AfterMarkMultiBufferState afterMarkMultiBuffer;
  UBPlanningContext planningContext;
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

inline std::vector<PlanMemoryAccessEffect>
AccessEffects(const std::string &name, size_t operandCount,
              const std::string &properties,
              PipelineMetadataCache &metadata) {
  std::vector<PlanMemoryAccessEffect> result(
      operandCount, PlanMemoryAccessEffect::ReadWrite);
  if (IsControlAliasOperation(name)) {
    std::fill(result.begin(), result.end(), PlanMemoryAccessEffect::None);
    return result;
  }
  std::string synthesizedProperties;
  const std::string *effectiveProperties = &properties;
  if (properties.empty() && IsDestinationStyleOp(name) &&
      name != "hivm.hir.varange" && name != "hivm.hir.vconcat" &&
      operandCount != 0)
    synthesizedProperties =
        "operandSegmentSizes = array<i32: " +
        std::to_string(operandCount - 1) + ", 1>";
  if (!synthesizedProperties.empty())
    effectiveProperties = &synthesizedProperties;
  // OperandMemoryEffect used to call DpsInitOperandIndices once per operand.
  // Compute the destination set once per event and project all effects in a
  // single pass instead.
  std::vector<bool> destinations(operandCount, false);
  if (IsDestinationStyleOp(name))
    for (size_t operand : metadata.dpsInitOperandIndices(
             name, operandCount, *effectiveProperties))
      if (operand < destinations.size())
        destinations[operand] = true;
  for (size_t operand = 0; operand < operandCount; ++operand) {
    InlineLoadCopyMemoryEffect effect;
    if (IsViewLikeOperation(name))
      effect = InlineLoadCopyMemoryEffect::None;
    else if (name == "hivm.hir.load" || name == "hivm.hir.copy" ||
             name == "hivm.hir.store" || name == "tensor.insert_slice")
      effect = operand == 0 ? InlineLoadCopyMemoryEffect::Read
                            : InlineLoadCopyMemoryEffect::Write;
    else if (name == "memref.load")
      effect = InlineLoadCopyMemoryEffect::Read;
    else if (name == "memref.store")
      effect = operand == 0 ? InlineLoadCopyMemoryEffect::Read
                            : InlineLoadCopyMemoryEffect::Write;
    else if (IsDestinationStyleOp(name))
      effect = destinations[operand] ? InlineLoadCopyMemoryEffect::Write
                                     : InlineLoadCopyMemoryEffect::Read;
    else
      effect = InlineLoadCopyMemoryEffect::Read |
               InlineLoadCopyMemoryEffect::Write;
    const bool read =
        HasMemoryEffect(effect, InlineLoadCopyMemoryEffect::Read);
    const bool write =
        HasMemoryEffect(effect, InlineLoadCopyMemoryEffect::Write);
    result[operand] =
        read && write ? PlanMemoryAccessEffect::ReadWrite
        : read        ? PlanMemoryAccessEffect::Read
        : write       ? PlanMemoryAccessEffect::Write
                      : PlanMemoryAccessEffect::None;
  }
  return result;
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

inline std::string PhysicalBufferMemRefType(
    const LocalBufferRecord &buffer, const AlignStorageResult &alignStorage,
    PipelineMetadataCache &metadata) {
  const std::optional<MemRefTypeModel> &parsed =
      metadata.memRefType(buffer.type);
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
    return "memref<" + std::to_string(buffer.constBits / 8) +
           "xi8, strided<[1], offset: 0>, #hivm.address_space<ub>>";
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
  std::vector<uint64_t> strides(shape.size(), 1);
  for (size_t reverse = shape.size(); reverse > 1; --reverse)
    strides[reverse - 2] = CheckedMul(
        strides[reverse - 1], shape[reverse - 1],
        "PlanMemoryInput physical allocation stride");
  std::ostringstream type;
  type << "memref<";
  for (uint64_t extent : shape)
    type << extent << 'x';
  type << parsed->elementType << ", strided<[";
  for (size_t index = 0; index < strides.size(); ++index) {
    if (index)
      type << ", ";
    type << strides[index];
  }
  type << "], offset: 0>, #hivm.address_space<ub>>";
  return type.str();
}

inline bool IsAIVFunction(const GenericOperation &function) {
  return DecomposeEnumValue(
             FindDictionaryValue(function.attributes, "hivm.func_core_type")) ==
         "AIV";
}

inline UBPlanningContext
BuildUBPlanningContext(const AfterMarkMultiBufferState &state) {
  UBPlanningContext result;
  const AfterInlineLoadCopyState &afterInline = state.afterInlineLoadCopy;
  const AfterAllocExtraBufferState &afterAlloc =
      afterInline.afterAllocExtraBuffer;
  const PostBufferizationRewriteState &post = afterAlloc.postBufferization;
  const size_t operationCount = post.bufferized.logicalModule.operations.size();
  result.outOfPlaceOperands.resize(operationCount);
  for (const GenericOperation &operation :
       post.bufferized.logicalModule.operations)
    result.outOfPlaceOperands[static_cast<size_t>(operation.id)].assign(
        operation.operands.size(), false);
  result.operationRewriteBySource.assign(operationCount,
                                         std::numeric_limits<size_t>::max());
  result.inlineLoadByCopyOperation.assign(operationCount,
                                          std::numeric_limits<size_t>::max());
  result.erasedInlineLoadOperations.assign(operationCount, false);
  for (const BufferAllocation &allocation : post.bufferized.allocations) {
    if (!startsWith(allocation.source, "operand:"))
      continue;
    const size_t operationEnd = allocation.source.find(':', 8);
    if (operationEnd == std::string::npos)
      continue;
    const size_t operation = static_cast<size_t>(
        std::stoull(allocation.source.substr(8, operationEnd - 8)));
    const size_t operand = static_cast<size_t>(
        std::stoull(allocation.source.substr(operationEnd + 1)));
    if (operation < result.outOfPlaceOperands.size() &&
        operand < result.outOfPlaceOperands[operation].size())
      result.outOfPlaceOperands[operation][operand] = true;
  }
  for (size_t ordinal = 0; ordinal < afterAlloc.buffers.size(); ++ordinal)
    result.afterAllocBufferBySource.emplace(
        afterAlloc.buffers[ordinal].sourceIdentity, ordinal);
  result.liveBufferOwners.reserve(afterInline.buffers.size());
  for (size_t ordinal = 0; ordinal < afterInline.buffers.size(); ++ordinal) {
    const LocalBufferRecord &buffer = afterInline.buffers[ordinal];
    result.liveBufferBySource.emplace(buffer.sourceIdentity, ordinal);
    result.liveBufferOwners.push_back(BufferOwnerOperation(afterAlloc, buffer));
  }
  for (size_t ordinal = 0; ordinal < post.operationRewrites.size(); ++ordinal) {
    const int source = post.operationRewrites[ordinal].sourceOperation;
    if (source >= 0 && static_cast<size_t>(source) < operationCount)
      result.operationRewriteBySource[static_cast<size_t>(source)] = ordinal;
  }
  for (size_t ordinal = 0; ordinal < afterInline.inlineLoadCopy.rewrites.size();
       ++ordinal) {
    const InlineLoadCopyRewrite &rewrite =
        afterInline.inlineLoadCopy.rewrites[ordinal];
    if (rewrite.copyOperation >= 0 &&
        static_cast<size_t>(rewrite.copyOperation) < operationCount)
      result.inlineLoadByCopyOperation[
          static_cast<size_t>(rewrite.copyOperation)] = ordinal;
    if (rewrite.loadOperation >= 0 &&
        static_cast<size_t>(rewrite.loadOperation) < operationCount)
      result.erasedInlineLoadOperations[
          static_cast<size_t>(rewrite.loadOperation)] = true;
  }
  return result;
}

inline PlanMemoryInputSemanticIR
BuildPlanMemoryInputSemanticIR(AfterMarkMultiBufferState afterMarkMultiBuffer) {
  PlanMemoryInputSemanticIR result;
  UBPlanningContext planningContext =
      BuildUBPlanningContext(afterMarkMultiBuffer);
  const PostBufferizationRewriteState &postBufferization =
      afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer
          .postBufferization;
  const BufferizedSemanticIR &bufferized = postBufferization.bufferized;
  const GenericModule &module = bufferized.logicalModule;
  result.buffers.reserve(
      afterMarkMultiBuffer.afterInlineLoadCopy.buffers.size());
  result.accesses.reserve(module.operations.size());

  // Build the stage-local indexes once.  The original staged implementation
  // answered these queries by repeatedly scanning operations, accesses,
  // values, and allocations while materializing the final access stream.
  const GenericModuleAnalysisIndexes &analysis =
      bufferized.logicalContext.analysis;
  PipelineMetadataCache &metadata = bufferized.logicalContext.metadata;
  analysis.ensureCompatible(module);
  std::vector<bool> aivFunctions(module.operations.size(), false);
  std::vector<std::string> functionNames(module.operations.size());
  for (const GenericOperation &operation : module.operations) {
    if (operation.name != "func.func")
      continue;
    aivFunctions[static_cast<size_t>(operation.id)] =
        DecomposeEnumValue(metadata.dictionaryValue(
            operation.attributes, "hivm.func_core_type")) == "AIV";
    std::string functionName =
        metadata.dictionaryValue(operation.properties, "sym_name");
    if (functionName.empty())
      functionName =
          metadata.dictionaryValue(operation.attributes, "sym_name");
    if (functionName.size() >= 2 && functionName.front() == '"' &&
        functionName.back() == '"')
      functionName = functionName.substr(1, functionName.size() - 2);
    functionNames[static_cast<size_t>(operation.id)] =
        std::move(functionName);
  }
  auto definingOperation = [&](int value) -> const GenericOperation * {
    const int operation = analysis.definingOperationId(value);
    return operation < 0
               ? nullptr
               : &module.operations.at(static_cast<size_t>(operation));
  };
  auto valueHasUser = [&](int value) {
    return !analysis.users(value).empty();
  };
  auto valueBuffer = [&](int value) {
    const std::string *buffer = FindBufferizedValueBuffer(bufferized, value);
    return buffer ? *buffer : std::string();
  };
  auto originalOperationBuffer = [&](int operation, size_t operand) {
    const std::string *buffer =
        FindBufferizedOperationBuffer(bufferized, operation, operand);
    return buffer ? *buffer : std::string();
  };
  std::map<std::string, std::string> mappedBufferCache;
  auto mappedBufferIdentity = [&](const std::string &buffer)
      -> const std::string & {
    auto found = mappedBufferCache.find(buffer);
    if (found != mappedBufferCache.end())
      return found->second;
    return mappedBufferCache
        .emplace(buffer,
                 MappedBufferIdentity(
                     buffer, postBufferization.singlePoint.bufferMapping))
        .first->second;
  };
  auto mappedOperationBuffers = [&](const GenericOperation &operation) {
    std::vector<std::string> buffers(operation.operands.size());
    ForEachBufferizedOperationBuffer(
        bufferized, operation.id,
        [&](size_t operand, const std::string &buffer) {
          if (operand < buffers.size())
            buffers[operand] = mappedBufferIdentity(buffer);
        });
    return buffers;
  };
  auto isOutOfPlaceOperand = [&](int operation, size_t operand) {
    return operation >= 0 &&
           static_cast<size_t>(operation) <
               planningContext.outOfPlaceOperands.size() &&
           operand < planningContext
                         .outOfPlaceOperands[static_cast<size_t>(operation)]
                         .size() &&
           planningContext
               .outOfPlaceOperands[static_cast<size_t>(operation)][operand];
  };
  auto tensorInsertMustMaterialize = [&](const GenericOperation &insert) {
    if (insert.results.empty())
      return false;
    const std::vector<int> &users = analysis.users(insert.results.front());
    if (users.empty())
      return false;
    return std::any_of(
        users.begin(), users.end(), [&](int user) {
          return postBufferization.singlePoint.scalarizedOperations.count(
                     user) == 0;
        });
  };
  for (size_t ordinal = 0; ordinal < afterMarkMultiBuffer.afterInlineLoadCopy.buffers.size(); ++ordinal) {
    const LocalBufferRecord &buffer = afterMarkMultiBuffer.afterInlineLoadCopy.buffers[ordinal];
    if (buffer.addressSpace != AddressSpace::UB)
      continue;
    const int ownerId = planningContext.liveBufferOwners.at(ordinal);
    if (ownerId < 0 || static_cast<size_t>(ownerId) >= module.operations.size())
      throw std::runtime_error("PlanMemoryInput: buffer owner operation is missing");
    const int functionId = analysis.enclosingFunctionId(ownerId);
    const GenericOperation *function =
        functionId < 0
            ? nullptr
            : &module.operations.at(static_cast<size_t>(functionId));
    if (!function ||
        !aivFunctions.at(static_cast<size_t>(function->id)))
      continue;
    auto multi = afterMarkMultiBuffer.markMultiBuffer.buffer2MultiNum.find(buffer.sourceIdentity);
    result.buffers.push_back(
        {ordinal, buffer.identity, buffer.sourceIdentity,
         functionNames.at(static_cast<size_t>(function->id)),
         PhysicalBufferMemRefType(
             buffer, afterMarkMultiBuffer.afterInlineLoadCopy
                         .afterAllocExtraBuffer.alignStorage,
             metadata),
         buffer.constBits, buffer.extraBuffer,
         multi == afterMarkMultiBuffer.markMultiBuffer.buffer2MultiNum.end()
             ? 1U
             : multi->second,
         afterMarkMultiBuffer.markMultiBuffer.preloadLocalBuffers.count(
             buffer.sourceIdentity) != 0});
  }
  std::map<std::string, std::string> finalIdentity;
  std::set<std::string> targetBuffers;
  for (const LocalBufferRecord &buffer :
       afterMarkMultiBuffer.afterInlineLoadCopy.buffers)
    finalIdentity[buffer.sourceIdentity] = buffer.identity;
  for (const PlanMemoryInputBufferRecord &buffer : result.buffers)
    targetBuffers.insert(buffer.sourceIdentity);
  std::map<std::pair<int, std::string>, std::vector<std::string>> extraBuffers;
  for (size_t ordinal = 0;
       ordinal < afterMarkMultiBuffer.afterInlineLoadCopy.buffers.size();
       ++ordinal) {
    const LocalBufferRecord &buffer =
        afterMarkMultiBuffer.afterInlineLoadCopy.buffers[ordinal];
    if (!buffer.extraBuffer)
      continue;
    const int owner = planningContext.liveBufferOwners.at(ordinal);
    extraBuffers[{owner, buffer.ownerName}].push_back(buffer.sourceIdentity);
  }
  std::map<std::pair<int, std::string>, size_t> nextExtraBuffer;
  std::vector<size_t> functionOrdinals(module.operations.size(), 0);
  auto append = [&](const GenericOperation &source, const std::string &name,
                    std::vector<std::string> buffers,
                    const std::string &properties = "",
                    bool buffersArePostSinglePoint = false,
                    size_t generatedOrdinal = 0,
                    bool outOfPlaceCopy = false,
                    bool preserveSourceProperties = false,
                    bool decomposedScalarVSub = false,
                    int sourceOperandNumber = -1) {
    const int functionId = analysis.enclosingFunctionId(source.id);
    const GenericOperation *function =
        functionId < 0
            ? nullptr
            : &module.operations.at(static_cast<size_t>(functionId));
    if (!function ||
        !aivFunctions.at(static_cast<size_t>(function->id)) ||
        IsElidedOperation(name))
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
    event.functionName = functionNames.at(static_cast<size_t>(function->id));
    size_t &functionOrdinal =
        functionOrdinals.at(static_cast<size_t>(function->id));
    event.ordinal = functionOrdinal++;
    event.sourceOperation = source.id;
    event.sourceOperationName = source.name;
    event.operationName = BufferizedOperationName(name);
    event.generatedOperation = buffersArePostSinglePoint;
    event.generatedOrdinal = generatedOrdinal;
    event.outOfPlaceCopy = outOfPlaceCopy;
    event.sourceOperandNumber = sourceOperandNumber;
    event.preserveSourceProperties = preserveSourceProperties;
    event.decomposedScalarVSub = decomposedScalarVSub;
    event.accesses.reserve(buffers.size());
    std::string effectProperties = properties;
    if (effectProperties.empty() && appendedExtraBuffer &&
        IsDestinationStyleOp(event.operationName) && buffers.size() >= 2)
      effectProperties =
          "operandSegmentSizes = array<i32: " +
          std::to_string(buffers.size() - 2) + ", 1, 1>";
    const std::vector<PlanMemoryAccessEffect> effects =
        AccessEffects(event.operationName, buffers.size(), effectProperties,
                      metadata);
    for (size_t operand = 0; operand < buffers.size(); ++operand) {
      std::set<std::string> candidates;
      for (const std::string &alternative :
           BufferAlternatives(buffers[operand])) {
        const std::string mapped =
            buffersArePostSinglePoint && startsWith(alternative, "local:")
                ? "base:" + alternative.substr(6)
                : mappedBufferIdentity(alternative);
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
        throw std::runtime_error(
            "PlanMemoryInput: access references missing buffer");
      event.accesses.push_back({final->second, effects[operand]});
      if (appendedExtraBuffer && operand + 1 == buffers.size())
        event.temporaryBuffers.insert(final->second);
    }
    if (!event.accesses.empty())
      result.accesses.push_back(std::move(event));
    else
      --functionOrdinal;
  };

  for (const GenericOperation &operation : module.operations) {
    if (afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.bufferized.preBufferizationCSE.erasedOperations.count(
            operation.id) != 0)
      continue;
    const bool tensorSinglePointBufferization =
        operation.name == "tensor.insert" || operation.name == "tensor.extract";
    if (operation.name == "tensor.extract" && !operation.results.empty() &&
        !valueHasUser(operation.results.front()))
      continue;
    if (operation.name == "tensor.extract" && !operation.operands.empty()) {
      const GenericOperation *definition =
          definingOperation(operation.operands.front());
      if (definition &&
          afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.singlePoint.scalarizedOperations.count(definition->id) !=
              0)
        continue;
    }
    if (afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.singlePoint.scalarizedOperations.count(operation.id) != 0 &&
        !tensorSinglePointBufferization) {
      const std::vector<std::string> scalarBuffers =
          mappedOperationBuffers(operation);
      const std::vector<size_t> &destinations =
          metadata.dpsInitOperandIndices(
              operation.name, operation.operands.size(),
              operation.properties);
      for (size_t input = 0; input < operation.operandTypes.size(); ++input) {
        if ((!IsTensorType(operation.operandTypes[input]) &&
             !IsMemRefType(operation.operandTypes[input])) ||
            std::find(destinations.begin(), destinations.end(), input) !=
                destinations.end())
          continue;
        if (input >= scalarBuffers.size() || scalarBuffers[input].empty())
          continue;
        const std::string original =
            originalOperationBuffer(operation.id, input);
        if (SinglePointBufferSurvives(
                original, afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.singlePoint.bufferMapping))
          append(operation, "memref.load", {scalarBuffers[input]});
      }
      for (size_t destination : destinations) {
        if (destination >= scalarBuffers.size() ||
            scalarBuffers[destination].empty())
          continue;
        const std::string original =
            originalOperationBuffer(operation.id, destination);
        if (SinglePointBufferSurvives(
                original, afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.singlePoint.bufferMapping))
          append(operation, "memref.store",
                 {"", scalarBuffers[destination]});
      }
      continue;
    }
    const size_t operationId = static_cast<size_t>(operation.id);
    const size_t noOrdinal = std::numeric_limits<size_t>::max();
    const size_t inlineLoad =
        planningContext.inlineLoadByCopyOperation.at(operationId);
    if (inlineLoad == noOrdinal &&
        afterMarkMultiBuffer.afterInlineLoadCopy.inlineLoadCopy
                .erasedOperations.count(operation.id) != 0)
      continue;
    const size_t rewritten =
        planningContext.operationRewriteBySource.at(operationId);
    if (rewritten != noOrdinal) {
      const OperationRewriteDelta &rewrite =
          postBufferization.operationRewrites.at(rewritten);
      for (size_t generatedOrdinal = 0;
           generatedOrdinal < rewrite.replacement.size();
           ++generatedOrdinal) {
        const GeneratedOperationRewrite &generated =
            rewrite.replacement[generatedOrdinal];
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
    std::vector<std::string> buffers = mappedOperationBuffers(operation);
    for (size_t operand = 0; operand < buffers.size(); ++operand)
      if (!buffers[operand].empty() &&
          postBufferization.singlePoint.canonicalizedIterArgAccesses.count(
              {operation.id, static_cast<int>(operand)}) != 0)
        buffers[operand].clear();
    if (operation.name == "hivm.hir.vconcat") {
      const std::vector<size_t> &destinations =
          metadata.dpsInitOperandIndices(
              operation.name, operation.operands.size(),
              operation.properties);
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
      const std::string destination = valueBuffer(operation.results.front());
      for (size_t element = 0; element < operation.operands.size(); ++element)
        append(operation, "memref.store", {"", destination}, "", false,
               element);
      continue;
    }
    const bool materializedTensorInsert =
        operation.name == "tensor.insert" &&
        tensorInsertMustMaterialize(operation);
    if (materializedTensorInsert && buffers.size() > 1 &&
        !operation.results.empty()) {
      const std::string resultBuffer = valueBuffer(operation.results.front());
      if (!resultBuffer.empty())
        buffers[1] = resultBuffer;
    }
    const std::vector<size_t> &initOperands =
        metadata.dpsInitOperandIndices(operation.name,
                                       operation.operands.size(),
                                       operation.properties);
    for (size_t operand = 0; operand < operation.operandTypes.size(); ++operand) {
      if (!IsTensorType(operation.operandTypes[operand]) ||
          operand >= buffers.size() || buffers[operand].empty() ||
          !isOutOfPlaceOperand(operation.id, operand))
        continue;
      if (startsWith(operation.name, "hivm.hir.v") &&
          std::find(initOperands.begin(), initOperands.end(), operand) !=
              initOperands.end())
        continue;
      const std::string destination =
          mappedBufferIdentity(buffers[operand]);
      if (targetBuffers.count(destination) == 0)
        continue;
      const std::string source = valueBuffer(operation.operands[operand]);
      if (!source.empty())
        append(operation, "hivm.hir.copy", {source, buffers[operand]},
               "", false, 0, true, false, false,
               static_cast<int>(operand));
    }
    if (inlineLoad != noOrdinal) {
      const InlineLoadCopyRewrite &rewrite =
          afterMarkMultiBuffer.afterInlineLoadCopy.inlineLoadCopy.rewrites.at(
              inlineLoad);
      append(operation, "hivm.hir.load",
             {rewrite.loadSource, rewrite.copyDestination});
      continue;
    }
    if (tensorSinglePointBufferization) {
      const size_t bufferOperand = operation.name == "tensor.insert" ? 1 : 0;
      if (bufferOperand >= buffers.size() ||
          (!SinglePointBufferSurvives(
               originalOperationBuffer(operation.id, bufferOperand),
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
      const std::string destination = mappedBufferIdentity(buffers[1]);
      const auto recordOrdinal =
          planningContext.liveBufferBySource.find(destination);
      const LocalBufferRecord *record =
          recordOrdinal == planningContext.liveBufferBySource.end()
              ? nullptr
              : &afterMarkMultiBuffer.afterInlineLoadCopy.buffers.at(
                    recordOrdinal->second);
      if (afterMarkMultiBuffer.options.inferHIVMDataLayout && record &&
          record->addressSpace == AddressSpace::L1)
        name = "hivm.hir.nd2nz";
    }
    append(operation, name, buffers, operation.properties);
  }
  result.planningContext = std::move(planningContext);
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
           << HexEncode(PhysicalTypeSignature(buffer.memrefType)) << '\t'
           << buffer.constBits << '\t'
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
