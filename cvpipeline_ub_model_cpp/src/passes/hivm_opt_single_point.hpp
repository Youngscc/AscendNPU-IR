#ifndef CVPIPELINE_UB_MODEL_CPP_HIVM_OPT_SINGLE_POINT_HPP
#define CVPIPELINE_UB_MODEL_CPP_HIVM_OPT_SINGLE_POINT_HPP

#include "../pipeline/bufferized_semantic_ir.hpp"

namespace cvub {

struct HIVMOptSinglePointResult {
  std::vector<BufferAllocation> allocations;
  std::map<std::string, std::string> bufferMapping;
  std::set<int> scalarizedOperations;
  std::map<std::string, size_t> canonicalizedIterArgResults;
  std::set<std::pair<int, int>> canonicalizedIterArgResultKeys;
  std::set<std::pair<int, int>> canonicalizedIterArgAccesses;
};

inline bool IsSinglePointType(const std::string &type,
                              bool requireScalarHardwareType) {
  std::optional<MemRefTypeModel> parsed = ParseMemRefType(type);
  if (!parsed ||
      !std::all_of(parsed->shape.begin(), parsed->shape.end(),
                   [](const std::optional<int64_t> &dimension) {
                     return dimension && *dimension == 1;
                   }))
    return false;
  return !requireScalarHardwareType || parsed->elementType == "f32" ||
         parsed->elementType == "i64";
}

inline std::set<std::string> BufferAlternatives(const std::string &buffer) {
  std::set<std::string> result;
  if (!startsWith(buffer, "choice(") || buffer.back() != ')') {
    result.insert(buffer);
    return result;
  }
  for (const std::string &item :
       splitTopLevel(buffer.substr(7, buffer.size() - 8))) {
    const std::set<std::string> nested = BufferAlternatives(item);
    result.insert(nested.begin(), nested.end());
  }
  return result;
}

class HIVMOptSinglePointModel {
public:
  explicit HIVMOptSinglePointModel(const BufferizedSemanticIR &inputModule)
      : module(inputModule) {
    for (size_t index = 0; index < module.allocations.size(); ++index)
      allocationTypes[LocalBufferId(index)] = module.allocations[index].type;
    for (const BufferizedOperandAccess &access : module.accesses)
      operationBuffers[access.operationId].push_back(access.bufferId);
    indexCanonicalizedIterArgs();
  }

  HIVMOptSinglePointResult Run() const {
    HIVMOptSinglePointResult result;
    for (const GenericOperation &operation : module.logicalModule.operations) {
      if (module.preBufferizationCSE.erasedOperations.count(operation.id) != 0)
        continue;
      if (isScalarized(operation))
        result.scalarizedOperations.insert(operation.id);
    }
    result.canonicalizedIterArgResults = collectCanonicalizedIterArgResults(
        &result.canonicalizedIterArgResultKeys);
    result.canonicalizedIterArgAccesses = ignoredIterArgAccesses;

    std::set<std::string> required;
    std::map<std::string, int> scalarAvailable;
    for (const GenericOperation &operation : module.logicalModule.operations) {
      if (module.preBufferizationCSE.erasedOperations.count(operation.id) != 0)
        continue;
      const std::vector<std::string> buffers = buffersForOperation(
          operation.id, &ignoredIterArgAccesses);
      if (result.scalarizedOperations.count(operation.id) == 0) {
        if (isNonMaterializing(operation.name))
          continue;
        for (const std::string &buffer : buffers)
          require(buffer, required);
        continue;
      }
      if (operation.name == "tensor.insert") {
        if (!buffers.empty())
          makeScalarAvailable(buffers.back(), operation.blockId,
                              scalarAvailable);
        continue;
      }
      if (operation.name == "tensor.extract") {
        if (!buffers.empty() &&
            !hasScalarAvailable(buffers.front(), operation.blockId,
                                scalarAvailable))
          require(buffers.front(), required);
        continue;
      }
      if (operation.name == "hivm.hir.vbrc") {
        if (!buffers.empty())
          makeScalarAvailable(buffers.back(), operation.blockId,
                              scalarAvailable);
        continue;
      }
      if (buffers.empty())
        continue;
      for (size_t index = 0; index + 1 < buffers.size(); ++index)
        if (!hasScalarAvailable(buffers[index], operation.blockId,
                                scalarAvailable))
          require(buffers[index], required);
      makeScalarAvailable(buffers.back(), operation.blockId, scalarAvailable);
    }

    for (size_t oldIndex = 0; oldIndex < module.allocations.size(); ++oldIndex) {
      const std::string oldId = LocalBufferId(oldIndex);
      if (required.count(oldId) == 0)
        continue;
      const std::string newId = LocalBufferId(result.allocations.size());
      result.bufferMapping[oldId] = newId;
      result.allocations.push_back(module.allocations[oldIndex]);
    }
    return result;
  }

private:
  std::string valueBuffer(int value) const {
    for (const BufferizedValueBinding &binding : module.values)
      if (binding.valueId == value)
        return binding.bufferId;
    return "";
  }

  std::map<std::string, size_t> collectCanonicalizedIterArgResults(
      std::set<std::pair<int, int>> *keys = nullptr) const {
    std::map<std::string, size_t> result;
    for (const GenericOperation &operation : module.logicalModule.operations) {
      if (operation.name == "scf.if") {
        for (size_t index = 0; index < operation.results.size(); ++index) {
          if (valueUseCount(operation.results[index]) == 0) {
            ++result[operation.name];
            if (keys)
              keys->insert({operation.id, static_cast<int>(index)});
            continue;
          }
          bool equivalent = !operation.regions.empty();
          std::optional<int> yieldedValue;
          for (int regionId : operation.regions) {
            const GenericRegion &region = module.logicalModule.regions.at(
                static_cast<size_t>(regionId));
            if (region.blocks.empty()) {
              equivalent = false;
              break;
            }
            const GenericBlock &block = module.logicalModule.blocks.at(
                static_cast<size_t>(region.blocks.back()));
            if (block.operations.empty()) {
              equivalent = false;
              break;
            }
            const GenericOperation &yield =
                module.logicalModule.operations.at(
                    static_cast<size_t>(block.operations.back()));
            if (yield.name != "scf.yield" || index >= yield.operands.size() ||
                (yieldedValue && *yieldedValue != yield.operands[index])) {
              equivalent = false;
              break;
            }
            yieldedValue = yield.operands[index];
          }
          if (equivalent) {
            ++result[operation.name];
            if (keys)
              keys->insert({operation.id, static_cast<int>(index)});
          }
        }
      } else if (operation.name == "scf.for" && !operation.regions.empty()) {
        const GenericRegion &region = module.logicalModule.regions.at(
            static_cast<size_t>(operation.regions.front()));
        if (region.blocks.empty())
          continue;
        const GenericBlock &block = module.logicalModule.blocks.at(
            static_cast<size_t>(region.blocks.front()));
        if (block.operations.empty())
          continue;
        const GenericOperation &yield = module.logicalModule.operations.at(
            static_cast<size_t>(block.operations.back()));
        if (yield.name != "scf.yield")
          continue;
        for (size_t index = 0; index < operation.results.size(); ++index) {
          const bool unchanged =
              index < yield.operands.size() &&
              isIterArgUnchanged(operation, block, yield, index);
          if (unchanged ||
              (valueUseCount(operation.results[index]) == 0 &&
               isIterationIndependent(block, yield, index))) {
            ++result[operation.name];
            if (keys)
              keys->insert({operation.id, static_cast<int>(index)});
          }
        }
      }
    }
    return result;
  }

  size_t valueUseCount(int value) const {
    size_t count = 0;
    for (const GenericOperation &operation : module.logicalModule.operations)
      count += static_cast<size_t>(
          std::count(operation.operands.begin(), operation.operands.end(),
                     value));
    return count;
  }

  bool hasNoSideEffects(const GenericOperation &operation) const {
    if (operation.effects != "none" && !operation.effects.empty())
      return false;
    if (operation.regions.empty())
      return true;
    if (operation.name != "scf.for" && operation.name != "scf.if" &&
        operation.name != "scf.while")
      return false;
    for (int regionId : operation.regions) {
      const GenericRegion &region = module.logicalModule.regions.at(
          static_cast<size_t>(regionId));
      for (int blockId : region.blocks) {
        const GenericBlock &block = module.logicalModule.blocks.at(
            static_cast<size_t>(blockId));
        for (int operationId : block.operations) {
          const GenericOperation &nested = module.logicalModule.operations.at(
              static_cast<size_t>(operationId));
          if (nested.name == "scf.yield" || nested.name == "scf.condition")
            continue;
          if (!hasNoSideEffects(nested))
            return false;
        }
      }
    }
    return true;
  }

  bool isIterationIndependent(const GenericBlock &body,
                              const GenericOperation &allowedYield,
                              size_t resultNumber) const {
    const size_t argumentNumber = resultNumber + 1;
    if (argumentNumber >= body.arguments.size())
      return false;
    const int iterArg = body.arguments[argumentNumber];
    std::vector<int> worklist = {iterArg};
    std::set<int> visitedValues = {iterArg};
    while (!worklist.empty()) {
      const int value = worklist.back();
      worklist.pop_back();
      for (const GenericOperation &user : module.logicalModule.operations) {
        for (size_t operandNumber = 0;
             operandNumber < user.operands.size(); ++operandNumber) {
          if (user.operands[operandNumber] != value)
            continue;
          if (user.name == "scf.yield") {
            if (user.id == allowedYield.id &&
                operandNumber + 1 < body.arguments.size() &&
                body.arguments[operandNumber + 1] == iterArg)
              continue;
            return false;
          }
          if (!hasNoSideEffects(user))
            return false;
          for (int produced : user.results)
            if (visitedValues.insert(produced).second)
              worklist.push_back(produced);
        }
      }
    }
    return true;
  }

  bool isOperationNestedIn(const GenericOperation &operation,
                           const GenericOperation &ancestor) const {
    const GenericOperation *current = &operation;
    while (current && current->parentId >= 0) {
      if (current->parentId == ancestor.id)
        return true;
      current = &module.logicalModule.operations.at(
          static_cast<size_t>(current->parentId));
    }
    return false;
  }

  const GenericOperation *definingOperation(int value,
                                             size_t &resultNumber) const {
    for (const GenericOperation &operation : module.logicalModule.operations)
      for (size_t index = 0; index < operation.results.size(); ++index)
        if (operation.results[index] == value) {
          resultNumber = index;
          return &operation;
        }
    return nullptr;
  }

  const GenericBlock *argumentBlock(int value, size_t &argumentNumber) const {
    for (const GenericBlock &block : module.logicalModule.blocks)
      for (size_t index = 0; index < block.arguments.size(); ++index)
        if (block.arguments[index] == value) {
          argumentNumber = index;
          return &block;
        }
    return nullptr;
  }

  bool traceEquivalentValue(int value, const GenericOperation &outerLoop,
                            std::set<int> &equivalence,
                            std::vector<int> &worklist) const {
    if (equivalence.count(value) != 0)
      return true;
    size_t resultNumber = 0;
    const GenericOperation *definition =
        definingOperation(value, resultNumber);
    const GenericOperation *loop = nullptr;
    size_t iterArgNumber = 0;
    if (definition) {
      if (!isOperationNestedIn(*definition, outerLoop))
        return false;
      if (IsDestinationStyleOp(definition->name)) {
        const std::string root = valueBuffer(value);
        const bool aliasesDestination =
            !root.empty() &&
            std::any_of(definition->dpsInits.begin(),
                        definition->dpsInits.end(), [&](int destination) {
                          return valueBuffer(destination) == root;
                        });
        const bool matchesEquivalence =
            std::any_of(equivalence.begin(), equivalence.end(),
                        [&](int equivalent) {
                          return valueBuffer(equivalent) == root;
                        });
        if (aliasesDestination && matchesEquivalence) {
          equivalence.insert(value);
          return true;
        }
      }
      static const std::set<std::string> bufferizationAliases = {
          "bufferization.to_tensor", "hivm.hir.bitcast", "tensor.cast",
          "tensor.collapse_shape", "tensor.expand_shape",
          "tensor.extract_slice", "tensor.insert", "tensor.insert_slice"};
      if (bufferizationAliases.count(definition->name) != 0) {
        const std::string root = valueBuffer(value);
        const bool matches = !root.empty() &&
            std::any_of(equivalence.begin(), equivalence.end(),
                        [&](int equivalent) {
                          return valueBuffer(equivalent) == root;
                        });
        if (!matches)
          return false;
        equivalence.insert(value);
        return true;
      }
      if (definition->name == "scf.if") {
        equivalence.insert(value);
        for (int regionId : definition->regions) {
          const GenericRegion &region = module.logicalModule.regions.at(
              static_cast<size_t>(regionId));
          if (region.blocks.empty())
            return false;
          const GenericBlock &block = module.logicalModule.blocks.at(
              static_cast<size_t>(region.blocks.front()));
          if (block.operations.empty())
            return false;
          const GenericOperation &yield = module.logicalModule.operations.at(
              static_cast<size_t>(block.operations.back()));
          if (yield.name != "scf.yield" ||
              resultNumber >= yield.operands.size())
            return false;
          worklist.push_back(yield.operands[resultNumber]);
        }
        return true;
      }
      if (definition->name != "scf.for" && definition->name != "scf.while")
        return false;
      loop = definition;
      iterArgNumber = resultNumber;
    } else {
      size_t argumentNumber = 0;
      const GenericBlock *block = argumentBlock(value, argumentNumber);
      if (!block)
        return false;
      const GenericRegion &region = module.logicalModule.regions.at(
          static_cast<size_t>(block->regionId));
      loop = &module.logicalModule.operations.at(
          static_cast<size_t>(region.parentOperation));
      if ((loop->name != "scf.for" && loop->name != "scf.while") ||
          !isOperationNestedIn(*loop, outerLoop))
        return false;
      const size_t offset = loop->name == "scf.for" ? 1 : 0;
      if (argumentNumber < offset)
        return false;
      iterArgNumber = argumentNumber - offset;
    }
    if (!loop || iterArgNumber >= loop->results.size() ||
        loop->regions.empty())
      return false;
    const size_t init = iterArgNumber + (loop->name == "scf.for" ? 3 : 0);
    if (init >= loop->operands.size())
      return false;
    equivalence.insert(loop->results[iterArgNumber]);
    for (int regionId : loop->regions) {
      const GenericRegion &region = module.logicalModule.regions.at(
          static_cast<size_t>(regionId));
      if (region.blocks.empty())
        return false;
      const GenericBlock &block = module.logicalModule.blocks.at(
          static_cast<size_t>(region.blocks.front()));
      const size_t argument =
          iterArgNumber + (loop->name == "scf.for" ? 1 : 0);
      if (argument >= block.arguments.size() || block.operations.empty())
        return false;
      equivalence.insert(block.arguments[argument]);
      const GenericOperation &terminator = module.logicalModule.operations.at(
          static_cast<size_t>(block.operations.back()));
      if (iterArgNumber >= terminator.operands.size())
        return false;
      worklist.push_back(terminator.operands[iterArgNumber]);
    }
    worklist.push_back(loop->operands[init]);
    return true;
  }

  bool isIterArgUnchanged(const GenericOperation &loop,
                          const GenericBlock &body,
                          const GenericOperation &yield,
                          size_t resultNumber) const {
    const size_t init = resultNumber + 3;
    const size_t argument = resultNumber + 1;
    if (init >= loop.operands.size() || argument >= body.arguments.size() ||
        resultNumber >= loop.results.size() ||
        resultNumber >= yield.operands.size())
      return false;
    std::set<int> equivalence = {loop.operands[init], loop.results[resultNumber],
                                 body.arguments[argument]};
    std::vector<int> worklist = {yield.operands[resultNumber]};
    while (!worklist.empty()) {
      const int value = worklist.back();
      worklist.pop_back();
      if (!traceEquivalentValue(value, loop, equivalence, worklist))
        return false;
    }
    return true;
  }

  bool isNonMaterializing(const std::string &name) const {
    static const std::set<std::string> operations = {
        "annotation.mark", "bufferization.to_tensor", "hivm.hir.bitcast",
        "memref.cast", "memref.collapse_shape", "memref.expand_shape",
        "memref.reinterpret_cast", "memref.subview", "memref.view",
        "tensor.cast", "tensor.collapse_shape", "tensor.expand_shape",
        "tensor.extract_slice"};
    return operations.count(name) != 0;
  }

  void indexCanonicalizedIterArgs() {
    for (const GenericOperation &operation : module.logicalModule.operations) {
      if (operation.name != "scf.for" || operation.regions.empty())
        continue;
      const GenericRegion &region = module.logicalModule.regions.at(
          static_cast<size_t>(operation.regions.front()));
      if (region.blocks.empty())
        continue;
      const GenericBlock &block = module.logicalModule.blocks.at(
          static_cast<size_t>(region.blocks.front()));
      if (block.operations.empty())
        continue;
      const GenericOperation &yield = module.logicalModule.operations.at(
          static_cast<size_t>(block.operations.back()));
      if (yield.name != "scf.yield")
        continue;
      for (size_t result = 0; result < operation.results.size(); ++result) {
        const size_t argument = result + 1;
        const size_t init = result + 3;
        if (result >= yield.operands.size() ||
            argument >= block.arguments.size() ||
            init >= operation.operands.size() ||
            yield.operands[result] != block.arguments[argument])
          continue;
        ignoredIterArgAccesses.insert(
            {operation.id, static_cast<int>(init)});
        ignoredIterArgAccesses.insert(
            {yield.id, static_cast<int>(result)});
      }
    }
  }

  std::vector<std::string> buffersForOperation(
      int operation,
      const std::set<std::pair<int, int>> *ignored = nullptr) const {
    std::vector<std::pair<int, std::string>> ordered;
    for (const BufferizedOperandAccess &access : module.accesses) {
      if (access.operationId != operation ||
          (ignored && ignored->count(
                          {access.operationId, access.operandNumber}) != 0))
        continue;
      ordered.push_back({access.operandNumber, access.bufferId});
    }
    std::sort(ordered.begin(), ordered.end());
    std::vector<std::string> result;
    for (const auto &[operand, buffer] : ordered) {
      (void)operand;
      result.push_back(buffer);
    }
    return result;
  }

  void require(const std::string &buffer,
               std::set<std::string> &required) const {
    for (const std::string &alternative : BufferAlternatives(buffer))
      if (startsWith(alternative, "local:"))
        required.insert(alternative);
  }

  bool hasScalarAvailable(const std::string &buffer,
                          int block,
                          const std::map<std::string, int> &available) const {
    const std::set<std::string> alternatives = BufferAlternatives(buffer);
    return !alternatives.empty() &&
           std::all_of(alternatives.begin(), alternatives.end(),
                       [&](const std::string &alternative) {
                         auto found = available.find(alternative);
                         return found != available.end() &&
                                found->second == block;
                       });
  }

  void makeScalarAvailable(const std::string &buffer, int block,
                           std::map<std::string, int> &available) const {
    const std::set<std::string> alternatives = BufferAlternatives(buffer);
    for (const std::string &alternative : alternatives)
      available[alternative] = block;
  }

  bool bufferHasSinglePointType(const std::string &buffer,
                                bool requireScalarHardwareType) const {
    for (const std::string &alternative : BufferAlternatives(buffer)) {
      auto type = allocationTypes.find(alternative);
      if (type == allocationTypes.end() ||
          !IsSinglePointType(type->second, requireScalarHardwareType))
        return false;
    }
    return true;
  }

  bool isScalarized(const GenericOperation &operation) const {
    auto buffers = operationBuffers.find(operation.id);
    if (buffers == operationBuffers.end() || buffers->second.empty())
      return false;
    if (operation.name == "tensor.insert" ||
        operation.name == "tensor.extract")
      return bufferHasSinglePointType(buffers->second.back(), false);
    if (operation.name == "hivm.hir.vbrc")
      return bufferHasSinglePointType(buffers->second.back(), false);
    static const std::set<std::string> elementwise = {
        "hivm.hir.vadd", "hivm.hir.vsub", "hivm.hir.vmul",
        "hivm.hir.vdiv", "hivm.hir.vabs", "hivm.hir.vsqrt",
        "hivm.hir.vmax", "hivm.hir.vmin"};
    return elementwise.count(operation.name) != 0 &&
           bufferHasSinglePointType(buffers->second.back(), true);
  }

  const BufferizedSemanticIR &module;
  std::map<std::string, std::string> allocationTypes;
  std::map<int, std::vector<std::string>> operationBuffers;
  std::set<std::pair<int, int>> ignoredIterArgAccesses;
};

inline HIVMOptSinglePointResult
ModelHIVMOptSinglePoint(const BufferizedSemanticIR &module) {
  return HIVMOptSinglePointModel(module).Run();
}

} // namespace cvub

#endif
