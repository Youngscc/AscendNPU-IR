#ifndef CVPIPELINE_UB_MODEL_CPP_CROSS_CORE_GSS_HPP
#define CVPIPELINE_UB_MODEL_CPP_CROSS_CORE_GSS_HPP

#include "../ir/generic_analysis.hpp"
#include "mark_real_core_type.hpp"

#include <array>
#include <queue>

namespace cvub {

inline std::optional<int> GetFFTSBaseAddressArgument(
    const GenericModule &module, const GenericOperation &function) {
  if (function.regions.size() != 1)
    return std::nullopt;
  const GenericRegion &region =
      module.regions.at(static_cast<size_t>(function.regions.front()));
  if (region.blocks.empty())
    return std::nullopt;
  const GenericBlock &entry =
      module.blocks.at(static_cast<size_t>(region.blocks.front()));

  std::string argumentAttributes =
      FindDictionaryValue(function.properties, "arg_attrs");
  if (argumentAttributes.empty())
    argumentAttributes =
        FindDictionaryValue(function.attributes, "arg_attrs");
  if (argumentAttributes.size() < 2 || argumentAttributes.front() != '[' ||
      argumentAttributes.back() != ']')
    return std::nullopt;
  const std::vector<std::string> attributes = splitTopLevel(
      argumentAttributes.substr(1, argumentAttributes.size() - 2));
  for (size_t index = 0;
       index < attributes.size() && index < entry.arguments.size(); ++index)
    if (attributes[index].find("ffts_base_address") != std::string::npos)
      return entry.arguments[index];
  return std::nullopt;
}

struct CrossCoreSyncDependency {
  int producer = -1;
  int consumer = -1;
  SplitMixCoreType producerCore = SplitMixCoreType::Common;
  SplitMixCoreType consumerCore = SplitMixCoreType::Common;
};

struct CrossCoreMemoryAccess {
  std::vector<int> reads;
  std::vector<int> writes;
};

// Mirrors GraphSyncSolver/SyncSolverIRTranslator.cpp's tracebackMemVals
// path. In cross-core, memory-based mode, function arguments and
// memref_ext.alloc_workspace results are roots; local memref.alloc values are
// deliberately not roots.
class CrossCoreMemoryValueTracer {
public:
  CrossCoreMemoryValueTracer(
      const GenericModule &inputModule,
      const GenericModuleAnalysisSnapshot &inputAnalysis)
      : module(inputModule), analysis(inputAnalysis) {
    indexBlockArguments();
    indexBranchAliases();
  }

  std::vector<int> getMemoryOps(const std::vector<int> &values) {
    std::vector<int> roots;
    std::set<int> seen;
    for (int value : values)
      for (int root : tracebackMemValsCached(value))
        if (seen.insert(root).second)
          roots.push_back(root);
    return roots;
  }

private:
  struct BlockArgumentOwner {
    int block = -1;
    size_t index = 0;
  };

  void indexBlockArguments() {
    for (const GenericBlock &block : module.blocks)
      for (size_t index = 0; index < block.arguments.size(); ++index)
        blockArguments[block.arguments[index]] = {block.id, index};
  }

  void addBranchAliases(int blockId, const std::vector<int> &operands) {
    if (blockId < 0 || static_cast<size_t>(blockId) >= module.blocks.size())
      return;
    const GenericBlock &block =
        module.blocks.at(static_cast<size_t>(blockId));
    for (size_t index = 0;
         index < block.arguments.size() && index < operands.size(); ++index)
      blockArgAliases[block.arguments[index]].push_back(operands[index]);
  }

  void indexBranchAliases() {
    for (const GenericOperation &operation : module.operations) {
      if (operation.name == "cf.br" && !operation.successors.empty()) {
        addBranchAliases(operation.successors.front(), operation.operands);
        continue;
      }
      if (operation.name != "cf.cond_br" || operation.successors.size() != 2)
        continue;
      const std::vector<size_t> segments =
          OperandSegmentSizes(operation.properties);
      if (segments.size() != 3 ||
          segments[0] + segments[1] + segments[2] !=
              operation.operands.size())
        throw std::runtime_error(
            "CrossCoreGSS: malformed cf.cond_br operand segments");
      const size_t trueBegin = segments[0];
      const size_t falseBegin = trueBegin + segments[1];
      addBranchAliases(
          operation.successors[0],
          std::vector<int>(operation.operands.begin() +
                               static_cast<std::ptrdiff_t>(trueBegin),
                           operation.operands.begin() +
                               static_cast<std::ptrdiff_t>(falseBegin)));
      addBranchAliases(
          operation.successors[1],
          std::vector<int>(operation.operands.begin() +
                               static_cast<std::ptrdiff_t>(falseBegin),
                           operation.operands.end()));
    }
  }

  const GenericOperation *terminator(const GenericOperation &operation,
                                     size_t regionIndex) const {
    if (regionIndex >= operation.regions.size())
      return nullptr;
    const GenericRegion &region = module.regions.at(
        static_cast<size_t>(operation.regions[regionIndex]));
    if (region.blocks.empty())
      return nullptr;
    const GenericBlock &block =
        module.blocks.at(static_cast<size_t>(region.blocks.back()));
    if (block.operations.empty())
      return nullptr;
    return &module.operations.at(
        static_cast<size_t>(block.operations.back()));
  }

  std::vector<int> tracebackBlockArgumentStep(int value) const {
    std::vector<int> aliases;
    const auto ownerIt = blockArguments.find(value);
    if (ownerIt == blockArguments.end())
      return aliases;
    const BlockArgumentOwner owner = ownerIt->second;
    const GenericBlock &block =
        module.blocks.at(static_cast<size_t>(owner.block));
    const GenericRegion &region =
        module.regions.at(static_cast<size_t>(block.regionId));
    const GenericOperation &parent = module.operations.at(
        static_cast<size_t>(region.parentOperation));

    if (parent.name == "scf.for" && owner.index > 0) {
      const size_t init = owner.index + 2;
      if (init < parent.operands.size())
        aliases.push_back(parent.operands[init]);
      if (const GenericOperation *yield = terminator(parent, 0)) {
        const size_t yielded = owner.index - 1;
        if (yielded < yield->operands.size())
          aliases.push_back(yield->operands[yielded]);
      }
    } else if (parent.name == "scf.while") {
      if (region.ordinal == 0) {
        if (owner.index < parent.operands.size())
          aliases.push_back(parent.operands[owner.index]);
        if (const GenericOperation *yield = terminator(parent, 1))
          if (owner.index < yield->operands.size())
            aliases.push_back(yield->operands[owner.index]);
      } else if (const GenericOperation *condition = terminator(parent, 0)) {
        const size_t argument = owner.index + 1;
        if (argument < condition->operands.size())
          aliases.push_back(condition->operands[argument]);
      }
    }

    const auto branch = blockArgAliases.find(value);
    if (branch != blockArgAliases.end())
      aliases.insert(aliases.end(), branch->second.begin(),
                     branch->second.end());
    return aliases;
  }

  std::vector<int> operationAliasInfo(const GenericOperation &operation,
                                      size_t resultIndex) const {
    if (operation.name == "arith.select" && operation.operands.size() >= 3)
      return {operation.operands[1], operation.operands[2]};

    static const std::set<std::string> viewLikeAliases = {
        "bufferization.to_memref", "bufferization.to_buffer",
        "bufferization.to_tensor", "hivm.hir.bitcast", "memref.cast",
        "memref.collapse_shape", "memref.expand_shape",
        "memref.memory_space_cast", "memref.reinterpret_cast",
        "memref.reshape", "memref.subview", "memref.transpose",
        "memref.view", "tensor.collapse_shape", "tensor.expand_shape",
        "tensor.extract_slice"};
    if (viewLikeAliases.count(operation.name) != 0 &&
        !operation.operands.empty())
      return {operation.operands.front()};
    if (operation.name == "memref.extract_strided_metadata" &&
        resultIndex == 0 && !operation.operands.empty())
      return {operation.operands.front()};
    if (operation.name == "scope.scope") {
      if (const GenericOperation *returnOp = terminator(operation, 0))
        if (resultIndex < returnOp->operands.size())
          return {returnOp->operands[resultIndex]};
    }
    return {};
  }

  std::vector<int> tracebackResultStep(const GenericOperation &operation,
                                       size_t resultIndex) const {
    if (operation.name == "scf.if") {
      std::vector<int> yielded;
      for (size_t region = 0; region < operation.regions.size(); ++region)
        if (const GenericOperation *yield = terminator(operation, region))
          if (resultIndex < yield->operands.size())
            yielded.push_back(yield->operands[resultIndex]);
      return yielded;
    }
    if (operation.name == "scf.for") {
      if (const GenericOperation *yield = terminator(operation, 0))
        if (resultIndex < yield->operands.size())
          return {yield->operands[resultIndex]};
      return {};
    }
    if (operation.name == "scf.while") {
      std::vector<int> yielded;
      if (const GenericOperation *condition = terminator(operation, 0)) {
        const size_t argument = resultIndex + 1;
        if (argument < condition->operands.size())
          yielded.push_back(condition->operands[argument]);
      }
      if (const GenericOperation *yield = terminator(operation, 1))
        if (resultIndex < yield->operands.size())
          yielded.push_back(yield->operands[resultIndex]);
      return yielded;
    }

    if (std::vector<int> aliases =
            operationAliasInfo(operation, resultIndex);
        !aliases.empty())
      return aliases;
    if (IsDestinationStyleOp(operation.name))
      return operation.dpsInits;
    return {};
  }

  std::vector<int> tracebackMemVals(int value) const {
    std::queue<int> worklist;
    std::set<int> visited;
    std::vector<int> roots;
    std::set<int> seenRoots;
    worklist.push(value);
    visited.insert(value);
    while (!worklist.empty()) {
      const int current = worklist.front();
      worklist.pop();
      std::vector<int> next;
      const auto blockArgument = blockArguments.find(current);
      if (blockArgument != blockArguments.end()) {
        next = tracebackBlockArgumentStep(current);
      } else {
        const GenericOperation *definition = analysis.definingOperation(current);
        if (definition) {
          const GenericOperation &operation = *definition;
          const auto result =
              std::find(operation.results.begin(), operation.results.end(),
                        current);
          if (result != operation.results.end())
            next = tracebackResultStep(
                operation, static_cast<size_t>(result -
                                               operation.results.begin()));
        }
      }
      if (!next.empty()) {
        for (int alias : next)
          if (visited.insert(alias).second)
            worklist.push(alias);
        continue;
      }

      if (blockArgument != blockArguments.end()) {
        if (seenRoots.insert(current).second)
          roots.push_back(current);
        continue;
      }
      const GenericOperation *definition = analysis.definingOperation(current);
      if (definition && definition->name == "memref_ext.alloc_workspace" &&
          seenRoots.insert(current).second)
        roots.push_back(current);
    }
    return roots;
  }

  const std::vector<int> &tracebackMemValsCached(int value) {
    const auto found = tracedRoots.find(value);
    if (found != tracedRoots.end())
      return found->second;
    return tracedRoots.emplace(value, tracebackMemVals(value)).first->second;
  }

  const GenericModule &module;
  const GenericModuleAnalysisSnapshot &analysis;
  std::map<int, BlockArgumentOwner> blockArguments;
  std::map<int, std::vector<int>> blockArgAliases;
  std::map<int, std::vector<int>> tracedRoots;
};

inline std::optional<CrossCoreMemoryAccess>
GetCrossCoreReadWriteMemoryOps(const GenericOperation &operation,
                               CrossCoreMemoryValueTracer &tracer) {
  std::vector<int> reads;
  std::vector<int> writes;
  if (IsDestinationStyleOp(operation.name)) {
    reads = tracer.getMemoryOps(operation.dpsInputs);
    writes = tracer.getMemoryOps(operation.dpsInits);
  } else if ((operation.name == "memref.load" ||
              operation.name == "affine.load") &&
             !operation.operands.empty()) {
    reads = tracer.getMemoryOps({operation.operands.front()});
  } else if ((operation.name == "memref.store" ||
              operation.name == "affine.store") &&
             operation.operands.size() >= 2) {
    writes = tracer.getMemoryOps({operation.operands[1]});
  } else if (operation.name == "tensor.extract" &&
             !operation.operands.empty()) {
    reads = tracer.getMemoryOps({operation.operands.front()});
  } else {
    return std::nullopt;
  }
  return CrossCoreMemoryAccess{std::move(reads), std::move(writes)};
}

inline bool CrossCoreMemoryHazard(const CrossCoreMemoryAccess &producer,
                                  const CrossCoreMemoryAccess &consumer) {
  auto intersects = [](const std::vector<int> &left,
                       const std::vector<int> &right) {
    return std::any_of(left.begin(), left.end(), [&](int value) {
      return std::find(right.begin(), right.end(), value) != right.end();
    });
  };
  return intersects(producer.reads, consumer.writes) ||
         intersects(producer.writes, consumer.reads) ||
         intersects(producer.writes, consumer.writes);
}

inline std::vector<CrossCoreSyncDependency>
FindCrossCoreSyncDependencies(const GenericModule &module,
                              const GenericOperation &function) {
  const GenericModuleAnalysisSnapshot analysis(
      module, kGenericAnalysisDefinitions |
                  kGenericAnalysisFunctionDescendants);
  const std::vector<int> &descendants = analysis.descendants(function);
  CrossCoreMemoryValueTracer tracer(module, analysis);

  // The reference solver scans every preceding memory operation backwards for
  // each consumer and stops at the first cross-core hazard.  Retain exactly
  // that nearest-producer rule while indexing the latest read/write position
  // for every root and concrete core.  Common/Both operations are not indexed
  // because the reference path never accepts them as producers.
  struct LatestRootAccess {
    std::array<int, 2> reads = {-1, -1};
    std::array<int, 2> writes = {-1, -1};
  };
  std::unordered_map<int, LatestRootAccess> latestByRoot;
  auto concreteCoreIndex = [](SplitMixCoreType core) -> std::optional<size_t> {
    if (core == SplitMixCoreType::Cube)
      return 0;
    if (core == SplitMixCoreType::Vector)
      return 1;
    return std::nullopt;
  };

  std::vector<CrossCoreSyncDependency> dependencies;
  for (size_t consumerIndex = 0; consumerIndex < descendants.size();
       ++consumerIndex) {
    const int consumerId = descendants[consumerIndex];
    const GenericOperation &consumer =
        module.operations.at(static_cast<size_t>(consumerId));
    std::optional<CrossCoreMemoryAccess> consumerAccess =
        GetCrossCoreReadWriteMemoryOps(consumer, tracer);
    if (!consumerAccess)
      continue;
    const SplitMixCoreType consumerCore = GetSplitMixCoreType(consumer);

    int nearestProducerIndex = -1;
    auto considerRoot = [&](int root, size_t producerCoreIndex,
                            bool includeReads, bool includeWrites) {
      const auto found = latestByRoot.find(root);
      if (found == latestByRoot.end())
        return;
      if (includeReads)
        nearestProducerIndex =
            std::max(nearestProducerIndex,
                     found->second.reads[producerCoreIndex]);
      if (includeWrites)
        nearestProducerIndex =
            std::max(nearestProducerIndex,
                     found->second.writes[producerCoreIndex]);
    };
    auto considerProducerCore = [&](size_t producerCoreIndex) {
      // producer.reads x consumer.writes
      // producer.writes x (consumer.reads U consumer.writes)
      for (int root : consumerAccess->writes)
        considerRoot(root, producerCoreIndex, true, true);
      for (int root : consumerAccess->reads)
        considerRoot(root, producerCoreIndex, false, true);
    };

    if (consumerCore == SplitMixCoreType::Cube)
      considerProducerCore(1);
    else if (consumerCore == SplitMixCoreType::Vector)
      considerProducerCore(0);
    else if (consumerCore == SplitMixCoreType::Both) {
      considerProducerCore(0);
      considerProducerCore(1);
    }

    if (nearestProducerIndex >= 0) {
      const int producerId =
          descendants.at(static_cast<size_t>(nearestProducerIndex));
      const GenericOperation &producer =
          module.operations.at(static_cast<size_t>(producerId));
      const SplitMixCoreType producerCore = GetSplitMixCoreType(producer);
      const SplitMixCoreType resolvedConsumerCore =
          consumerCore == SplitMixCoreType::Both
              ? (producerCore == SplitMixCoreType::Vector
                     ? SplitMixCoreType::Cube
                     : SplitMixCoreType::Vector)
              : consumerCore;
      dependencies.push_back({producer.id, consumer.id, producerCore,
                              resolvedConsumerCore});
    }

    const std::optional<size_t> producerCoreIndex =
        concreteCoreIndex(consumerCore);
    if (!producerCoreIndex)
      continue;
    const int position = static_cast<int>(consumerIndex);
    for (int root : consumerAccess->reads)
      latestByRoot[root].reads[*producerCoreIndex] = position;
    for (int root : consumerAccess->writes)
      latestByRoot[root].writes[*producerCoreIndex] = position;
  }
  return dependencies;
}

inline const char *SplitMixCoreTypeName(SplitMixCoreType core) {
  if (core == SplitMixCoreType::Cube)
    return "CUBE";
  if (core == SplitMixCoreType::Vector)
    return "VECTOR";
  throw std::runtime_error("CrossCoreGSS: common core has no sync operation");
}

// Cross-core dependency endpoints are immutable while the solver chooses
// backward-sync placement.  Cache each endpoint's outer-to-inner loop path
// once and exploit the fact that two ancestor paths share a common prefix.
// This preserves the old outermost/innermost common-loop choices exactly.
class CrossCoreLoopAncestry {
public:
  explicit CrossCoreLoopAncestry(const GenericModule &inputModule)
      : module(inputModule) {}

  std::pair<std::optional<int>, std::optional<int>>
  commonLoops(int producer, int consumer) {
    const std::vector<int> &producerLoops = loops(producer);
    const std::vector<int> &consumerLoops = loops(consumer);
    const auto commonEnd =
        std::mismatch(producerLoops.begin(), producerLoops.end(),
                      consumerLoops.begin(), consumerLoops.end())
            .first;
    const size_t commonCount = static_cast<size_t>(
        std::distance(producerLoops.begin(), commonEnd));
    if (commonCount == 0)
      return {std::nullopt, std::nullopt};
    return {producerLoops.front(), producerLoops[commonCount - 1]};
  }

private:
  const std::vector<int> &loops(int operationId) {
    const auto found = ancestorLoops.find(operationId);
    if (found != ancestorLoops.end())
      return found->second;
    std::vector<int> result;
    int parent =
        module.operations.at(static_cast<size_t>(operationId)).parentId;
    while (parent >= 0) {
      const GenericOperation &operation =
          module.operations.at(static_cast<size_t>(parent));
      if (operation.name == "scf.for" || operation.name == "scf.while")
        result.push_back(parent);
      parent = operation.parentId;
    }
    std::reverse(result.begin(), result.end());
    return ancestorLoops.emplace(operationId, std::move(result)).first->second;
  }

  const GenericModule &module;
  std::unordered_map<int, std::vector<int>> ancestorLoops;
};

inline std::string CrossCoreSyncProperties(int64_t eventId,
                                           SplitMixCoreType core) {
  return "{static_flag_id = " + std::to_string(eventId) +
         " : i64, tcore_type = #hivm.tcore_type<" +
         SplitMixCoreTypeName(core) + ">}";
}

inline int CreateCrossCoreSync(GenericModule &module,
                               GenericRewriter &rewriter, int anchorId,
                               const std::string &name, int64_t eventId,
                               SplitMixCoreType core, bool after,
                               size_t afterOffset = 0) {
  const GenericOperation anchor =
      module.operations.at(static_cast<size_t>(anchorId));
  const int sync = rewriter.createOperation(
      anchor.parentId, anchor.regionId, anchor.blockId, name, {}, {}, {},
      CrossCoreSyncProperties(eventId, core));
  module.operations.at(static_cast<size_t>(sync)).effects = "none";
  const GenericOperation &currentAnchor =
      module.operations.at(static_cast<size_t>(anchorId));
  rewriter.insertToBlock(currentAnchor.blockId,
                         static_cast<size_t>(currentAnchor.ordinal) +
                             (after ? 1 + afterOffset : 0),
                         sync);
  return sync;
}

inline void CreateCrossCoreBarrierAll(GenericModule &module,
                                      GenericRewriter &rewriter,
                                      int anchorId) {
  const GenericOperation anchor =
      module.operations.at(static_cast<size_t>(anchorId));
  const int barrier = rewriter.createOperation(
      anchor.parentId, anchor.regionId, anchor.blockId,
      "hivm.hir.pipe_barrier", {}, {}, {},
      "{pipe = #hivm.pipe<PIPE_ALL>}");
  module.operations.at(static_cast<size_t>(barrier)).effects = "none";
  rewriter.insertToBlock(
      anchor.blockId,
      static_cast<size_t>(
          module.operations.at(static_cast<size_t>(anchorId)).ordinal),
      barrier);

  // SyncSolverCodeGen::insertBlockOp expands a cross-core PIPE_ALL barrier
  // into the barrier plus the two reserved event-id handshakes.
  CreateCrossCoreSync(module, rewriter, anchorId,
                      "hivm.hir.sync_block_set", 15,
                      SplitMixCoreType::Vector, false);
  CreateCrossCoreSync(module, rewriter, anchorId,
                      "hivm.hir.sync_block_wait", 15,
                      SplitMixCoreType::Cube, false);
  CreateCrossCoreSync(module, rewriter, anchorId,
                      "hivm.hir.sync_block_set", 14,
                      SplitMixCoreType::Cube, false);
  CreateCrossCoreSync(module, rewriter, anchorId,
                      "hivm.hir.sync_block_wait", 14,
                      SplitMixCoreType::Vector, false);
}

struct CrossCoreBackwardSync {
  size_t dependency = 0;
  int loop = -1;
  int64_t eventId = -1;
};

struct CrossCoreBackwardCandidate {
  size_t dependency = 0;
  int innermostLoop = -1;
  int outermostLoop = -1;
};

inline GenericModule RunCrossCoreGSS(GenericModule module) {
  std::vector<int> mixFunctions;
  for (const GenericOperation &operation : module.operations)
    if (IsSplitMixFunction(operation))
      mixFunctions.push_back(operation.id);
  if (mixFunctions.empty())
    return module;

  GenericRewriter rewriter(module);
  for (int functionId : mixFunctions) {
    const GenericOperation &function =
        module.operations.at(static_cast<size_t>(functionId));
    const std::optional<int> baseAddress =
        GetFFTSBaseAddressArgument(module, function);
    if (!baseAddress)
      throw std::runtime_error(
          "CrossCoreGSS: MIX function has no FFTS base address argument");
    const GenericRegion &functionRegion =
        module.regions.at(static_cast<size_t>(function.regions.front()));
    const int entryBlock = functionRegion.blocks.front();
    const int setBase = rewriter.createOperation(
        functionId, function.regions.front(), entryBlock,
        "hivm.hir.set_ffts_base_addr", {}, {*baseAddress}, {"i64"});
    module.operations.at(static_cast<size_t>(setBase)).effects = "none";
    rewriter.insertToBlock(entryBlock, 0, setBase);

    const std::vector<CrossCoreSyncDependency> dependencies =
        FindCrossCoreSyncDependencies(
            module, module.operations.at(static_cast<size_t>(functionId)));
    std::map<std::pair<SplitMixCoreType, SplitMixCoreType>, int64_t>
        forwardEventIds;
    int64_t nextForwardEventId = 0;
    for (const CrossCoreSyncDependency &dependency : dependencies) {
      const auto direction =
          std::make_pair(dependency.producerCore, dependency.consumerCore);
      if (forwardEventIds.count(direction) == 0)
        forwardEventIds[direction] = nextForwardEventId++;
    }
    // Cross-core mode exposes 16 hardware ids, with 14 and 15 reserved for
    // barrier-all. EventIdSolver therefore colors normal pairs with [0, 14).
    // Backward pairs in one outermost loop overlap and consume distinct ids.
    // If coloring runs out, Solver inserts a persistent barrier and reruns;
    // the pair immediately preceding that barrier is covered by the barrier.
    constexpr int64_t crossCoreEventIdCount = 14;
    std::vector<CrossCoreBackwardCandidate> backwardCandidates;
    CrossCoreLoopAncestry loopAncestry(module);
    for (size_t index = 0; index < dependencies.size(); ++index) {
      const CrossCoreSyncDependency &dependency = dependencies[index];
      const auto [outermostLoop, innermostLoop] = loopAncestry.commonLoops(
          dependency.producer, dependency.consumer);
      if (outermostLoop && innermostLoop)
        backwardCandidates.push_back(
            {index, *innermostLoop, *outermostLoop});
    }

    std::vector<CrossCoreBackwardSync> backwardSyncs;
    std::vector<int> barrierAnchors;
    const size_t availableBackwardEvents =
        nextForwardEventId < crossCoreEventIdCount
            ? static_cast<size_t>(crossCoreEventIdCount - nextForwardEventId)
            : 0;
    const bool eventIdsRanOut =
        backwardCandidates.size() > availableBackwardEvents;
    size_t retained = backwardCandidates.size();
    if (eventIdsRanOut) {
      barrierAnchors.push_back(
          dependencies
              .at(backwardCandidates.at(availableBackwardEvents).dependency)
              .producer);
      retained = availableBackwardEvents == 0
                     ? 0
                     : availableBackwardEvents - 1;
    }
    for (size_t index = 0; index < retained; ++index) {
      const CrossCoreBackwardCandidate &candidate = backwardCandidates[index];
      // Solver first tries moving backward pairs to the outermost loop. Once
      // that run needs barrier-all it retries with movement disabled, leaving
      // the pairs at their parent LCA (innermost common loop).
      backwardSyncs.push_back(
          {candidate.dependency,
           eventIdsRanOut ? candidate.innermostLoop : candidate.outermostLoop,
           nextForwardEventId + static_cast<int64_t>(index)});
    }

    std::map<int, size_t> syncAfterCount;
    for (const CrossCoreSyncDependency &dependency : dependencies) {
      const GenericOperation &producer =
          module.operations.at(static_cast<size_t>(dependency.producer));
      const GenericOperation &consumer =
          module.operations.at(static_cast<size_t>(dependency.consumer));
      if (producer.blockId < 0 || consumer.blockId < 0)
        throw std::runtime_error(
            "CrossCoreGSS: synchronization endpoint has no block");
      const int64_t forwardEvent = forwardEventIds.at(
          {dependency.producerCore, dependency.consumerCore});
      CreateCrossCoreSync(module, rewriter, dependency.producer,
                          "hivm.hir.sync_block_set", forwardEvent,
                          dependency.producerCore, true,
                          syncAfterCount[dependency.producer]++);
      CreateCrossCoreSync(
          module, rewriter, dependency.consumer,
          "hivm.hir.sync_block_wait", forwardEvent,
          dependency.consumerCore, false);
    }

    for (const CrossCoreBackwardSync &backward : backwardSyncs) {
      const CrossCoreSyncDependency &dependency =
          dependencies.at(backward.dependency);
      if (dependency.producerCore == SplitMixCoreType::Cube &&
          dependency.consumerCore == SplitMixCoreType::Vector) {
        CreateCrossCoreSync(module, rewriter, backward.loop,
                            "hivm.hir.sync_block_set", backward.eventId,
                            SplitMixCoreType::Vector, false);
        CreateCrossCoreSync(module, rewriter, dependency.producer,
                            "hivm.hir.sync_block_wait", backward.eventId,
                            SplitMixCoreType::Cube, false);
        CreateCrossCoreSync(module, rewriter, dependency.consumer,
                            "hivm.hir.sync_block_set", backward.eventId,
                            SplitMixCoreType::Vector, true);
        CreateCrossCoreSync(module, rewriter, backward.loop,
                            "hivm.hir.sync_block_wait", backward.eventId,
                            SplitMixCoreType::Cube, true,
                            syncAfterCount[backward.loop]++);
      } else if (dependency.producerCore == SplitMixCoreType::Vector &&
                 dependency.consumerCore == SplitMixCoreType::Cube) {
        CreateCrossCoreSync(module, rewriter, backward.loop,
                            "hivm.hir.sync_block_set", backward.eventId,
                            SplitMixCoreType::Cube, false);
        CreateCrossCoreSync(module, rewriter, dependency.producer,
                            "hivm.hir.sync_block_wait", backward.eventId,
                            SplitMixCoreType::Vector, false);
        CreateCrossCoreSync(module, rewriter, dependency.consumer,
                            "hivm.hir.sync_block_set", backward.eventId,
                            SplitMixCoreType::Cube, true);
        CreateCrossCoreSync(module, rewriter, backward.loop,
                            "hivm.hir.sync_block_wait", backward.eventId,
                            SplitMixCoreType::Vector, true,
                            syncAfterCount[backward.loop]++);
      }
    }

    for (int anchor : barrierAnchors)
      CreateCrossCoreBarrierAll(module, rewriter, anchor);
  }
  return module;
}

} // namespace cvub

#endif
