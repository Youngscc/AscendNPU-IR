#ifndef CVPIPELINE_UB_MODEL_CPP_MARK_MULTI_BUFFER_HPP
#define CVPIPELINE_UB_MODEL_CPP_MARK_MULTI_BUFFER_HPP

#include "../pipeline/after_inline_load_copy.hpp"

namespace cvub {

enum class MultiBufferStrategy {
  NoLimit,
  OnlyCube,
  OnlyVector,
  CubeNoL0C,
};

struct MarkMultiBufferOptions {
  bool enableAuto = false;
  bool limitAutoMultiBufferOnlyForLocalBuffer = true;
  MultiBufferStrategy limitAutoMultiBufferOfLocalBuffer =
      MultiBufferStrategy::CubeNoL0C;
  MultiBufferStrategy limitMixAutoMultiBufferBuffer =
      MultiBufferStrategy::OnlyCube;
  bool inferHIVMDataLayout = true;
};

struct MultiBufferMark {
  std::string functionName;
  std::string sourceIdentity;
  unsigned multiBufferNum = 1;
  bool preloadLocalBuffer = false;
  int triggerOperation = -1;
  std::string triggerName;
};

struct MarkMultiBufferResult {
  std::vector<MultiBufferMark> marks;
  std::map<std::string, unsigned> buffer2MultiNum;
  std::set<std::string> preloadLocalBuffers;
};

[[noreturn]] inline void
MarkMultiBufferExactBlocker(const GenericOperation &operation,
                            const std::string &reason) {
  throw std::runtime_error("MarkMultiBuffer exact blocker at operation " +
                           std::to_string(operation.id) + " (" +
                           operation.name + "): " + reason);
}

inline MultiBufferStrategy ParseMultiBufferStrategy(const std::string &value) {
  if (value == "only-cube")
    return MultiBufferStrategy::OnlyCube;
  if (value == "only-vector")
    return MultiBufferStrategy::OnlyVector;
  if (value == "no-l0c")
    return MultiBufferStrategy::CubeNoL0C;
  return MultiBufferStrategy::NoLimit;
}

inline bool IsHostFunction(const GenericOperation &function) {
  return DecomposeEnumValue(
             FindDictionaryValue(function.attributes, "hacc.function_kind")) ==
         "HOST";
}

inline bool IsMixFunction(const GenericOperation &function) {
  const std::string core = DecomposeEnumValue(
      FindDictionaryValue(function.attributes, "hivm.func_core_type"));
  return core == "MIX" ||
         HasDictionaryEntry(function.attributes, "hivm.part_of_mix");
}

inline bool IsForLoop(const GenericOperation &operation) {
  return operation.name == "scf.for";
}

inline const GenericOperation *ParentLoop(
    const GenericModule &module, const GenericOperation &operation) {
  const GenericOperation *current = &operation;
  while (current->parentId >= 0) {
    current = &module.operations.at(static_cast<size_t>(current->parentId));
    if (current->name != "scf.for" && current->name != "scf.while" &&
        current->name != "affine.for" && current->name != "scf.forall")
      continue;
    // MapForToForall followed by HIVMMapForallToBlocks consumes this loop, so
    // it is no longer a parent LoopLikeOpInterface at MarkMultiBuffer.
    if (HasDictionaryEntry(current->properties, "map_for_to_forall") ||
        HasDictionaryEntry(current->attributes, "map_for_to_forall"))
      continue;
    else
      return current;
  }
  return nullptr;
}

inline int BufferOwnerOperation(const PostBufferizationRewriteState &postBufferization,
                                  const LocalBufferRecord &buffer) {
  if (startsWith(buffer.sourceIdentity, "base:")) {
    const size_t ordinal = static_cast<size_t>(
        std::stoull(buffer.sourceIdentity.substr(std::string("base:").size())));
    if (ordinal >= postBufferization.singlePoint.allocations.size())
      return -1;
    return AllocationSourceOperation(
        postBufferization.singlePoint.allocations[ordinal].source);
  }
  if (startsWith(buffer.sourceIdentity, "decompose:")) {
    const size_t begin = std::string("decompose:").size();
    const size_t end = buffer.sourceIdentity.find(':', begin);
    return end == std::string::npos
               ? -1
               : std::stoi(buffer.sourceIdentity.substr(begin, end - begin));
  }
  if (startsWith(buffer.sourceIdentity, "extra-source:")) {
    for (const GenericOperation &operation :
         postBufferization.bufferized.logicalModule.operations)
      if (operation.name == buffer.ownerName)
        return operation.id;
  }
  return -1;
}

inline int BufferOwnerOperation(const AfterAllocExtraBufferState &afterAllocExtraBuffer,
                                  const LocalBufferRecord &buffer) {
  auto found = afterAllocExtraBuffer.bufferOwnerOperations.find(buffer.sourceIdentity);
  return found == afterAllocExtraBuffer.bufferOwnerOperations.end()
             ? BufferOwnerOperation(afterAllocExtraBuffer.postBufferization, buffer)
             : found->second;
}

inline const LocalBufferRecord *FindLocalBuffer(
    const AfterInlineLoadCopyState &afterInlineLoadCopy, const std::string &identity) {
  return FindSourceBuffer(afterInlineLoadCopy.buffers, identity);
}

inline std::optional<AddressSpace> MarkMultiBufferAddressSpace(
    const AfterInlineLoadCopyState &afterInlineLoadCopy,
    const std::string &identity) {
  if (const LocalBufferRecord *buffer =
          FindLocalBuffer(afterInlineLoadCopy, identity))
    return buffer->addressSpace;
  // InferHIVMMemScopePass::inferAndPropagateMemScopeForFunc assigns GM to
  // every memref function argument before MarkMultiBuffer runs.
  if (startsWith(identity, "arg:") || startsWith(identity, "workspace:"))
    return AddressSpace::GM;
  const PostBufferizationRewriteState &postBufferization =
      afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization;
  for (const BufferizedValueBinding &binding :
       postBufferization.bufferized.values) {
    if (MappedBufferIdentity(binding.bufferId,
                             postBufferization.singlePoint.bufferMapping) !=
        identity)
      continue;
    const std::string type = IsTensorType(binding.type)
                                 ? ConvertTensorToMemRefType(binding.type)
                                 : binding.type;
    const std::optional<MemRefTypeModel> memref = ParseMemRefType(type);
    if (memref)
      return memref->addressSpace;
  }
  return std::nullopt;
}

inline std::optional<bool>
HasProperParentLoop(const AfterInlineLoadCopyState &afterInlineLoadCopy,
                    const LocalBufferRecord &buffer) {
  const GenericModule &module = afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.bufferized.logicalModule;
  const int ownerId = BufferOwnerOperation(afterInlineLoadCopy.afterAllocExtraBuffer, buffer);
  if (ownerId < 0 || static_cast<size_t>(ownerId) >= module.operations.size())
    return std::nullopt;
  const GenericOperation *loop =
      ParentLoop(module, module.operations.at(static_cast<size_t>(ownerId)));
  if (!loop)
    return false;
  while (loop) {
    if (!IsForLoop(*loop))
      return false;
    loop = ParentLoop(module, *loop);
  }
  return true;
}

inline bool MarkMultiBufferOperationWasErased(
    const AfterInlineLoadCopyState &afterInlineLoadCopy, int operationId) {
  if (afterInlineLoadCopy.inlineLoadCopy.erasedOperations.count(operationId) !=
      0)
    return true;
  return afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.bufferized
             .preBufferizationCSE.erasedOperations.count(operationId) != 0;
}

inline std::optional<std::string> MarkMultiBufferReplacementName(
    const AfterInlineLoadCopyState &afterInlineLoadCopy, int operationId) {
  for (const InlineLoadCopyRewrite &rewrite :
       afterInlineLoadCopy.inlineLoadCopy.rewrites) {
    if (rewrite.loadOperation == operationId)
      return std::string{};
    if (rewrite.copyOperation == operationId)
      return std::string{"hivm.hir.load"};
  }
  for (const OperationRewriteDelta &rewrite :
       afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization
           .operationRewrites) {
    if (rewrite.sourceOperation != operationId)
      continue;
    for (const GeneratedOperationRewrite &generated : rewrite.replacement)
      if (generated.name == "hivm.hir.load" ||
          generated.name == "hivm.hir.store" ||
          generated.name == "hivm.hir.nd2nz" ||
          generated.name == "hivm.hir.fixpipe")
        return generated.name;
    return std::string{};
  }
  return std::nullopt;
}

inline std::vector<std::pair<std::string, std::string>>
FinalOperationBuffers(const AfterInlineLoadCopyState &afterInlineLoadCopy,
                        const GenericOperation &operation) {
  for (const InlineLoadCopyRewrite &rewrite : afterInlineLoadCopy.inlineLoadCopy.rewrites) {
    if (rewrite.copyOperation == operation.id)
      return {{rewrite.loadSource, "source"},
              {rewrite.copyDestination, "destination"}};
  }
  if (afterInlineLoadCopy.inlineLoadCopy.erasedOperations.count(operation.id) != 0)
    return {};
  const PostBufferizationRewriteState &postBufferization = afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization;
  for (const OperationRewriteDelta &rewrite : postBufferization.operationRewrites) {
    if (rewrite.sourceOperation != operation.id)
      continue;
    std::vector<std::pair<std::string, std::string>> result;
    for (const GeneratedOperationRewrite &generated : rewrite.replacement) {
      if (generated.name != "hivm.hir.load" &&
          generated.name != "hivm.hir.store" &&
          generated.name != "hivm.hir.nd2nz" &&
          generated.name != "hivm.hir.fixpipe")
        continue;
      for (size_t index = 0; index < generated.buffers.size(); ++index)
        result.push_back({MappedBufferIdentity(
                              generated.buffers[index],
                              postBufferization.singlePoint.bufferMapping),
                          index == 0 ? "source" : "destination"});
    }
    return result;
  }
  const std::map<size_t, std::string> buffers =
      OperationBufferMap(postBufferization, operation.id);
  std::vector<size_t> inits = DpsInitOperandIndices(
      operation.name, operation.operands.size(), operation.properties);
  std::vector<size_t> inputs;
  for (size_t index = 0; index < operation.operands.size(); ++index)
    if (std::find(inits.begin(), inits.end(), index) == inits.end() &&
        buffers.count(index) != 0)
      inputs.push_back(index);
  std::vector<std::pair<std::string, std::string>> result;
  if (!inputs.empty()) {
    auto found = buffers.find(inputs.front());
    if (found != buffers.end())
      result.push_back({found->second, "source"});
  }
  if (!inits.empty()) {
    auto found = buffers.find(inits.front());
    if (found != buffers.end())
      result.push_back({found->second, "destination"});
  }
  return result;
}

inline MarkMultiBufferResult ModelMarkMultiBuffer(
    const AfterInlineLoadCopyState &afterInlineLoadCopy, const MarkMultiBufferOptions &options) {
  MarkMultiBufferResult result;
  std::set<std::string> markedBuffers;
  const GenericModule &module = afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.bufferized.logicalModule;
  auto mark = [&](const GenericOperation &function,
                  const LocalBufferRecord &buffer, unsigned count, bool preload,
                  const GenericOperation &trigger) {
    if (markedBuffers.count(buffer.sourceIdentity) != 0)
      return false;
    markedBuffers.insert(buffer.sourceIdentity);
    result.buffer2MultiNum[buffer.sourceIdentity] = count;
    if (preload)
      result.preloadLocalBuffers.insert(buffer.sourceIdentity);
    result.marks.push_back({GenericFunctionName(function), buffer.sourceIdentity,
                            count, preload, trigger.id, trigger.name});
    return true;
  };

  for (const GenericOperation &operation : module.operations) {
    if (operation.name != "annotation.mark")
      continue;
    std::string count =
        FindDictionaryValue(operation.attributes, "hivm.multi_buffer");
    if (count.empty())
      count = FindDictionaryValue(operation.properties, "hivm.multi_buffer");
    if (count.empty())
      continue;
    const std::map<size_t, std::string> buffers =
        OperationBufferMap(afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization, operation.id);
    auto operand = buffers.find(0);
    if (operand == buffers.end())
      MarkMultiBufferExactBlocker(
          operation, "explicit multi-buffer mark has no modeled buffer");
    const LocalBufferRecord *buffer = FindLocalBuffer(afterInlineLoadCopy, operand->second);
    const GenericOperation *function = EnclosingFunction(module, operation);
    if (!buffer) {
      const std::optional<AddressSpace> addressSpace =
          MarkMultiBufferAddressSpace(afterInlineLoadCopy, operand->second);
      if (!addressSpace || *addressSpace == AddressSpace::Unknown ||
          *addressSpace == AddressSpace::UB)
        MarkMultiBufferExactBlocker(
            operation, "explicit multi-buffer mark buffer is unresolved: " +
                           operand->second);
      // Non-UB arguments/workspaces do not participate in local UB planning.
      continue;
    }
    if (!function)
      MarkMultiBufferExactBlocker(
          operation, "explicit multi-buffer mark has no enclosing function");
    std::string preload = FindDictionaryValue(
        operation.attributes, "hivm.preload_local_buffer");
    if (preload.empty())
      preload = FindDictionaryValue(operation.properties,
                                    "hivm.preload_local_buffer");
    const size_t colon = count.find(':');
    const unsigned multiBufferNum = static_cast<unsigned>(
        std::stoul(trim(count.substr(0, colon))));
    if (multiBufferNum < 1)
      MarkMultiBufferExactBlocker(
          operation, "hivm.multi_buffer must be at least one");
    const bool isPreload = !preload.empty() && std::stoll(preload) != 0;
    // MarkMultiBuffer::isMarked treats any existing multi-buffer annotation as
    // marked. PlanMemory consumes non-one counts in walk order, while count 1
    // is a no-op in UpdateMultiBufferInfo. Preload annotations are cumulative.
    markedBuffers.insert(buffer->sourceIdentity);
    if (multiBufferNum != 1)
      result.buffer2MultiNum[buffer->sourceIdentity] = multiBufferNum;
    if (isPreload)
      result.preloadLocalBuffers.insert(buffer->sourceIdentity);
    result.marks.push_back({GenericFunctionName(*function),
                            buffer->sourceIdentity, multiBufferNum, isPreload,
                            operation.id, operation.name});
  }

  if (!options.enableAuto)
    return result;

  for (const GenericOperation &operation : module.operations) {
    const GenericOperation *function = EnclosingFunction(module, operation);
    if (!function || IsHostFunction(*function))
      continue;

    if (operation.name == "scope.scope") {
      std::string preload =
          FindDictionaryValue(operation.attributes, "hivm.preload_num");
      if (preload.empty())
        preload = FindDictionaryValue(operation.properties, "hivm.preload_num");
      const std::string core = DecomposeEnumValue(FindDictionaryValue(
          operation.attributes, "hivm.loop_core_type"));
      if (preload.empty() || std::stoll(preload) == 0 || core == "CUBE")
        continue;
      if (operation.regions.empty())
        MarkMultiBufferExactBlocker(
            operation, "preload scope has no modeled region");
      const GenericRegion &region =
          module.regions.at(static_cast<size_t>(operation.regions.front()));
      if (region.blocks.empty())
        MarkMultiBufferExactBlocker(
            operation, "preload scope has no modeled entry block");
      const GenericBlock &block =
          module.blocks.at(static_cast<size_t>(region.blocks.front()));
      if (block.operations.empty())
        MarkMultiBufferExactBlocker(
            operation, "preload scope has no modeled terminator");
      const GenericOperation &terminator = module.operations.at(
          static_cast<size_t>(block.operations.back()));
      const std::map<int, const GenericOperation *> definitions =
          DefiningOperations(module);
      for (size_t index = 0;
           index < terminator.operands.size() && index < operation.results.size();
           ++index) {
        auto returned = definitions.find(terminator.operands[index]);
        if (returned == definitions.end() ||
            returned->second->name != "memref.alloc")
          continue;
        bool usedByV1 = false;
        for (const GenericOperation &user : module.operations) {
          if (std::find(user.operands.begin(), user.operands.end(),
                        operation.results[index]) == user.operands.end())
            continue;
          if (user.name == "scope.scope" ||
              (user.parentId >= 0 &&
               module.operations.at(static_cast<size_t>(user.parentId)).name ==
                   "scope.scope"))
            usedByV1 = true;
        }
        if (!usedByV1)
          continue;
        for (const BufferizedValueBinding &binding :
             afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.bufferized.values) {
          if (binding.valueId != terminator.operands[index])
            continue;
          const LocalBufferRecord *buffer = FindLocalBuffer(
              afterInlineLoadCopy, MappedBufferIdentity(
                      binding.bufferId,
                      afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.singlePoint.bufferMapping));
          if (!buffer) {
            const std::string identity = MappedBufferIdentity(
                binding.bufferId,
                afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization
                    .singlePoint.bufferMapping);
            const std::optional<AddressSpace> addressSpace =
                MarkMultiBufferAddressSpace(afterInlineLoadCopy, identity);
            if (!addressSpace || *addressSpace == AddressSpace::Unknown ||
                *addressSpace == AddressSpace::UB)
              MarkMultiBufferExactBlocker(
                  operation, "preload scope result buffer is unresolved");
            continue;
          }
          if (buffer->addressSpace == AddressSpace::Unknown)
            MarkMultiBufferExactBlocker(
                operation, "preload scope result address space is unresolved");
          if (buffer->addressSpace != AddressSpace::GM)
            mark(*function, *buffer, 4, true, operation);
        }
      }
      continue;
    }

    const std::optional<std::string> replacementName =
        MarkMultiBufferReplacementName(afterInlineLoadCopy, operation.id);
    if (replacementName && replacementName->empty())
      continue;
    std::string finalName = replacementName ? *replacementName : operation.name;
    if (finalName != "hivm.hir.nd2nz" && finalName != "hivm.hir.fixpipe" &&
        finalName != "hivm.hir.load" && finalName != "hivm.hir.store")
      continue;

    const std::vector<std::pair<std::string, std::string>> operands =
        FinalOperationBuffers(afterInlineLoadCopy, operation);
    const LocalBufferRecord *source = nullptr;
    const LocalBufferRecord *destination = nullptr;
    AddressSpace sourceAddressSpace = AddressSpace::Unknown;
    AddressSpace destinationAddressSpace = AddressSpace::Unknown;
    bool hasOperandWithoutMemorySpace = false;
    for (const auto &[identity, role] : operands) {
      const LocalBufferRecord *buffer = FindLocalBuffer(afterInlineLoadCopy, identity);
      const std::optional<AddressSpace> addressSpace =
          MarkMultiBufferAddressSpace(afterInlineLoadCopy, identity);
      if (!addressSpace)
        MarkMultiBufferExactBlocker(
            operation, "buffer operand address space is unresolved: " + identity);
      // MarkMultiBuffer::matchAndRewrite returns failure when either memref has
      // no memory-space attribute. AddressSpace::Unknown represents that
      // modeled no-match; a missing binding above remains an exact blocker.
      if (*addressSpace == AddressSpace::Unknown)
        hasOperandWithoutMemorySpace = true;
      if (role == "source" && sourceAddressSpace == AddressSpace::Unknown) {
        source = buffer;
        sourceAddressSpace = *addressSpace;
      }
      if (role == "destination" &&
          destinationAddressSpace == AddressSpace::Unknown) {
        destination = buffer;
        destinationAddressSpace = *addressSpace;
      }
    }
    if (hasOperandWithoutMemorySpace)
      continue;
    // InferHIVMDataLayout folds an ND-to-NZ load into ND2NZ when the target
    // buffer has been inferred as L1. MarkMultiBuffer observes that folded op.
    if (options.inferHIVMDataLayout && finalName == "hivm.hir.load" &&
        destinationAddressSpace == AddressSpace::L1)
      finalName = "hivm.hir.nd2nz";

    const bool mix = IsMixFunction(*function);
    bool enabledPattern = false;
    if (finalName == "hivm.hir.nd2nz" ||
        finalName == "hivm.hir.fixpipe") {
      enabledPattern = !mix ||
                       options.limitMixAutoMultiBufferBuffer !=
                           MultiBufferStrategy::OnlyVector;
      if (finalName == "hivm.hir.fixpipe" &&
          options.limitAutoMultiBufferOfLocalBuffer ==
              MultiBufferStrategy::CubeNoL0C)
        enabledPattern = false;
    } else if (finalName == "hivm.hir.load" ||
               finalName == "hivm.hir.store") {
      enabledPattern = !mix ||
                       options.limitMixAutoMultiBufferBuffer !=
                           MultiBufferStrategy::OnlyCube;
    }
    if (!enabledPattern)
      continue;

    if (operands.empty() &&
        !MarkMultiBufferOperationWasErased(afterInlineLoadCopy, operation.id))
      MarkMultiBufferExactBlocker(
          operation, "enabled auto multi-buffer pattern has no modeled "
                     "source/destination buffers");

    const LocalBufferRecord *candidate =
        source && source->addressSpace != AddressSpace::GM ? source
        : destination && destination->addressSpace != AddressSpace::GM
            ? destination
            : nullptr;
    if (candidate) {
      const std::optional<bool> properParentLoop =
          HasProperParentLoop(afterInlineLoadCopy, *candidate);
      if (!properParentLoop)
        MarkMultiBufferExactBlocker(
            operation, "candidate allocation owner is unresolved: " +
                           candidate->sourceIdentity);
      if (*properParentLoop)
        mark(*function, *candidate, 2, false, operation);
    }
  }
  return result;
}

inline std::string SerializeMarkMultiBufferResult(
    const MarkMultiBufferResult &result) {
  std::ostringstream output;
  output << "MARK_MULTI_BUFFER\t1\n";
  for (const MultiBufferMark &mark : result.marks)
    output << "MARK\t" << HexEncode(mark.functionName) << '\t'
           << HexEncode(mark.sourceIdentity) << '\t' << mark.multiBufferNum
           << '\t' << (mark.preloadLocalBuffer ? 1 : 0) << '\t'
           << mark.triggerOperation << '\t' << HexEncode(mark.triggerName)
           << '\n';
  return output.str();
}

} // namespace cvub

#endif
