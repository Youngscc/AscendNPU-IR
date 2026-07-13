#ifndef CVPIPELINE_UB_MODEL_CPP_BUFFERIZED_SEMANTIC_IR_HPP
#define CVPIPELINE_UB_MODEL_CPP_BUFFERIZED_SEMANTIC_IR_HPP

#include "one_shot_analysis.hpp"

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
  std::vector<BufferAllocation> allocations;
  std::vector<BufferizedValueBinding> values;
  std::vector<BufferizedOperandAccess> accesses;
};

inline std::string LocalBufferId(size_t ordinal) {
  return "local:" + std::to_string(ordinal);
}

class BufferizedSemanticIRBuilder {
public:
  BufferizedSemanticIRBuilder(
      const GenericModule &module, std::vector<BufferAllocation> allocations)
      : module(module), allocations(std::move(allocations)),
        valueTypes(ValueTypes(module)),
        definitions(DefiningOperations(module)) {
    indexBlockArguments();
    indexAllocationOwners();
  }

  BufferizedSemanticIR Build() {
    initializeExternalBuffers();
    initializeAllocationResults();
    initializeExistingAliases();
    propagateTensorBuffers();

    BufferizedSemanticIR result;
    result.logicalModule = module;
    result.allocations = allocations;
    for (const auto &[value, type] : valueTypes) {
      if (!IsTensorType(type) && !IsMemRefType(type))
        continue;
      if (bufferForValue(value) == "dead")
        continue;
      result.values.push_back(
          {value, bufferForValue(value),
           IsTensorType(type) ? ConvertTensorToMemRefType(type) : type,
           ownerForValue(value)});
    }
    collectAccesses(result.accesses);
    return result;
  }

private:
  void indexBlockArguments() {
    for (const GenericBlock &block : module.blocks)
      for (size_t index = 0; index < block.arguments.size(); ++index)
        blockArguments[block.arguments[index]] =
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
    std::map<int, size_t> useCounts;
    for (const GenericOperation &operation : module.operations)
      for (int operand : operation.operands)
        ++useCounts[operand];
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
          buffers[block.arguments[index]] =
              "arg:" + std::to_string(owner.id) + ":" +
              std::to_string(index);
      }
    }
    for (const GenericOperation &operation : module.operations) {
      if (operation.name == "memref_ext.alloc_workspace")
        for (int result : operation.results)
          buffers[result] = "workspace:" + std::to_string(operation.id);
      if (operation.name == "hivm.hir.pointer_cast")
        for (int result : operation.results)
          buffers[result] = "pointer:" + std::to_string(operation.id);
      if (operation.name == "bufferization.alloc_tensor")
        for (int result : operation.results)
          if (useCounts[result] == 0)
            buffers[result] = "dead";
    }
  }

  void initializeAllocationResults() {
    for (const GenericOperation &operation : module.operations) {
      for (size_t index = 0; index < operation.results.size(); ++index) {
        auto allocation = allocationBySource.find(
            resultSource(operation.id, static_cast<int>(index)));
        if (allocation != allocationBySource.end())
          buffers[operation.results[index]] =
              LocalBufferId(allocation->second);
        if (allocation != allocationBySource.end())
          fixedBuffers.insert(operation.results[index]);
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
        auto source = buffers.find(operation.operands.front());
        if (source == buffers.end())
          continue;
        for (int result : operation.results)
          changed |= setBuffer(result, source->second);
      }
    }
  }

  std::string effectiveOperandBuffer(const GenericOperation &operation,
                                     size_t operand) const {
    auto allocation = allocationBySource.find(
        operandSource(operation.id, static_cast<int>(operand)));
    if (allocation != allocationBySource.end())
      return LocalBufferId(allocation->second);
    auto found = buffers.find(operation.operands.at(operand));
    return found == buffers.end() ? std::string() : found->second;
  }

  bool setBuffer(int value, const std::string &buffer) {
    if (buffer.empty() || startsWith(buffer, "unresolved:"))
      return false;
    auto found = buffers.find(value);
    if (found == buffers.end()) {
      buffers[value] = buffer;
      return true;
    }
    if (found->second == buffer)
      return false;
    if (fixedBuffers.count(value) != 0)
      return false;
    if (startsWith(found->second, "unresolved:")) {
      found->second = buffer;
      return true;
    }
    std::set<std::string> alternatives;
    collectAlternatives(found->second, alternatives);
    collectAlternatives(buffer, alternatives);
    std::string choice = "choice(";
    for (const std::string &alternative : alternatives) {
      if (choice.size() != 7)
        choice += ',';
      choice += alternative;
    }
    choice += ')';
    if (found->second == choice)
      return false;
    found->second = choice;
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
    for (const GenericOperation &operation : module.operations)
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

  std::string bufferForValue(int value) const {
    auto found = buffers.find(value);
    return found == buffers.end() ? "unresolved:" + std::to_string(value)
                                  : found->second;
  }

  std::string ownerForValue(int value) const {
    auto definition = definitions.find(value);
    if (definition != definitions.end())
      return "result:" + std::to_string(definition->second->id);
    auto argument = blockArguments.find(value);
    if (argument != blockArguments.end())
      return "block:" + std::to_string(argument->second.first) + ":" +
             std::to_string(argument->second.second);
    return "unknown";
  }

  const GenericModule &module;
  std::vector<BufferAllocation> allocations;
  std::map<int, std::string> valueTypes;
  std::map<int, const GenericOperation *> definitions;
  std::map<int, std::pair<int, int>> blockArguments;
  std::map<std::string, size_t> allocationBySource;
  std::map<int, std::string> buffers;
  std::set<int> fixedBuffers;
};

inline BufferizedSemanticIR BuildBufferizedSemanticIR(
    const GenericModule &module,
    const OneShotBufferizationResult &bufferization) {
  return BufferizedSemanticIRBuilder(module, bufferization.allocations)
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
