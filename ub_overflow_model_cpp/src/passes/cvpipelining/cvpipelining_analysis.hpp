#ifndef CVPIPELINE_UB_MODEL_CPP_CVPIPELINING_ANALYSIS_HPP
#define CVPIPELINE_UB_MODEL_CPP_CVPIPELINING_ANALYSIS_HPP

#include "cvpipelining.hpp"

namespace cvub {

struct CVPipelineWorkItem {
  CVPipelineCoreType core = CVPipelineCoreType::Unknown;
  std::set<int> ops;
  std::vector<std::pair<int, int>> localOutputs;
  std::vector<std::pair<int, size_t>> yieldedOutputs;
  std::vector<int> workspaceOutputs;
  unsigned multibuffer = 0;
};

struct CVPipelineWorkspaceAllocParams {
  unsigned multibuffer = 0;
  int marker = -1;
  int toTensor = -1;
};

struct CVPipelineAnalysisResult {
  bool success = false;
  std::string failureReason;
  int loop = -1;
  unsigned multibuffer = 0;
  std::vector<CVPipelineWorkItem> worklist;
  std::map<int, int> outputMemrefMap;
  std::map<int, CVPipelineWorkspaceAllocParams> workspaceAllocs;
  std::map<int, std::string> atomicEffectMap;
  std::string trailingAtomicEffect;
};

class CVPipelineImplAnalysis {
public:
  CVPipelineImplAnalysis(GenericModule &inputModule, int loop,
                         int multibuffer, bool shouldEnableLazyLoading)
      : module(inputModule), pipelineLoop(loop), numMultibuffer(multibuffer),
        enableLazyLoading(shouldEnableLazyLoading) {
    index();
  }

  CVPipelineAnalysisResult run() {
    CVPipelineAnalysisResult result;
    result.loop = pipelineLoop;
    if (!createWorkItems()) {
      result.failureReason = failureReason;
      return result;
    }
    markOutputs();
    result.success = true;
    result.multibuffer = static_cast<unsigned>(numMultibuffer);
    result.worklist = worklist;
    result.outputMemrefMap = outputMemrefMap;
    result.workspaceAllocs = workspaceAllocs;
    result.atomicEffectMap = atomicEffectMap;
    result.trailingAtomicEffect = trailingAtomicEffect;
    return result;
  }

private:
  void index() {
    for (const GenericBlock &block : module.blocks)
      for (size_t index = 0; index < block.arguments.size(); ++index) {
        valueTypes[block.arguments[index]] = block.argumentTypes[index];
        blockArguments[block.arguments[index]] =
            {block.id, static_cast<int>(index)};
      }
    for (const GenericOperation &operation : module.operations) {
      for (size_t index = 0; index < operation.results.size(); ++index) {
        definitions[operation.results[index]] = operation.id;
        if (index < operation.resultTypes.size())
          valueTypes[operation.results[index]] = operation.resultTypes[index];
      }
      for (int operand : operation.operands)
        users[operand].push_back(operation.id);
    }
    const GenericOperation &loop = module.operations.at(
        static_cast<size_t>(pipelineLoop));
    if (loop.regions.empty())
      return;
    const GenericRegion &region = module.regions.at(
        static_cast<size_t>(loop.regions.front()));
    if (!region.blocks.empty())
      body = region.blocks.front();
  }

  int getContainedParent(int inner) const {
    int current = inner;
    int parent = module.operations.at(static_cast<size_t>(current)).parentId;
    while (parent >= 0 && parent != pipelineLoop) {
      current = parent;
      parent = module.operations.at(static_cast<size_t>(current)).parentId;
    }
    return parent == pipelineLoop ? current : -1;
  }

  int definingContainedParent(int value) const {
    auto definition = definitions.find(value);
    return definition == definitions.end() ? -1
                                           : getContainedParent(definition->second);
  }

  bool isShaped(int value) const {
    auto type = valueTypes.find(value);
    return type != valueTypes.end() &&
           (startsWith(type->second, "tensor<") ||
            startsWith(type->second, "memref<"));
  }

  bool isMemRef(int value) const {
    auto type = valueTypes.find(value);
    return type != valueTypes.end() && startsWith(type->second, "memref<");
  }

  bool isTensor(int value) const {
    auto type = valueTypes.find(value);
    return type != valueTypes.end() && startsWith(type->second, "tensor<");
  }

  int traceValueDef(int value) const {
    auto definition = definitions.find(value);
    if (definition != definitions.end()) {
      const GenericOperation &operation = module.operations.at(
          static_cast<size_t>(definition->second));
      if (operation.name == "tensor.reshape" ||
          operation.name == "tensor.extract_slice" ||
          operation.name == "tensor.collapse_shape" ||
          operation.name == "tensor.expand_shape" ||
          operation.name == "bufferization.to_tensor" ||
          operation.name == "bufferization.to_memref" ||
          operation.name == "memref.cast" ||
          operation.name == "memref.collapse_shape" ||
          operation.name == "memref.expand_shape" ||
          operation.name == "memref.memory_space_cast" ||
          operation.name == "memref.reinterpret_cast" ||
          operation.name == "memref.reshape" ||
          operation.name == "memref.view" ||
          operation.name == "memref.subview") {
        if (!operation.operands.empty())
          return traceValueDef(operation.operands.front());
      }
      if (operation.name == "tensor.insert_slice" && operation.operands.size() > 1)
        return traceValueDef(operation.operands[1]);
      if (operation.name == "scf.for") {
        const GenericOperation *parentYield = loopTerminator(operation.id);
        if (parentYield) {
          auto resultIt = std::find(operation.results.begin(),
                                    operation.results.end(), value);
          const size_t resultIndex =
              resultIt == operation.results.end()
                  ? 0
                  : static_cast<size_t>(resultIt - operation.results.begin());
          if (resultIndex + 3 < operation.operands.size())
            return traceValueDef(operation.operands[resultIndex + 3]);
        }
      }
      return value;
    }

    auto argument = blockArguments.find(value);
    if (argument == blockArguments.end())
      return value;
    const GenericBlock &block =
        module.blocks.at(static_cast<size_t>(argument->second.first));
    const int region = block.regionId;
    if (region < 0)
      return value;
    const int parent =
        module.regions.at(static_cast<size_t>(region)).parentOperation;
    if (parent < 0)
      return value;
    const GenericOperation &parentOp =
        module.operations.at(static_cast<size_t>(parent));
    if (parentOp.name != "scf.for" || argument->second.second == 0)
      return value;
    const size_t initIndex = static_cast<size_t>(argument->second.second + 2);
    if (initIndex < parentOp.operands.size())
      return traceValueDef(parentOp.operands[initIndex]);
    return value;
  }

  int traceAlloc(int value) const {
    const int root = traceValueDef(value);
    auto definition = definitions.find(root);
    if (definition == definitions.end())
      return -1;
    const GenericOperation &operation = module.operations.at(
        static_cast<size_t>(definition->second));
    return operation.name == "memref.alloc" ? operation.id : -1;
  }

  bool isCrossCoreCopy(const GenericOperation &operation) const {
    if (operation.name != "hivm.hir.copy" || operation.operands.size() < 2)
      return false;
    const int allocation = traceAlloc(operation.operands[1]);
    if (allocation < 0)
      return false;
    const GenericOperation &alloc =
        module.operations.at(static_cast<size_t>(allocation));
    return !alloc.resultTypes.empty() &&
           alloc.resultTypes.front().find("#hivm.address_space<cbuf>") !=
               std::string::npos;
  }

  bool isCoreOp(const GenericOperation &operation) const {
    return CVPipelineHasAttribute(operation, "pipeline.cubeonly") ||
           CVPipelineHasAttribute(operation, "pipeline.veconly") ||
           (startsWith(operation.name, "hivm.hir.") &&
            IsDestinationStyleOp(operation.name));
  }

  bool isSeparator(const GenericOperation &operation) const {
    return operation.name == "hivm.hir.fixpipe" ||
           operation.name == "hivm.hir.store" || isCrossCoreCopy(operation);
  }

  const GenericOperation *loopTerminator(int loopId) const {
    const GenericOperation &loop =
        module.operations.at(static_cast<size_t>(loopId));
    if (loop.regions.empty())
      return nullptr;
    const GenericRegion &region =
        module.regions.at(static_cast<size_t>(loop.regions.front()));
    if (region.blocks.empty())
      return nullptr;
    const GenericBlock &block =
        module.blocks.at(static_cast<size_t>(region.blocks.front()));
    if (block.operations.empty())
      return nullptr;
    return &module.operations.at(static_cast<size_t>(block.operations.back()));
  }

  void mapOpToItem(int operation, size_t item) {
    opToWorkItemMap[operation].push_back(item);
    worklist[item].ops.insert(operation);
  }

  void populateDependencies(int separator) {
    std::vector<int> stack = {separator};
    std::set<int> visited;
    while (!stack.empty()) {
      const int operationId = stack.back();
      stack.pop_back();
      if (!visited.insert(operationId).second ||
          !CVPipelineIsDescendant(module, operationId, pipelineLoop))
        continue;
      const GenericOperation &operation = module.operations.at(
          static_cast<size_t>(operationId));
      for (int result : operation.results) {
        if (!isShaped(result))
          continue;
        for (int rawUser : users[result]) {
          const int user = getContainedParent(rawUser);
          if (user < 0 || user == separator)
            continue;
          const std::string &name =
              module.operations.at(static_cast<size_t>(user)).name;
          if (name == "scf.yield" || name == "scf.condition")
            continue;
          stack.push_back(user);
          dependenceMap[user].insert(separator);
        }
      }
    }
  }

  void populateLoopCarriedDependencies() {
    if (body < 0)
      return;
    const GenericBlock &block =
        module.blocks.at(static_cast<size_t>(body));
    if (block.operations.empty())
      return;
    const GenericOperation &yield = module.operations.at(
        static_cast<size_t>(block.operations.back()));
    for (size_t index = 0;
         index < yield.operands.size() && index + 1 < block.arguments.size();
         ++index) {
      if (!isShaped(yield.operands[index]) ||
          !startsWith(valueTypes[yield.operands[index]], "tensor<"))
        continue;
      const int defining = definingContainedParent(yield.operands[index]);
      if (defining < 0)
        continue;
      std::vector<int> stack = users[block.arguments[index + 1]];
      std::set<int> visited;
      while (!stack.empty()) {
        const int operation = getContainedParent(stack.back());
        stack.pop_back();
        if (operation < 0 || operation == defining ||
            !visited.insert(operation).second)
          continue;
        const GenericOperation &record = module.operations.at(
            static_cast<size_t>(operation));
        if (IsDestinationStyleOp(record.name)) {
          loopCarriedDependenceMap[operation].insert(defining);
          continue;
        }
        for (int result : record.results)
          for (int user : users[result])
            if (module.operations.at(static_cast<size_t>(user)).name !=
                "scf.yield")
              stack.push_back(user);
      }
    }
  }

  bool traceOperands(int operand, size_t item, std::vector<int> &stack) {
    if (operand < 0)
      return true;
    int defining = definingContainedParent(operand);
    if (defining >= 0 && worklist[item].ops.count(defining) != 0)
      return true;
    if (defining < 0) {
      auto argument = blockArguments.find(operand);
      if (argument == blockArguments.end() ||
          argument->second.first != body || argument->second.second == 0)
        return true;
      for (int user : users[operand]) {
        const int contained = getContainedParent(user);
        if (contained >= 0 &&
            module.operations.at(static_cast<size_t>(contained)).name ==
                "hivm.hir.debug" &&
            worklist[item].ops.count(contained) == 0)
          stack.push_back(contained);
      }
      if (startsWith(valueTypes[operand], "tensor<"))
        return true;
      const GenericBlock &block = module.blocks.at(static_cast<size_t>(body));
      const GenericOperation &yield = module.operations.at(
          static_cast<size_t>(block.operations.back()));
      const size_t yielded = static_cast<size_t>(argument->second.second - 1);
      if (yielded < yield.operands.size())
        defining = definingContainedParent(yield.operands[yielded]);
    }
    if (defining >= 0 && defining != pipelineLoop && isMemRef(operand))
      memrefDFS(operand, stack);
    if (defining >= 0 && worklist[item].ops.count(defining) == 0)
      stack.push_back(defining);
    return true;
  }

  void memrefDFS(int memref, std::vector<int> &stack) const {
    const int root = traceValueDef(memref);
    std::vector<int> trace;
    auto found = users.find(root);
    if (found != users.end())
      trace = found->second;
    std::set<int> visited;
    while (!trace.empty()) {
      const int operationId = trace.back();
      trace.pop_back();
      if (!visited.insert(operationId).second)
        continue;
      stack.push_back(operationId);
      const GenericOperation &operation =
          module.operations.at(static_cast<size_t>(operationId));
      if (operation.results.size() == 1 && !isMemRef(operation.results.front()))
        continue;
      for (int result : operation.results) {
        auto resultUsers = users.find(result);
        if (resultUsers != users.end())
          trace.insert(trace.end(), resultUsers->second.begin(),
                       resultUsers->second.end());
      }
    }
  }

  bool traceMemrefSubnet(int start, std::vector<int> &stack) {
    const GenericOperation &initial =
        module.operations.at(static_cast<size_t>(start));
    if (std::all_of(initial.operands.begin(), initial.operands.end(),
                    [&](int value) { return isTensor(value); }))
      return true;
    int current = start;
    int targetOperand = initial.operands.size() > 1 ? initial.operands[1] : -1;
    int writer = isTensor(targetOperand) && IsDestinationStyleOp(initial.name)
                     ? start
                     : -1;
    auto defining = definitions.find(targetOperand);
    while (defining != definitions.end()) {
      const int definingOp = defining->second;
      if (!CVPipelineIsDescendant(module, definingOp, pipelineLoop))
        break;
      current = definingOp;
      const std::string &name =
          module.operations.at(static_cast<size_t>(definingOp)).name;
      if (name == "memref.alloc" || name == "memref_ext.alloc_workspace")
        break;
      if (name == "memref.cast" || name == "memref.reinterpret_cast" ||
          name == "memref.memory_space_cast" ||
          name == "memref.collapse_shape" ||
          name == "memref.expand_shape" || name == "memref.subview" ||
          name == "memref.view" || name == "bufferization.to_tensor" ||
          name == "tensor.extract_slice") {
        const GenericOperation &operation =
            module.operations.at(static_cast<size_t>(definingOp));
        if (operation.operands.empty())
          break;
        defining = definitions.find(operation.operands.front());
        continue;
      }
      return fail("unexpected memref op in chain: " + name);
    }

    std::vector<int> trace = {current};
    int toTensor = -1;
    std::set<int> visited;
    while (!trace.empty()) {
      const int definition = trace.back();
      trace.pop_back();
      if (!visited.insert(definition).second)
        continue;
      stack.push_back(definition);
      for (int result : module.operations.at(static_cast<size_t>(definition)).results) {
        for (int user : users[result]) {
          const GenericOperation &use =
              module.operations.at(static_cast<size_t>(user));
          if (IsDestinationStyleOp(use.name)) {
            if (writer >= 0)
              return fail("multiple writers for defined memref");
            writer = user;
            continue;
          }
          if (use.name == "bufferization.to_tensor") {
            if (toTensor >= 0)
              return fail("multiple to_tensor for defined memref");
            toTensor = user;
            stack.push_back(user);
            continue;
          }
          if (use.name == "memref.cast" || use.name == "memref.reinterpret_cast" ||
              use.name == "memref.memory_space_cast" ||
              use.name == "memref.collapse_shape" ||
              use.name == "memref.expand_shape" || use.name == "memref.subview" ||
              use.name == "memref.view")
            trace.push_back(user);
        }
      }
    }
    if (toTensor >= 0 && writer < 0)
      return fail("to_tensor has no dps writer");
    if (toTensor >= 0 && writer >= 0)
      outputMemrefMap[toTensor] = writer;
    return true;
  }

  bool traceDependentOps(size_t item) {
    std::vector<int> stack(worklist[item].ops.begin(), worklist[item].ops.end());
    while (!stack.empty()) {
      const int operationId = stack.back();
      stack.pop_back();
      GenericOperation &operation = module.operations.at(
          static_cast<size_t>(operationId));
      if (isCoreOp(operation)) {
        auto mapped = opToWorkItemMap.find(operationId);
        if (mapped != opToWorkItemMap.end() &&
            worklist[item].ops.count(operationId) == 0 &&
            !(enableLazyLoading && operation.name == "hivm.hir.load"))
          continue;
        if (mapped == opToWorkItemMap.end() &&
            operation.name != "hivm.hir.load")
          return fail("unsatisfied core dependency: " + operation.name);
      } else if (worklist[item].ops.count(operationId)) {
        continue;
      }
      if (operation.parentId != pipelineLoop || operation.name == "scf.yield")
        continue;
      if (operation.name == "bufferization.to_tensor" &&
          !operation.operands.empty()) {
        auto definition = definitions.find(operation.operands.front());
        if (definition != definitions.end() &&
            module.operations.at(static_cast<size_t>(definition->second)).name ==
                "memref_ext.alloc_workspace")
          continue;
      }
      mapOpToItem(operationId, item);
      toBePipelined.erase(operationId);
      for (int result : operation.results)
        for (int user : users[result]) {
          const GenericOperation &use =
              module.operations.at(static_cast<size_t>(user));
          if (use.name == "annotation.mark" || use.name == "hivm.hir.debug")
            mapOpToItem(getContainedParent(user), item);
        }
      if (operation.name == "hivm.hir.load" ||
          operation.name == "hivm.hir.fixpipe" || isCrossCoreCopy(operation)) {
        if (!traceMemrefSubnet(operationId, stack))
          return false;
        for (int operand : operation.operands)
          if (!traceOperands(operand, item, stack))
            return false;
        continue;
      }
      if (operation.name == "bufferization.to_tensor" &&
          !operation.operands.empty()) {
        for (int user : users[operation.operands.front()]) {
          const int contained = getContainedParent(user);
          if (contained < 0 || contained == operationId)
            continue;
          const GenericOperation &writer = module.operations.at(
              static_cast<size_t>(contained));
          if (IsDestinationStyleOp(writer.name))
            stack.push_back(contained);
        }
      }
      for (int operand : operation.operands)
        if (!traceOperands(operand, item, stack))
          return false;
      for (const GenericOperation &nested : module.operations)
        if (nested.id != operationId &&
            CVPipelineIsDescendant(module, nested.id, operationId))
          for (int operand : nested.operands)
            if (!traceOperands(operand, item, stack))
              return false;
    }
    return true;
  }

  std::vector<int> extractAvailableOps(CVPipelineCoreType &core) {
    std::vector<int> available;
    const GenericBlock &block = module.blocks.at(static_cast<size_t>(body));
    for (int operationId : block.operations) {
      const GenericOperation &operation = module.operations.at(
          static_cast<size_t>(operationId));
      if (opToWorkItemMap.count(operationId))
        continue;
      CVPipelineCoreType maybeCore =
          CVPipelineHasAttribute(operation, "pipeline.cubeonly")
              ? CVPipelineCoreType::Cube
              : CVPipelineHasAttribute(operation, "pipeline.veconly")
                    ? CVPipelineCoreType::Vector
                    : CVPipelineCoreType::Unknown;
      if (maybeCore == CVPipelineCoreType::Unknown) {
        if (!isCoreOp(operation) || operation.name == "hivm.hir.load")
          continue;
        maybeCore = CVPipelineQueryCoreType(operation);
        if (maybeCore != CVPipelineCoreType::Vector &&
            isCrossCoreCopy(operation))
          maybeCore = CVPipelineCoreType::Vector;
      }
      if (maybeCore != CVPipelineCoreType::Cube &&
          maybeCore != CVPipelineCoreType::Vector)
        return {};
      if ((maybeCore == CVPipelineCoreType::Vector &&
           core == CVPipelineCoreType::Cube) ||
          (maybeCore == CVPipelineCoreType::Cube &&
           core == CVPipelineCoreType::Vector))
        continue;
      core = maybeCore;
      if (dependenceMap[operationId].empty())
        available.push_back(operationId);
    }
    std::set<int> candidates(available.begin(), available.end());
    std::set<int> deferred;
    for (int operation : available) {
      auto carried = loopCarriedDependenceMap.find(operation);
      if (carried == loopCarriedDependenceMap.end())
        continue;
      if (!std::all_of(carried->second.begin(), carried->second.end(),
                       [&](int dependency) {
                         return candidates.count(dependency) != 0;
                       }))
        deferred.insert(operation);
    }
    std::vector<int> stack(deferred.begin(), deferred.end());
    while (!stack.empty()) {
      const int operation = stack.back();
      stack.pop_back();
      if (candidates.count(operation) != 0)
        deferred.insert(operation);
      for (int result : module.operations.at(static_cast<size_t>(operation)).results) {
        auto found = users.find(result);
        if (found != users.end())
          stack.insert(stack.end(), found->second.begin(), found->second.end());
      }
    }
    available.erase(std::remove_if(available.begin(), available.end(),
                                   [&](int operation) {
                                     return deferred.count(operation) != 0;
                                   }),
                    available.end());
    return available;
  }

  bool createWorkItems() {
    if (body < 0)
      return fail("missing loop body");
    int multibuffer = numMultibuffer > 1 ? numMultibuffer : 2;
    const GenericBlock &block = module.blocks.at(static_cast<size_t>(body));
    collectAtomicEffects(block);
    for (int operationId : block.operations) {
      GenericOperation &operation = module.operations.at(
          static_cast<size_t>(operationId));
      if (isCoreOp(operation))
        toBePipelined.insert(operationId);
      if (isSeparator(operation))
        separators.push_back(operationId);
      else if (operation.name == "annotation.mark") {
        if (numMultibuffer == -1) {
          const int marked = operation.operands.empty() ? -1 : operation.operands.front();
          const int markMultibuffer = getMultibufferCount(operation);
          if (markMultibuffer != -1) {
            if (multibuffer <= 2)
              multibuffer = markMultibuffer;
            else if (multibuffer != markMultibuffer)
              multibuffer = std::min(multibuffer, markMultibuffer);
          }
          if (marked >= 0)
            markWorkspaceOps(operationId, static_cast<unsigned>(multibuffer));
        } else {
          markWorkspaceOps(operationId, static_cast<unsigned>(multibuffer));
        }
      } else if (operation.name == "bufferization.to_tensor") {
        if (!markWorkspaceOps(operationId, static_cast<unsigned>(multibuffer)))
          return false;
      }
      else if (!operation.regions.empty() &&
               CVPipelineIllegalRegionedOp(module, operation))
        return fail("illegal regioned operation: " + operation.name);
    }
    if (multibuffer < 2)
      return fail("multibuffer count is less than two");
    if (numMultibuffer < 1)
      numMultibuffer = multibuffer;
    for (int separator : separators)
      populateDependencies(separator);
    populateLoopCarriedDependencies();

    CVPipelineCoreType core = CVPipelineCoreType::Unknown;
    while (true) {
      std::vector<int> available = extractAvailableOps(core);
      if (core == CVPipelineCoreType::Unknown)
        return fail("core type could not be inferred");
      if (available.empty())
        break;
      CVPipelineWorkItem item;
      item.core = core;
      item.multibuffer = static_cast<unsigned>(numMultibuffer);
      worklist.push_back(std::move(item));
      const size_t itemIndex = worklist.size() - 1;
      for (int operation : available)
        mapOpToItem(operation, itemIndex);
      if (!traceDependentOps(itemIndex))
        return false;
      for (auto &[operation, dependencies] : dependenceMap)
        for (int processed : available)
          dependencies.erase(processed);
      core = core == CVPipelineCoreType::Vector ? CVPipelineCoreType::Cube
                                                : CVPipelineCoreType::Vector;
    }
    if (!toBePipelined.empty()) {
      std::string remaining;
      for (int operation : toBePipelined) {
        if (!remaining.empty())
          remaining += ",";
        remaining += std::to_string(operation) + ":" +
                     module.operations.at(static_cast<size_t>(operation)).name;
      }
      return fail("unassigned core operations remain: " + remaining);
    }
    if (worklist.size() < 2)
      return fail("fewer than two work items");
    if (!absorbMergerOpsIntoWorkItems())
      return false;
    return true;
  }

  bool fail(std::string reason) {
    failureReason = std::move(reason);
    return false;
  }

  void markOutputs() {
    const GenericBlock &block = module.blocks.at(static_cast<size_t>(body));
    const GenericOperation &yield = module.operations.at(
        static_cast<size_t>(block.operations.back()));
    for (size_t itemIndex = 0; itemIndex < worklist.size(); ++itemIndex) {
      CVPipelineWorkItem &item = worklist[itemIndex];
      for (int operationId : item.ops) {
        const GenericOperation &operation = module.operations.at(
            static_cast<size_t>(operationId));
        bool workspaceOutput = false;
        if ((operation.name == "hivm.hir.store" ||
             operation.name == "hivm.hir.fixpipe") &&
            !operation.dpsInits.empty()) {
          if (getAllocWorkspace(operation.dpsInits.front()) >= 0) {
            item.workspaceOutputs.push_back(operationId);
            workspaceOutput = true;
          }
        }
        if (workspaceOutput || operation.name == "tensor.empty")
          continue;
        if (enableLazyLoading && operation.name == "bufferization.to_tensor") {
          auto mapped = outputMemrefMap.find(operation.id);
          if (mapped != outputMemrefMap.end() &&
              module.operations.at(static_cast<size_t>(mapped->second)).name ==
                  "hivm.hir.load")
            continue;
        }
        for (int result : operation.results) {
          auto yielded = std::find(yield.operands.begin(), yield.operands.end(),
                                   result);
          if (yielded != yield.operands.end()) {
            item.yieldedOutputs.push_back(
                {result, static_cast<size_t>(yielded - yield.operands.begin())});
            continue;
          }
          if (!startsWith(valueTypes[result], "tensor<"))
            continue;
          for (int user : users[result]) {
            const int contained = getContainedParent(user);
            auto mapped = opToWorkItemMap.find(contained);
            if (mapped != opToWorkItemMap.end() &&
                std::find(mapped->second.begin(), mapped->second.end(), itemIndex) ==
                    mapped->second.end()) {
              item.localOutputs.push_back({result, -1});
              break;
            }
          }
        }
      }
      std::sort(item.yieldedOutputs.begin(), item.yieldedOutputs.end(),
                [](const auto &lhs, const auto &rhs) {
                  return lhs.second < rhs.second;
                });
    }
  }

  int getMultibufferCount(const GenericOperation &marker) const {
    const std::string dict = marker.properties + marker.attributes;
    const size_t key = dict.find("hivm.multi_buffer");
    if (key == std::string::npos)
      return -1;
    const size_t equal = dict.find('=', key);
    if (equal == std::string::npos)
      return -1;
    size_t begin = equal + 1;
    while (begin < dict.size() &&
           !std::isdigit(static_cast<unsigned char>(dict[begin])) &&
           dict[begin] != '-')
      ++begin;
    if (begin == dict.size())
      return -1;
    size_t end = begin + 1;
    while (end < dict.size() &&
           std::isdigit(static_cast<unsigned char>(dict[end])))
      ++end;
    return std::stoi(dict.substr(begin, end - begin));
  }

  int getAllocWorkspace(int value) const {
    auto definition = definitions.find(value);
    if (definition == definitions.end())
      return -1;
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(definition->second));
    if (operation.name == "memref_ext.alloc_workspace")
      return operation.id;
    if ((operation.name == "bufferization.to_tensor" ||
         (IsDestinationStyleOp(operation.name) && !operation.dpsInits.empty())) &&
        !operation.operands.empty())
      return getAllocWorkspace(operation.name == "bufferization.to_tensor"
                                   ? operation.operands.front()
                                   : operation.dpsInits.front());
    return -1;
  }

  bool markWorkspaceOps(int operationId, unsigned multibuffer) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    if (operation.name == "annotation.mark") {
      if (operation.operands.empty())
        return true;
      auto allocation = definitions.find(operation.operands.front());
      if (allocation == definitions.end() ||
          module.operations.at(static_cast<size_t>(allocation->second)).name !=
              "memref_ext.alloc_workspace")
        return true;
      CVPipelineWorkspaceAllocParams &params =
          workspaceAllocs[allocation->second];
      params.multibuffer = multibuffer;
      params.marker = operationId;
      return true;
    }
    if (operation.name != "bufferization.to_tensor" || operation.operands.empty())
      return true;
    auto allocation = definitions.find(operation.operands.front());
    if (allocation == definitions.end() ||
        module.operations.at(static_cast<size_t>(allocation->second)).name !=
            "memref_ext.alloc_workspace")
      return true;
    if (users[operation.results.front()].size() != 1 ||
        users[operation.operands.front()].size() != 2)
      return fail("workspace alloc/to_tensor has unexpected users");
    CVPipelineWorkspaceAllocParams &params =
        workspaceAllocs[allocation->second];
    params.toTensor = operationId;
    return true;
  }

  void collectAtomicEffects(const GenericBlock &block) {
    std::string current;
    for (int operationId : block.operations) {
      const GenericOperation &operation =
          module.operations.at(static_cast<size_t>(operationId));
      if (operation.name == "hivm.hir.set_atomic") {
        if ((operation.properties + operation.attributes).find("NONE") ==
            std::string::npos)
          current = operation.properties + operation.attributes;
        else
          current.clear();
        continue;
      }
      if (!current.empty() &&
          (operation.name == "hivm.hir.fixpipe" ||
           operation.name == "hivm.hir.store"))
        atomicEffectMap[operationId] = current;
    }
    trailingAtomicEffect = current;
  }

  bool absorbMergerOpsIntoWorkItems() {
    const GenericBlock &block = module.blocks.at(static_cast<size_t>(body));
    if (block.operations.empty())
      return true;
    const GenericOperation &terminator =
        module.operations.at(static_cast<size_t>(block.operations.back()));
    for (int yieldOperand : terminator.operands) {
      auto rootDefinition = definitions.find(yieldOperand);
      if (rootDefinition == definitions.end())
        continue;
      const int root = getContainedParent(rootDefinition->second);
      if (root < 0 || opToWorkItemMap.count(root))
        continue;
      std::set<int> chain;
      std::set<size_t> producers;
      std::vector<int> stack = {root};
      while (!stack.empty()) {
        const int current = stack.back();
        stack.pop_back();
        if (!chain.insert(current).second)
          continue;
        const GenericOperation &operation =
            module.operations.at(static_cast<size_t>(current));
        for (int operand : operation.operands) {
          auto definition = definitions.find(operand);
          if (definition == definitions.end())
            continue;
          const int contained = getContainedParent(definition->second);
          if (contained < 0)
            continue;
          auto mapped = opToWorkItemMap.find(contained);
          if (mapped != opToWorkItemMap.end()) {
            producers.insert(mapped->second.begin(), mapped->second.end());
            continue;
          }
          stack.push_back(contained);
        }
      }
      if (producers.size() != 1)
        return fail("cannot absorb merger chain into a single work item");
      const size_t item = *producers.begin();
      for (int operation : chain)
        mapOpToItem(operation, item);
    }

    for (size_t itemIndex = 0; itemIndex < worklist.size(); ++itemIndex) {
      std::set<int> loadedCounters;
      for (int operationId : worklist[itemIndex].ops) {
        const GenericOperation &operation =
            module.operations.at(static_cast<size_t>(operationId));
        if (operation.name != "memref.load" && operation.name != "hivm.hir.load")
          continue;
        if (operation.operands.empty())
          continue;
        auto definition = definitions.find(traceValueDef(operation.operands.front()));
        if (definition == definitions.end())
          continue;
        const GenericOperation &alloc =
            module.operations.at(static_cast<size_t>(definition->second));
        if (alloc.name == "memref.alloca" &&
            (alloc.properties + alloc.attributes)
                    .find("normalize_matmul_counter") != std::string::npos)
          loadedCounters.insert(operation.operands.front());
      }
      for (int operationId : block.operations) {
        GenericOperation &operation =
            module.operations.at(static_cast<size_t>(operationId));
        if (operation.name != "memref.store" || operation.operands.size() < 2 ||
            loadedCounters.count(operation.operands[1]) == 0 ||
            opToWorkItemMap.count(operationId))
          continue;
        auto inc = definitions.find(operation.operands.front());
        if (inc != definitions.end() &&
            module.operations.at(static_cast<size_t>(inc->second)).name ==
                "arith.addi" &&
            opToWorkItemMap.count(inc->second) == 0)
          mapOpToItem(inc->second, itemIndex);
        mapOpToItem(operationId, itemIndex);
      }
    }
    return true;
  }

  GenericModule &module;
  int pipelineLoop = -1;
  int numMultibuffer = -1;
  bool enableLazyLoading = false;
  int body = -1;
  std::map<int, std::string> valueTypes;
  std::map<int, int> definitions;
  std::map<int, std::pair<int, int>> blockArguments;
  std::map<int, std::vector<int>> users;
  std::vector<int> separators;
  std::set<int> toBePipelined;
  std::map<int, std::set<int>> dependenceMap;
  std::map<int, std::set<int>> loopCarriedDependenceMap;
  std::map<int, std::vector<size_t>> opToWorkItemMap;
  std::vector<CVPipelineWorkItem> worklist;
  std::map<int, int> outputMemrefMap;
  std::map<int, CVPipelineWorkspaceAllocParams> workspaceAllocs;
  std::map<int, std::string> atomicEffectMap;
  std::string trailingAtomicEffect;
  std::string failureReason;
};

inline std::string SerializeCVPipelineAnalysis(
    const CVPipelineAnalysisResult &analysis) {
  std::ostringstream output;
  output << "CVPIPELINE_ANALYSIS\t1\nSTATUS\t"
         << (analysis.success ? "success" : "failure") << '\n';
  if (!analysis.success)
    output << "REASON\t" << analysis.failureReason << '\n';
  for (size_t index = 0; index < analysis.worklist.size(); ++index) {
    const CVPipelineWorkItem &item = analysis.worklist[index];
    output << "WORK_ITEM\t" << index << '\t'
           << (item.core == CVPipelineCoreType::Cube ? "CUBE" : "VECTOR")
           << '\t' << item.multibuffer;
    for (int operation : item.ops)
      output << '\t' << operation;
    output << '\n';
    for (const auto &[value, expanded] : item.localOutputs) {
      (void)expanded;
      output << "LOCAL_OUTPUT\t" << index << '\t' << value << '\n';
    }
    for (const auto &[value, ordinal] : item.yieldedOutputs)
      output << "YIELDED_OUTPUT\t" << index << '\t' << value << '\t'
             << ordinal << '\n';
    for (int operation : item.workspaceOutputs)
      output << "WORKSPACE_OUTPUT\t" << index << '\t' << operation << '\n';
  }
  for (const auto &[toTensor, writer] : analysis.outputMemrefMap)
    output << "OUTPUT_MEMREF_MAP\t" << toTensor << '\t' << writer << '\n';
  for (const auto &[allocation, info] : analysis.workspaceAllocs)
    output << "WORKSPACE_ALLOC\t" << allocation << '\t' << info.multibuffer
           << '\t' << info.marker << '\t' << info.toTensor << '\n';
  for (const auto &[operation, effect] : analysis.atomicEffectMap)
    output << "ATOMIC_EFFECT\t" << operation << '\t' << HexEncode(effect)
           << '\n';
  if (!analysis.trailingAtomicEffect.empty())
    output << "TRAILING_ATOMIC_EFFECT\t"
           << HexEncode(analysis.trailingAtomicEffect) << '\n';
  return output.str();
}

} // namespace cvub

#endif
