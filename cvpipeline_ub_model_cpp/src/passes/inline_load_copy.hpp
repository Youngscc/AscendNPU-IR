#ifndef CVPIPELINE_UB_MODEL_CPP_INLINE_LOAD_COPY_HPP
#define CVPIPELINE_UB_MODEL_CPP_INLINE_LOAD_COPY_HPP

#include "../pipeline/after_alloc_extra_buffer.hpp"

namespace cvub {

enum class InlineLoadCopyMemoryEffect : unsigned {
  None = 0,
  Read = 1,
  Write = 2,
};

inline InlineLoadCopyMemoryEffect operator|(InlineLoadCopyMemoryEffect lhs, InlineLoadCopyMemoryEffect rhs) {
  return static_cast<InlineLoadCopyMemoryEffect>(static_cast<unsigned>(lhs) |
                                     static_cast<unsigned>(rhs));
}

inline bool HasMemoryEffect(InlineLoadCopyMemoryEffect effects, InlineLoadCopyMemoryEffect effect) {
  return (static_cast<unsigned>(effects) & static_cast<unsigned>(effect)) != 0;
}

struct InlineLoadCopyRewrite {
  std::string functionName;
  int loadOperation = -1;
  int copyOperation = -1;
  std::string loadSource;
  std::string middleBuffer;
  std::string copyDestination;
};

struct InlineLoadCopyResult {
  std::vector<InlineLoadCopyRewrite> rewrites;
  std::set<std::string> erasedBuffers;
  std::set<int> erasedOperations;
};

inline std::string GenericFunctionName(const GenericOperation &function) {
  std::string name = FindDictionaryValue(function.properties, "sym_name");
  if (name.empty())
    name = FindDictionaryValue(function.attributes, "sym_name");
  if (name.size() >= 2 && name.front() == '"' && name.back() == '"')
    name = name.substr(1, name.size() - 2);
  return name;
}

inline bool IsViewLikeOperation(const std::string &name) {
  static const std::set<std::string> operations = {
      "bufferization.to_memref",
      "bufferization.to_tensor",
      "hivm.hir.bitcast",
      "memref.cast",
      "memref.collapse_shape",
      "memref.expand_shape",
      "memref.extract_strided_metadata",
      "memref.reinterpret_cast",
      "memref.subview",
      "memref.view",
      "tensor.cast",
      "tensor.collapse_shape",
      "tensor.expand_shape",
      "tensor.extract_slice",
  };
  return operations.count(name) != 0;
}

inline bool IsCopyOperation(const std::string &name) {
  return name == "hivm.hir.copy" || name == "tensor.insert_slice";
}

inline std::map<size_t, std::string> OperationBufferMap(
    const PostBufferizationRewriteState &postBufferization, int operationId) {
  std::map<size_t, std::string> result;
  for (const BufferizedOperandAccess &access : postBufferization.bufferized.accesses)
    if (access.operationId == operationId)
      result[static_cast<size_t>(access.operandNumber)] =
          MappedBufferIdentity(access.bufferId,
                                 postBufferization.singlePoint.bufferMapping);
  return result;
}

inline InlineLoadCopyMemoryEffect OperandMemoryEffect(const GenericOperation &operation,
                                      size_t operandNumber) {
  if (IsViewLikeOperation(operation.name))
    return InlineLoadCopyMemoryEffect::None;
  if (operation.name == "hivm.hir.load" ||
      operation.name == "hivm.hir.copy" ||
      operation.name == "hivm.hir.store" ||
      operation.name == "tensor.insert_slice")
    return operandNumber == 0 ? InlineLoadCopyMemoryEffect::Read : InlineLoadCopyMemoryEffect::Write;
  if (operation.name == "memref.load")
    return InlineLoadCopyMemoryEffect::Read;
  if (operation.name == "memref.store")
    return operandNumber == 0 ? InlineLoadCopyMemoryEffect::Read : InlineLoadCopyMemoryEffect::Write;
  if (IsDestinationStyleOp(operation.name)) {
    const std::vector<size_t> initIndices = DpsInitOperandIndices(
        operation.name, operation.operands.size(), operation.properties);
    return std::find(initIndices.begin(), initIndices.end(), operandNumber) ==
                   initIndices.end()
               ? InlineLoadCopyMemoryEffect::Read
               : InlineLoadCopyMemoryEffect::Write;
  }
  return InlineLoadCopyMemoryEffect::Read | InlineLoadCopyMemoryEffect::Write;
}

inline bool OperationIsBefore(const GenericOperation &lhs,
                                const GenericOperation &rhs) {
  return lhs.blockId == rhs.blockId && lhs.ordinal < rhs.ordinal;
}

inline bool WriteBeforeCopy(const GenericOperation &write,
                              const GenericOperation &load,
                              const GenericOperation &copy) {
  if (write.id == load.id)
    return false;
  return write.blockId != copy.blockId || OperationIsBefore(write, copy);
}

inline bool ReadAfterLoad(const GenericOperation &read,
                            const GenericOperation &load,
                            const GenericOperation &copy) {
  if (read.id == copy.id)
    return false;
  return read.blockId != load.blockId || OperationIsBefore(load, read);
}

inline bool HappensAfterLoadBeforeCopy(const GenericOperation &candidate,
                                         const GenericOperation &load,
                                         const GenericOperation &copy) {
  if (candidate.id == load.id || candidate.id == copy.id)
    return false;
  if (candidate.blockId == load.blockId)
    return OperationIsBefore(load, candidate);
  if (candidate.blockId == copy.blockId)
    return OperationIsBefore(candidate, copy);
  return true;
}

inline int TraceTensorBufferValue(
    int value, const std::map<int, const GenericOperation *> &definitions) {
  std::set<int> visited;
  while (visited.insert(value).second) {
    auto found = definitions.find(value);
    if (found == definitions.end() || found->second->operands.empty())
      return value;
    const std::string &name = found->second->name;
    if (name != "bufferization.to_tensor" && name != "tensor.cast" &&
        name != "tensor.collapse_shape" && name != "tensor.expand_shape" &&
        name != "tensor.extract_slice")
      return value;
    value = found->second->operands.front();
  }
  return value;
}

inline bool HasInterveningMemoryEffect(
    const PostBufferizationRewriteState &postBufferization, const std::string &buffer,
    const GenericOperation &load, const GenericOperation &copy,
    bool copySourceRole) {
  for (const BufferizedOperandAccess &access : postBufferization.bufferized.accesses) {
    const std::string identity = MappedBufferIdentity(
        access.bufferId, postBufferization.singlePoint.bufferMapping);
    if (identity != buffer || access.operationId == load.id)
      continue;
    const GenericOperation &operation =
        postBufferization.bufferized.logicalModule.operations.at(
            static_cast<size_t>(access.operationId));
    if (IsViewLikeOperation(operation.name))
      continue;
    const InlineLoadCopyMemoryEffect effects = OperandMemoryEffect(
        operation, static_cast<size_t>(access.operandNumber));
    if (HasMemoryEffect(effects, InlineLoadCopyMemoryEffect::Write)) {
      if (copySourceRole ? WriteBeforeCopy(operation, load, copy)
                         : HappensAfterLoadBeforeCopy(operation, load, copy))
        return true;
    }
    if (copySourceRole && HasMemoryEffect(effects, InlineLoadCopyMemoryEffect::Read) &&
        ReadAfterLoad(operation, load, copy)) {
      return true;
    }
  }
  return false;
}

inline InlineLoadCopyResult ModelInlineLoadCopy(const AfterAllocExtraBufferState &afterAllocExtraBuffer) {
  const PostBufferizationRewriteState &postBufferization = afterAllocExtraBuffer.postBufferization;
  const GenericModule &module = postBufferization.bufferized.logicalModule;
  const std::map<int, const GenericOperation *> definitions =
      DefiningOperations(module);
  InlineLoadCopyResult result;
  std::set<int> erasedOperations;
  for (const GenericOperation &copy : module.operations) {
    if (!IsCopyOperation(copy.name) ||
        postBufferization.singlePoint.scalarizedOperations.count(copy.id) != 0 ||
        erasedOperations.count(copy.id) != 0)
      continue;
    const std::map<size_t, std::string> copyBuffers =
        OperationBufferMap(postBufferization, copy.id);
    auto copySource = copyBuffers.find(0);
    auto copyDestination = copyBuffers.find(1);
    if (copySource == copyBuffers.end() ||
        copyDestination == copyBuffers.end())
      continue;

    const GenericOperation *matchedLoad = nullptr;
    for (const GenericOperation &load : module.operations) {
      if (load.name != "hivm.hir.load" ||
          erasedOperations.count(load.id) != 0 ||
          load.blockId != copy.blockId ||
          !OperationIsBefore(load, copy))
        continue;
      const std::map<size_t, std::string> loadBuffers =
          OperationBufferMap(postBufferization, load.id);
      auto loadDestination = loadBuffers.find(1);
      if (loadDestination == loadBuffers.end() ||
          loadDestination->second != copySource->second)
        continue;
      if (!copy.operands.empty() && load.operands.size() > 1 &&
          TraceTensorBufferValue(copy.operands[0], definitions) !=
              TraceTensorBufferValue(load.operands[1], definitions))
        continue;
      matchedLoad = &load;
      break;
    }
    if (!matchedLoad)
      continue;
    const std::map<size_t, std::string> loadBuffers =
        OperationBufferMap(postBufferization, matchedLoad->id);
    auto loadSource = loadBuffers.find(0);
    if (loadSource == loadBuffers.end() ||
        HasInterveningMemoryEffect(postBufferization, copySource->second, *matchedLoad,
                                     copy, true) ||
        HasInterveningMemoryEffect(postBufferization, loadSource->second, *matchedLoad,
                                     copy, false))
      continue;
    const LocalBufferRecord *middle =
        FindSourceBuffer(afterAllocExtraBuffer.buffers, copySource->second);
    if (!middle || middle->extraBuffer)
      continue;
    const GenericOperation *function = EnclosingFunction(module, copy);
    if (!function)
      throw std::runtime_error("InlineLoadCopy: copy outside function");
    result.rewrites.push_back(
        {GenericFunctionName(*function), matchedLoad->id, copy.id,
         loadSource->second, copySource->second, copyDestination->second});
    result.erasedBuffers.insert(middle->sourceIdentity);
    erasedOperations.insert(matchedLoad->id);
    erasedOperations.insert(copy.id);
  }
  result.erasedOperations = std::move(erasedOperations);
  return result;
}

inline std::string
SerializeInlineLoadCopyResult(const InlineLoadCopyResult &result) {
  std::ostringstream output;
  output << "INLINE_LOAD_COPY\t1\n";
  for (const InlineLoadCopyRewrite &rewrite : result.rewrites)
    output << "INLINE\t" << HexEncode(rewrite.functionName) << '\t'
           << rewrite.loadOperation << '\t' << rewrite.copyOperation << '\t'
           << HexEncode(rewrite.loadSource) << '\t'
           << HexEncode(rewrite.middleBuffer) << '\t'
           << HexEncode(rewrite.copyDestination) << '\n';
  return output.str();
}

} // namespace cvub

#endif
