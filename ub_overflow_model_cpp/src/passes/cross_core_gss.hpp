#ifndef CVPIPELINE_UB_MODEL_CPP_CROSS_CORE_GSS_HPP
#define CVPIPELINE_UB_MODEL_CPP_CROSS_CORE_GSS_HPP

#include "../ir/generic_analysis.hpp"
#include "mark_real_core_type.hpp"

#include <array>
#include <cstdlib>
#include <iostream>
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

inline std::string CrossCoreDynamicSyncProperties(SplitMixCoreType core) {
  return "{tcore_type = #hivm.tcore_type<" +
         std::string(SplitMixCoreTypeName(core)) + ">}";
}

inline int CreateCrossCoreSync(GenericModule &module,
                               GenericRewriter &rewriter, int anchorId,
                               const std::string &name, int64_t eventId,
                               SplitMixCoreType core, bool after,
                               size_t afterOffset = 0,
                               size_t beforeOffset = 0) {
  const GenericOperation anchor =
      module.operations.at(static_cast<size_t>(anchorId));
  const int sync = rewriter.createOperation(
      anchor.parentId, anchor.regionId, anchor.blockId, name, {}, {}, {},
      CrossCoreSyncProperties(eventId, core));
  module.operations.at(static_cast<size_t>(sync)).effects = "none";
  const GenericOperation &currentAnchor =
      module.operations.at(static_cast<size_t>(anchorId));
  const size_t anchorOrdinal = static_cast<size_t>(currentAnchor.ordinal);
  if (!after && beforeOffset > anchorOrdinal)
    throw std::runtime_error("CrossCoreGSS: invalid sync-before offset");
  rewriter.insertToBlock(
      currentAnchor.blockId,
      after ? anchorOrdinal + 1 + afterOffset : anchorOrdinal - beforeOffset,
      sync);
  return sync;
}

inline int CreateCrossCoreDynamicSync(GenericModule &module,
                                      GenericRewriter &rewriter,
                                      int anchorId,
                                      const std::string &name,
                                      int eventIdValue,
                                      SplitMixCoreType core, bool after,
                                      size_t afterOffset = 0,
                                      size_t beforeOffset = 0) {
  const GenericOperation anchor =
      module.operations.at(static_cast<size_t>(anchorId));
  const int sync = rewriter.createOperation(
      anchor.parentId, anchor.regionId, anchor.blockId, name, {},
      {eventIdValue}, {"i64"}, CrossCoreDynamicSyncProperties(core));
  module.operations.at(static_cast<size_t>(sync)).effects = "none";
  const GenericOperation &currentAnchor =
      module.operations.at(static_cast<size_t>(anchorId));
  const size_t anchorOrdinal = static_cast<size_t>(currentAnchor.ordinal);
  if (!after && beforeOffset > anchorOrdinal)
    throw std::runtime_error("CrossCoreGSS: invalid dynamic sync-before offset");
  rewriter.insertToBlock(
      currentAnchor.blockId,
      after ? anchorOrdinal + 1 + afterOffset : anchorOrdinal - beforeOffset,
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
  int multibufferLoop = -1;
  int producerUnrollLoop = -1;
  int consumerUnrollLoop = -1;
  std::vector<int64_t> eventIds;
  int64_t eventIdRepeatNum = 1;
  std::optional<int64_t> producerPreloadOffset;
  std::optional<int64_t> consumerPreloadOffset;
};

struct CrossCoreBackwardCandidate {
  size_t dependency = 0;
  int innermostLoop = -1;
  int outermostLoop = -1;
  int multibufferLoop = -1;
  int producerUnrollLoop = -1;
  int consumerUnrollLoop = -1;
  int64_t eventIdNum = 1;
  std::optional<int64_t> producerPreloadOffset;
  std::optional<int64_t> consumerPreloadOffset;
};

struct CrossCorePreloadScopeInfo {
  int scope = -1;
  int loop = -1;
  int64_t maxPreloadNum = 0;
  int64_t preloadOffset = 0;
};

struct CrossCoreUnrollLoopInfo {
  int loop = -1;
  int64_t unrollNum = 0;
};

inline std::optional<int64_t>
ParseCrossCoreIntegerAttribute(const GenericOperation &operation,
                               const std::string &name) {
  std::string value = FindDictionaryValue(operation.properties, name);
  if (value.empty())
    value = FindDictionaryValue(operation.attributes, name);
  if (value.empty())
    return std::nullopt;
  size_t begin = 0;
  while (begin < value.size() && std::isspace(
                                     static_cast<unsigned char>(value[begin])))
    ++begin;
  size_t end = begin;
  if (end < value.size() && (value[end] == '-' || value[end] == '+'))
    ++end;
  while (end < value.size() &&
         std::isdigit(static_cast<unsigned char>(value[end])))
    ++end;
  if (end == begin ||
      (end == begin + 1 && (value[begin] == '-' || value[begin] == '+')))
    return std::nullopt;
  try {
    return std::stoll(value.substr(begin, end - begin));
  } catch (const std::exception &) {
    return std::nullopt;
  }
}

// Mirrors Solver::checkCVMultiBufferUnrollEventIdInfo's upward search for the
// nearest Loop carrying kMultibufferUnrollAttrName.  CVPipelining emits
// normalized scf.for loops with this attribute on both sides of a cross-core
// dependency; the two loop operations need not be the same operation.
inline std::optional<CrossCoreUnrollLoopInfo>
GetCrossCoreUnrollLoopInfo(const GenericModule &module, int operationId) {
  int parent =
      module.operations.at(static_cast<size_t>(operationId)).parentId;
  while (parent >= 0) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(parent));
    if (operation.name == "scf.for") {
      const std::optional<int64_t> unrollNum =
          ParseCrossCoreIntegerAttribute(operation,
                                         "multibuffer_unroll_factor");
      if (unrollNum && *unrollNum > 0)
        return CrossCoreUnrollLoopInfo{parent, *unrollNum};
    }
    parent = operation.parentId;
  }
  return std::nullopt;
}

// Mirrors Solver::checkCVMultiBufferPreloadEventIdInfo: find the nearest
// enclosing scope carrying max/preload numbers, then use its parent scf.for
// as the multi-buffer loop and derive max - preload - 1.
inline std::optional<CrossCorePreloadScopeInfo>
GetCrossCorePreloadScopeInfo(const GenericModule &module, int operationId) {
  int parent =
      module.operations.at(static_cast<size_t>(operationId)).parentId;
  while (parent >= 0) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(parent));
    if (operation.name == "scope.scope") {
      const std::optional<int64_t> maxPreload =
          ParseCrossCoreIntegerAttribute(operation, "hivm.max_preload_num");
      if (maxPreload) {
        const std::optional<int64_t> preload =
            ParseCrossCoreIntegerAttribute(operation, "hivm.preload_num");
        if (!preload || *maxPreload <= 0 || *preload < 0 ||
            *preload >= *maxPreload)
          return std::nullopt;
        int loop = operation.parentId;
        while (loop >= 0) {
          const GenericOperation &ancestor =
              module.operations.at(static_cast<size_t>(loop));
          if (ancestor.name == "scf.for")
            return CrossCorePreloadScopeInfo{
                operation.id, loop, *maxPreload,
                *maxPreload - *preload - 1};
          loop = ancestor.parentId;
        }
        return std::nullopt;
      }
    }
    parent = operation.parentId;
  }
  return std::nullopt;
}

// Mirrors the cv-preloading branch in Solver::getFixedSetWaitOcc.  When the
// LCA children are two preload scopes, the production solver sinks the pair
// into those scopes and uses the direct child containing each original
// endpoint.  If that child is a loop, SyncSolverCodeGen consequently places
// the wait before the loop and the set after it.
inline int GetCrossCorePreloadSyncAnchor(const GenericModule &module,
                                         int endpoint, int preloadScope) {
  int anchor = endpoint;
  while (anchor >= 0) {
    const int parent =
        module.operations.at(static_cast<size_t>(anchor)).parentId;
    if (parent == preloadScope)
      return anchor;
    if (parent < 0)
      break;
    anchor = parent;
  }
  return endpoint;
}

// Mirrors getFixedSetWaitOcc(..., sinkSyncIntoCVLoops=true).  When the LCA
// pair is formed by two CV multi-buffer unroll loops, the production solver
// sinks each synchronization endpoint into its corresponding unroll loop and
// anchors it on the direct child containing the original memory operation.
// Code generation consequently emits a wait before that child or a set after
// it, rather than placing the synchronization at the leaf memory operation.
inline int GetCrossCoreUnrollSyncAnchor(const GenericModule &module,
                                        int endpoint, int unrollLoop) {
  int anchor = endpoint;
  while (anchor >= 0) {
    const int parent =
        module.operations.at(static_cast<size_t>(anchor)).parentId;
    if (parent == unrollLoop)
      return anchor;
    anchor = parent;
  }
  return endpoint;
}

enum class CrossCoreDynamicEventPlacement {
  LoopBodyStart,
  BeforeSyncAnchor,
};

class CrossCoreDynamicEventBuilder {
public:
  CrossCoreDynamicEventBuilder(GenericModule &inputModule,
                               GenericRewriter &inputRewriter,
                               CrossCoreDynamicEventPlacement inputPlacement =
                                   CrossCoreDynamicEventPlacement::LoopBodyStart)
      : module(inputModule), rewriter(inputRewriter),
        placement(inputPlacement) {}

  int selectedEvent(int loop, int64_t eventIdNum, int64_t preloadOffset,
                    int64_t firstEventId) {
    const auto key =
        std::make_tuple(loop, eventIdNum, preloadOffset, firstEventId);
    if (const auto found = selectedEvents.find(key);
        found != selectedEvents.end())
      return found->second;

    const int modular = modularIndex(loop, eventIdNum, preloadOffset);
    const int base = createAtLoopBodyStart(
        loop, "arith.constant", {"index"}, {}, {},
        "{value = " + std::to_string(firstEventId) + " : index}");
    const int add = createAtLoopBodyStart(
        loop, "arith.addi", {"index"},
        {result(modular), result(base)}, {"index", "index"},
        "{overflowFlags = #arith.overflow<none>}");
    const int cast = createAtLoopBodyStart(
        loop, "arith.index_cast", {"i64"}, {result(add)}, {"index"});
    return selectedEvents.emplace(key, result(cast)).first->second;
  }

  // SyncSolverCodeGen::getCVMultiBufferSelectOpConsecutive selects a
  // consecutive event group with `induction_variable + first_event_id`.
  // Unlike the preload path, the producer and consumer may use two distinct
  // CVPipelining loops, so construct the expression independently in each
  // loop body.
  int selectedCVUnrollEvent(int loop, int64_t firstEventId,
                            int syncAnchor = -1) {
    const GenericOperation &loopOp =
        module.operations.at(static_cast<size_t>(loop));
    if (loopOp.regions.empty())
      throw std::runtime_error("CrossCoreGSS: unroll loop has no body");
    const GenericRegion &region =
        module.regions.at(static_cast<size_t>(loopOp.regions.front()));
    if (region.blocks.empty())
      throw std::runtime_error("CrossCoreGSS: unroll loop has no body block");
    const GenericBlock &body =
        module.blocks.at(static_cast<size_t>(region.blocks.front()));
    if (body.arguments.empty())
      throw std::runtime_error(
          "CrossCoreGSS: unroll loop has no induction variable");

    const int base = createCVUnrollOperation(
        loop, syncAnchor, "arith.constant", {"index"}, {}, {},
        "{value = " + std::to_string(firstEventId) + " : index}");
    const int add = createCVUnrollOperation(
        loop, syncAnchor, "arith.addi", {"index"},
        {body.arguments.front(), result(base)}, {"index", "index"},
        "{overflowFlags = #arith.overflow<none>}");
    const int cast = createCVUnrollOperation(
        loop, syncAnchor, "arith.index_cast", {"i64"}, {result(add)},
        {"index"});
    return result(cast);
  }

private:
  int result(int operation) const {
    const GenericOperation &record =
        module.operations.at(static_cast<size_t>(operation));
    if (record.results.size() != 1)
      throw std::runtime_error(
          "CrossCoreGSS: dynamic event operation must have one result");
    return record.results.front();
  }

  int createAtLoopBodyStart(
      int loop, const std::string &name,
      const std::vector<std::string> &resultTypes,
      const std::vector<int> &operands = {},
      const std::vector<std::string> &operandTypes = {},
      const std::string &properties = "") {
    const GenericOperation &loopOp =
        module.operations.at(static_cast<size_t>(loop));
    if (loopOp.regions.empty())
      throw std::runtime_error("CrossCoreGSS: loop has no body region");
    const GenericRegion &region =
        module.regions.at(static_cast<size_t>(loopOp.regions.front()));
    if (region.blocks.empty())
      throw std::runtime_error("CrossCoreGSS: loop has no body block");
    const int block = region.blocks.front();
    const int operation = rewriter.createOperation(
        loop, loopOp.regions.front(), block, name, resultTypes, operands,
        operandTypes, properties);
    module.operations.at(static_cast<size_t>(operation)).effects = "none";
    rewriter.insertToBlock(block, bodyInsertions[block]++, operation);
    return operation;
  }

  // InjectSync/SyncCodegen.cpp::CreateSetWaitBlockOpForMultiBuffer builds
  // the IV + first-event-id selector at the synchronization endpoint.  GSS
  // builds the equivalent selector at the loop-body start.  Keeping this
  // placement distinction is observable by PlanMemory because the scalar SSA
  // values participate in MLIR liveness ordering before non-UB values are
  // filtered out.
  int createCVUnrollOperation(
      int loop, int syncAnchor, const std::string &name,
      const std::vector<std::string> &resultTypes,
      const std::vector<int> &operands = {},
      const std::vector<std::string> &operandTypes = {},
      const std::string &properties = "") {
    if (placement == CrossCoreDynamicEventPlacement::LoopBodyStart)
      return createAtLoopBodyStart(loop, name, resultTypes, operands,
                                   operandTypes, properties);
    if (syncAnchor < 0)
      throw std::runtime_error(
          "InjectBlockSync: dynamic event has no synchronization anchor");
    const GenericOperation anchor =
        module.operations.at(static_cast<size_t>(syncAnchor));
    const int operation = rewriter.createOperation(
        anchor.parentId, anchor.regionId, anchor.blockId, name, resultTypes,
        operands, operandTypes, properties);
    module.operations.at(static_cast<size_t>(operation)).effects = "none";
    rewriter.insertToBlock(
        anchor.blockId,
        static_cast<size_t>(
            module.operations.at(static_cast<size_t>(syncAnchor)).ordinal),
        operation);
    return operation;
  }

  int baseModularIndex(int loop, int64_t eventIdNum) {
    const auto key = std::make_pair(loop, eventIdNum);
    if (const auto found = baseModularIndices.find(key);
        found != baseModularIndices.end())
      return found->second;
    const int counter = createAtLoopBodyStart(
        loop, "hivm.hir.multi_buffer_counter", {"i64"});
    const int modulus = createAtLoopBodyStart(
        loop, "arith.constant", {"i64"}, {}, {},
        "{value = " + std::to_string(eventIdNum) + " : i64}");
    const int remainder = createAtLoopBodyStart(
        loop, "arith.remui", {"i64"},
        {result(counter), result(modulus)}, {"i64", "i64"});
    const int cast = createAtLoopBodyStart(
        loop, "arith.index_cast", {"index"}, {result(remainder)}, {"i64"});
    return baseModularIndices.emplace(key, cast).first->second;
  }

  int modularIndex(int loop, int64_t eventIdNum, int64_t preloadOffset) {
    const auto key = std::make_tuple(loop, eventIdNum, preloadOffset);
    if (const auto found = modularIndices.find(key);
        found != modularIndices.end())
      return found->second;
    const int base = baseModularIndex(loop, eventIdNum);
    if (preloadOffset <= 0)
      return modularIndices.emplace(key, base).first->second;
    const int modulus = createAtLoopBodyStart(
        loop, "arith.constant", {"index"}, {}, {},
        "{value = " + std::to_string(eventIdNum) + " : index}");
    const int64_t shiftedOffset =
        eventIdNum - (preloadOffset % eventIdNum);
    const int offset = createAtLoopBodyStart(
        loop, "arith.constant", {"index"}, {}, {},
        "{value = " + std::to_string(shiftedOffset) + " : index}");
    const int add = createAtLoopBodyStart(
        loop, "arith.addi", {"index"},
        {result(base), result(offset)}, {"index", "index"},
        "{overflowFlags = #arith.overflow<none>}");
    const int remainder = createAtLoopBodyStart(
        loop, "arith.remsi", {"index"},
        {result(add), result(modulus)}, {"index", "index"});
    return modularIndices.emplace(key, remainder).first->second;
  }

  GenericModule &module;
  GenericRewriter &rewriter;
  CrossCoreDynamicEventPlacement placement;
  std::map<int, size_t> bodyInsertions;
  std::map<std::pair<int, int64_t>, int> baseModularIndices;
  std::map<std::tuple<int, int64_t, int64_t>, int> modularIndices;
  std::map<std::tuple<int, int64_t, int64_t, int64_t>, int> selectedEvents;
};

inline GenericModule RunCrossCoreBlockSyncAnalysis(
    GenericModule module,
    CrossCoreDynamicEventPlacement dynamicEventPlacement,
    const std::set<int> *selectedFunctions = nullptr) {
  std::vector<int> mixFunctions;
  for (const GenericOperation &operation : module.operations)
    if (IsSplitMixFunction(operation) &&
        (!selectedFunctions || selectedFunctions->count(operation.id) != 0))
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
    if (std::getenv("CVUB_DEBUG_GSS")) {
      for (size_t dependencyIndex = 0;
           dependencyIndex < dependencies.size(); ++dependencyIndex) {
        const auto &dependency = dependencies[dependencyIndex];
        const auto &producer =
            module.operations.at(static_cast<size_t>(dependency.producer));
        const auto &consumer =
            module.operations.at(static_cast<size_t>(dependency.consumer));
        std::cerr << "GSS_DEP " << dependencyIndex << " producer="
                  << dependency.producer << ':' << producer.name
                  << " consumer=" << dependency.consumer << ':'
                  << consumer.name << " cores="
                  << static_cast<int>(dependency.producerCore) << "->"
                  << static_cast<int>(dependency.consumerCore) << '\n';
      }
    }
    std::set<std::pair<SplitMixCoreType, SplitMixCoreType>>
        forwardDirections;
    for (const CrossCoreSyncDependency &dependency : dependencies) {
      const auto direction =
          std::make_pair(dependency.producerCore, dependency.consumerCore);
      forwardDirections.insert(direction);
    }
    // Cross-core mode exposes 16 hardware ids, with 14 and 15 reserved for
    // barrier-all. EventIdSolver therefore colors normal pairs with [0, 14).
    // Backward pairs in one outermost loop overlap and consume distinct ids.
    // If coloring runs out, Solver inserts a persistent barrier and reruns;
    // the pair immediately preceding that barrier is covered by the barrier.
    constexpr int64_t crossCoreEventIdCount = 14;
    std::vector<CrossCoreBackwardCandidate> backwardCandidates;
    std::vector<std::pair<int, int>> dependencySyncAnchors;
    dependencySyncAnchors.reserve(dependencies.size());
    CrossCoreLoopAncestry loopAncestry(module);
    for (size_t index = 0; index < dependencies.size(); ++index) {
      const CrossCoreSyncDependency &dependency = dependencies[index];
      const std::optional<CrossCorePreloadScopeInfo> producerPreload =
          GetCrossCorePreloadScopeInfo(module, dependency.producer);
      const std::optional<CrossCorePreloadScopeInfo> consumerPreload =
          GetCrossCorePreloadScopeInfo(module, dependency.consumer);
      const std::optional<CrossCoreUnrollLoopInfo> producerUnroll =
          GetCrossCoreUnrollLoopInfo(module, dependency.producer);
      const std::optional<CrossCoreUnrollLoopInfo> consumerUnroll =
          GetCrossCoreUnrollLoopInfo(module, dependency.consumer);
      int producerSyncAnchor = dependency.producer;
      int consumerSyncAnchor = dependency.consumer;
      // Solver::getEventIdInfo checks CV multi-buffer unroll before preload,
      // and getFixedSetWaitOcc sinks synchronization into the selected CV
      // loops. Preserve the same precedence and direct-child anchors.
      if (producerUnroll && consumerUnroll &&
          producerUnroll->unrollNum == consumerUnroll->unrollNum) {
        producerSyncAnchor = GetCrossCoreUnrollSyncAnchor(
            module, dependency.producer, producerUnroll->loop);
        consumerSyncAnchor = GetCrossCoreUnrollSyncAnchor(
            module, dependency.consumer, consumerUnroll->loop);
      } else if (producerPreload && consumerPreload &&
          producerPreload->loop == consumerPreload->loop &&
          producerPreload->maxPreloadNum ==
              consumerPreload->maxPreloadNum) {
        producerSyncAnchor = GetCrossCorePreloadSyncAnchor(
            module, dependency.producer, producerPreload->scope);
        consumerSyncAnchor = GetCrossCorePreloadSyncAnchor(
            module, dependency.consumer, consumerPreload->scope);
      }
      dependencySyncAnchors.emplace_back(producerSyncAnchor,
                                         consumerSyncAnchor);

      const auto [outermostLoop, innermostLoop] = loopAncestry.commonLoops(
          dependency.producer, dependency.consumer);
      if (!outermostLoop || !innermostLoop)
        continue;
      CrossCoreBackwardCandidate candidate{
          index, *innermostLoop, *outermostLoop, -1, -1, -1, 1,
          std::nullopt, std::nullopt};
      // getEventIdInfo checks CV multi-buffer unroll before CV preload.
      if (producerUnroll && consumerUnroll &&
          producerUnroll->unrollNum == consumerUnroll->unrollNum) {
        candidate.eventIdNum = producerUnroll->unrollNum;
        candidate.producerUnrollLoop = producerUnroll->loop;
        candidate.consumerUnrollLoop = consumerUnroll->loop;
      } else if (producerPreload && consumerPreload &&
          producerPreload->loop == consumerPreload->loop &&
          producerPreload->maxPreloadNum ==
              consumerPreload->maxPreloadNum) {
        candidate.eventIdNum = producerPreload->maxPreloadNum;
        candidate.multibufferLoop = producerPreload->loop;
        candidate.producerPreloadOffset = producerPreload->preloadOffset;
        candidate.consumerPreloadOffset = consumerPreload->preloadOffset;
      }
      backwardCandidates.push_back(std::move(candidate));
    }

    std::vector<CrossCoreBackwardSync> backwardSyncs;
    std::vector<int> barrierAnchors;
    const int64_t availableBackwardEvents =
        std::max<int64_t>(
            0, crossCoreEventIdCount -
                   static_cast<int64_t>(forwardDirections.size()));
    std::map<std::pair<SplitMixCoreType, SplitMixCoreType>, int64_t>
        forwardEventIds;
    // Dynamic CV-pipeline event groups cannot use repeat flag ids.  Keep that
    // distinction here because EventIdSolver sees one forward node per memory
    // conflict, not one node per core direction.  Same-direction forward
    // nodes can reuse a color, but their individual graph degrees interleave
    // the forward color with the weighted backward nodes.  Collapsing those
    // nodes before coloring incorrectly puts the first dynamic group at zero.
    const bool hasCannotRepeatBackward = std::any_of(
        backwardCandidates.begin(), backwardCandidates.end(),
        [](const CrossCoreBackwardCandidate &candidate) {
          return candidate.producerUnrollLoop >= 0 ||
                 candidate.consumerUnrollLoop >= 0;
        });
    // EventIdSolver colors weighted conflict nodes, it does not reserve the
    // sum of every backward pair in the function.  Backward pairs moved to
    // different disjoint outer loops have non-overlapping live intervals and
    // reuse the same colors.  Within one outer-loop occurrence every pair
    // spans the loop boundary and conflicts, so the exact weighted interval
    // coloring reduces to consecutive blocks local to that outer loop.
    //
    // Solver::checkRepeatMultiBufferFlagId is applied before coloring.  A
    // candidate whose innermost common loop is already the outmost loop was
    // not moved by handleSetWaitConflict, so an N-way multi-buffer pair uses
    // one weighted color with eventIdRepeatNum=N.  Reserving N distinct ids in
    // that case both disagrees with CrossCoreGSS and materializes an otherwise
    // absent runtime multi_buffer_counter in the generated IR.
    std::vector<std::vector<int64_t>> distinctCandidateEventIds(
        backwardCandidates.size());
    std::vector<int64_t> coloredCandidateEventCounts(
        backwardCandidates.size(), 1);
    for (size_t index = 0; index < backwardCandidates.size(); ++index) {
      const CrossCoreBackwardCandidate &candidate = backwardCandidates[index];
      const bool movedToOuterLoop =
          candidate.innermostLoop != candidate.outermostLoop;
      // checkCVMultiBufferUnrollEventIdInfo marks a dynamic CV-pipeline
      // event group cannotRepeatFlagId.  Those groups must retain distinct
      // ids even when the backward pair cannot move outward; preload event
      // groups have no unroll-loop endpoints and remain repeatable.
      const bool canRepeatFlagId = candidate.producerUnrollLoop < 0 &&
                                   candidate.consumerUnrollLoop < 0;
      coloredCandidateEventCounts[index] =
          movedToOuterLoop || !canRepeatFlagId ? candidate.eventIdNum : 1;
    }
    int64_t totalDistinctEventCount = 0;
    if (hasCannotRepeatBackward) {
      // This is the weighted interval-coloring order produced by
      // EventIdSolver for CV-unrolled conflicts: core directions are visited
      // in reverse order; the reusable forward color precedes the backward
      // nodes of that direction; and the latter are eliminated in reverse
      // dependency order.  Disjoint outer loops use the same local palette.
      // For the common alternating C/V chain this yields exactly
      //   forward, backward[N-1..0], forward, backward[N-1..0]
      // rather than assigning all backward nodes before both forward colors.
      for (auto directionIt = forwardDirections.rbegin();
           directionIt != forwardDirections.rend(); ++directionIt) {
        const auto &direction = *directionIt;
        forwardEventIds[direction] = totalDistinctEventCount++;
        const int64_t directionBase = totalDistinctEventCount;
        std::map<int, int64_t> nextEventInOuterLoop;
        int64_t directionEnd = directionBase;
        for (size_t reverseIndex = backwardCandidates.size();
             reverseIndex > 0; --reverseIndex) {
          const size_t index = reverseIndex - 1;
          const CrossCoreBackwardCandidate &candidate =
              backwardCandidates[index];
          const CrossCoreSyncDependency &dependency =
              dependencies.at(candidate.dependency);
          if (std::make_pair(dependency.producerCore,
                             dependency.consumerCore) != direction)
            continue;
          auto [nextIt, inserted] = nextEventInOuterLoop.emplace(
              candidate.outermostLoop, directionBase);
          (void)inserted;
          int64_t &next = nextIt->second;
          std::vector<int64_t> &eventIds =
              distinctCandidateEventIds[index];
          const int64_t eventCount = coloredCandidateEventCounts[index];
          eventIds.resize(static_cast<size_t>(eventCount));
          std::iota(eventIds.begin(), eventIds.end(), next);
          next += eventCount;
          directionEnd = std::max(directionEnd, next);
        }
        totalDistinctEventCount = directionEnd;
      }
    } else {
      // Repeatable preload groups follow the ordinary low-forward-color path.
      int64_t nextForwardEventId = 0;
      for (const auto &direction : forwardDirections)
        forwardEventIds[direction] = nextForwardEventId++;
      std::map<int, int64_t> nextEventInOuterLoop;
      for (size_t index = 0; index < backwardCandidates.size(); ++index) {
        const CrossCoreBackwardCandidate &candidate = backwardCandidates[index];
        auto [nextIt, inserted] = nextEventInOuterLoop.emplace(
            candidate.outermostLoop, nextForwardEventId);
        (void)inserted;
        int64_t &next = nextIt->second;
        std::vector<int64_t> &eventIds = distinctCandidateEventIds[index];
        const int64_t eventCount = coloredCandidateEventCounts[index];
        eventIds.resize(static_cast<size_t>(eventCount));
        std::iota(eventIds.begin(), eventIds.end(), next);
        next += eventCount;
        totalDistinctEventCount =
            std::max(totalDistinctEventCount, next);
      }
      totalDistinctEventCount =
          std::max(totalDistinctEventCount, nextForwardEventId);
    }
    const bool distinctEventIdsRanOut =
        totalDistinctEventCount > crossCoreEventIdCount;
    // Solver::tryMovingOutBackwardSyncPairsToOuterLoops first attempts to
    // reserve every multi-buffer event while moving backward pairs outward.
    // If that coloring fails, it retries at the innermost common loop. There
    // checkRepeatMultiBufferFlagId turns an N-event preload pair into one
    // hardware id repeated N times. Only a second coloring failure may become
    // a persistent barrier-all; the first failure itself is not emitted.
    const bool repeatEventIdsRanOut =
        distinctEventIdsRanOut &&
        static_cast<int64_t>(backwardCandidates.size()) >
            availableBackwardEvents;
    size_t retained = backwardCandidates.size();
    if (repeatEventIdsRanOut) {
      retained = static_cast<size_t>(availableBackwardEvents);
      barrierAnchors.push_back(
          dependencies
              .at(backwardCandidates.at(retained).dependency)
              .producer);
      // The persistent barrier covers the immediately preceding pair on the
      // retry, matching Solver's event-id-ran-out recovery.
      retained = retained == 0 ? 0 : retained - 1;
    }
    int64_t nextEventId = hasCannotRepeatBackward
                              ? 0
                              : static_cast<int64_t>(forwardDirections.size());
    for (size_t index = 0; index < retained; ++index) {
      const CrossCoreBackwardCandidate &candidate = backwardCandidates[index];
      // Solver first tries moving backward pairs to the outermost loop. Once
      // that run needs barrier-all it retries with movement disabled, leaving
      // the pairs at their parent LCA (innermost common loop).
      CrossCoreBackwardSync sync;
      sync.dependency = candidate.dependency;
      sync.loop = distinctEventIdsRanOut ? candidate.innermostLoop
                                         : candidate.outermostLoop;
      sync.multibufferLoop = candidate.multibufferLoop;
      sync.producerUnrollLoop = candidate.producerUnrollLoop;
      sync.consumerUnrollLoop = candidate.consumerUnrollLoop;
      sync.producerPreloadOffset = candidate.producerPreloadOffset;
      sync.consumerPreloadOffset = candidate.consumerPreloadOffset;
      if (distinctEventIdsRanOut) {
        sync.eventIds = {nextEventId++};
        sync.eventIdRepeatNum = candidate.eventIdNum;
      } else {
        sync.eventIds = distinctCandidateEventIds[index];
        if (candidate.innermostLoop == candidate.outermostLoop &&
            candidate.producerUnrollLoop < 0 &&
            candidate.consumerUnrollLoop < 0)
          sync.eventIdRepeatNum = candidate.eventIdNum;
      }
      backwardSyncs.push_back(std::move(sync));
    }

    std::map<int, size_t> syncBeforeCount;
    std::map<int, size_t> syncAfterCount;
    for (size_t dependencyIndex = 0;
         dependencyIndex < dependencies.size(); ++dependencyIndex) {
      const CrossCoreSyncDependency &dependency =
          dependencies[dependencyIndex];
      const GenericOperation &producer =
          module.operations.at(static_cast<size_t>(dependency.producer));
      const GenericOperation &consumer =
          module.operations.at(static_cast<size_t>(dependency.consumer));
      if (producer.blockId < 0 || consumer.blockId < 0)
        throw std::runtime_error(
            "CrossCoreGSS: synchronization endpoint has no block");
      const int64_t forwardEvent = forwardEventIds.at(
          {dependency.producerCore, dependency.consumerCore});
      const auto [producerAnchor, consumerAnchor] =
          dependencySyncAnchors[dependencyIndex];
      CreateCrossCoreSync(module, rewriter, producerAnchor,
                          "hivm.hir.sync_block_set", forwardEvent,
                          dependency.producerCore, true,
                          syncAfterCount[producerAnchor]++);
      CreateCrossCoreSync(
          module, rewriter, consumerAnchor,
          "hivm.hir.sync_block_wait", forwardEvent,
          dependency.consumerCore, false, 0,
          syncBeforeCount[consumerAnchor]++);
    }

    CrossCoreDynamicEventBuilder dynamicEvents(module, rewriter,
                                               dynamicEventPlacement);
    for (const CrossCoreBackwardSync &backward : backwardSyncs) {
      const CrossCoreSyncDependency &dependency =
          dependencies.at(backward.dependency);
      const auto [producerAnchor, consumerAnchor] =
          dependencySyncAnchors.at(backward.dependency);
      if (backward.eventIds.empty())
        throw std::runtime_error("CrossCoreGSS: empty backward event group");
      const bool dynamic = backward.eventIds.size() > 1 &&
                           backward.multibufferLoop >= 0 &&
                           backward.producerPreloadOffset.has_value() &&
                           backward.consumerPreloadOffset.has_value();
      const bool cvUnrollDynamic = backward.eventIds.size() > 1 &&
                                   backward.producerUnrollLoop >= 0 &&
                                   backward.consumerUnrollLoop >= 0;
      const int producerEvent =
          cvUnrollDynamic
              ? dynamicEvents.selectedCVUnrollEvent(
                    backward.producerUnrollLoop, backward.eventIds.front(),
                    producerAnchor)
          : dynamic ? dynamicEvents.selectedEvent(
                        backward.multibufferLoop,
                        static_cast<int64_t>(backward.eventIds.size()),
                        *backward.producerPreloadOffset,
                        backward.eventIds.front())
                  : -1;
      const int consumerEvent =
          cvUnrollDynamic
              ? dynamicEvents.selectedCVUnrollEvent(
                    backward.consumerUnrollLoop, backward.eventIds.front(),
                    consumerAnchor)
          : dynamic ? dynamicEvents.selectedEvent(
                        backward.multibufferLoop,
                        static_cast<int64_t>(backward.eventIds.size()),
                        *backward.consumerPreloadOffset,
                        backward.eventIds.front())
                  : -1;
      if (dependency.producerCore == SplitMixCoreType::Cube &&
          dependency.consumerCore == SplitMixCoreType::Vector) {
        for (int64_t eventId : backward.eventIds)
          for (int64_t repeat = 0; repeat < backward.eventIdRepeatNum;
               ++repeat)
            CreateCrossCoreSync(module, rewriter, backward.loop,
                                "hivm.hir.sync_block_set", eventId,
                                SplitMixCoreType::Vector, false);
        if (dynamic || cvUnrollDynamic) {
          const size_t producerBeforeOffset =
              dynamicEventPlacement ==
                      CrossCoreDynamicEventPlacement::BeforeSyncAnchor
                  ? 0
                  : syncBeforeCount[producerAnchor];
          ++syncBeforeCount[producerAnchor];
          CreateCrossCoreDynamicSync(
              module, rewriter, producerAnchor,
              "hivm.hir.sync_block_wait", producerEvent,
              SplitMixCoreType::Cube, false, 0,
              producerBeforeOffset);
          CreateCrossCoreDynamicSync(
              module, rewriter, consumerAnchor,
              "hivm.hir.sync_block_set", consumerEvent,
              SplitMixCoreType::Vector, true,
              syncAfterCount[consumerAnchor]++);
        } else {
          CreateCrossCoreSync(module, rewriter, producerAnchor,
                              "hivm.hir.sync_block_wait",
                              backward.eventIds.front(),
                              SplitMixCoreType::Cube, false, 0,
                              syncBeforeCount[producerAnchor]++);
          CreateCrossCoreSync(module, rewriter, consumerAnchor,
                              "hivm.hir.sync_block_set",
                              backward.eventIds.front(),
                              SplitMixCoreType::Vector, true,
                              syncAfterCount[consumerAnchor]++);
        }
        for (int64_t eventId : backward.eventIds)
          for (int64_t repeat = 0; repeat < backward.eventIdRepeatNum;
               ++repeat)
            CreateCrossCoreSync(module, rewriter, backward.loop,
                                "hivm.hir.sync_block_wait", eventId,
                                SplitMixCoreType::Cube, true,
                                syncAfterCount[backward.loop]++);
      } else if (dependency.producerCore == SplitMixCoreType::Vector &&
                 dependency.consumerCore == SplitMixCoreType::Cube) {
        for (int64_t eventId : backward.eventIds)
          for (int64_t repeat = 0; repeat < backward.eventIdRepeatNum;
               ++repeat)
            CreateCrossCoreSync(module, rewriter, backward.loop,
                                "hivm.hir.sync_block_set", eventId,
                                SplitMixCoreType::Cube, false);
        if (dynamic || cvUnrollDynamic) {
          const size_t producerBeforeOffset =
              dynamicEventPlacement ==
                      CrossCoreDynamicEventPlacement::BeforeSyncAnchor
                  ? 0
                  : syncBeforeCount[producerAnchor];
          ++syncBeforeCount[producerAnchor];
          CreateCrossCoreDynamicSync(
              module, rewriter, producerAnchor,
              "hivm.hir.sync_block_wait", producerEvent,
              SplitMixCoreType::Vector, false, 0,
              producerBeforeOffset);
          CreateCrossCoreDynamicSync(
              module, rewriter, consumerAnchor,
              "hivm.hir.sync_block_set", consumerEvent,
              SplitMixCoreType::Cube, true,
              syncAfterCount[consumerAnchor]++);
        } else {
          CreateCrossCoreSync(module, rewriter, producerAnchor,
                              "hivm.hir.sync_block_wait",
                              backward.eventIds.front(),
                              SplitMixCoreType::Vector, false, 0,
                              syncBeforeCount[producerAnchor]++);
          CreateCrossCoreSync(module, rewriter, consumerAnchor,
                              "hivm.hir.sync_block_set",
                              backward.eventIds.front(),
                              SplitMixCoreType::Cube, true,
                              syncAfterCount[consumerAnchor]++);
        }
        for (int64_t eventId : backward.eventIds)
          for (int64_t repeat = 0; repeat < backward.eventIdRepeatNum;
               ++repeat)
            CreateCrossCoreSync(module, rewriter, backward.loop,
                                "hivm.hir.sync_block_wait", eventId,
                                SplitMixCoreType::Vector, true,
                                syncAfterCount[backward.loop]++);
      }
    }

    for (int anchor : barrierAnchors)
      CreateCrossCoreBarrierAll(module, rewriter, anchor);
  }
  return module;
}

inline GenericModule RunCrossCoreGSS(GenericModule module) {
  return RunCrossCoreBlockSyncAnalysis(
      std::move(module), CrossCoreDynamicEventPlacement::LoopBodyStart);
}

inline GenericModule RunInjectBlockSyncAnalysis(
    GenericModule module, const std::set<int> &selectedFunctions) {
  return RunCrossCoreBlockSyncAnalysis(
      std::move(module),
      CrossCoreDynamicEventPlacement::BeforeSyncAnchor,
      &selectedFunctions);
}

} // namespace cvub

#endif
