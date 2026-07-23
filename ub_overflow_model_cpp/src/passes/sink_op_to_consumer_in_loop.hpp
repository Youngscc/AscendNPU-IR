#ifndef CVPIPELINE_UB_MODEL_CPP_SINK_OP_TO_CONSUMER_IN_LOOP_HPP
#define CVPIPELINE_UB_MODEL_CPP_SINK_OP_TO_CONSUMER_IN_LOOP_HPP

#include "../ir/post_pipeline_ir_utils.hpp"

namespace cvub {

inline bool IsSinkOpToConsumerCandidate(const GenericOperation &operation) {
  return operation.results.size() == 1 &&
         (operation.name == "hivm.hir.vbrc" ||
          operation.name == "linalg.broadcast" ||
          operation.name == "linalg.fill");
}

inline int EnclosingForOperation(const GenericModule &module,
                                 const GenericOperation &operation) {
  int parent = operation.parentId;
  while (parent >= 0) {
    const GenericOperation &ancestor =
        module.operations.at(static_cast<size_t>(parent));
    if (ancestor.name == "scf.for")
      return parent;
    parent = ancestor.parentId;
  }
  return -1;
}

inline bool IsInsideOutlinedScope(const GenericModule &module,
                                  const GenericOperation &operation) {
  int parent = operation.parentId;
  while (parent >= 0) {
    const GenericOperation &ancestor =
        module.operations.at(static_cast<size_t>(parent));
    if (ancestor.name == "scope.scope" &&
        HasSplitMixDictionaryEntry(ancestor.attributes, "outline"))
      return true;
    parent = ancestor.parentId;
  }
  return false;
}

inline size_t OperationPosition(const GenericModule &module,
                                const GenericOperation &operation) {
  const std::vector<int> &operations =
      module.blocks.at(static_cast<size_t>(operation.blockId)).operations;
  const auto position =
      std::find(operations.begin(), operations.end(), operation.id);
  if (position == operations.end())
    throw std::runtime_error(
        "SinkOpToConsumerInLoop: operation is not attached");
  return static_cast<size_t>(std::distance(operations.begin(), position));
}

struct SinkOpUse {
  int operation = -1;
  size_t operand = 0;
};

inline std::vector<SinkOpUse> SinkOpUses(const GenericModule &module,
                                        int value) {
  std::vector<SinkOpUse> uses;
  for (const GenericBlock &block : module.blocks)
    for (int operationId : block.operations) {
      const GenericOperation &operation =
          module.operations.at(static_cast<size_t>(operationId));
      for (size_t operand = 0; operand < operation.operands.size(); ++operand)
        if (operation.operands[operand] == value)
          uses.push_back({operation.id, operand});
    }
  // OpOperands are linked into MLIR's use list at the front as they are
  // created.  Reversing source order reproduces Value::getUses(), including
  // the observable order of two operands on the same user.
  std::reverse(uses.begin(), uses.end());
  return uses;
}

inline int CloneSinkOpBefore(GenericModule &module, int sourceId,
                             int userId) {
  const GenericOperation user =
      module.operations.at(static_cast<size_t>(userId));
  GenericRewriter rewriter(module);
  const int clone = rewriter.cloneOperation(
      sourceId, user.parentId, user.regionId, user.blockId, {});
  rewriter.insertToBlock(user.blockId, OperationPosition(module, user), clone);
  return clone;
}

inline GenericModule RunSinkOpToConsumerInLoop(GenericModule module) {
  const size_t originalOperationCount = module.operations.size();
  // applyPatternsGreedily seeds its worklist in block order and processes it
  // from the back.  Reverse source order is observable when several
  // independent fills are all cloned immediately before the same loop.
  for (size_t cursor = originalOperationCount; cursor > 0; --cursor) {
    const size_t operationId = cursor - 1;
    const GenericOperation source = module.operations[operationId];
    if (!IsSinkOpToConsumerCandidate(source) || source.blockId < 0)
      continue;
    const int sourceValue = source.results.front();
    const std::vector<SinkOpUse> uses = SinkOpUses(module, sourceValue);
    if (uses.empty())
      continue;

    // SinkOpToConsumerInLoop checks every OpOperand independently.  All users
    // must be in a different block from the producer and nested in some
    // scf.for, but they do not have to belong to the same loop.  In
    // particular, accepting a user merely because its immediate parent is an
    // scf.if over-sinks values that the real pass leaves outside the loop.
    if (std::any_of(uses.begin(), uses.end(), [&](const SinkOpUse &use) {
          const GenericOperation &user = module.operations.at(
              static_cast<size_t>(use.operation));
          return source.blockId == user.blockId ||
                 EnclosingForOperation(module, user) < 0 ||
                 IsInsideOutlinedScope(module, user);
        }))
      continue;

    // Value::getUses() contains one entry per OpOperand.  Preserve that
    // multiplicity and clone once per use, including repeated operands on the
    // same operation.
    for (const SinkOpUse &use : uses) {
      const int clone = CloneSinkOpBefore(module, source.id, use.operation);
      module.operations.at(static_cast<size_t>(use.operation))
          .operands.at(use.operand) =
          module.operations.at(static_cast<size_t>(clone)).results.front();
      ApplyOperationSemantics(
          module.operations.at(static_cast<size_t>(use.operation)));
    }
    if (SinkOpUses(module, sourceValue).empty())
      EraseOperationTree(module, source.id);
  }
  module = CompactGenericModule(std::move(module));
  ApplyOperationSemanticsToAll(module.operations);
  ValidateGenericModule(module);
  return module;
}

} // namespace cvub

#endif
