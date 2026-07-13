#ifndef CVPIPELINE_UB_MODEL_CPP_MARK_MULTI_BUFFER_HPP
#define CVPIPELINE_UB_MODEL_CPP_MARK_MULTI_BUFFER_HPP

#include "after_inline_load_copy.hpp"

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

inline bool HasProperParentLoop(const AfterInlineLoadCopyState &afterInlineLoadCopy,
                                  const LocalBufferRecord &buffer) {
  const GenericModule &module = afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.bufferized.logicalModule;
  const int ownerId = BufferOwnerOperation(afterInlineLoadCopy.afterAllocExtraBuffer, buffer);
  if (ownerId < 0 || static_cast<size_t>(ownerId) >= module.operations.size())
    return false;
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

inline std::vector<std::pair<std::string, std::string>>
FinalOperationBuffers(const AfterInlineLoadCopyState &afterInlineLoadCopy,
                        const GenericOperation &operation) {
  for (const InlineLoadCopyRewrite &rewrite : afterInlineLoadCopy.inlineLoadCopy.rewrites) {
    if (rewrite.loadOperation == operation.id)
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
  const GenericModule &module = afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.bufferized.logicalModule;
  auto mark = [&](const GenericOperation &function,
                  const LocalBufferRecord &buffer, unsigned count, bool preload,
                  const GenericOperation &trigger) {
    if (result.buffer2MultiNum.count(buffer.sourceIdentity) != 0)
      return false;
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
      continue;
    const LocalBufferRecord *buffer = FindLocalBuffer(afterInlineLoadCopy, operand->second);
    const GenericOperation *function = EnclosingFunction(module, operation);
    if (!buffer || !function)
      continue;
    std::string preload = FindDictionaryValue(
        operation.attributes, "hivm.preload_local_buffer");
    if (preload.empty())
      preload = FindDictionaryValue(operation.properties,
                                    "hivm.preload_local_buffer");
    const size_t colon = count.find(':');
    const unsigned multiBufferNum = static_cast<unsigned>(
        std::stoul(trim(count.substr(0, colon))));
    const bool isPreload = !preload.empty() && std::stoll(preload) != 0;
    mark(*function, *buffer, multiBufferNum, isPreload, operation);
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
      if (preload.empty() || std::stoll(preload) == 0 || core == "CUBE" ||
          operation.regions.empty())
        continue;
      const GenericRegion &region =
          module.regions.at(static_cast<size_t>(operation.regions.front()));
      if (region.blocks.empty())
        continue;
      const GenericBlock &block =
          module.blocks.at(static_cast<size_t>(region.blocks.front()));
      if (block.operations.empty())
        continue;
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
          if (buffer && buffer->addressSpace != AddressSpace::GM)
            mark(*function, *buffer, 4, true, operation);
        }
      }
      continue;
    }

    const std::vector<std::pair<std::string, std::string>> operands =
        FinalOperationBuffers(afterInlineLoadCopy, operation);
    const LocalBufferRecord *source = nullptr;
    const LocalBufferRecord *destination = nullptr;
    for (const auto &[identity, role] : operands) {
      const LocalBufferRecord *buffer = FindLocalBuffer(afterInlineLoadCopy, identity);
      if (role == "source" && !source)
        source = buffer;
      if (role == "destination" && !destination)
        destination = buffer;
    }
    std::string finalName = operation.name;
    // InferHIVMDataLayout folds an ND-to-NZ load into ND2NZ when the target
    // buffer has been inferred as L1. MarkMultiBuffer observes that folded op.
    if (finalName == "hivm.hir.load" && destination &&
        destination->addressSpace == AddressSpace::L1)
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

    const LocalBufferRecord *candidate =
        source && source->addressSpace != AddressSpace::GM ? source
        : destination && destination->addressSpace != AddressSpace::GM
            ? destination
            : nullptr;
    if (candidate && HasProperParentLoop(afterInlineLoadCopy, *candidate))
      mark(*function, *candidate, 2, false, operation);
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
