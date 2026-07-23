#include "../src/passes/auto_blockify_parallel_loop.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}

const cvub::GenericOperation *DefiningOperation(const cvub::GenericModule &module,
                                                int value) {
  for (const cvub::GenericOperation &operation : module.operations)
    for (int result : operation.results)
      if (result == value)
        return &operation;
  return nullptr;
}

int FunctionEntryBlock(const cvub::GenericModule &module,
                       const std::string &name) {
  for (const cvub::GenericOperation &operation : module.operations) {
    if (operation.name != "func.func" ||
        cvub::AutoBlockifyFunctionName(operation) != name)
      continue;
    const int region = operation.regions.front();
    return module.regions.at(static_cast<size_t>(region)).blocks.front();
  }
  throw std::runtime_error("function not found: " + name);
}

const std::vector<int> &BlockChildren(const cvub::GenericModule &module,
                                      int block) {
  return module.blocks.at(static_cast<size_t>(block)).operations;
}

bool BlockHasOperation(const cvub::GenericModule &module, int block,
                       const std::string &name) {
  for (int operationId : BlockChildren(module, block))
    if (module.operations.at(static_cast<size_t>(operationId)).name == name)
      return true;
  return false;
}

} // namespace

int main() {
  const cvub::GenericModule before = cvub::ParseGenericIR(
      "cvpipeline_ub_model_cpp/tests/fixtures/auto_blockify_grid_parallel.mlir",
      false);
  const cvub::GenericModule after = cvub::RunAutoBlockifyParallelLoop(before);

  // Exactly one blockify loop is introduced (the plain kernel gets none).
  std::vector<int> subloops;
  for (const cvub::GenericOperation &operation : after.operations)
    if (operation.name == "scf.for" &&
        cvub::AutoBlockifyHasAttr(operation.attributes,
                                  cvub::kAutoBlockifySubloopAttr))
      subloops.push_back(operation.id);
  Check(subloops.size() == 1,
        "transform must introduce exactly one autoblockify.subloop loop");
  const cvub::GenericOperation &loop =
      after.operations.at(static_cast<size_t>(subloops.front()));
  Check(loop.operands.size() == 3, "blockify loop must have lb/ub/step operands");

  // Locate the anchor ops in the transformed module.
  int blockIdxResult = -1;
  int logicalValue = -1;
  int getBlockIdxBlock = -1;
  int markBlock = -1;
  for (const cvub::GenericOperation &operation : after.operations) {
    if (operation.name == "hivm.hir.get_block_idx" &&
        operation.parentId == loop.parentId) {
      blockIdxResult = operation.results.front();
      getBlockIdxBlock = operation.blockId;
    }
    if (operation.name == "annotation.mark" &&
        operation.parentId == loop.parentId &&
        cvub::AutoBlockifyHasAttr(operation.attributes, "logical_block_num")) {
      logicalValue = operation.operands.front();
      markBlock = operation.blockId;
    }
  }
  Check(blockIdxResult >= 0, "get_block_idx must survive the transform");
  Check(logicalValue >= 0, "logical_block_num mark must survive the transform");

  const int gridEntryBlock = FunctionEntryBlock(after, "grid_kernel");

  // lower bound = trunci(get_block_idx); the trunci is the new outer-loop id.
  const cvub::GenericOperation *lowerBound =
      DefiningOperation(after, loop.operands[0]);
  Check(lowerBound != nullptr && lowerBound->name == "arith.trunci",
        "loop lower bound must be produced by arith.trunci");
  Check(lowerBound->operands.size() == 1 &&
            lowerBound->operands.front() == blockIdxResult,
        "loop lower bound trunci must consume the get_block_idx result");

  // upper bound = the logical_block_num value.
  Check(loop.operands[1] == logicalValue,
        "loop upper bound must be the logical_block_num value");

  // step = arith.constant equal to VECTOR_CORE_COUNT (48).
  const cvub::GenericOperation *step =
      DefiningOperation(after, loop.operands[2]);
  Check(step != nullptr && step->name == "arith.constant",
        "loop step must be an arith.constant");
  Check(step->attributes.find("48") != std::string::npos,
        "loop step must equal the AIV physical core count (48)");

  // The anchor ops stay outside the loop, in the grid function entry block.
  Check(getBlockIdxBlock == gridEntryBlock,
        "get_block_idx must stay in the function entry block");
  Check(markBlock == gridEntryBlock,
        "logical_block_num mark must stay in the function entry block");
  Check(loop.blockId == gridEntryBlock,
        "the blockify loop must live in the function entry block");
  const cvub::GenericOperation *logicalDef =
      DefiningOperation(after, logicalValue);
  Check(logicalDef != nullptr && logicalDef->blockId == gridEntryBlock,
        "logical_block_num def-chain must stay outside the loop");

  // Loop body: the moved body ops (the UB alloc and the store) landed inside.
  const int loopBodyBlock =
      after.regions.at(static_cast<size_t>(loop.regions.front())).blocks.front();
  Check(BlockHasOperation(after, loopBodyBlock, "memref.alloc"),
        "the UB allocation must move inside the loop");
  Check(BlockHasOperation(after, loopBodyBlock, "memref.store"),
        "the body store must move inside the loop");
  Check(!BlockHasOperation(after, gridEntryBlock, "memref.alloc"),
        "the UB allocation must not remain in the entry block");

  // Per-iteration block id: extsi(muli(iv, blockify_size)) at the loop head,
  // and the original block-idx use is rewritten onto that casted value.
  const int inductionValue =
      after.blocks.at(static_cast<size_t>(loopBodyBlock)).arguments.front();
  int mulResult = -1;
  int extResult = -1;
  for (int operationId : BlockChildren(after, loopBodyBlock)) {
    const cvub::GenericOperation &operation =
        after.operations.at(static_cast<size_t>(operationId));
    if (operation.name == "arith.muli" && operation.operands.size() == 2 &&
        operation.operands.front() == inductionValue)
      mulResult = operation.results.front();
    if (operation.name == "arith.extsi")
      extResult = operation.results.front();
  }
  Check(mulResult >= 0, "loop head must scale the induction var by blockify size");
  Check(extResult >= 0, "loop head must widen the scaled id to i64");
  const cvub::GenericOperation *extDef = DefiningOperation(after, extResult);
  Check(extDef != nullptr && extDef->operands.size() == 1 &&
            extDef->operands.front() == mulResult,
        "extsi must consume the muli result");

  bool rewrittenUseInLoop = false;
  for (int operationId : BlockChildren(after, loopBodyBlock)) {
    const cvub::GenericOperation &operation =
        after.operations.at(static_cast<size_t>(operationId));
    if (operation.name == "arith.trunci" && !operation.operands.empty() &&
        operation.operands.front() == extResult)
      rewrittenUseInLoop = true;
  }
  Check(rewrittenUseInLoop,
        "the original block-idx use must be rewritten onto the casted id");

  // get_block_idx result is consumed only by the new outer-loop lower bound.
  int blockIdxUsers = 0;
  for (const cvub::GenericOperation &operation : after.operations)
    for (int operand : operation.operands)
      if (operand == blockIdxResult)
        ++blockIdxUsers;
  Check(blockIdxUsers == 1,
        "get_block_idx must be used only by the outer-loop lower bound");

  // The non-grid device entry is untouched (no loop introduced).
  const int plainEntryBlock = FunctionEntryBlock(after, "plain_kernel");
  Check(!BlockHasOperation(after, plainEntryBlock, "scf.for"),
        "a device entry without get_block_idx must be left unchanged");
  Check(BlockHasOperation(after, plainEntryBlock, "memref.alloc") &&
            BlockHasOperation(after, plainEntryBlock, "memref.store"),
        "the untouched kernel must keep its original body");

  std::cout << "[PASS] auto-blockify wraps the grid-parallel body in an "
               "autoblockify.subloop loop\n";
  return 0;
}
