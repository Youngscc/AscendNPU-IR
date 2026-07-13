#ifndef CVPIPELINE_UB_MODEL_CPP_INLINE_LOAD_COPY_HPP
#define CVPIPELINE_UB_MODEL_CPP_INLINE_LOAD_COPY_HPP

#include "c4_semantic_ir.hpp"

namespace cvub {

enum class C5MemoryEffect : unsigned {
  None = 0,
  Read = 1,
  Write = 2,
};

inline C5MemoryEffect operator|(C5MemoryEffect lhs, C5MemoryEffect rhs) {
  return static_cast<C5MemoryEffect>(static_cast<unsigned>(lhs) |
                                     static_cast<unsigned>(rhs));
}

inline bool C5HasEffect(C5MemoryEffect effects, C5MemoryEffect effect) {
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

inline std::string C5FunctionName(const C1OperationRecord &function) {
  std::string name = FindDictionaryValue(function.properties, "sym_name");
  if (name.empty())
    name = FindDictionaryValue(function.attributes, "sym_name");
  if (name.size() >= 2 && name.front() == '"' && name.back() == '"')
    name = name.substr(1, name.size() - 2);
  return name;
}

inline bool C5IsViewLikeOperation(const std::string &name) {
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

inline bool C5IsCopyOperation(const std::string &name) {
  return name == "hivm.hir.copy" || name == "tensor.insert_slice";
}

inline std::map<size_t, std::string> C5OperationBuffers(
    const C3SemanticIR &c3, int operationId) {
  std::map<size_t, std::string> result;
  for (const BufferizedOperandAccess &access : c3.bufferized.accesses)
    if (access.operationId == operationId)
      result[static_cast<size_t>(access.operandNumber)] =
          C4MappedBufferIdentity(access.bufferId,
                                 c3.singlePoint.bufferMapping);
  return result;
}

inline C5MemoryEffect C5OperandEffect(const C1OperationRecord &operation,
                                      size_t operandNumber) {
  if (C5IsViewLikeOperation(operation.name))
    return C5MemoryEffect::None;
  if (operation.name == "hivm.hir.load" ||
      operation.name == "hivm.hir.copy" ||
      operation.name == "hivm.hir.store" ||
      operation.name == "tensor.insert_slice")
    return operandNumber == 0 ? C5MemoryEffect::Read : C5MemoryEffect::Write;
  if (operation.name == "memref.load")
    return C5MemoryEffect::Read;
  if (operation.name == "memref.store")
    return operandNumber == 0 ? C5MemoryEffect::Read : C5MemoryEffect::Write;
  if (IsDestinationStyleOp(operation.name)) {
    const std::vector<size_t> initIndices = C1DpsInitOperandIndices(
        operation.name, operation.operands.size(), operation.properties);
    return std::find(initIndices.begin(), initIndices.end(), operandNumber) ==
                   initIndices.end()
               ? C5MemoryEffect::Read
               : C5MemoryEffect::Write;
  }
  return C5MemoryEffect::Read | C5MemoryEffect::Write;
}

inline bool C5OperationIsBefore(const C1OperationRecord &lhs,
                                const C1OperationRecord &rhs) {
  return lhs.blockId == rhs.blockId && lhs.ordinal < rhs.ordinal;
}

inline bool C5WriteBeforeCopy(const C1OperationRecord &write,
                              const C1OperationRecord &load,
                              const C1OperationRecord &copy) {
  if (write.id == load.id)
    return false;
  return write.blockId != copy.blockId || C5OperationIsBefore(write, copy);
}

inline bool C5ReadAfterLoad(const C1OperationRecord &read,
                            const C1OperationRecord &load,
                            const C1OperationRecord &copy) {
  if (read.id == copy.id)
    return false;
  return read.blockId != load.blockId || C5OperationIsBefore(load, read);
}

inline bool C5HappensAfterLoadBeforeCopy(const C1OperationRecord &candidate,
                                         const C1OperationRecord &load,
                                         const C1OperationRecord &copy) {
  if (candidate.id == load.id || candidate.id == copy.id)
    return false;
  if (candidate.blockId == load.blockId)
    return C5OperationIsBefore(load, candidate);
  if (candidate.blockId == copy.blockId)
    return C5OperationIsBefore(candidate, copy);
  return true;
}

inline int C5TraceTensorBufferValue(
    int value, const std::map<int, const C1OperationRecord *> &definitions) {
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

inline bool C5HasInterveningMemoryEffect(
    const C3SemanticIR &c3, const std::string &buffer,
    const C1OperationRecord &load, const C1OperationRecord &copy,
    bool copySourceRole) {
  for (const BufferizedOperandAccess &access : c3.bufferized.accesses) {
    const std::string identity = C4MappedBufferIdentity(
        access.bufferId, c3.singlePoint.bufferMapping);
    if (identity != buffer || access.operationId == load.id)
      continue;
    const C1OperationRecord &operation =
        c3.bufferized.logicalModule.operations.at(
            static_cast<size_t>(access.operationId));
    if (C5IsViewLikeOperation(operation.name))
      continue;
    const C5MemoryEffect effects = C5OperandEffect(
        operation, static_cast<size_t>(access.operandNumber));
    if (C5HasEffect(effects, C5MemoryEffect::Write)) {
      if (copySourceRole ? C5WriteBeforeCopy(operation, load, copy)
                         : C5HappensAfterLoadBeforeCopy(operation, load, copy))
        return true;
    }
    if (copySourceRole && C5HasEffect(effects, C5MemoryEffect::Read) &&
        C5ReadAfterLoad(operation, load, copy)) {
      return true;
    }
  }
  return false;
}

inline InlineLoadCopyResult ModelInlineLoadCopy(const C4SemanticIR &c4) {
  const C3SemanticIR &c3 = c4.c3;
  const C1SemanticModule &module = c3.bufferized.logicalModule;
  const std::map<int, const C1OperationRecord *> definitions =
      C1DefiningOperations(module);
  InlineLoadCopyResult result;
  std::set<int> erasedOperations;
  for (const C1OperationRecord &copy : module.operations) {
    if (!C5IsCopyOperation(copy.name) ||
        c3.singlePoint.scalarizedOperations.count(copy.id) != 0 ||
        erasedOperations.count(copy.id) != 0)
      continue;
    const std::map<size_t, std::string> copyBuffers =
        C5OperationBuffers(c3, copy.id);
    auto copySource = copyBuffers.find(0);
    auto copyDestination = copyBuffers.find(1);
    if (copySource == copyBuffers.end() ||
        copyDestination == copyBuffers.end())
      continue;

    const C1OperationRecord *matchedLoad = nullptr;
    for (const C1OperationRecord &load : module.operations) {
      if (load.name != "hivm.hir.load" ||
          erasedOperations.count(load.id) != 0 ||
          load.blockId != copy.blockId ||
          !C5OperationIsBefore(load, copy))
        continue;
      const std::map<size_t, std::string> loadBuffers =
          C5OperationBuffers(c3, load.id);
      auto loadDestination = loadBuffers.find(1);
      if (loadDestination == loadBuffers.end() ||
          loadDestination->second != copySource->second)
        continue;
      if (!copy.operands.empty() && load.operands.size() > 1 &&
          C5TraceTensorBufferValue(copy.operands[0], definitions) !=
              C5TraceTensorBufferValue(load.operands[1], definitions))
        continue;
      matchedLoad = &load;
      break;
    }
    if (!matchedLoad)
      continue;
    const std::map<size_t, std::string> loadBuffers =
        C5OperationBuffers(c3, matchedLoad->id);
    auto loadSource = loadBuffers.find(0);
    if (loadSource == loadBuffers.end() ||
        C5HasInterveningMemoryEffect(c3, copySource->second, *matchedLoad,
                                     copy, true) ||
        C5HasInterveningMemoryEffect(c3, loadSource->second, *matchedLoad,
                                     copy, false))
      continue;
    const C4BufferRecord *middle =
        C4FindSourceBuffer(c4.buffers, copySource->second);
    if (!middle || middle->extraBuffer)
      continue;
    const C1OperationRecord *function = C4EnclosingFunction(module, copy);
    if (!function)
      throw std::runtime_error("InlineLoadCopy: copy outside function");
    result.rewrites.push_back(
        {C5FunctionName(*function), matchedLoad->id, copy.id,
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
