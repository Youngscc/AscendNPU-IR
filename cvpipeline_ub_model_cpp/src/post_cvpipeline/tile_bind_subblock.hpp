#ifndef CVPIPELINE_UB_MODEL_CPP_POST_CVPIPELINE_TILE_BIND_SUBBLOCK_HPP
#define CVPIPELINE_UB_MODEL_CPP_POST_CVPIPELINE_TILE_BIND_SUBBLOCK_HPP

#include "ir_utils.hpp"
#include "types.hpp"

#include <set>
#include <string>
#include <vector>

namespace cvub {
namespace tile_bind_subblock_detail {

// Attribute and op-name spellings mirrored from the real compiler pass
// (bishengir/lib/Dialect/HIVM/Transforms/TileAndBindSubBlock.cpp and its
// helpers).  Keeping the names identical makes the modeled IR observable-match
// the post-pass IR the suffix planner ultimately consumes.
inline constexpr const char *kDisableAttrName =
    "hivm.disable_auto_tile_and_bind_subblock";
inline constexpr const char *kLimitSubBlockAttrName = "limit_sub_block_id0";
inline constexpr const char *kFuncCoreTypeAttr = "hivm.func_core_type";
inline constexpr const char *kAIVCoreType = "#hivm.func_core_type<AIV>";
inline constexpr const char *kAICCoreType = "#hivm.func_core_type<AIC>";
inline constexpr const char *kPartOfMixAttr = "hivm.part_of_mix";
inline constexpr const char *kImplicitTransposeMark =
    "MayImplicitTransposeWithLastAxis";
inline constexpr const char *kBatchMatmulAttr = "batch_matmul";

inline std::string CoreTypeOf(const GenericOperation &function) {
  return trim(IRDictionaryValue(function.attributes, kFuncCoreTypeAttr));
}

inline bool HasUnitAttribute(const std::string &dictionary,
                             const std::string &name) {
  if (dictionary.size() < 2 || dictionary.front() != '{' ||
      dictionary.back() != '}')
    return false;
  for (const std::string &entry :
       splitTopLevel(dictionary.substr(1, dictionary.size() - 2)))
    if (trim(entry) == name)
      return true;
  return false;
}

inline bool IsDeviceFunction(const GenericOperation &operation) {
  if (operation.name != "func.func" || operation.regions.empty())
    return false;
  const std::string kind =
      IRDictionaryValue(operation.attributes, "hacc.function_kind");
  return trim(kind) == "#hacc.function_kind<DEVICE>";
}

inline bool IsMixPart(const GenericOperation &function) {
  return HasUnitAttribute(function.attributes, kPartOfMixAttr);
}

// collectMixAicAndAivFuncs in the real pass only collects DEVICE functions that
// carry hivm.part_of_mix and a concrete AIV/AIC core type.  The projected AIV
// module still carries part_of_mix (see SplitMixKernelAIVProjection), so this
// selects the single projected function.
inline std::vector<int> CollectAivFunctions(const GenericModule &module) {
  std::vector<int> result;
  for (const GenericOperation &operation : module.operations)
    if (IsDeviceFunction(operation) && IsMixPart(operation) &&
        CoreTypeOf(operation) == kAIVCoreType)
      result.push_back(operation.id);
  return result;
}

inline std::vector<int> CollectAicFunctions(const GenericModule &module) {
  std::vector<int> result;
  for (const GenericOperation &operation : module.operations)
    if (IsDeviceFunction(operation) && IsMixPart(operation) &&
        CoreTypeOf(operation) == kAICCoreType)
      result.push_back(operation.id);
  return result;
}

// Depth-first walk of every operation nested under `rootId` (inclusive),
// mirroring the real pass's func->walk().  Stores inside tiled vector loops are
// nested, so a top-level scan would silently miss tileable functions.
inline std::vector<int> AllOperations(const GenericModule &module, int rootId) {
  std::vector<int> result;
  std::vector<int> worklist{rootId};
  std::set<int> visited;
  while (!worklist.empty()) {
    const int opId = worklist.back();
    worklist.pop_back();
    if (!visited.insert(opId).second)
      continue;
    result.push_back(opId);
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(opId));
    for (int regionId : operation.regions) {
      for (int blockId :
           module.regions.at(static_cast<size_t>(regionId)).blocks) {
        const std::vector<int> &children =
            module.blocks.at(static_cast<size_t>(blockId)).operations;
        worklist.insert(worklist.end(), children.begin(), children.end());
      }
    }
  }
  return result;
}

inline std::vector<int> DirectRegionBlocks(const GenericModule &module,
                                           const GenericOperation &owner) {
  std::vector<int> blocks;
  for (int regionId : owner.regions)
    for (int blockId : module.regions.at(static_cast<size_t>(regionId)).blocks)
      blocks.push_back(blockId);
  return blocks;
}

inline bool IsFunctionBlockArgument(const GenericModule &module,
                                    const GenericOperation &function,
                                    int value) {
  for (int blockId : DirectRegionBlocks(module, function)) {
    const std::vector<int> &args =
        module.blocks.at(static_cast<size_t>(blockId)).arguments;
    if (std::find(args.begin(), args.end(), value) != args.end())
      return true;
  }
  return false;
}

inline const GenericOperation *Definition(const GenericModule &module,
                                          int value) {
  for (const GenericOperation &operation : module.operations)
    if (std::find(operation.results.begin(), operation.results.end(), value) !=
        operation.results.end())
      return &operation;
  return nullptr;
}

inline const GenericOperation *ReinterpretCastDef(const GenericModule &module,
                                                  int value) {
  const GenericOperation *def = Definition(module, value);
  if (def == nullptr || def->name != "memref.reinterpret_cast" ||
      def->operands.empty())
    return nullptr;
  return def;
}

// Mirrors hasImplicitTransposeWithLastAxisInAiv exactly: the hazard is raised
// solely by an annotation.mark annotated with MayImplicitTransposeWithLastAxis.
// The load/store snake-case attribute is intentionally NOT consulted, because
// without the real mark-propagation analysis consulting it could make the model
// apply the Exact fallback for a function the real pass would actually tile.
inline bool HasImplicitTransposeHazard(const GenericModule &module,
                                       const std::vector<int> &aivFuncs) {
  for (int functionId : aivFuncs) {
    for (int opId : AllOperations(module, functionId)) {
      const GenericOperation &operation =
          module.operations.at(static_cast<size_t>(opId));
      if (operation.name != "annotation.mark")
        continue;
      const std::string mark =
          IRDictionaryValue(operation.attributes, kImplicitTransposeMark);
      if (!mark.empty() && mark != "false")
        return true;
      if (HasUnitAttribute(operation.attributes, kImplicitTransposeMark))
        return true;
    }
  }
  return false;
}

// Mirrors areLoadAndStoreSameAddress: a load source and a store destination
// both trace through memref.reinterpret_cast to the same function block
// argument.
inline bool HasSameAddressHazard(const GenericModule &module,
                                 const std::vector<int> &aivFuncs) {
  for (int functionId : aivFuncs) {
    const GenericOperation &function =
        module.operations.at(static_cast<size_t>(functionId));
    std::set<int> loadRootArgs;
    for (int opId : AllOperations(module, functionId)) {
      const GenericOperation &operation =
          module.operations.at(static_cast<size_t>(opId));
      if (operation.name != "hivm.hir.load" || operation.operands.empty())
        continue;
      const GenericOperation *cast =
          ReinterpretCastDef(module, operation.operands.front());
      if (cast != nullptr && IsFunctionBlockArgument(module, function,
                                                     cast->operands.front()))
        loadRootArgs.insert(cast->operands.front());
    }
    for (int opId : AllOperations(module, functionId)) {
      const GenericOperation &operation =
          module.operations.at(static_cast<size_t>(opId));
      if (operation.name != "hivm.hir.store" || operation.operands.size() < 2)
        continue;
      const GenericOperation *cast =
          ReinterpretCastDef(module, operation.operands[1]);
      if (cast != nullptr && loadRootArgs.count(cast->operands.front()) != 0)
        return true;
    }
  }
  return false;
}

// Mirrors hasBatchMatmulLoopInAicFuncs: any mmadL1 carrying batch_matmul in an
// AIC function forces the pass to fall back.  Projected AIV modules have no AIC
// functions, so this never fires there, but it is modeled for completeness.
inline bool HasBatchMatmulHazard(const GenericModule &module,
                                 const std::vector<int> &aicFuncs) {
  for (int functionId : aicFuncs)
    for (int opId : AllOperations(module, functionId)) {
      const GenericOperation &operation =
          module.operations.at(static_cast<size_t>(opId));
      if (operation.name == "hivm.hir.mmadL1" &&
          HasUnitAttribute(operation.attributes, kBatchMatmulAttr))
        return true;
    }
  return false;
}

inline bool IsStoreLikeOp(const GenericOperation &operation) {
  return operation.name == "hivm.hir.store" ||
         operation.name == "hivm.hir.indirect_store" ||
         operation.name == "hivm.hir.copy";
}

// Mirrors isCopytoL1: a copy is only restricted when its destination traces to
// an L1 allocation.  Store/indirect_store are always eligible.
inline bool IsLimitEligible(const GenericModule &module,
                            const GenericOperation &operation) {
  if (operation.name == "hivm.hir.store" ||
      operation.name == "hivm.hir.indirect_store")
    return true;
  if (operation.name != "hivm.hir.copy" || operation.operands.size() < 2)
    return false;
  const GenericOperation *def = Definition(module, operation.operands[1]);
  if (def == nullptr || def->name != "memref.alloc" ||
      def->resultTypes.empty())
    return false;
  const std::string &type = def->resultTypes.front();
  return type.find("#hivm.address_space<L1>") != std::string::npos ||
         type.find("#hivm.mem_scope<L1>") != std::string::npos;
}

// A function with no store/copy/indirect_store leaves the isFailed flag true in
// attemptBindSubBlock, which makes the real pass revert to
// limitUniqueSubBlockToStore (a no-op when there is nothing to restrict).
inline bool HasAnyStoreLikeOp(const GenericModule &module, int functionId) {
  for (int opId : AllOperations(module, functionId)) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(opId));
    if (IsStoreLikeOp(operation))
      return true;
  }
  return false;
}

// Mirrors the dynamic-store guard inside tileAndSliceOp: any store whose
// source/destination is not a static shaped type aborts tiling and reverts to
// limitUniqueSubBlockToStore.
inline bool IsDynamicStore(const GenericOperation &store) {
  auto isStatic = [](const std::string &type) {
    const std::string trimmed = trim(type);
    if (trimmed.rfind("tensor<", 0) != 0 && trimmed.rfind("memref<", 0) != 0)
      return false;
    return trimmed.find('?') == std::string::npos;
  };
  if (store.operandTypes.size() < 2)
    return true;
  return !isStatic(store.operandTypes[0]) || !isStatic(store.operandTypes[1]);
}

inline bool FunctionHasDynamicStore(const GenericModule &module,
                                    int functionId) {
  for (int opId : AllOperations(module, functionId)) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(opId));
    if (operation.name == "hivm.hir.store" && IsDynamicStore(operation))
      return true;
  }
  return false;
}

// Rewraps a single valueless store/indirect_store/L1-copy in an
// `if (sub_block_idx == 0)` guard carrying `limit_sub_block_id0`, matching the
// case-1 rewrite in LimitUniqueSubBlockIdToStoreCopy.  Returns false if the
// operation carries results (case 2), which this lightweight model does not
// reproduce, so the caller can fail closed.
inline bool WrapOneStoreInSubBlockGuard(GenericModule &module, int storeId) {
  const int blockId =
      module.operations.at(static_cast<size_t>(storeId)).blockId;
  const int regionId =
      module.operations.at(static_cast<size_t>(storeId)).regionId;
  const int parentId =
      module.operations.at(static_cast<size_t>(storeId)).parentId;
  const bool hasResults =
      !module.operations.at(static_cast<size_t>(storeId)).results.empty();
  if (hasResults || blockId < 0)
    return false;
  GenericBlock &block = module.blocks.at(static_cast<size_t>(blockId));
  const auto position =
      std::find(block.operations.begin(), block.operations.end(), storeId);
  if (position == block.operations.end())
    return false;
  const size_t ordinal = static_cast<size_t>(
      std::distance(block.operations.begin(), position));

  GenericRewriter rewriter(module);
  const int idx = rewriter.createOperation(parentId, regionId, blockId,
                                           "hivm.hir.get_sub_block_idx", {"i64"});
  const int cast = rewriter.createOperation(
      parentId, regionId, blockId, "arith.index_cast", {"index"},
      {module.operations.at(static_cast<size_t>(idx)).results.front()}, {"i64"});
  const int zero = rewriter.createOperation(parentId, regionId, blockId,
                                            "arith.constant", {"index"}, {}, {},
                                            "", "{value = 0 : index}");
  const int cond = rewriter.createOperation(
      parentId, regionId, blockId, "arith.cmpi", {"i1"},
      {module.operations.at(static_cast<size_t>(cast)).results.front(),
       module.operations.at(static_cast<size_t>(zero)).results.front()},
      {"index", "index"}, "", "{predicate = 0 : i64}");
  const int ifOp = rewriter.createOperation(
      parentId, regionId, blockId, "scf.if", {},
      {module.operations.at(static_cast<size_t>(cond)).results.front()}, {"i1"},
      "", "{limit_sub_block_id0}");

  const int plumbing[4] = {idx, cast, zero, cond};
  for (size_t inserted = 0; inserted < 4; ++inserted)
    rewriter.insertToBlock(blockId, ordinal + inserted,
                           plumbing[inserted]);
  rewriter.insertToBlock(blockId, ordinal + 4, ifOp);

  const int ifRegion = rewriter.createRegion(ifOp);
  const int ifBlock = rewriter.createBlock(ifRegion, {});

  rewriter.removeFromBlock(blockId, storeId);
  rewriter.appendToBlock(ifBlock, storeId);
  GenericOperation &moved = module.operations.at(static_cast<size_t>(storeId));
  moved.parentId = ifOp;
  moved.regionId = ifRegion;
  moved.blockId = ifBlock;

  const int yield = rewriter.createOperation(ifOp, ifRegion, ifBlock,
                                             "scf.yield", {});
  rewriter.appendToBlock(ifBlock, yield);
  return true;
}

// Mirrors limitUniqueSubBlockToStore over the AIV functions of the module.
// Returns false if a result-bearing eligible operation prevents an exact
// rewrite, so the caller can downgrade precision instead of silently producing
// an inexact result.
inline bool LimitStoresToSubBlockZero(GenericModule &module,
                                      const std::vector<int> &aivFuncs) {
  for (int functionId : aivFuncs) {
    std::vector<int> eligible;
    for (int opId : AllOperations(module, functionId)) {
      const GenericOperation &operation =
          module.operations.at(static_cast<size_t>(opId));
      if (IsLimitEligible(module, operation))
        eligible.push_back(opId);
    }
    for (int storeId : eligible)
      if (!WrapOneStoreInSubBlockGuard(module, storeId))
        return false;
  }
  return true;
}

inline void AddDiagnostic(StageResult &result, int operationId,
                          const std::string &operation,
                          const std::string &reason) {
  result.precision = Precision::Incomplete;
  result.diagnostics.push_back(
      {"TileAndBindSubBlock", "", operationId, operation, reason});
}

} // namespace tile_bind_subblock_detail

// Models createTileAndBindSubBlockPass on a projected AIV module.
//
// Decision order mirrored from the real pass:
//   1. hivm.disable_auto_tile_and_bind_subblock module attribute -> no-op.
//   2. enableTile=false -> apply limitUniqueSubBlockToStore to every AIV func.
//   3. batch-matmul / implicit-transpose / same-address hazards -> recognized
//      skip, apply limitUniqueSubBlockToStore (Exact).
//   4. attemptBindSubBlock per AIV func:
//        - no store-like op            -> isFailed revert (recognized, Exact).
//        - dynamic-shaped store source -> tileAndSliceOp abort (recognized,
//                                         Exact) with limitUniqueSubBlockToStore.
//        - static-shaped store         -> the real pass would derive a tiling
//                                         dimension and wrap the body in a
//                                         sub-block loop.  That bubble-up tiling
//                                         is not reproducible exactly here, so
//                                         the form is reported incomplete and the
//                                         legal IR is left untouched (fail
//                                         closed: never silently exact).
inline StageResult RunTileAndBindSubBlock(GenericModule module,
                                          bool enableTile) {
  using namespace tile_bind_subblock_detail;
  StageResult result;
  result.module = std::move(module);
  ValidateGenericModule(result.module);

  if (HasUnitAttribute(result.module.operations.front().attributes,
                       kDisableAttrName))
    return result; // Recognized no-op, Exact.

  const std::vector<int> aivFuncs = CollectAivFunctions(result.module);
  const std::vector<int> aicFuncs = CollectAicFunctions(result.module);
  if (aivFuncs.empty())
    return result; // Nothing to bind, Exact.

  const auto limitAll = [&]() -> bool {
    return LimitStoresToSubBlockZero(result.module, aivFuncs);
  };
  const std::string resultBearerReason =
      "result-bearing store/copy cannot be restricted to a sub-block by the "
      "exact model";

  if (!enableTile) {
    if (!limitAll())
      AddDiagnostic(result, -1, "", resultBearerReason);
    ValidateGenericModule(result.module);
    return result;
  }

  if (HasBatchMatmulHazard(result.module, aicFuncs) ||
      HasImplicitTransposeHazard(result.module, aivFuncs) ||
      HasSameAddressHazard(result.module, aivFuncs)) {
    if (!limitAll())
      AddDiagnostic(result, -1, "", resultBearerReason);
    ValidateGenericModule(result.module);
    return result; // Recognized skip, Exact.
  }

  // attemptBindSubBlock across the AIV functions.  Projected modules carry a
  // single AIV function, but the per-function classification generalizes.
  for (int functionId : aivFuncs) {
    if (!HasAnyStoreLikeOp(result.module, functionId))
      continue; // isFailed revert -> limitUniqueSubBlockToStore no-op, Exact.
    if (FunctionHasDynamicStore(result.module, functionId)) {
      // tileAndSliceOp aborts on a dynamic store; the real pass reverts and
      // applies limitUniqueSubBlockToStore.  Recognized rollback, Exact.
      if (!LimitStoresToSubBlockZero(result.module, {functionId}))
        AddDiagnostic(result, functionId, "func.func", resultBearerReason);
      continue;
    }
    // Static-shaped store: the real pass would wrap the body in a sub-block
    // loop and slice the store.  That transformation depends on the
    // DimensionAnalyzer + bubble-up-extract-slice machinery, which is not
    // reproducible exactly.  Fail closed: keep the legal IR and report the
    // function as an incomplete (unmodeled potentially-successful) form.
    AddDiagnostic(result, functionId, "func.func",
                  "AIV sub-block tiling would transform this function but the "
                  "exact sub-block loop IR is not modeled");
  }

  ValidateGenericModule(result.module);
  return result;
}

} // namespace cvub

#endif
