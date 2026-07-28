#ifndef UB_OVERFLOW_MODEL_CPP_SHADOW_OVERLAY_HPP
#define UB_OVERFLOW_MODEL_CPP_SHADOW_OVERLAY_HPP

#include "generic_rewriter.hpp"
#include "stable_id.hpp"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace cvub {

// A mutation overlay over immutable Generic IR. Base operation payloads are
// borrowed; only structural order and fields actually changed by modeled
// passes are owned. Synthetic nodes live in append-only arenas and IDs are
// never recycled or renumbered during an evaluation.
class GenericShadowOverlay {
public:
  enum Override : uint32_t {
    kOperands = 1U << 0,
    kResultTypes = 1U << 1,
    kAttributes = 1U << 2,
    kEffects = 1U << 3,
    kDpsInputs = 1U << 4,
    kDpsInits = 1U << 5,
  };

  enum class UseKind : uint8_t { Operand, DpsInput, DpsInit };

  struct Use {
    OpId operation;
    size_t index = 0;
    UseKind kind = UseKind::Operand;

    friend bool operator==(const Use &lhs, const Use &rhs) {
      return lhs.operation == rhs.operation && lhs.index == rhs.index &&
             lhs.kind == rhs.kind;
    }
  };

  struct OperationState {
    OpId id;
    OpId projectionSource;
    const GenericOperation *base = nullptr;
    std::optional<GenericOperation> synthetic;
    bool active = true;
    uint32_t overrides = 0;
    OpId parent;
    RegionId region;
    BlockId block;
    std::vector<ValueId> operandOverride;
    std::vector<ValueId> dpsInputOverride;
    std::vector<ValueId> dpsInitOverride;
    std::vector<std::string> resultTypeOverride;
    std::string attributeOverride;
    std::string effectOverride;
  };

  struct RegionState {
    RegionId id;
    OpId parent;
    int ordinal = 0;
    const GenericRegion *base = nullptr;
    bool active = true;
    std::vector<BlockId> blocks;
  };

  struct BlockState {
    BlockId id;
    RegionId region;
    int ordinal = 0;
    const GenericBlock *base = nullptr;
    bool active = true;
    std::vector<ValueId> arguments;
    std::vector<std::string> argumentTypes;
    std::vector<OpId> operations;
  };

  explicit GenericShadowOverlay(const GenericModule &module)
      : baseModule_(&module) {
    operations_.reserve(module.operations.size());
    regions_.reserve(module.regions.size());
    blocks_.reserve(module.blocks.size());

    int maximumValue = -1;
    for (const GenericOperation &operation : module.operations) {
      OperationState state;
      state.id = OpId::fromIndex(static_cast<size_t>(operation.id));
      state.projectionSource =
          operation.projectionSourceId < 0
              ? state.id
              : OpId::fromIndex(
                    static_cast<size_t>(operation.projectionSourceId));
      state.base = &operation;
      state.parent = idOrInvalid<OpId>(operation.parentId);
      state.region = idOrInvalid<RegionId>(operation.regionId);
      state.block = idOrInvalid<BlockId>(operation.blockId);
      operations_.push_back(std::move(state));
      for (int result : operation.results)
        maximumValue = std::max(maximumValue, result);
      for (int operand : operation.operands)
        maximumValue = std::max(maximumValue, operand);
    }
    for (const GenericRegion &region : module.regions) {
      RegionState state;
      state.id = RegionId::fromIndex(static_cast<size_t>(region.id));
      state.parent = idOrInvalid<OpId>(region.parentOperation);
      state.ordinal = region.ordinal;
      state.base = &region;
      for (int block : region.blocks)
        state.blocks.push_back(BlockId::fromIndex(static_cast<size_t>(block)));
      regions_.push_back(std::move(state));
    }
    for (const GenericBlock &block : module.blocks) {
      BlockState state;
      state.id = BlockId::fromIndex(static_cast<size_t>(block.id));
      state.region = RegionId::fromIndex(static_cast<size_t>(block.regionId));
      state.ordinal = block.ordinal;
      state.base = &block;
      for (int argument : block.arguments) {
        state.arguments.push_back(
            ValueId::fromIndex(static_cast<size_t>(argument)));
        maximumValue = std::max(maximumValue, argument);
      }
      state.argumentTypes = block.argumentTypes;
      for (int operation : block.operations)
        state.operations.push_back(
            OpId::fromIndex(static_cast<size_t>(operation)));
      blocks_.push_back(std::move(state));
    }
    nextValue_ = maximumValue < 0
                     ? 0
                     : static_cast<size_t>(maximumValue) + 1;
    rebuildUses();
  }

  size_t baseOperationCount() const {
    return baseModule_ ? baseModule_->operations.size() : 0;
  }
  size_t operationArenaSize() const { return operations_.size(); }
  size_t syntheticOperationCount() const {
    return operations_.size() - baseOperationCount();
  }

  bool isActive(OpId operation) const { return op(operation).active; }
  const OperationState &operation(OpId operation) const {
    return op(operation);
  }
  const BlockState &block(BlockId block) const { return blockState(block); }
  const RegionState &region(RegionId region) const {
    return regionState(region);
  }

  const std::string &name(OpId operation) const {
    return payload(op(operation)).name;
  }

  std::vector<OpId> activeOperations() const {
    std::vector<OpId> result;
    result.reserve(operations_.size());
    for (const OperationState &state : operations_)
      if (state.active)
        result.push_back(state.id);
    return result;
  }

  std::vector<ValueId> results(OpId operation) const {
    std::vector<ValueId> result;
    for (int value : payload(op(operation)).results)
      result.push_back(ValueId::fromIndex(static_cast<size_t>(value)));
    return result;
  }

  std::vector<RegionId> regions(OpId operation) const {
    std::vector<RegionId> result;
    for (int region : payload(op(operation)).regions)
      if (region >= 0 && static_cast<size_t>(region) < regions_.size() &&
          regions_[static_cast<size_t>(region)].active)
        result.push_back(RegionId::fromIndex(static_cast<size_t>(region)));
    return result;
  }

  std::vector<ValueId> operands(OpId operation) const {
    const OperationState &state = op(operation);
    if ((state.overrides & kOperands) != 0)
      return state.operandOverride;
    std::vector<ValueId> result;
    for (int value : payload(state).operands)
      result.push_back(ValueId::fromIndex(static_cast<size_t>(value)));
    return result;
  }

  const std::vector<std::string> &resultTypes(OpId operation) const {
    const OperationState &state = op(operation);
    return (state.overrides & kResultTypes) != 0
               ? state.resultTypeOverride
               : payload(state).resultTypes;
  }

  const std::vector<std::string> &operandTypes(OpId operation) const {
    return payload(op(operation)).operandTypes;
  }

  const std::string &properties(OpId operation) const {
    return payload(op(operation)).properties.get();
  }

  std::vector<ValueId> dpsInputs(OpId operation) const {
    const OperationState &state = op(operation);
    if ((state.overrides & kDpsInputs) != 0)
      return state.dpsInputOverride;
    std::vector<ValueId> result;
    for (int value : payload(state).dpsInputs)
      result.push_back(ValueId::fromIndex(static_cast<size_t>(value)));
    return result;
  }

  std::vector<ValueId> dpsInits(OpId operation) const {
    const OperationState &state = op(operation);
    if ((state.overrides & kDpsInits) != 0)
      return state.dpsInitOverride;
    std::vector<ValueId> result;
    for (int value : payload(state).dpsInits)
      result.push_back(ValueId::fromIndex(static_cast<size_t>(value)));
    return result;
  }

  const std::string &attributes(OpId operation) const {
    const OperationState &state = op(operation);
    if ((state.overrides & kAttributes) != 0)
      return state.attributeOverride;
    return payload(state).attributes.get();
  }

  const std::string &effects(OpId operation) const {
    const OperationState &state = op(operation);
    if ((state.overrides & kEffects) != 0)
      return state.effectOverride;
    return payload(state).effects.get();
  }

  const std::vector<Use> &users(ValueId value) const {
    return value.index() < users_.size() ? users_[value.index()] : emptyUses_;
  }

  OpId createOperation(BlockId block, const std::string &name,
                       const std::vector<std::string> &resultTypes,
                       const std::vector<ValueId> &operands = {},
                       const std::vector<std::string> &operandTypes = {},
                       const std::string &properties = "",
                       const std::string &attributes = "{}") {
    const BlockState &destination = blockState(block);
    GenericOperation synthetic;
    synthetic.id = static_cast<int>(operations_.size());
    synthetic.parentId = toLegacy(regionState(destination.region).parent);
    synthetic.regionId = toLegacy(destination.region);
    synthetic.blockId = toLegacy(block);
    synthetic.name = name;
    synthetic.resultTypes = resultTypes;
    synthetic.operandTypes = operandTypes;
    synthetic.properties = properties;
    synthetic.attributes = attributes;
    for (ValueId value : operands)
      synthetic.operands.push_back(toLegacy(value));
    for (size_t index = 0; index < resultTypes.size(); ++index)
      synthetic.results.push_back(
          static_cast<int>(ValueId::fromIndex(nextValue_++).raw()));

    OperationState state;
    state.id = OpId::fromIndex(operations_.size());
    state.projectionSource = state.id;
    state.synthetic = std::move(synthetic);
    state.parent = regionState(destination.region).parent;
    state.region = destination.region;
    state.block = block;
    operations_.push_back(std::move(state));
    appendToBlock(block, operations_.back().id);
    addOperationUses(operations_.back().id);
    return operations_.back().id;
  }

  RegionId createRegion(OpId parent) {
    OperationState &owner = op(parent);
    RegionState state;
    state.id = RegionId::fromIndex(regions_.size());
    state.parent = parent;
    state.ordinal = static_cast<int>(payload(owner).regions.size());
    regions_.push_back(std::move(state));
    mutablePayload(owner).regions.push_back(
        static_cast<int>(regions_.back().id.raw()));
    return regions_.back().id;
  }

  BlockId createBlock(RegionId region,
                      const std::vector<std::string> &argumentTypes = {}) {
    BlockState state;
    state.id = BlockId::fromIndex(blocks_.size());
    state.region = region;
    state.ordinal = static_cast<int>(regionState(region).blocks.size());
    state.argumentTypes = argumentTypes;
    for (size_t index = 0; index < argumentTypes.size(); ++index)
      state.arguments.push_back(ValueId::fromIndex(nextValue_++));
    blocks_.push_back(std::move(state));
    regionState(region).blocks.push_back(blocks_.back().id);
    return blocks_.back().id;
  }

  void insertToBlock(BlockId block, size_t position, OpId operation) {
    OperationState &state = op(operation);
    if (state.block)
      detachFromBlock(state.block, operation);
    BlockState &destination = blockState(block);
    position = std::min(position, destination.operations.size());
    destination.operations.insert(destination.operations.begin() +
                                      static_cast<std::ptrdiff_t>(position),
                                  operation);
    state.block = block;
    state.region = destination.region;
    state.parent = regionState(destination.region).parent;
    state.active = true;
  }

  void appendToBlock(BlockId block, OpId operation) {
    insertToBlock(block, blockState(block).operations.size(), operation);
  }

  void moveToBlock(OpId operation, BlockId block, size_t position) {
    insertToBlock(block, position, operation);
  }

  void eraseOperation(OpId operation) {
    OperationState &state = op(operation);
    if (!state.active)
      return;
    removeOperationUses(operation);
    if (state.block)
      detachFromBlock(state.block, operation);
    state.active = false;
  }

  void eraseOperationTree(OpId operation) {
    OperationState &state = op(operation);
    const std::vector<int> nestedRegions = payload(state).regions;
    for (int regionId : nestedRegions) {
      RegionState &nestedRegion =
          regionState(RegionId::fromIndex(static_cast<size_t>(regionId)));
      for (BlockId blockId : nestedRegion.blocks) {
        BlockState &nestedBlock = blockState(blockId);
        const std::vector<OpId> children = nestedBlock.operations;
        for (OpId child : children)
          eraseOperationTree(child);
        nestedBlock.active = false;
      }
      nestedRegion.active = false;
    }
    eraseOperation(operation);
  }

  void replaceOperand(OpId operation, size_t operand, ValueId replacement) {
    OperationState &state = op(operation);
    std::vector<ValueId> &values = mutableOperands(state);
    if (operand >= values.size())
      throw std::out_of_range("shadow overlay operand index");
    const ValueId previous = values[operand];
    if (previous == replacement)
      return;
    eraseUse(previous, Use{operation, operand, UseKind::Operand});
    values[operand] = replacement;
    addUse(replacement, Use{operation, operand, UseKind::Operand});
  }

  void replaceAllUses(ValueId from, ValueId to) {
    replaceUsesExcept(from, to, {});
  }

  void replaceUsesExcept(ValueId from, ValueId to,
                         const std::set<OpId> &excluded) {
    if (from == to)
      return;
    const std::vector<Use> snapshot = users(from);
    for (const Use &use : snapshot)
      if (isActive(use.operation) && excluded.count(use.operation) == 0)
        replaceUse(use, to);
  }

  OpId cloneSemanticNode(OpId source, BlockId destination) {
    const OperationState &sourceState = op(source);
    const GenericOperation &sourcePayload = payload(sourceState);
    const OpId projectionSource = sourceState.projectionSource;
    const std::string sourceName = sourcePayload.name;
    const std::vector<std::string> sourceResultTypes = resultTypes(source);
    const std::vector<ValueId> sourceOperands = operands(source);
    const std::vector<std::string> sourceOperandTypes =
        sourcePayload.operandTypes;
    const std::string sourceProperties = sourcePayload.properties;
    const std::string sourceAttributes = attributes(source);
    const std::string sourceEffects = effects(source);
    const std::vector<int> sourceDpsInputs = sourcePayload.dpsInputs;
    const std::vector<int> sourceDpsInits = sourcePayload.dpsInits;
    const std::vector<int> sourceSuccessors = sourcePayload.successors;
    const OpId clone = createOperation(destination, sourceName,
                                       sourceResultTypes, sourceOperands);
    removeOperationUses(clone);
    OperationState &cloneState = op(clone);
    cloneState.projectionSource = projectionSource;
    GenericOperation &clonePayload = mutablePayload(cloneState);
    clonePayload.operandTypes = sourceOperandTypes;
    clonePayload.properties = sourceProperties;
    clonePayload.attributes = sourceAttributes;
    clonePayload.effects = sourceEffects;
    clonePayload.dpsInputs = sourceDpsInputs;
    clonePayload.dpsInits = sourceDpsInits;
    clonePayload.successors = sourceSuccessors;
    addOperationUses(clone);
    return clone;
  }

  void setResultType(OpId operation, size_t result,
                     std::string replacement) {
    OperationState &state = op(operation);
    if ((state.overrides & kResultTypes) == 0) {
      state.resultTypeOverride = payload(state).resultTypes;
      state.overrides |= kResultTypes;
    }
    if (result >= state.resultTypeOverride.size())
      throw std::out_of_range("shadow overlay result type index");
    state.resultTypeOverride[result] = std::move(replacement);
  }

  void setAttributes(OpId operation, std::string replacement) {
    OperationState &state = op(operation);
    state.attributeOverride = std::move(replacement);
    state.overrides |= kAttributes;
  }

  void setEffects(OpId operation, std::string replacement) {
    OperationState &state = op(operation);
    state.effectOverride = std::move(replacement);
    state.overrides |= kEffects;
  }

  // Compatibility boundary for a pass that has migrated to the shadow
  // mutation API while its downstream consumer still expects GenericModule.
  // Stable IDs/tombstones remain untouched during the pass and are compacted
  // once, here, rather than after every mutation wave.
  GenericModule materializeLegacyGenericModule() const {
    if (!baseModule_)
      throw std::runtime_error("shadow overlay has no base module");
    GenericModule result = *baseModule_;
    result.operations.resize(operations_.size());
    result.regions.resize(regions_.size());
    result.blocks.resize(blocks_.size());

    for (const OperationState &state : operations_) {
      GenericOperation operation = payload(state);
      operation.id = toLegacy(state.id);
      operation.projectionSourceId = toLegacy(state.projectionSource);
      operation.parentId = toLegacy(state.parent);
      operation.regionId = toLegacy(state.region);
      operation.blockId = toLegacy(state.block);
      if ((state.overrides & kOperands) != 0) {
        operation.operands.clear();
        for (ValueId value : state.operandOverride)
          operation.operands.push_back(toLegacy(value));
      }
      if ((state.overrides & kResultTypes) != 0)
        operation.resultTypes = state.resultTypeOverride;
      if ((state.overrides & kAttributes) != 0)
        operation.attributes = state.attributeOverride;
      if ((state.overrides & kEffects) != 0)
        operation.effects = state.effectOverride;
      if ((state.overrides & kDpsInputs) != 0) {
        operation.dpsInputs.clear();
        for (ValueId value : state.dpsInputOverride)
          operation.dpsInputs.push_back(toLegacy(value));
      }
      if ((state.overrides & kDpsInits) != 0) {
        operation.dpsInits.clear();
        for (ValueId value : state.dpsInitOverride)
          operation.dpsInits.push_back(toLegacy(value));
      }
      std::vector<int> activeRegions;
      for (int region : operation.regions)
        if (region >= 0 && static_cast<size_t>(region) < regions_.size() &&
            regions_[static_cast<size_t>(region)].active)
          activeRegions.push_back(region);
      operation.regions = std::move(activeRegions);
      result.operations[state.id.index()] = std::move(operation);
    }

    for (const RegionState &state : regions_) {
      GenericRegion region = state.base ? *state.base : GenericRegion{};
      region.id = toLegacy(state.id);
      region.parentOperation = toLegacy(state.parent);
      region.ordinal = state.ordinal;
      region.blocks.clear();
      if (state.active)
        for (BlockId block : state.blocks)
          if (blocks_.at(block.index()).active)
            region.blocks.push_back(toLegacy(block));
      result.regions[state.id.index()] = std::move(region);
    }

    for (const BlockState &state : blocks_) {
      GenericBlock block = state.base ? *state.base : GenericBlock{};
      block.id = toLegacy(state.id);
      block.regionId = toLegacy(state.region);
      block.ordinal = state.ordinal;
      block.arguments.clear();
      for (ValueId argument : state.arguments)
        block.arguments.push_back(toLegacy(argument));
      block.argumentTypes = state.argumentTypes;
      block.operations.clear();
      if (state.active)
        for (OpId operation : state.operations)
          if (operations_.at(operation.index()).active)
            block.operations.push_back(toLegacy(operation));
      result.blocks[state.id.index()] = std::move(block);
    }
    return CompactGenericModule(std::move(result));
  }

private:
  template <typename Id> static Id idOrInvalid(int value) {
    return value < 0 ? Id() : Id::fromIndex(static_cast<size_t>(value));
  }
  template <typename Id> static int toLegacy(Id id) {
    return id ? static_cast<int>(id.raw()) : -1;
  }

  OperationState &op(OpId id) { return operations_.at(id.index()); }
  const OperationState &op(OpId id) const {
    return operations_.at(id.index());
  }
  RegionState &regionState(RegionId id) { return regions_.at(id.index()); }
  const RegionState &regionState(RegionId id) const {
    return regions_.at(id.index());
  }
  BlockState &blockState(BlockId id) { return blocks_.at(id.index()); }
  const BlockState &blockState(BlockId id) const {
    return blocks_.at(id.index());
  }

  static const GenericOperation &payload(const OperationState &state) {
    return state.synthetic ? *state.synthetic : *state.base;
  }
  static GenericOperation &mutablePayload(OperationState &state) {
    if (!state.synthetic)
      state.synthetic = *state.base;
    return *state.synthetic;
  }

  std::vector<ValueId> &mutableOperands(OperationState &state) {
    if ((state.overrides & kOperands) == 0) {
      for (int value : payload(state).operands)
        state.operandOverride.push_back(
            ValueId::fromIndex(static_cast<size_t>(value)));
      state.overrides |= kOperands;
    }
    return state.operandOverride;
  }

  std::vector<ValueId> &mutableDpsInputs(OperationState &state) {
    if ((state.overrides & kDpsInputs) == 0) {
      for (int value : payload(state).dpsInputs)
        state.dpsInputOverride.push_back(
            ValueId::fromIndex(static_cast<size_t>(value)));
      state.overrides |= kDpsInputs;
    }
    return state.dpsInputOverride;
  }

  std::vector<ValueId> &mutableDpsInits(OperationState &state) {
    if ((state.overrides & kDpsInits) == 0) {
      for (int value : payload(state).dpsInits)
        state.dpsInitOverride.push_back(
            ValueId::fromIndex(static_cast<size_t>(value)));
      state.overrides |= kDpsInits;
    }
    return state.dpsInitOverride;
  }

  void replaceUse(const Use &use, ValueId replacement) {
    if (use.kind == UseKind::Operand) {
      replaceOperand(use.operation, use.index, replacement);
      return;
    }
    OperationState &state = op(use.operation);
    std::vector<ValueId> &values =
        use.kind == UseKind::DpsInput ? mutableDpsInputs(state)
                                     : mutableDpsInits(state);
    if (use.index >= values.size())
      throw std::out_of_range("shadow overlay semantic use index");
    const ValueId previous = values[use.index];
    if (previous == replacement)
      return;
    eraseUse(previous, use);
    values[use.index] = replacement;
    addUse(replacement, use);
  }

  void detachFromBlock(BlockId block, OpId operation) {
    BlockState &source = blockState(block);
    const auto position =
        std::find(source.operations.begin(), source.operations.end(), operation);
    if (position != source.operations.end())
      source.operations.erase(position);
    op(operation).block = BlockId();
  }

  void rebuildUses() {
    users_.clear();
    for (const OperationState &state : operations_)
      addOperationUses(state.id);
  }

  void addOperationUses(OpId operation) {
    const std::vector<ValueId> values = operands(operation);
    for (size_t index = 0; index < values.size(); ++index)
      addUse(values[index], Use{operation, index, UseKind::Operand});
    const std::vector<ValueId> inputs = dpsInputs(operation);
    for (size_t index = 0; index < inputs.size(); ++index)
      addUse(inputs[index], Use{operation, index, UseKind::DpsInput});
    const std::vector<ValueId> inits = dpsInits(operation);
    for (size_t index = 0; index < inits.size(); ++index)
      addUse(inits[index], Use{operation, index, UseKind::DpsInit});
  }

  void removeOperationUses(OpId operation) {
    const std::vector<ValueId> values = operands(operation);
    for (size_t index = 0; index < values.size(); ++index)
      eraseUse(values[index], Use{operation, index, UseKind::Operand});
    const std::vector<ValueId> inputs = dpsInputs(operation);
    for (size_t index = 0; index < inputs.size(); ++index)
      eraseUse(inputs[index], Use{operation, index, UseKind::DpsInput});
    const std::vector<ValueId> inits = dpsInits(operation);
    for (size_t index = 0; index < inits.size(); ++index)
      eraseUse(inits[index], Use{operation, index, UseKind::DpsInit});
  }

  void addUse(ValueId value, Use use) {
    if (users_.size() <= value.index())
      users_.resize(value.index() + 1);
    users_[value.index()].push_back(use);
  }

  void eraseUse(ValueId value, Use use) {
    if (value.index() >= users_.size())
      throw std::runtime_error("shadow overlay removing unknown use");
    std::vector<Use> &records = users_[value.index()];
    const auto found = std::find(records.begin(), records.end(), use);
    if (found == records.end())
      throw std::runtime_error("shadow overlay removing unknown use");
    records.erase(found);
  }

  const GenericModule *baseModule_ = nullptr;
  std::vector<OperationState> operations_;
  std::vector<RegionState> regions_;
  std::vector<BlockState> blocks_;
  std::vector<std::vector<Use>> users_;
  std::vector<Use> emptyUses_;
  size_t nextValue_ = 0;
};

} // namespace cvub

#endif
