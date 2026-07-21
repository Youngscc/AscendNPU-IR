#ifndef CVPIPELINE_UB_MODEL_CPP_AUTO_BLOCKIFY_PARALLEL_LOOP_HPP
#define CVPIPELINE_UB_MODEL_CPP_AUTO_BLOCKIFY_PARALLEL_LOOP_HPP

// Lightweight model of the real `AutoBlockifyParallelLoop` pass
// (bishengir/lib/Dialect/HIVM/Transforms/AutoBlockifyParallelLoop.cpp,
// `loopOnLogicBlock`). The real pass runs inside
// `hivmPreBufferizationOptimizationPipeline`, gated on
// `enableTritonKernelCompile && enableAutoBlockifyLoop`, BEFORE the
// before-CVPipelining boundary that this model consumes. It is therefore
// modeled here as an IR-to-IR transform on the before-CVPipelining generic
// module: enabling the flag applies the rewrite up front, disabling it is the
// identity.
//
// Rewrite (mirrors the C++ exactly):
//   For every grid-parallel device entry (a func carrying an
//   `annotation.mark {logical_block_num}` op AND a `hivm.hir.get_block_idx`),
//   wrap the per-block body in
//
//     scf.for %iv = trunci(block_idx) to logical_block_num
//                   step physical_core_count : i32 {
//       %mul    = arith.muli %iv, %blockify_size : i32
//       %casted = arith.extsi %mul : i32 to i64        // per-iteration block id
//       ...body...
//     } {autoblockify.subloop}
//
//   * OUTSIDE the loop, unchanged: the `annotation.mark {logical_block_num}`
//     op, the transitive def-chain of `logical_block_num` (`traceExceptions`),
//     and the `hivm.hir.get_block_idx` op.
//   * INSIDE the loop: every other body op in original order; uses of the
//     block-idx result are rewritten to the per-iteration `arith.extsi` value.
//   * `physical_core_count` = VECTOR_CORE_COUNT (AIV) / CUBE_CORE_COUNT (AIC),
//     read from the module `dlti.target_system_spec`. AIC_OR_AIV / a missing
//     spec makes the real `getPhysicalBlockNum` fail; this model fails closed
//     (throws) rather than emitting an unmodeled result.
//
// Standalone transform only: this header is not yet wired into
// `cvpipelining_ub_pipeline.hpp` or the CLI. Validating the flag-on path
// end-to-end needs a blockified oracle corpus regenerated with the real
// compiler (`--enable-auto-blockify-loop --enable-triton-kernel-compile`),
// which is a separate step.

#include "../ir/generic_rewriter.hpp"
#include "../pipeline/buffer_topology.hpp"

#include <optional>
#include <regex>
#include <unordered_map>

namespace cvub {

constexpr char kAutoBlockifySubloopAttr[] = "autoblockify.subloop";

// Value between the last '<' and '>' of an enum-attribute spelling, e.g.
// "#hivm.func_core_type<AIV>" -> "AIV".
inline std::string AutoBlockifyEnumInner(const std::string &text) {
  const size_t open = text.rfind('<');
  const size_t close = text.rfind('>');
  if (open == std::string::npos || close == std::string::npos || open >= close)
    return text;
  return text.substr(open + 1, close - open - 1);
}

// Presence test for a (possibly unit) attribute in a `{...}` dictionary body.
inline bool AutoBlockifyHasAttr(const std::string &dictionary,
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

// Numeric target-spec entry (e.g. VECTOR_CORE_COUNT) from the module attrs.
inline std::optional<int64_t>
AutoBlockifySpecCount(const std::string &attributes, const std::string &name) {
  const std::regex entry("\"" + name +
                         "\"[[:space:]]*,[[:space:]]*([0-9]+)");
  std::smatch match;
  if (!std::regex_search(attributes, match, entry))
    return std::nullopt;
  try {
    return std::stoll(match[1].str());
  } catch (const std::exception &) {
    return std::nullopt;
  }
}

inline std::string AutoBlockifyFunctionName(const GenericOperation &function) {
  std::string name = FindDictionaryValue(function.properties, "sym_name");
  if (name.empty())
    name = FindDictionaryValue(function.attributes, "sym_name");
  if (name.size() >= 2 && name.front() == '"' && name.back() == '"')
    name = name.substr(1, name.size() - 2);
  return name;
}

// Mirror `getPhysicalBlockNum`: AIV -> VECTOR_CORE_COUNT, AIC -> CUBE_CORE_COUNT,
// AIC_OR_AIV / missing spec -> no value (the real pass fails).
inline std::optional<int64_t>
AutoBlockifyPhysicalBlockNum(const std::string &coreType,
                             const std::string &moduleAttributes) {
  if (coreType == "AIV")
    return AutoBlockifySpecCount(moduleAttributes, "VECTOR_CORE_COUNT");
  if (coreType == "AIC")
    return AutoBlockifySpecCount(moduleAttributes, "CUBE_CORE_COUNT");
  return std::nullopt;
}

// Apply the rewrite to one func. No-op unless the func is a grid-parallel
// device entry; throws (fail closed) on a grid-parallel entry whose blockify
// parameters cannot be modeled exactly.
inline void AutoBlockifyOneFunction(GenericModule &module,
                                    GenericRewriter &rewriter, int funcId) {
  const std::string coreKind = AutoBlockifyEnumInner(FindDictionaryValue(
      module.operations.at(static_cast<size_t>(funcId)).attributes,
      "hacc.function_kind"));
  if (coreKind != "DEVICE")
    return;
  {
    const GenericOperation &function =
        module.operations.at(static_cast<size_t>(funcId));
    if (function.regions.empty())
      return;
  }
  const int entryRegionId =
      module.operations.at(static_cast<size_t>(funcId)).regions.front();
  if (module.regions.at(static_cast<size_t>(entryRegionId)).blocks.empty())
    return;
  const int entryBlockId =
      module.regions.at(static_cast<size_t>(entryRegionId)).blocks.front();

  const std::vector<int> original =
      module.blocks.at(static_cast<size_t>(entryBlockId)).operations;

  int markId = -1;
  int getBlockIdxId = -1;
  int returnId = -1;
  int logicalValue = -1;
  int blockIdxResult = -1;
  for (int operationId : original) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    if (operation.name == "annotation.mark" &&
        AutoBlockifyHasAttr(operation.attributes, "logical_block_num")) {
      markId = operationId;
      if (!operation.operands.empty())
        logicalValue = operation.operands.front();
      continue;
    }
    if (operation.name == "hivm.hir.get_block_idx") {
      getBlockIdxId = operationId;
      if (!operation.results.empty())
        blockIdxResult = operation.results.front();
    }
    if (operation.name == "func.return")
      returnId = operationId;
  }

  // The real pass returns success (no-op) when there is no get_block_idx.
  if (getBlockIdxId < 0)
    return;

  const std::string functionName =
      AutoBlockifyFunctionName(module.operations.at(static_cast<size_t>(funcId)));
  if (markId < 0 || logicalValue < 0)
    throw std::runtime_error(
        "AutoBlockifyParallelLoop: logical_block_num mark not found in " +
        functionName);
  if (blockIdxResult < 0)
    throw std::runtime_error(
        "AutoBlockifyParallelLoop: get_block_idx has no result in " +
        functionName);

  const std::string coreType = AutoBlockifyEnumInner(FindDictionaryValue(
      module.operations.at(static_cast<size_t>(funcId)).attributes,
      "hivm.func_core_type"));
  const std::optional<int64_t> physical = AutoBlockifyPhysicalBlockNum(
      coreType, module.operations.front().attributes);
  if (!physical)
    throw std::runtime_error(
        "AutoBlockifyParallelLoop: physical block num cannot be inferred for " +
        functionName + " (core=" + coreType + ")");

  int64_t blockifyNum = 1;
  const std::string blockifyAttr = FindDictionaryValue(
      module.operations.at(static_cast<size_t>(funcId)).attributes,
      "auto_blockify_size");
  if (!blockifyAttr.empty()) {
    try {
      blockifyNum = std::stoll(blockifyAttr);
    } catch (const std::exception &) {
      throw std::runtime_error(
          "AutoBlockifyParallelLoop: unparseable auto_blockify_size in " +
          functionName);
    }
  }
  if (blockifyNum != 1)
    throw std::runtime_error(
        "AutoBlockifyParallelLoop: auto_blockify_size > 1 is not modeled in " +
        functionName);

  // traceExceptions: the transitive def-chain of logical_block_num stays
  // outside the loop, together with get_block_idx.
  std::unordered_map<int, int> definitionOf;
  for (const GenericOperation &operation : module.operations)
    for (int result : operation.results)
      definitionOf[result] = operation.id;
  std::set<int> exceptions;
  exceptions.insert(getBlockIdxId);
  {
    std::vector<int> worklist = {logicalValue};
    std::set<int> visited;
    while (!worklist.empty()) {
      const int value = worklist.back();
      worklist.pop_back();
      if (!visited.insert(value).second)
        continue;
      const auto found = definitionOf.find(value);
      if (found == definitionOf.end())
        continue; // block argument: chain terminates
      exceptions.insert(found->second);
      for (int operand :
           module.operations.at(static_cast<size_t>(found->second)).operands)
        worklist.push_back(operand);
    }
  }

  std::vector<int> opsToMove;
  for (int operationId : original) {
    if (operationId == returnId || operationId == markId ||
        exceptions.count(operationId) != 0)
      continue;
    opsToMove.push_back(operationId);
  }

  // New per-block loop bounds, created in the entry block.
  const int physicalConst = rewriter.createOperation(
      funcId, entryRegionId, entryBlockId, "arith.constant", {"i32"}, {}, {}, "",
      "{value = " + std::to_string(*physical) + " : i32}");
  const int blockifyConst = rewriter.createOperation(
      funcId, entryRegionId, entryBlockId, "arith.constant", {"i32"}, {}, {}, "",
      "{value = " + std::to_string(blockifyNum) + " : i32}");
  const int blockIdTrunc = rewriter.createOperation(
      funcId, entryRegionId, entryBlockId, "arith.trunci", {"i32"},
      {blockIdxResult}, {"i64"}, "", "{}");
  const int physicalValue =
      module.operations.at(static_cast<size_t>(physicalConst)).results.front();
  const int blockifyValue =
      module.operations.at(static_cast<size_t>(blockifyConst)).results.front();
  const int blockIdValue =
      module.operations.at(static_cast<size_t>(blockIdTrunc)).results.front();

  const int forOp = rewriter.createOperation(
      funcId, entryRegionId, entryBlockId, "scf.for", {},
      {blockIdValue, logicalValue, physicalValue}, {"i32", "i32", "i32"}, "",
      std::string("{") + kAutoBlockifySubloopAttr + "}");
  const int loopRegionId = rewriter.createRegion(forOp);
  const int loopBlockId = rewriter.createBlock(loopRegionId, {"i32"});
  const int inductionValue =
      module.blocks.at(static_cast<size_t>(loopBlockId)).arguments.front();

  // Per-iteration block id: extsi(muli(iv, blockify_size)).
  const int mulOp = rewriter.createOperation(
      forOp, loopRegionId, loopBlockId, "arith.muli", {"i32"},
      {inductionValue, blockifyValue}, {"i32", "i32"}, "", "{}");
  const int mulValue =
      module.operations.at(static_cast<size_t>(mulOp)).results.front();
  const int extOp = rewriter.createOperation(
      forOp, loopRegionId, loopBlockId, "arith.extsi", {"i64"}, {mulValue},
      {"i32"}, "", "{}");
  const int castedValue =
      module.operations.at(static_cast<size_t>(extOp)).results.front();
  rewriter.appendToBlock(loopBlockId, mulOp);
  rewriter.appendToBlock(loopBlockId, extOp);

  for (int operationId : opsToMove) {
    rewriter.removeFromBlock(entryBlockId, operationId);
    GenericOperation &moved =
        module.operations.at(static_cast<size_t>(operationId));
    moved.parentId = forOp;
    moved.regionId = loopRegionId;
    moved.blockId = loopBlockId;
    rewriter.appendToBlock(loopBlockId, operationId);
  }
  const int yieldOp = rewriter.createOperation(forOp, loopRegionId, loopBlockId,
                                               "scf.yield", {});
  rewriter.appendToBlock(loopBlockId, yieldOp);

  // Splice the loop bounds and the loop itself in front of the terminator.
  const std::vector<int> outside =
      module.blocks.at(static_cast<size_t>(entryBlockId)).operations;
  size_t insertPosition = outside.size();
  for (size_t index = 0; index < outside.size(); ++index)
    if (outside[index] == returnId) {
      insertPosition = index;
      break;
    }
  rewriter.insertToBlock(entryBlockId, insertPosition++, physicalConst);
  rewriter.insertToBlock(entryBlockId, insertPosition++, blockifyConst);
  rewriter.insertToBlock(entryBlockId, insertPosition++, blockIdTrunc);
  rewriter.insertToBlock(entryBlockId, insertPosition, forOp);

  // replaceAllUsesExcept(get_block_idx, casted, blockIdTrunc).
  for (GenericOperation &operation : module.operations) {
    if (operation.id == blockIdTrunc)
      continue;
    for (int &operand : operation.operands)
      if (operand == blockIdxResult)
        operand = castedValue;
    for (int &operand : operation.dpsInputs)
      if (operand == blockIdxResult)
        operand = castedValue;
    for (int &operand : operation.dpsInits)
      if (operand == blockIdxResult)
        operand = castedValue;
  }
}

inline GenericModule RunAutoBlockifyParallelLoop(GenericModule module) {
  std::vector<int> functionIds;
  for (const GenericOperation &operation : module.operations)
    if (operation.name == "func.func")
      functionIds.push_back(operation.id);
  GenericRewriter rewriter(module);
  for (int functionId : functionIds)
    AutoBlockifyOneFunction(module, rewriter, functionId);
  return CompactGenericModule(std::move(module));
}

} // namespace cvub

#endif
