#ifndef CVPIPELINE_UB_MODEL_CPP_POST_CVPIPELINE_TILE_BIND_SUBBLOCK_HPP
#define CVPIPELINE_UB_MODEL_CPP_POST_CVPIPELINE_TILE_BIND_SUBBLOCK_HPP

#include "ir_utils.hpp"
#include "tile_cube_vector_loop.hpp"
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

// collectMixAicAndAivFuncs in the real pass collects every function carrying
// hivm.part_of_mix and a concrete AIV/AIC core type; it does NOT filter on
// hacc.function_kind.  This model additionally narrows the selection to DEVICE
// functions, which is safe for the projected AIV modules this stage consumes
// (SplitMixKernelAIVProjection emits DEVICE functions) and only ever makes the
// model select fewer functions than the real pass.
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

// runTileAndBindSubBlockEarlyPatterns always runs before function collection.
// CanonicalizeAllocToTensor replaces a one-use memref.alloc ->
// bufferization.to_tensor chain with tensor.empty and erases the allocation.
// Until the semantic rewrite is modeled, detecting applicability is required to
// prevent no-store and hazard paths from being mislabeled Exact.
inline const GenericOperation *ApplicableAllocToTensorEarlyPattern(
    const GenericModule &module) {
  for (const GenericOperation &allocation : module.operations) {
    if (allocation.name != "memref.alloc" || allocation.results.size() != 1)
      continue;
    const int value = allocation.results.front();
    const GenericOperation *onlyUser = nullptr;
    size_t useCount = 0;
    for (const GenericOperation &user : module.operations)
      for (int operand : user.operands)
        if (operand == value) {
          ++useCount;
          onlyUser = &user;
        }
    if (useCount == 1 && onlyUser != nullptr &&
        onlyUser->name == "bufferization.to_tensor")
      return &allocation;
  }
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

// Mirrors hasImplicitTransposeWithLastAxisInAiv: the hazard is raised solely
// by an annotation.mark annotated with MayImplicitTransposeWithLastAxis.  The
// real pass consults markOp.isAnnotatedBy(...), which reduces to hasAttr(key)
// and is therefore value-agnostic, so any spelling of the attribute (including
// an explicit `= false`) is a hazard.  The load/store snake-case attribute is
// intentionally NOT consulted, because without the real mark-propagation
// analysis consulting it could make the model apply the Exact fallback for a
// function the real pass would actually tile.
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
      if (!mark.empty())
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
// an L1 allocation, and the real pass checks the memref memory space via
// getOptionalHIVMAddressSpace (i.e. #hivm.address_space<L1>) only — a bare
// #hivm.mem_scope<L1> does not qualify.  Store/indirect_store are always
// eligible.
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
  return type.find("#hivm.address_space<L1>") != std::string::npos;
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

// Mirrors the direct-operand dynamic check that opens the dynamic-store guard
// inside tileAndSliceOp: a store whose direct source/destination operand type
// is not a static shaped type is a candidate for the abort path.  This is only
// the first half of the real decision; see ClassifyDynamicStores for the
// slice-tracing refinement.
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

// The real pass (TileAndBindSubBlock.cpp:859-877) traces a dynamic-looking
// store source/destination through tensor.extract_slice / memref.subview to the
// underlying type before deciding the store is genuinely dynamic.  A store
// whose dynamic-looking operand is a slice of a (possibly static) value is
// therefore NOT an abort: the pass proceeds to tile it.  This helper recognizes
// that slice-wrapped form so the model can avoid the unsound Exact
// dynamic-store rollback for it.
inline bool IsSliceWrappedSource(const GenericModule &module, int value) {
  const GenericOperation *def = Definition(module, value);
  return def != nullptr && (def->name == "tensor.extract_slice" ||
                            def->name == "memref.subview");
}

enum class DynamicStoreDisposition {
  // No dynamic store: the real pass proceeds to tile -> model reports Incomplete
  // because the exact sub-block loop IR is not reproduced.
  kNone,
  // A non-slice-wrapped dynamic store: the real pass aborts tiling and reverts
  // to limitUniqueSubBlockToStore -> model reproduces that as Exact.
  kGenuineDynamic,
  // A dynamic-looking store whose source/destination is a slice op: the real
  // pass traces through the slice and proceeds to tile -> the model cannot
  // reproduce the trace-and-tile, so it fails closed (Incomplete) rather than
  // claiming the Exact dynamic-store rollback.
  kSliceWrapped,
};

// Mirrors the dynamic-store guard in tileAndSliceOp at function granularity.
// tileAndSliceOp aborts (and reverts to limitUniqueSubBlockToStore) as soon as
// ANY store is genuinely dynamic.  A dynamic-looking but slice-wrapped store is
// not an abort in the real pass, so it is reported separately (kSliceWrapped)
// for fail-closed handling instead of the Exact rollback.
inline DynamicStoreDisposition
ClassifyDynamicStores(const GenericModule &module, int functionId) {
  bool anySliceWrapped = false;
  for (int opId : AllOperations(module, functionId)) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(opId));
    if (operation.name != "hivm.hir.store" || !IsDynamicStore(operation))
      continue;
    const bool srcSlice =
        operation.operands.size() >= 1 &&
        IsSliceWrappedSource(module, operation.operands.front());
    const bool dstSlice =
        operation.operands.size() >= 2 &&
        IsSliceWrappedSource(module, operation.operands[1]);
    if (srcSlice || dstSlice) {
      // Conservatively fail closed: the real pass would trace the slice before
      // deciding, which the model does not reproduce, so never claim Exact.
      anySliceWrapped = true;
      continue;
    }
    return DynamicStoreDisposition::kGenuineDynamic;
  }
  return anySliceWrapped ? DynamicStoreDisposition::kSliceWrapped
                         : DynamicStoreDisposition::kNone;
}

// Exact UB-semantic subset of successful sub-block tiling.  The real pass
// oracle for this topology produces an axis-0 two-way tile: tensor temporaries
// become half-sized while the destination allocation remains full-sized and is
// addressed through a subview.  Keeping the full destination value here is UB
// equivalent because the suffix sees the same allocation and alias root; only
// the tensor temporaries need their physical types reduced.
inline bool ApplySimpleVectorStoreUBTiling(GenericModule &module,
                                           int functionId) {
  using tile_cube_vector_loop_detail::ParseStaticShapedType;
  using tile_cube_vector_loop_detail::PrintShapedType;
  using tile_cube_vector_loop_detail::ShapedType;

  const GenericOperation &function =
      module.operations.at(static_cast<size_t>(functionId));
  const GenericOperation *load = nullptr;
  const GenericOperation *add = nullptr;
  std::vector<const GenericOperation *> empties;
  GenericOperation *store = nullptr;
  size_t returnCount = 0;
  for (int opId : AllOperations(module, functionId)) {
    GenericOperation &operation = module.operations.at(static_cast<size_t>(opId));
    if (operation.name == "hivm.hir.load") {
      if (load != nullptr)
        return false;
      load = &operation;
    } else if (operation.name == "hivm.hir.vadd") {
      if (add != nullptr)
        return false;
      add = &operation;
    } else if (operation.name == "hivm.hir.store") {
      if (store != nullptr)
        return false;
      store = &operation;
    } else if (operation.name == "tensor.empty") {
      empties.push_back(&operation);
      if (empties.size() > 2)
        return false;
    } else if (operation.name == "func.return") {
      ++returnCount;
      if (!operation.operands.empty() || !operation.results.empty())
        return false;
    } else if (operation.name != "func.func") {
      return false;
    }
  }
  if (load == nullptr || add == nullptr || store == nullptr ||
      empties.size() != 2 || returnCount != 1 ||
      load->operands.size() != 2 || add->operands.size() != 3)
    return false;
  const GenericOperation *loadEmpty = Definition(module, load->operands[1]);
  const GenericOperation *addEmpty = Definition(module, add->operands[2]);
  if (loadEmpty == nullptr || addEmpty == nullptr || loadEmpty == addEmpty ||
      loadEmpty->name != "tensor.empty" || addEmpty->name != "tensor.empty" ||
      loadEmpty->results.size() != 1 || addEmpty->results.size() != 1 ||
      !loadEmpty->operands.empty() || !addEmpty->operands.empty() ||
      load->results.size() != 1 || add->results.size() != 1 ||
      load->operandTypes.size() != 2 ||
      add->operands.size() != 3 || add->operandTypes.size() != 3 ||
      store->operands.size() != 2 || store->operandTypes.size() != 2 ||
      store->operands[0] != add->results.front() ||
      add->operands[0] != load->results.front() ||
      add->operands[1] != load->results.front() ||
      load->operands[1] != loadEmpty->results.front() ||
      add->operands[2] != addEmpty->results.front())
    return false;

  // Prove the complete value topology before mutating anything.  In
  // particular, neither temporary may have an additional user whose type or
  // lifetime the narrow rewrite below would silently change.
  const int sourceValue = load->operands[0];
  const int destinationValue = store->operands[1];
  if (!IsFunctionBlockArgument(module, function, sourceValue) ||
      !IsFunctionBlockArgument(module, function, destinationValue) ||
      Definition(module, sourceValue) != nullptr ||
      Definition(module, destinationValue) != nullptr)
    return false;
  const auto useCount = [&](int value) {
    size_t count = 0;
    for (int opId : AllOperations(module, functionId))
      for (int operand :
           module.operations.at(static_cast<size_t>(opId)).operands)
        if (operand == value)
          ++count;
    return count;
  };
  if (useCount(sourceValue) != 1 ||
      useCount(loadEmpty->results.front()) != 1 ||
      useCount(load->results.front()) != 2 ||
      useCount(addEmpty->results.front()) != 1 ||
      useCount(add->results.front()) != 1 || useCount(destinationValue) != 1)
    return false;

  const int storeId = store->id;

  const auto source = ParseStaticShapedType(store->operandTypes.front());
  const auto destination = ParseStaticShapedType(store->operandTypes[1]);
  if (!source || source->kind != "tensor" || source->shape.size() != 2 ||
      source->shape.front() <= 0 || source->shape.front() % 2 != 0 ||
      !destination || destination->kind != "memref" ||
      destination->shape != source->shape ||
      tile_cube_vector_loop_detail::ElementBitWidth(destination->tail) !=
          tile_cube_vector_loop_detail::ElementBitWidth(source->tail) ||
      store->operandTypes[1].find("#hivm.address_space<gm>") ==
          std::string::npos)
    return false;
  ShapedType tiled = *source;
  tiled.shape.front() /= 2;
  const std::string tiledType = PrintShapedType(tiled);
  const int loadEmptyId = loadEmpty->id;
  const int addEmptyId = addEmpty->id;
  const int addId = add->id;

  const std::string fullTensorType = PrintShapedType(*source);
  const auto allTypesAreFull = [&](const GenericOperation &operation) {
    for (const std::string &type : operation.operandTypes) {
      const auto shaped = ParseStaticShapedType(type);
      if (shaped && shaped->kind == "tensor" && type != fullTensorType)
        return false;
    }
    for (const std::string &type : operation.resultTypes) {
      const auto shaped = ParseStaticShapedType(type);
      if (shaped && shaped->kind == "tensor" && type != fullTensorType)
        return false;
    }
    return true;
  };
  if (!allTypesAreFull(*loadEmpty) || !allTypesAreFull(*load) ||
      !allTypesAreFull(*addEmpty) || !allTypesAreFull(*add) ||
      !allTypesAreFull(*store) || load->operandTypes[0] != fullTensorType)
    return false;

  // The real bubble-up tiler slices an external tensor source before the load.
  // Preserve that alias boundary instead of changing the function block
  // argument type (which would be invalid and would also misrepresent the
  // externally owned buffer).
  const int loadId = load->id;
  const GenericOperation loadCopy =
      module.operations.at(static_cast<size_t>(loadId));
  GenericRewriter rewriter(module);
  const int externalSliceId = rewriter.createOperation(
          loadCopy.parentId, loadCopy.regionId, loadCopy.blockId,
          "tensor.extract_slice", {tiledType}, {sourceValue},
          {fullTensorType},
          "operandSegmentSizes = array<i32: 1, 0, 0, 0>, "
          "static_offsets = array<i64: 0, 0>, "
          "static_sizes = array<i64: " + std::to_string(tiled.shape[0]) +
              ", " + std::to_string(tiled.shape[1]) +
              ">, static_strides = array<i64: 1, 1>");
  const GenericBlock &block =
      module.blocks.at(static_cast<size_t>(loadCopy.blockId));
  const auto position =
      std::find(block.operations.begin(), block.operations.end(), loadId);
  rewriter.insertToBlock(
          loadCopy.blockId,
          static_cast<size_t>(std::distance(block.operations.begin(), position)),
          externalSliceId);
  GenericOperation &mutableLoad =
      module.operations.at(static_cast<size_t>(loadId));
  const int slicedValue = module.operations.at(
          static_cast<size_t>(externalSliceId)).results.front();
  mutableLoad.operands.front() = slicedValue;
  RemapValues(mutableLoad.dpsInputs, {{sourceValue, slicedValue}});
  RemapValues(mutableLoad.dpsInits, {{sourceValue, slicedValue}});

  // Only the proven load/add/store closure is tiled.  The function arguments
  // and unrelated values (there cannot be any in this matcher) retain their
  // external full-shape types.
  for (int opId : {loadEmptyId, loadId, addEmptyId, addId}) {
    GenericOperation &operation = module.operations.at(static_cast<size_t>(opId));
    for (std::string &type : operation.operandTypes) {
      const auto shaped = ParseStaticShapedType(type);
      if (shaped && shaped->kind == "tensor")
        type = tiledType;
    }
    for (std::string &type : operation.resultTypes) {
      const auto shaped = ParseStaticShapedType(type);
      if (shaped && shaped->kind == "tensor")
        type = tiledType;
    }
  }
  GenericOperation &tiledStore =
      module.operations.at(static_cast<size_t>(storeId));
  tiledStore.operandTypes[0] = tiledType;
  if (tiledStore.attributes == "{}" || tiledStore.attributes.empty())
    tiledStore.attributes = "{tiled_op}";
  else
    tiledStore.attributes.insert(tiledStore.attributes.size() - 1,
                                 ", tiled_op");
  return true;
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
//        - genuinely dynamic store (a source/destination that is dynamic and
//          NOT slice-wrapped) -> tileAndSliceOp abort (recognized, Exact) with
//          limitUniqueSubBlockToStore.
//        - dynamic-looking but slice-wrapped store (operand is a
//          tensor.extract_slice / memref.subview) -> the real pass traces to
//          the underlying type and tiles; that trace-and-tile is not reproduced
//          here, so the form is reported incomplete (fail closed).
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

  if (const GenericOperation *allocation =
          ApplicableAllocToTensorEarlyPattern(result.module)) {
    AddDiagnostic(result, allocation->id, allocation->name,
                  "alloc-to-tensor early pattern would change UB ownership but "
                  "is not modeled");
    return result;
  }

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
  // single AIV function.  NOTE: the real pass's tileAivFuncs bails out of the
  // whole AIV step on the FIRST function whose attemptBindSubBlock fails (it
  // does not continue to later functions); this model instead classifies each
  // function independently.  That is a deliberate model-added narrowing that
  // stays fail-closed: every path that the real pass would actively tile is
  // reported Incomplete here, so the model never silently produces an exact
  // untiled result for a function the real pass transformed.
  for (int functionId : aivFuncs) {
    if (!HasAnyStoreLikeOp(result.module, functionId))
      continue; // isFailed revert -> limitUniqueSubBlockToStore no-op, Exact.
    const DynamicStoreDisposition disposition =
        ClassifyDynamicStores(result.module, functionId);
    if (disposition == DynamicStoreDisposition::kGenuineDynamic) {
      // tileAndSliceOp aborts on a non-slice-wrapped dynamic store; the real
      // pass reverts and applies limitUniqueSubBlockToStore.  Recognized
      // rollback, Exact.
      if (!LimitStoresToSubBlockZero(result.module, {functionId}))
        AddDiagnostic(result, functionId, "func.func", resultBearerReason);
      continue;
    }
    if (disposition == DynamicStoreDisposition::kNone &&
        ApplySimpleVectorStoreUBTiling(result.module, functionId))
      continue;
    // kNone (other static store) or kSliceWrapped (the real pass traces the slice and
    // then tiles): the exact sub-block loop IR is not reproduced.  Fail closed:
    // keep the legal IR and report the function as an incomplete (unmodeled
    // potentially-successful) form.
    AddDiagnostic(
        result, functionId, "func.func",
        disposition == DynamicStoreDisposition::kSliceWrapped
            ? "AIV sub-block tiling would trace the slice-wrapped dynamic "
              "store and transform this function, but the exact sub-block loop "
              "IR is not modeled"
            : "AIV sub-block tiling would transform this function but the "
              "exact sub-block loop IR is not modeled");
  }

  ValidateGenericModule(result.module);
  return result;
}

} // namespace cvub

#endif
