#ifndef CVPIPELINE_UB_MODEL_CPP_ONE_SHOT_BUFFERIZE_HPP
#define CVPIPELINE_UB_MODEL_CPP_ONE_SHOT_BUFFERIZE_HPP

#include "../ir/generic_analysis.hpp"
#include "../ir/generic_rewriter.hpp"
#include "../ir/operation_folder.hpp"
#include "../pipeline/buffer_topology.hpp"

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

inline std::vector<int>
PostBufferizationCSEOperationPreOrder(const GenericModule &module) {
  std::vector<int> result;
  std::function<void(int)> visit = [&](int operationId) {
    result.push_back(operationId);
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    for (int regionId : operation.regions) {
      const GenericRegion &region =
          module.regions.at(static_cast<size_t>(regionId));
      for (int blockId : region.blocks)
        for (int childId :
             module.blocks.at(static_cast<size_t>(blockId)).operations)
          visit(childId);
    }
  };
  if (!module.operations.empty())
    visit(0);
  return result;
}

inline std::string
PostBufferizationCSEOperationKey(const GenericOperation &operation) {
  std::ostringstream key;
  key << operation.name << '\n' << JoinDelimited(operation.resultTypes, ",")
      << '\n' << JoinDelimited(operation.operandTypes, ",") << '\n'
      << joinIds(operation.operands) << '\n';
  if (operation.name == "arith.constant") {
    if (const std::optional<ArithIntegerConstant> integer =
            ParseArithIntegerConstant(operation)) {
      key << "integer:" << integer->width << ':' << integer->bits;
    } else {
      std::string value =
          FindDictionaryValue(operation.properties, "value");
      if (value.empty())
        value = FindDictionaryValue(operation.attributes, "value");
      key << "value:" << trim(std::move(value));
    }
  } else {
    key << operation.properties << '\n' << operation.attributes;
  }
  return key.str();
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

// OneShotBufferize is followed immediately by the HIVM canonicalization
// pipeline. Scalar-only CSE commutes with tensor bufferization, so applying
// this projection before the compact model's OneShot analysis preserves its
// tensor decisions while also reproducing the operation/value renumbering
// produced by the real post-bufferization CSE.
inline GenericModule
RunPostOneShotScalarCSEProjection(GenericModule module) {
  const GenericModuleAnalysisIndexes enclosingFunctions(
      module, kGenericAnalysisEnclosingFunctions);
  std::map<std::pair<int, std::string>, std::vector<int>> availableOperations;
  GenericRewriter rewriter(module);
  for (int operationId : PostBufferizationCSEOperationPreOrder(module)) {
    const GenericOperation snapshot =
        module.operations.at(static_cast<size_t>(operationId));
    if (snapshot.results.empty() || snapshot.blockId < 0 ||
        !snapshot.regions.empty() ||
        (!snapshot.effects.empty() && snapshot.effects != "none") ||
        std::any_of(snapshot.resultTypes.begin(), snapshot.resultTypes.end(),
                    [](const std::string &type) {
                      return startsWith(type, "tensor<") || IsMemRefType(type);
                    }))
      continue;
    const auto key = std::make_pair(
        enclosingFunctions.enclosingFunctionId(snapshot.id),
        PostBufferizationCSEOperationKey(snapshot));
    int dominating = -1;
    for (auto candidate = availableOperations[key].rbegin();
         candidate != availableOperations[key].rend(); ++candidate) {
      if (GenericOperationDominates(
              module, module.operations.at(static_cast<size_t>(*candidate)),
              snapshot)) {
        dominating = *candidate;
        break;
      }
    }
    if (dominating < 0) {
      availableOperations[key].push_back(snapshot.id);
      continue;
    }
    const GenericOperation &candidate =
        module.operations.at(static_cast<size_t>(dominating));
    if (candidate.results.size() != snapshot.results.size())
      continue;
    for (size_t index = 0; index < snapshot.results.size(); ++index)
      rewriter.replaceAllUses(snapshot.results[index], candidate.results[index]);
    rewriter.removeFromBlock(snapshot.blockId, snapshot.id);
  }
  return CompactGenericModule(std::move(module));
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

inline std::string SerializeBufferAllocationDetails(
    const std::vector<BufferAllocation> &allocations) {
  std::ostringstream output;
  output << "ONE_SHOT_BUFFERIZE_ALLOCATION_DETAILS\t1\n";
  for (size_t index = 0; index < allocations.size(); ++index) {
    const BufferAllocation &allocation = allocations[index];
    output << "ALLOC\t" << index << '\t' << HexEncode(allocation.type) << '\t'
           << HexEncode(allocation.alignment) << '\t'
           << allocation.dynamicExtentCount << '\t'
           << HexEncode(allocation.source) << '\t'
           << allocation.ownerOperation << '\n';
  }
  return output.str();
}

} // namespace cvub

#endif
