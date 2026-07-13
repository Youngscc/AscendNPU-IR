#ifndef CVPIPELINE_UB_MODEL_CPP_ONE_SHOT_BUFFERIZE_HPP
#define CVPIPELINE_UB_MODEL_CPP_ONE_SHOT_BUFFERIZE_HPP

#include "buffer_topology.hpp"

namespace cvub {

struct BufferAllocation {
  std::string type;
  std::string alignment;
  size_t dynamicExtentCount = 0;
  std::string source;
  int ownerOperation = -1;
  std::vector<int> dynamicExtentValues;
};

struct PreBufferizationCSEState {
  std::map<int, int> valueAliases;
  std::set<int> erasedOperations;
  std::set<int> elidedTensorEmptyResults;
};

inline std::string ConvertTensorToMemRefType(const std::string &type) {
  if (!startsWith(type, "tensor<") || type.back() != '>')
    throw std::runtime_error("OneShotBufferize: expected ranked tensor type");
  return "memref<" + type.substr(7);
}

inline std::map<int, std::string>
ValueTypes(const GenericModule &module) {
  std::map<int, std::string> result;
  for (const GenericBlock &block : module.blocks)
    for (size_t index = 0; index < block.arguments.size(); ++index)
      result[block.arguments[index]] = block.argumentTypes[index];
  for (const GenericOperation &operation : module.operations)
    for (size_t index = 0; index < operation.results.size() &&
                           index < operation.resultTypes.size(); ++index)
      result[operation.results[index]] = operation.resultTypes[index];
  return result;
}

inline std::map<int, const GenericOperation *>
DefiningOperations(const GenericModule &module) {
  std::map<int, const GenericOperation *> result;
  for (const GenericOperation &operation : module.operations)
    for (int value : operation.results)
      result[value] = &operation;
  return result;
}

inline std::map<int, size_t> ValueUseCounts(const GenericModule &module) {
  std::map<int, size_t> result;
  for (const GenericOperation &operation : module.operations)
    for (int value : operation.operands)
      ++result[value];
  return result;
}

inline PreBufferizationCSEState
ModelPreBufferizationCSE(const GenericModule &module) {
  PreBufferizationCSEState result;
  const std::map<int, const GenericOperation *> definitions =
      DefiningOperations(module);
  const std::map<int, size_t> useCounts = ValueUseCounts(module);
  std::map<std::string, int> canonicalVbrcResult;
  for (const GenericOperation &operation : module.operations) {
    if (operation.name != "hivm.hir.vbrc" || operation.operands.size() < 2 ||
        operation.results.size() != 1 || operation.resultTypes.size() != 1 ||
        operation.operandTypes.empty() ||
        !startsWith(operation.resultTypes.front(), "tensor<"))
      continue;
    std::ostringstream key;
    key << operation.name << '\n'
        << operation.operands.front() << '\n'
        << operation.operandTypes.front() << '\n'
        << operation.resultTypes.front() << '\n'
        << operation.attributes << '\n'
        << operation.properties;
    auto canonical = canonicalVbrcResult.find(key.str());
    if (canonical == canonicalVbrcResult.end()) {
      canonicalVbrcResult[key.str()] = operation.results.front();
      continue;
    }
    result.erasedOperations.insert(operation.id);
    result.valueAliases[operation.results.front()] = canonical->second;
    const int destination = operation.operands[1];
    auto definition = definitions.find(destination);
    auto uses = useCounts.find(destination);
    if (definition != definitions.end() &&
        definition->second->name == "tensor.empty" &&
        uses != useCounts.end() && uses->second == 1)
      result.elidedTensorEmptyResults.insert(destination);
  }
  return result;
}

inline std::vector<BufferAllocation>
CollectBufferAllocationOracle(const GenericModule &module) {
  std::vector<BufferAllocation> result;
  for (const GenericOperation &operation : module.operations) {
    if (operation.name != "memref.alloc")
      continue;
    for (size_t index = 0; index < operation.results.size() &&
                           index < operation.resultTypes.size(); ++index) {
      if (!IsMemRefType(operation.resultTypes[index]))
        continue;
      result.push_back({operation.resultTypes[index],
                        FindDictionaryValue(operation.attributes, "alignment"),
                        operation.operands.size(), "memref.alloc",
                        operation.id,
                        operation.operands});
    }
  }
  return result;
}

inline std::string SerializeBufferAllocations(
    const std::vector<BufferAllocation> &allocations) {
  std::ostringstream output;
  output << "ONE_SHOT_BUFFERIZE_ALLOCATIONS\t1\n";
  for (size_t index = 0; index < allocations.size(); ++index) {
    const BufferAllocation &allocation = allocations[index];
    output << "ALLOC\t" << index << '\t' << HexEncode(allocation.type) << '\t'
           << HexEncode(allocation.alignment) << '\t'
           << allocation.dynamicExtentCount << '\n';
  }
  return output.str();
}

} // namespace cvub

#endif
