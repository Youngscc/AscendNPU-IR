// Read-only, stage-scoped indexes over GenericModule.
#ifndef CVPIPELINE_UB_MODEL_CPP_GENERIC_ANALYSIS_HPP
#define CVPIPELINE_UB_MODEL_CPP_GENERIC_ANALYSIS_HPP

#include "generic_ir.hpp"

namespace cvub {

enum GenericAnalysisIndex : unsigned {
  kGenericAnalysisDefinitions = 1U << 0,
  kGenericAnalysisUsers = 1U << 1,
  kGenericAnalysisValueTypes = 1U << 2,
  kGenericAnalysisEnclosingFunctions = 1U << 3,
  kGenericAnalysisFunctionDescendants = 1U << 4,
  kGenericAnalysisAll = (1U << 5) - 1,
};

// Movable, module-independent storage for derived IDs. Keeping the indexes
// separate from a module reference lets immutable pipeline states build them
// once and hand them through several semantic projections without rescanning
// the same GenericModule at every stage boundary.
class GenericModuleAnalysisIndexes {
public:
  GenericModuleAnalysisIndexes() = default;

  explicit GenericModuleAnalysisIndexes(
      const GenericModule &module,
      unsigned requestedAnalysisIndexes = kGenericAnalysisAll) {
    Build(module, requestedAnalysisIndexes);
  }

  void Build(const GenericModule &module,
             unsigned requestedAnalysisIndexes = kGenericAnalysisAll) {
    operationCount = module.operations.size();
    regionCount = module.regions.size();
    blockCount = module.blocks.size();
    requestedIndexes = requestedAnalysisIndexes;
    definingOperationIds.clear();
    usersByValue.clear();
    valueTypes.clear();
    hasValueType.clear();
    enclosingFunctionIds.clear();
    functionDescendants.clear();

    int maximumValue = -1;
    auto observeValue = [&](int value) {
      if (value >= 0)
        maximumValue = std::max(maximumValue, value);
    };
    if (hasIndex(kGenericAnalysisValueTypes))
      for (const GenericBlock &block : module.blocks)
        for (int argument : block.arguments)
          observeValue(argument);
    for (const GenericOperation &operation : module.operations) {
      if (hasIndex(kGenericAnalysisDefinitions) ||
          hasIndex(kGenericAnalysisValueTypes))
        for (int result : operation.results)
          observeValue(result);
      if (hasIndex(kGenericAnalysisUsers))
        for (int operand : operation.operands)
          observeValue(operand);
    }

    const size_t valueCount = maximumValue < 0
                                  ? 0
                                  : static_cast<size_t>(maximumValue) + 1;
    if (hasIndex(kGenericAnalysisDefinitions))
      definingOperationIds.assign(valueCount, -1);
    if (hasIndex(kGenericAnalysisUsers))
      usersByValue.resize(valueCount);
    if (hasIndex(kGenericAnalysisValueTypes)) {
      valueTypes.resize(valueCount);
      hasValueType.assign(valueCount, false);
    }

    if (hasIndex(kGenericAnalysisValueTypes))
      for (const GenericBlock &block : module.blocks) {
        const size_t count =
            std::min(block.arguments.size(), block.argumentTypes.size());
        for (size_t index = 0; index < count; ++index) {
          const int value = block.arguments[index];
          valueTypes[static_cast<size_t>(value)] = block.argumentTypes[index];
          hasValueType[static_cast<size_t>(value)] = true;
        }
      }
    for (const GenericOperation &operation : module.operations) {
      const size_t resultCount =
          std::min(operation.results.size(), operation.resultTypes.size());
      for (size_t index = 0; index < operation.results.size(); ++index) {
        const int value = operation.results[index];
        if (hasIndex(kGenericAnalysisDefinitions))
          definingOperationIds[static_cast<size_t>(value)] = operation.id;
        if (hasIndex(kGenericAnalysisValueTypes) && index < resultCount) {
          valueTypes[static_cast<size_t>(value)] = operation.resultTypes[index];
          hasValueType[static_cast<size_t>(value)] = true;
        }
      }
      if (!hasIndex(kGenericAnalysisUsers))
        continue;
      for (int value : operation.operands)
        if (value >= 0)
          usersByValue[static_cast<size_t>(value)].push_back(operation.id);
    }

    if (hasIndex(kGenericAnalysisEnclosingFunctions)) {
      enclosingFunctionIds.assign(module.operations.size(), -1);
      for (const GenericOperation &operation : module.operations) {
        int current = operation.id;
        size_t remaining = module.operations.size() + 1;
        while (current >= 0 && remaining-- != 0) {
          const GenericOperation &ancestor =
              module.operations.at(static_cast<size_t>(current));
          if (ancestor.name == "func.func") {
            enclosingFunctionIds[static_cast<size_t>(operation.id)] = current;
            break;
          }
          current = ancestor.parentId;
        }
      }
    }

    if (hasIndex(kGenericAnalysisFunctionDescendants))
      for (const GenericOperation &operation : module.operations) {
        if (operation.name != "func.func")
          continue;
        std::vector<int> descendants;
        collectDescendants(module, operation, descendants);
        functionDescendants.emplace(operation.id, std::move(descendants));
      }
  }

  void ensureCompatible(const GenericModule &module) const {
    if (module.operations.size() != operationCount ||
        module.regions.size() != regionCount ||
        module.blocks.size() != blockCount)
      throw std::runtime_error(
          "GenericModuleAnalysisIndexes used after module mutation");
  }

  int definingOperationId(int value) const {
    requireIndex(kGenericAnalysisDefinitions);
    if (value < 0 || static_cast<size_t>(value) >= definingOperationIds.size())
      return -1;
    return definingOperationIds[static_cast<size_t>(value)];
  }

  const std::vector<int> &users(int value) const {
    requireIndex(kGenericAnalysisUsers);
    if (value < 0 || static_cast<size_t>(value) >= usersByValue.size())
      return emptyOperationIds;
    return usersByValue[static_cast<size_t>(value)];
  }

  const std::string *valueType(int value) const {
    requireIndex(kGenericAnalysisValueTypes);
    if (value < 0 || static_cast<size_t>(value) >= valueTypes.size() ||
        !hasValueType[static_cast<size_t>(value)])
      return nullptr;
    return &valueTypes[static_cast<size_t>(value)];
  }

  size_t valueCount() const {
    requireIndex(kGenericAnalysisValueTypes);
    return valueTypes.size();
  }

  int enclosingFunctionId(int operation) const {
    requireIndex(kGenericAnalysisEnclosingFunctions);
    if (operation < 0 ||
        static_cast<size_t>(operation) >= enclosingFunctionIds.size())
      return -1;
    return enclosingFunctionIds[static_cast<size_t>(operation)];
  }

  const std::vector<int> &descendants(int function) const {
    requireIndex(kGenericAnalysisFunctionDescendants);
    auto found = functionDescendants.find(function);
    return found == functionDescendants.end() ? emptyOperationIds
                                              : found->second;
  }

  bool provides(unsigned indexes) const {
    return (requestedIndexes & indexes) == indexes;
  }

private:
  bool hasIndex(GenericAnalysisIndex index) const {
    return (requestedIndexes & static_cast<unsigned>(index)) != 0;
  }

  void requireIndex(GenericAnalysisIndex index) const {
    if (!hasIndex(index))
      throw std::runtime_error(
          "GenericModuleAnalysisIndexes queried an unrequested index");
  }

  void collectDescendants(const GenericModule &module,
                          const GenericOperation &operation,
                          std::vector<int> &result) const {
    for (int regionId : operation.regions)
      for (int blockId :
           module.regions.at(static_cast<size_t>(regionId)).blocks)
        for (int childId :
             module.blocks.at(static_cast<size_t>(blockId)).operations) {
          result.push_back(childId);
          collectDescendants(
              module, module.operations.at(static_cast<size_t>(childId)),
              result);
        }
  }

  size_t operationCount = 0;
  size_t regionCount = 0;
  size_t blockCount = 0;
  unsigned requestedIndexes = 0;
  std::vector<int> definingOperationIds;
  std::vector<std::vector<int>> usersByValue;
  std::vector<std::string> valueTypes;
  std::vector<bool> hasValueType;
  std::vector<int> enclosingFunctionIds;
  std::map<int, std::vector<int>> functionDescendants;
  std::vector<int> emptyOperationIds;
};

// Mutation-aware index for passes that only rewrite operation operands while
// preserving the operation/value tables.  Such passes previously rescanned
// every operation for each deadness query.  Call replaceUse together with the
// operand mutation to keep O(1) hasUsers queries exact without constraining
// rewrite order.
class GenericMutableOperandUseIndex {
public:
  GenericMutableOperandUseIndex() = default;

  explicit GenericMutableOperandUseIndex(const GenericModule &module) {
    Build(module);
  }

  void Build(const GenericModule &module) {
    size_t valueCount = 0;
    for (const GenericOperation &operation : module.operations)
      for (int operand : operation.operands)
        if (operand >= 0)
          valueCount = std::max(valueCount,
                                static_cast<size_t>(operand) + 1);
    uses.assign(valueCount, 0);
    for (const GenericOperation &operation : module.operations)
      for (int operand : operation.operands)
        addUse(operand);
  }

  void BuildActive(const GenericModule &module) {
    size_t valueCount = 0;
    for (const GenericBlock &block : module.blocks)
      for (int operationId : block.operations)
        for (int operand : module.operations
                               .at(static_cast<size_t>(operationId))
                               .operands)
          if (operand >= 0)
            valueCount = std::max(valueCount,
                                  static_cast<size_t>(operand) + 1);
    uses.assign(valueCount, 0);
    for (const GenericBlock &block : module.blocks)
      for (int operationId : block.operations)
        for (int operand : module.operations
                               .at(static_cast<size_t>(operationId))
                               .operands)
          addUse(operand);
  }

  void clear() { uses.clear(); }

  bool hasUsers(int value) const {
    return value >= 0 && static_cast<size_t>(value) < uses.size() &&
           uses[static_cast<size_t>(value)] != 0;
  }

  size_t useCount(int value) const {
    return value < 0 || static_cast<size_t>(value) >= uses.size()
               ? 0
               : uses[static_cast<size_t>(value)];
  }

  void addUse(int value) {
    if (value < 0)
      return;
    const size_t index = static_cast<size_t>(value);
    if (index >= uses.size())
      uses.resize(index + 1, 0);
    ++uses[index];
  }

  void removeUse(int value) {
    if (value < 0 || static_cast<size_t>(value) >= uses.size() ||
        uses[static_cast<size_t>(value)] == 0)
      throw std::runtime_error(
          "GenericMutableOperandUseIndex: removing unknown use");
    --uses[static_cast<size_t>(value)];
  }

  void replaceUse(int oldValue, int newValue) {
    if (oldValue == newValue)
      return;
    removeUse(oldValue);
    addUse(newValue);
  }

private:
  std::vector<size_t> uses;
};

enum class GenericOperationKind {
  Other,
  Module,
  Function,
  Return,
  Constant,
  For,
  While,
  If,
  Yield,
  Condition,
  AffineApply,
  AffineMin,
  AffineMax,
  TensorEmpty,
  ExtractSlice,
  InsertSlice,
  Load,
  Store,
  Copy,
};

inline GenericOperationKind ClassifyGenericOperation(
    const std::string &name) {
  if (name == "builtin.module")
    return GenericOperationKind::Module;
  if (name == "func.func")
    return GenericOperationKind::Function;
  if (name == "func.return")
    return GenericOperationKind::Return;
  if (name == "arith.constant")
    return GenericOperationKind::Constant;
  if (name == "scf.for")
    return GenericOperationKind::For;
  if (name == "scf.while")
    return GenericOperationKind::While;
  if (name == "scf.if")
    return GenericOperationKind::If;
  if (name == "scf.yield" || name == "affine.yield")
    return GenericOperationKind::Yield;
  if (name == "scf.condition")
    return GenericOperationKind::Condition;
  if (name == "affine.apply")
    return GenericOperationKind::AffineApply;
  if (name == "affine.min")
    return GenericOperationKind::AffineMin;
  if (name == "affine.max")
    return GenericOperationKind::AffineMax;
  if (name == "tensor.empty")
    return GenericOperationKind::TensorEmpty;
  if (name == "tensor.extract_slice")
    return GenericOperationKind::ExtractSlice;
  if (name == "tensor.insert_slice")
    return GenericOperationKind::InsertSlice;
  if (name == "hivm.hir.load" || name == "memref.load")
    return GenericOperationKind::Load;
  if (name == "hivm.hir.store" || name == "memref.store")
    return GenericOperationKind::Store;
  if (name == "hivm.hir.copy")
    return GenericOperationKind::Copy;
  return GenericOperationKind::Other;
}

// Pass-scoped analysis owner that combines immutable derived indexes with an
// incrementally maintained operand-use index. GenericRewriter notifications
// keep cheap queries current and lazily invalidate only analyses whose source
// relation may have changed.
class PipelineAnalysisContext final : public GenericMutationListener {
public:
  explicit PipelineAnalysisContext(
      GenericModule &inputModule,
      unsigned requestedAnalysisIndexes = kGenericAnalysisAll)
      : module(inputModule), requestedIndexes(requestedAnalysisIndexes),
        indexes(inputModule, requestedAnalysisIndexes) {
    rebuildMutableIndexes();
  }

  int definingOperationId(int value) const {
    ensureIndexes();
    return indexes.definingOperationId(value);
  }

  const std::vector<int> &users(int value) const {
    ensureUsers();
    if (value < 0 || static_cast<size_t>(value) >= usersByValue.size())
      return emptyUsers;
    return usersByValue[static_cast<size_t>(value)];
  }

  const std::vector<int> *replacementUsers(int value) const override {
    ensureUsers();
    if (value < 0 || static_cast<size_t>(value) >= usersByValue.size())
      return &emptyUsers;
    return &usersByValue[static_cast<size_t>(value)];
  }

  const std::string *valueType(int value) const {
    ensureIndexes();
    return indexes.valueType(value);
  }

  int enclosingFunctionId(int operation) const {
    ensureIndexes();
    return indexes.enclosingFunctionId(operation);
  }

  const std::vector<int> &descendants(int function) const {
    ensureIndexes();
    return indexes.descendants(function);
  }

  bool hasUsers(int value) const {
    ensureUses();
    return uses.hasUsers(value);
  }

  size_t useCount(int value) const {
    ensureUses();
    return uses.useCount(value);
  }

  GenericOperationKind operationKind(int operation) const {
    if (operation < 0 ||
        static_cast<size_t>(operation) >= module.operations.size())
      return GenericOperationKind::Other;
    const size_t index = static_cast<size_t>(operation);
    if (index >= operationKinds.size())
      rebuildOperationKinds();
    if (operationKindDirty[index]) {
      operationKinds[index] =
          ClassifyGenericOperation(module.operations[index].name);
      operationKindDirty[index] = false;
    }
    return operationKinds[index];
  }

  void operationCreated(const GenericOperation &operation) override {
    indexesDirty = true;
    if (!usesDirty)
      for (int operand : operation.operands)
        uses.addUse(operand);
    if (!usersDirty)
      for (int operand : operation.operands)
        addUser(operand, operation.id);
    const size_t required = static_cast<size_t>(operation.id) + 1;
    if (operationKinds.size() < required) {
      operationKinds.resize(required, GenericOperationKind::Other);
      operationKindDirty.resize(required, false);
    }
    operationKinds[static_cast<size_t>(operation.id)] =
        ClassifyGenericOperation(operation.name);
  }

  void operationWillModify(const GenericOperation &operation) override {
    indexesDirty = true;
    usesDirty = true;
    usersDirty = true;
    const size_t index = static_cast<size_t>(operation.id);
    if (operationKindDirty.size() <= index)
      operationKindDirty.resize(index + 1, true);
    operationKindDirty[index] = true;
  }

  void operandReplaced(int operation, size_t, int oldValue,
                       int newValue) override {
    indexesDirty = true;
    if (!usesDirty) {
      if (uses.useCount(oldValue) == 0)
        usesDirty = true;
      else
        uses.replaceUse(oldValue, newValue);
    }
    if (!usersDirty) {
      const bool tracked =
          oldValue >= 0 &&
          static_cast<size_t>(oldValue) < usersByValue.size() &&
          std::find(usersByValue[static_cast<size_t>(oldValue)].begin(),
                    usersByValue[static_cast<size_t>(oldValue)].end(),
                    operation) !=
              usersByValue[static_cast<size_t>(oldValue)].end();
      if (!tracked)
        usersDirty = true;
      else {
        removeUser(oldValue, operation);
        addUser(newValue, operation);
      }
    }
  }

  void operationMoved(int operation, int oldBlock, int newBlock) override {
    indexesDirty = true;
    if ((oldBlock < 0) == (newBlock < 0))
      return;
    const GenericOperation &record =
        module.operations.at(static_cast<size_t>(operation));
    if (oldBlock >= 0) {
      std::map<int, size_t> removedUses;
      for (int operand : record.operands)
        ++removedUses[operand];
      if (!usesDirty) {
        for (const auto &[operand, count] : removedUses)
          if (uses.useCount(operand) < count) {
            usesDirty = true;
            break;
          }
        if (!usesDirty)
          for (int operand : record.operands)
            uses.removeUse(operand);
      }
      if (!usersDirty) {
        for (const auto &[operand, count] : removedUses) {
          const size_t tracked =
              operand < 0 ||
                      static_cast<size_t>(operand) >= usersByValue.size()
                  ? 0
                  : static_cast<size_t>(std::count(
                        usersByValue[static_cast<size_t>(operand)].begin(),
                        usersByValue[static_cast<size_t>(operand)].end(),
                        operation));
          if (tracked < count) {
            usersDirty = true;
            break;
          }
        }
        if (!usersDirty)
          for (int operand : record.operands)
            removeUser(operand, operation);
      }
      return;
    }
    if (!usesDirty)
      for (int operand : record.operands)
        uses.addUse(operand);
    if (!usersDirty)
      for (int operand : record.operands)
        addUser(operand, operation);
  }
  void regionCreated(const GenericRegion &) override { indexesDirty = true; }
  void blockCreated(const GenericBlock &) override { indexesDirty = true; }

private:
  void ensureIndexes() const {
    if (!indexesDirty)
      return;
    indexes.Build(module, requestedIndexes);
    indexesDirty = false;
  }

  void ensureUses() const {
    if (!usesDirty)
      return;
    uses.BuildActive(module);
    usesDirty = false;
  }

  void ensureUsers() const {
    if (!usersDirty)
      return;
    rebuildUsers();
    usersDirty = false;
  }

  void rebuildUsers() const {
    size_t valueCount = 0;
    for (const GenericBlock &block : module.blocks)
      for (int operationId : block.operations)
        for (int operand : module.operations
                               .at(static_cast<size_t>(operationId))
                               .operands)
          if (operand >= 0)
            valueCount = std::max(valueCount,
                                  static_cast<size_t>(operand) + 1);
    usersByValue.assign(valueCount, {});
    for (const GenericBlock &block : module.blocks)
      for (int operationId : block.operations)
        for (int operand : module.operations
                               .at(static_cast<size_t>(operationId))
                               .operands)
          addUser(operand, operationId);
  }

  void rebuildMutableIndexes() const {
    uses.clear();
    usersByValue.clear();
    operationKinds.resize(module.operations.size());
    operationKindDirty.assign(module.operations.size(), false);
    for (const GenericOperation &operation : module.operations)
      operationKinds[static_cast<size_t>(operation.id)] =
          ClassifyGenericOperation(operation.name);
    for (const GenericBlock &block : module.blocks)
      for (int operationId : block.operations)
        for (int operand : module.operations
                               .at(static_cast<size_t>(operationId))
                               .operands) {
          uses.addUse(operand);
          addUser(operand, operationId);
        }
  }

  void addUser(int value, int operation) const {
    if (value < 0)
      return;
    const size_t index = static_cast<size_t>(value);
    if (usersByValue.size() <= index)
      usersByValue.resize(index + 1);
    std::vector<int> &users = usersByValue[index];
    users.insert(std::upper_bound(users.begin(), users.end(), operation),
                 operation);
  }

  void removeUser(int value, int operation) const {
    if (value < 0 || static_cast<size_t>(value) >= usersByValue.size())
      throw std::runtime_error(
          "PipelineAnalysisContext: removing unknown user");
    std::vector<int> &users = usersByValue[static_cast<size_t>(value)];
    const auto found = std::find(users.begin(), users.end(), operation);
    if (found == users.end())
      throw std::runtime_error(
          "PipelineAnalysisContext: removing unknown user");
    users.erase(found);
  }

  void rebuildOperationKinds() const {
    operationKinds.resize(module.operations.size());
    operationKindDirty.assign(module.operations.size(), false);
    for (const GenericOperation &operation : module.operations)
      operationKinds[static_cast<size_t>(operation.id)] =
          ClassifyGenericOperation(operation.name);
  }

  GenericModule &module;
  unsigned requestedIndexes = kGenericAnalysisAll;
  mutable GenericModuleAnalysisIndexes indexes;
  mutable GenericMutableOperandUseIndex uses;
  mutable std::vector<std::vector<int>> usersByValue;
  mutable std::vector<int> emptyUsers;
  mutable std::vector<GenericOperationKind> operationKinds;
  mutable std::vector<bool> operationKindDirty;
  mutable bool indexesDirty = false;
  mutable bool usesDirty = false;
  mutable bool usersDirty = false;
};

// Convenience view for pass-local analyses. Immutable pipeline states should
// retain GenericModuleAnalysisIndexes directly when several stages consume
// the same module.
class GenericModuleAnalysisSnapshot {
public:
  explicit GenericModuleAnalysisSnapshot(
      const GenericModule &inputModule,
      unsigned requestedAnalysisIndexes = kGenericAnalysisAll)
      : module(inputModule), indexes(inputModule, requestedAnalysisIndexes) {}

  const GenericOperation *definingOperation(int value) const {
    indexes.ensureCompatible(module);
    const int operation = indexes.definingOperationId(value);
    return operation < 0
               ? nullptr
               : &module.operations.at(static_cast<size_t>(operation));
  }

  const std::vector<int> &users(int value) const {
    indexes.ensureCompatible(module);
    return indexes.users(value);
  }

  const std::string *valueType(int value) const {
    indexes.ensureCompatible(module);
    return indexes.valueType(value);
  }

  size_t valueCount() const {
    indexes.ensureCompatible(module);
    return indexes.valueCount();
  }

  const GenericOperation *
  enclosingFunction(const GenericOperation &operation) const {
    indexes.ensureCompatible(module);
    const int function = indexes.enclosingFunctionId(operation.id);
    return function < 0
               ? nullptr
               : &module.operations.at(static_cast<size_t>(function));
  }

  const std::vector<int> &
  descendants(const GenericOperation &function) const {
    indexes.ensureCompatible(module);
    return indexes.descendants(function.id);
  }

private:
  const GenericModule &module;
  GenericModuleAnalysisIndexes indexes;
};

} // namespace cvub

#endif
