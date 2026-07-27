#ifndef CVPIPELINE_UB_MODEL_CPP_INFER_HIVM_DATA_LAYOUT_HPP
#define CVPIPELINE_UB_MODEL_CPP_INFER_HIVM_DATA_LAYOUT_HPP

#include "../pipeline/bufferized_semantic_ir.hpp"
#include "split_mix_kernel.hpp"

namespace cvub {

inline std::optional<size_t>
InferHIVMDataLayoutRank(const std::string &type,
                        PipelineMetadataCache &metadata) {
  std::string memrefType = type;
  if (IsTensorType(memrefType))
    memrefType = ConvertTensorToMemRefType(memrefType);
  const std::optional<MemRefTypeModel> parsed = metadata.memRefType(memrefType);
  if (!parsed)
    return std::nullopt;
  return parsed->shape.size();
}

inline bool InferHIVMDataLayoutRunsOnFunction(
    const GenericOperation &function, PipelineMetadataCache &metadata) {
  if (function.name != "func.func")
    return false;
  const std::string functionKind = SplitMixEnumValue(
      metadata.dictionaryValue(function.attributes, "hacc.function_kind"));
  if (functionKind == "HOST")
    return false;
  const std::string coreType = SplitMixEnumValue(
      metadata.dictionaryValue(function.attributes, "hivm.func_core_type"));
  // InferHIVMDataLayoutPass returns for a missing core attribute and AIV.  A
  // MIX function reaches this model-side preflight before SplitMixKernel; its
  // Cube projection becomes AIC and therefore executes the production pass.
  return !coreType.empty() && coreType != "AIV";
}

inline std::set<std::string>
InferHIVMDataLayoutBufferAlternatives(const std::string &buffer) {
  std::set<std::string> result;
  if (!startsWith(buffer, "choice(") || buffer.back() != ')') {
    result.insert(buffer);
    return result;
  }
  for (const std::string &item :
       splitTopLevel(buffer.substr(7, buffer.size() - 8))) {
    const std::set<std::string> nested =
        InferHIVMDataLayoutBufferAlternatives(item);
    result.insert(nested.begin(), nested.end());
  }
  return result;
}

inline bool AnyInferHIVMDataLayoutBufferAlternativeIn(
    const std::string &buffer,
    const std::unordered_set<std::string> &targets) {
  for (const std::string &alternative :
       InferHIVMDataLayoutBufferAlternatives(buffer))
    if (targets.count(alternative) != 0)
      return true;
  return false;
}

// Reproduce the verifier-visible scalar-store failure caused by
// InferHIVMDataLayout's real rewrite order. MmadL1 is an anchor: a rank-2 A,
// B, or C operand has DOTA_ND/DOTB_ND/DOTC_ND as its current layout and zN/nZ
// as its target layout. The root allocation is consequently rewritten to a
// rank-4 fractal memref. memref.store is intentionally not in the pass's
// CopyOp whitelist, so its destination is remapped while its scalar indices
// are left unchanged; MLIR then rejects a two-index store into that rank-4
// allocation.
//
// The lightweight pipeline retains only the AIV projection after
// SplitMixKernel. Run this check on an actual Cube projection made from the
// last common MIX boundary, using the same OneShot physical-buffer alias
// relation, so deterministic failures in the discarded AIC projection are
// not silently reported as successful UB plans. This is a semantic projection
// of the production pass, not a kernel or diagnostic-name special case.
inline void ValidateInferHIVMDataLayoutScalarStores(
    const BufferizedSemanticIR &bufferized) {
  const GenericModule &module = bufferized.logicalModule;
  const GenericModuleAnalysisIndexes &analysis =
      bufferized.logicalContext.analysis;
  PipelineMetadataCache &metadata = bufferized.logicalContext.metadata;
  analysis.ensureCompatible(module);

  std::vector<bool> eligibleFunctions(module.operations.size(), false);
  for (const GenericOperation &operation : module.operations)
    if (operation.name == "func.func")
      eligibleFunctions[static_cast<size_t>(operation.id)] =
          InferHIVMDataLayoutRunsOnFunction(operation, metadata);

  std::unordered_set<std::string> rankFourAnchorBuffers;
  for (const GenericOperation &operation : module.operations) {
    if (operation.name != "hivm.hir.mmadL1")
      continue;
    const int function = analysis.enclosingFunctionId(operation.id);
    if (function < 0 ||
        !eligibleFunctions.at(static_cast<size_t>(function)))
      continue;

    std::vector<size_t> anchorOperands = {0, 1};
    const std::vector<size_t> &inits = metadata.dpsInitOperandIndices(
        operation.name, operation.operands.size(), operation.properties);
    if (!inits.empty())
      anchorOperands.push_back(inits.front());
    for (size_t operand : anchorOperands) {
      if (operand >= operation.operandTypes.size())
        continue;
      const std::optional<size_t> rank = InferHIVMDataLayoutRank(
          operation.operandTypes[operand], metadata);
      // MmadL1's real layout interface accepts rank 2 (ND) or rank 4
      // (already fractal). Only rank 2 is rewritten to rank 4.
      if (!rank || *rank != 2)
        continue;
      const std::string *buffer = FindBufferizedOperationBuffer(
          bufferized, operation.id, operand);
      if (!buffer)
        continue;
      for (const std::string &alternative :
           InferHIVMDataLayoutBufferAlternatives(*buffer))
        rankFourAnchorBuffers.insert(alternative);
    }
  }
  if (rankFourAnchorBuffers.empty())
    return;

  for (const GenericOperation &operation : module.operations) {
    const int function = analysis.enclosingFunctionId(operation.id);
    if (function < 0 ||
        !eligibleFunctions.at(static_cast<size_t>(function)))
      continue;

    size_t destinationOperand = 0;
    size_t indexCount = 0;
    if (operation.name == "memref.store") {
      if (operation.operands.size() < 2)
        continue;
      destinationOperand = 1;
      indexCount = operation.operands.size() - 2;
    } else if (operation.name == "tensor.insert") {
      // OneShotBufferize lowers a materialized tensor.insert to the same
      // scalar memref.store(value, destination, indices...) checked above.
      if (operation.operands.size() < 2 || operation.results.empty() ||
          analysis.users(operation.results.front()).empty())
        continue;
      destinationOperand = 1;
      indexCount = operation.operands.size() - 2;
    } else {
      continue;
    }

    const std::string *destination = FindBufferizedOperationBuffer(
        bufferized, operation.id, destinationOperand);
    if (!destination ||
        !AnyInferHIVMDataLayoutBufferAlternativeIn(
            *destination, rankFourAnchorBuffers) ||
        indexCount == 4)
      continue;
    throw std::runtime_error(
        "'memref.store' op store index operand count not equal to memref "
        "rank");
  }
}

inline bool MayContainAICProjection(const GenericModule &module) {
  return std::any_of(
      module.operations.begin(), module.operations.end(),
      [](const GenericOperation &operation) {
        return IsSplitMixFunction(operation) ||
               (operation.name == "func.func" &&
                SplitMixEnumValue(FindDictionaryValue(
                    operation.attributes, "hivm.func_core_type")) == "AIC");
      });
}

inline void ValidateInferHIVMDataLayoutOnAICProjection(
    const GenericModule &aicProjection) {
  const bool hasMmadL1 = std::any_of(
      aicProjection.operations.begin(), aicProjection.operations.end(),
      [](const GenericOperation &operation) {
        return operation.name == "hivm.hir.mmadL1";
      });
  if (!hasMmadL1)
    return;

  GenericModule validationModule = aicProjection;
  OneShotBufferizationResult oneShot =
      RunOneShotBufferize(validationModule);
  BufferizedSemanticIR bufferized =
      BuildBufferizedSemanticIR(std::move(validationModule),
                                std::move(oneShot));
  ValidateInferHIVMDataLayoutScalarStores(bufferized);
}

inline void ValidateInferHIVMDataLayoutAICProjection(
    const GenericModule &module) {
  if (!MayContainAICProjection(module))
    return;

  // SplitMixKernel's greedy cleanup is part of the observable production
  // behavior: vector-only scalar insert loops disappear from AIC, while an
  // insert loop whose result feeds MmadL1 remains and can expose the rank
  // rewrite verifier failure.  Validate the projected tree rather than an
  // approximation over the unsplit MIX function.
  const GenericModule aicProjection =
      RunSplitMixKernelProjection(module, SplitMixCoreType::Cube);
  ValidateInferHIVMDataLayoutOnAICProjection(aicProjection);
}

} // namespace cvub

#endif
