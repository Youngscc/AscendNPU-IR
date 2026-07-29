#ifndef UB_OVERFLOW_MODEL_CPP_PRE_CV_MARK_MULTI_BUFFER_HPP
#define UB_OVERFLOW_MODEL_CPP_PRE_CV_MARK_MULTI_BUFFER_HPP

#include "../../include/ub_overflow_model/api.hpp"
#include "../ir/generic_rewriter.hpp"
#include "../ir/memref_type_model.hpp"
#include "../ir/operation_folder.hpp"
#include "../pipeline/buffer_topology.hpp"

namespace cvub {

struct PreCVMarkMultiBufferOptions {
  bool disableAutoCVWorkSpaceManage = false;
  bool enableAuto = false;
  bool limitAutoMultiBufferOnlyForLocalBuffer = false;
  MultiBufferStrategy limitAutoMultiBufferOfLocalBuffer =
      MultiBufferStrategy::CubeNoL0C;
  MultiBufferStrategy limitMixAutoMultiBufferBuffer =
      MultiBufferStrategy::OnlyCube;
  unsigned workspaceMultiBufferNum = 4;
};

inline std::string PreCVEnumInner(const std::string &text) {
  const size_t open = text.rfind('<');
  const size_t close = text.rfind('>');
  if (open == std::string::npos || close == std::string::npos || open >= close)
    return text;
  return text.substr(open + 1, close - open - 1);
}

inline bool PreCVHasAttribute(const std::string &dictionary,
                              const std::string &name) {
  if (dictionary.size() < 2)
    return false;
  for (const std::string &entry :
       splitTopLevel(dictionary.substr(1, dictionary.size() - 2))) {
    const size_t equal = entry.find('=');
    if (trim(entry.substr(0, equal)) == name)
      return true;
  }
  return false;
}

inline bool PreCVIsAllocLike(const GenericOperation &operation) {
  return operation.name == "memref.alloc" ||
         operation.name == "memref.alloca" ||
         operation.name == "memref_ext.alloc_workspace";
}

inline std::optional<AddressSpace>
PreCVExplicitAddressSpace(const std::string &type) {
  std::string lowered = type;
  std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                 [](unsigned char character) {
                   return static_cast<char>(std::tolower(character));
                 });
  const std::string prefix = "#hivm.address_space<";
  const size_t begin = lowered.find(prefix);
  if (begin == std::string::npos)
    return std::nullopt;
  const size_t end = lowered.find('>', begin + prefix.size());
  if (end == std::string::npos)
    return std::nullopt;
  const std::string value =
      lowered.substr(begin + prefix.size(), end - begin - prefix.size());
  if (value == "zero")
    return AddressSpace::Zero;
  if (value == "gm")
    return AddressSpace::GM;
  if (value == "cbuf")
    return AddressSpace::L1;
  if (value == "ca")
    return AddressSpace::L0A;
  if (value == "cb")
    return AddressSpace::L0B;
  if (value == "cc")
    return AddressSpace::L0C;
  if (value == "ub")
    return AddressSpace::UB;
  return std::nullopt;
}

inline bool PreCVIsWorkspaceViewLike(const std::string &name) {
  static const std::set<std::string> names = {
      "memref.cast",
      "memref.collapse_shape",
      "memref.expand_shape",
      "memref.extract_strided_metadata",
      "memref.memory_space_cast",
      "memref.reinterpret_cast",
      "memref.reshape",
      "memref.subview",
      "memref.view"};
  return names.count(name) != 0;
}

inline bool PreCVIsLocalCastLike(const std::string &name) {
  static const std::set<std::string> names = {
      "memref.cast",          "memref.collapse_shape",
      "memref.expand_shape", "memref.memory_space_cast",
      "memref.reinterpret_cast", "memref.reshape",
      "memref.transpose"};
  return names.count(name) != 0;
}

class PreCVMarkMultiBufferModel {
public:
  PreCVMarkMultiBufferModel(GenericModule input,
                            PreCVMarkMultiBufferOptions inputOptions)
      : module(std::move(input)), options(inputOptions), rewriter(module) {
    indexValues();
  }

  GenericModule Run() {
    // The native pipeline omits this pass when workspace management is off;
    // the pass itself returns before constructing a greedy driver when
    // enableAuto is false. Both branches are exact identities.
    if (options.disableAutoCVWorkSpaceManage || !options.enableAuto)
      return module;

    const std::vector<int> functions = functionIds();
    for (int functionId : functions) {
      if (isHost(module.operations.at(static_cast<size_t>(functionId))))
        continue;
      runFunction(functionId);
      RunGreedyOperationFolder(module, functionId);
    }
    module = CompactGenericModule(std::move(module));
    ApplyOperationSemanticsToAll(module.operations);
    return module;
  }

private:
  void indexValues() {
    for (const GenericOperation &operation : module.operations)
      for (size_t index = 0; index < operation.results.size(); ++index)
        definitions[operation.results[index]] = operation.id;
    for (const GenericBlock &block : module.blocks)
      for (size_t index = 0; index < block.arguments.size(); ++index)
        blockArguments[block.arguments[index]] =
            std::make_pair(block.id, index);
  }

  std::vector<int> functionIds() const {
    std::vector<int> result;
    for (const GenericOperation &operation : module.operations)
      if (operation.name == "func.func")
        result.push_back(operation.id);
    return result;
  }

  static bool isHost(const GenericOperation &function) {
    return PreCVEnumInner(FindDictionaryValue(
               function.attributes, "hacc.function_kind")) == "HOST";
  }

  static bool isMix(const GenericOperation &function) {
    const std::string core = PreCVEnumInner(FindDictionaryValue(
        function.attributes, "hivm.func_core_type"));
    return core == "MIX" ||
           PreCVHasAttribute(function.attributes, "hivm.part_of_mix");
  }

  static std::optional<int64_t>
  integerAttribute(const GenericOperation &operation,
                   const std::string &name) {
    std::string value = FindDictionaryValue(operation.attributes, name);
    if (value.empty())
      value = FindDictionaryValue(operation.properties, name);
    if (value.empty())
      return std::nullopt;
    const size_t separator = value.find(':');
    if (separator != std::string::npos)
      value.resize(separator);
    value = trim(std::move(value));
    try {
      size_t consumed = 0;
      const int64_t parsed = std::stoll(value, &consumed, 0);
      return consumed == value.size() ? std::optional<int64_t>(parsed)
                                      : std::nullopt;
    } catch (const std::exception &) {
      return std::nullopt;
    }
  }

  const GenericOperation *definition(int value) const {
    const auto found = definitions.find(value);
    return found == definitions.end()
               ? nullptr
               : &module.operations.at(static_cast<size_t>(found->second));
  }

  size_t resultIndex(const GenericOperation &operation, int value) const {
    const auto found =
        std::find(operation.results.begin(), operation.results.end(), value);
    return found == operation.results.end()
               ? operation.results.size()
               : static_cast<size_t>(
                     std::distance(operation.results.begin(), found));
  }

  const GenericOperation *terminator(int regionId) const {
    if (regionId < 0 || static_cast<size_t>(regionId) >= module.regions.size())
      return nullptr;
    const GenericRegion &region =
        module.regions.at(static_cast<size_t>(regionId));
    if (region.blocks.empty())
      return nullptr;
    const GenericBlock &block =
        module.blocks.at(static_cast<size_t>(region.blocks.front()));
    return block.operations.empty()
               ? nullptr
               : &module.operations.at(
                     static_cast<size_t>(block.operations.back()));
  }

  bool hasExistingMark(const GenericOperation &allocation) const {
    if (allocation.results.empty())
      return false;
    const int value = allocation.results.front();
    for (const GenericOperation &operation : module.operations) {
      if (!IsGreedyOperationFolderAttached(module, operation.id))
        continue;
      if (operation.name != "annotation.mark" || operation.operands.empty() ||
          operation.operands.front() != value)
        continue;
      std::string count =
          FindDictionaryValue(operation.attributes, "hivm.multi_buffer");
      if (count.empty())
        count =
            FindDictionaryValue(operation.properties, "hivm.multi_buffer");
      if (count.empty())
        continue;
      const size_t separator = count.find(':');
      try {
        if (std::stoll(trim(count.substr(0, separator))) < 1)
          throw std::runtime_error("hivm.multi_buffer must be at least one");
      } catch (const std::exception &) {
        throw std::runtime_error(
            "pre-CV MarkMultiBuffer: illegal existing multi-buffer mark");
      }
      return true;
    }
    return false;
  }

  bool mark(GenericOperation &allocation, unsigned count,
            bool preload = false) {
    if (allocation.results.empty() || allocation.resultTypes.empty())
      return false;
    if (hasExistingMark(allocation))
      return false;
    // GenericIR's normalized form retains the inherent `effects` property in
    // both dictionaries after parsing native generic MLIR. Mirror that form
    // for marks created by the lightweight rewrite.
    const std::string attributes =
        "{effects = [\"write\"], hivm.multi_buffer = " +
        std::to_string(count) + " : i32" +
        (preload ? ", hivm.preload_local_buffer = 1 : i32}" : "}");
    const int markId = rewriter.createOperation(
        allocation.parentId, allocation.regionId, allocation.blockId,
        "annotation.mark", {}, {allocation.results.front()},
        {allocation.resultTypes.front()}, "{effects = [\"write\"]}",
        attributes);
    const GenericBlock &block =
        module.blocks.at(static_cast<size_t>(allocation.blockId));
    const auto position =
        std::find(block.operations.begin(), block.operations.end(),
                  allocation.id);
    if (position == block.operations.end())
      throw std::runtime_error(
          "pre-CV MarkMultiBuffer: allocation is detached");
    const size_t insertion = static_cast<size_t>(
        std::distance(block.operations.begin(), position)) + 1;
    rewriter.insertToBlock(allocation.blockId, insertion, markId);
    return true;
  }

  GenericOperation *traceWorkspace(int value) {
    std::set<int> visited;
    while (visited.insert(value).second) {
      GenericOperation *operation = const_cast<GenericOperation *>(definition(value));
      if (!operation)
        return nullptr;
      if (operation->name == "memref_ext.alloc_workspace")
        return operation;
      if ((operation->name == "bufferization.to_tensor" ||
           PreCVIsWorkspaceViewLike(operation->name)) &&
          !operation->operands.empty()) {
        value = operation->operands.front();
        continue;
      }
      return nullptr;
    }
    return nullptr;
  }

  std::vector<int> tracebackStep(int value) const {
    std::vector<int> result;
    const auto blockArgument = blockArguments.find(value);
    if (blockArgument != blockArguments.end()) {
      const GenericBlock &block = module.blocks.at(
          static_cast<size_t>(blockArgument->second.first));
      const GenericRegion &region =
          module.regions.at(static_cast<size_t>(block.regionId));
      const GenericOperation &parent = module.operations.at(
          static_cast<size_t>(region.parentOperation));
      const size_t argument = blockArgument->second.second;
      if (parent.name == "scf.for" && argument > 0) {
        const size_t resultCount = parent.results.size();
        const size_t initBegin = parent.operands.size() >= resultCount
                                     ? parent.operands.size() - resultCount
                                     : parent.operands.size();
        const size_t tied = argument - 1;
        const GenericOperation *yield =
            parent.regions.empty() ? nullptr : terminator(parent.regions[0]);
        if (tied < resultCount && initBegin + tied < parent.operands.size() &&
            yield && yield->name == "scf.yield" &&
            tied < yield->operands.size()) {
          result.push_back(parent.operands[initBegin + tied]);
          result.push_back(yield->operands[tied]);
        }
      } else if (parent.name == "scf.while" && region.ordinal == 0 &&
                 argument < parent.operands.size()) {
        // WhileOp::getRegionIterArgs exposes only the before-region arguments;
        // getTiedLoopInit maps them positionally to the loop inits.
        result.push_back(parent.operands[argument]);
      }
    }

    const GenericOperation *operation = definition(value);
    if (!operation)
      return result;
    const size_t index = resultIndex(*operation, value);
    if (index >= operation->results.size())
      return result;

    if (PreCVIsLocalCastLike(operation->name) &&
        !operation->operands.empty()) {
      result.push_back(operation->operands.front());
    } else if (operation->name == "unrealized_conversion_cast" &&
               index < operation->operands.size()) {
      result.push_back(operation->operands[index]);
    } else if (operation->name == "scf.for") {
      const size_t resultCount = operation->results.size();
      const size_t initBegin = operation->operands.size() >= resultCount
                                   ? operation->operands.size() - resultCount
                                   : operation->operands.size();
      const GenericOperation *yield = operation->regions.empty()
                                          ? nullptr
                                          : terminator(operation->regions[0]);
      if (initBegin + index < operation->operands.size() && yield &&
          yield->name == "scf.yield" && index < yield->operands.size()) {
        result.push_back(operation->operands[initBegin + index]);
        result.push_back(yield->operands[index]);
      }
    } else if (operation->name == "scf.if" &&
               operation->regions.size() == 2) {
      const GenericOperation *thenYield = terminator(operation->regions[0]);
      const GenericOperation *elseYield = terminator(operation->regions[1]);
      if (thenYield && elseYield && thenYield->name == "scf.yield" &&
          elseYield->name == "scf.yield" &&
          index < thenYield->operands.size() &&
          index < elseYield->operands.size()) {
        result.push_back(thenYield->operands[index]);
        result.push_back(elseYield->operands[index]);
      }
    }

    if (!result.empty())
      return result;
    if ((operation->name == "memref.view" ||
         operation->name == "memref.subview") &&
        !operation->operands.empty())
      result.push_back(operation->operands.front());
    return result;
  }

  GenericOperation *traceLocalAllocation(int value) {
    std::vector<int> values = {value};
    int loopBound = 256;
    while (!values.empty() &&
           std::any_of(values.begin(), values.end(), [&](int candidate) {
             const GenericOperation *operation = definition(candidate);
             return !operation || !PreCVIsAllocLike(*operation);
           })) {
      const auto unresolved =
          std::find_if(values.begin(), values.end(), [&](int candidate) {
            const GenericOperation *operation = definition(candidate);
            return !operation || !PreCVIsAllocLike(*operation);
          });
      if (unresolved == values.end())
        break;
      const std::vector<int> upward = tracebackStep(*unresolved);
      if (upward.empty())
        break;
      const size_t position =
          static_cast<size_t>(std::distance(values.begin(), unresolved));
      values.erase(values.begin() + static_cast<std::ptrdiff_t>(position));
      values.insert(values.begin() + static_cast<std::ptrdiff_t>(position),
                    upward.begin(), upward.end());
      if (loopBound-- < 0)
        break;
    }
    if (values.empty())
      return nullptr;
    return const_cast<GenericOperation *>(definition(values.front()));
  }

  static bool isLoopLike(const GenericOperation &operation) {
    return operation.name == "scf.for" || operation.name == "scf.while" ||
           operation.name == "affine.for" ||
           operation.name == "scf.parallel" ||
           operation.name == "scf.forall";
  }

  const GenericOperation *closestParentLoop(int operationId) const {
    int parent =
        module.operations.at(static_cast<size_t>(operationId)).parentId;
    while (parent >= 0) {
      const GenericOperation &operation =
          module.operations.at(static_cast<size_t>(parent));
      if (isLoopLike(operation))
        return &operation;
      parent = operation.parentId;
    }
    return nullptr;
  }

  bool isTerminator(const GenericOperation &operation) const {
    if (!IsGreedyOperationFolderAttached(module, operation.id) ||
        operation.blockId < 0)
      return false;
    const GenericBlock &block =
        module.blocks.at(static_cast<size_t>(operation.blockId));
    return !block.operations.empty() &&
           block.operations.back() == operation.id;
  }

  bool isConsumedInLoop(int value, const GenericOperation &loop) const {
    for (const GenericOperation &user : module.operations) {
      if (!IsGreedyOperationFolderAttached(module, user.id) ||
          isTerminator(user) ||
          user.name == "annotation.mark" ||
          std::find(user.operands.begin(), user.operands.end(), value) ==
              user.operands.end())
        continue;
      int ancestor = user.id;
      while (ancestor >= 0) {
        const GenericOperation &operation =
            module.operations.at(static_cast<size_t>(ancestor));
        if (operation.id == loop.id)
          return true;
        if (operation.name == "scf.for" || operation.name == "scf.while")
          break;
        ancestor = operation.parentId;
      }
    }
    return false;
  }

  std::vector<int> loopYieldedValues(const GenericOperation &loop) const {
    if (loop.name == "scf.for" && !loop.regions.empty()) {
      const GenericOperation *yield = terminator(loop.regions[0]);
      return yield && yield->name == "scf.yield" ? yield->operands
                                                   : std::vector<int>{};
    }
    if (loop.name == "scf.while" && loop.regions.size() >= 2) {
      const GenericOperation *yield = terminator(loop.regions[1]);
      return yield && yield->name == "scf.yield" ? yield->operands
                                                   : std::vector<int>{};
    }
    return {};
  }

  const GenericOperation *getParentLoopImpl(
      int value, const GenericOperation *consumerLoop) const {
    const GenericOperation *valueDefinition = definition(value);
    if (!valueDefinition)
      throw std::runtime_error(
          "pre-CV MarkMultiBuffer: loop value has no defining operation");
    const GenericOperation *parentLoop =
        closestParentLoop(valueDefinition->id);
    if (!parentLoop)
      return consumerLoop;
    if (isConsumedInLoop(value, *parentLoop))
      consumerLoop = parentLoop;

    const std::vector<int> yielded = loopYieldedValues(*parentLoop);
    if (yielded.empty())
      return consumerLoop ? consumerLoop : parentLoop;
    const auto found = std::find(yielded.begin(), yielded.end(), value);
    if (found != yielded.end()) {
      const size_t index =
          static_cast<size_t>(std::distance(yielded.begin(), found));
      if (index >= parentLoop->results.size())
        return consumerLoop ? consumerLoop : parentLoop;
      return getParentLoopImpl(parentLoop->results[index], consumerLoop);
    }

    const GenericOperation *parentIf = nullptr;
    int ancestor = valueDefinition->parentId;
    while (ancestor >= 0) {
      const GenericOperation &operation =
          module.operations.at(static_cast<size_t>(ancestor));
      if (operation.name == "scf.if") {
        parentIf = &operation;
        break;
      }
      ancestor = operation.parentId;
    }
    if (!parentIf || parentIf->results.empty() ||
        parentIf->regions.size() != 2)
      return consumerLoop ? consumerLoop : parentLoop;
    for (int regionId : parentIf->regions) {
      const GenericOperation *yield = terminator(regionId);
      if (!yield || yield->name != "scf.yield")
        continue;
      const auto yieldedValue =
          std::find(yield->operands.begin(), yield->operands.end(), value);
      if (yieldedValue == yield->operands.end())
        continue;
      const size_t index = static_cast<size_t>(
          std::distance(yield->operands.begin(), yieldedValue));
      if (index < parentIf->results.size())
        return getParentLoopImpl(parentIf->results[index], consumerLoop);
    }
    return consumerLoop ? consumerLoop : parentLoop;
  }

  bool hasSupportedParentLoops(const GenericOperation &allocation) const {
    if (allocation.results.empty())
      return false;
    const GenericOperation *parentLoop =
        getParentLoopImpl(allocation.results.front(), nullptr);
    if (!parentLoop)
      return false;
    while (parentLoop) {
      if (parentLoop->name != "scf.for" && parentLoop->name != "scf.while")
        return false;
      parentLoop = closestParentLoop(parentLoop->id);
    }
    return true;
  }

  bool markLocal(const GenericOperation &operation) {
    if (operation.operands.size() < 2 || operation.operandTypes.size() < 2 ||
        !IsMemRefType(operation.operandTypes[0]) ||
        !IsMemRefType(operation.operandTypes[1]))
      return false;
    const std::optional<AddressSpace> source =
        PreCVExplicitAddressSpace(operation.operandTypes[0]);
    const std::optional<AddressSpace> destination =
        PreCVExplicitAddressSpace(operation.operandTypes[1]);
    if (!source || !destination)
      return false;
    int candidate = -1;
    if (*source != AddressSpace::GM)
      candidate = operation.operands[0];
    else if (*destination != AddressSpace::GM)
      candidate = operation.operands[1];
    if (candidate < 0)
      return false;
    GenericOperation *allocation = traceLocalAllocation(candidate);
    if (!allocation || !hasSupportedParentLoops(*allocation))
      return false;
    return mark(*allocation, 2);
  }

  bool markWorkspace(const GenericOperation &operation) {
    const bool pureTensor =
        !operation.operandTypes.empty() &&
        std::all_of(operation.operandTypes.begin(),
                    operation.operandTypes.end(), [](const std::string &type) {
                      return !IsMemRefType(type);
                    }) &&
        std::all_of(operation.resultTypes.begin(), operation.resultTypes.end(),
                    [](const std::string &type) {
                      return type.find("tensor<") != std::string::npos;
                    });
    if (!pureTensor)
      return false;
    if (operation.dpsInits.size() != 1)
      return false;
    GenericOperation *allocation = traceWorkspace(operation.dpsInits.front());
    if (!allocation || allocation->parentId < 0)
      return false;
    const GenericOperation &parent = module.operations.at(
        static_cast<size_t>(allocation->parentId));
    if (!isLoopLike(parent))
      return false;
    return mark(*allocation, options.workspaceMultiBufferNum);
  }

  bool usedByScopeOrDescendant(int value) const {
    for (const GenericOperation &user : module.operations) {
      if (!IsGreedyOperationFolderAttached(module, user.id) ||
          std::find(user.operands.begin(), user.operands.end(), value) ==
              user.operands.end())
        continue;
      if (user.name == "scope.scope")
        return true;
      int parent = user.parentId;
      while (parent >= 0) {
        const GenericOperation &ancestor =
            module.operations.at(static_cast<size_t>(parent));
        if (ancestor.name == "scope.scope")
          return true;
        parent = ancestor.parentId;
      }
    }
    return false;
  }

  bool markScope(const GenericOperation &scope) {
    const std::optional<int64_t> preload =
        integerAttribute(scope, "hivm.preload_num");
    if (!preload || *preload == 0)
      return false;
    std::string core =
        FindDictionaryValue(scope.attributes, "hivm.loop_core_type");
    if (core.empty())
      core = FindDictionaryValue(scope.properties, "hivm.loop_core_type");
    core = PreCVEnumInner(core);
    if (core.empty() || core == "CUBE")
      return false;
    if (scope.regions.empty())
      return false;
    const GenericOperation *returnOperation = terminator(scope.regions.front());
    if (!returnOperation || returnOperation->name != "scope.return")
      return false;

    bool changed = false;
    for (size_t index = 0; index < returnOperation->operands.size(); ++index) {
      if (index >= scope.results.size() ||
          !usedByScopeOrDescendant(scope.results[index]))
        continue;
      GenericOperation *allocation = const_cast<GenericOperation *>(
          definition(returnOperation->operands[index]));
      if (!allocation || allocation->name != "memref.alloc" ||
          allocation->resultTypes.empty())
        continue;
      const std::optional<AddressSpace> addressSpace =
          PreCVExplicitAddressSpace(allocation->resultTypes.front());
      if (!addressSpace || *addressSpace == AddressSpace::GM)
        continue;
      // The native pattern returns failure immediately when a relevant output
      // is already marked. Earlier marks from the same invocation remain in
      // the IR, matching PatternRewriter's actual mutation order.
      if (hasExistingMark(*allocation))
        return false;
      changed = mark(*allocation, 4, true) || changed;
    }
    // The native pattern reports success after passing its scope-level gate,
    // even when no output is eligible. Reachable fixtures always include at
    // least one mutation; returning `changed` avoids a non-converging model
    // worklist for malformed/unreachable scopes.
    return changed;
  }

  void runFunction(int functionId) {
    ApplyOperationSemanticsToAll(module.operations);
    const GenericOperation &function =
        module.operations.at(static_cast<size_t>(functionId));
    const bool mix = isMix(function);
    std::vector<int> postOrder;
    std::function<void(int)> collect = [&](int operationId) {
      const GenericOperation &operation =
          module.operations.at(static_cast<size_t>(operationId));
      for (int regionId : operation.regions)
        for (int blockId :
             module.regions.at(static_cast<size_t>(regionId)).blocks)
          for (int child :
               module.blocks.at(static_cast<size_t>(blockId)).operations) {
            collect(child);
            postOrder.push_back(child);
          }
    };
    collect(functionId);

    for (int operationId : postOrder) {
      GenericOperation &operation =
          module.operations.at(static_cast<size_t>(operationId));
      if (operation.name == "scope.scope") {
        markScope(operation);
        continue;
      }
      const bool cubePattern =
          operation.name == "hivm.hir.nd2nz" ||
          operation.name == "hivm.hir.fixpipe";
      const bool vectorPattern = operation.name == "hivm.hir.load" ||
                                 operation.name == "hivm.hir.store";
      if (cubePattern &&
          (!mix || options.limitMixAutoMultiBufferBuffer !=
                       MultiBufferStrategy::OnlyVector) &&
          (operation.name != "hivm.hir.fixpipe" ||
           options.limitAutoMultiBufferOfLocalBuffer !=
               MultiBufferStrategy::CubeNoL0C))
        markLocal(operation);
      if (vectorPattern &&
          (!mix || options.limitMixAutoMultiBufferBuffer !=
                       MultiBufferStrategy::OnlyCube))
        markLocal(operation);

      if (!options.limitAutoMultiBufferOnlyForLocalBuffer && mix &&
          (operation.name == "hivm.hir.store" ||
           operation.name == "hivm.hir.fixpipe"))
        markWorkspace(operation);
    }
    RunGreedyArithIdentityFolds(module, functionId);
  }

  GenericModule module;
  PreCVMarkMultiBufferOptions options;
  GenericRewriter rewriter;
  std::map<int, int> definitions;
  std::map<int, std::pair<int, size_t>> blockArguments;
};

inline GenericModule RunPreCVMarkMultiBuffer(
    GenericModule module, const PreCVMarkMultiBufferOptions &options = {}) {
  return PreCVMarkMultiBufferModel(std::move(module), options).Run();
}

} // namespace cvub

#endif
