#ifndef CVPIPELINE_UB_MODEL_CPP_CVPIPELINING_REWRITE_HPP
#define CVPIPELINE_UB_MODEL_CPP_CVPIPELINING_REWRITE_HPP

#include "cvpipelining_analysis.hpp"

namespace cvub {

inline std::string CVPipelineExpandShapedType(const std::string &type,
                                              unsigned count) {
  const size_t open = type.find('<');
  if (open == std::string::npos)
    throw std::runtime_error("CVPipelining: expected shaped type");
  return type.substr(0, open + 1) + std::to_string(count) + "x" +
         type.substr(open + 1);
}

inline bool CVPipelineIsTensorType(const std::string &type) {
  return startsWith(type, "tensor<");
}

inline bool CVPipelineIsMemRefType(const std::string &type) {
  return startsWith(type, "memref<");
}

inline std::string CVPipelineAddressSpaceSuffix(const std::string &type) {
  const size_t memory = type.find("#hivm.address_space<");
  if (memory == std::string::npos)
    return {};
  const size_t end = type.find('>', memory);
  if (end == std::string::npos)
    return {};
  return type.substr(memory, end - memory + 1);
}

inline std::string CVPipelineFormatMemRefType(
    const std::vector<std::optional<int64_t>> &shape,
    const std::string &elementType, const std::string &memorySpace = {},
    const std::vector<std::optional<int64_t>> *strides = nullptr) {
  std::ostringstream output;
  output << "memref<";
  for (const std::optional<int64_t> &extent : shape)
    output << (extent ? std::to_string(*extent) : "?") << 'x';
  output << elementType;
  if (strides) {
    output << ", strided<[";
    for (size_t axis = 0; axis < strides->size(); ++axis) {
      if (axis)
        output << ", ";
      output << ((*strides)[axis] ? std::to_string(*(*strides)[axis]) : "?");
    }
    output << "], offset: ?>";
  }
  if (!memorySpace.empty())
    output << ", " << memorySpace;
  output << ">";
  return output.str();
}

inline std::string CVPipelineExpandLocalMemRefType(const std::string &type,
                                                   unsigned count) {
  const std::optional<MemRefTypeModel> parsed = ParseMemRefType(type);
  if (!parsed)
    throw std::runtime_error("CVPipelining: expected memref local output");
  std::vector<std::optional<int64_t>> shape;
  shape.push_back(static_cast<int64_t>(count));
  shape.insert(shape.end(), parsed->shape.begin(), parsed->shape.end());
  return CVPipelineFormatMemRefType(shape, parsed->elementType,
                                    CVPipelineAddressSpaceSuffix(type));
}

inline std::string CVPipelineMemRefWithoutMemorySpace(
    const std::string &type) {
  const std::optional<MemRefTypeModel> parsed = ParseMemRefType(type);
  if (!parsed)
    throw std::runtime_error("CVPipelining: expected memref type");
  return CVPipelineFormatMemRefType(parsed->shape, parsed->elementType, {},
                                    parsed->hasStridedLayout
                                        ? &parsed->strides
                                        : nullptr);
}

inline std::string CVPipelineTensorFromMemRefType(const std::string &type) {
  const std::optional<MemRefTypeModel> parsed = ParseMemRefType(type);
  if (!parsed)
    throw std::runtime_error("CVPipelining: expected memref type");
  std::ostringstream output;
  output << "tensor<";
  for (const std::optional<int64_t> &extent : parsed->shape)
    output << (extent ? std::to_string(*extent) : "?") << 'x';
  output << parsed->elementType << ">";
  return output.str();
}

inline std::string CVPipelineElementType(const std::string &type) {
  const std::optional<MemRefTypeModel> parsed = ParseMemRefType(type);
  if (parsed)
    return parsed->elementType;
  if (startsWith(type, "tensor<")) {
    const size_t close = type.rfind('>');
    const std::string body = type.substr(7, close - 7);
    const size_t split = body.rfind('x');
    return split == std::string::npos ? body : body.substr(split + 1);
  }
  return {};
}

inline std::string CVPipelineSubviewType(const std::string &expandedType) {
  const std::optional<MemRefTypeModel> parsed = ParseMemRefType(expandedType);
  if (!parsed || parsed->shape.size() < 2)
    throw std::runtime_error("CVPipelining: malformed expanded workspace");
  std::ostringstream output;
  output << "memref<1x";
  for (size_t axis = 1; axis < parsed->shape.size(); ++axis) {
    if (parsed->shape[axis])
      output << *parsed->shape[axis];
    else
      output << '?';
    output << 'x';
  }
  output << parsed->elementType << ", strided<[";
  for (size_t axis = 0; axis < parsed->strides.size(); ++axis) {
    if (axis)
      output << ", ";
    if (parsed->strides[axis])
      output << *parsed->strides[axis];
    else
      output << '?';
  }
  const size_t memory = expandedType.find("#hivm.address_space<");
  if (memory != std::string::npos) {
    const size_t end = expandedType.find('>', memory);
    if (end != std::string::npos)
      output << "], offset: ?>, "
             << expandedType.substr(memory, end - memory + 1) << ">";
    else
      output << "], offset: ?>>";
  } else {
    output << "], offset: ?>>";
  }
  return output.str();
}

inline std::string CVPipelineCollapsedWorkspaceType(
    const std::string &expandedType) {
  const std::optional<MemRefTypeModel> parsed = ParseMemRefType(expandedType);
  if (!parsed || parsed->shape.size() < 2)
    throw std::runtime_error("CVPipelining: malformed workspace collapse");
  std::ostringstream output;
  output << "memref<";
  for (size_t axis = 1; axis < parsed->shape.size(); ++axis) {
    if (parsed->shape[axis])
      output << *parsed->shape[axis];
    else
      output << '?';
    output << 'x';
  }
  output << parsed->elementType << ", strided<[";
  for (size_t axis = 1; axis < parsed->strides.size(); ++axis) {
    if (axis > 1)
      output << ", ";
    if (parsed->strides[axis])
      output << *parsed->strides[axis];
    else
      output << '?';
  }
  const size_t memory = expandedType.find("#hivm.address_space<");
  if (memory != std::string::npos) {
    const size_t end = expandedType.find('>', memory);
    if (end != std::string::npos)
      output << "], offset: ?>, "
             << expandedType.substr(memory, end - memory + 1) << ">";
    else
      output << "], offset: ?>>";
  } else {
    output << "], offset: ?>>";
  }
  return output.str();
}

inline std::string CVPipelineWorkspaceSubviewAttributes(
    const std::string &expandedType) {
  const std::optional<MemRefTypeModel> parsed = ParseMemRefType(expandedType);
  if (!parsed || parsed->shape.size() < 2)
    throw std::runtime_error("CVPipelining: malformed workspace subview");
  std::ostringstream offsets, sizes, strides;
  offsets << "array<i64: -9223372036854775808";
  sizes << "array<i64: 1";
  strides << "array<i64: 1";
  for (size_t axis = 1; axis < parsed->shape.size(); ++axis) {
    offsets << ", 0";
    sizes << ", " << (parsed->shape[axis] ? *parsed->shape[axis] : -1);
    strides << ", 1";
  }
  offsets << ">";
  sizes << ">";
  strides << ">";
  return "{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = " +
         offsets.str() + ", static_sizes = " + sizes.str() +
         ", static_strides = " + strides.str() + "}";
}

inline std::string
CVPipelineLocalSubviewAttributes(const std::string &targetType) {
  const std::optional<MemRefTypeModel> target = ParseMemRefType(targetType);
  if (!target)
    throw std::runtime_error("CVPipelining: malformed local subview target");
  std::ostringstream offsets, sizes, strides;
  offsets << "array<i64: -9223372036854775808";
  sizes << "array<i64: 1";
  strides << "array<i64: 1";
  for (size_t axis = 0; axis < target->shape.size(); ++axis) {
    offsets << ", 0";
    sizes << ", " << (target->shape[axis] ? *target->shape[axis] : -1);
    strides << ", 1";
  }
  offsets << ">";
  sizes << ">";
  strides << ">";
  return "{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = " +
         offsets.str() + ", static_sizes = " + sizes.str() +
         ", static_strides = " + strides.str() + "}";
}

inline std::string CVPipelineLocalSubviewType(const std::string &expandedType,
                                              const std::string &targetType) {
  const std::optional<MemRefTypeModel> target = ParseMemRefType(targetType);
  if (!target)
    throw std::runtime_error("CVPipelining: malformed local subview target");
  return CVPipelineFormatMemRefType(target->shape, target->elementType,
                                    CVPipelineAddressSpaceSuffix(expandedType),
                                    &target->strides);
}

inline std::string
CVPipelineWorkspaceCollapseReassociation(const std::string &expandedType) {
  const std::optional<MemRefTypeModel> parsed = ParseMemRefType(expandedType);
  if (!parsed || parsed->shape.size() < 2)
    throw std::runtime_error("CVPipelining: malformed workspace collapse");
  std::ostringstream reassociation;
  reassociation << "{reassociation = [[0, 1]";
  for (size_t axis = 2; axis < parsed->shape.size(); ++axis)
    reassociation << ", [" << axis << "]";
  reassociation << "]}";
  return reassociation.str();
}

class CVPipelineLoopRewriter {
public:
  CVPipelineLoopRewriter(GenericModule &inputModule,
                         CVPipelineAnalysisResult analysisResult)
      : module(inputModule), analysis(std::move(analysisResult)),
        rewriter(inputModule) {
    index();
  }

  bool rewrite() {
    if (!analysis.success || analysis.worklist.empty())
      return false;
    const GenericOperation oldLoop =
        module.operations.at(static_cast<size_t>(analysis.loop));
    if (oldLoop.name != "scf.for" || oldLoop.operands.size() < 3 ||
        oldLoop.regions.size() != 1)
      return false;
    const int parentBlock = oldLoop.blockId;
    const int parentRegion = oldLoop.regionId;
    const int parentOperation = oldLoop.parentId;
    const GenericBlock parentSnapshot =
        module.blocks.at(static_cast<size_t>(parentBlock));
    const auto position = std::find(parentSnapshot.operations.begin(),
                                    parentSnapshot.operations.end(), analysis.loop);
    if (position == parentSnapshot.operations.end())
      return false;
    const size_t insertion = static_cast<size_t>(
        position - parentSnapshot.operations.begin());
    std::vector<int> generated;
    auto emitParent = [&](const std::string &name,
                          const std::vector<std::string> &resultTypes,
                          const std::vector<int> &operands,
                          const std::vector<std::string> &operandTypes,
                          const std::string &attributes = "{}") {
      const int operation = rewriter.createOperation(
          parentOperation, parentRegion, parentBlock, name, resultTypes,
          operands, operandTypes, "", attributes);
      generated.push_back(operation);
      return operation;
    };

    std::map<int, int> workspaceAllocations;
    std::map<int, std::string> workspaceTypes;
    for (const auto &[allocation, info] : analysis.workspaceAllocs) {
      const GenericOperation &source =
          module.operations.at(static_cast<size_t>(allocation));
      if (source.resultTypes.empty())
        continue;
      const std::string expanded =
          CVPipelineExpandShapedType(source.resultTypes.front(),
                                     analysis.multibuffer);
      const int created = emitParent(source.name, {expanded}, source.operands,
                                     source.operandTypes, source.attributes);
      workspaceAllocations[allocation] =
          module.operations.at(static_cast<size_t>(created)).results.front();
      workspaceTypes[allocation] = expanded;
      if (info.marker >= 0 &&
          static_cast<size_t>(info.marker) < module.operations.size()) {
        GenericOperation &marker =
            module.operations.at(static_cast<size_t>(info.marker));
        if (!marker.operands.empty())
          marker.operands.front() = workspaceAllocations[allocation];
        marker.operandTypes = {expanded};
        marker.attributes =
            removeDictionaryEntry(marker.attributes, "hivm.multi_buffer");
      }
    }

    const std::string stepType = oldLoop.operandTypes.at(2);
    const int unroll = emitParent("arith.constant", {stepType}, {}, {},
                                  "{value = " +
                                      std::to_string(analysis.multibuffer) +
                                      " : " + stepType + "}");
    const int newStep = emitParent(
        "arith.muli", {stepType},
        {oldLoop.operands[2], result(unroll)}, {stepType, stepType},
        "{overflowFlags = #arith.overflow<none>}");
    const int indexOne = emitParent("arith.constant", {"index"}, {}, {},
                                    "{value = 1 : index}");
    const int indexZero = emitParent("arith.constant", {"index"}, {}, {},
                                     "{value = 0 : index}");
    const int pipelineIters = emitParent(
        "arith.constant", {"index"}, {}, {},
        "{value = " + std::to_string(analysis.multibuffer) + " : index}");

    std::vector<int> outerOperands = oldLoop.operands;
    outerOperands[2] = result(newStep);
    const int newLoop = emitParent("scf.for", oldLoop.resultTypes,
                                   outerOperands, oldLoop.operandTypes,
                                   oldLoop.attributes);
    const int outerRegion = rewriter.createRegion(newLoop);
    std::vector<std::string> outerArgumentTypes = {stepType};
    outerArgumentTypes.insert(outerArgumentTypes.end(),
                              oldLoop.operandTypes.begin() + 3,
                              oldLoop.operandTypes.end());
    const int outerBlock = rewriter.createBlock(outerRegion, outerArgumentTypes);
    const GenericBlock outerBody =
        module.blocks.at(static_cast<size_t>(outerBlock));
    indexBlockArguments(outerBody);

    std::map<int, int> globalValues;
    const GenericBlock oldBody = oldLoopBody();
    globalValues[oldBody.arguments.front()] = outerBody.arguments.front();
    for (size_t index = 1;
         index < oldBody.arguments.size() && index < outerBody.arguments.size();
         ++index)
      globalValues[oldBody.arguments[index]] = outerBody.arguments[index];

    std::map<int, int> expandedLocalOutputs;
    for (const CVPipelineWorkItem &item : analysis.worklist)
      for (const auto &[value, unused] : item.localOutputs) {
        (void)unused;
        if (expandedLocalOutputs.count(value))
          continue;
        const int defining = definingOperation(value);
        if (defining < 0)
          throw std::runtime_error("CVPipelining: local output has no def");
        const GenericOperation &source =
            module.operations.at(static_cast<size_t>(defining));
        if (source.dpsInits.empty())
          throw std::runtime_error("CVPipelining: local output is not DPS");
        const int init = source.dpsInits.front();
        const int initDef = definingOperation(init);
        if (initDef >= 0 &&
            module.operations.at(static_cast<size_t>(initDef)).name ==
                "tensor.empty") {
          const std::string expanded =
              CVPipelineExpandShapedType(valueType(value, "expanded output"),
                                         analysis.multibuffer);
          const int empty = createInBlock(newLoop, outerRegion, outerBlock,
                                          "tensor.empty", {expanded}, {}, {});
          expandedLocalOutputs[value] = result(empty);
          continue;
        }
        const int allocation = traceAlloc(init);
        if (allocation < 0)
          throw std::runtime_error(
              "CVPipelining: expected alloc from local output init");
        const GenericOperation &alloc =
            module.operations.at(static_cast<size_t>(allocation));
        const std::string expanded =
            CVPipelineExpandLocalMemRefType(alloc.resultTypes.front(),
                                            analysis.multibuffer);
        const int expandedAlloc = createInBlock(
            newLoop, outerRegion, outerBlock, "memref.alloc", {expanded},
            alloc.operands, alloc.operandTypes, alloc.attributes);
        expandedLocalOutputs[value] = result(expandedAlloc);
      }

    int upper = oldLoop.operands[1];
    int outerIV = outerBody.arguments.front();
    int originalStep = oldLoop.operands[2];
    if (stepType != "index") {
      upper = result(createInBlock(newLoop, outerRegion, outerBlock,
                                   "arith.index_cast", {"index"}, {upper},
                                   {stepType}));
      outerIV = result(createInBlock(newLoop, outerRegion, outerBlock,
                                     "arith.index_cast", {"index"}, {outerIV},
                                     {stepType}));
      originalStep = result(createInBlock(
          newLoop, outerRegion, outerBlock, "arith.index_cast", {"index"},
          {originalStep}, {stepType}));
    }
    const int capped = createInBlock(
        newLoop, outerRegion, outerBlock, "affine.apply", {"index"},
        {upper, outerIV, originalStep}, {"index", "index", "index"},
        "{map = affine_map<(d0, d1)[s0] -> ((d0 - d1) ceildiv s0)>}");
    const int actualUpper = createInBlock(
        newLoop, outerRegion, outerBlock, "arith.minui", {"index"},
        {result(capped), result(pipelineIters)}, {"index", "index"});

    std::map<int, int> outerResultValues;
    std::map<int, int> expandedProducedValues;
    for (size_t itemIndex = 0; itemIndex < analysis.worklist.size();
         ++itemIndex) {
      try {
        if (!rewriteWorkItem(
                analysis.worklist[itemIndex], oldLoop, oldBody, newLoop,
                outerRegion, outerBlock, result(indexZero), result(actualUpper),
                result(indexOne), originalStep, outerIV, globalValues,
                expandedLocalOutputs, workspaceAllocations, workspaceTypes,
                expandedProducedValues, outerResultValues))
          return false;
      } catch (const std::out_of_range &) {
        throw std::runtime_error("CVPipelining: unmapped value in work item " +
                                 std::to_string(itemIndex));
      }
    }

    if (!analysis.trailingAtomicEffect.empty())
      createSetAtomic(newLoop, outerRegion, outerBlock,
                      analysis.trailingAtomicEffect);

    const GenericOperation &oldYield = module.operations.at(
        static_cast<size_t>(oldBody.operations.back()));
    std::vector<int> outerYields;
    for (int operand : oldYield.operands) {
      const auto outerMapped = outerResultValues.find(operand);
      if (outerMapped != outerResultValues.end()) {
        outerYields.push_back(outerMapped->second);
        continue;
      }
      const auto workspaceMapped = expandedProducedValues.find(operand);
      if (workspaceMapped != expandedProducedValues.end()) {
        const int lastSlice = createInBlock(
            newLoop, outerRegion, outerBlock, "arith.constant", {"index"}, {},
            {}, "{value = " + std::to_string(analysis.multibuffer - 1) +
                    " : index}");
        const int slice = createTensorExtract(
            newLoop, outerRegion, outerBlock, workspaceMapped->second,
            valueType(operand, "workspace yielded output"), result(lastSlice));
        outerYields.push_back(result(slice));
        continue;
      }
      const auto globalMapped = globalValues.find(operand);
      outerYields.push_back(globalMapped == globalValues.end()
                                ? operand
                                : globalMapped->second);
    }
    createInBlock(newLoop, outerRegion, outerBlock, "scf.yield", {},
                  outerYields, oldYield.operandTypes);

    std::map<int, int> replacements;
    const GenericOperation &createdLoop =
        module.operations.at(static_cast<size_t>(newLoop));
    for (size_t index = 0;
         index < oldLoop.results.size() && index < createdLoop.results.size();
         ++index)
      replacements[oldLoop.results[index]] = createdLoop.results[index];
    for (GenericOperation &operation : module.operations)
      if (!CVPipelineIsDescendant(module, operation.id, analysis.loop))
        for (int &operand : operation.operands)
          if (replacements.count(operand))
            operand = replacements.at(operand);

    GenericBlock &parent = module.blocks.at(static_cast<size_t>(parentBlock));
    parent.operations.erase(parent.operations.begin() +
                            static_cast<std::ptrdiff_t>(insertion));
    parent.operations.insert(parent.operations.begin() +
                                 static_cast<std::ptrdiff_t>(insertion),
                             generated.begin(), generated.end());
    module = CompactGenericModule(std::move(module));
    return true;
  }

private:
  void index() {
    for (const GenericBlock &block : module.blocks)
      for (size_t index = 0; index < block.arguments.size(); ++index)
        valueTypes[block.arguments[index]] = block.argumentTypes[index];
    for (const GenericOperation &operation : module.operations) {
      for (size_t index = 0; index < operation.results.size(); ++index) {
        definitions[operation.results[index]] = operation.id;
        valueTypes[operation.results[index]] = operation.resultTypes[index];
      }
    }
  }

  int result(int operation, size_t index = 0) const {
    return module.operations.at(static_cast<size_t>(operation)).results.at(index);
  }

  const std::string &valueType(int value, const char *context) const {
    const auto found = valueTypes.find(value);
    if (found == valueTypes.end())
      throw std::runtime_error(std::string("CVPipelining: missing type for ") +
                               context + " value " + std::to_string(value));
    return found->second;
  }

  int definingOperation(int value) const {
    const auto found = definitions.find(value);
    return found == definitions.end() ? -1 : found->second;
  }

  int traceValueDef(int value) const {
    const int definition = definingOperation(value);
    if (definition < 0)
      return value;
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(definition));
    if ((operation.name == "tensor.reshape" ||
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
         operation.name == "memref.subview") &&
        !operation.operands.empty())
      return traceValueDef(operation.operands.front());
    if (operation.name == "tensor.insert_slice" && operation.operands.size() > 1)
      return traceValueDef(operation.operands[1]);
    return value;
  }

  int traceAlloc(int value) const {
    const int root = traceValueDef(value);
    const int definition = definingOperation(root);
    if (definition < 0)
      return -1;
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(definition));
    return operation.name == "memref.alloc" ? operation.id : -1;
  }

  std::string removeDictionaryEntry(const std::string &dictionary,
                                    const std::string &key) const {
    if (dictionary.size() < 2 || dictionary.front() != '{')
      return dictionary;
    std::vector<std::string> kept;
    for (const std::string &entry :
         splitTopLevel(dictionary.substr(1, dictionary.size() - 2))) {
      const size_t equal = entry.find('=');
      if (trim(entry.substr(0, equal)) != key)
        kept.push_back(entry);
    }
    std::string result = "{";
    for (size_t index = 0; index < kept.size(); ++index) {
      if (index)
        result += ", ";
      result += kept[index];
    }
    result += "}";
    return result;
  }

  void indexBlockArguments(const GenericBlock &block) {
    for (size_t index = 0;
         index < block.arguments.size() && index < block.argumentTypes.size();
         ++index)
      valueTypes[block.arguments[index]] = block.argumentTypes[index];
  }

  int createInBlock(int parent, int region, int block, const std::string &name,
                    const std::vector<std::string> &resultTypes,
                    const std::vector<int> &operands,
                    const std::vector<std::string> &operandTypes,
                    const std::string &attributes = "{}") {
    const int operation = rewriter.createOperation(
        parent, region, block, name, resultTypes, operands, operandTypes, "",
        attributes);
    rewriter.appendToBlock(block, operation);
    for (size_t index = 0; index < resultTypes.size(); ++index)
      valueTypes[result(operation, index)] = resultTypes[index];
    return operation;
  }

  const GenericBlock &oldLoopBody() const {
    const GenericOperation &loop =
        module.operations.at(static_cast<size_t>(analysis.loop));
    const GenericRegion &region =
        module.regions.at(static_cast<size_t>(loop.regions.front()));
    return module.blocks.at(static_cast<size_t>(region.blocks.front()));
  }

  int workspaceAllocation(int output) const {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(output));
    if (operation.operands.empty())
      return -1;
    auto toTensor = definitions.find(operation.operands.back());
    if (toTensor == definitions.end())
      return -1;
    const GenericOperation &conversion = module.operations.at(
        static_cast<size_t>(toTensor->second));
    if (conversion.name != "bufferization.to_tensor" ||
        conversion.operands.empty())
      return -1;
    auto allocation = definitions.find(conversion.operands.front());
    return allocation == definitions.end() ? -1 : allocation->second;
  }

  int createTensorExtract(int parent, int region, int block, int expanded,
                          const std::string &resultType, int iv) {
    const std::string expandedType = valueType(expanded, "extract source");
    const std::optional<MemRefTypeModel> tensor = ParseMemRefType(
        "memref<" + resultType.substr(7));
    const size_t rank = tensor ? tensor->shape.size() : 1;
    std::ostringstream offsets, sizes, strides;
    offsets << "array<i64: -9223372036854775808";
    sizes << "array<i64: 1";
    strides << "array<i64: 1";
    for (size_t axis = 0; axis < rank; ++axis) {
      offsets << ", 0";
      sizes << ", " << (tensor->shape[axis] ? *tensor->shape[axis] : -1);
      strides << ", 1";
    }
    offsets << ">"; sizes << ">"; strides << ">";
    return createInBlock(
        parent, region, block, "tensor.extract_slice", {resultType},
        {expanded, iv}, {expandedType, "index"},
        "{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = " +
            offsets.str() + ", static_sizes = " + sizes.str() +
            ", static_strides = " + strides.str() + "}");
  }

  int createTensorInsert(int parent, int region, int block, int source,
                         int destination, const std::string &sourceType,
                         int iv) {
    const std::string destinationType = valueType(destination, "insert target");
    const std::optional<MemRefTypeModel> tensor = ParseMemRefType(
        "memref<" + sourceType.substr(7));
    const size_t rank = tensor ? tensor->shape.size() : 1;
    std::ostringstream offsets, sizes, strides;
    offsets << "array<i64: -9223372036854775808";
    sizes << "array<i64: 1";
    strides << "array<i64: 1";
    for (size_t axis = 0; axis < rank; ++axis) {
      offsets << ", 0";
      sizes << ", " << (tensor->shape[axis] ? *tensor->shape[axis] : -1);
      strides << ", 1";
    }
    offsets << ">"; sizes << ">"; strides << ">";
    return createInBlock(
        parent, region, block, "tensor.insert_slice", {destinationType},
        {source, destination, iv}, {sourceType, destinationType, "index"},
        "{operandSegmentSizes = array<i32: 1, 1, 1, 0, 0>, static_offsets = " +
            offsets.str() + ", static_sizes = " + sizes.str() +
            ", static_strides = " + strides.str() + "}");
  }

  int createSubview(int parent, int region, int block, int expanded,
                    const std::string &targetType, int iv) {
    const std::string expandedType = valueType(expanded, "subview source");
    const std::string subviewType =
        CVPipelineLocalSubviewType(expandedType, targetType);
    int subview = createInBlock(
        parent, region, block, "memref.subview", {subviewType},
        {expanded, iv}, {expandedType, "index"},
        CVPipelineLocalSubviewAttributes(targetType));
    const std::string sourceSpace = CVPipelineAddressSpaceSuffix(expandedType);
    const std::string targetSpace = CVPipelineAddressSpaceSuffix(targetType);
    if (sourceSpace != targetSpace) {
      const std::string castType =
          CVPipelineMemRefWithoutMemorySpace(subviewType);
      subview = createInBlock(parent, region, block,
                              "memref.memory_space_cast", {castType},
                              {result(subview)}, {subviewType});
    }
    return subview;
  }

  int createToTensor(int parent, int region, int block, int memref,
                     bool writable) {
    std::string memrefType = valueType(memref, "to_tensor source");
    int source = memref;
    if (!CVPipelineAddressSpaceSuffix(memrefType).empty()) {
      const std::string castType =
          CVPipelineMemRefWithoutMemorySpace(memrefType);
      const int cast = createInBlock(parent, region, block,
                                     "memref.memory_space_cast", {castType},
                                     {source}, {memrefType});
      source = result(cast);
      memrefType = castType;
    }
    const std::string tensorType = CVPipelineTensorFromMemRefType(memrefType);
    return createInBlock(parent, region, block, "bufferization.to_tensor",
                         {tensorType}, {source}, {memrefType},
                         writable ? "{restrict, writable}" : "{restrict}");
  }

  int createSetAtomic(int parent, int region, int block,
                      const std::string &effect) {
    const int operation =
        rewriter.createOperation(parent, region, block, "hivm.hir.set_atomic",
                                 {}, {}, {}, effect, effect);
    rewriter.appendToBlock(block, operation);
    return operation;
  }

  std::string atomicNoneEffect(std::string effect) const {
    const size_t key = effect.find("#hivm.atomic_kind<");
    if (key == std::string::npos)
      return effect;
    const size_t begin = effect.find('<', key);
    const size_t end = effect.find('>', begin);
    if (begin == std::string::npos || end == std::string::npos)
      return effect;
    effect.replace(begin + 1, end - begin - 1, "none");
    return effect;
  }

  std::string ConvertTensorResultToMemRef(int value) const {
    const int definition = definingOperation(value);
    if (definition >= 0) {
      const GenericOperation &operation =
          module.operations.at(static_cast<size_t>(definition));
      if (operation.name == "bufferization.to_tensor" &&
          !operation.operandTypes.empty())
        return operation.operandTypes.front();
    }
    const std::string &type = valueType(value, "tensor result");
    if (!CVPipelineIsTensorType(type) || type.back() != '>')
      throw std::runtime_error("CVPipelining: expected tensor result");
    return "memref<" + type.substr(7);
  }

  bool rewriteWorkItem(
      const CVPipelineWorkItem &item, const GenericOperation &oldLoop,
      const GenericBlock &oldBody, int outerLoop, int outerRegion,
      int outerBlock, int lower, int upper, int step, int originalStep,
      int outerIV, std::map<int, int> &globalValues,
      const std::map<int, int> &expandedLocalOutputs,
      const std::map<int, int> &workspaceAllocations,
      const std::map<int, std::string> &workspaceTypes,
      std::map<int, int> &expandedProducedValues,
      std::map<int, int> &outerResultValues) {
    std::vector<int> inits;
    std::vector<std::string> initTypes;
    for (const auto &[value, ordinal] : item.yieldedOutputs) {
      (void)value;
      if (ordinal + 1 >= oldBody.arguments.size())
        return false;
      const int argument = oldBody.arguments[ordinal + 1];
      if (!globalValues.count(argument))
        throw std::runtime_error("CVPipelining: missing outer iter argument " +
                                 std::to_string(argument));
      inits.push_back(globalValues.at(argument));
      initTypes.push_back(valueType(argument, "yield init"));
    }
    for (const auto &[value, unused] : item.localOutputs) {
      (void)unused;
      auto expanded = expandedLocalOutputs.find(value);
      if (expanded != expandedLocalOutputs.end() &&
          CVPipelineIsTensorType(valueType(expanded->second, "local init"))) {
        inits.push_back(expanded->second);
        initTypes.push_back(valueType(expanded->second, "local init"));
      }
    }
    const std::string attributes =
        "{hivm.loop_core_type = #hivm.tcore_type<" +
        std::string(item.core == CVPipelineCoreType::Cube ? "CUBE" : "VECTOR") +
        ">, multibuffer_unroll_factor = " +
        std::to_string(analysis.multibuffer) + " : i32}";
    std::vector<int> operands = {lower, upper, step};
    operands.insert(operands.end(), inits.begin(), inits.end());
    std::vector<std::string> operandTypes = {"index", "index", "index"};
    operandTypes.insert(operandTypes.end(), initTypes.begin(), initTypes.end());
    const int loop = createInBlock(outerLoop, outerRegion, outerBlock,
                                   "scf.for", initTypes, operands,
                                   operandTypes, attributes);
    const int region = rewriter.createRegion(loop);
    std::vector<std::string> argumentTypes = {"index"};
    argumentTypes.insert(argumentTypes.end(), initTypes.begin(), initTypes.end());
    const int block = rewriter.createBlock(region, argumentTypes);
    const GenericBlock inner = module.blocks.at(static_cast<size_t>(block));
    indexBlockArguments(inner);
    const int innerIV = inner.arguments.front();
    const int reconstructed = createInBlock(
        loop, region, block, "affine.apply", {"index"},
        {innerIV, originalStep, outerIV}, {"index", "index", "index"},
        "{map = affine_map<(d0)[s0, s1] -> (d0 * s0 + s1)>}");
    int oldIV = result(reconstructed);
    if (oldLoop.operandTypes.front() != "index")
      oldIV = result(createInBlock(loop, region, block, "arith.index_cast",
                                   {oldLoop.operandTypes.front()}, {oldIV},
                                   {"index"}));

    std::map<int, int> values = globalValues;
    values[oldBody.arguments.front()] = oldIV;
    size_t argumentIndex = 1;
    for (const auto &[value, ordinal] : item.yieldedOutputs) {
      (void)value;
      values[oldBody.arguments[ordinal + 1]] = inner.arguments[argumentIndex++];
    }
    std::map<int, int> localArguments;
    for (const auto &[value, unused] : item.localOutputs) {
      (void)unused;
      auto expanded = expandedLocalOutputs.find(value);
      if (expanded != expandedLocalOutputs.end() &&
          CVPipelineIsTensorType(valueType(expanded->second, "local init")) &&
          argumentIndex < inner.arguments.size())
        localArguments[value] = inner.arguments[argumentIndex++];
    }
    std::map<int, int> memrefOutputWriters;
    for (const auto &[toTensor, writer] : analysis.outputMemrefMap) {
      const GenericOperation &toTensorOp =
          module.operations.at(static_cast<size_t>(toTensor));
      for (int resultValue : toTensorOp.results)
        if (std::find_if(item.localOutputs.begin(), item.localOutputs.end(),
                         [&](const auto &entry) {
                           return entry.first == resultValue;
                         }) != item.localOutputs.end())
          memrefOutputWriters[writer] = resultValue;
    }
    std::map<int, int> localMemrefSubviews;
    auto getLocalMemrefSubview = [&](int localValue) -> int {
      auto existing = localMemrefSubviews.find(localValue);
      if (existing != localMemrefSubviews.end())
        return existing->second;
      auto expanded = expandedLocalOutputs.find(localValue);
      if (expanded == expandedLocalOutputs.end() ||
          !CVPipelineIsMemRefType(valueType(expanded->second,
                                            "local memref output")))
        throw std::runtime_error("CVPipelining: missing local memref output");
      const int subview =
          createSubview(loop, region, block, expanded->second,
                        ConvertTensorResultToMemRef(localValue), innerIV);
      localMemrefSubviews[localValue] = result(subview);
      return result(subview);
    };
    std::map<int, int> extracted;
    std::vector<int> localYieldValues;
    const std::set<int> workspaceSet(item.workspaceOutputs.begin(),
                                     item.workspaceOutputs.end());
    for (int operationId : oldBody.operations) {
      if (item.ops.count(operationId) == 0)
        continue;
      const GenericOperation source = module.operations.at(
          static_cast<size_t>(operationId));
      std::map<int, int> operationValues = values;
      for (int operand : source.operands) {
        auto produced = expandedProducedValues.find(operand);
        if (produced == expandedProducedValues.end())
          continue;
        if (!extracted.count(operand)) {
          const int slice = createTensorExtract(loop, region, block,
                                                produced->second,
                                                valueType(operand, "cross-item operand"), innerIV);
          extracted[operand] = result(slice);
        }
        operationValues[operand] = extracted.at(operand);
      }
      auto local = std::find_if(
          item.localOutputs.begin(), item.localOutputs.end(),
          [&](const auto &entry) {
            return std::find(source.results.begin(), source.results.end(),
                             entry.first) != source.results.end();
          });
      if (local != item.localOutputs.end() && !source.dpsInits.empty()) {
        const int output = local->first;
        const int destination = source.dpsInits.front();
        if (localArguments.count(output)) {
          const int slice = createTensorExtract(
              loop, region, block, localArguments.at(output),
              valueType(output, "local output"), innerIV);
          operationValues[destination] = result(slice);
        } else if (expandedLocalOutputs.count(output)) {
          operationValues[destination] = getLocalMemrefSubview(output);
        }
      }
      auto memrefWriter = memrefOutputWriters.find(operationId);
      if (memrefWriter != memrefOutputWriters.end() && !source.dpsInits.empty())
        operationValues[source.dpsInits.front()] =
            getLocalMemrefSubview(memrefWriter->second);
      if (local != item.localOutputs.end() &&
          source.name == "bufferization.to_tensor" && !source.operands.empty() &&
          expandedLocalOutputs.count(local->first) &&
          CVPipelineIsMemRefType(valueType(expandedLocalOutputs.at(local->first),
                                           "local memref output")))
        operationValues[source.operands.front()] =
            getLocalMemrefSubview(local->first);
      int cloned = -1;
      const auto atomic = analysis.atomicEffectMap.find(operationId);
      if (atomic != analysis.atomicEffectMap.end())
        createSetAtomic(loop, region, block, atomic->second);
      if (workspaceSet.count(operationId)) {
        const int allocation = workspaceAllocation(operationId);
        if (!workspaceAllocations.count(allocation))
          throw std::runtime_error("CVPipelining: missing expanded workspace " +
                                   std::to_string(allocation));
        const int expanded = workspaceAllocations.at(allocation);
        const std::string &expandedType = workspaceTypes.at(allocation);
        const std::string subviewType = CVPipelineSubviewType(expandedType);
        const int subview = createInBlock(
            loop, region, block, "memref.subview", {subviewType},
            {expanded, innerIV}, {expandedType, "index"},
            CVPipelineWorkspaceSubviewAttributes(expandedType));
        const std::string collapsedType =
            CVPipelineCollapsedWorkspaceType(expandedType);
        const int collapse = createInBlock(
            loop, region, block, "memref.collapse_shape", {collapsedType},
            {result(subview)}, {subviewType},
            CVPipelineWorkspaceCollapseReassociation(expandedType));
        std::vector<int> separatorOperands = source.operands;
        if (!separatorOperands.empty())
          separatorOperands.back() = result(collapse);
        for (int &operand : separatorOperands)
          if (operationValues.count(operand))
            operand = operationValues.at(operand);
        std::vector<std::string> separatorTypes = source.operandTypes;
        if (!separatorTypes.empty())
          separatorTypes.back() = collapsedType;
        cloned = createInBlock(loop, region, block, source.name, {},
                               separatorOperands, separatorTypes,
                               source.attributes);
      } else {
        cloned = rewriter.cloneOperationTree(operationId, loop, region, block,
                                              operationValues);
        rewriter.appendToBlock(block, cloned);
        // OpBuilder::clone updates IRMapping for every nested result as well as
        // the top-level results. Preserve that mapping for later top-level ops
        // that consume values defined in a cloned region.
        values.insert(operationValues.begin(), operationValues.end());
      }
      if (atomic != analysis.atomicEffectMap.end())
        createSetAtomic(loop, region, block, atomicNoneEffect(atomic->second));
      const GenericOperation &clonedOperation =
          module.operations.at(static_cast<size_t>(cloned));
      for (size_t index = 0;
           index < source.results.size() && index < clonedOperation.results.size();
           ++index)
        values[source.results[index]] = clonedOperation.results[index];
      if (local != item.localOutputs.end()) {
        const int output = local->first;
        if (localArguments.count(output)) {
          if (!values.count(output))
            throw std::runtime_error("CVPipelining: incomplete local output " +
                                     std::to_string(output));
          const int inserted = createTensorInsert(
              loop, region, block, values.at(output), localArguments.at(output),
              valueType(output, "local insert"), innerIV);
          localYieldValues.push_back(result(inserted));
        }
      }
    }

    std::vector<int> yields;
    for (const auto &[value, ordinal] : item.yieldedOutputs) {
      (void)ordinal;
      if (!values.count(value))
        throw std::runtime_error("CVPipelining: missing yielded output " +
                                 std::to_string(value));
      yields.push_back(values.at(value));
    }
    yields.insert(yields.end(), localYieldValues.begin(), localYieldValues.end());
    createInBlock(loop, region, block, "scf.yield", {}, yields, initTypes);
    const GenericOperation &createdLoop =
        module.operations.at(static_cast<size_t>(loop));
    size_t loopResult = 0;
    for (const auto &[value, ordinal] : item.yieldedOutputs) {
      (void)ordinal;
      outerResultValues[value] = createdLoop.results.at(loopResult++);
    }
    for (const auto &[value, unused] : item.localOutputs) {
      (void)unused;
      const auto expanded = expandedLocalOutputs.find(value);
      if (expanded != expandedLocalOutputs.end() &&
          CVPipelineIsTensorType(valueType(expanded->second, "local output"))) {
        expandedProducedValues[value] = createdLoop.results.at(loopResult++);
      } else if (expanded != expandedLocalOutputs.end()) {
        const int toTensor = createToTensor(outerLoop, outerRegion, outerBlock,
                                            expanded->second, true);
        expandedProducedValues[value] = result(toTensor);
      }
    }
    for (int output : item.workspaceOutputs) {
      const int allocation = workspaceAllocation(output);
      if (!workspaceAllocations.count(allocation))
        throw std::runtime_error("CVPipelining: missing final workspace " +
                                 std::to_string(allocation));
      const int expanded = workspaceAllocations.at(allocation);
      const std::string expandedType = workspaceTypes.at(allocation);
      const std::string tensorType = "tensor<" + expandedType.substr(7);
      const int toTensor = createInBlock(
          outerLoop, outerRegion, outerBlock, "bufferization.to_tensor",
          {tensorType}, {expanded}, {expandedType}, "{restrict}");
      const GenericOperation &source = module.operations.at(
          static_cast<size_t>(output));
      if (!source.results.empty()) {
        const int expandedResult = result(toTensor);
        (void)valueType(expandedResult, "workspace tensor");
        expandedProducedValues[source.results.front()] = expandedResult;
      }
    }
    for (const auto &[source, replacement] : outerResultValues)
      globalValues[source] = replacement;
    return true;
  }

  GenericModule &module;
  CVPipelineAnalysisResult analysis;
  GenericRewriter rewriter;
  std::map<int, int> definitions;
  std::map<int, std::string> valueTypes;
};

} // namespace cvub

#endif
