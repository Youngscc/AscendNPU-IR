#ifndef CVPIPELINE_UB_MODEL_CPP_SPLIT_MIX_KERNEL_HPP
#define CVPIPELINE_UB_MODEL_CPP_SPLIT_MIX_KERNEL_HPP

#include "../ir/generic_analysis.hpp"
#include "../ir/generic_rewriter.hpp"
#include "one_shot_bufferize.hpp"
#include "../ir/operation_folder.hpp"

namespace cvub {

enum class SplitMixCoreType { Common, Cube, Vector, Both };

inline std::string SplitMixEnumValue(const std::string &text) {
  const size_t open = text.rfind('<');
  const size_t close = text.rfind('>');
  return open == std::string::npos || close == std::string::npos || open >= close
             ? text
             : text.substr(open + 1, close - open - 1);
}

inline bool HasSplitMixDictionaryEntry(const std::string &dictionary,
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

inline bool IsSplitMixTerminator(const GenericOperation &operation) {
  static const std::set<std::string> terminators = {
      "affine.yield", "cf.br",       "cf.cond_br", "func.return",
      "scf.condition", "scf.yield"};
  return terminators.count(operation.name) != 0;
}

inline bool IsSplitMixKnownPureOperation(
    const GenericOperation &operation) {
  // These dialect operations carry MLIR's Pure trait. Keep this list narrow:
  // result-less HIVM setup/synchronization ops may also serialize with
  // effects="none", but are not trivially dead.
  return startsWith(operation.name, "arith.") ||
         startsWith(operation.name, "affine.");
}

inline SplitMixCoreType GetSplitMixCoreType(
    const GenericOperation &operation) {
  std::string core =
      FindDictionaryValue(operation.properties, "hivm.tcore_type");
  if (core.empty())
    core = FindDictionaryValue(operation.attributes, "hivm.tcore_type");
  if (core.empty())
    core = FindDictionaryValue(operation.properties, "tcoretype");
  if (core.empty())
    core = FindDictionaryValue(operation.attributes, "tcoretype");
  if (core.empty())
    core = FindDictionaryValue(operation.properties, "tcore_type");
  if (core.empty())
    core = FindDictionaryValue(operation.attributes, "tcore_type");
  core = SplitMixEnumValue(core);
  if (core == "CUBE")
    return SplitMixCoreType::Cube;
  if (core == "VECTOR")
    return SplitMixCoreType::Vector;
  if (core == "CUBE_AND_VECTOR")
    return SplitMixCoreType::Both;

  if (IsHIVMVectorOp(operation.name) ||
      operation.name == "hivm.hir.store" ||
      operation.name == "hivm.hir.atomic_rmw")
    return SplitMixCoreType::Vector;

  static const std::set<std::string> cubeOperations = {
      "hivm.hir.Conv1dL1", "hivm.hir.Conv2dL1",
      "hivm.hir.Conv3dL1", "hivm.hir.batchMmadL1",
      "hivm.hir.fixpipe", "hivm.hir.matmul", "hivm.hir.mmadL1"};
  return cubeOperations.count(operation.name) != 0
             ? SplitMixCoreType::Cube
             : SplitMixCoreType::Common;
}

inline bool IsSplitMixFunction(const GenericOperation &operation) {
  return operation.name == "func.func" &&
         SplitMixEnumValue(FindDictionaryValue(
             operation.attributes, "hivm.func_core_type")) == "MIX";
}

inline void MarkSplitMixProjectedFunction(GenericOperation &function,
                                          SplitMixCoreType retainedCore) {
  const std::string from = "#hivm.func_core_type<MIX>";
  const std::string to = retainedCore == SplitMixCoreType::Vector
                             ? "#hivm.func_core_type<AIV>"
                             : "#hivm.func_core_type<AIC>";
  const size_t core = function.attributes.find(from);
  if (core == std::string::npos)
    throw std::runtime_error(
        "SplitMixKernel: malformed MIX function core type attribute");
  function.attributes.replace(core, from.size(), to);
  if (!HasSplitMixDictionaryEntry(function.attributes, "hivm.part_of_mix")) {
    const size_t close = function.attributes.rfind('}');
    if (close == std::string::npos)
      throw std::runtime_error(
          "SplitMixKernel: malformed function attribute dictionary");
    const bool empty = trim(function.attributes.substr(1, close - 1)).empty();
    function.attributes.insert(close, empty ? "hivm.part_of_mix"
                                            : ", hivm.part_of_mix");
  }
}

inline void ReplaceSplitMixValue(GenericModule &module, int from, int to) {
  for (GenericOperation &operation : module.operations) {
    for (int &operand : operation.operands)
      if (operand == from)
        operand = to;
    for (int &operand : operation.dpsInputs)
      if (operand == from)
        operand = to;
    for (int &operand : operation.dpsInits)
      if (operand == from)
        operand = to;
  }
}

inline bool IsSplitMixTriviallyDead(const GenericModule &module,
                                    const GenericOperation &operation,
                                    const std::set<int> &activeOperations,
                                    const PipelineAnalysisContext &useLists) {
  if (operation.blockId < 0 || operation.name == "func.func" ||
      IsSplitMixTerminator(operation))
    return false;
  // HIVM_SynchronizationOp carries ordering side effects even though it has no
  // shaped memory effect. MLIR's isOpTriviallyDead therefore never removes it.
  if (operation.name == "hivm.hir.sync_block" ||
      operation.name == "hivm.hir.sync_block_set" ||
      operation.name == "hivm.hir.sync_block_wait" ||
      operation.name == "hivm.hir.pipe_barrier")
    return false;
  if (std::any_of(operation.results.begin(), operation.results.end(),
                  [&](int result) { return useLists.hasUsers(result); }))
    return false;

  if (!operation.regions.empty()) {
    for (int regionId : operation.regions)
      for (int blockId :
           module.regions.at(static_cast<size_t>(regionId)).blocks)
        for (int childId :
             module.blocks.at(static_cast<size_t>(blockId)).operations) {
          if (activeOperations.count(childId) == 0)
            continue;
          const GenericOperation &child =
              module.operations.at(static_cast<size_t>(childId));
          if (IsSplitMixTerminator(child))
            continue;
          if (!IsSplitMixTriviallyDead(module, child, activeOperations,
                                       useLists))
            return false;
        }
    return true;
  }

  if (operation.effects.empty() ||
      (operation.effects == "none" &&
       IsSplitMixKnownPureOperation(operation)))
    return true;
  const std::vector<std::string> effects = split(operation.effects, ',');
  return !effects.empty() &&
         std::all_of(effects.begin(), effects.end(),
                     [](const std::string &effect) {
                       const std::string value = trim(effect);
                       return startsWith(value, "read@") ||
                              startsWith(value, "allocate@");
                     });
}

inline void RunSplitMixOperationFolder(
    GenericModule &module, int functionId, std::set<int> &activeOperations,
    PipelineAnalysisContext &useLists) {
  const GenericOperation &function =
      module.operations.at(static_cast<size_t>(functionId));
  if (function.regions.size() != 1)
    return;
  const GenericRegion &functionRegion =
      module.regions.at(static_cast<size_t>(function.regions.front()));
  if (functionRegion.blocks.empty())
    return;
  const int insertionBlock = functionRegion.blocks.front();

  std::vector<int> postOrder;
  std::function<void(int)> collect = [&](int operationId) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    for (int regionId : operation.regions)
      for (int blockId :
           module.regions.at(static_cast<size_t>(regionId)).blocks)
        for (int child :
             module.blocks.at(static_cast<size_t>(blockId)).operations) {
          collect(child);
          postOrder.push_back(child);
        }
  };
  collect(functionId);

  GenericRewriter rewriter(module, &useLists);
  std::set<int> folderOwnedConstants;
  std::map<std::tuple<std::string, std::string, std::string>, int>
      uniquedConstants;
  for (int operationId : postOrder) {
    if (activeOperations.count(operationId) == 0)
      continue;
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    if (operation.name != "arith.constant" || operation.results.size() != 1 ||
        operation.resultTypes.size() != 1)
      continue;

    const auto key = std::make_tuple(operation.name,
                                     operation.properties + operation.attributes,
                                     operation.resultTypes.front());
    auto existing = uniquedConstants.find(key);
    if (existing != uniquedConstants.end()) {
      const int replacement =
          module.operations.at(static_cast<size_t>(existing->second))
              .results.front();
      rewriter.replaceAllUses(operation.results.front(), replacement);
      rewriter.removeFromBlock(operation.blockId, operationId);
      activeOperations.erase(operationId);
      continue;
    }

    uniquedConstants.emplace(key, operationId);
    const int sourceBlock = operation.blockId;
    const GenericBlock &block =
        module.blocks.at(static_cast<size_t>(sourceBlock));
    const auto position =
        std::find(block.operations.begin(), block.operations.end(), operationId);
    const bool atFront = position == block.operations.begin();
    const bool followsFolderConstant =
        position != block.operations.begin() &&
        folderOwnedConstants.count(*std::prev(position)) != 0;
    if (sourceBlock != insertionBlock || (!atFront && !followsFolderConstant)) {
      rewriter.removeFromBlock(sourceBlock, operationId);
      rewriter.insertToBlock(insertionBlock, 0, operationId);
    }
    folderOwnedConstants.insert(operationId);
  }
}

// Mirrors RemoveUselessMarkOps in SplitMixKernel.cpp's vector post-process.
inline void RemoveUselessSplitMixMarkOps(
    GenericModule &module, std::set<int> &activeOperations) {
  std::map<int, const GenericOperation *> definitions =
      DefiningOperations(module);
  std::map<int, std::vector<int>> users;
  for (int operationId : activeOperations)
    for (int operand :
         module.operations.at(static_cast<size_t>(operationId)).operands)
      users[operand].push_back(operationId);

  GenericRewriter rewriter(module);
  std::vector<int> marks;
  std::vector<int> erasedMarks;
  for (int operationId : activeOperations)
    if (module.operations.at(static_cast<size_t>(operationId)).name ==
        "annotation.mark")
      marks.push_back(operationId);
  for (int operationId : marks) {
    const GenericOperation &mark =
        module.operations.at(static_cast<size_t>(operationId));
    if ((trim(mark.attributes) != "" && trim(mark.attributes) != "{}") ||
        (trim(mark.properties) != "" && trim(mark.properties) != "{}") ||
        mark.operands.empty())
      continue;
    const int source = mark.operands.front();
    auto definition = definitions.find(source);
    if (definition != definitions.end() &&
        (definition->second->name == "hivm.hir.fixpipe" ||
         definition->second->name == "hivm.hir.store"))
      continue;
    const bool onlyMarks =
        !users[source].empty() &&
        std::all_of(users[source].begin(), users[source].end(),
                    [&](int user) {
                      return module.operations.at(static_cast<size_t>(user))
                                 .name == "annotation.mark";
                    });
    if (onlyMarks)
      continue;
    erasedMarks.push_back(operationId);
    activeOperations.erase(operationId);
  }
  rewriter.removeManyFromBlocks(erasedMarks);
}

// Mirrors postProcessVectorFunc. InsertLoadStoreForMixCV may duplicate a
// tensor.extract for the cube half and connect the original/replacement with
// an annotation mark. Those cube-only records must not survive in AIV.
inline void PostProcessSplitMixVectorFunc(
    GenericModule &module, int functionId,
    std::set<int> &activeOperations, PipelineAnalysisContext &useLists) {
  std::vector<int> descendants;
  std::function<void(int)> collect = [&](int operationId) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    for (int regionId : operation.regions)
      for (int blockId :
           module.regions.at(static_cast<size_t>(regionId)).blocks)
        for (int child :
             module.blocks.at(static_cast<size_t>(blockId)).operations) {
          descendants.push_back(child);
          collect(child);
        }
  };
  collect(functionId);

  GenericRewriter rewriter(module, &useLists);
  std::vector<int> erasedOperations;
  for (int operationId : descendants) {
    if (activeOperations.count(operationId) == 0)
      continue;
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    const bool replacementMark =
        operation.name == "annotation.mark" &&
        operation.attributes.find(
            "DuplicateTensorExtractForCube::replacementLabel") !=
            std::string::npos;
    const bool cubeExtract =
        operation.name == "tensor.extract" &&
        operation.attributes.find(
            "DuplicateTensorExtractForCube::newExtractLabel") !=
            std::string::npos;
    if (!replacementMark && !cubeExtract)
      continue;
    erasedOperations.push_back(operationId);
    activeOperations.erase(operationId);
  }
  rewriter.removeManyFromBlocks(erasedOperations);
}

// Mirrors PostCubeReplacement and the explicit cube-erasure cleanup in
// postProcessCubeFunc. This projection is used by MarkRealCoreType, whose
// source pass runs SplitMixKernel on a clone and observes which original
// operations survive in the AIC and AIV functions.
inline void PostProcessSplitMixCubeFunc(
    GenericModule &module, int functionId,
    std::set<int> &activeOperations, PipelineAnalysisContext &useLists) {
  std::vector<int> descendants;
  std::function<void(int)> collect = [&](int operationId) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    for (int regionId : operation.regions)
      for (int blockId :
           module.regions.at(static_cast<size_t>(regionId)).blocks)
        for (int child :
             module.blocks.at(static_cast<size_t>(blockId)).operations) {
          descendants.push_back(child);
          collect(child);
        }
  };
  collect(functionId);

  GenericRewriter rewriter(module, &useLists);
  std::vector<int> erasedOperations;
  for (int operationId : descendants) {
    if (activeOperations.count(operationId) == 0)
      continue;
    const GenericOperation operation =
        module.operations.at(static_cast<size_t>(operationId));
    const bool replacementMark =
        operation.name == "annotation.mark" &&
        operation.attributes.find(
            "DuplicateTensorExtractForCube::replacementLabel") !=
            std::string::npos;
    if (replacementMark && operation.operands.size() >= 2) {
      rewriter.replaceAllUses(operation.operands[0], operation.operands[1]);
      continue;
    }

    const bool cubeErasure =
        operation.attributes.find(
            "DuplicateTensorExtractForCube::cubeErasureLabel") !=
        std::string::npos;
    if (cubeErasure &&
        (operation.name == "bufferization.to_tensor" ||
         operation.name == "hivm.hir.load" ||
         operation.name == "memref.alloc")) {
      erasedOperations.push_back(operationId);
      activeOperations.erase(operationId);
    }
  }
  rewriter.removeManyFromBlocks(erasedOperations);
}

inline std::optional<bool>
SplitMixKnownBoolean(
    const GenericOperation &operation,
    const std::map<int, ArithIntegerConstant> &constants) {
  if (operation.results.size() != 1 || operation.resultTypes.size() != 1 ||
      operation.resultTypes.front() != "i1")
    return std::nullopt;
  if (operation.name == "arith.constant") {
    std::string value = FindDictionaryValue(operation.properties, "value");
    if (value.empty())
      value = FindDictionaryValue(operation.attributes, "value");
    const size_t separator = value.find(':');
    if (separator != std::string::npos)
      value.resize(separator);
    value = trim(std::move(value));
    if (value == "true" || value == "1")
      return true;
    if (value == "false" || value == "0")
      return false;
  }
  return FoldArithCmpI(operation, constants);
}

// applyPatternsGreedily in postProcessVectorFunc invokes each operation's
// fold hook even when no explicit Arith pattern was registered.
inline bool FoldSplitMixBooleanOperation(
    GenericModule &module, std::set<int> &activeOperations,
    PipelineAnalysisContext &useLists) {
  std::map<int, ArithIntegerConstant> integerConstants;
  for (int operationId : activeOperations) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    if (const std::optional<ArithIntegerConstant> value =
            ParseArithIntegerConstant(operation))
      integerConstants[operation.results.front()] = *value;
  }
  std::map<int, bool> knownValues;
  for (int operationId : activeOperations) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    const std::optional<bool> value =
        SplitMixKnownBoolean(operation, integerConstants);
    if (value)
      knownValues[operation.results.front()] = *value;
  }

  GenericRewriter rewriter(module, &useLists);
  for (int operationId : activeOperations) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    if ((operation.name != "arith.ori" &&
         operation.name != "arith.andi") ||
        operation.operands.size() != 2 || operation.results.size() != 1 ||
        operation.blockId < 0)
      continue;
    const int lhs = operation.operands.front();
    const int rhs = operation.operands.back();
    const auto lhsKnown = knownValues.find(lhs);
    const auto rhsKnown = knownValues.find(rhs);
    int replacement = -1;
    if (operation.name == "arith.ori") {
      if (lhsKnown != knownValues.end() && !lhsKnown->second)
        replacement = rhs;
      else if (rhsKnown != knownValues.end() && !rhsKnown->second)
        replacement = lhs;
      else if (lhsKnown != knownValues.end() && lhsKnown->second)
        replacement = lhs;
      else if (rhsKnown != knownValues.end() && rhsKnown->second)
        replacement = rhs;
    } else {
      if (lhsKnown != knownValues.end() && lhsKnown->second)
        replacement = rhs;
      else if (rhsKnown != knownValues.end() && rhsKnown->second)
        replacement = lhs;
      else if (lhsKnown != knownValues.end() && !lhsKnown->second)
        replacement = lhs;
      else if (rhsKnown != knownValues.end() && !rhsKnown->second)
        replacement = rhs;
    }
    if (replacement < 0)
      continue;
    rewriter.replaceAllUses(operation.results.front(), replacement);
    rewriter.removeFromBlock(operation.blockId, operationId);
    activeOperations.erase(operationId);
    return true;
  }
  return false;
}

inline void AnnotateSplitMixOperand(
    GenericModule &module, int value, SplitMixCoreType retainedCore,
    GenericRewriter &rewriter, std::set<int> &activeOperations,
    std::set<int> &visiting,
    const GenericModuleAnalysisIndexes &analysis) {
  if (!visiting.insert(value).second)
    return;
  const int definition = analysis.definingOperationId(value);
  if (definition < 0) {
    visiting.erase(value);
    return;
  }
  const GenericOperation producer =
      module.operations.at(static_cast<size_t>(definition));
  const SplitMixCoreType producerCore = GetSplitMixCoreType(producer);
  if (producerCore == retainedCore) {
    if (producer.blockId < 0) {
      visiting.erase(value);
      return;
    }
    const std::string *type = analysis.valueType(value);
    if (!type)
      throw std::runtime_error(
          "SplitMixKernel: annotated operand has no type");
    const int mark = rewriter.createOperation(
        producer.parentId, producer.regionId, producer.blockId,
        "annotation.mark", {}, {value}, {*type});
    rewriter.insertToBlock(producer.blockId,
                           static_cast<size_t>(producer.ordinal + 1), mark);
    activeOperations.insert(mark);
  } else if (producerCore == SplitMixCoreType::Common) {
    const std::vector<int> inputs = producer.dpsInputs.empty()
                                        ? producer.operands
                                        : producer.dpsInputs;
    for (int operand : inputs)
      AnnotateSplitMixOperand(module, operand, retainedCore, rewriter,
                              activeOperations, visiting, analysis);
  }
  visiting.erase(value);
}

inline void AnnotateSplitMixOperationInputs(
    GenericModule &module, const GenericOperation &operation,
    SplitMixCoreType retainedCore, GenericRewriter &rewriter,
    std::set<int> &activeOperations,
    const GenericModuleAnalysisIndexes &analysis) {
  const std::vector<int> inputs = operation.dpsInputs.empty()
                                      ? operation.operands
                                      : operation.dpsInputs;
  std::set<int> visiting;
  for (int operand : inputs)
    AnnotateSplitMixOperand(module, operand, retainedCore, rewriter,
                            activeOperations, visiting, analysis);

  // annotateOpOperand also propagates through LoopLikeOpInterface yielded
  // values and through both scf.if yields. Those values are not ordinary DPS
  // inputs, but may be the vector producer feeding an erased cube operation.
  const bool isLoopLike = operation.name == "scf.for" ||
                          operation.name == "scf.while" ||
                          operation.name == "scf.forall" ||
                          operation.name == "scf.parallel" ||
                          operation.name == "affine.for" ||
                          operation.name == "affine.parallel";
  if (!isLoopLike && operation.name != "scf.if")
    return;
  for (int regionId : operation.regions) {
    const GenericRegion &region =
        module.regions.at(static_cast<size_t>(regionId));
    for (int blockId : region.blocks) {
      const GenericBlock &block =
          module.blocks.at(static_cast<size_t>(blockId));
      if (block.operations.empty())
        continue;
      const GenericOperation &terminator = module.operations.at(
          static_cast<size_t>(block.operations.back()));
      if (terminator.name != "scf.yield" &&
          terminator.name != "affine.yield" &&
          terminator.name != "scf.condition")
        continue;
      const size_t first = terminator.name == "scf.condition" ? 1 : 0;
      for (size_t index = first; index < terminator.operands.size(); ++index)
        AnnotateSplitMixOperand(module, terminator.operands[index],
                                retainedCore, rewriter, activeOperations,
                                visiting, analysis);
    }
  }
}

inline GenericModule RunSplitMixKernelSelectedProjections(
    GenericModule module,
    const std::vector<std::pair<int, SplitMixCoreType>> &projectionRequests) {
  for (const auto &[functionId, retainedCore] : projectionRequests) {
    if (functionId < 0 ||
        static_cast<size_t>(functionId) >= module.operations.size() ||
        !IsSplitMixFunction(
            module.operations.at(static_cast<size_t>(functionId))))
      throw std::runtime_error(
          "SplitMixKernel: projection request is not a MIX function");
    if (retainedCore == SplitMixCoreType::Common ||
        retainedCore == SplitMixCoreType::Both)
      throw std::runtime_error(
          "SplitMixKernel: projection core must be AIC or AIV");
  }
  // The projection is meaningful only for functions that still carry the
  // MIX core type.  Most corpus inputs are already AIV-only at this point.
  // Detect that cheap no-op case before building the module-wide definition,
  // type and active-operation indexes.
  if (projectionRequests.empty())
    return module;

  // Projection inserts resultless annotation.mark operations and removes
  // operations from blocks, but it does not redefine existing SSA values.
  // Definitions and value types are therefore immutable throughout the
  // projection and can be indexed once for every recursive annotation walk.
  const GenericModuleAnalysisIndexes analysis(
      module, kGenericAnalysisDefinitions | kGenericAnalysisValueTypes);
  PipelineAnalysisContext useLists(module, kGenericAnalysisUsers);
  std::set<int> activeOperations;
  for (const GenericBlock &block : module.blocks)
    activeOperations.insert(block.operations.begin(), block.operations.end());

  bool changed = false;
  std::vector<std::pair<int, SplitMixCoreType>> projectedFunctions;
  for (const auto &[functionId, retainedCore] : projectionRequests) {
    const SplitMixCoreType filteredCore =
        retainedCore == SplitMixCoreType::Vector ? SplitMixCoreType::Cube
                                                 : SplitMixCoreType::Vector;
    GenericOperation &function =
        module.operations.at(static_cast<size_t>(functionId));
    changed = true;
    MarkSplitMixProjectedFunction(function, retainedCore);
    projectedFunctions.emplace_back(functionId, retainedCore);

    std::vector<int> descendants;
    std::function<void(int)> collect = [&](int operationId) {
      const GenericOperation &operation =
          module.operations.at(static_cast<size_t>(operationId));
      for (int regionId : operation.regions)
        for (int blockId :
             module.regions.at(static_cast<size_t>(regionId)).blocks)
          for (int child :
               module.blocks.at(static_cast<size_t>(blockId)).operations) {
            collect(child);
            descendants.push_back(child);
          }
    };
    collect(functionId);

    GenericRewriter rewriter(module, &useLists);
    for (int operationId : descendants) {
      if (activeOperations.count(operationId) == 0)
        continue;
      const GenericOperation operation =
          module.operations.at(static_cast<size_t>(operationId));
      if (GetSplitMixCoreType(operation) != filteredCore)
        continue;
      if (operation.name == "scf.for" && operation.operands.size() >= 2) {
        // filterMixFunc sets upperBound to lowerBound. The greedy rewrite in
        // postProcessVectorFunc then folds that zero-trip loop to its init
        // operands and erases the loop.
        if (operation.results.size() + 3 > operation.operands.size())
          throw std::runtime_error(
              "SplitMixKernel: scf.for result/init operand mismatch");
        for (size_t index = 0; index < operation.results.size(); ++index)
          rewriter.replaceAllUses(operation.results[index],
                                  operation.operands[index + 3]);
        rewriter.removeFromBlock(operation.blockId, operationId);
        activeOperations.erase(operationId);
        continue;
      }
      AnnotateSplitMixOperationInputs(
          module, operation, retainedCore, rewriter,
          activeOperations, analysis);
      if (operation.results.size() > operation.dpsInits.size())
        throw std::runtime_error(
            "SplitMixKernel: CUBE result has no DPS init replacement: " +
            operation.name);
      for (size_t index = 0; index < operation.results.size(); ++index)
        rewriter.replaceAllUses(operation.results[index],
                                operation.dpsInits[index]);
      rewriter.removeFromBlock(operation.blockId, operationId);
      activeOperations.erase(operationId);
    }
  }

  if (!changed)
    return module;

  for (const auto &[functionId, retainedCore] : projectedFunctions) {
    if (retainedCore == SplitMixCoreType::Vector)
      PostProcessSplitMixVectorFunc(module, functionId, activeOperations,
                                    useLists);
    else
      PostProcessSplitMixCubeFunc(module, functionId, activeOperations,
                                  useLists);
  }

  for (const auto &[functionId, retainedCore] : projectedFunctions) {
    (void)retainedCore;
    RunSplitMixOperationFolder(module, functionId, activeOperations, useLists);
  }
  while (FoldSplitMixBooleanOperation(module, activeOperations, useLists)) {
  }

  GenericRewriter rewriter(module, &useLists);
  bool erased = true;
  while (erased) {
    erased = false;
    std::vector<int> erasedOperations;
    const std::vector<int> candidates(activeOperations.rbegin(),
                                      activeOperations.rend());
    for (int operationId : candidates) {
      if (activeOperations.count(operationId) == 0)
        continue;
      const GenericOperation &operation =
          module.operations.at(static_cast<size_t>(operationId));
      if (!IsSplitMixTriviallyDead(module, operation, activeOperations,
                                   useLists))
        continue;
      erasedOperations.push_back(operationId);
      activeOperations.erase(operationId);
      erased = true;
    }
    rewriter.removeManyFromBlocks(erasedOperations);
  }
  RemoveUselessSplitMixMarkOps(module, activeOperations);
  return CompactGenericModule(std::move(module));
}

inline GenericModule RunSplitMixKernelProjection(
    GenericModule module, SplitMixCoreType retainedCore) {
  std::vector<std::pair<int, SplitMixCoreType>> projectionRequests;
  for (const GenericOperation &operation : module.operations)
    if (IsSplitMixFunction(operation))
      projectionRequests.emplace_back(operation.id, retainedCore);
  return RunSplitMixKernelSelectedProjections(std::move(module),
                                               projectionRequests);
}

inline GenericModule RunSplitMixKernelCombinedProjection(GenericModule module) {
  std::vector<int> mixFunctions;
  for (const GenericOperation &operation : module.operations)
    if (IsSplitMixFunction(operation))
      mixFunctions.push_back(operation.id);
  if (mixFunctions.empty())
    return module;

  // The combined projection duplicates the MIX function trees. Reserve the
  // backing tables up front so cloning does not repeatedly relocate the
  // original IR and its string/vector payloads.
  module.operations.reserve(module.operations.size() * 2);
  module.regions.reserve(module.regions.size() * 2);
  module.blocks.reserve(module.blocks.size() * 2);
  GenericRewriter rewriter(module);
  std::vector<std::pair<int, SplitMixCoreType>> projectionRequests;
  projectionRequests.reserve(mixFunctions.size() * 2);
  for (int functionId : mixFunctions) {
    const GenericOperation source =
        module.operations.at(static_cast<size_t>(functionId));
    std::map<int, int> values;
    std::map<int, int> blocks;
    const size_t firstClonedOperation = module.operations.size();
    const int clone = rewriter.cloneOperationTree(
        functionId, source.parentId, source.regionId, source.blockId, values,
        &blocks);
    for (size_t operationId = firstClonedOperation;
         operationId < module.operations.size(); ++operationId)
      for (int &successor : module.operations[operationId].successors) {
        const auto mapped = blocks.find(successor);
        if (mapped != blocks.end())
          successor = mapped->second;
      }
    rewriter.insertToBlock(
        source.blockId,
        static_cast<size_t>(
            module.operations.at(static_cast<size_t>(functionId)).ordinal + 1),
        clone);
    projectionRequests.emplace_back(functionId, SplitMixCoreType::Cube);
    projectionRequests.emplace_back(clone, SplitMixCoreType::Vector);
  }
  return RunSplitMixKernelSelectedProjections(std::move(module),
                                               projectionRequests);
}

inline GenericModule RunSplitMixKernel(GenericModule module) {
  return RunSplitMixKernelProjection(std::move(module),
                                     SplitMixCoreType::Vector);
}

} // namespace cvub

#endif
