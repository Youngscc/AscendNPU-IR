#ifndef CVPIPELINE_UB_MODEL_CPP_GLOBAL_WORKSPACE_PLAN_HPP
#define CVPIPELINE_UB_MODEL_CPP_GLOBAL_WORKSPACE_PLAN_HPP

#include "../ir/generic_rewriter.hpp"
#include "../ir/operation_folder.hpp"

namespace cvub {

constexpr uint64_t kGlobalWorkspaceAlignmentBits = 32 * 8;

inline uint64_t AlignGlobalWorkspaceSize(uint64_t bits) {
  const uint64_t remainder = bits % kGlobalWorkspaceAlignmentBits;
  return remainder == 0 ? bits
                        : CheckedAdd(bits,
                                     kGlobalWorkspaceAlignmentBits - remainder,
                                     "global workspace aligned size");
}

inline std::string SetOperandSegmentSizes(
    const std::string &properties, const std::vector<size_t> &sizes) {
  const std::string marker = "operandSegmentSizes = array<i32:";
  const size_t begin = properties.find(marker);
  if (begin == std::string::npos)
    throw std::runtime_error(
        "GlobalWorkspacePlan: alloc_workspace has no operandSegmentSizes");
  const size_t valuesBegin = begin + marker.size();
  const size_t end = properties.find('>', valuesBegin);
  if (end == std::string::npos)
    throw std::runtime_error(
        "GlobalWorkspacePlan: malformed operandSegmentSizes");

  std::ostringstream replacement;
  for (size_t index = 0; index < sizes.size(); ++index) {
    if (index != 0)
      replacement << ", ";
    replacement << sizes[index];
  }
  std::string result = properties;
  result.replace(valuesBegin, end - valuesBegin, replacement.str());
  return result;
}

inline std::vector<int> CollectGlobalWorkspaceAllocations(
    const GenericModule &module, const GenericOperation &function) {
  std::vector<int> allocations;
  std::function<void(int)> collect = [&](int operationId) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    if (operation.name == "memref_ext.alloc_workspace")
      allocations.push_back(operationId);
    for (int regionId : operation.regions)
      for (int blockId :
           module.regions.at(static_cast<size_t>(regionId)).blocks)
        for (int child :
             module.blocks.at(static_cast<size_t>(blockId)).operations)
          collect(child);
  };
  collect(function.id);
  return allocations;
}

inline GenericModule RunGlobalWorkspacePlan(GenericModule module) {
  GenericRewriter rewriter(module);
  std::vector<int> functions;
  for (const GenericOperation &operation : module.operations)
    if (operation.name == "func.func")
      functions.push_back(operation.id);
  for (int functionId : functions) {
    const GenericOperation &function =
        module.operations.at(static_cast<size_t>(functionId));

    std::map<int, uint64_t> workspaceOffsets;
    const std::vector<int> allocations =
        CollectGlobalWorkspaceAllocations(module, function);
    for (int operationId : allocations) {
      GenericOperation &allocation =
          module.operations.at(static_cast<size_t>(operationId));
      const std::vector<size_t> segments =
          OperandSegmentSizes(allocation.properties);
      if (segments.size() != 3 || segments.front() != 1)
        throw std::runtime_error(
            "GlobalWorkspacePlan: unsupported alloc_workspace operand layout");
      if (segments[2] != 0)
        continue;
      if (allocation.operands.empty() || allocation.resultTypes.size() != 1)
        throw std::runtime_error(
            "GlobalWorkspacePlan: malformed alloc_workspace operation");

      const int workspace = allocation.operands.front();
      const uint64_t offsetBits = workspaceOffsets[workspace];
      if (offsetBits % 8 != 0)
        throw std::runtime_error(
            "GlobalWorkspacePlan: workspace offset is not byte aligned");
      const uint64_t offsetBytes = offsetBits / 8;

      const int parent = allocation.parentId;
      const int region = allocation.regionId;
      const int block = allocation.blockId;
      const size_t position = static_cast<size_t>(allocation.ordinal);
      std::optional<int> offsetValue =
          FindArithConstantValue(module, block, "index",
                                 std::to_string(offsetBytes));
      if (!offsetValue) {
        const int constant = rewriter.createOperation(
            parent, region, block, "arith.constant", {"index"}, {}, {},
            "{value = " + std::to_string(offsetBytes) + " : index}");
        offsetValue = module.operations.at(static_cast<size_t>(constant))
                          .results.front();
        rewriter.insertToBlock(block, position, constant);
      }

      GenericOperation &updated =
          module.operations.at(static_cast<size_t>(operationId));
      updated.operands.push_back(*offsetValue);
      updated.operandTypes.push_back("index");
      std::vector<size_t> updatedSegments = segments;
      updatedSegments[2] = 1;
      updated.properties =
          SetOperandSegmentSizes(updated.properties, updatedSegments);

      workspaceOffsets[workspace] = CheckedAdd(
          offsetBits,
          AlignGlobalWorkspaceSize(GetBufferConstBits(
              updated.resultTypes.front())),
          "global workspace offset");
    }
    RunGreedyOperationFolder(module, functionId);
  }
  ApplyOperationSemanticsToAll(module.operations);
  return module;
}

} // namespace cvub

#endif
