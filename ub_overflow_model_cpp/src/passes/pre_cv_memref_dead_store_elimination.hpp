#ifndef UB_OVERFLOW_MODEL_CPP_PRE_CV_MEMREF_DEAD_STORE_ELIMINATION_HPP
#define UB_OVERFLOW_MODEL_CPP_PRE_CV_MEMREF_DEAD_STORE_ELIMINATION_HPP

#include "../ir/generic_analysis.hpp"
#include "../ir/generic_op_semantics.hpp"
#include "../ir/generic_rewriter.hpp"

#include <map>
#include <set>

namespace cvub {

inline bool PreCVDSEIsViewLike(const std::string &name) {
  static const std::set<std::string> names = {
      "hivm.hir.convert_layout", "memref.cast",
      "memref.collapse_shape", "memref.expand_shape",
      "memref.extract_strided_metadata", "memref.memory_space_cast",
      "memref.reinterpret_cast", "memref.reshape", "memref.subview",
      "memref.view"};
  return names.count(name) != 0;
}

inline bool PreCVDSEIsReturnLike(const std::string &name) {
  static const std::set<std::string> names = {
      "affine.yield", "func.return", "scf.condition", "scf.reduce.return",
      "scf.yield", "scope.return"};
  return names.count(name) != 0;
}

inline bool PreCVDSEHasReadEffect(const GenericOperation &operation) {
  for (const std::string &entry : split(operation.effects, ','))
    if (startsWith(trim(entry), "read@"))
      return true;
  return false;
}

inline std::vector<int>
PreCVDSEWriteEffectValues(const GenericOperation &operation) {
  std::vector<int> values;
  for (const std::string &entry : split(operation.effects, ',')) {
    const std::string effect = trim(entry);
    if (!startsWith(effect, "write@"))
      continue;
    const size_t begin = std::string("write@").size();
    const size_t end = effect.find('@', begin);
    if (end == std::string::npos)
      throw std::runtime_error("pre-CV memref DSE: malformed write effect");
    const std::string value = effect.substr(begin, end - begin);
    if (value != "-")
      values.push_back(std::stoi(value));
  }
  return values;
}

inline bool PreCVDSEIsScalarLikeMemRef(const std::string &type) {
  const std::optional<MemRefTypeModel> memref = ParseMemRefType(type);
  return memref &&
         std::all_of(memref->shape.begin(), memref->shape.end(),
                     [](const std::optional<int64_t> &dimension) {
                       return dimension && *dimension == 1;
                     });
}

class PreCVDSEUnionFind {
public:
  explicit PreCVDSEUnionFind(size_t size) : parent(size), minimum(size) {
    for (size_t index = 0; index < size; ++index) {
      parent[index] = index;
      minimum[index] = index;
    }
  }

  size_t find(size_t value) {
    if (parent[value] != value)
      parent[value] = find(parent[value]);
    return parent[value];
  }

  void join(size_t lhs, size_t rhs) {
    lhs = find(lhs);
    rhs = find(rhs);
    if (lhs == rhs)
      return;
    if (lhs > rhs)
      std::swap(lhs, rhs);
    parent[rhs] = lhs;
    minimum[lhs] = std::min(minimum[lhs], minimum[rhs]);
  }

  size_t minIndex(size_t value) { return minimum[find(value)]; }

private:
  std::vector<size_t> parent;
  std::vector<size_t> minimum;
};

class PreCVMemrefDSEFunctionDriver {
public:
  PreCVMemrefDSEFunctionDriver(GenericModule &input, int inputFunction,
                               bool inputRegBased)
      : module(input), functionId(inputFunction), regBased(inputRegBased),
        rewriter(module) {
    collectPreOrder();
    buildValueDependency();
  }

  void run() {
    precomputeStoreToLoad(functionId);
    // The native pass keeps raw Operation pointers in OpsOfAlloc, erases a
    // forwarded memref.load, and then dereferences that list in the reg-based
    // cleanup. A minimal Ascend950 store/load fixture reproducibly aborts in
    // runOnOperation. Preserve failure parity without reproducing undefined
    // behavior in the lightweight process.
    if (regBased && !loadedByStore.empty())
      throw std::runtime_error(
          "pre-CV memref DSE: native reg-based load forwarding aborts");
    forwardLoads();
    if (regBased)
      eraseUnusedHIVMLoads();
    eraseDeadAllocAndStores();
  }

private:
  void collectNested(int operationId) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    for (int regionId : operation.regions)
      for (int blockId :
           module.regions.at(static_cast<size_t>(regionId)).blocks)
        for (int childId :
             module.blocks.at(static_cast<size_t>(blockId)).operations) {
          preOrder.push_back(childId);
          collectNested(childId);
        }
  }

  void collectPreOrder() { collectNested(functionId); }

  void observeValue(int value) {
    if (valueToIndex.count(value) != 0)
      return;
    valueToIndex[value] = valueList.size();
    valueList.push_back(value);
    operationsOfAlloc.emplace_back();
  }

  void buildValueDependency() {
    const GenericOperation &function =
        module.operations.at(static_cast<size_t>(functionId));
    if (!function.regions.empty()) {
      const GenericRegion &region =
          module.regions.at(static_cast<size_t>(function.regions.front()));
      if (!region.blocks.empty())
        for (int argument : module.blocks.at(
                 static_cast<size_t>(region.blocks.front())).arguments)
          observeValue(argument);
    }
    for (int operationId : preOrder) {
      const GenericOperation &operation =
          module.operations.at(static_cast<size_t>(operationId));
      for (int operand : operation.operands)
        observeValue(operand);
      for (int result : operation.results)
        observeValue(result);
    }
    disjointSet.emplace(valueList.size());
    for (int operationId : preOrder) {
      const GenericOperation &operation =
          module.operations.at(static_cast<size_t>(operationId));
      if (PreCVDSEIsViewLike(operation.name) &&
          !operation.operands.empty()) {
        const size_t source = valueToIndex.at(operation.operands.front());
        for (int result : operation.results)
          disjointSet->join(source, valueToIndex.at(result));
      }
      for (int result : operation.results)
        operationsOfAlloc[rootIndex(result)].push_back(operationId);
      for (int operand : operation.operands)
        operationsOfAlloc[rootIndex(operand)].push_back(operationId);
    }
  }

  size_t rootIndex(int value) {
    return disjointSet->minIndex(valueToIndex.at(value));
  }

  int rootValue(int value) { return valueList.at(rootIndex(value)); }

  bool isAttached(int operationId) const {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    if (operation.blockId < 0)
      return false;
    const GenericBlock &block =
        module.blocks.at(static_cast<size_t>(operation.blockId));
    return std::find(block.operations.begin(), block.operations.end(),
                     operationId) != block.operations.end();
  }

  static std::vector<int> storeIndices(const GenericOperation &store) {
    return store.operands.size() <= 2
               ? std::vector<int>{}
               : std::vector<int>(store.operands.begin() + 2,
                                  store.operands.end());
  }

  static std::vector<int> loadIndices(const GenericOperation &load) {
    return load.operands.size() <= 1
               ? std::vector<int>{}
               : std::vector<int>(load.operands.begin() + 1,
                                  load.operands.end());
  }

  std::vector<int> precomputeStoreToLoad(int parentId) {
    std::vector<int> overwritten;
    std::map<int, std::vector<int>> storesByRoot;
    const GenericOperation &parent =
        module.operations.at(static_cast<size_t>(parentId));
    for (int regionId : parent.regions)
      for (int blockId :
           module.regions.at(static_cast<size_t>(regionId)).blocks)
        for (int operationId :
             module.blocks.at(static_cast<size_t>(blockId)).operations) {
          const GenericOperation &operation =
              module.operations.at(static_cast<size_t>(operationId));
          if (operation.name == "memref.store") {
            if (operation.operands.size() < 2)
              throw std::runtime_error(
                  "pre-CV memref DSE: malformed memref.store");
            const int root = rootValue(operation.operands[1]);
            storesByRoot[root].push_back(operationId);
            overwritten.push_back(root);
            continue;
          }
          if (operation.name == "memref.load") {
            if (operation.operands.empty() || operation.results.size() != 1)
              throw std::runtime_error(
                  "pre-CV memref DSE: malformed memref.load");
            const int root = rootValue(operation.operands.front());
            const std::vector<int> indices = loadIndices(operation);
            const std::vector<int> &stores = storesByRoot[root];
            for (auto store = stores.rbegin(); store != stores.rend(); ++store) {
              const GenericOperation &candidate = module.operations.at(
                  static_cast<size_t>(*store));
              if ((candidate.operandTypes.size() > 1 &&
                   PreCVDSEIsScalarLikeMemRef(candidate.operandTypes[1])) ||
                  storeIndices(candidate) == indices) {
                loadedByStore[operationId] = *store;
                break;
              }
            }
            continue;
          }

          const std::vector<int> writes =
              PreCVDSEWriteEffectValues(operation);
          if (!writes.empty()) {
            for (int value : writes) {
              const int root = rootValue(value);
              storesByRoot[root].clear();
              overwritten.push_back(root);
            }
            continue;
          }
          if (operation.name == "func.call") {
            for (int argument : operation.operands) {
              const int root = rootValue(argument);
              storesByRoot[root].clear();
              overwritten.push_back(root);
            }
            continue;
          }
          if (!operation.regions.empty()) {
            const std::vector<int> nested =
                precomputeStoreToLoad(operationId);
            overwritten.insert(overwritten.end(), nested.begin(), nested.end());
            for (int root : nested)
              storesByRoot[root].clear();
          }
        }
    return overwritten;
  }

  void forwardLoads() {
    for (int operationId : preOrder) {
      const auto mapping = loadedByStore.find(operationId);
      if (mapping == loadedByStore.end() || !isAttached(operationId))
        continue;
      const GenericOperation load =
          module.operations.at(static_cast<size_t>(operationId));
      const GenericOperation &store =
          module.operations.at(static_cast<size_t>(mapping->second));
      if (load.results.size() != 1 || store.operands.empty())
        throw std::runtime_error(
            "pre-CV memref DSE: invalid store-to-load forwarding");
      rewriter.replaceAllUses(load.results.front(), store.operands.front());
      rewriter.removeFromBlock(load.blockId, load.id);
    }
  }

  bool valueHasUsers(int value) const {
    for (const GenericOperation &operation : module.operations) {
      if (!isAttached(operation.id))
        continue;
      if (std::find(operation.operands.begin(), operation.operands.end(),
                    value) != operation.operands.end())
        return true;
    }
    return false;
  }

  bool operationUseEmpty(const GenericOperation &operation) const {
    for (int result : operation.results)
      if (valueHasUsers(result))
        return false;
    return true;
  }

  void eraseUnusedHIVMLoads() {
    for (size_t allocationIndex = 0;
         allocationIndex < operationsOfAlloc.size(); ++allocationIndex) {
      std::vector<int> &users = operationsOfAlloc[allocationIndex];
      if (users.empty() ||
          module.operations.at(static_cast<size_t>(users.front())).name !=
              "memref.alloc")
        continue;
      while (!users.empty()) {
        const int operationId = users.back();
        if (!isAttached(operationId))
          break;
        const GenericOperation &load =
            module.operations.at(static_cast<size_t>(operationId));
        if (load.name != "hivm.hir.load" || !operationUseEmpty(load) ||
            load.dpsInits.empty() ||
            rootIndex(load.dpsInits.front()) != allocationIndex)
          break;
        users.pop_back();
        rewriter.removeFromBlock(load.blockId, load.id);
      }
    }
  }

  std::map<int, std::vector<int>> currentUsers() const {
    std::map<int, std::vector<int>> users;
    for (const GenericOperation &operation : module.operations) {
      if (!isAttached(operation.id))
        continue;
      for (int operand : operation.operands)
        users[operand].push_back(operation.id);
    }
    return users;
  }

  bool resultIsNotRead(int operationId, std::vector<int> &uses,
                       const std::map<int, std::vector<int>> &users,
                       std::set<int> &visiting) const {
    if (!visiting.insert(operationId).second)
      return false;
    std::vector<int> localUses;
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    for (int result : operation.results) {
      const auto found = users.find(result);
      if (found == users.end())
        continue;
      for (int userId : found->second) {
        const GenericOperation &user =
            module.operations.at(static_cast<size_t>(userId));
        if (user.name == "func.call" || PreCVDSEIsReturnLike(user.name) ||
            !user.regions.empty()) {
          visiting.erase(operationId);
          return false;
        }
        if (user.name == "memref.dealloc" ||
            (user.results.empty() && user.regions.empty() &&
             !PreCVDSEHasReadEffect(user))) {
          localUses.push_back(userId);
          continue;
        }
        if (user.name == "memref.subview") {
          std::vector<int> nested;
          if (!resultIsNotRead(userId, nested, users, visiting)) {
            visiting.erase(operationId);
            return false;
          }
          localUses.insert(localUses.end(), nested.begin(), nested.end());
          localUses.push_back(userId);
          continue;
        }
        visiting.erase(operationId);
        return false;
      }
    }
    visiting.erase(operationId);
    uses.insert(uses.end(), localUses.begin(), localUses.end());
    return true;
  }

  void eraseDeadAllocAndStores() {
    const std::map<int, std::vector<int>> users = currentUsers();
    std::vector<int> candidates;
    std::set<int> seen;
    for (int operationId : preOrder) {
      const GenericOperation &operation =
          module.operations.at(static_cast<size_t>(operationId));
      if (operation.name != "memref.alloc")
        continue;
      std::vector<int> uses;
      std::set<int> visiting;
      if (!resultIsNotRead(operationId, uses, users, visiting))
        continue;
      for (int use : uses)
        if (seen.insert(use).second)
          candidates.push_back(use);
      if (seen.insert(operationId).second)
        candidates.push_back(operationId);
    }
    for (int operationId : candidates) {
      if (!isAttached(operationId))
        continue;
      const GenericOperation &operation =
          module.operations.at(static_cast<size_t>(operationId));
      if (operationUseEmpty(operation))
        rewriter.removeFromBlock(operation.blockId, operation.id);
    }
  }

  GenericModule &module;
  int functionId;
  bool regBased;
  GenericRewriter rewriter;
  std::vector<int> preOrder;
  std::map<int, size_t> valueToIndex;
  std::vector<int> valueList;
  std::optional<PreCVDSEUnionFind> disjointSet;
  std::vector<std::vector<int>> operationsOfAlloc;
  std::map<int, int> loadedByStore;
};

inline bool PreCVDSEIsRegBasedModule(const GenericModule &module) {
  if (module.operations.empty())
    return false;
  const GenericOperation &root = module.operations.front();
  const std::string text = root.properties + " " + root.attributes;
  return text.find("Ascend310B") != std::string::npos ||
         text.find("Ascend910_95") != std::string::npos ||
         text.find("Ascend950PR") != std::string::npos ||
         text.find("Ascend950DT") != std::string::npos;
}

inline GenericModule RunPreCVMemrefDeadStoreElimination(
    GenericModule module) {
  ApplyOperationSemanticsToAll(module.operations);
  std::vector<int> functions;
  for (const GenericOperation &operation : module.operations)
    if (operation.name == "func.func")
      functions.push_back(operation.id);
  const bool regBased = PreCVDSEIsRegBasedModule(module);
  for (int function : functions)
    PreCVMemrefDSEFunctionDriver(module, function, regBased).run();
  ApplyOperationSemanticsToAll(module.operations);
  return CompactGenericModule(std::move(module));
}

} // namespace cvub

#endif
