// Lightweight equivalent of MLIR Liveness for PlanMemory-visible semantics.
#ifndef CVPIPELINE_UB_MODEL_CPP_LIVENESS_HPP
#define CVPIPELINE_UB_MODEL_CPP_LIVENESS_HPP

#include "buffer_alias.hpp"
#include "operation_index.hpp"
#include "../../support/debug_trace.hpp"

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

struct OrderedLiveCandidate {
  std::string name;
  int firstOperation = 0;
  int lastOperation = -1;
};

class Liveness {
public:
  Liveness(const std::vector<OperationRecord> &linearOperations,
           const std::vector<std::string> &functionArguments,
           const PlanMemoryOperationIndex &indexedOperations,
           DebugTrace *trace = nullptr)
      : operationIndex(indexedOperations) {
    for (const OperationRecord &op : linearOperations) {
      if (operationIds.insert(op.operationId).second) {
        operationOrder.push_back(op.operationId);
        blocks[op.regionPath].path = op.regionPath;
        blocks[op.regionPath].operations.push_back(op.operationId);
      }
    }
    for (const auto &[path, blockInfo] : blocks) {
      auto &orders = blockOperationOrder[path];
      for (size_t index = 0; index < blockInfo.operations.size(); ++index)
        orders[blockInfo.operations[index]] = static_cast<int>(index);
    }
    blocks[{}].path = {};
    blockOperationOrder[{}];
    for (int operationId : operationOrder) {
      const OperationRecord &op = operation(operationId);
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
          const OperationRecord &candidate = operation(candidateId);
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
      if (!operationIndex.isCanonicalRecord(op))
        continue;
      for (const std::string &argument : op.blockArguments)
        addBlockArgument(op.regionPath, argument);
      if (op.opName != "scf.for" && op.opName != "scf.while")
        continue;
      std::vector<int> child = childPath(op);
      std::vector<std::string> childArguments;
      if (op.opName == "scf.for") {
        if (auto induction = extractForInductionVariable(op.text)) {
          addBlockArgument(child, *induction);
          childArguments.push_back(*induction);
        }
        for (const auto &pair : extractIterArgPairs(op.text)) {
          addBlockArgument(child, pair.first);
          childArguments.push_back(pair.first);
        }
      } else {
        for (const auto &pair : extractWhileInitArgPairs(op.text)) {
          addBlockArgument(child, pair.first);
          childArguments.push_back(pair.first);
        }
      }
      structuredChildArguments.emplace(op.operationId,
                                       std::move(childArguments));
    }

    for (int operationId : operationOrder) {
      const OperationRecord &op = operation(operationId);
      for (const std::string &result : results(operationId)) {
        ValueLivenessInfo &value = values[result];
        value.definingOperationId = operationId;
        value.definingBlock = op.regionPath;
      }
      for (const std::string &operand : operands(operationId)) {
        ensureExternalValue(operand);
        values[operand].users.push_back(operationId);
      }
    }
    MeasureStage(trace, "PlanMemory.Liveness.LiveIns",
                 [&] { buildLiveIns(); });
    MeasureStage(trace, "PlanMemory.Liveness.Intervals",
                 [&] { buildLiveIntervals(); });
    MeasureStage(trace, "PlanMemory.Liveness.CFG",
                 [&] { buildCFGLiveness(functionArguments); });
  }

  const OperationRecord &operation(int operationId) const {
    return operationIndex.operation(operationId);
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
    std::vector<std::string> result;
    const int queryOrder = orderInBlock(operationId, query.regionPath);
    const std::vector<OrderedLiveCandidate> &candidates =
        orderedCandidates(query.regionPath);
    result.reserve(candidates.size());
    for (const OrderedLiveCandidate &candidate : candidates)
      if (candidate.firstOperation <= queryOrder &&
          queryOrder <= candidate.lastOperation)
        result.push_back(candidate.name);
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
    auto block = cfgBlockOperationOrder.find(blockId);
    if (block == cfgBlockOperationOrder.end())
      return -1;
    auto operation = block->second.find(operationId);
    return operation == block->second.end() ? -1 : operation->second;
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
    std::vector<std::string> result;
    const int queryOrder = orderInCFGBlock(operationId, query.blockId);
    const std::vector<OrderedLiveCandidate> &candidates =
        orderedCFGCandidates(query.blockId);
    result.reserve(candidates.size());
    for (const OrderedLiveCandidate &candidate : candidates)
      if (candidate.firstOperation <= queryOrder &&
          queryOrder <= candidate.lastOperation)
        result.push_back(candidate.name);
    return result;
  }

  const std::vector<OrderedLiveCandidate> &
  orderedCandidates(const std::vector<int> &path) const {
    auto cached = orderedCandidateCache.find(path);
    if (cached != orderedCandidateCache.end())
      return cached->second;
    const BlockLivenessInfo &blockInfo = block(path);
    std::vector<std::string> names;
    std::unordered_set<std::string> observed;
    auto append = [&](const std::string &name) {
      if (observed.insert(name).second)
        names.push_back(name);
    };
    for (const std::string &argument : blockInfo.arguments)
      append(argument);
    for (const std::string &liveIn : blockInfo.liveIns)
      append(liveIn);
    for (int blockOperationId : blockInfo.operations)
      for (const std::string &result : results(blockOperationId))
        append(result);
    std::vector<OrderedLiveCandidate> candidates;
    candidates.reserve(names.size());
    const auto &intervals = liveIntervals.at(path);
    for (std::string &name : names) {
      auto interval = intervals.find(name);
      if (interval == intervals.end())
        continue;
      candidates.push_back(
          {std::move(name), interval->second.first, interval->second.second});
    }
    return orderedCandidateCache.emplace(path, std::move(candidates))
        .first->second;
  }

  const std::vector<OrderedLiveCandidate> &
  orderedCFGCandidates(int blockId) const {
    auto cached = orderedCFGCandidateCache.find(blockId);
    if (cached != orderedCFGCandidateCache.end())
      return cached->second;
    const CFGBlockLivenessInfo &blockInfo = cfgBlocks.at(blockId);
    std::vector<std::string> names;
    std::unordered_set<std::string> observed;
    auto append = [&](const std::string &name) {
      if (observed.insert(name).second)
        names.push_back(name);
    };
    for (const std::string &argument : blockInfo.arguments)
      append(argument);
    for (const std::string &liveIn : blockInfo.liveIns)
      append(liveIn);
    for (int blockOperationId : blockInfo.operations)
      for (const std::string &result : results(blockOperationId))
        append(result);
    std::vector<OrderedLiveCandidate> candidates;
    candidates.reserve(names.size());
    for (std::string &name : names) {
      const ValueLivenessInfo *valueInfo = value(name);
      if (!valueInfo)
        continue;
      const bool liveIn =
          std::find(blockInfo.liveIns.begin(), blockInfo.liveIns.end(), name) !=
          blockInfo.liveIns.end();
      const bool blockArgument =
          std::find(blockInfo.arguments.begin(), blockInfo.arguments.end(),
                    name) != blockInfo.arguments.end();
      int first = 0;
      if (!liveIn && !blockArgument) {
        if (valueInfo->definingOperationId < 0 ||
            operation(valueInfo->definingOperationId).blockId != blockId)
          continue;
        first = orderInCFGBlock(valueInfo->definingOperationId, blockId);
      }
      int last = first;
      for (int userId : valueInfo->users)
        if (operation(userId).blockId == blockId)
          last = std::max(last, orderInCFGBlock(userId, blockId));
      if (std::find(blockInfo.liveOuts.begin(), blockInfo.liveOuts.end(),
                    name) != blockInfo.liveOuts.end())
        last = static_cast<int>(blockInfo.operations.size()) - 1;
      candidates.push_back({std::move(name), first, last});
    }
    return orderedCFGCandidateCache.emplace(blockId, std::move(candidates))
        .first->second;
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
      cfgBlockOperationOrder[op.blockId][operationId] =
          static_cast<int>(block.operations.size() - 1);
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
      auto arguments = structuredChildArguments.find(operationId);
      if (arguments != structuredChildArguments.end())
        for (const std::string &argument : arguments->second)
          insertUnique(cfgBlocks[childBlock].arguments, argument);
    }
    for (auto &entry : cfgBlocks) {
      CFGBlockLivenessInfo &block = entry.second;
      block.definitions = block.arguments;
      for (int operationId : block.operations) {
        const OperationRecord &op = operation(operationId);
        for (const std::string &operand : operands(operationId))
          insertUnique(block.uses, operand);
        for (const std::string &result : results(operationId))
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
    auto cached = childPathCache.find(parent.operationId);
    if (cached != childPathCache.end())
      return cached->second;
    for (int operationId : operationOrder) {
      const OperationRecord &candidate = operation(operationId);
      if (candidate.index <= parent.index)
        continue;
      if (candidate.regionPath.size() == parent.regionPath.size() + 1 &&
          pathPrefix(parent.regionPath, candidate.regionPath))
        return childPathCache.emplace(parent.operationId,
                                      candidate.regionPath)
            .first->second;
      if (candidate.regionPath.size() <= parent.regionPath.size())
        break;
    }
    return childPathCache.emplace(parent.operationId, parent.regionPath)
        .first->second;
  }

  void buildLiveIns() {
    for (auto &blockEntry : blocks) {
      const std::vector<int> &path = blockEntry.first;
      std::vector<std::string> definitions = blockEntry.second.arguments;
      std::vector<std::string> uses;
      std::unordered_set<std::string> definitionSet(definitions.begin(),
                                                    definitions.end());
      std::unordered_set<std::string> useSet;
      const auto insertDefinition = [&](const std::string &value) {
        if (definitionSet.insert(value).second)
          definitions.push_back(value);
      };
      const auto insertUse = [&](const std::string &value) {
        if (useSet.insert(value).second)
          uses.push_back(value);
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
          insertDefinition(argument);
        for (const std::string &result : results(operationId))
          insertDefinition(result);
        for (const std::string &operand : operands(operationId))
          insertUse(operand);
        if (op.opName == "scf.for" || op.opName == "scf.while") {
          std::vector<int> child = childPath(op);
          if (pathPrefix(path, child)) {
            auto arguments = structuredChildArguments.find(operationId);
            if (arguments != structuredChildArguments.end())
              for (const std::string &argument : arguments->second)
                insertDefinition(argument);
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
                             return definitionSet.find(use) !=
                                    definitionSet.end();
                           }),
            uses.end());
      } else if (uses.size() < definitions.size()) {
        size_t index = 0;
        while (index < uses.size()) {
          if (definitionSet.find(uses[index]) != definitionSet.end()) {
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

  void buildLiveIntervals() {
    for (const auto &[path, blockInfo] : blocks) {
      if (hasMultipleBlocks(path))
        continue;
      auto &intervals = liveIntervals[path];
      for (const auto &[name, valueInfo] : values) {
        const bool liveIn =
            std::find(blockInfo.liveIns.begin(), blockInfo.liveIns.end(),
                      name) != blockInfo.liveIns.end();
        int start = 0;
        if (!liveIn && !valueInfo.blockArgument) {
          std::optional<int> projected =
              projectToBlock(valueInfo.definingOperationId, path);
          if (!projected)
            continue;
          start = orderInBlock(*projected, path);
        }
        int end = start;
        for (int userId : valueInfo.users) {
          std::optional<int> projected = projectToBlock(userId, path);
          if (projected)
            end = std::max(end, orderInBlock(*projected, path));
        }
        intervals.emplace(name, std::make_pair(start, end));
      }
    }
  }

  int orderInBlock(int operationId, const std::vector<int> &path) const {
    const auto &orders = blockOperationOrder.at(path);
    auto found = orders.find(operationId);
    if (found != orders.end())
      return found->second;
    int projected = projectAtCommonBlock(operationId, path);
    found = orders.find(projected);
    return found == orders.end() ? -1 : found->second;
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
    auto blockIntervals = liveIntervals.find(blockInfo.path);
    if (blockIntervals == liveIntervals.end())
      return false;
    auto interval = blockIntervals->second.find(name);
    if (interval == blockIntervals->second.end())
      return false;
    const int queryOrder = orderInBlock(operationId, blockInfo.path);
    return interval->second.first <= queryOrder &&
           queryOrder <= interval->second.second;
  }

  const std::vector<std::string> &results(int operationId) const {
    return operationIndex.results(operationId);
  }

  const std::vector<std::string> &operands(int operationId) const {
    return operationIndex.operands(operationId);
  }

  const PlanMemoryOperationIndex &operationIndex;
  std::set<int> operationIds;
  std::vector<int> operationOrder;
  std::unordered_map<std::string, ValueLivenessInfo> values;
  std::map<std::vector<int>, BlockLivenessInfo> blocks;
  std::map<std::vector<int>, std::map<int, int>> blockOperationOrder;
  std::map<std::vector<int>,
           std::map<std::string, std::pair<int, int>>>
      liveIntervals;
  std::map<int, int> regionOwner;
  std::map<int, std::vector<std::string>> structuredChildArguments;
  mutable std::map<int, std::vector<int>> childPathCache;
  std::map<int, CFGBlockLivenessInfo> cfgBlocks;
  std::map<int, std::map<int, int>> cfgBlockOperationOrder;
  std::map<std::vector<int>, std::vector<int>> regionBlocks;
  mutable std::map<std::vector<int>, std::vector<OrderedLiveCandidate>>
      orderedCandidateCache;
  mutable std::map<int, std::vector<OrderedLiveCandidate>>
      orderedCFGCandidateCache;
};

} // namespace cvub

#endif
