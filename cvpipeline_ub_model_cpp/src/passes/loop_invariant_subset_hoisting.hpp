#ifndef CVPIPELINE_UB_MODEL_CPP_PASSES_LOOP_INVARIANT_SUBSET_HOISTING_HPP
#define CVPIPELINE_UB_MODEL_CPP_PASSES_LOOP_INVARIANT_SUBSET_HOISTING_HPP

#include "../ir/post_pipeline_ir_utils.hpp"
#include "tile_cube_vector_loop.hpp"
#include "../pipeline/modeling_result.hpp"

#include <limits>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace cvub {
namespace code_motion_detail {

// Modeled loop-like operations.  Upstream createLoopInvariantCodeMotionPass and
// createLoopInvariantSubsetHoistingPass both walk LoopLikeOpInterface
// (registered at bishengir/lib/Dialect/HIVM/Pipelines/HIVMPipelines.cpp:283-284,
// gated by enableCodeMotion).  This model reproduces the common scf.for /
// scf.forall single-block bodies; other loop kinds fail closed.
inline bool IsModeledLoop(const std::string &name) {
  return name == "scf.for" || name == "scf.forall";
}

// Loop-like operations the real passes also walk but whose invariant code motion
// this model does not reproduce.  Their presence forces a fail-closed report so
// the model never silently drops a lifetime-affecting hoist.
inline bool IsUnmodeledLoop(const std::string &name) {
  return name == "scf.while" || name == "affine.for" ||
         name == "affine.parallel" || name == "scf.parallel";
}

inline bool IsTerminator(const std::string &name) {
  return name == "scf.yield" || name == "func.return" ||
         name == "cf.br" || name == "cf.cond_br" || name == "scf.condition" ||
         name == "scope.return" || name == "scope.yield";
}

// hivm.hir.store / indirect_store / scatter_store and scalar memref.store are
// pure writes: the real LICM never hoists them out of a loop (hoisting a store
// past a possibly-zero-trip loop would change semantics), so a loop-invariant
// store is left in place and the model reports Exact.
inline bool IsWriteOnlyStore(const std::string &name) {
  return name == "hivm.hir.store" || name == "hivm.hir.indirect_store" ||
         name == "hivm.hir.scatter_store" || name == "memref.store";
}

// Operations that carry inherent DMA / allocate / sync / annotation effects.
// When such an op is loop-invariant the model cannot prove the real pass leaves
// it in place (the real compiler's advanced memory analysis, or a later pass,
// may hoist invariant loads/copies and change lifetimes), so the model fails
// closed instead of claiming an exact (and possibly wrong-lifetime) result.
inline bool IsInherentMemoryOp(const std::string &name) {
  static const std::set<std::string> operations = {
      "annotation.mark",       "bufferization.alloc_tensor",
      "bufferization.to_tensor", "hivm.hir.atomic_cas",
      "hivm.hir.atomic_rmw",   "hivm.hir.atomic_xchg",
      "hivm.hir.copy",         "hivm.hir.debug",
      "hivm.hir.fixpipe",      "hivm.hir.gather_load",
      "hivm.hir.load",         "hivm.hir.nd2nz",
      "hivm.hir.nz2nd",        "hivm.hir.pipe_barrier",
      "hivm.hir.pointer_cast", "hivm.hir.set_mask_norm",
      "hivm.hir.sync_block",   "llvm.inline_asm",
      "memref.alloc",          "memref.alloca",
      "memref.load",           "memref_ext.alloc_workspace"};
  return operations.count(name) != 0;
}

// The full set of arith ops that are ConditionallySpeculatable in upstream MLIR
// (i.e. can be NotSpeculatable).  Verified against ArithOps.td/.cpp: exactly
// arith.divui, arith.divsi, arith.ceildivui, arith.ceildivsi, arith.remui,
// arith.remsi declare [ConditionallySpeculatable] and a getSpeculatability()
// override.  All other arith.* ops are always speculatable.
inline bool IsNonSpeculatableArith(const std::string &name) {
  return name == "arith.divui" || name == "arith.divsi" ||
         name == "arith.ceildivui" || name == "arith.ceildivsi" ||
         name == "arith.remui" || name == "arith.remsi";
}

inline bool IsPureHoistableName(const std::string &name) {
  if (startsWith(name, "arith.")) {
    // Integer division/remainder ops are ConditionallySpeculatable in upstream
    // MLIR (ArithOps.cpp/.td) and return NotSpeculatable unless the divisor is
    // provably non-zero (and, for signed division, not -1).  Upstream LICM's
    // hoist predicate is isMemoryEffectFree(op) && isSpeculatable(op), so a
    // loop-invariant such op with an unprovable divisor is LEFT IN THE LOOP.
    // The model has no integer-range analysis, so it conservatively treats these
    // as non-hoistable by name; the AnalyzeLoop classifier then routes a loop
    // carrying a loop-invariant instance to Incomplete (fail-closed-safe).
    // arith.floordivsi is NOT here: it is an Arith_TotalIntBinaryOp (no
    // ConditionallySpeculatable trait, no getSpeculatability override) and is
    // therefore always speculatable, so upstream LICM does hoist it.
    if (IsNonSpeculatableArith(name))
      return false;
    return true;
  }
  static const std::set<std::string> operations = {
      "affine.apply", "affine.max", "affine.min",
      "memref.cast",  "memref.collapse_shape", "memref.expand_shape",
      "memref.reinterpret_cast", "memref.subview",
      "tensor.cast",  "tensor.collapse_shape", "tensor.expand_shape",
      "tensor.extract", "tensor.extract_slice", "tensor.from_elements",
      "tensor.insert", "tensor.insert_slice",  "tensor.empty"};
  if (operations.count(name) != 0)
    return true;
  // HIVM structured elementwise ops are pure when they operate on tensors (no
  // memref operands -> empty effects).  The caller re-checks the effects field.
  return IsHIVMStructuredOp(name);
}

// An op is reproducibly hoistable by upstream LICM when it is pure (no memory
// effects), speculatively executable, not a terminator, and carries no nested
// region.  This positive classification is the basis for the Exact hoist; any
// op that does not qualify and is still loop-invariant triggers a fail-close.
inline bool IsPureHoistable(const GenericOperation &operation) {
  if (!operation.regions.empty())
    return false;
  if (IsTerminator(operation.name))
    return false;
  if (IsWriteOnlyStore(operation.name))
    return false;
  if (IsInherentMemoryOp(operation.name))
    return false;
  if (!IsPureHoistableName(operation.name))
    return false;
  if (IsHIVMStructuredOp(operation.name)) {
    // HIVM structured elementwise ops are pure only on tensor operands: a memref
    // operand carries a read/write effect.  Inspect operand types directly so
    // the classification stays correct when memory effects are not pre-computed
    // (the test fixtures parse with semantics application disabled).
    for (const std::string &type : operation.operandTypes)
      if (startsWith(trim(type), "memref<"))
        return false;
    if (!operation.effects.empty() && operation.effects != "none")
      return false;
  }
  return true;
}

inline bool IsNestedUnder(const GenericModule &module, int operationId,
                          int ancestorId) {
  int current = operationId;
  std::set<int> visited;
  while (current >= 0 && visited.insert(current).second) {
    if (current == ancestorId)
      return true;
    current =
        module.operations.at(static_cast<size_t>(current)).parentId;
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

// True when `value` is defined outside the loop, or by an op already selected
// for hoisting.  Mirrors MLIR's isDefinedOutsideOfLoop plus the fixed-point
// "already hoisted" allowance.
inline bool DefinedOutsideOrHoisted(const GenericModule &module, int value,
                                    int loopId,
                                    const std::set<int> &hoisted) {
  const GenericOperation *definition = Definition(module, value);
  if (definition != nullptr)
    return hoisted.count(definition->id) != 0 ||
           !IsNestedUnder(module, definition->id, loopId);
  for (const GenericBlock &block : module.blocks)
    if (std::find(block.arguments.begin(), block.arguments.end(), value) !=
        block.arguments.end()) {
      const GenericRegion &region =
          module.regions.at(static_cast<size_t>(block.regionId));
      return !IsNestedUnder(module, region.parentOperation, loopId);
    }
  return false;
}

inline int LoopBodyBlock(const GenericModule &module,
                         const GenericOperation &loop) {
  if (loop.regions.size() != 1)
    return -1;
  const GenericRegion &region =
      module.regions.at(static_cast<size_t>(loop.regions.front()));
  if (region.blocks.size() != 1)
    return -1;
  return region.blocks.front();
}

struct LoopHoistPlan {
  bool classifiable = true;
  std::vector<int> hoistedOps;
  std::string reason;
};

// Analyze one loop: compute the fixed-point set of pure ops to hoist, and
// decide whether the remaining body is fully classifiable.  Returns
// plan.classifiable == false (with a reason) when the model cannot reproduce
// the real pass's effect on this loop, so the caller can fail closed and leave
// the legal IR untouched.
inline LoopHoistPlan AnalyzeLoop(const GenericModule &module, int loopId) {
  LoopHoistPlan plan;
  const GenericOperation &loop =
      module.operations.at(static_cast<size_t>(loopId));
  const int bodyBlock = LoopBodyBlock(module, loop);
  if (bodyBlock < 0) {
    plan.classifiable = false;
    plan.reason = "loop body shape is not modeled";
    return plan;
  }
  const GenericBlock &body = module.blocks.at(static_cast<size_t>(bodyBlock));

  std::set<int> hoisted;
  bool changed = true;
  while (changed) {
    changed = false;
    for (int operationId : body.operations) {
      if (hoisted.count(operationId) != 0)
        continue;
      const GenericOperation &operation =
          module.operations.at(static_cast<size_t>(operationId));
      if (IsTerminator(operation.name))
        continue;
      if (!IsPureHoistable(operation))
        continue;
      bool allOutside = true;
      for (int operand : operation.operands)
        if (!DefinedOutsideOrHoisted(module, operand, loopId, hoisted)) {
          allOutside = false;
          break;
        }
      if (!allOutside)
        continue;
      hoisted.insert(operationId);
      plan.hoistedOps.push_back(operationId);
      changed = true;
    }
  }

  for (int operationId : body.operations) {
    if (hoisted.count(operationId) != 0)
      continue;
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    if (IsTerminator(operation.name))
      continue;
    if (!operation.regions.empty()) {
      plan.classifiable = false;
      plan.reason = "loop body contains nested region-bearing operation " +
                    operation.name +
                    " whose invariant code motion is not modeled";
      return plan;
    }
    bool invariant = true;
    for (int operand : operation.operands)
      if (!DefinedOutsideOrHoisted(module, operand, loopId, hoisted)) {
        invariant = false;
        break;
      }
    if (!invariant)
      continue; // loop-variant op stays in place; real pass leaves it too.
    if (IsWriteOnlyStore(operation.name))
      continue; // loop-invariant write stays; real LICM never hoists stores.
    if (IsNonSpeculatableArith(operation.name)) {
      // A loop-invariant integer division/remainder op is NotSpeculatable when
      // its divisor is not provably non-zero, so upstream LICM leaves it in the
      // loop.  The model cannot reproduce the integer-range analysis, so it
      // fails closed rather than claiming an exact (wrong) hoist.
      plan.classifiable = false;
      plan.reason = "loop-invariant " + operation.name +
                    " is not unconditionally speculatable; the model cannot "
                    "reproduce upstream LICM's divisor speculatability analysis";
      return plan;
    }
    plan.classifiable = false;
    plan.reason = "loop-invariant " + operation.name +
                  " has memory effects the model cannot hoist exactly";
    return plan;
  }
  return plan;
}

inline std::vector<int> AllNestedOperations(const GenericModule &module,
                                            int rootId) {
  std::vector<int> result;
  std::vector<int> worklist{rootId};
  std::set<int> visited;
  while (!worklist.empty()) {
    const int operationId = worklist.back();
    worklist.pop_back();
    if (!visited.insert(operationId).second)
      continue;
    result.push_back(operationId);
    for (int regionId :
         module.operations.at(static_cast<size_t>(operationId)).regions)
      for (int blockId :
           module.regions.at(static_cast<size_t>(regionId)).blocks)
        for (int child :
             module.blocks.at(static_cast<size_t>(blockId)).operations)
          worklist.push_back(child);
  }
  return result;
}

inline bool IsSubsetSliceOp(const std::string &name) {
  return name == "tensor.extract_slice" || name == "tensor.insert_slice" ||
         name == "vector.transfer_read" || name == "vector.transfer_write";
}

inline bool LoopCarriesSubsetPattern(const GenericModule &module, int loopId) {
  const GenericOperation &loop =
      module.operations.at(static_cast<size_t>(loopId));
  const int bodyBlock = LoopBodyBlock(module, loop);
  if (bodyBlock < 0)
    return false;
  const GenericBlock &body = module.blocks.at(static_cast<size_t>(bodyBlock));
  if (body.arguments.size() <= 1)
    return false; // no loop-carried iter args (arg 0 is the induction var).
  const std::set<int> iterArgs(body.arguments.begin() + 1,
                               body.arguments.end());
  for (int operationId : AllNestedOperations(module, loopId)) {
    if (operationId == loopId)
      continue;
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    if (!IsSubsetSliceOp(operation.name))
      continue;
    const auto hits = [&](const std::vector<int> &values) {
      return std::any_of(values.begin(), values.end(),
                         [&](int value) { return iterArgs.count(value) != 0; });
    };
    if (hits(operation.operands) || hits(operation.dpsInputs) ||
        hits(operation.dpsInits))
      return true;
  }
  return false;
}

struct SubsetHoistPlan {
  bool candidate = false;
  bool exact = false;
  int loopId = -1;
  int bodyBlock = -1;
  int extractId = -1;
  int insertId = -1;
  int yieldId = -1;
  std::string reason;
};

// Recognize the static tensor.extract_slice/tensor.insert_slice form emitted by
// the CV pipeline corpus.  Requiring the complete three-op body is deliberate:
// it makes the rewrite below a proof of the UB-relevant loop-carried values,
// rather than an approximation of the much broader upstream interface pass.
inline SubsetHoistPlan AnalyzeStaticSubsetHoist(const GenericModule &module,
                                                int loopId) {
  SubsetHoistPlan plan;
  plan.loopId = loopId;
  if (!LoopCarriesSubsetPattern(module, loopId))
    return plan;
  plan.candidate = true;

  const GenericOperation &loop =
      module.operations.at(static_cast<size_t>(loopId));
  plan.bodyBlock = LoopBodyBlock(module, loop);
  if (loop.name != "scf.for" || plan.bodyBlock < 0 ||
      loop.operands.size() != 4 || loop.operandTypes.size() != 4 ||
      loop.results.size() != 1 || loop.resultTypes.size() != 1) {
    plan.reason = "subset-carrying loop is outside the modeled single-iter-arg "
                  "scf.for form";
    return plan;
  }
  const GenericBlock &body =
      module.blocks.at(static_cast<size_t>(plan.bodyBlock));
  if (body.arguments.size() != 2 || body.argumentTypes.size() != 2 ||
      body.operations.size() != 3) {
    plan.reason = "subset-carrying loop body is outside the modeled static "
                  "extract/insert/yield form";
    return plan;
  }

  const GenericOperation &extract =
      module.operations.at(static_cast<size_t>(body.operations[0]));
  const GenericOperation &insert =
      module.operations.at(static_cast<size_t>(body.operations[1]));
  const GenericOperation &yield =
      module.operations.at(static_cast<size_t>(body.operations[2]));
  using tile_cube_vector_loop_detail::ParseIntegerArray;
  using tile_cube_vector_loop_detail::ParseStaticShapedType;
  const auto propertyArray = [&](const GenericOperation &operation,
                                 const char *key) {
    return ParseIntegerArray(IRDictionaryValue(operation.properties, key));
  };
  const std::vector<int64_t> extractOffsets =
      propertyArray(extract, "static_offsets");
  const std::vector<int64_t> extractSizes =
      propertyArray(extract, "static_sizes");
  const std::vector<int64_t> extractStrides =
      propertyArray(extract, "static_strides");
  const std::vector<int64_t> insertOffsets =
      propertyArray(insert, "static_offsets");
  const std::vector<int64_t> insertSizes =
      propertyArray(insert, "static_sizes");
  const std::vector<int64_t> insertStrides =
      propertyArray(insert, "static_strides");
  const auto fullType = ParseStaticShapedType(body.argumentTypes[1]);
  const auto subsetType = extract.resultTypes.empty()
                              ? std::nullopt
                              : ParseStaticShapedType(extract.resultTypes[0]);
  const int64_t dynamicSentinel = std::numeric_limits<int64_t>::min();
  const auto allStatic = [&](const std::vector<int64_t> &values) {
    return std::none_of(values.begin(), values.end(),
                        [&](int64_t value) { return value == dynamicSentinel; });
  };
  const bool matchingStaticProperties =
      fullType && subsetType && !extractOffsets.empty() &&
      extractOffsets == insertOffsets && extractSizes == insertSizes &&
      extractStrides == insertStrides &&
      extractOffsets.size() == fullType->shape.size() &&
      extractSizes.size() == fullType->shape.size() &&
      extractStrides.size() == fullType->shape.size() &&
      subsetType->shape.size() == fullType->shape.size() &&
      allStatic(extractOffsets) && allStatic(extractSizes) &&
      allStatic(extractStrides) &&
      std::all_of(extractSizes.begin(), extractSizes.end(),
                  [](int64_t value) { return value > 0; }) &&
      std::all_of(extractStrides.begin(), extractStrides.end(),
                  [](int64_t value) { return value > 0; });
  const std::vector<int64_t> extractSegments =
      propertyArray(extract, "operandSegmentSizes");
  const std::vector<int64_t> insertSegments =
      propertyArray(insert, "operandSegmentSizes");
  const bool noDynamicOperands =
      extractSegments == std::vector<int64_t>({1, 0, 0, 0}) &&
      insertSegments == std::vector<int64_t>({1, 1, 0, 0, 0});
  if (extract.name != "tensor.extract_slice" ||
      extract.operands != std::vector<int>{body.arguments[1]} ||
      extract.operandTypes.size() != 1 || extract.results.size() != 1 ||
      extract.resultTypes.size() != 1 || !extract.regions.empty() ||
      insert.name != "tensor.insert_slice" ||
      insert.operands !=
          std::vector<int>{extract.results[0], body.arguments[1]} ||
      insert.operandTypes.size() != 2 || insert.results.size() != 1 ||
      insert.resultTypes != loop.resultTypes || !insert.regions.empty() ||
      yield.name != "scf.yield" ||
      yield.operands != std::vector<int>{insert.results[0]} ||
      yield.operandTypes != loop.resultTypes || !matchingStaticProperties ||
      !noDynamicOperands) {
    plan.reason = "subset-carrying loop does not match the proven static "
                  "extract/insert pair";
    return plan;
  }
  if (loop.operandTypes[3] != body.argumentTypes[1] ||
      loop.resultTypes[0] != body.argumentTypes[1] ||
      extract.operandTypes[0] != body.argumentTypes[1] ||
      insert.operandTypes[1] != body.argumentTypes[1] ||
      insert.operandTypes[0] != extract.resultTypes[0]) {
    plan.reason = "subset-carrying loop has inconsistent tensor types";
    return plan;
  }

  plan.extractId = extract.id;
  plan.insertId = insert.id;
  plan.yieldId = yield.id;
  plan.exact = true;
  return plan;
}

inline void ReplaceUsesExcept(GenericModule &module, int oldValue, int newValue,
                              int excludedOperation) {
  const std::map<int, int> replacement = {{oldValue, newValue}};
  for (GenericOperation &operation : module.operations) {
    if (operation.id == excludedOperation)
      continue;
    RemapValues(operation.operands, replacement);
    RemapValues(operation.dpsInputs, replacement);
    RemapValues(operation.dpsInits, replacement);
  }
}

inline void ApplyStaticSubsetHoist(GenericModule &module,
                                   const SubsetHoistPlan &plan) {
  GenericRewriter rewriter(module);
  const int subsetBlockArgument = rewriter.newValue();
  const int subsetLoopResult = rewriter.newValue();

  GenericOperation &loop =
      module.operations.at(static_cast<size_t>(plan.loopId));
  GenericBlock &body = module.blocks.at(static_cast<size_t>(plan.bodyBlock));
  GenericOperation &extract =
      module.operations.at(static_cast<size_t>(plan.extractId));
  GenericOperation &insert =
      module.operations.at(static_cast<size_t>(plan.insertId));
  GenericOperation &yield =
      module.operations.at(static_cast<size_t>(plan.yieldId));
  const int fullLoopResult = loop.results.front();
  const int fullLoopInit = loop.operands[3];
  const std::string subsetType = extract.resultTypes.front();

  extract.operands = {fullLoopInit};
  extract.operandTypes = {loop.operandTypes[3]};
  ApplyOperationSemantics(extract);

  loop.operands.push_back(extract.results.front());
  loop.operandTypes.push_back(subsetType);
  loop.results.push_back(subsetLoopResult);
  loop.resultTypes.push_back(subsetType);
  body.arguments.push_back(subsetBlockArgument);
  body.argumentTypes.push_back(subsetType);

  yield.operands = {body.arguments[1], subsetBlockArgument};
  yield.operandTypes = {body.argumentTypes[1], subsetType};
  ApplyOperationSemantics(yield);

  insert.operands = {subsetLoopResult, fullLoopResult};
  insert.operandTypes = {subsetType, loop.resultTypes.front()};
  ApplyOperationSemantics(insert);
  ReplaceUsesExcept(module, fullLoopResult, insert.results.front(), insert.id);

  MoveOperationBefore(module, extract.id, loop.id);
  rewriter.removeFromBlock(plan.bodyBlock, insert.id);
  const GenericBlock &outer =
      module.blocks.at(static_cast<size_t>(loop.blockId));
  const auto loopPosition =
      std::find(outer.operations.begin(), outer.operations.end(), loop.id);
  if (loopPosition == outer.operations.end())
    throw std::runtime_error("subset hoisting: loop is not attached");
  rewriter.insertToBlock(
      loop.blockId,
      static_cast<size_t>(std::distance(outer.operations.begin(), loopPosition)) +
          1,
      insert.id);
}

inline void AddDiagnostic(StageResult &result, int operationId,
                          const std::string &operation,
                          const std::string &stage, const std::string &reason) {
  result.precision = Precision::Incomplete;
  result.diagnostics.push_back({stage, "", operationId, operation, reason});
}

} // namespace code_motion_detail

// Models upstream createLoopInvariantCodeMotionPass on a projected AIV module.
//
// Hoist side-effect-free ops whose operands are all defined outside the loop
// (or by an already-hoisted op) to just before the loop, iterating to a fixed
// point -- the gen-kill interval of the hoisted value moves from
// live-across-the-loop to defined-before-the-loop, which is the UB-relevant
// effect mirrored here.  enableCodeMotion=false gates the pass to a recognized
// no-op.  Loop-variant ops and loop-invariant pure-write stores stay in place
// (Exact).  Any loop-invariant memory-effecting op the model cannot prove the
// real pass leaves untouched, plus nested region-bearing bodies and unmodeled
// loop kinds, fail closed to Incomplete with the legal IR left untouched.
inline StageResult RunStrictLoopInvariantCodeMotion(GenericModule module,
                                              bool enableCodeMotion) {
  using namespace code_motion_detail;
  StageResult result;
  result.module = std::move(module);
  ValidateGenericModule(result.module);
  if (!enableCodeMotion)
    return result; // Recognized no-op, Exact.

  for (const GenericOperation &operation : result.module.operations) {
    if (operation.blockId < 0)
      continue;
    if (IsUnmodeledLoop(operation.name))
      AddDiagnostic(result, operation.id, operation.name,
                    "LoopInvariantCodeMotion",
                    "loop-like operation is not modeled by the code-motion model");
  }

  struct Pending {
    int loopId;
    LoopHoistPlan plan;
  };
  std::vector<Pending> pending;
  for (const GenericOperation &operation : result.module.operations) {
    if (operation.blockId < 0 || !IsModeledLoop(operation.name))
      continue;
    pending.push_back({operation.id, AnalyzeLoop(result.module, operation.id)});
  }

  for (const Pending &entry : pending) {
    if (entry.plan.classifiable)
      continue;
    AddDiagnostic(result, entry.loopId,
                  result.module.operations.at(static_cast<size_t>(entry.loopId))
                      .name,
                  "LoopInvariantCodeMotion", entry.plan.reason);
  }

  if (result.precision == Precision::Incomplete) {
    ValidateGenericModule(result.module);
    return result; // Fail closed: legal IR untouched.
  }

  for (const Pending &entry : pending)
    for (int operationId : entry.plan.hoistedOps)
      MoveOperationBefore(result.module, operationId, entry.loopId);

  ValidateGenericModule(result.module);
  return result;
}

// Models upstream createLoopInvariantSubsetHoistingPass on a projected AIV
// module.
//
// The proven static single-iter-arg form is rebuilt at the UB-semantic level:
// extraction before the loop, an extra carried subset arg/result, and insertion
// after the loop.  All other applicable forms fail closed before any mutation.
// enableCodeMotion=false gates the pass to a recognized no-op.
inline StageResult RunLoopInvariantSubsetHoisting(GenericModule module,
                                                  bool enableCodeMotion) {
  using namespace code_motion_detail;
  StageResult result;
  result.module = std::move(module);
  ValidateGenericModule(result.module);
  if (!enableCodeMotion)
    return result; // Recognized no-op, Exact.

  std::vector<SubsetHoistPlan> plans;
  for (const GenericOperation &operation : result.module.operations) {
    if (operation.blockId < 0 || !IsModeledLoop(operation.name))
      continue;
    SubsetHoistPlan plan =
        AnalyzeStaticSubsetHoist(result.module, operation.id);
    if (!plan.candidate)
      continue;
    if (!plan.exact)
      AddDiagnostic(result, operation.id, operation.name,
                    "LoopInvariantSubsetHoisting", plan.reason);
    plans.push_back(std::move(plan));
  }

  if (result.precision == Precision::Incomplete) {
    ValidateGenericModule(result.module);
    return result; // Transactional fail-close: no partially rebuilt loops.
  }
  for (const SubsetHoistPlan &plan : plans)
    ApplyStaticSubsetHoist(result.module, plan);

  ValidateGenericModule(result.module);
  return result;
}

} // namespace cvub

#endif
