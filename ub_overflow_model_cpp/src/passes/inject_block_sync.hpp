#ifndef UB_OVERFLOW_MODEL_CPP_INJECT_BLOCK_SYNC_HPP
#define UB_OVERFLOW_MODEL_CPP_INJECT_BLOCK_SYNC_HPP

#include "cross_core_gss.hpp"

namespace cvub {

inline std::string InjectBlockSyncProperties(int64_t flag,
                                             SplitMixCoreType core,
                                             const std::string &tpipe) {
  return "{pipe = #hivm.pipe<PIPE_MTE2>, static_flag_id = " +
         std::to_string(flag) +
         " : i64, tcore_type = #hivm.tcore_type<" +
         SplitMixCoreTypeName(core) + ">, tpipe = #hivm.pipe<" + tpipe +
         ">}";
}

inline int CreateInjectBlockOperation(GenericModule &module,
                                      GenericRewriter &rewriter,
                                      const GenericOperation &anchor,
                                      const std::string &name,
                                      const std::string &properties,
                                      size_t position) {
  const int operation = rewriter.createOperation(
      anchor.parentId, anchor.regionId, anchor.blockId, name, {}, {}, {},
      properties);
  module.operations.at(static_cast<size_t>(operation)).effects = "none";
  rewriter.insertToBlock(anchor.blockId, position, operation);
  return operation;
}

inline void InsertInjectBlockHandshake(GenericModule &module,
                                       GenericRewriter &rewriter,
                                       int anchorId, bool after,
                                       int64_t flag,
                                       SplitMixCoreType setCore,
                                       SplitMixCoreType waitCore,
                                       const std::string &tpipe) {
  const GenericOperation anchor =
      module.operations.at(static_cast<size_t>(anchorId));
  size_t position = static_cast<size_t>(
      module.operations.at(static_cast<size_t>(anchorId)).ordinal);
  if (after)
    ++position;
  CreateInjectBlockOperation(
      module, rewriter, anchor, "hivm.hir.sync_block_set",
      InjectBlockSyncProperties(flag, setCore, tpipe), position++);
  CreateInjectBlockOperation(
      module, rewriter, anchor, "hivm.hir.sync_block_wait",
      InjectBlockSyncProperties(flag, waitCore, tpipe), position);
}

inline void InsertInjectBlockAllSequence(GenericModule &module,
                                         GenericRewriter &rewriter,
                                         int anchorId, bool after) {
  const GenericOperation anchor =
      module.operations.at(static_cast<size_t>(anchorId));
  size_t position = static_cast<size_t>(
      module.operations.at(static_cast<size_t>(anchorId)).ordinal);
  if (after)
    ++position;
  CreateInjectBlockOperation(module, rewriter, anchor,
                             "hivm.hir.pipe_barrier",
                             "{pipe = #hivm.pipe<PIPE_ALL>}", position++);
  const std::array<std::tuple<const char *, int64_t, SplitMixCoreType>, 4>
      syncs = {{{"hivm.hir.sync_block_set", 8,
                 SplitMixCoreType::Vector},
                {"hivm.hir.sync_block_wait", 8, SplitMixCoreType::Cube},
                {"hivm.hir.sync_block_set", 9, SplitMixCoreType::Cube},
                {"hivm.hir.sync_block_wait", 9,
                 SplitMixCoreType::Vector}}};
  for (const auto &[name, flag, core] : syncs)
    CreateInjectBlockOperation(
        module, rewriter, anchor, name,
        InjectBlockSyncProperties(flag, core, "PIPE_S"), position++);
}

inline bool IsInjectBlockAllTarget(const GenericOperation &operation) {
  static const std::unordered_set<std::string> targets = {
      "hivm.hir.load", "hivm.hir.mmadL1", "hivm.hir.fixpipe",
      "hivm.hir.store", "hivm.hir.copy", "tensor.insert_slice"};
  return targets.count(operation.name) != 0;
}

inline std::string InjectBlockFusionKind(const GenericOperation &function) {
  std::string kind =
      FindDictionaryValue(function.attributes, "hfusion.fusion_kind");
  if (kind.empty())
    kind = FindDictionaryValue(function.properties, "hfusion.fusion_kind");
  return SplitMixEnumValue(kind);
}

inline std::optional<SplitMixCoreType>
InjectBlockOperationCore(const GenericModule &module,
                         const GenericOperation &operation) {
  std::string core =
      FindDictionaryValue(operation.attributes, "hivm.tcore_type");
  if (core.empty())
    core = FindDictionaryValue(operation.properties, "hivm.tcore_type");
  core = SplitMixEnumValue(core);
  if (core == "CUBE")
    return SplitMixCoreType::Cube;
  if (core == "VECTOR")
    return SplitMixCoreType::Vector;
  if (core == "CUBE_AND_VECTOR" || core == "CUBE_OR_VECTOR")
    return SplitMixCoreType::Both;

  if (operation.name != "func.call")
    return std::nullopt;
  std::string callee = FindDictionaryValue(operation.properties, "callee");
  if (callee.empty())
    callee = FindDictionaryValue(operation.attributes, "callee");
  callee = trim(callee);
  if (!callee.empty() && callee.front() == '@')
    callee.erase(callee.begin());
  if (callee.size() >= 2 && callee.front() == '"' && callee.back() == '"')
    callee = callee.substr(1, callee.size() - 2);
  for (const GenericOperation &function : module.operations) {
    if (function.name != "func.func")
      continue;
    std::string symbol = FindDictionaryValue(function.properties, "sym_name");
    if (symbol.empty())
      symbol = FindDictionaryValue(function.attributes, "sym_name");
    symbol = trim(symbol);
    if (symbol.size() >= 2 && symbol.front() == '"' && symbol.back() == '"')
      symbol = symbol.substr(1, symbol.size() - 2);
    if (symbol != callee)
      continue;
    const std::string functionCore = SplitMixEnumValue(
        FindDictionaryValue(function.attributes, "hivm.func_core_type"));
    if (functionCore == "AIC")
      return SplitMixCoreType::Cube;
    if (functionCore == "AIV")
      return SplitMixCoreType::Vector;
  }
  return std::nullopt;
}

inline bool IsInjectBlockTransparentUser(const GenericOperation &operation) {
  static const std::unordered_set<std::string> transparent = {
      "tensor.collapse_shape", "tensor.expand_shape", "memref.collapse_shape",
      "memref.expand_shape", "memref.subview", "memref.view",
      "memref.reinterpret_cast", "bufferization.to_memref",
      "bufferization.to_buffer", "bufferization.to_tensor"};
  return transparent.count(operation.name) != 0;
}

inline std::vector<int> InjectBlockTerminalUsers(
    const GenericModule &module, const GenericOperation &operation,
    const GenericModuleAnalysisSnapshot &analysis) {
  std::vector<int> terminals;
  std::queue<int> worklist;
  std::set<int> visited;
  for (int result : operation.results)
    for (int user : analysis.users(result))
      if (visited.insert(user).second)
        worklist.push(user);
  while (!worklist.empty()) {
    const int userId = worklist.front();
    worklist.pop();
    const GenericOperation &user =
        module.operations.at(static_cast<size_t>(userId));
    if (!IsInjectBlockTransparentUser(user)) {
      terminals.push_back(userId);
      continue;
    }
    for (int result : user.results)
      for (int next : analysis.users(result))
        if (visited.insert(next).second)
          worklist.push(next);
  }
  return terminals;
}

inline void RunInjectBlockShallowSync(GenericModule &module,
                                      GenericRewriter &rewriter,
                                      const GenericOperation &function) {
  const GenericModuleAnalysisSnapshot analysis(
      module, kGenericAnalysisUsers | kGenericAnalysisFunctionDescendants);
  uint64_t flag = 0;
  const std::vector<int> descendants = analysis.descendants(function);
  for (int operationId : descendants) {
    const GenericOperation operation =
        module.operations.at(static_cast<size_t>(operationId));
    if (operation.name != "hivm.hir.matmul" &&
        operation.name != "hivm.hir.mix_matmul" &&
        operation.name != "func.call")
      continue;
    const std::optional<SplitMixCoreType> producerCore =
        InjectBlockOperationCore(module, operation);
    if (!producerCore || *producerCore == SplitMixCoreType::Both)
      continue;
    std::set<SplitMixCoreType> userCores;
    for (int userId : InjectBlockTerminalUsers(module, operation, analysis)) {
      const std::optional<SplitMixCoreType> userCore = InjectBlockOperationCore(
          module, module.operations.at(static_cast<size_t>(userId)));
      if (userCore)
        userCores.insert(*userCore);
    }
    if (userCores.empty())
      continue;

    const GenericOperation anchor =
        module.operations.at(static_cast<size_t>(operationId));
    size_t position = static_cast<size_t>(anchor.ordinal) + 1;
    const std::string mode = *producerCore == SplitMixCoreType::Cube
                                 ? "ALL_CUBE"
                                 : "ALL_VECTOR";
    const int block = CreateInjectBlockOperation(
        module, rewriter, anchor, "hivm.hir.sync_block",
        "{static_flag_id = " + std::to_string(flag++ & 0xf) +
            " : i64, sync_block_mode = #hivm.sync_block_mode<" + mode +
            ">}",
        position++);
    (void)block;
    if (userCores.size() > 1 || *userCores.begin() != *producerCore) {
      const SplitMixCoreType consumerCore =
          *producerCore == SplitMixCoreType::Cube ? SplitMixCoreType::Vector
                                                  : SplitMixCoreType::Cube;
      const std::string tpipe = *producerCore == SplitMixCoreType::Cube
                                    ? "PIPE_FIX"
                                    : "PIPE_MTE3";
      const int64_t handshakeFlag = static_cast<int64_t>(flag++ & 0xf);
      CreateInjectBlockOperation(
          module, rewriter, anchor, "hivm.hir.sync_block_set",
          InjectBlockSyncProperties(handshakeFlag, *producerCore, tpipe),
          position++);
      CreateInjectBlockOperation(
          module, rewriter, anchor, "hivm.hir.sync_block_wait",
          InjectBlockSyncProperties(handshakeFlag, consumerCore, tpipe),
          position++);
    }
  }
}

inline GenericModule RunInjectBlockSync(GenericModule module,
                                        bool blockAllSync,
                                        bool disableAutoInjectBlockSync) {
  std::vector<int> mixFunctions;
  for (const GenericOperation &operation : module.operations)
    if (IsSplitMixFunction(operation))
      mixFunctions.push_back(operation.id);
  if (mixFunctions.empty())
    return module;

  const bool moduleDisablesAutoInjection = std::any_of(
      module.operations.begin(), module.operations.end(),
      [](const GenericOperation &operation) {
        return operation.name == "builtin.module" &&
               (operation.attributes.find(
                    "hivm.disable_auto_inject_block_sync") !=
                    std::string::npos ||
                operation.properties.find(
                    "hivm.disable_auto_inject_block_sync") !=
                    std::string::npos);
      });

  // Native InjectBlockSync's ordinary MIX path uses the same cross-core
  // memory-conflict graph and forward/backward event schedule as the shared
  // BLOCKSYNC analysis.  Its code generator differs from GSS for CV-unrolled
  // multi-buffer selectors: the constant/add/index-cast chain is created at
  // each sync endpoint (SyncCodegen::CreateSetWaitBlockOpForMultiBuffer), not
  // at the loop-body start.  Reuse the common source-aligned analysis while
  // selecting that native placement mode.
  std::set<int> ordinaryMixFunctions;
  if (!moduleDisablesAutoInjection && !disableAutoInjectBlockSync &&
      !blockAllSync) {
    for (int functionId : mixFunctions) {
      const GenericOperation &function =
          module.operations.at(static_cast<size_t>(functionId));
      if (InjectBlockFusionKind(function) != "SHALLOW_CV")
        ordinaryMixFunctions.insert(functionId);
    }
  }
  if (!ordinaryMixFunctions.empty())
    module = RunInjectBlockSyncAnalysis(std::move(module),
                                        ordinaryMixFunctions);

  GenericRewriter rewriter(module);
  for (int functionId : mixFunctions) {
    if (ordinaryMixFunctions.count(functionId) != 0)
      continue;
    const GenericOperation function =
        module.operations.at(static_cast<size_t>(functionId));
    const std::optional<int> baseAddress =
        GetFFTSBaseAddressArgument(module, function);
    if (!baseAddress)
      throw std::runtime_error(
          "InjectBlockSync: MIX function has no FFTS base address argument");
    const GenericRegion &region =
        module.regions.at(static_cast<size_t>(function.regions.front()));
    const int entryBlock = region.blocks.front();
    const int setBase = rewriter.createOperation(
        functionId, function.regions.front(), entryBlock,
        "hivm.hir.set_ffts_base_addr", {}, {*baseAddress}, {"i64"});
    module.operations.at(static_cast<size_t>(setBase)).effects = "none";
    rewriter.insertToBlock(entryBlock, 0, setBase);

    if (moduleDisablesAutoInjection || disableAutoInjectBlockSync)
      continue;
    if (blockAllSync) {
      const GenericModuleAnalysisSnapshot analysis(
          module, kGenericAnalysisFunctionDescendants);
      std::vector<int> targets;
      for (int operation : analysis.descendants(
               module.operations.at(static_cast<size_t>(functionId))))
        if (IsInjectBlockAllTarget(
                module.operations.at(static_cast<size_t>(operation))))
          targets.push_back(operation);
      for (int target : targets) {
        InsertInjectBlockAllSequence(module, rewriter, target, false);
        InsertInjectBlockAllSequence(module, rewriter, target, true);
      }
    } else if (InjectBlockFusionKind(function) == "SHALLOW_CV") {
      RunInjectBlockShallowSync(module, rewriter, function);
    }
  }
  return module;
}

} // namespace cvub

#endif
