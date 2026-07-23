#ifndef CVPIPELINE_UB_MODEL_CPP_INFER_HIVM_MEM_SCOPE_HPP
#define CVPIPELINE_UB_MODEL_CPP_INFER_HIVM_MEM_SCOPE_HPP

#include "post_bufferization_rewrites.hpp"

namespace cvub {

inline const GenericOperation *EnclosingFunction(
    const GenericModule &module, const GenericOperation &operation) {
  const GenericOperation *current = &operation;
  while (current && current->name != "func.func") {
    if (current->parentId < 0)
      return nullptr;
    current = &module.operations.at(static_cast<size_t>(current->parentId));
  }
  return current;
}

inline AddressSpace FunctionDefaultAddressSpace(
    const GenericOperation &function, PipelineMetadataCache &metadata) {
  const std::string core = DecomposeEnumValue(
      metadata.dictionaryValue(function.attributes, "hivm.func_core_type"));
  return core == "AIC" ? AddressSpace::L1 : AddressSpace::UB;
}

inline std::string AddressSpaceName(AddressSpace space) {
  switch (space) {
  case AddressSpace::GM:
    return "gm";
  case AddressSpace::L1:
    return "cbuf";
  case AddressSpace::L0A:
    return "ca";
  case AddressSpace::L0B:
    return "cb";
  case AddressSpace::L0C:
    return "cc";
  case AddressSpace::UB:
    return "ub";
  case AddressSpace::Zero:
    return "zero";
  case AddressSpace::Unknown:
    return "unknown";
  }
  return "unknown";
}

inline int AllocationSourceOperation(const std::string &source) {
  const size_t first = source.find(':');
  if (first == std::string::npos)
    return -1;
  const size_t second = source.find(':', first + 1);
  if (second == std::string::npos)
    return -1;
  return std::stoi(source.substr(first + 1, second - first - 1));
}

inline std::map<std::string, AddressSpace>
InferHIVMMemScope(const PostBufferizationRewriteState &module) {
  std::map<std::string, AddressSpace> result;
  const GenericModule &logical = module.bufferized.logicalModule;
  const GenericModuleAnalysisIndexes &analysis =
      module.bufferized.logicalContext.analysis;
  PipelineMetadataCache &metadata = module.bufferized.logicalContext.metadata;
  analysis.ensureCompatible(logical);
  for (size_t index = 0; index < module.singlePoint.allocations.size(); ++index) {
    const BufferAllocation &allocation = module.singlePoint.allocations[index];
    const int operationId = AllocationSourceOperation(allocation.source);
    if (operationId < 0)
      throw std::runtime_error("InferHIVMMemScope: malformed allocation source");
    const int functionId = analysis.enclosingFunctionId(operationId);
    const GenericOperation *function =
        functionId < 0
            ? nullptr
            : &logical.operations.at(static_cast<size_t>(functionId));
    if (!function)
      throw std::runtime_error("InferHIVMMemScope: allocation outside function");
    const std::optional<MemRefTypeModel> type =
        metadata.memRefType(allocation.type);
    result["base:" + std::to_string(index)] =
        type && type->addressSpace != AddressSpace::Unknown
            ? type->addressSpace
            : FunctionDefaultAddressSpace(*function, metadata);
  }

  std::map<int, size_t> decomposeOrdinals;
  for (const DecomposeBufferAllocation &allocation :
       module.decomposeAllocations) {
    const int functionId =
        analysis.enclosingFunctionId(allocation.ownerOperation);
    const GenericOperation *function =
        functionId < 0
            ? nullptr
            : &logical.operations.at(static_cast<size_t>(functionId));
    if (!function)
      throw std::runtime_error("InferHIVMMemScope: temp outside function");
    const size_t ordinal = decomposeOrdinals[allocation.ownerOperation]++;
    result["decompose:" + std::to_string(allocation.ownerOperation) + ":" +
           std::to_string(ordinal)] =
        FunctionDefaultAddressSpace(*function, metadata);
  }

  for (const GenericOperation &operation : logical.operations) {
    if (operation.name != "hivm.hir.mmadL1")
      continue;
    const std::vector<std::string> buffers = OperationBufferOperands(
        module.bufferized, operation.id, &module.singlePoint.bufferMapping);
    if (buffers.size() < 3)
      continue;
    for (size_t index = 0; index < 3; ++index) {
      const AddressSpace space =
          index < 2 ? AddressSpace::L1 : AddressSpace::L0C;
      for (const std::string &alternative : BufferAlternatives(buffers[index]))
        if (startsWith(alternative, "local:"))
          result["base:" + alternative.substr(6)] = space;
    }
  }
  return result;
}

} // namespace cvub

#endif
