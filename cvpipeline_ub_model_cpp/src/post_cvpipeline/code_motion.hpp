#ifndef CVPIPELINE_UB_MODEL_CPP_POST_CVPIPELINE_CODE_MOTION_HPP
#define CVPIPELINE_UB_MODEL_CPP_POST_CVPIPELINE_CODE_MOTION_HPP

#include "ir_utils.hpp"
#include "types.hpp"

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

inline bool IsPureHoistableName(const std::string &name) {
  if (startsWith(name, "arith."))
    return true;
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

// Upstream createLoopInvariantSubsetHoistingPass rebuilds a loop when an iter
// arg flows through a matching extract_slice/insert_slice (or transfer) pair,
// threading a new carried iter arg/result.  That loop rebuild is not reproduced
// here, so the presence of the pattern is detected and reported as a scope gap.
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
inline StageResult RunLoopInvariantCodeMotion(GenericModule module,
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
// When no loop carries an iter arg through a tensor.extract_slice /
// tensor.insert_slice (or vector transfer) pair, the pass is a recognized no-op
// and the model reports Exact with the IR unchanged.  When the pattern is
// present, the real pass rebuilds the loop with an extra carried iter
// arg/result (extraction before the loop, insertion after); that loop rebuild
// is not reproduced at this altitude, so the model fails closed to Incomplete
// and flags the function as a scope candidate rather than silently dropping a
// lifetime-affecting hoist.  enableCodeMotion=false gates the pass to a no-op.
inline StageResult RunLoopInvariantSubsetHoisting(GenericModule module,
                                                  bool enableCodeMotion) {
  using namespace code_motion_detail;
  StageResult result;
  result.module = std::move(module);
  ValidateGenericModule(result.module);
  if (!enableCodeMotion)
    return result; // Recognized no-op, Exact.

  for (const GenericOperation &operation : result.module.operations) {
    if (operation.blockId < 0 || !IsModeledLoop(operation.name))
      continue;
    if (LoopCarriesSubsetPattern(result.module, operation.id))
      AddDiagnostic(result, operation.id, operation.name,
                    "LoopInvariantSubsetHoisting",
                    "loop carries a loop-invariant subset extraction/insertion "
                    "pair whose hoisting is not modeled");
  }

  ValidateGenericModule(result.module);
  return result;
}

} // namespace cvub

#endif
