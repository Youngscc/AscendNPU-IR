#ifndef CVPIPELINE_UB_MODEL_CPP_CVPIPELINING_HPP
#define CVPIPELINE_UB_MODEL_CPP_CVPIPELINING_HPP

#include "../../ir/generic_rewriter.hpp"

namespace cvub {

struct CVPipeliningOptions {
  bool disabled = false;
  int setDepthInUnrollMode = -1;
  bool enableSkewMode = false;
  bool enableLazyLoading = false;
};

enum class CVPipelineCoreType { Unknown, Cube, Vector, Mixed };

inline std::string_view CVPipelineTrimView(std::string_view value) {
  while (!value.empty() &&
         std::isspace(static_cast<unsigned char>(value.front())) != 0)
    value.remove_prefix(1);
  while (!value.empty() &&
         std::isspace(static_cast<unsigned char>(value.back())) != 0)
    value.remove_suffix(1);
  return value;
}

inline bool CVPipelineHasAttribute(const GenericOperation &operation,
                                   std::string_view name) {
  std::string_view dictionary = operation.attributes;
  if (dictionary.size() < 2)
    return false;
  dictionary.remove_prefix(1);
  dictionary.remove_suffix(1);
  size_t entryBegin = 0;
  unsigned depth = 0;
  bool quoted = false;
  bool escaped = false;
  for (size_t index = 0; index <= dictionary.size(); ++index) {
    const bool atEnd = index == dictionary.size();
    const char value = atEnd ? ',' : dictionary[index];
    if (!atEnd && quoted) {
      if (escaped)
        escaped = false;
      else if (value == '\\')
        escaped = true;
      else if (value == '"')
        quoted = false;
      continue;
    }
    if (!atEnd && value == '"') {
      quoted = true;
      continue;
    }
    if (!atEnd && isOpening(value)) {
      ++depth;
      continue;
    }
    if (!atEnd && (value == ')' || value == ']' || value == '}' ||
                   value == '>')) {
      if (depth != 0)
        --depth;
      continue;
    }
    if (value != ',' || depth != 0)
      continue;
    std::string_view entry =
        CVPipelineTrimView(dictionary.substr(entryBegin, index - entryBegin));
    const size_t equal = entry.find('=');
    if (CVPipelineTrimView(entry.substr(0, equal)) == name)
      return true;
    entryBegin = index + 1;
  }
  return false;
}

inline void CVPipelineSetUnitAttribute(GenericOperation &operation,
                                       const std::string &name) {
  std::vector<std::string> entries;
  if (operation.attributes.size() >= 2)
    entries = splitTopLevel(operation.attributes.substr(
        1, operation.attributes.size() - 2));
  if (std::find(entries.begin(), entries.end(), name) == entries.end())
    entries.push_back(name);
  std::sort(entries.begin(), entries.end());
  operation.attributes = "{";
  for (size_t index = 0; index < entries.size(); ++index) {
    if (index)
      operation.attributes += ", ";
    operation.attributes += entries[index];
  }
  operation.attributes += "}";
}

inline CVPipelineCoreType CVPipelineQueryCoreTypeFromSemantics(
    const GenericOperation &operation) {
  const auto contains = [&](std::string_view value) {
    return operation.properties.find(value) != std::string::npos ||
           operation.attributes.find(value) != std::string::npos;
  };
  if (contains("CUBE_AND_VECTOR"))
    return CVPipelineCoreType::Mixed;
  if (contains("CUBE_OR_VECTOR"))
    return CVPipelineCoreType::Unknown;
  if (contains("tcoretype<CUBE>") || contains("tcore_type<CUBE>"))
    return CVPipelineCoreType::Cube;
  if (contains("tcoretype<VECTOR>") || contains("tcore_type<VECTOR>"))
    return CVPipelineCoreType::Vector;
  if (!startsWith(operation.name, "hivm.hir."))
    return CVPipelineCoreType::Unknown;
  const std::string mnemonic = operation.name.substr(9);
  if (startsWith(mnemonic, "v") || mnemonic == "load" ||
      mnemonic == "store" || mnemonic == "atomic_xchg" ||
      mnemonic == "atomic_rmw" || mnemonic == "atomic_cas")
    return CVPipelineCoreType::Vector;
  if (mnemonic.find("mmad") != std::string::npos ||
      mnemonic.find("matmul") != std::string::npos ||
      mnemonic.find("Conv") != std::string::npos || mnemonic == "fixpipe")
    return CVPipelineCoreType::Cube;
  return CVPipelineCoreType::Unknown;
}

inline CVPipelineCoreType
CVPipelineQueryCoreType(const GenericOperation &operation) {
  if (CVPipelineHasAttribute(operation, "pipeline.cubeonly"))
    return CVPipelineCoreType::Cube;
  if (CVPipelineHasAttribute(operation, "pipeline.veconly"))
    return CVPipelineCoreType::Vector;
  return CVPipelineQueryCoreTypeFromSemantics(operation);
}

inline bool CVPipelineIsDescendant(const GenericModule &module,
                                   int operation, int ancestor) {
  int current = operation;
  while (current >= 0) {
    if (current == ancestor)
      return true;
    current = module.operations.at(static_cast<size_t>(current)).parentId;
  }
  return false;
}

inline bool CVPipelineIllegalRegionedOp(GenericModule &module,
                                        GenericOperation &operation) {
  if (operation.regions.empty())
    return false;
  bool hasCube = false;
  bool hasVector = false;
  std::vector<int> nestedOperations;
  for (auto region = operation.regions.rbegin();
       region != operation.regions.rend(); ++region) {
    const GenericRegion &nestedRegion =
        module.regions.at(static_cast<size_t>(*region));
    for (auto block = nestedRegion.blocks.rbegin();
         block != nestedRegion.blocks.rend(); ++block) {
      const std::vector<int> &operations =
          module.blocks.at(static_cast<size_t>(*block)).operations;
      nestedOperations.insert(nestedOperations.end(), operations.rbegin(),
                              operations.rend());
    }
  }
  while (!nestedOperations.empty()) {
    const int nestedId = nestedOperations.back();
    nestedOperations.pop_back();
    const GenericOperation &nested =
        module.operations.at(static_cast<size_t>(nestedId));
    const CVPipelineCoreType core = CVPipelineQueryCoreType(nested);
    hasCube |= core == CVPipelineCoreType::Cube;
    hasVector |= core == CVPipelineCoreType::Vector;
    if (hasCube && hasVector)
      return true;
    for (auto region = nested.regions.rbegin(); region != nested.regions.rend();
         ++region) {
      const GenericRegion &nestedRegion =
          module.regions.at(static_cast<size_t>(*region));
      for (auto block = nestedRegion.blocks.rbegin();
           block != nestedRegion.blocks.rend(); ++block) {
        const std::vector<int> &operations =
            module.blocks.at(static_cast<size_t>(*block)).operations;
        nestedOperations.insert(nestedOperations.end(), operations.rbegin(),
                                operations.rend());
      }
    }
  }
  if (hasCube)
    CVPipelineSetUnitAttribute(operation, "pipeline.cubeonly");
  else if (hasVector)
    CVPipelineSetUnitAttribute(operation, "pipeline.veconly");
  return false;
}

inline void CVPipelineMarkRegionCoreTypes(GenericModule &module,
                                          const GenericOperation &loop) {
  if (loop.regions.empty())
    return;
  const GenericRegion &region =
      module.regions.at(static_cast<size_t>(loop.regions.front()));
  if (region.blocks.empty())
    return;
  const GenericBlock &body =
      module.blocks.at(static_cast<size_t>(region.blocks.front()));
  for (int operationId : body.operations) {
    GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    if (!operation.regions.empty() && CVPipelineIllegalRegionedOp(module, operation))
      return;
  }
}

inline void CVPipelineRemoveWorkspaceMultiBufferMarks(
    GenericModule &module) {
  std::map<int, const GenericOperation *> definitions;
  for (const GenericOperation &operation : module.operations)
    for (int result : operation.results)
      definitions[result] = &operation;
  for (GenericOperation &operation : module.operations) {
    if (operation.name != "annotation.mark" || operation.operands.empty() ||
        !CVPipelineHasAttribute(operation, "hivm.multi_buffer"))
      continue;
    auto definition = definitions.find(operation.operands.front());
    if (definition == definitions.end() ||
        definition->second->name != "memref_ext.alloc_workspace")
      continue;
    std::vector<std::string> kept;
    for (const std::string &entry : splitTopLevel(operation.attributes.substr(
             1, operation.attributes.size() - 2)))
      if (entry.find("hivm.multi_buffer") == std::string::npos)
        kept.push_back(entry);
    operation.attributes = "{";
    for (size_t index = 0; index < kept.size(); ++index) {
      if (index)
        operation.attributes += ", ";
      operation.attributes += kept[index];
    }
    operation.attributes += "}";
  }
}

inline GenericModule RunCVPipelining(
    GenericModule module, const CVPipeliningOptions &options) {
  if (options.disabled || options.setDepthInUnrollMode == 0 ||
      options.setDepthInUnrollMode == 1)
    return module;
  for (size_t index = 0; index < module.operations.size(); ++index) {
    GenericOperation &operation = module.operations[index];
    if (operation.name == "scf.for")
      CVPipelineMarkRegionCoreTypes(module, operation);
  }
  CVPipelineRemoveWorkspaceMultiBufferMarks(module);
  return module;
}

} // namespace cvub

#endif
