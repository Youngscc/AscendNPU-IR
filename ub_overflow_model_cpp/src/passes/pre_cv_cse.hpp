#ifndef UB_OVERFLOW_MODEL_CPP_PRE_CV_CSE_HPP
#define UB_OVERFLOW_MODEL_CPP_PRE_CV_CSE_HPP

#include "../ir/generic_analysis.hpp"
#include "../ir/generic_rewriter.hpp"

#include <functional>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace cvub {

// Source of truth: third-party/llvm-project/mlir/lib/Transforms/CSE.cpp.
// This is the standalone module CSE in canonicalizationHIVMPipeline.  The
// traversal and the lifetime of available expressions intentionally mirror
// CSEDriver: nested regions are simplified before their owning operation,
// values are scoped by region/dominator path, and erasure is deferred until
// the traversal has completed.

inline bool PreCVCSEIsTerminator(const GenericOperation &operation) {
  static const std::set<std::string> names = {
      "affine.yield", "cf.br",       "cf.cond_br", "func.return",
      "scf.condition", "scf.reduce.return", "scf.yield", "scope.return"};
  return names.count(operation.name) != 0;
}

inline bool PreCVCSEIsSymbol(const GenericOperation &operation) {
  return operation.name == "func.func" || operation.name == "builtin.module";
}

inline bool PreCVCSEHasRecursiveMemoryEffects(
    const GenericOperation &operation) {
  static const std::set<std::string> names = {
      "scf.for", "scf.forall", "scf.forall.in_parallel", "scf.if",
      "scf.parallel", "scf.while", "scope.scope"};
  return names.count(operation.name) != 0;
}

inline bool PreCVCSEIsIsolatedFromAbove(const GenericOperation &operation) {
  return operation.name == "func.func" || operation.name == "builtin.module";
}

inline bool PreCVCSEIsCommutative(const GenericOperation &operation) {
  static const std::set<std::string> names = {
      "arith.addf",          "arith.addi",    "arith.addui_extended",
      "arith.andi",          "arith.maximumf", "arith.maxnumf",
      "arith.maxsi",         "arith.maxui",   "arith.minimumf",
      "arith.minnumf",       "arith.minsi",   "arith.minui",
      "arith.mulf",          "arith.muli",    "arith.mulsi_extended",
      "arith.mului_extended", "arith.ori",     "arith.xori"};
  return names.count(operation.name) != 0;
}

enum class PreCVCSEEffectsKind { Free, ReadOnly, Other };

inline PreCVCSEEffectsKind PreCVCSEDirectEffects(
    const GenericOperation &operation) {
  if (operation.effects.empty())
    return PreCVCSEEffectsKind::Free;
  if (operation.effects == "none")
    return PreCVCSEEffectsKind::Other;
  bool sawEffect = false;
  for (const std::string &entry : split(operation.effects, ',')) {
    const std::string effect = trim(entry);
    if (effect.empty())
      continue;
    sawEffect = true;
    if (!startsWith(effect, "read@"))
      return PreCVCSEEffectsKind::Other;
  }
  return sawEffect ? PreCVCSEEffectsKind::ReadOnly
                   : PreCVCSEEffectsKind::Other;
}

inline PreCVCSEEffectsKind PreCVCSERecursiveEffects(
    const GenericModule &module, const GenericOperation &operation,
    const std::set<int> &pendingErase) {
  const PreCVCSEEffectsKind direct = PreCVCSEDirectEffects(operation);
  if (!PreCVCSEHasRecursiveMemoryEffects(operation))
    return direct;
  bool sawRead = direct == PreCVCSEEffectsKind::ReadOnly;
  if (direct == PreCVCSEEffectsKind::Other && operation.effects != "none")
    return PreCVCSEEffectsKind::Other;
  std::function<bool(int)> visit = [&](int operationId) {
    if (pendingErase.count(operationId) != 0)
      return true;
    const GenericOperation &nested =
        module.operations.at(static_cast<size_t>(operationId));
    const PreCVCSEEffectsKind effects = PreCVCSEDirectEffects(nested);
    if (PreCVCSEHasRecursiveMemoryEffects(nested)) {
      if (effects == PreCVCSEEffectsKind::Other && nested.effects != "none")
        return false;
      if (effects == PreCVCSEEffectsKind::ReadOnly)
        sawRead = true;
      for (int regionId : nested.regions)
        for (int blockId : module.regions.at(static_cast<size_t>(regionId)).blocks)
          for (int childId : module.blocks.at(static_cast<size_t>(blockId)).operations)
            if (!visit(childId))
              return false;
      return true;
    }
    if (effects == PreCVCSEEffectsKind::Other)
      return false;
    if (effects == PreCVCSEEffectsKind::ReadOnly)
      sawRead = true;
    return true;
  };
  for (int regionId : operation.regions)
    for (int blockId : module.regions.at(static_cast<size_t>(regionId)).blocks)
      for (int childId : module.blocks.at(static_cast<size_t>(blockId)).operations)
        if (!visit(childId))
          return PreCVCSEEffectsKind::Other;
  return sawRead ? PreCVCSEEffectsKind::ReadOnly
                 : PreCVCSEEffectsKind::Free;
}

inline bool PreCVCSEAllocationIsDroppable(
    const GenericOperation &operation) {
  if (operation.effects.empty() || operation.effects == "none")
    return false;
  for (const std::string &entry : split(operation.effects, ',')) {
    const std::string effect = trim(entry);
    if (startsWith(effect, "read@"))
      continue;
    if (!startsWith(effect, "allocate@"))
      return false;
    bool ownsAllocation = false;
    for (int result : operation.results)
      if (effect.find("@" + std::to_string(result) + "@") !=
          std::string::npos) {
        ownsAllocation = true;
        break;
      }
    if (!ownsAllocation)
      return false;
  }
  return true;
}

inline bool PreCVCSEWouldBeTriviallyDead(
    const GenericModule &module, const GenericOperation &operation,
    const std::set<int> &pendingErase) {
  if (PreCVCSEIsTerminator(operation) || PreCVCSEIsSymbol(operation))
    return false;
  const PreCVCSEEffectsKind effects =
      PreCVCSERecursiveEffects(module, operation, pendingErase);
  return effects == PreCVCSEEffectsKind::Free ||
         effects == PreCVCSEEffectsKind::ReadOnly ||
         PreCVCSEAllocationIsDroppable(operation);
}

inline std::vector<int> PreCVCSEComparableOperands(
    const GenericOperation &operation) {
  std::vector<int> operands = operation.operands;
  if (PreCVCSEIsCommutative(operation))
    std::sort(operands.begin(), operands.end());
  return operands;
}

inline std::string PreCVCSEOperationKey(const GenericOperation &operation) {
  std::ostringstream key;
  key << operation.name << '\n' << operation.properties << '\n'
      << operation.attributes << '\n'
      << JoinDelimited(operation.resultTypes, ",") << '\n'
      << joinIds(PreCVCSEComparableOperands(operation)) << '\n'
      << operation.regions.size() << ':' << operation.successors.size();
  return key.str();
}

inline bool PreCVCSEOperationsEquivalent(
    const GenericModule &module, int lhsId, int rhsId) {
  std::map<int, int> valueMap;
  std::map<int, int> blockMap;
  std::function<bool(int, int)> equivalentOperation;
  equivalentOperation = [&](int leftId, int rightId) {
    const GenericOperation &lhs =
        module.operations.at(static_cast<size_t>(leftId));
    const GenericOperation &rhs =
        module.operations.at(static_cast<size_t>(rightId));
    if (lhs.name != rhs.name || lhs.properties != rhs.properties ||
        lhs.attributes != rhs.attributes ||
        lhs.resultTypes != rhs.resultTypes ||
        lhs.regions.size() != rhs.regions.size() ||
        lhs.successors.size() != rhs.successors.size() ||
        lhs.operands.size() != rhs.operands.size())
      return false;

    auto mappedValue = [&](int value) {
      const auto mapped = valueMap.find(value);
      return mapped == valueMap.end() ? value : mapped->second;
    };
    if (PreCVCSEIsCommutative(lhs)) {
      std::vector<int> leftOperands;
      leftOperands.reserve(lhs.operands.size());
      for (int value : lhs.operands)
        leftOperands.push_back(mappedValue(value));
      std::vector<int> rightOperands = rhs.operands;
      std::sort(leftOperands.begin(), leftOperands.end());
      std::sort(rightOperands.begin(), rightOperands.end());
      if (leftOperands != rightOperands)
        return false;
    } else {
      for (size_t index = 0; index < lhs.operands.size(); ++index)
        if (mappedValue(lhs.operands[index]) != rhs.operands[index])
          return false;
    }
    for (size_t index = 0; index < lhs.results.size(); ++index)
      valueMap[lhs.results[index]] = rhs.results[index];

    for (size_t regionIndex = 0; regionIndex < lhs.regions.size();
         ++regionIndex) {
      const GenericRegion &leftRegion = module.regions.at(
          static_cast<size_t>(lhs.regions[regionIndex]));
      const GenericRegion &rightRegion = module.regions.at(
          static_cast<size_t>(rhs.regions[regionIndex]));
      if (leftRegion.blocks.size() != rightRegion.blocks.size())
        return false;
      for (size_t blockIndex = 0; blockIndex < leftRegion.blocks.size();
           ++blockIndex) {
        const GenericBlock &leftBlock = module.blocks.at(
            static_cast<size_t>(leftRegion.blocks[blockIndex]));
        const GenericBlock &rightBlock = module.blocks.at(
            static_cast<size_t>(rightRegion.blocks[blockIndex]));
        if (leftBlock.argumentTypes != rightBlock.argumentTypes ||
            leftBlock.operations.size() != rightBlock.operations.size())
          return false;
        blockMap[leftBlock.id] = rightBlock.id;
        for (size_t argument = 0; argument < leftBlock.arguments.size();
             ++argument)
          valueMap[leftBlock.arguments[argument]] =
              rightBlock.arguments[argument];
        for (size_t operation = 0; operation < leftBlock.operations.size();
             ++operation)
          if (!equivalentOperation(leftBlock.operations[operation],
                                   rightBlock.operations[operation]))
            return false;
      }
    }
    for (size_t index = 0; index < lhs.successors.size(); ++index) {
      const auto mapped = blockMap.find(lhs.successors[index]);
      if ((mapped == blockMap.end() ? lhs.successors[index]
                                    : mapped->second) !=
          rhs.successors[index])
        return false;
    }
    return true;
  };
  return equivalentOperation(lhsId, rhsId);
}

class PreCVCSEDriver {
public:
  explicit PreCVCSEDriver(GenericModule &input)
      : module(input), analysis(module), rewriter(module, &analysis) {}

  void run() {
    if (module.operations.empty())
      return;
    State state;
    for (int regionId : module.operations.front().regions)
      simplifyRegion(state, regionId);
    rewriter.removeManyFromBlocks(
        std::vector<int>(pendingErase.begin(), pendingErase.end()));
  }

private:
  struct State {
    std::map<std::string, std::vector<int>> known;
    std::vector<std::string> inserted;
  };

  class Scope {
  public:
    explicit Scope(State &input) : state(input), begin(input.inserted.size()) {}
    ~Scope() {
      while (state.inserted.size() > begin) {
        const std::string key = std::move(state.inserted.back());
        state.inserted.pop_back();
        auto found = state.known.find(key);
        if (found == state.known.end() || found->second.empty())
          continue;
        found->second.pop_back();
        if (found->second.empty())
          state.known.erase(found);
      }
    }
  private:
    State &state;
    size_t begin;
  };

  void insert(State &state, const std::string &key, int operation) {
    state.known[key].push_back(operation);
    state.inserted.push_back(key);
  }

  bool hasUsers(const GenericOperation &operation) const {
    return std::any_of(operation.results.begin(), operation.results.end(),
                       [&](int result) { return analysis.hasUsers(result); });
  }

  bool hasWriteOrUnknownBetween(const GenericOperation &from,
                                const GenericOperation &to) const {
    if (from.blockId != to.blockId)
      return true;
    const GenericBlock &block =
        module.blocks.at(static_cast<size_t>(from.blockId));
    bool afterFrom = false;
    for (int operationId : block.operations) {
      if (operationId == from.id) {
        afterFrom = true;
        continue;
      }
      if (!afterFrom)
        continue;
      if (operationId == to.id)
        return false;
      if (pendingErase.count(operationId) != 0)
        continue;
      const PreCVCSEEffectsKind effects = PreCVCSERecursiveEffects(
          module, module.operations.at(static_cast<size_t>(operationId)),
          pendingErase);
      if (effects == PreCVCSEEffectsKind::Other)
        return true;
    }
    return true;
  }

  bool simplifyOperation(State &state, int operationId,
                         bool hasSSADominance) {
    GenericOperation snapshot =
        module.operations.at(static_cast<size_t>(operationId));
    if (PreCVCSEIsTerminator(snapshot))
      return false;
    if (!hasUsers(snapshot) &&
        PreCVCSEWouldBeTriviallyDead(module, snapshot, pendingErase)) {
      pendingErase.insert(operationId);
      return true;
    }
    if (std::any_of(snapshot.regions.begin(), snapshot.regions.end(),
                    [&](int regionId) {
                      return module.regions.at(static_cast<size_t>(regionId))
                                 .blocks.size() > 1;
                    }))
      return false;

    const PreCVCSEEffectsKind effects =
        PreCVCSERecursiveEffects(module, snapshot, pendingErase);
    if (effects != PreCVCSEEffectsKind::Free &&
        effects != PreCVCSEEffectsKind::ReadOnly)
      return false;
    const std::string key = PreCVCSEOperationKey(snapshot);
    auto found = state.known.find(key);
    if (found != state.known.end()) {
      for (auto candidate = found->second.rbegin();
           candidate != found->second.rend(); ++candidate) {
        if (!PreCVCSEOperationsEquivalent(module, *candidate, operationId))
          continue;
        const GenericOperation &existing =
            module.operations.at(static_cast<size_t>(*candidate));
        if (effects == PreCVCSEEffectsKind::ReadOnly &&
            (existing.blockId != snapshot.blockId ||
             hasWriteOrUnknownBetween(existing, snapshot)))
          break;
        if (existing.results.size() != snapshot.results.size())
          break;
        // All regions encountered in this pipeline have SSA dominance.  Keep
        // the conditional branch explicit to mirror CSEDriver's contract and
        // fail closed if a non-SSA multi-block region becomes reachable.
        if (!hasSSADominance)
          throw std::runtime_error(
              "pre-cv CSE: non-SSA region replacement is unsupported");
        for (size_t index = 0; index < snapshot.results.size(); ++index)
          rewriter.replaceAllUses(snapshot.results[index],
                                  existing.results[index]);
        pendingErase.insert(operationId);
        return true;
      }
    }
    insert(state, key, operationId);
    return false;
  }

  void simplifyBlock(State &state, int blockId, bool hasSSADominance) {
    const std::vector<int> operations =
        module.blocks.at(static_cast<size_t>(blockId)).operations;
    for (int operationId : operations) {
      const GenericOperation snapshot =
          module.operations.at(static_cast<size_t>(operationId));
      if (!snapshot.regions.empty()) {
        if (PreCVCSEIsIsolatedFromAbove(snapshot)) {
          State nested;
          for (int regionId : snapshot.regions)
            simplifyRegion(nested, regionId);
        } else {
          for (int regionId : snapshot.regions)
            simplifyRegion(state, regionId);
        }
      }
      simplifyOperation(state, operationId, hasSSADominance);
    }
  }

  void simplifyRegion(State &state, int regionId) {
    const GenericRegion &region =
        module.regions.at(static_cast<size_t>(regionId));
    if (region.blocks.empty())
      return;
    if (region.blocks.size() == 1) {
      Scope scope(state);
      simplifyBlock(state, region.blocks.front(), true);
      return;
    }
    // The prefix currently contains only single-block structured regions.
    // Native CSE uses DominanceInfo for multi-block SSACFG regions; silently
    // linearizing those blocks would be semantically wrong.
    throw std::runtime_error(
        "pre-cv CSE: multi-block dominance tree is not modeled");
  }

  GenericModule &module;
  PipelineAnalysisContext analysis;
  GenericRewriter rewriter;
  std::set<int> pendingErase;
};

inline GenericModule RunPreCVCSE(GenericModule module) {
  PreCVCSEDriver(module).run();
  return CompactGenericModule(std::move(module));
}

} // namespace cvub

#endif
