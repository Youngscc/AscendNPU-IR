#ifndef CVPIPELINE_UB_MODEL_CPP_BUFFERIZED_SEMANTIC_IR_HPP
#define CVPIPELINE_UB_MODEL_CPP_BUFFERIZED_SEMANTIC_IR_HPP

#include "../analysis/one_shot_analysis.hpp"
#include "../analysis/generic_pipeline_context.hpp"
#include "../ir/generic_analysis.hpp"

namespace cvub {

struct BufferizedValueBinding {
  int valueId = -1;
  std::string bufferId;
  std::string type;
  std::string owner;
};

struct BufferizedOperandAccess {
  int operationId = -1;
  int operandNumber = -1;
  std::string bufferId;
};

struct BufferizedSemanticIR {
  GenericModule logicalModule;
  GenericPipelineSnapshotContext logicalContext;
  PreBufferizationCSEState preBufferizationCSE;
  std::vector<BufferAllocation> allocations;
  std::vector<BufferizedValueBinding> values;
  std::vector<BufferizedOperandAccess> accesses;
  // Read-only dense indexes derived from values/accesses. Generic value and
  // operation IDs are non-negative dense integers, so tree maps add an
  // allocation and string copy at every stage boundary for no semantic gain.
  // Keep the ordered source records authoritative for serialization, while
  // all normal pipeline queries share these O(1) indexes.
  std::vector<std::string> bufferByValue;
  std::vector<std::vector<std::string>> buffersByOperation;
  size_t indexedValueCount = 0;
  size_t indexedAccessCount = 0;
};

inline const std::string *
FindBufferizedValueBuffer(const BufferizedSemanticIR &module, int valueId) {
  if (module.indexedValueCount == module.values.size()) {
    if (valueId < 0 ||
        static_cast<size_t>(valueId) >= module.bufferByValue.size() ||
        module.bufferByValue[static_cast<size_t>(valueId)].empty())
      return nullptr;
    return &module.bufferByValue[static_cast<size_t>(valueId)];
  }
  // Focused tests and embedding callers may assemble the public records
  // directly. Preserve that construction path without penalizing the normal
  // immutable pipeline state.
  for (const BufferizedValueBinding &binding : module.values)
    if (binding.valueId == valueId)
      return &binding.bufferId;
  return nullptr;
}

inline const std::string *FindBufferizedOperationBuffer(
    const BufferizedSemanticIR &module, int operationId, size_t operand) {
  if (module.indexedAccessCount == module.accesses.size()) {
    if (operationId < 0 ||
        static_cast<size_t>(operationId) >= module.buffersByOperation.size())
      return nullptr;
    const std::vector<std::string> &operation =
        module.buffersByOperation[static_cast<size_t>(operationId)];
    if (operand >= operation.size() || operation[operand].empty())
      return nullptr;
    return &operation[operand];
  }
  for (const BufferizedOperandAccess &access : module.accesses)
    if (access.operationId == operationId && access.operandNumber >= 0 &&
        static_cast<size_t>(access.operandNumber) == operand)
      return &access.bufferId;
  return nullptr;
}

template <typename Callback>
inline void ForEachBufferizedOperationBuffer(
    const BufferizedSemanticIR &module, int operationId, Callback &&callback) {
  if (module.indexedAccessCount == module.accesses.size()) {
    if (operationId < 0 ||
        static_cast<size_t>(operationId) >= module.buffersByOperation.size())
      return;
    const std::vector<std::string> &operation =
        module.buffersByOperation[static_cast<size_t>(operationId)];
    for (size_t operand = 0; operand < operation.size(); ++operand)
      if (!operation[operand].empty())
        callback(operand, operation[operand]);
    return;
  }
  for (const BufferizedOperandAccess &access : module.accesses)
    if (access.operationId == operationId && access.operandNumber >= 0)
      callback(static_cast<size_t>(access.operandNumber), access.bufferId);
}

inline std::map<size_t, std::string>
BufferizedOperationBuffers(const BufferizedSemanticIR &module,
                           int operationId) {
  if (module.indexedAccessCount == module.accesses.size()) {
    std::map<size_t, std::string> result;
    ForEachBufferizedOperationBuffer(
        module, operationId,
        [&](size_t operand, const std::string &buffer) {
          result.emplace(operand, buffer);
        });
    return result;
  }

  // Focused tests and embedding callers may assemble the public semantic
  // state directly. Keep the ordered records authoritative for those mutable
  // construction paths; normal pipeline states use the shared index above.
  std::map<size_t, std::string> result;
  for (const BufferizedOperandAccess &access : module.accesses)
    if (access.operationId == operationId)
      result[static_cast<size_t>(access.operandNumber)] = access.bufferId;
  return result;
}

inline std::string LocalBufferId(size_t ordinal) {
  return "local:" + std::to_string(ordinal);
}

class BufferizedSemanticIRBuilder {
public:
  BufferizedSemanticIRBuilder(
      GenericModule inputModule,
      std::vector<BufferAllocation> inputAllocations,
      PreBufferizationCSEState inputPreBufferizationCSE,
      GenericModuleAnalysisIndexes inputAnalysis)
      : module(std::move(inputModule)),
        analysis(std::move(inputAnalysis)),
        allocations(std::move(inputAllocations)),
        preBufferizationCSE(std::move(inputPreBufferizationCSE)) {
    constexpr unsigned requiredAnalysis =
        kGenericAnalysisDefinitions | kGenericAnalysisUsers |
        kGenericAnalysisValueTypes | kGenericAnalysisEnclosingFunctions;
    // Preserve the public/manual construction path while the normal pipeline
    // moves the already-built OneShot snapshot through without rescanning.
    if (!analysis.provides(requiredAnalysis))
      analysis.Build(module, requiredAnalysis);
    else
      analysis.ensureCompatible(module);
    blockArguments.assign(analysis.valueCount(), {-1, -1});
    buffers.resize(analysis.valueCount());
    fixedBuffers.assign(analysis.valueCount(), false);
    indexBlockArguments();
    indexAllocationOwners();
  }

  BufferizedSemanticIR Build() {
    initializeExternalBuffers();
    initializeAllocationResults();
    initializeExistingAliases();
    propagateTensorBuffers();

    BufferizedSemanticIR result;
    for (size_t valueOrdinal = 0; valueOrdinal < analysis.valueCount();
         ++valueOrdinal) {
      const int value = static_cast<int>(valueOrdinal);
      const std::string *type = analysis.valueType(value);
      if (!type || (!IsTensorType(*type) && !IsMemRefType(*type)))
        continue;
      const int definitionId = analysis.definingOperationId(value);
      const GenericOperation *definition =
          definitionId < 0
              ? nullptr
              : &module.operations.at(static_cast<size_t>(definitionId));
      if (definition &&
          preBufferizationCSE.erasedOperations.count(
              definition->id) != 0)
        continue;
      if (preBufferizationCSE.elidedTensorEmptyResults.count(value) != 0)
        continue;
      if (bufferForValue(value) == "dead")
        continue;
      BufferizedValueBinding binding{
          value, bufferForValue(value),
          IsTensorType(*type) ? ConvertTensorToMemRefType(*type) : *type,
          ownerForValue(value)};
      if (binding.valueId >= 0) {
        const size_t valueId = static_cast<size_t>(binding.valueId);
        if (result.bufferByValue.size() <= valueId)
          result.bufferByValue.resize(valueId + 1);
        result.bufferByValue[valueId] = binding.bufferId;
      }
      result.values.push_back(std::move(binding));
    }
    result.indexedValueCount = result.values.size();
    collectAccesses(result.accesses);
    result.buffersByOperation.resize(module.operations.size());
    for (const BufferizedOperandAccess &access : result.accesses) {
      if (access.operationId < 0 || access.operandNumber < 0)
        continue;
      const size_t operationId = static_cast<size_t>(access.operationId);
      if (result.buffersByOperation.size() <= operationId)
        result.buffersByOperation.resize(operationId + 1);
      std::vector<std::string> &operation =
          result.buffersByOperation[operationId];
      const size_t operand = static_cast<size_t>(access.operandNumber);
      if (operation.size() <= operand)
        operation.resize(operand + 1);
      operation[operand] = access.bufferId;
    }
    result.indexedAccessCount = result.accesses.size();
    result.logicalContext.analysis = std::move(analysis);
    result.logicalModule = std::move(module);
    result.preBufferizationCSE = std::move(preBufferizationCSE);
    result.allocations = std::move(allocations);
    return result;
  }

private:
  void indexBlockArguments() {
    for (const GenericBlock &block : module.blocks)
      for (size_t index = 0; index < block.arguments.size(); ++index)
        blockArguments.at(static_cast<size_t>(block.arguments[index])) =
            {block.id, static_cast<int>(index)};
  }

  void indexAllocationOwners() {
    for (size_t ordinal = 0; ordinal < allocations.size(); ++ordinal)
      allocationBySource[allocations[ordinal].source] = ordinal;
  }

  std::string resultSource(int operation, int result) const {
    return "result:" + std::to_string(operation) + ":" +
           std::to_string(result);
  }

  std::string operandSource(int operation, int operand) const {
    return "operand:" + std::to_string(operation) + ":" +
           std::to_string(operand);
  }

  void initializeExternalBuffers() {
    for (const GenericBlock &block : module.blocks) {
      const GenericRegion &region =
          module.regions.at(static_cast<size_t>(block.regionId));
      const GenericOperation &owner = module.operations.at(
          static_cast<size_t>(region.parentOperation));
      if (owner.name != "func.func")
        continue;
      for (size_t index = 0; index < block.arguments.size(); ++index) {
        const std::string &type = block.argumentTypes[index];
        if (IsTensorType(type) || IsMemRefType(type))
          buffers.at(static_cast<size_t>(block.arguments[index])) =
              "arg:" + std::to_string(owner.id) + ":" +
              std::to_string(index);
      }
    }
    for (const GenericOperation &operation : module.operations) {
      if (operation.name == "memref_ext.alloc_workspace")
        for (int result : operation.results)
          buffers.at(static_cast<size_t>(result)) =
              "workspace:" + std::to_string(operation.id);
      if (operation.name == "hivm.hir.pointer_cast")
        for (int result : operation.results)
          buffers.at(static_cast<size_t>(result)) =
              "pointer:" + std::to_string(operation.id);
      if (operation.name == "bufferization.alloc_tensor")
        for (int result : operation.results)
          if (analysis.users(result).empty())
            buffers.at(static_cast<size_t>(result)) = "dead";
    }
  }

  void initializeAllocationResults() {
    for (const GenericOperation &operation : module.operations) {
      for (size_t index = 0; index < operation.results.size(); ++index) {
        auto allocation = allocationBySource.find(
            resultSource(operation.id, static_cast<int>(index)));
        if (allocation != allocationBySource.end())
          buffers.at(static_cast<size_t>(operation.results[index])) =
              LocalBufferId(allocation->second);
        if (allocation != allocationBySource.end())
          fixedBuffers.at(static_cast<size_t>(operation.results[index])) = true;
      }
    }
  }

  void initializeExistingAliases() {
    bool changed = true;
    while (changed) {
      changed = false;
      for (const GenericOperation &operation : module.operations) {
        if (operation.results.empty() || operation.operands.empty())
          continue;
        static const std::set<std::string> aliases = {
            "memref.cast", "memref.collapse_shape", "memref.expand_shape",
            "memref.reinterpret_cast", "memref.subview", "memref.view",
            "hivm.hir.pointer_cast"};
        if (aliases.count(operation.name) == 0)
          continue;
        const int sourceValue = operation.operands.front();
        if (sourceValue < 0 ||
            static_cast<size_t>(sourceValue) >= buffers.size() ||
            buffers[static_cast<size_t>(sourceValue)].empty())
          continue;
        for (int result : operation.results)
          changed |= setBuffer(
              result, buffers[static_cast<size_t>(sourceValue)]);
      }
    }
  }

  std::string effectiveOperandBuffer(const GenericOperation &operation,
                                     size_t operand) const {
    auto allocation = allocationBySource.find(
        operandSource(operation.id, static_cast<int>(operand)));
    if (allocation != allocationBySource.end())
      return LocalBufferId(allocation->second);
    const std::string buffer = bufferForValue(operation.operands.at(operand));
    return startsWith(buffer, "unresolved:") ? std::string() : buffer;
  }

  bool setBuffer(int value, const std::string &buffer) {
    if (buffer.empty() || startsWith(buffer, "unresolved:"))
      return false;
    if (value < 0 || static_cast<size_t>(value) >= buffers.size())
      throw std::runtime_error("OneShotBufferize: invalid value ID");
    std::string &found = buffers[static_cast<size_t>(value)];
    if (found.empty()) {
      found = buffer;
      return true;
    }
    if (found == buffer)
      return false;
    if (fixedBuffers[static_cast<size_t>(value)])
      return false;
    if (startsWith(found, "unresolved:")) {
      found = buffer;
      return true;
    }
    std::set<std::string> alternatives;
    collectAlternatives(found, alternatives);
    collectAlternatives(buffer, alternatives);
    std::string choice = "choice(";
    for (const std::string &alternative : alternatives) {
      if (choice.size() != 7)
        choice += ',';
      choice += alternative;
    }
    choice += ')';
    if (found == choice)
      return false;
    found = choice;
    return true;
  }

  void collectAlternatives(const std::string &buffer,
                           std::set<std::string> &result) const {
    if (!startsWith(buffer, "choice(") || buffer.back() != ')') {
      result.insert(buffer);
      return;
    }
    for (const std::string &item :
         splitTopLevel(buffer.substr(7, buffer.size() - 8)))
      collectAlternatives(item, result);
  }

  void propagateTensorBuffers() {
    for (size_t iteration = 0; iteration <= module.operations.size(); ++iteration) {
      bool changed = false;
      for (const GenericOperation &operation : module.operations) {
        if (operation.name == "bufferization.to_tensor" &&
            !operation.results.empty() && !operation.operands.empty()) {
          changed |= setBuffer(operation.results.front(),
                               effectiveOperandBuffer(operation, 0));
          continue;
        }
        static const std::set<std::string> aliases = {
            "tensor.cast", "tensor.collapse_shape", "tensor.expand_shape",
            "tensor.extract_slice", "hivm.hir.bitcast"};
        if (aliases.count(operation.name) != 0 &&
            !operation.results.empty() && !operation.operands.empty()) {
          changed |= setBuffer(operation.results.front(),
                               effectiveOperandBuffer(operation, 0));
        }
        const std::vector<size_t> inits = DpsInitOperandIndices(
            operation.name, operation.operands.size(), operation.properties);
        for (size_t result = 0;
             result < inits.size() && result < operation.results.size(); ++result)
          changed |= setBuffer(operation.results[result],
                               effectiveOperandBuffer(operation, inits[result]));
        if (operation.name == "arith.select" && !operation.results.empty())
          for (size_t operand = 1; operand < operation.operands.size(); ++operand)
            changed |= setBuffer(operation.results.front(),
                                 effectiveOperandBuffer(operation, operand));
        if (operation.name == "scf.for")
          changed |= propagateForOp(operation);
        if (operation.name == "scf.if")
          changed |= propagateIfOp(operation);
      }
      if (!changed)
        return;
    }
    throw std::runtime_error("OneShotBufferize: buffer propagation did not converge");
  }

  bool propagateForOp(const GenericOperation &operation) {
    if (operation.regions.empty())
      return false;
    const GenericRegion &region = module.regions.at(
        static_cast<size_t>(operation.regions.front()));
    if (region.blocks.empty())
      return false;
    const GenericBlock &block = module.blocks.at(
        static_cast<size_t>(region.blocks.front()));
    bool changed = false;
    for (size_t result = 0; result < operation.results.size(); ++result) {
      const size_t operand = result + 3;
      const size_t argument = result + 1;
      if (operand >= operation.operands.size() ||
          argument >= block.arguments.size())
        continue;
      const std::string buffer = effectiveOperandBuffer(operation, operand);
      changed |= setBuffer(operation.results[result], buffer);
      changed |= setBuffer(block.arguments[argument], buffer);
    }
    return changed;
  }

  bool propagateIfOp(const GenericOperation &operation) {
    bool changed = false;
    for (size_t result = 0; result < operation.results.size(); ++result)
      for (int regionId : operation.regions) {
        const GenericRegion &region =
            module.regions.at(static_cast<size_t>(regionId));
        if (region.blocks.empty())
          continue;
        const GenericBlock &block = module.blocks.at(
            static_cast<size_t>(region.blocks.front()));
        if (block.operations.empty())
          continue;
        const GenericOperation &yield = module.operations.at(
            static_cast<size_t>(block.operations.back()));
        if (yield.name == "scf.yield" && result < yield.operands.size())
          changed |= setBuffer(operation.results[result],
                               bufferForValue(yield.operands[result]));
      }
    return changed;
  }

  void collectAccesses(std::vector<BufferizedOperandAccess> &result) const {
    for (const GenericOperation &operation : module.operations) {
      if (preBufferizationCSE.erasedOperations.count(operation.id) != 0)
        continue;
      for (size_t operand = 0; operand < operation.operandTypes.size(); ++operand) {
        if (!IsTensorType(operation.operandTypes[operand]) &&
            !IsMemRefType(operation.operandTypes[operand]))
          continue;
        const std::string buffer = effectiveOperandBuffer(operation, operand);
        if (buffer.empty())
          continue;
        result.push_back(
            {operation.id, static_cast<int>(operand), buffer});
      }
    }
  }

  std::string bufferForValue(int value) const {
    auto alias = preBufferizationCSE.valueAliases.find(value);
    if (alias != preBufferizationCSE.valueAliases.end())
      value = alias->second;
    if (value < 0 || static_cast<size_t>(value) >= buffers.size() ||
        buffers[static_cast<size_t>(value)].empty())
      return "unresolved:" + std::to_string(value);
    return buffers[static_cast<size_t>(value)];
  }

  std::string ownerForValue(int value) const {
    const int definitionId = analysis.definingOperationId(value);
    const GenericOperation *definition =
        definitionId < 0
            ? nullptr
            : &module.operations.at(static_cast<size_t>(definitionId));
    if (definition)
      return "result:" + std::to_string(definition->id);
    if (value >= 0 && static_cast<size_t>(value) < blockArguments.size()) {
      const std::pair<int, int> &argument =
          blockArguments[static_cast<size_t>(value)];
      if (argument.first >= 0)
        return "block:" + std::to_string(argument.first) + ":" +
               std::to_string(argument.second);
    }
    return "unknown";
  }

  GenericModule module;
  GenericModuleAnalysisIndexes analysis;
  std::vector<BufferAllocation> allocations;
  PreBufferizationCSEState preBufferizationCSE;
  std::vector<std::pair<int, int>> blockArguments;
  std::map<std::string, size_t> allocationBySource;
  std::vector<std::string> buffers;
  std::vector<bool> fixedBuffers;
};

inline BufferizedSemanticIR BuildBufferizedSemanticIR(
    GenericModule module, OneShotBufferizationResult bufferization) {
  return BufferizedSemanticIRBuilder(
             std::move(module), std::move(bufferization.allocations),
             std::move(bufferization.preBufferizationCSE),
             std::move(bufferization.analysis))
      .Build();
}

inline std::string
SerializeBufferizedSemanticIR(const BufferizedSemanticIR &module) {
  std::ostringstream output;
  output << "BUFFERIZED_SEMANTIC_IR\t1\n";
  for (size_t index = 0; index < module.allocations.size(); ++index) {
    const BufferAllocation &allocation = module.allocations[index];
    output << "ALLOC\t" << index << '\t' << HexEncode(allocation.type) << '\t'
           << HexEncode(allocation.alignment) << '\t'
           << allocation.dynamicExtentCount << '\t'
           << HexEncode(allocation.source) << '\t'
           << joinIds(allocation.dynamicExtentValues) << '\n';
  }
  for (int operation : module.preBufferizationCSE.erasedOperations)
    output << "PRE_BUFFERIZATION_CSE_ERASED\t" << operation << '\n';
  for (const auto &[value, alias] : module.preBufferizationCSE.valueAliases)
    output << "PRE_BUFFERIZATION_CSE_VALUE_ALIAS\t" << value << '\t' << alias
           << '\n';
  for (int value : module.preBufferizationCSE.elidedTensorEmptyResults)
    output << "PRE_BUFFERIZATION_CSE_ELIDED_EMPTY\t" << value << '\n';
  for (const BufferizedValueBinding &value : module.values)
    output << "VALUE\t" << value.valueId << '\t' << HexEncode(value.bufferId)
           << '\t' << HexEncode(value.type) << '\t' << HexEncode(value.owner)
           << '\n';
  for (const BufferizedOperandAccess &access : module.accesses)
    output << "ACCESS\t" << access.operationId << '\t'
           << access.operandNumber << '\t' << HexEncode(access.bufferId)
           << '\n';
  return output.str();
}

} // namespace cvub

#endif
