#ifndef CVPIPELINE_UB_MODEL_CPP_BUFFER_TOPOLOGY_HPP
#define CVPIPELINE_UB_MODEL_CPP_BUFFER_TOPOLOGY_HPP

#include "../semantic_ir/c1_semantic_ir.hpp"

namespace cvub {

struct SuffixBufferUse {
  int operationId = -1;
  int operandOrdinal = -1;
  int valueId = -1;
};

struct SuffixBufferRecord {
  int ordinal = -1;
  int valueId = -1;
  int operationId = -1;
  std::string type;
  std::string alignment;
  std::vector<int> dynamicExtents;
  std::vector<int> aliases;
  std::vector<SuffixBufferUse> uses;
};

struct SuffixBufferTopology {
  std::vector<SuffixBufferRecord> localBuffers;
};

inline bool IsMemRefType(const std::string &type) {
  return startsWith(type, "memref<");
}

inline std::string FindDictionaryValue(const std::string &dictionary,
                                       const std::string &name) {
  if (dictionary.size() < 2)
    return "";
  for (const std::string &entry :
       splitTopLevel(dictionary.substr(1, dictionary.size() - 2))) {
    size_t equal = entry.find('=');
    if (equal != std::string::npos && trim(entry.substr(0, equal)) == name)
      return trim(entry.substr(equal + 1));
  }
  return "";
}

class SuffixBufferTopologyBuilder {
public:
  explicit SuffixBufferTopologyBuilder(const C1SemanticModule &module)
      : module(module) {
    indexValues();
  }

  SuffixBufferTopology Build() {
    SuffixBufferTopology topology;
    std::map<int, size_t> recordByRoot;
    for (const C1OperationRecord &operation : module.operations) {
      if (operation.name != "memref.alloc")
        continue;
      for (size_t index = 0; index < operation.results.size(); ++index) {
        if (index >= operation.resultTypes.size() ||
            !IsMemRefType(operation.resultTypes[index]))
          continue;
        SuffixBufferRecord record;
        record.ordinal = static_cast<int>(topology.localBuffers.size());
        record.valueId = operation.results[index];
        record.operationId = operation.id;
        record.type = operation.resultTypes[index];
        record.alignment = FindDictionaryValue(operation.attributes, "alignment");
        record.dynamicExtents = operation.operands;
        recordByRoot[record.valueId] = topology.localBuffers.size();
        topology.localBuffers.push_back(std::move(record));
      }
    }

    for (const auto &[valueId, type] : valueTypes) {
      if (!IsMemRefType(type))
        continue;
      int root = resolveRoot(valueId);
      auto found = recordByRoot.find(root);
      if (found != recordByRoot.end())
        topology.localBuffers[found->second].aliases.push_back(valueId);
    }
    for (const C1OperationRecord &operation : module.operations) {
      for (size_t index = 0; index < operation.operands.size(); ++index) {
        int valueId = operation.operands[index];
        int root = resolveRoot(valueId);
        auto found = recordByRoot.find(root);
        if (found == recordByRoot.end())
          continue;
        topology.localBuffers[found->second].uses.push_back(
            {operation.id, static_cast<int>(index), valueId});
      }
    }
    return topology;
  }

private:
  void indexValues() {
    for (const C1BlockRecord &block : module.blocks)
      for (size_t index = 0; index < block.arguments.size(); ++index) {
        valueTypes[block.arguments[index]] = block.argumentTypes[index];
        blockArgumentOwner[block.arguments[index]] = {block.id,
                                                       static_cast<int>(index)};
      }
    for (const C1OperationRecord &operation : module.operations)
      for (size_t index = 0; index < operation.results.size(); ++index) {
        if (index < operation.resultTypes.size())
          valueTypes[operation.results[index]] = operation.resultTypes[index];
        definingOperation[operation.results[index]] = {operation.id,
                                                        static_cast<int>(index)};
      }
  }

  int resolveRoot(int valueId) {
    auto cached = roots.find(valueId);
    if (cached != roots.end())
      return cached->second;
    if (!resolving.insert(valueId).second)
      return -1;
    int root = resolveRootImpl(valueId);
    resolving.erase(valueId);
    roots[valueId] = root;
    return root;
  }

  int resolveRootImpl(int valueId) {
    auto definition = definingOperation.find(valueId);
    if (definition != definingOperation.end()) {
      const C1OperationRecord &operation = module.operations.at(
          static_cast<size_t>(definition->second.first));
      const int resultIndex = definition->second.second;
      if (operation.name == "memref.alloc")
        return valueId;
      static const std::set<std::string> aliases = {
          "memref.cast", "memref.collapse_shape", "memref.expand_shape",
          "memref.reinterpret_cast", "memref.subview", "memref.view"};
      if (aliases.count(operation.name) != 0 && !operation.operands.empty())
        return resolveRoot(operation.operands.front());
      if (operation.name == "scf.for" &&
          static_cast<size_t>(resultIndex + 3) < operation.operands.size())
        return resolveRoot(operation.operands[static_cast<size_t>(resultIndex + 3)]);
      if (operation.name == "scf.while" &&
          static_cast<size_t>(resultIndex) < operation.operands.size())
        return resolveRoot(operation.operands[static_cast<size_t>(resultIndex)]);
      if (operation.name == "scf.if")
        return resolveIfResult(operation, resultIndex);
      return -1;
    }

    auto blockArgument = blockArgumentOwner.find(valueId);
    if (blockArgument == blockArgumentOwner.end())
      return -1;
    const C1BlockRecord &block = module.blocks.at(
        static_cast<size_t>(blockArgument->second.first));
    const C1RegionRecord &region = module.regions.at(
        static_cast<size_t>(block.regionId));
    const C1OperationRecord &owner = module.operations.at(
        static_cast<size_t>(region.parentOperation));
    const int argument = blockArgument->second.second;
    if (owner.name == "scf.for" && argument > 0 &&
        static_cast<size_t>(argument + 2) < owner.operands.size())
      return resolveRoot(owner.operands[static_cast<size_t>(argument + 2)]);
    if (owner.name == "scf.while" &&
        static_cast<size_t>(argument) < owner.operands.size())
      return resolveRoot(owner.operands[static_cast<size_t>(argument)]);
    return -1;
  }

  int resolveIfResult(const C1OperationRecord &operation, int resultIndex) {
    int common = -1;
    for (int regionId : operation.regions) {
      const C1RegionRecord &region =
          module.regions.at(static_cast<size_t>(regionId));
      if (region.blocks.empty())
        continue;
      const C1BlockRecord &block =
          module.blocks.at(static_cast<size_t>(region.blocks.back()));
      if (block.operations.empty())
        continue;
      const C1OperationRecord &yield =
          module.operations.at(static_cast<size_t>(block.operations.back()));
      if (yield.name != "scf.yield" ||
          static_cast<size_t>(resultIndex) >= yield.operands.size())
        return -1;
      int candidate =
          resolveRoot(yield.operands[static_cast<size_t>(resultIndex)]);
      if (common == -1)
        common = candidate;
      else if (common != candidate)
        return -1;
    }
    return common;
  }

  const C1SemanticModule &module;
  std::map<int, std::string> valueTypes;
  std::map<int, std::pair<int, int>> definingOperation;
  std::map<int, std::pair<int, int>> blockArgumentOwner;
  std::map<int, int> roots;
  std::set<int> resolving;
};

inline SuffixBufferTopology
BuildSuffixBufferTopology(const C1SemanticModule &module) {
  return SuffixBufferTopologyBuilder(module).Build();
}

inline std::string SerializeSuffixBufferTopology(
    const SuffixBufferTopology &topology) {
  std::ostringstream output;
  output << "SUFFIX_BUFFER_TOPOLOGY\t1\n";
  for (const SuffixBufferRecord &buffer : topology.localBuffers) {
    output << "ALLOC\t" << buffer.ordinal << '\t' << buffer.valueId << '\t'
           << buffer.operationId << '\t' << HexEncode(buffer.type) << '\t'
           << HexEncode(buffer.alignment) << '\t'
           << joinIds(buffer.dynamicExtents) << '\t'
           << joinIds(buffer.aliases) << '\n';
    for (const SuffixBufferUse &use : buffer.uses)
      output << "USE\t" << buffer.ordinal << '\t' << use.operationId << '\t'
             << use.operandOrdinal << '\t' << use.valueId << '\n';
  }
  return output.str();
}

} // namespace cvub

#endif
