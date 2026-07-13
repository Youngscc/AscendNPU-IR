#ifndef CVPIPELINE_UB_MODEL_CPP_BUFFERIZED_SEMANTIC_IR_ORACLE_HPP
#define CVPIPELINE_UB_MODEL_CPP_BUFFERIZED_SEMANTIC_IR_ORACLE_HPP

#include "../suffix/bufferized_semantic_ir.hpp"

namespace cvub {

struct BufferizedSemanticIRValidation {
  size_t checkedValues = 0;
  size_t checkedAccesses = 0;
  std::vector<std::string> errors;
};

inline bool HasShapedSemantics(const C1OperationRecord &operation) {
  if (operation.name == "func.func")
    return true;
  return std::any_of(operation.operandTypes.begin(), operation.operandTypes.end(),
                     [](const std::string &type) {
                       return IsTensorType(type) || IsMemRefType(type);
                     }) ||
         std::any_of(operation.resultTypes.begin(), operation.resultTypes.end(),
                     [](const std::string &type) {
                       return IsTensorType(type) || IsMemRefType(type);
                     });
}

inline std::string BufferizedOperationName(const std::string &name) {
  if (name == "tensor.extract")
    return "memref.load";
  if (name == "tensor.insert")
    return "memref.store";
  if (name == "tensor.extract_slice" || name == "tensor.insert_slice")
    return "memref.subview";
  if (name == "tensor.expand_shape")
    return "memref.expand_shape";
  if (name == "tensor.collapse_shape")
    return "memref.collapse_shape";
  return name;
}

inline bool IsErasedByOneShotBufferize(const std::string &name) {
  return name == "bufferization.to_tensor" ||
         name == "bufferization.alloc_tensor" || name == "tensor.empty" ||
         name == "tensor.from_elements" || name == "memref.alloc";
}

class BufferizedOperationCorrespondence {
public:
  BufferizedOperationCorrespondence(const C1SemanticModule &before,
                                    const C1SemanticModule &after)
      : before(before), after(after) {
    build();
  }

  int lookup(int beforeOperation) const {
    auto found = operationMap.find(beforeOperation);
    return found == operationMap.end() ? -1 : found->second;
  }

  const std::vector<std::string> &getErrors() const { return errors; }

private:
  void build() {
    std::map<std::string, std::vector<int>> expected;
    std::map<std::string, std::vector<int>> actual;
    for (const C1OperationRecord &operation : before.operations) {
      if (operation.name == "tensor.from_elements") {
        for (size_t index = 0; index < operation.operands.size(); ++index)
          expected["memref.store"].push_back(-1);
        continue;
      }
      if (!HasShapedSemantics(operation) ||
          IsErasedByOneShotBufferize(operation.name))
        continue;
      expected[BufferizedOperationName(operation.name)].push_back(operation.id);
    }
    for (const C1OperationRecord &operation : after.operations)
      if (HasShapedSemantics(operation))
        actual[operation.name].push_back(operation.id);

    for (const auto &[name, sourceOperations] : expected) {
      const std::vector<int> &afterOperations = actual[name];
      if (sourceOperations.size() != afterOperations.size()) {
        errors.push_back("operation count mismatch for " + name + ": " +
                         std::to_string(sourceOperations.size()) + " != " +
                         std::to_string(afterOperations.size()));
        continue;
      }
      for (size_t index = 0; index < sourceOperations.size(); ++index)
        if (sourceOperations[index] >= 0)
          operationMap[sourceOperations[index]] = afterOperations[index];
    }
  }

  const C1SemanticModule &before;
  const C1SemanticModule &after;
  std::map<int, int> operationMap;
  std::vector<std::string> errors;
};

class AfterBufferRootResolver {
public:
  explicit AfterBufferRootResolver(const C1SemanticModule &module)
      : module(module) {
    initializeRoots();
    propagate();
  }

  std::string root(int value) const {
    auto found = roots.find(value);
    return found == roots.end() ? "unresolved:" + std::to_string(value)
                                : found->second;
  }

  int allocationValue(size_t ordinal) const {
    return ordinal < allocationValues.size() ? allocationValues[ordinal] : -1;
  }

private:
  void initializeRoots() {
    size_t allocation = 0;
    for (const C1OperationRecord &operation : module.operations) {
      if (operation.name == "memref.alloc")
        for (size_t index = 0; index < operation.results.size(); ++index)
          if (index < operation.resultTypes.size() &&
              IsMemRefType(operation.resultTypes[index])) {
            roots[operation.results[index]] = LocalBufferId(allocation++);
            allocationValues.push_back(operation.results[index]);
          }
      if (operation.name == "memref_ext.alloc_workspace")
        for (int result : operation.results)
          roots[result] = "workspace:" + std::to_string(operation.id);
      if (operation.name == "hivm.hir.pointer_cast")
        for (int result : operation.results)
          roots[result] = "pointer:" + std::to_string(operation.id);
    }
    for (const C1BlockRecord &block : module.blocks) {
      const C1RegionRecord &region =
          module.regions.at(static_cast<size_t>(block.regionId));
      const C1OperationRecord &owner = module.operations.at(
          static_cast<size_t>(region.parentOperation));
      if (owner.name != "func.func")
        continue;
      for (size_t index = 0; index < block.arguments.size(); ++index)
        if (index < block.argumentTypes.size() &&
            IsMemRefType(block.argumentTypes[index]))
          roots[block.arguments[index]] =
              "arg:" + std::to_string(owner.id) + ":" +
              std::to_string(index);
    }
  }

  bool setRoot(int value, const std::string &buffer) {
    if (buffer.empty() || startsWith(buffer, "unresolved:"))
      return false;
    auto found = roots.find(value);
    if (found == roots.end()) {
      roots[value] = buffer;
      return true;
    }
    if (found->second == buffer)
      return false;
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
    if (choice == found->second)
      return false;
    found->second = std::move(choice);
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

  void propagate() {
    for (size_t iteration = 0; iteration <= module.operations.size(); ++iteration) {
      bool changed = false;
      for (const C1OperationRecord &operation : module.operations) {
        static const std::set<std::string> aliases = {
            "arith.select", "hivm.hir.bitcast", "memref.cast",
            "memref.collapse_shape", "memref.expand_shape",
            "memref.extract_strided_metadata", "memref.memory_space_cast",
            "memref.reinterpret_cast", "memref.reshape", "memref.subview",
            "memref.transpose", "memref.view"};
        if (aliases.count(operation.name) != 0 && !operation.results.empty()) {
          const size_t first = operation.name == "arith.select" ? 1 : 0;
          for (size_t operand = first; operand < operation.operands.size(); ++operand)
            changed |= setRoot(operation.results.front(),
                               root(operation.operands[operand]));
        }
        if (operation.name == "scf.for")
          changed |= propagateFor(operation);
        if (operation.name == "scf.if")
          changed |= propagateIf(operation);
      }
      if (!changed)
        return;
    }
    throw std::runtime_error("bufferized oracle root propagation did not converge");
  }

  bool propagateFor(const C1OperationRecord &operation) {
    if (operation.regions.empty())
      return false;
    const C1RegionRecord &region = module.regions.at(
        static_cast<size_t>(operation.regions.front()));
    if (region.blocks.empty())
      return false;
    const C1BlockRecord &block = module.blocks.at(
        static_cast<size_t>(region.blocks.front()));
    bool changed = false;
    for (size_t result = 0; result < operation.results.size(); ++result) {
      const size_t operand = result + 3;
      const size_t argument = result + 1;
      if (operand >= operation.operands.size() ||
          argument >= block.arguments.size())
        continue;
      const std::string buffer = root(operation.operands[operand]);
      changed |= setRoot(operation.results[result], buffer);
      changed |= setRoot(block.arguments[argument], buffer);
    }
    return changed;
  }

  bool propagateIf(const C1OperationRecord &operation) {
    bool changed = false;
    for (size_t result = 0; result < operation.results.size(); ++result)
      for (int regionId : operation.regions) {
        const C1RegionRecord &region =
            module.regions.at(static_cast<size_t>(regionId));
        if (region.blocks.empty())
          continue;
        const C1BlockRecord &block = module.blocks.at(
            static_cast<size_t>(region.blocks.front()));
        if (block.operations.empty())
          continue;
        const C1OperationRecord &yield = module.operations.at(
            static_cast<size_t>(block.operations.back()));
        if (yield.name == "scf.yield" && result < yield.operands.size())
          changed |= setRoot(operation.results[result],
                             root(yield.operands[result]));
      }
    return changed;
  }

  const C1SemanticModule &module;
  std::map<int, std::string> roots;
  std::vector<int> allocationValues;
};

class SourceToAfterValues {
public:
  SourceToAfterValues(const C1SemanticModule &before,
                      const C1SemanticModule &after,
                      const BufferizedOperationCorrespondence &operations,
                      const AfterBufferRootResolver &roots,
                      const std::vector<BufferAllocation> &allocations)
      : before(before), after(after), operations(operations), roots(roots) {
    mapAllocationResults(allocations);
    mapBlockArguments();
    propagate();
  }

  int lookup(int sourceValue) const {
    auto found = valueMap.find(sourceValue);
    return found == valueMap.end() ? -1 : found->second;
  }

private:
  void mapAllocationResults(const std::vector<BufferAllocation> &allocations) {
    for (size_t ordinal = 0; ordinal < allocations.size(); ++ordinal) {
      const std::string &source = allocations[ordinal].source;
      if (!startsWith(source, "result:"))
        continue;
      const std::vector<std::string> fields = split(source, ':');
      if (fields.size() != 3)
        continue;
      const int operation = std::stoi(fields[1]);
      const int result = std::stoi(fields[2]);
      if (operation < 0 || static_cast<size_t>(operation) >= before.operations.size())
        continue;
      const C1OperationRecord &owner =
          before.operations.at(static_cast<size_t>(operation));
      if (result < 0 || static_cast<size_t>(result) >= owner.results.size())
        continue;
      valueMap[owner.results[static_cast<size_t>(result)]] =
          roots.allocationValue(ordinal);
    }
  }

  void mapBlockArguments() {
    for (const C1BlockRecord &sourceBlock : before.blocks) {
      const C1RegionRecord &sourceRegion =
          before.regions.at(static_cast<size_t>(sourceBlock.regionId));
      const int afterOwner = operations.lookup(sourceRegion.parentOperation);
      if (afterOwner < 0)
        continue;
      const C1OperationRecord &owner =
          after.operations.at(static_cast<size_t>(afterOwner));
      if (sourceRegion.ordinal < 0 ||
          static_cast<size_t>(sourceRegion.ordinal) >= owner.regions.size())
        continue;
      const C1RegionRecord &afterRegion = after.regions.at(
          static_cast<size_t>(owner.regions[static_cast<size_t>(sourceRegion.ordinal)]));
      if (sourceBlock.ordinal < 0 ||
          static_cast<size_t>(sourceBlock.ordinal) >= afterRegion.blocks.size())
        continue;
      const C1BlockRecord &afterBlock = after.blocks.at(
          static_cast<size_t>(afterRegion.blocks[static_cast<size_t>(sourceBlock.ordinal)]));
      for (size_t index = 0; index < sourceBlock.arguments.size() &&
                             index < afterBlock.arguments.size(); ++index)
        valueMap[sourceBlock.arguments[index]] = afterBlock.arguments[index];
    }
  }

  bool setValue(int source, int target) {
    if (target < 0)
      return false;
    auto found = valueMap.find(source);
    if (found != valueMap.end())
      return false;
    valueMap[source] = target;
    return true;
  }

  void propagate() {
    for (size_t iteration = 0; iteration <= before.operations.size(); ++iteration) {
      bool changed = false;
      for (const C1OperationRecord &source : before.operations) {
        if (source.results.empty())
          continue;
        if (source.name == "bufferization.to_tensor" &&
            !source.operands.empty()) {
          changed |= setValue(source.results.front(), lookup(source.operands.front()));
          continue;
        }
        const int afterOperation = operations.lookup(source.id);
        if (afterOperation < 0)
          continue;
        const C1OperationRecord &target =
            after.operations.at(static_cast<size_t>(afterOperation));
        if (source.name == "tensor.insert" && target.operands.size() > 1) {
          changed |= setValue(source.results.front(), target.operands[1]);
          continue;
        }
        if (source.name == "tensor.insert_slice" && !target.operands.empty()) {
          changed |= setValue(source.results.front(), target.operands.front());
          continue;
        }
        const std::vector<size_t> inits = C1DpsInitOperandIndices(
            source.name, source.operands.size(), source.properties);
        for (size_t result = 0;
             result < source.results.size() && result < inits.size(); ++result)
          if (inits[result] < target.operands.size())
            changed |= setValue(source.results[result],
                                target.operands[inits[result]]);
        if (!inits.empty())
          continue;
        for (size_t result = 0;
             result < source.results.size() && result < target.results.size(); ++result)
          changed |= setValue(source.results[result], target.results[result]);
      }
      if (!changed)
        return;
    }
  }

  const C1SemanticModule &before;
  const C1SemanticModule &after;
  const BufferizedOperationCorrespondence &operations;
  const AfterBufferRootResolver &roots;
  std::map<int, int> valueMap;
};

inline bool IsLocalOrChoiceBuffer(const std::string &buffer) {
  return startsWith(buffer, "local:") || startsWith(buffer, "choice(");
}

inline int AfterOperandNumber(const C1OperationRecord &source, int operand) {
  if (source.name == "tensor.insert_slice")
    return operand == 1 ? 0 : -1;
  return operand;
}

inline const C1OperationRecord *FindInsertSliceCopy(
    const C1SemanticModule &after, int subviewOperation) {
  const C1OperationRecord &subview = after.operations.at(
      static_cast<size_t>(subviewOperation));
  if (subview.blockId < 0)
    return nullptr;
  const C1BlockRecord &block =
      after.blocks.at(static_cast<size_t>(subview.blockId));
  auto position = std::find(block.operations.begin(), block.operations.end(),
                            subviewOperation);
  if (position == block.operations.end())
    return nullptr;
  for (++position; position != block.operations.end(); ++position) {
    const C1OperationRecord &candidate =
        after.operations.at(static_cast<size_t>(*position));
    if (candidate.name == "memref.copy")
      return &candidate;
    if (candidate.name != "memref.cast" && candidate.name != "memref.subview")
      break;
  }
  return nullptr;
}

inline BufferizedSemanticIRValidation ValidateBufferizedSemanticIR(
    const C1SemanticModule &before, const BufferizedSemanticIR &model,
    const C1SemanticModule &after) {
  BufferizedSemanticIRValidation validation;
  BufferizedOperationCorrespondence operations(before, after);
  validation.errors = operations.getErrors();
  AfterBufferRootResolver roots(after);
  SourceToAfterValues values(before, after, operations, roots,
                             model.allocations);

  for (const BufferizedValueBinding &binding : model.values) {
    if (!IsLocalOrChoiceBuffer(binding.bufferId))
      continue;
    const int afterValue = values.lookup(binding.valueId);
    if (afterValue < 0) {
      validation.errors.push_back("missing after value for source value " +
                                  std::to_string(binding.valueId));
      continue;
    }
    ++validation.checkedValues;
    const std::string actual = roots.root(afterValue);
    if (actual != binding.bufferId)
      validation.errors.push_back("value root mismatch " +
                                  std::to_string(binding.valueId) + ": " +
                                  binding.bufferId + " != " + actual);
  }

  for (const BufferizedOperandAccess &access : model.accesses) {
    if (!IsLocalOrChoiceBuffer(access.bufferId))
      continue;
    const C1OperationRecord &source = before.operations.at(
        static_cast<size_t>(access.operationId));
    if (source.name == "bufferization.to_tensor") {
      if (source.results.empty())
        continue;
      const int afterValue = values.lookup(source.results.front());
      if (afterValue < 0) {
        validation.errors.push_back("missing to_tensor replacement for " +
                                    std::to_string(access.operationId));
        continue;
      }
      ++validation.checkedAccesses;
      const std::string actual = roots.root(afterValue);
      if (actual != access.bufferId)
        validation.errors.push_back("to_tensor alias mismatch " +
                                    std::to_string(access.operationId));
      continue;
    }
    const int afterOperation = operations.lookup(access.operationId);
    const int afterOperand = AfterOperandNumber(source, access.operandNumber);
    if (source.name == "tensor.insert_slice" && access.operandNumber == 0 &&
        afterOperation >= 0) {
      const C1OperationRecord *copy =
          FindInsertSliceCopy(after, afterOperation);
      if (!copy || copy->operands.empty()) {
        validation.errors.push_back("missing insert_slice copy for " +
                                    std::to_string(access.operationId));
        continue;
      }
      ++validation.checkedAccesses;
      const std::string actual = roots.root(copy->operands.front());
      if (actual != access.bufferId)
        validation.errors.push_back("insert_slice source mismatch " +
                                    std::to_string(access.operationId));
      continue;
    }
    if (afterOperation < 0 || afterOperand < 0)
      continue;
    const C1OperationRecord &target = after.operations.at(
        static_cast<size_t>(afterOperation));
    if (static_cast<size_t>(afterOperand) >= target.operands.size()) {
      validation.errors.push_back("missing after operand for source operation " +
                                  std::to_string(access.operationId));
      continue;
    }
    ++validation.checkedAccesses;
    const std::string actual =
        roots.root(target.operands[static_cast<size_t>(afterOperand)]);
    if (actual != access.bufferId)
      validation.errors.push_back("access root mismatch " +
                                  std::to_string(access.operationId) + ":" +
                                  std::to_string(access.operandNumber) + ": " +
                                  access.bufferId + " != " + actual);
  }
  return validation;
}

} // namespace cvub

#endif
