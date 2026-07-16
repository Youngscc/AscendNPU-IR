#ifndef CVPIPELINE_UB_MODEL_CPP_LOOP_INVARIANT_CODE_MOTION_HPP
#define CVPIPELINE_UB_MODEL_CPP_LOOP_INVARIANT_CODE_MOTION_HPP

#include "../ir/generic_rewriter.hpp"
#include "../ir/operation_folder.hpp"

#include <deque>

namespace cvub {

inline bool IsLoopInvariantCodeMotionTerminator(
    const GenericOperation &operation) {
  static const std::set<std::string> terminators = {
      "affine.yield", "cf.br",       "cf.cond_br", "func.return",
      "scf.condition", "scf.yield"};
  return terminators.count(operation.name) != 0;
}

inline bool IsLoopInvariantCodeMotionSpeculatable(
    const GenericOperation &operation,
    const std::map<int, ArithIntegerConstant> &integerConstants) {
  if (IsHIVMVectorOp(operation.name))
    return true;

  if (startsWith(operation.name, "arith.")) {
    const bool isUnsignedDivision = operation.name == "arith.divui" ||
                                    operation.name == "arith.ceildivui";
    const bool isSignedDivision = operation.name == "arith.divsi" ||
                                  operation.name == "arith.ceildivsi";
    if (!isUnsignedDivision && !isSignedDivision)
      return true;
    if (operation.operands.size() != 2)
      return false;

    const auto rhs = integerConstants.find(operation.operands[1]);
    if (rhs == integerConstants.end())
      return false;
    if (rhs->second.bits == 0)
      return false;

    // Mirrors the four Arith getSpeculatability implementations. Unsigned
    // division only rejects zero; signed division also rejects an all-ones
    // RHS because INT_MIN / -1 has undefined behavior.
    if (isUnsignedDivision)
      return true;
    const uint64_t allOnes = rhs->second.width == 64
                                 ? ~uint64_t{0}
                                 : (uint64_t{1} << rhs->second.width) - 1;
    return rhs->second.bits != allOnes;
  }

  // MLIR's Pure trait is AlwaysSpeculatable plus NoMemoryEffect. The reviewed
  // tensor/affine/memref view operations in HasNoMemoryEffect carry that
  // trait; terminators are rejected before this predicate.
  return operation.name != "llvm.inline_asm" &&
         HasNoMemoryEffect(operation.name);
}

inline bool IsLoopInvariantCodeMotionMemoryEffectFree(
    const GenericOperation &operation) {
  if (HasNoMemoryEffect(operation.name))
    return true;
  if (!IsHIVMVectorOp(operation.name))
    return false;

  // HIVMStructuredOpInterface::getEffects returns no effects for pure tensor
  // semantics. Any memref operand switches the operation to buffer semantics
  // and therefore prevents LICM.
  return std::none_of(operation.operandTypes.begin(),
                      operation.operandTypes.end(),
                      [](const std::string &type) {
                        return GenericIsMemRefType(type);
                      });
}

class LoopInvariantCodeMotionModel {
public:
  explicit LoopInvariantCodeMotionModel(GenericModule &inputModule)
      : module(inputModule), rewriter(module) {
    indexValuesAndUsers();
  }

  size_t Run() {
    std::vector<int> loops;
    if (!module.operations.empty())
      collectLoopsPostOrder(0, loops);
    size_t moved = 0;
    for (int loop : loops)
      moved += moveLoopInvariantCode(loop);
    return moved;
  }

private:
  void indexValuesAndUsers() {
    for (const GenericBlock &block : module.blocks)
      for (int argument : block.arguments)
        blockArgumentOwners[argument] = block.id;
    for (const GenericOperation &operation : module.operations) {
      for (int result : operation.results)
        definingOperations[result] = operation.id;
      if (const std::optional<ArithIntegerConstant> value =
              ParseArithIntegerConstant(operation))
        integerConstants[operation.results.front()] = *value;
      for (int operand : operation.operands)
        users[operand].push_back(operation.id);
    }
  }

  void collectLoopsPostOrder(int operationId, std::vector<int> &loops) const {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    for (int regionId : operation.regions) {
      const GenericRegion &region =
          module.regions.at(static_cast<size_t>(regionId));
      for (int blockId : region.blocks)
        for (int child :
             module.blocks.at(static_cast<size_t>(blockId)).operations)
          collectLoopsPostOrder(child, loops);
    }
    static const std::set<std::string> loopLikeOperations = {
        "affine.for", "affine.parallel", "scf.for", "scf.forall",
        "scf.parallel", "scf.while"};
    if (loopLikeOperations.count(operation.name) != 0) {
      loops.push_back(operationId);
      return;
    }
  }

  bool isDefinedOutsideLoop(int value, int loopId) const {
    auto isInsideLoop = [&](int regionId) {
      while (regionId >= 0) {
        const int parentOperation = module.regions.at(
            static_cast<size_t>(regionId)).parentOperation;
        if (parentOperation == loopId)
          return true;
        if (parentOperation < 0)
          return false;
        regionId = module.operations.at(
            static_cast<size_t>(parentOperation)).regionId;
      }
      return false;
    };
    auto definition = definingOperations.find(value);
    if (definition != definingOperations.end())
      return !isInsideLoop(module.operations.at(
          static_cast<size_t>(definition->second)).regionId);
    auto blockArgument = blockArgumentOwners.find(value);
    if (blockArgument == blockArgumentOwners.end())
      return true;
    return !isInsideLoop(module.blocks.at(
        static_cast<size_t>(blockArgument->second)).regionId);
  }

  bool canBeHoisted(const GenericOperation &operation, int loopId) const {
    if (IsLoopInvariantCodeMotionTerminator(operation) ||
        !operation.regions.empty() ||
        !IsLoopInvariantCodeMotionSpeculatable(operation, integerConstants) ||
        !IsLoopInvariantCodeMotionMemoryEffectFree(operation))
      return false;
    return std::all_of(operation.operands.begin(), operation.operands.end(),
                       [&](int value) {
                         return isDefinedOutsideLoop(value, loopId);
                       });
  }

  size_t moveLoopInvariantCode(int loopId) {
    GenericOperation &loop =
        module.operations.at(static_cast<size_t>(loopId));
    if (loop.regions.empty() || loop.blockId < 0)
      return 0;
    const int parentBlockId = loop.blockId;

    size_t moved = 0;
    for (int regionId : loop.regions) {
      const GenericRegion &region =
          module.regions.at(static_cast<size_t>(regionId));
      for (int bodyBlockId : region.blocks) {
        std::deque<int> worklist;
        for (int operation : module.blocks.at(
                 static_cast<size_t>(bodyBlockId)).operations)
          worklist.push_back(operation);
        while (!worklist.empty()) {
          const int operationId = worklist.front();
          worklist.pop_front();
          GenericOperation &operation =
              module.operations.at(static_cast<size_t>(operationId));
          if (operation.regionId != regionId ||
              !canBeHoisted(operation, loopId))
            continue;

          rewriter.removeFromBlock(bodyBlockId, operationId);
          const GenericBlock &parent =
              module.blocks.at(static_cast<size_t>(parentBlockId));
          auto loopPosition = std::find(parent.operations.begin(),
                                        parent.operations.end(), loopId);
          if (loopPosition == parent.operations.end())
            throw std::runtime_error(
                "loop-invariant-code-motion: loop is missing from parent block");
          rewriter.insertToBlock(
              parentBlockId,
              static_cast<size_t>(loopPosition - parent.operations.begin()),
              operationId);
          ++moved;

          for (int result : operation.results) {
            auto resultUsers = users.find(result);
            if (resultUsers == users.end())
              continue;
            for (int user : resultUsers->second)
              if (module.operations.at(static_cast<size_t>(user)).regionId ==
                  regionId)
                worklist.push_back(user);
          }
        }
      }
    }
    return moved;
  }

  GenericModule &module;
  GenericRewriter rewriter;
  std::map<int, int> definingOperations;
  std::map<int, int> blockArgumentOwners;
  std::map<int, ArithIntegerConstant> integerConstants;
  std::map<int, std::vector<int>> users;
};

inline size_t RunLoopInvariantCodeMotion(GenericModule &module) {
  return LoopInvariantCodeMotionModel(module).Run();
}

} // namespace cvub

#endif
