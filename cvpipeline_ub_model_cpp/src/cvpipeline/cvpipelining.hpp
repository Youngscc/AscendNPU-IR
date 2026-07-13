#ifndef CVPIPELINE_UB_MODEL_CPP_CVPIPELINING_HPP
#define CVPIPELINE_UB_MODEL_CPP_CVPIPELINING_HPP

#include "../semantic_ir/generic_rewriter.hpp"

namespace cvub {

struct CVPipeliningOptions {
  bool disabled = false;
  int setDepthInUnrollMode = -1;
  bool enableSkewMode = false;
  bool enableLazyLoading = false;
};

enum class CVPipelineCoreType { Unknown, Cube, Vector, Mixed };

inline bool CVPipelineHasAttribute(const GenericOperation &operation,
                                   const std::string &name) {
  for (const std::string &entry : splitTopLevel(
           operation.attributes.size() >= 2
               ? operation.attributes.substr(1,
                                             operation.attributes.size() - 2)
               : std::string())) {
    const size_t equal = entry.find('=');
    if (trim(entry.substr(0, equal)) == name)
      return true;
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

inline CVPipelineCoreType
CVPipelineQueryCoreType(const GenericOperation &operation) {
  if (CVPipelineHasAttribute(operation, "pipeline.cubeonly"))
    return CVPipelineCoreType::Cube;
  if (CVPipelineHasAttribute(operation, "pipeline.veconly"))
    return CVPipelineCoreType::Vector;
  const std::string dictionaries = operation.properties + operation.attributes;
  if (dictionaries.find("CUBE_AND_VECTOR") != std::string::npos)
    return CVPipelineCoreType::Mixed;
  if (dictionaries.find("CUBE_OR_VECTOR") != std::string::npos)
    return CVPipelineCoreType::Unknown;
  if (dictionaries.find("tcoretype<CUBE>") != std::string::npos ||
      dictionaries.find("tcore_type<CUBE>") != std::string::npos)
    return CVPipelineCoreType::Cube;
  if (dictionaries.find("tcoretype<VECTOR>") != std::string::npos ||
      dictionaries.find("tcore_type<VECTOR>") != std::string::npos)
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
  for (const GenericOperation &nested : module.operations) {
    if (nested.id == operation.id ||
        !CVPipelineIsDescendant(module, nested.id, operation.id))
      continue;
    const CVPipelineCoreType core = CVPipelineQueryCoreType(nested);
    hasCube |= core == CVPipelineCoreType::Cube;
    hasVector |= core == CVPipelineCoreType::Vector;
    if (hasCube && hasVector)
      return true;
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
