// Lightweight equivalent of MLIR Liveness for PlanMemory-visible semantics.
#ifndef CVPIPELINE_UB_MODEL_CPP_LIVENESS_HPP
#define CVPIPELINE_UB_MODEL_CPP_LIVENESS_HPP

#include "buffer_alias.hpp"

namespace cvub {

struct ValueLivenessInfo {
  bool blockArgument = false;
  int definingOperationId = -1;
  std::vector<int> definingBlock;
  std::vector<int> users;
};

struct BlockLivenessInfo {
  std::vector<int> path;
  std::vector<int> operations;
  std::vector<std::string> arguments;
  std::vector<std::string> liveIns;
};

struct CFGBlockLivenessInfo {
  int id = 0;
  std::vector<int> path;
  std::string label;
  std::vector<int> operations;
  std::vector<std::string> arguments;
  std::vector<std::string> uses;
  std::vector<std::string> definitions;
  std::vector<std::string> liveIns;
  std::vector<std::string> liveOuts;
  std::vector<int> successors;
};

class Liveness {
public:
  Liveness(const std::vector<OperationRecord> &linearOperations,
                const std::vector<std::string> &functionArguments) {
    for (const OperationRecord &op : linearOperations) {
      if (!operationById.count(op.operationId)) {
        operationById[op.operationId] = op;
        operationOrder.push_back(op.operationId);
        blocks[op.regionPath].path = op.regionPath;
        blocks[op.regionPath].operations.push_back(op.operationId);
      }
    }
    blocks[{}].path = {};
    for (int operationId : operationOrder) {
      const OperationRecord &op = operationById.at(operationId);
      for (size_t depth = 0; depth < op.regionPath.size(); ++depth) {
        int region = op.regionPath[depth];
        if (regionOwner.count(region))
          continue;
        using Difference = std::vector<int>::difference_type;
        std::vector<int> parentPath(
            op.regionPath.begin(),
            std::next(op.regionPath.begin(), static_cast<Difference>(depth)));
        int owner = -1;
        for (int candidateId : blocks[parentPath].operations) {
          const OperationRecord &candidate = operationById.at(candidateId);
          if (candidate.index >= op.index)
            break;
          if (candidate.opName == "scf.for" ||
              candidate.opName == "scf.if" ||
              candidate.opName == "scf.while" ||
              candidate.opName == "scope.scope")
            owner = candidateId;
        }
        if (owner >= 0)
          regionOwner[region] = owner;
      }
    }
    for (const std::string &argument : functionArguments)
      addBlockArgument({}, argument);

    for (const OperationRecord &op : linearOperations) {
      if (operationById.at(op.operationId).index != op.index)
        continue;
      for (const std::string &argument : op.blockArguments)
        addBlockArgument(op.regionPath, argument);
      if (op.opName != "scf.for" && op.opName != "scf.while")
        continue;
      std::vector<int> child = childPath(op);
      if (op.opName == "scf.for") {
        if (auto induction = extractForInductionVariable(op.text))
          addBlockArgument(child, *induction);
        for (const auto &pair : extractIterArgPairs(op.text))
          addBlockArgument(child, pair.first);
      } else {
        for (const auto &pair : extractWhileInitArgPairs(op.text))
          addBlockArgument(child, pair.first);
      }
    }

    for (int operationId : operationOrder) {
      const OperationRecord &op = operationById.at(operationId);
      for (const std::string &result : operationResultNames(op)) {
        ValueLivenessInfo &value = values[result];
        value.definingOperationId = operationId;
        value.definingBlock = op.regionPath;
      }
      for (const std::string &operand : operationOperandNames(op)) {
        ensureExternalValue(operand);
        values[operand].users.push_back(operationId);
      }
    }
    buildLiveIns();
    buildCFGLiveness(functionArguments);
  }

  const OperationRecord &operation(int operationId) const {
    return operationById.at(operationId);
  }

  const ValueLivenessInfo *value(const std::string &name) const {
    auto it = values.find(name);
    return it == values.end() ? nullptr : &it->second;
  }

  const BlockLivenessInfo &block(const std::vector<int> &path) const {
    return blocks.at(path);
  }

  std::vector<std::string> currentlyLiveValuesOrdered(int operationId) const {
    const OperationRecord &query = operation(operationId);
    if (hasMultipleBlocks(query.regionPath))
      return currentlyLiveValuesOrderedCFG(operationId);
    const BlockLivenessInfo &blockInfo = block(query.regionPath);
    std::vector<std::string> result;
    auto addIfLive = [&](const std::string &name) {
      if (isCurrentlyLive(name, operationId, blockInfo) &&
          std::find(result.begin(), result.end(), name) == result.end())
        result.push_back(name);
    };
    for (const std::string &argument : blockInfo.arguments)
      addIfLive(argument);
    for (const std::string &liveIn : blockInfo.liveIns)
      addIfLive(liveIn);
    for (int blockOperationId : blockInfo.operations) {
      for (const std::string &opResult :
           operationResultNames(operation(blockOperationId)))
        addIfLive(opResult);
      if (blockOperationId == operationId)
        break;
    }
    return result;
  }

  bool isDeadAfter(const std::string &name, int operationId) const {
    const ValueLivenessInfo *valueInfo = value(name);
    if (!valueInfo)
      return true;
    const OperationRecord &query = operation(operationId);
    if (hasMultipleBlocks(query.regionPath))
      return isDeadAfterCFG(name, operationId);
    int queryOrder = orderInBlock(operationId, query.regionPath);
    for (int userId : valueInfo->users) {
      std::optional<int> projected = projectToBlock(userId, query.regionPath);
      if (projected && orderInBlock(*projected, query.regionPath) > queryOrder)
        return false;
    }
    return true;
  }

  bool isDeadAfterOp(const std::string &name, int operationId) const {
    const ValueLivenessInfo *valueInfo = value(name);
    if (!valueInfo)
      return true;
    const OperationRecord &query = operation(operationId);
    if (hasMultipleBlocks(query.regionPath)) {
      const CFGBlockLivenessInfo &queryBlock = cfgBlocks.at(query.blockId);
      int queryOrder = orderInCFGBlock(operationId, query.blockId);
      for (int userId : valueInfo->users) {
        const OperationRecord &user = operation(userId);
        if (user.regionPath != query.regionPath)
          continue;
        if (user.blockId == queryBlock.id &&
            orderInCFGBlock(userId, user.blockId) > queryOrder)
          return false;
      }
    }
    for (int userId : valueInfo->users) {
      const OperationRecord &user = operation(userId);
      // PlanMemory::IsDeadAfterOp only matches blocks on the current
      // operation's ancestor chain. A user in a sibling CFG block is on a
      // mutually exclusive path and does not make the value live after this
      // operation.
      if (hasMultipleBlocks(query.regionPath) &&
          user.regionPath == query.regionPath &&
          user.blockId != query.blockId)
        continue;
      size_t common = 0;
      while (common < query.regionPath.size() &&
             common < user.regionPath.size() &&
             query.regionPath[common] == user.regionPath[common])
        ++common;
      using Difference = std::vector<int>::difference_type;
      std::vector<int> commonPath(
          query.regionPath.begin(),
          std::next(query.regionPath.begin(),
                    static_cast<Difference>(common)));
      int queryProjected = projectAtCommonBlock(operationId, commonPath);
      int userProjected = projectAtCommonBlock(userId, commonPath);
      if (queryProjected == userProjected && common < query.regionPath.size() &&
          common < user.regionPath.size() &&
          user.regionPath[common] > query.regionPath[common])
        return false;
      if (orderInBlock(userProjected, commonPath) >
          orderInBlock(queryProjected, commonPath))
        return false;
    }
    return true;
  }

  bool definingParentDominatedBy(int definingOperationId,
                                 int currentOperationId) const {
    return pathPrefix(operation(currentOperationId).regionPath,
                      operation(definingOperationId).regionPath);
  }

private:
  template <typename T>
  static void insertUnique(std::vector<T> &values, const T &value) {
    if (std::find(values.begin(), values.end(), value) == values.end())
      values.push_back(value);
  }

  bool hasMultipleBlocks(const std::vector<int> &path) const {
    auto it = regionBlocks.find(path);
    return it != regionBlocks.end() && it->second.size() > 1;
  }

  int orderInCFGBlock(int operationId, int blockId) const {
    const std::vector<int> &operations = cfgBlocks.at(blockId).operations;
    auto it = std::find(operations.begin(), operations.end(), operationId);
    return it == operations.end()
               ? -1
               : static_cast<int>(std::distance(operations.begin(), it));
  }

  bool isCurrentlyLiveCFG(const std::string &name, int operationId,
                          const CFGBlockLivenessInfo &blockInfo) const {
    const ValueLivenessInfo *valueInfo = value(name);
    if (!valueInfo)
      return false;
    int queryOrder = orderInCFGBlock(operationId, blockInfo.id);
    bool liveIn = std::find(blockInfo.liveIns.begin(), blockInfo.liveIns.end(),
                            name) != blockInfo.liveIns.end();
    bool blockArgument =
        std::find(blockInfo.arguments.begin(), blockInfo.arguments.end(), name) !=
        blockInfo.arguments.end();
    int start = 0;
    if (!liveIn && !blockArgument) {
      if (valueInfo->definingOperationId < 0 ||
          operation(valueInfo->definingOperationId).blockId != blockInfo.id)
        return false;
      start = orderInCFGBlock(valueInfo->definingOperationId, blockInfo.id);
    }
    int end = start;
    for (int userId : valueInfo->users) {
      if (operation(userId).blockId == blockInfo.id)
        end = std::max(end, orderInCFGBlock(userId, blockInfo.id));
    }
    if (std::find(blockInfo.liveOuts.begin(), blockInfo.liveOuts.end(), name) !=
        blockInfo.liveOuts.end())
      end = static_cast<int>(blockInfo.operations.size()) - 1;
    return start <= queryOrder && queryOrder <= end;
  }

  std::vector<std::string>
  currentlyLiveValuesOrderedCFG(int operationId) const {
    const OperationRecord &query = operation(operationId);
    const CFGBlockLivenessInfo &blockInfo = cfgBlocks.at(query.blockId);
    std::vector<std::string> result;
    auto addIfLive = [&](const std::string &name) {
      if (isCurrentlyLiveCFG(name, operationId, blockInfo))
        insertUnique(result, name);
    };
    for (const std::string &argument : blockInfo.arguments)
      addIfLive(argument);
    for (const std::string &liveIn : blockInfo.liveIns)
      addIfLive(liveIn);
    for (int blockOperationId : blockInfo.operations) {
      for (const std::string &opResult :
           operationResultNames(operation(blockOperationId)))
        addIfLive(opResult);
      if (blockOperationId == operationId)
        break;
    }
    return result;
  }

  bool isDeadAfterCFG(const std::string &name, int operationId) const {
    const OperationRecord &query = operation(operationId);
    const CFGBlockLivenessInfo &blockInfo = cfgBlocks.at(query.blockId);
    int queryOrder = orderInCFGBlock(operationId, query.blockId);
    const ValueLivenessInfo *valueInfo = value(name);
    for (int userId : valueInfo->users) {
      const OperationRecord &user = operation(userId);
      if (user.blockId == query.blockId &&
          orderInCFGBlock(userId, query.blockId) > queryOrder)
        return false;
    }
    return std::find(blockInfo.liveOuts.begin(), blockInfo.liveOuts.end(),
                     name) == blockInfo.liveOuts.end();
  }

  void buildCFGLiveness(const std::vector<std::string> &functionArguments) {
    std::map<std::pair<std::vector<int>, std::string>, int> labelToBlock;
    for (int operationId : operationOrder) {
      const OperationRecord &op = operation(operationId);
      CFGBlockLivenessInfo &block = cfgBlocks[op.blockId];
      block.id = op.blockId;
      block.path = op.regionPath;
      block.label = op.blockLabel;
      block.operations.push_back(operationId);
      for (const std::string &argument : op.blockArguments)
        insertUnique(block.arguments, argument);
      insertUnique(regionBlocks[op.regionPath], op.blockId);
      labelToBlock[{op.regionPath, op.blockLabel}] = op.blockId;
    }
    for (const std::string &argument : functionArguments) {
      for (int operationId : operationOrder) {
        const OperationRecord &op = operation(operationId);
        if (op.regionPath.empty()) {
          insertUnique(cfgBlocks[op.blockId].arguments, argument);
          break;
        }
      }
    }
    for (int operationId : operationOrder) {
      const OperationRecord &op = operation(operationId);
      if (op.opName != "scf.for" && op.opName != "scf.while")
        continue;
      std::vector<int> child = childPath(op);
      int childBlock = -1;
      for (int candidateId : operationOrder) {
        const OperationRecord &candidate = operation(candidateId);
        if (candidate.regionPath == child) {
          childBlock = candidate.blockId;
          break;
        }
      }
      if (childBlock < 0)
        continue;
      if (op.opName == "scf.for") {
        if (auto induction = extractForInductionVariable(op.text))
          insertUnique(cfgBlocks[childBlock].arguments, *induction);
        for (const auto &pair : extractIterArgPairs(op.text))
          insertUnique(cfgBlocks[childBlock].arguments, pair.first);
      } else {
        for (const auto &pair : extractWhileInitArgPairs(op.text))
          insertUnique(cfgBlocks[childBlock].arguments, pair.first);
      }
    }
    for (auto &entry : cfgBlocks) {
      CFGBlockLivenessInfo &block = entry.second;
      block.definitions = block.arguments;
      for (int operationId : block.operations) {
        const OperationRecord &op = operation(operationId);
        for (const std::string &operand : operationOperandNames(op))
          insertUnique(block.uses, operand);
        for (const std::string &result : operationResultNames(op))
          insertUnique(block.definitions, result);
        for (const std::string &label : op.successorLabels) {
          auto destination = labelToBlock.find({op.regionPath, label});
          if (destination != labelToBlock.end())
            insertUnique(block.successors, destination->second);
        }
      }
      // BlockInfoBuilder gathers useValues and defValues independently, then
      // applies llvm::set_subtract. SmallPtrSet small-mode erase fills the
      // removed slot with the last value, which is observable before the
      // deterministic PlanMemory shuffle.  Large blocks have already left
      // SmallPtrSet's inline storage; preserve their semantic walk order.
      if (block.uses.size() > 16) {
        block.uses.erase(
            std::remove_if(block.uses.begin(), block.uses.end(),
                           [&](const std::string &use) {
                             return std::find(block.definitions.begin(),
                                              block.definitions.end(),
                                              use) != block.definitions.end();
                           }),
            block.uses.end());
      } else if (block.uses.size() < block.definitions.size()) {
        size_t index = 0;
        while (index < block.uses.size()) {
          if (std::find(block.definitions.begin(), block.definitions.end(),
                        block.uses[index]) != block.definitions.end()) {
            block.uses[index] = block.uses.back();
            block.uses.pop_back();
          } else {
            ++index;
          }
        }
      } else {
        for (const std::string &definition : block.definitions) {
          auto use = std::find(block.uses.begin(), block.uses.end(), definition);
          if (use == block.uses.end())
            continue;
          *use = block.uses.back();
          block.uses.pop_back();
        }
      }
    }
    bool changed = true;
    while (changed) {
      changed = false;
      for (auto blockIt = cfgBlocks.rbegin(); blockIt != cfgBlocks.rend();
           ++blockIt) {
        CFGBlockLivenessInfo &block = blockIt->second;
        std::vector<std::string> liveOuts;
        for (int successor : block.successors)
          for (const std::string &value : cfgBlocks.at(successor).liveIns)
            insertUnique(liveOuts, value);
        std::vector<std::string> liveIns = block.uses;
        for (const std::string &value : liveOuts)
          if (std::find(block.definitions.begin(), block.definitions.end(),
                        value) == block.definitions.end())
            insertUnique(liveIns, value);
        if (liveIns != block.liveIns || liveOuts != block.liveOuts) {
          block.liveIns = std::move(liveIns);
          block.liveOuts = std::move(liveOuts);
          changed = true;
        }
      }
    }
  }

  void addBlockArgument(const std::vector<int> &path,
                        const std::string &name) {
    blocks[path].path = path;
    if (std::find(blocks[path].arguments.begin(), blocks[path].arguments.end(),
                  name) == blocks[path].arguments.end())
      blocks[path].arguments.push_back(name);
    ValueLivenessInfo &value = values[name];
    value.blockArgument = true;
    value.definingBlock = path;
  }

  void ensureExternalValue(const std::string &name) {
    if (values.count(name))
      return;
    ValueLivenessInfo value;
    value.blockArgument = true;
    value.definingBlock = {};
    values[name] = value;
  }

  std::vector<int> childPath(const OperationRecord &parent) const {
    for (int operationId : operationOrder) {
      const OperationRecord &candidate = operation(operationId);
      if (candidate.index <= parent.index)
        continue;
      if (candidate.regionPath.size() == parent.regionPath.size() + 1 &&
          pathPrefix(parent.regionPath, candidate.regionPath))
        return candidate.regionPath;
      if (candidate.regionPath.size() <= parent.regionPath.size())
        break;
    }
    return parent.regionPath;
  }

  void buildLiveIns() {
    for (auto &blockEntry : blocks) {
      const std::vector<int> &path = blockEntry.first;
      std::vector<std::string> definitions = blockEntry.second.arguments;
      std::vector<std::string> uses;
      auto insertUnique = [](std::vector<std::string> &set,
                             const std::string &value) {
        if (std::find(set.begin(), set.end(), value) == set.end())
          set.push_back(value);
      };
      for (int operationId : operationOrder) {
        const OperationRecord &op = operation(operationId);
        if (!pathPrefix(path, op.regionPath))
          continue;
        // BlockInfoBuilder marks arguments of every nested block as
        // definitions while walking the parent block. Without this, an after
        // region argument (for example an scf.while `do` argument) is
        // incorrectly classified as a live-in of the enclosing block.
        for (const std::string &argument : op.blockArguments)
          insertUnique(definitions, argument);
        for (const std::string &result : operationResultNames(op))
          insertUnique(definitions, result);
        for (const std::string &operand : operationOperandNames(op))
          insertUnique(uses, operand);
        if (op.opName == "scf.for" || op.opName == "scf.while") {
          std::vector<int> child = childPath(op);
          if (pathPrefix(path, child)) {
            if (op.opName == "scf.for") {
              if (auto induction = extractForInductionVariable(op.text))
                insertUnique(definitions, *induction);
              for (const auto &pair : extractIterArgPairs(op.text))
                insertUnique(definitions, pair.first);
            } else {
              for (const auto &pair : extractWhileInitArgPairs(op.text))
                insertUnique(definitions, pair.first);
            }
          }
        }
      }
      // llvm::set_subtract selects the smaller side. SmallPtrSet::remove_if
      // and erase both replace a removed small-mode slot with the last value.
      // Once SmallPtrSet<Value, 16> grows out of small mode, the surviving
      // live-ins observed by PlanMemory retain the walk order relevant to its
      // deterministic shuffle.  Preserve that order instead of applying the
      // small-mode swap-with-last behavior to large blocks as well.
      if (uses.size() > 16) {
        uses.erase(
            std::remove_if(uses.begin(), uses.end(),
                           [&](const std::string &use) {
                             return std::find(definitions.begin(),
                                              definitions.end(),
                                              use) != definitions.end();
                           }),
            uses.end());
      } else if (uses.size() < definitions.size()) {
        size_t index = 0;
        while (index < uses.size()) {
          if (std::find(definitions.begin(), definitions.end(), uses[index]) !=
              definitions.end()) {
            uses[index] = uses.back();
            uses.pop_back();
          } else {
            ++index;
          }
        }
      } else {
        for (const std::string &definition : definitions) {
          auto it = std::find(uses.begin(), uses.end(), definition);
          if (it != uses.end()) {
            *it = uses.back();
            uses.pop_back();
          }
        }
      }
      blockEntry.second.liveIns = std::move(uses);
    }
  }

  int orderInBlock(int operationId, const std::vector<int> &path) const {
    const auto &ops = blocks.at(path).operations;
    auto it = std::find(ops.begin(), ops.end(), operationId);
    if (it != ops.end())
      return static_cast<int>(std::distance(ops.begin(), it));
    int projected = projectAtCommonBlock(operationId, path);
    it = std::find(ops.begin(), ops.end(), projected);
    return it == ops.end() ? -1
                           : static_cast<int>(std::distance(ops.begin(), it));
  }

  std::optional<int> projectToBlock(int operationId,
                                    const std::vector<int> &path) const {
    const OperationRecord &op = operation(operationId);
    if (!pathPrefix(path, op.regionPath))
      return std::nullopt;
    return projectAtCommonBlock(operationId, path);
  }

  int projectAtCommonBlock(int operationId,
                           const std::vector<int> &path) const {
    const OperationRecord &op = operation(operationId);
    if (op.regionPath == path)
      return operationId;
    if (!pathPrefix(path, op.regionPath))
      return operationId;
    int childRegion = op.regionPath[path.size()];
    auto owner = regionOwner.find(childRegion);
    if (owner != regionOwner.end())
      return owner->second;
    return operationId;
  }

  bool isCurrentlyLive(const std::string &name, int operationId,
                       const BlockLivenessInfo &blockInfo) const {
    const ValueLivenessInfo *valueInfo = value(name);
    if (!valueInfo)
      return false;
    int queryOrder = orderInBlock(operationId, blockInfo.path);
    bool liveIn = std::find(blockInfo.liveIns.begin(), blockInfo.liveIns.end(),
                            name) != blockInfo.liveIns.end();
    int start = 0;
    if (!liveIn && !valueInfo->blockArgument) {
      std::optional<int> projected =
          projectToBlock(valueInfo->definingOperationId, blockInfo.path);
      if (!projected)
        return false;
      start = orderInBlock(*projected, blockInfo.path);
    }
    int end = start;
    for (int userId : valueInfo->users) {
      std::optional<int> projected = projectToBlock(userId, blockInfo.path);
      if (projected)
        end = std::max(end, orderInBlock(*projected, blockInfo.path));
    }
    return start <= queryOrder && queryOrder <= end;
  }

  std::map<int, OperationRecord> operationById;
  std::vector<int> operationOrder;
  std::map<std::string, ValueLivenessInfo> values;
  std::map<std::vector<int>, BlockLivenessInfo> blocks;
  std::map<int, int> regionOwner;
  std::map<int, CFGBlockLivenessInfo> cfgBlocks;
  std::map<std::vector<int>, std::vector<int>> regionBlocks;
};

} // namespace cvub

#endif
