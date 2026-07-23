#ifndef CVPIPELINE_UB_MODEL_CPP_HIVM_DIMENSION_ANALYZER_HPP
#define CVPIPELINE_UB_MODEL_CPP_HIVM_DIMENSION_ANALYZER_HPP

#include "../passes/one_shot_bufferize.hpp"

namespace cvub {

struct ShapedTypeModel {
  bool tensor = false;
  std::vector<std::optional<int64_t>> shape;
};

inline std::optional<ShapedTypeModel>
ParseShapedTypeForDimensionAnalysis(const std::string &type) {
  const bool tensor = startsWith(type, "tensor<");
  const bool memref = startsWith(type, "memref<");
  if (!tensor && !memref)
    return std::nullopt;
  const std::string memrefType = tensor ? ConvertTensorToMemRefType(type) : type;
  const std::optional<MemRefTypeModel> parsed = ParseMemRefType(memrefType);
  if (!parsed)
    return std::nullopt;
  return ShapedTypeModel{tensor, parsed->shape};
}

inline std::vector<int64_t> ParseDimensionI64Array(
    const std::string &dictionary, const std::string &key) {
  std::string value = FindDictionaryValue(dictionary, key);
  const size_t colon = value.find(':');
  const size_t close = value.rfind('>');
  if (colon == std::string::npos || close == std::string::npos ||
      colon >= close)
    return {};
  std::vector<int64_t> result;
  for (const std::string &item :
       splitTopLevel(value.substr(colon + 1, close - colon - 1)))
    result.push_back(std::stoll(trim(item)));
  return result;
}

inline std::vector<std::vector<int64_t>> ParseReassociationIndices(
    const std::string &properties) {
  std::string value = FindDictionaryValue(properties, "reassociation");
  value = trim(std::move(value));
  if (value.size() < 2 || value.front() != '[' || value.back() != ']')
    return {};
  std::vector<std::vector<int64_t>> result;
  for (std::string group :
       splitTopLevel(value.substr(1, value.size() - 2))) {
    group = trim(std::move(group));
    if (group.size() < 2 || group.front() != '[' || group.back() != ']')
      return {};
    std::vector<int64_t> indices;
    for (const std::string &item :
         splitTopLevel(group.substr(1, group.size() - 2)))
      indices.push_back(std::stoll(trim(item)));
    result.push_back(std::move(indices));
  }
  return result;
}

class DimensionAnalyzer {
public:
  explicit DimensionAnalyzer(const GenericModule &inputModule)
      : module(inputModule) {
    indexValues();
  }

  bool initialize() {
    initializeDimensions();
    processOperations();
    return true;
  }

  void computeTilingDim(const GenericOperation &function) {
    analyzedFunction = function.id;
    std::vector<const GenericOperation *> stores;
    for (int operationId : descendants(function)) {
      const GenericOperation &operation =
          module.operations.at(static_cast<size_t>(operationId));
      if ((operation.name == "hivm.hir.store" ||
           operation.name == "hivm.hir.copy" ||
           operation.name == "hivm.hir.indirect_store") &&
          !operation.operands.empty())
        stores.push_back(&operation);
    }
    if (stores.empty())
      return;

    std::map<int, std::vector<const GenericOperation *>> storesByGroup;
    for (const GenericOperation *store : stores) {
      const int source = store->operands.front();
      auto value = valueToNode.find(source);
      if (value == valueToNode.end())
        continue;
      storesByGroup[findValue(value->second)].push_back(store);
    }

    for (const auto &[group, groupStores] : storesByGroup) {
      (void)group;
      std::map<int, std::vector<std::pair<int, int>>> candidates;
      for (const GenericOperation *store : groupStores) {
        const int source = store->operands.front();
        const auto type = shapedTypes.find(source);
        if (type == shapedTypes.end())
          continue;
        std::set<int> usedRoots;
        for (size_t axis = 0; axis < type->second.shape.size(); ++axis) {
          const std::optional<int64_t> extent = type->second.shape[axis];
          const int root = findDimension(dimensionNode(source, axis));
          if ((!extent || *extent == 1) &&
              !checkTileableMaskedStore(*store, axis))
            continue;
          if ((extent && *extent < 1) || reducedDimensions.count(root) != 0 ||
              !usedRoots.insert(root).second)
            continue;
          candidates[root].push_back(
              {source, static_cast<int>(axis)});
        }
      }

      std::map<int, int> selectedAxis;
      for (const auto &[root, values] : candidates) {
        if (values.size() != groupStores.size())
          continue;
        int higherAxisCount = 0;
        for (const auto &[value, axis] : values) {
          auto current = selectedAxis.find(value);
          if (current == selectedAxis.end() || current->second > axis)
            ++higherAxisCount;
        }
        if (2 * higherAxisCount < static_cast<int>(groupStores.size()))
          continue;
        selectedRoots.insert(root);
        for (const auto &[value, axis] : values)
          selectedAxis[value] = axis;
      }
    }

    for (const auto &[value, type] : shapedTypes) {
      int selected = -1;
      for (size_t axis = 0; axis < type.shape.size(); ++axis) {
        const int root = findDimension(dimensionNode(value, axis));
        if (selectedRoots.count(root) != 0 &&
            reducedDimensions.count(root) == 0) {
          selected = static_cast<int>(axis);
          break;
        }
      }
      tilingDims[value] = selected;
    }
  }

  int64_t getTilingDim(int value) const {
    auto found = tilingDims.find(value);
    return found == tilingDims.end() ? -1 : found->second;
  }

  bool hasSelectedTilingDim() const { return !selectedRoots.empty(); }

  std::string serializeAnalysis() const {
    std::ostringstream output;
    output << "DIMENSION_ANALYSIS\t1\n";
    for (const auto &[value, type] : shapedTypes) {
      const auto valueNode = valueToNode.find(value);
      output << "VALUE\t" << value << '\t'
             << (valueNode == valueToNode.end()
                     ? -1
                     : findValueConst(valueNode->second))
             << '\t' << getTilingDim(value) << '\t'
             << HexEncode(valueTypes.at(value));
      for (size_t axis = 0; axis < type.shape.size(); ++axis) {
        const int root = findDimension(dimensionNode(value, axis));
        output << '\t' << axis << ':' << root << ':'
               << (selectedRoots.count(root) != 0 ? 1 : 0) << ':'
               << (reducedDimensions.count(root) != 0 ? 1 : 0);
      }
      output << '\n';
    }
    if (analyzedFunction >= 0) {
      for (int operationId :
           descendants(module.operations.at(static_cast<size_t>(analyzedFunction)))) {
        const GenericOperation &operation =
            module.operations.at(static_cast<size_t>(operationId));
        if ((operation.name != "hivm.hir.store" &&
             operation.name != "hivm.hir.copy" &&
             operation.name != "hivm.hir.indirect_store") ||
            operation.operands.empty())
          continue;
        const int source = operation.operands.front();
        const auto value = valueToNode.find(source);
        output << "STORE\t" << operation.id << '\t' << source << '\t'
               << (value == valueToNode.end()
                       ? -1
                       : findValueConst(value->second));
        const auto type = shapedTypes.find(source);
        if (type != shapedTypes.end())
          for (size_t axis = 0; axis < type->second.shape.size(); ++axis)
            output << '\t' << axis << ':'
                   << findDimension(dimensionNode(source, axis));
        output << '\n';
      }
    }
    return output.str();
  }

private:
  void indexValues() {
    for (const GenericBlock &block : module.blocks)
      for (size_t index = 0; index < block.arguments.size(); ++index)
        valueTypes[block.arguments[index]] = block.argumentTypes[index];
    for (const GenericOperation &operation : module.operations) {
      for (size_t index = 0;
           index < operation.results.size() &&
           index < operation.resultTypes.size();
           ++index) {
        valueTypes[operation.results[index]] = operation.resultTypes[index];
        valueDefinitions[operation.results[index]] = operation.id;
      }
    }
    for (const auto &[value, type] : valueTypes)
      if (auto shaped = ParseShapedTypeForDimensionAnalysis(type))
        shapedTypes[value] = *shaped;
  }

  void initializeDimensions() {
    for (const auto &[value, type] : shapedTypes) {
      valueToNode[value] = newValueNode();
      for (size_t axis = 0; axis < type.shape.size(); ++axis)
        dimensionNodes[{value, axis}] = newDimensionNode();
    }
  }

  int newValueNode() {
    const int node = static_cast<int>(valueParents.size());
    valueParents.push_back(node);
    return node;
  }

  int newDimensionNode() {
    const int node = static_cast<int>(dimensionParents.size());
    dimensionParents.push_back(node);
    return node;
  }

  int findValue(int node) {
    if (valueParents.at(static_cast<size_t>(node)) != node)
      valueParents[static_cast<size_t>(node)] =
          findValue(valueParents.at(static_cast<size_t>(node)));
    return valueParents.at(static_cast<size_t>(node));
  }

  int findValueConst(int node) const {
    while (valueParents.at(static_cast<size_t>(node)) != node)
      node = valueParents.at(static_cast<size_t>(node));
    return node;
  }

  int findDimension(int node) {
    if (dimensionParents.at(static_cast<size_t>(node)) != node)
      dimensionParents[static_cast<size_t>(node)] =
          findDimension(dimensionParents.at(static_cast<size_t>(node)));
    return dimensionParents.at(static_cast<size_t>(node));
  }

  int findDimension(int node) const {
    while (dimensionParents.at(static_cast<size_t>(node)) != node)
      node = dimensionParents.at(static_cast<size_t>(node));
    return node;
  }

  void joinValues(int lhs, int rhs) {
    auto left = valueToNode.find(lhs);
    auto right = valueToNode.find(rhs);
    if (left == valueToNode.end() || right == valueToNode.end())
      return;
    const int leftRoot = findValue(left->second);
    const int rightRoot = findValue(right->second);
    if (leftRoot != rightRoot)
      valueParents[static_cast<size_t>(rightRoot)] = leftRoot;
  }

  int dimensionNode(int value, size_t axis) const {
    auto found = dimensionNodes.find({value, axis});
    if (found == dimensionNodes.end())
      throw std::runtime_error("DimensionAnalyzer: missing shaped dimension");
    return found->second;
  }

  void joinDimensions(int lhs, size_t lhsAxis, int rhs, size_t rhsAxis) {
    if (shapedTypes.count(lhs) == 0 || shapedTypes.count(rhs) == 0 ||
        lhsAxis >= shapedTypes.at(lhs).shape.size() ||
        rhsAxis >= shapedTypes.at(rhs).shape.size())
      return;
    joinValues(lhs, rhs);
    const int leftRoot = findDimension(dimensionNode(lhs, lhsAxis));
    const int rightRoot = findDimension(dimensionNode(rhs, rhsAxis));
    if (leftRoot != rightRoot)
      dimensionParents[static_cast<size_t>(rightRoot)] = leftRoot;
  }

  void joinSameRank(int lhs, int rhs) {
    auto left = shapedTypes.find(lhs);
    auto right = shapedTypes.find(rhs);
    if (left == shapedTypes.end() || right == shapedTypes.end() ||
        left->second.shape.size() != right->second.shape.size())
      return;
    for (size_t axis = 0; axis < left->second.shape.size(); ++axis)
      joinDimensions(lhs, axis, rhs, axis);
  }

  std::optional<int64_t> getViewSourceDimension(int value,
                                                size_t axis) const {
    const auto definition = valueDefinitions.find(value);
    if (definition == valueDefinitions.end())
      return std::nullopt;
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(definition->second));
    if ((operation.name != "tensor.extract_slice" &&
         operation.name != "memref.subview") ||
        operation.operands.empty())
      return std::nullopt;
    const auto source = shapedTypes.find(operation.operands.front());
    if (source == shapedTypes.end() || axis >= source->second.shape.size())
      return std::nullopt;
    return source->second.shape[axis];
  }

  bool checkTileableMaskedStore(const GenericOperation &store,
                                size_t axis) const {
    if (store.operands.empty())
      return false;
    const int source = store.operands.front();
    const int destination = !store.dpsInits.empty()
                                ? store.dpsInits.front()
                                : store.operands.size() > 1
                                      ? store.operands[1]
                                      : -1;
    const std::optional<int64_t> sourceOriginalDimension =
        getViewSourceDimension(source, axis);
    const std::optional<int64_t> destinationOriginalDimension =
        getViewSourceDimension(destination, axis);
    return sourceOriginalDimension && destinationOriginalDimension &&
           *sourceOriginalDimension != 1 &&
           *sourceOriginalDimension == *destinationOriginalDimension;
  }

  void processReshape(const GenericOperation &operation) {
    if (operation.operands.empty() || operation.results.empty())
      return;
    const int input = operation.operands.front();
    const int output = operation.results.front();
    const std::vector<std::vector<int64_t>> reassociation =
        ParseReassociationIndices(operation.properties);
    if (reassociation.empty())
      return;
    if (operation.name == "tensor.expand_shape") {
      for (size_t inputAxis = 0; inputAxis < reassociation.size(); ++inputAxis) {
        int64_t outputAxis = reassociation[inputAxis].front();
        for (int64_t candidate : reassociation[inputAxis])
          if (shapedTypes.at(output).shape.at(static_cast<size_t>(outputAxis)) ==
              1)
            outputAxis = candidate;
        joinDimensions(input, inputAxis, output,
                       static_cast<size_t>(outputAxis));
      }
    } else {
      for (size_t outputAxis = 0; outputAxis < reassociation.size(); ++outputAxis) {
        int64_t inputAxis = reassociation[outputAxis].front();
        for (int64_t candidate : reassociation[outputAxis])
          if (shapedTypes.at(input).shape.at(static_cast<size_t>(inputAxis)) == 1)
            inputAxis = candidate;
        joinDimensions(input, static_cast<size_t>(inputAxis), output,
                       outputAxis);
      }
    }
  }

  void processFor(const GenericOperation &operation) {
    if (operation.regions.empty())
      return;
    const GenericRegion &region =
        module.regions.at(static_cast<size_t>(operation.regions.front()));
    if (region.blocks.empty())
      return;
    const GenericBlock &block =
        module.blocks.at(static_cast<size_t>(region.blocks.front()));
    const size_t initBegin = operation.operands.size() >= operation.results.size()
                                 ? operation.operands.size() - operation.results.size()
                                 : operation.operands.size();
    for (size_t index = 0; index < operation.results.size(); ++index) {
      if (initBegin + index < operation.operands.size())
        joinSameRank(operation.operands[initBegin + index],
                     operation.results[index]);
      if (index + 1 < block.arguments.size())
        joinSameRank(operation.results[index], block.arguments[index + 1]);
    }
  }

  void processYield(const GenericOperation &operation) {
    if (operation.parentId < 0)
      return;
    const GenericOperation &parent =
        module.operations.at(static_cast<size_t>(operation.parentId));
    for (size_t index = 0;
         index < operation.operands.size() && index < parent.results.size();
         ++index)
      joinSameRank(operation.operands[index], parent.results[index]);
  }

  void processOperations() {
    for (const GenericOperation &operation : module.operations) {
      if (operation.name == "tensor.expand_shape" ||
          operation.name == "tensor.collapse_shape") {
        processReshape(operation);
        continue;
      }
      if (operation.name == "scf.for") {
        processFor(operation);
        continue;
      }
      if (operation.name == "scf.yield") {
        processYield(operation);
        continue;
      }

      std::vector<int> shapedValues;
      for (int operand : operation.operands)
        if (shapedTypes.count(operand) != 0)
          shapedValues.push_back(operand);
      for (int result : operation.results)
        if (shapedTypes.count(result) != 0)
          shapedValues.push_back(result);
      if (shapedValues.empty())
        continue;

      if (operation.name == "hivm.hir.vtranspose" &&
          !operation.operands.empty() && !operation.results.empty()) {
        std::vector<int64_t> permutation =
            ParseDimensionI64Array(operation.properties, "permutation");
        if (permutation.empty())
          permutation = ParseDimensionI64Array(operation.attributes,
                                               "permutation");
        for (size_t axis = 0; axis < permutation.size(); ++axis)
          joinDimensions(operation.operands.front(),
                         static_cast<size_t>(permutation[axis]),
                         operation.results.front(), axis);
      } else {
        for (size_t index = 1; index < shapedValues.size(); ++index)
          joinSameRank(shapedValues.front(), shapedValues[index]);
      }

      if (operation.name == "hivm.hir.vreduce" &&
          !operation.operands.empty()) {
        std::vector<int64_t> reduceDims =
            ParseDimensionI64Array(operation.properties, "reduce_dims");
        for (int64_t axis : reduceDims)
          if (axis >= 0 &&
              static_cast<size_t>(axis) <
                  shapedTypes.at(operation.operands.front()).shape.size())
            reducedDimensions.insert(findDimension(dimensionNode(
                operation.operands.front(), static_cast<size_t>(axis))));
      }
    }
  }

  std::vector<int> descendants(const GenericOperation &operation) const {
    std::vector<int> result;
    std::function<void(int)> collect = [&](int operationId) {
      const GenericOperation &current =
          module.operations.at(static_cast<size_t>(operationId));
      for (int regionId : current.regions)
        for (int blockId :
             module.regions.at(static_cast<size_t>(regionId)).blocks)
          for (int child :
               module.blocks.at(static_cast<size_t>(blockId)).operations) {
            result.push_back(child);
            collect(child);
          }
    };
    collect(operation.id);
    return result;
  }

  const GenericModule &module;
  std::map<int, std::string> valueTypes;
  std::map<int, int> valueDefinitions;
  std::map<int, ShapedTypeModel> shapedTypes;
  std::map<int, int> valueToNode;
  std::map<std::pair<int, size_t>, int> dimensionNodes;
  std::vector<int> valueParents;
  std::vector<int> dimensionParents;
  std::set<int> reducedDimensions;
  std::set<int> selectedRoots;
  std::map<int, int> tilingDims;
  int analyzedFunction = -1;
};

} // namespace cvub

#endif
