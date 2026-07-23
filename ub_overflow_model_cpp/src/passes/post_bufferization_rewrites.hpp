#ifndef CVPIPELINE_UB_MODEL_CPP_POST_BUFFERIZATION_REWRITES_HPP
#define CVPIPELINE_UB_MODEL_CPP_POST_BUFFERIZATION_REWRITES_HPP

#include "convert_non_contiguous_reshape_to_copy.hpp"
#include "hivm_opt_single_point.hpp"

namespace cvub {

struct GeneratedOperationRewrite {
  std::string name;
  std::vector<std::string> buffers;
  bool preserveSourceProperties = false;
  bool decomposedScalarVSub = false;
};

struct OperationRewriteDelta {
  int sourceOperation = -1;
  std::string sourceName;
  std::vector<GeneratedOperationRewrite> replacement;
};

struct PostBufferizationRewriteState {
  BufferizedSemanticIR bufferized;
  HIVMOptSinglePointResult singlePoint;
  std::vector<DecomposeBufferAllocation> decomposeAllocations;
  DecomposeOperationDelta decomposeOperationDelta;
  std::vector<NonContiguousReshapeCopy> nonContiguousReshapeCopies;
  std::vector<OperationRewriteDelta> operationRewrites;
};

inline std::vector<std::string> OperationBufferOperands(
    const BufferizedSemanticIR &module, int operation,
    const std::map<std::string, std::string> *mapping = nullptr) {
  if (module.indexedAccessCount == module.accesses.size() && operation >= 0 &&
      static_cast<size_t>(operation) < module.buffersByOperation.size()) {
    const std::vector<std::string> &indexed =
        module.buffersByOperation[static_cast<size_t>(operation)];
    std::vector<std::string> result;
    result.reserve(indexed.size());
    for (const std::string &sourceBuffer : indexed) {
      if (sourceBuffer.empty())
        continue;
      const std::string *buffer = &sourceBuffer;
      if (mapping) {
        auto found = mapping->find(sourceBuffer);
        if (found != mapping->end())
          buffer = &found->second;
      }
      result.push_back(*buffer);
    }
    return result;
  }
  std::vector<std::pair<int, std::string>> ordered;
  ForEachBufferizedOperationBuffer(
      module, operation,
      [&](size_t operand, const std::string &sourceBuffer) {
      std::string buffer = sourceBuffer;
      if (mapping) {
        auto found = mapping->find(buffer);
        if (found != mapping->end())
          buffer = found->second;
      }
      ordered.push_back({static_cast<int>(operand), std::move(buffer)});
      });
  std::sort(ordered.begin(), ordered.end());
  std::vector<std::string> result;
  for (const auto &[operand, buffer] : ordered) {
    (void)operand;
    result.push_back(buffer);
  }
  return result;
}

inline std::vector<OperationRewriteDelta> BuildOperationRewriteDeltas(
    const BufferizedSemanticIR &module,
    const std::vector<DecomposeBufferAllocation> &allocations,
    const std::map<std::string, std::string> &mapping) {
  std::map<int, std::vector<std::string>> generatedBuffers;
  for (const DecomposeBufferAllocation &allocation : allocations) {
    std::vector<std::string> &buffers =
        generatedBuffers[allocation.ownerOperation];
    buffers.push_back("decompose:" +
                      std::to_string(allocation.ownerOperation) + ":" +
                      std::to_string(buffers.size()));
  }

  std::vector<OperationRewriteDelta> result;
  for (const GenericOperation &operation : module.logicalModule.operations) {
    if (module.preBufferizationCSE.erasedOperations.count(operation.id) != 0)
      continue;
    const std::vector<std::string> buffers =
        OperationBufferOperands(module, operation.id, &mapping);
    static const std::vector<std::string> noTemporaryBuffers;
    auto generated = generatedBuffers.find(operation.id);
    const std::vector<std::string> &temporary =
        generated == generatedBuffers.end() ? noTemporaryBuffers
                                            : generated->second;
    OperationRewriteDelta rewrite{operation.id, operation.name, {}};
    if (operation.name == "hivm.hir.vcast" && !temporary.empty() &&
        buffers.size() >= 2) {
      if (temporary.size() == 1) {
        rewrite.replacement = {{"hivm.hir.vcast",
                                {buffers.front(), temporary[0]}},
                               {"hivm.hir.vcast",
                                {temporary[0], buffers.back()}, true}};
      } else if (temporary.size() == 2) {
        rewrite.replacement = {
            {"hivm.hir.vcast", {buffers.front(), temporary[0]}},
            {"hivm.hir.vbrc", {temporary[1]}},
            {"hivm.hir.vcmp",
             {temporary[0], temporary[1], buffers.back()}}};
      }
    } else if (operation.name == "hivm.hir.vcmp" &&
               temporary.size() == 3 && buffers.size() >= 3) {
      std::vector<std::string> compareInputs(buffers.begin(), buffers.end() - 1);
      compareInputs.push_back(temporary[0]);
      rewrite.replacement = {
          {"hivm.hir.vcmp", compareInputs},
          {"hivm.hir.vcast", {temporary[0], temporary[1]}},
          {"hivm.hir.vbrc", {temporary[2]}},
          {"hivm.hir.vcmp",
           {temporary[1], temporary[2], buffers.back()}, true}};
    } else if (operation.name == "hivm.hir.vsub" &&
               operation.operandTypes.size() > 1 &&
               !IsTensorType(operation.operandTypes[1]) &&
               !IsMemRefType(operation.operandTypes[1])) {
      rewrite.replacement = {
          {"hivm.hir.vadd", buffers, false, true}};
    } else if (operation.name == "hivm.hir.vrec" && buffers.size() >= 2) {
      rewrite.replacement = {{"hivm.hir.vbrc", {buffers.back()}},
                             {"hivm.hir.vdiv",
                              {buffers.back(), buffers.front(),
                               buffers.back()}}};
    } else if (operation.name == "hivm.hir.atomic_rmw" &&
               temporary.size() == 2 && buffers.size() >= 2) {
      const std::string kind = DecomposeEnumValue(
          FindDictionaryValue(operation.properties, "atomic_kind"));
      rewrite.replacement = {
          {"hivm.hir.load", {buffers.back(), temporary[0]}},
          {"hivm.hir.v" + kind,
           {buffers.front(), temporary[0], temporary[1]}},
          {"hivm.hir.store", {temporary[1], buffers.back()}}};
    } else if (operation.name == "hivm.hir.atomic_cas" &&
               temporary.size() == 3 && buffers.size() >= 3) {
      rewrite.replacement = {
          {"hivm.hir.load", {buffers.back(), temporary[0]}},
          {"hivm.hir.vcmp", {temporary[0], buffers[0], temporary[1]}},
          {"hivm.hir.vsel",
           {temporary[1], buffers[1], temporary[0], temporary[2]}},
          {"hivm.hir.store", {temporary[2], buffers.back()}}};
    } else if (operation.name == "hivm.hir.atomic_xchg" &&
               !temporary.empty() && buffers.size() >= 2) {
      rewrite.replacement = {
          {"hivm.hir.load", {buffers.back(), temporary[0]}},
          {"hivm.hir.store", {buffers.front(), buffers.back()}},
          {"hivm.hir.copy", {temporary[0], buffers.front()}}};
    }
    if (!rewrite.replacement.empty())
      result.push_back(std::move(rewrite));
  }
  return result;
}

inline PostBufferizationRewriteState BuildPostBufferizationRewriteState(BufferizedSemanticIR bufferized) {
  PostBufferizationRewriteState result;
  result.singlePoint = ModelHIVMOptSinglePoint(bufferized);
  HIVMDecomposeResult decompose =
      ModelHIVMDecompose(bufferized.logicalModule);
  result.decomposeAllocations = std::move(decompose.allocations);
  result.decomposeOperationDelta = std::move(decompose.operationDelta);
  result.nonContiguousReshapeCopies =
      ModelConvertNonContiguousReshapeToCopy(bufferized.logicalModule);
  result.operationRewrites =
      BuildOperationRewriteDeltas(bufferized, result.decomposeAllocations,
                               result.singlePoint.bufferMapping);
  result.bufferized = std::move(bufferized);
  return result;
}

inline std::string SerializePostBufferizationRewriteState(const PostBufferizationRewriteState &module) {
  std::ostringstream output;
  output << "POST_BUFFERIZATION_REWRITE_STATE\t1\n";
  output << SerializeBufferizedSemanticIR(module.bufferized);
  output << "POST_SINGLE_POINT_ALLOCATIONS\t1\n";
  for (size_t index = 0; index < module.singlePoint.allocations.size(); ++index)
    output << "SINGLE_POINT_ALLOC\t" << index << '\t'
           << HexEncode(module.singlePoint.allocations[index].type) << '\n';
  for (int operation : module.singlePoint.scalarizedOperations)
    output << "SCALARIZED_OP\t" << operation << '\t'
           << HexEncode(module.bufferized.logicalModule.operations.at(
                            static_cast<size_t>(operation)).name)
           << '\n';
  for (const auto &[oldBuffer, newBuffer] :
       module.singlePoint.bufferMapping)
    output << "SINGLE_POINT_BUFFER_MAP\t" << HexEncode(oldBuffer) << '\t'
           << HexEncode(newBuffer) << '\n';
  for (const auto &[operation, count] :
       module.singlePoint.canonicalizedIterArgResults)
    output << "CANONICALIZED_ITER_ARG_RESULTS\t" << HexEncode(operation) << '\t'
           << count << '\n';
  output << SerializeDecomposeBufferAllocations(module.decomposeAllocations);
  output << SerializeDecomposeOperationDelta(module.decomposeOperationDelta);
  for (const NonContiguousReshapeCopy &copy :
       module.nonContiguousReshapeCopies)
    output << "NON_CONTIGUOUS_RESHAPE_COPY\t" << copy.collapseOperation
           << '\t' << HexEncode(copy.type) << '\n';
  for (const OperationRewriteDelta &rewrite : module.operationRewrites) {
    output << "REWRITE\t" << rewrite.sourceOperation << '\t'
           << HexEncode(rewrite.sourceName) << '\n';
    for (const GeneratedOperationRewrite &operation : rewrite.replacement) {
      output << "GENERATED_OP\t" << HexEncode(operation.name);
      for (const std::string &buffer : operation.buffers)
        output << '\t' << HexEncode(buffer);
      output << '\n';
    }
  }
  return output.str();
}

} // namespace cvub

#endif
