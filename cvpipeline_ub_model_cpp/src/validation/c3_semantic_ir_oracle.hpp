#ifndef CVPIPELINE_UB_MODEL_CPP_C3_SEMANTIC_IR_ORACLE_HPP
#define CVPIPELINE_UB_MODEL_CPP_C3_SEMANTIC_IR_ORACLE_HPP

#include "bufferized_semantic_ir_oracle.hpp"
#include "hivm_decompose_op_oracle.hpp"
#include "../suffix/c3_semantic_ir.hpp"

namespace cvub {

using C3OperationSignatures = std::map<std::string, int64_t>;

struct C3RewriteValidation {
  size_t checkedSignatures = 0;
  std::vector<std::string> errors;
};

struct C3FunctionAllocation {
  size_t globalOrdinal = 0;
  std::string type;
};

inline std::vector<C3FunctionAllocation> C3FunctionAllocations(
    const C1SemanticModule &module, const std::string &function) {
  std::vector<C3FunctionAllocation> result;
  size_t global = 0;
  for (const C1OperationRecord &operation : module.operations) {
    if (operation.name != "memref.alloc")
      continue;
    const C1OperationRecord *owner = C1EnclosingFunction(module, operation);
    for (const std::string &type : operation.resultTypes) {
      if (!IsMemRefType(type))
        continue;
      if (owner && C1FunctionName(*owner) == function)
        result.push_back({global, type});
      ++global;
    }
  }
  return result;
}

inline std::map<std::string, std::string> C3ActualBufferLabels(
    const std::vector<C3FunctionAllocation> &before,
    const std::vector<C3FunctionAllocation> &after, bool afterPass) {
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

inline std::string C3NormalizeBuffer(
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
    const std::string normalized = C3NormalizeBuffer(item, labels);
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

inline std::string C3OperationSignature(
    const std::string &name, const std::vector<std::string> &buffers) {
  std::string result = name;
  for (const std::string &buffer : buffers)
    result += "\t" + buffer;
  return result;
}

inline C3OperationSignatures C3ActualOperationSignatures(
    const C1SemanticModule &module, const std::string &function,
    const std::map<std::string, std::string> &labels) {
  C3OperationSignatures result;
  AfterBufferRootResolver roots(module);
  for (const C1OperationRecord &operation : module.operations) {
    const C1OperationRecord *owner = C1EnclosingFunction(module, operation);
    if (!owner || C1FunctionName(*owner) != function ||
        !IsUBRelevantDecomposeOperation(operation))
      continue;
    std::vector<std::string> buffers;
    for (size_t index = 0; index < operation.operands.size(); ++index) {
      if (index >= operation.operandTypes.size() ||
          !IsMemRefType(operation.operandTypes[index]))
        continue;
      const std::string normalized =
          C3NormalizeBuffer(roots.root(operation.operands[index]), labels);
      if (!normalized.empty())
        buffers.push_back(normalized);
    }
    if (!buffers.empty())
      ++result[C3OperationSignature(operation.name, buffers)];
  }
  return result;
}

inline int C3AllocationSourceOperation(const BufferAllocation &allocation) {
  const size_t colon = allocation.source.find(':');
  if (colon == std::string::npos)
    return -1;
  const size_t end = allocation.source.find(':', colon + 1);
  if (end == std::string::npos)
    return -1;
  return std::stoi(allocation.source.substr(colon + 1, end - colon - 1));
}

inline std::map<std::string, std::string> C3ModelBufferLabels(
    const C3SemanticIR &model, const std::string &function,
    const std::map<std::string, std::string> &baseLabels) {
  std::map<std::string, std::string> result = baseLabels;
  size_t inserted = 0;
  std::map<int, size_t> ownerOrdinals;
  for (const DecomposeBufferAllocation &allocation :
       model.decomposeAllocations) {
    const C1OperationRecord &operation = model.bufferized.logicalModule.operations
                                             .at(static_cast<size_t>(
                                                 allocation.ownerOperation));
    const C1OperationRecord *owner = C1EnclosingFunction(
        model.bufferized.logicalModule, operation);
    const size_t ordinal = ownerOrdinals[allocation.ownerOperation]++;
    if (owner && C1FunctionName(*owner) == function)
      result["decompose:" + std::to_string(allocation.ownerOperation) + ":" +
             std::to_string(ordinal)] = "new:" + std::to_string(inserted++);
  }
  return result;
}

inline C3OperationSignatures C3ModelOperationDelta(
    const C3SemanticIR &model, const std::string &function,
    const std::map<std::string, std::string> &baseLabels) {
  const std::map<std::string, std::string> labels =
      C3ModelBufferLabels(model, function, baseLabels);
  C3OperationSignatures result;
  for (const C3OperationRewrite &rewrite : model.operationRewrites) {
    const C1OperationRecord &source = model.bufferized.logicalModule.operations
                                          .at(static_cast<size_t>(
                                              rewrite.sourceOperation));
    const C1OperationRecord *owner =
        C1EnclosingFunction(model.bufferized.logicalModule, source);
    if (!owner || C1FunctionName(*owner) != function)
      continue;
    std::vector<std::string> sourceBuffers;
    for (const std::string &buffer :
         C3OperationBuffers(model.bufferized, rewrite.sourceOperation,
                            &model.singlePoint.bufferMapping)) {
      const std::string normalized = C3NormalizeBuffer(buffer, labels);
      if (!normalized.empty())
        sourceBuffers.push_back(normalized);
    }
    if (!sourceBuffers.empty())
      --result[C3OperationSignature(rewrite.sourceName, sourceBuffers)];
    for (const C3GeneratedOperation &operation : rewrite.replacement) {
      std::vector<std::string> buffers;
      for (const std::string &buffer : operation.buffers) {
        const std::string normalized = C3NormalizeBuffer(buffer, labels);
        if (!normalized.empty())
          buffers.push_back(normalized);
      }
      if (!buffers.empty())
        ++result[C3OperationSignature(operation.name, buffers)];
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

inline C3RewriteValidation ValidateC3OperationRewrites(
    const C3SemanticIR &model, const C1SemanticModule &before,
    const C1SemanticModule &after, const std::string &function) {
  C3RewriteValidation validation;
  const std::vector<C3FunctionAllocation> beforeAllocations =
      C3FunctionAllocations(before, function);
  const std::vector<C3FunctionAllocation> afterAllocations =
      C3FunctionAllocations(after, function);
  C3OperationSignatures actual = C3ActualOperationSignatures(
      after, function,
      C3ActualBufferLabels(beforeAllocations, afterAllocations, true));
  const C3OperationSignatures baseline = C3ActualOperationSignatures(
      before, function,
      C3ActualBufferLabels(beforeAllocations, beforeAllocations, false));
  for (const auto &[signature, count] : baseline) {
    actual[signature] -= count;
    if (actual[signature] == 0)
      actual.erase(signature);
  }
  const C3OperationSignatures expected =
      C3ModelOperationDelta(
          model, function,
          C3ActualBufferLabels(beforeAllocations, beforeAllocations, false));
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
CollectNonContiguousReshapeToCopyOracle(const C1SemanticModule &before,
                                        const C1SemanticModule &after) {
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
    const C1SemanticModule &before, const C1SemanticModule &after) {
  const auto count = [](const C1SemanticModule &module) {
    std::map<std::string, size_t> result;
    for (const C1OperationRecord &operation : module.operations) {
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
