#ifndef CVPIPELINE_UB_MODEL_CPP_POST_BUFFERIZATION_REWRITE_ORACLE_HPP
#define CVPIPELINE_UB_MODEL_CPP_POST_BUFFERIZATION_REWRITE_ORACLE_HPP

#include "bufferized_semantic_ir_oracle.hpp"
#include "hivm_decompose_op_oracle.hpp"
#include "../passes/post_bufferization_rewrites.hpp"

namespace cvub {

using OperationSignatures = std::map<std::string, int64_t>;

struct PostBufferizationRewriteValidation {
  size_t checkedSignatures = 0;
  std::vector<std::string> errors;
};

struct FunctionAllocation {
  size_t globalOrdinal = 0;
  std::string type;
};

inline std::vector<FunctionAllocation> FunctionAllocations(
    const GenericModule &module, const std::string &function) {
  std::vector<FunctionAllocation> result;
  size_t global = 0;
  for (const GenericOperation &operation : module.operations) {
    if (operation.name != "memref.alloc")
      continue;
    const GenericOperation *owner = EnclosingFunction(module, operation);
    for (const std::string &type : operation.resultTypes) {
      if (!IsMemRefType(type))
        continue;
      if (owner && FunctionSymName(*owner) == function)
        result.push_back({global, type});
      ++global;
    }
  }
  return result;
}

inline std::map<std::string, std::string> ActualBufferLabels(
    const std::vector<FunctionAllocation> &before,
    const std::vector<FunctionAllocation> &after, bool afterPass) {
  std::map<std::string, std::string> result;
  if (!afterPass) {
    for (size_t index = 0; index < before.size(); ++index)
      result[LocalBufferId(before[index].globalOrdinal)] =
          "base:" + std::to_string(index);
    return result;
  }
  std::vector<std::vector<size_t>> lcs(
      before.size() + 1, std::vector<size_t>(after.size() + 1));
  for (size_t left = before.size(); left > 0; --left)
    for (size_t right = after.size(); right > 0; --right)
      lcs[left - 1][right - 1] =
          before[left - 1].type == after[right - 1].type
              ? lcs[left][right] + 1
              : std::max(lcs[left][right - 1], lcs[left - 1][right]);
  size_t left = 0;
  size_t right = 0;
  size_t inserted = 0;
  while (right < after.size()) {
    std::string label;
    if (left < before.size() && before[left].type == after[right].type) {
      label = "base:" + std::to_string(left++);
    } else if (left < before.size() &&
               lcs[left + 1][right] >= lcs[left][right + 1]) {
      ++left;
      continue;
    } else {
      label = "new:" + std::to_string(inserted++);
    }
    result[LocalBufferId(after[right].globalOrdinal)] = std::move(label);
    ++right;
  }
  return result;
}

inline std::string NormalizeBufferLabel(
    const std::string &buffer,
    const std::map<std::string, std::string> &labels) {
  auto found = labels.find(buffer);
  if (found != labels.end())
    return found->second;
  if (!startsWith(buffer, "choice(") || buffer.back() != ')')
    return "";
  std::vector<std::string> alternatives;
  for (const std::string &item :
       splitTopLevel(buffer.substr(7, buffer.size() - 8))) {
    const std::string normalized = NormalizeBufferLabel(item, labels);
    if (!normalized.empty())
      alternatives.push_back(normalized);
  }
  if (alternatives.empty())
    return "";
  std::sort(alternatives.begin(), alternatives.end());
  alternatives.erase(std::unique(alternatives.begin(), alternatives.end()),
                     alternatives.end());
  std::string result = "choice(";
  for (size_t index = 0; index < alternatives.size(); ++index) {
    if (index != 0)
      result += ',';
    result += alternatives[index];
  }
  return result + ')';
}

inline std::string OperationSignature(
    const std::string &name, const std::vector<std::string> &buffers) {
  std::string result = name;
  for (const std::string &buffer : buffers)
    result += "\t" + buffer;
  return result;
}

inline OperationSignatures ActualOperationSignatures(
    const GenericModule &module, const std::string &function,
    const std::map<std::string, std::string> &labels) {
  OperationSignatures result;
  AfterBufferRootResolver roots(module);
  for (const GenericOperation &operation : module.operations) {
    const GenericOperation *owner = EnclosingFunction(module, operation);
    if (!owner || FunctionSymName(*owner) != function ||
        !IsUBRelevantDecomposeOperation(operation))
      continue;
    std::vector<std::string> buffers;
    for (size_t index = 0; index < operation.operands.size(); ++index) {
      if (index >= operation.operandTypes.size() ||
          !IsMemRefType(operation.operandTypes[index]))
        continue;
      const std::string normalized =
          NormalizeBufferLabel(roots.root(operation.operands[index]), labels);
      if (!normalized.empty())
        buffers.push_back(normalized);
    }
    if (!buffers.empty())
      ++result[OperationSignature(operation.name, buffers)];
  }
  return result;
}

inline int RewriteAllocationSourceOperation(const BufferAllocation &allocation) {
  const size_t colon = allocation.source.find(':');
  if (colon == std::string::npos)
    return -1;
  const size_t end = allocation.source.find(':', colon + 1);
  if (end == std::string::npos)
    return -1;
  return std::stoi(allocation.source.substr(colon + 1, end - colon - 1));
}

inline std::map<std::string, std::string> ModelBufferLabels(
    const PostBufferizationRewriteState &model, const std::string &function,
    const std::map<std::string, std::string> &baseLabels) {
  std::map<std::string, std::string> result = baseLabels;
  size_t inserted = 0;
  std::map<int, size_t> ownerOrdinals;
  for (const DecomposeBufferAllocation &allocation :
       model.decomposeAllocations) {
    const GenericOperation &operation = model.bufferized.logicalModule.operations
                                             .at(static_cast<size_t>(
                                                 allocation.ownerOperation));
    const GenericOperation *owner = EnclosingFunction(
        model.bufferized.logicalModule, operation);
    const size_t ordinal = ownerOrdinals[allocation.ownerOperation]++;
    if (owner && FunctionSymName(*owner) == function)
      result["decompose:" + std::to_string(allocation.ownerOperation) + ":" +
             std::to_string(ordinal)] = "new:" + std::to_string(inserted++);
  }
  return result;
}

inline OperationSignatures ModelOperationDelta(
    const PostBufferizationRewriteState &model, const std::string &function,
    const std::map<std::string, std::string> &baseLabels) {
  const std::map<std::string, std::string> labels =
      ModelBufferLabels(model, function, baseLabels);
  OperationSignatures result;
  for (const OperationRewriteDelta &rewrite : model.operationRewrites) {
    const GenericOperation &source = model.bufferized.logicalModule.operations
                                          .at(static_cast<size_t>(
                                              rewrite.sourceOperation));
    const GenericOperation *owner =
        EnclosingFunction(model.bufferized.logicalModule, source);
    if (!owner || FunctionSymName(*owner) != function)
      continue;
    std::vector<std::string> sourceBuffers;
    for (const std::string &buffer :
         OperationBufferOperands(model.bufferized, rewrite.sourceOperation,
                            &model.singlePoint.bufferMapping)) {
      const std::string normalized = NormalizeBufferLabel(buffer, labels);
      if (!normalized.empty())
        sourceBuffers.push_back(normalized);
    }
    if (!sourceBuffers.empty())
      --result[OperationSignature(rewrite.sourceName, sourceBuffers)];
    for (const GeneratedOperationRewrite &operation : rewrite.replacement) {
      std::vector<std::string> buffers;
      for (const std::string &buffer : operation.buffers) {
        const std::string normalized = NormalizeBufferLabel(buffer, labels);
        if (!normalized.empty())
          buffers.push_back(normalized);
      }
      if (!buffers.empty())
        ++result[OperationSignature(operation.name, buffers)];
    }
  }
  for (auto iterator = result.begin(); iterator != result.end();) {
    if (iterator->second == 0)
      iterator = result.erase(iterator);
    else
      ++iterator;
  }
  return result;
}

inline PostBufferizationRewriteValidation ValidatePostBufferizationOperationRewrites(
    const PostBufferizationRewriteState &model, const GenericModule &before,
    const GenericModule &after, const std::string &function) {
  PostBufferizationRewriteValidation validation;
  const std::vector<FunctionAllocation> beforeAllocations =
      FunctionAllocations(before, function);
  const std::vector<FunctionAllocation> afterAllocations =
      FunctionAllocations(after, function);
  OperationSignatures actual = ActualOperationSignatures(
      after, function,
      ActualBufferLabels(beforeAllocations, afterAllocations, true));
  const OperationSignatures baseline = ActualOperationSignatures(
      before, function,
      ActualBufferLabels(beforeAllocations, beforeAllocations, false));
  for (const auto &[signature, count] : baseline) {
    actual[signature] -= count;
    if (actual[signature] == 0)
      actual.erase(signature);
  }
  const OperationSignatures expected =
      ModelOperationDelta(
          model, function,
          ActualBufferLabels(beforeAllocations, beforeAllocations, false));
  validation.checkedSignatures = actual.size() + expected.size();
  if (actual != expected) {
    for (const auto &[signature, count] : expected)
      if (actual.find(signature) == actual.end() ||
          actual.at(signature) != count)
        validation.errors.push_back("expected " + std::to_string(count) +
                                    " x " + signature);
    for (const auto &[signature, count] : actual)
      if (expected.find(signature) == expected.end() ||
          expected.at(signature) != count)
        validation.errors.push_back("actual " + std::to_string(count) +
                                    " x " + signature);
  }
  return validation;
}

inline std::vector<NonContiguousReshapeCopy>
CollectNonContiguousReshapeToCopyOracle(const GenericModule &before,
                                        const GenericModule &after) {
  const std::vector<BufferAllocation> lhs =
      CollectBufferAllocationOracle(before);
  const std::vector<BufferAllocation> rhs = CollectBufferAllocationOracle(after);
  std::vector<std::vector<size_t>> lcs(
      lhs.size() + 1, std::vector<size_t>(rhs.size() + 1));
  for (size_t left = lhs.size(); left > 0; --left)
    for (size_t right = rhs.size(); right > 0; --right)
      lcs[left - 1][right - 1] =
          lhs[left - 1].type == rhs[right - 1].type
              ? lcs[left][right] + 1
              : std::max(lcs[left][right - 1], lcs[left - 1][right]);
  std::vector<NonContiguousReshapeCopy> result;
  size_t left = 0;
  size_t right = 0;
  while (right < rhs.size()) {
    if (left < lhs.size() && lhs[left].type == rhs[right].type) {
      ++left;
      ++right;
    } else if (left < lhs.size() &&
               lcs[left + 1][right] >= lcs[left][right + 1]) {
      ++left;
    } else {
      result.push_back({-1, rhs[right++].type});
    }
  }
  return result;
}

inline std::map<std::string, size_t> CollectCanonicalizedIterArgResultsOracle(
    const GenericModule &before, const GenericModule &after) {
  const auto count = [](const GenericModule &module) {
    std::map<std::string, size_t> result;
    for (const GenericOperation &operation : module.operations) {
      if (operation.name != "scf.if" && operation.name != "scf.for" &&
          operation.name != "scf.while")
        continue;
      for (const std::string &type : operation.resultTypes)
        if (IsMemRefType(type) || IsTensorType(type))
          ++result[operation.name];
    }
    return result;
  };
  const std::map<std::string, size_t> lhs = count(before);
  const std::map<std::string, size_t> rhs = count(after);
  std::map<std::string, size_t> result;
  for (const auto &[name, beforeCount] : lhs) {
    const auto found = rhs.find(name);
    const size_t afterCount = found == rhs.end() ? 0 : found->second;
    if (beforeCount > afterCount)
      result[name] = beforeCount - afterCount;
  }
  return result;
}

inline std::string SerializeCanonicalizedIterArgResults(
    const std::map<std::string, size_t> &results) {
  std::ostringstream output;
  output << "CANONICALIZED_ITER_ARG_RESULTS\t1\n";
  for (const auto &[name, count] : results)
    output << "DROP\t" << HexEncode(name) << '\t' << count << '\n';
  return output.str();
}

} // namespace cvub

#endif
