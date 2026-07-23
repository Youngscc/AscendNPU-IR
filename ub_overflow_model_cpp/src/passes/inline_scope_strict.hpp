#ifndef CVPIPELINE_UB_MODEL_CPP_PASSES_INLINE_SCOPE_STRICT_HPP
#define CVPIPELINE_UB_MODEL_CPP_PASSES_INLINE_SCOPE_STRICT_HPP

#include "../ir/post_pipeline_ir_utils.hpp"
#include "../pipeline/modeling_result.hpp"
#include "canonicalization_hivm_pipeline.hpp"

#include <algorithm>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace cvub {
namespace inline_scope_detail {

inline std::string UnsupportedScopeReason(const GenericModule &module,
                                          const GenericOperation &scope) {
  if (scope.regions.size() != 1)
    return "only a single-region scope is supported";
  const GenericRegion &region =
      module.regions.at(static_cast<size_t>(scope.regions.front()));
  if (region.blocks.size() != 1)
    return "only a single-block scope is supported";
  const GenericBlock &block =
      module.blocks.at(static_cast<size_t>(region.blocks.front()));
  if (!block.arguments.empty())
    return "scope block arguments are unsupported";
  if (block.operations.empty())
    return "scope body has no terminator";
  const GenericOperation &terminator = module.operations.at(
      static_cast<size_t>(block.operations.back()));
  if (terminator.name != "scope.return" || !terminator.regions.empty())
    return "scope body has an unsupported terminator";
  if (scope.results.size() != terminator.operands.size() ||
      scope.resultTypes.size() != terminator.operandTypes.size())
    return "scope results do not match terminator operands";
  return "";
}

inline void InlineOneScope(GenericModule &module, int scopeId) {
  const GenericOperation scope =
      module.operations.at(static_cast<size_t>(scopeId));
  const GenericRegion &region =
      module.regions.at(static_cast<size_t>(scope.regions.front()));
  const GenericBlock &body =
      module.blocks.at(static_cast<size_t>(region.blocks.front()));
  const std::vector<int> children = body.operations;
  const GenericOperation terminator =
      module.operations.at(static_cast<size_t>(children.back()));

  for (int child : children)
    if (child != terminator.id)
      MoveOperationBefore(module, child, scope.id);
  for (size_t index = 0; index < scope.results.size(); ++index)
    ReplaceAllUses(module, scope.results[index], terminator.operands[index]);

  GenericRewriter rewriter(module);
  rewriter.removeFromBlock(body.id, terminator.id);
  rewriter.removeFromBlock(scope.blockId, scope.id);
  module = CompactGenericModule(std::move(module));
}

inline bool IsNoInline(const GenericOperation &operation) {
  return HasUnitAttribute(operation.attributes, "no_inline") ||
         HasUnitAttribute(operation.attributes, "noinline") ||
         HasUnitAttribute(operation.properties, "no_inline") ||
         HasUnitAttribute(operation.properties, "noinline");
}

inline std::string CalleeName(const GenericOperation &call) {
  std::string callee = IRDictionaryValue(call.properties, "callee");
  if (callee.empty())
    callee = IRDictionaryValue(call.attributes, "callee");
  callee = trim(std::move(callee));
  if (!callee.empty() && callee.front() == '@')
    callee.erase(callee.begin());
  return UnquoteIRString(std::move(callee));
}

inline const GenericOperation *FindFunction(const GenericModule &module,
                                            const std::string &symbol) {
  for (const GenericOperation &operation : module.operations)
    if (operation.name == "func.func" &&
        FunctionSymbolName(operation) == symbol)
      return &operation;
  return nullptr;
}

inline std::set<std::string> DirectCallees(const GenericModule &module,
                                           const GenericOperation &root) {
  std::set<std::string> result;
  std::function<void(int)> visit = [&](int operationId) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    if (operation.name == "func.call") {
      const std::string callee = CalleeName(operation);
      if (!callee.empty())
        result.insert(callee);
    }
    for (int regionId : operation.regions)
      for (int blockId :
           module.regions.at(static_cast<size_t>(regionId)).blocks)
        for (int child :
             module.blocks.at(static_cast<size_t>(blockId)).operations)
          visit(child);
  };
  visit(root.id);
  return result;
}

inline bool CallsReach(const GenericModule &module, const std::string &current,
                       const std::string &target,
                       std::set<std::string> &visited) {
  if (!visited.insert(current).second)
    return false;
  const GenericOperation *function = FindFunction(module, current);
  if (function == nullptr || function->regions.empty())
    return false;
  for (const std::string &callee : DirectCallees(module, *function)) {
    if (callee == target)
      return true;
    if (CallsReach(module, callee, target, visited))
      return true;
  }
  return false;
}

inline std::string UnsupportedCallReason(const GenericModule &module,
                                         const GenericOperation &call) {
  if (IsNoInline(call))
    return "";
  const std::string symbol = CalleeName(call);
  if (symbol.empty())
    return "call does not have a direct callee symbol";
  const GenericOperation *callee = FindFunction(module, symbol);
  if (callee == nullptr)
    return "call target is unavailable to the exact inliner";
  if (IsNoInline(*callee))
    return "";
  if (!IsProvablyLocalFunction(*callee))
    return "call target is not provably private or internal";
  if (callee->regions.size() != 1)
    return "private or internal call target has no available body";
  const GenericRegion &region =
      module.regions.at(static_cast<size_t>(callee->regions.front()));
  if (region.blocks.size() != 1)
    return "private or internal call target is not single-block";
  const GenericBlock &block =
      module.blocks.at(static_cast<size_t>(region.blocks.front()));
  if (block.arguments.size() != call.operands.size() ||
      block.argumentTypes != call.operandTypes)
    return "call operands do not match private or internal target arguments";
  if (block.operations.empty())
    return "private or internal call target has no return";
  const GenericOperation &terminator = module.operations.at(
      static_cast<size_t>(block.operations.back()));
  if (terminator.name != "func.return" || !terminator.regions.empty() ||
      terminator.operands.size() != call.results.size() ||
      terminator.operandTypes != call.resultTypes)
    return "call results do not match private or internal target return";
  std::set<std::string> visited;
  if (CallsReach(module, symbol, symbol, visited))
    return "recursive private or internal call cannot be modeled exactly";
  return "";
}

inline void InlineOneCall(GenericModule &module, int callId) {
  const GenericOperation call =
      module.operations.at(static_cast<size_t>(callId));
  const GenericOperation callee = *FindFunction(module, CalleeName(call));
  const GenericRegion calleeRegion =
      module.regions.at(static_cast<size_t>(callee.regions.front()));
  const GenericBlock calleeBlock =
      module.blocks.at(static_cast<size_t>(calleeRegion.blocks.front()));
  const GenericOperation terminator = module.operations.at(
      static_cast<size_t>(calleeBlock.operations.back()));
  std::map<int, int> values;
  for (size_t index = 0; index < calleeBlock.arguments.size(); ++index)
    values[calleeBlock.arguments[index]] = call.operands[index];

  GenericRewriter rewriter(module);
  const GenericBlock &callerBlock =
      module.blocks.at(static_cast<size_t>(call.blockId));
  auto callPosition = std::find(callerBlock.operations.begin(),
                                callerBlock.operations.end(), call.id);
  size_t position = static_cast<size_t>(
      std::distance(callerBlock.operations.begin(), callPosition));
  for (int child : calleeBlock.operations) {
    if (child == terminator.id)
      continue;
    const int clone = rewriter.cloneOperationTree(
        child, call.parentId, call.regionId, call.blockId, values);
    rewriter.insertToBlock(call.blockId, position++, clone);
  }
  for (size_t index = 0; index < call.results.size(); ++index) {
    const auto replacement = values.find(terminator.operands[index]);
    if (replacement == values.end())
      throw std::runtime_error("inline scope: unmapped callee return value");
    ReplaceAllUses(module, call.results[index], replacement->second);
  }
  rewriter.removeFromBlock(call.blockId, call.id);
  module = CompactGenericModule(std::move(module));
}

inline bool FunctionIsReferenced(const GenericModule &module,
                                 const std::string &symbol, int functionId) {
  for (const GenericOperation &operation : module.operations) {
    if (operation.id == functionId || operation.blockId < 0)
      continue;
    if (ReferencedSymbols(module, operation.id).count(symbol) != 0)
      return true;
  }
  return false;
}

inline void EraseDeadLocalFunctions(GenericModule &module,
                                    const std::set<std::string> &candidates) {
  bool changed = true;
  while (changed) {
    changed = false;
    for (const GenericOperation &operation : module.operations) {
      if (operation.name != "func.func" || !IsProvablyLocalFunction(operation))
        continue;
      const std::string symbol = FunctionSymbolName(operation);
      if (candidates.count(symbol) == 0)
        continue;
      if (FunctionIsReferenced(module, symbol, operation.id))
        continue;
      GenericRewriter(module).removeFromBlock(operation.blockId, operation.id);
      module = CompactGenericModule(std::move(module));
      changed = true;
      break;
    }
  }
}

// InlineScopePass runs createInlinerPass after extracting scope bodies.  The
// inliner's default callable optimization pipeline is createCanonicalizerPass.
inline void RunInlinerCanonicalizer(GenericModule &module) {
  PipelineAnalysisContext useLists(module, kGenericAnalysisUsers);
  while (FoldCanonicalizationBooleanOps(module, useLists)) {
  }
  while (EliminateCanonicalizationDeadCode(module, useLists)) {
  }
  module = CompactGenericModule(std::move(module));
}

} // namespace inline_scope_detail

inline StageResult RunStrictInlineScope(GenericModule module) {
  StageResult result;
  result.module = std::move(module);
  ValidateGenericModule(result.module);
  std::set<std::string> inlineCandidates;
  for (const GenericOperation &operation : result.module.operations)
    if (operation.name == "func.call" &&
        !inline_scope_detail::IsNoInline(operation)) {
      const std::string callee = inline_scope_detail::CalleeName(operation);
      if (!callee.empty())
        inlineCandidates.insert(callee);
    }

  // The compiler asserts on these shapes.  The lightweight model must retain
  // the input and report incomplete instead of partially inlining a module.
  for (const GenericOperation &operation : result.module.operations) {
    if (operation.name != "scope.scope" ||
        inline_scope_detail::IsNoInline(operation))
      continue;
    const std::string reason =
        inline_scope_detail::UnsupportedScopeReason(result.module, operation);
    if (reason.empty())
      continue;
    result.precision = Precision::Incomplete;
    result.diagnostics.push_back(
        {"InlineScope", "", operation.id, operation.name, reason});
  }
  if (result.precision == Precision::Incomplete)
    return result;

  for (const GenericOperation &operation : result.module.operations) {
    if (operation.name != "func.call")
      continue;
    const std::string reason =
        inline_scope_detail::UnsupportedCallReason(result.module, operation);
    if (reason.empty())
      continue;
    result.precision = Precision::Incomplete;
    result.diagnostics.push_back(
        {"InlineScope", "", operation.id, operation.name, reason});
  }
  if (result.precision == Precision::Incomplete)
    return result;

  while (true) {
    const auto scope = std::find_if(
        result.module.operations.begin(), result.module.operations.end(),
        [](const GenericOperation &operation) {
          return operation.name == "scope.scope" && operation.blockId >= 0 &&
                 !inline_scope_detail::IsNoInline(operation);
        });
    if (scope == result.module.operations.end())
      break;
    inline_scope_detail::InlineOneScope(result.module, scope->id);
  }
  while (true) {
    const auto call = std::find_if(
        result.module.operations.begin(), result.module.operations.end(),
        [&](const GenericOperation &operation) {
          if (operation.name != "func.call" || operation.blockId < 0 ||
              inline_scope_detail::IsNoInline(operation))
            return false;
          const GenericOperation *callee = inline_scope_detail::FindFunction(
              result.module, inline_scope_detail::CalleeName(operation));
          return callee != nullptr && !inline_scope_detail::IsNoInline(*callee);
        });
    if (call == result.module.operations.end())
      break;
    inline_scope_detail::InlineOneCall(result.module, call->id);
    // Inlined bodies can contain scopes.  Mirror the nested greedy pass
    // manager by extracting those before considering the next call.
    while (true) {
      const auto scope = std::find_if(
          result.module.operations.begin(), result.module.operations.end(),
          [](const GenericOperation &operation) {
            return operation.name == "scope.scope" && operation.blockId >= 0 &&
                   !inline_scope_detail::IsNoInline(operation);
          });
      if (scope == result.module.operations.end())
        break;
      inline_scope_detail::InlineOneScope(result.module, scope->id);
    }
  }
  inline_scope_detail::EraseDeadLocalFunctions(result.module,
                                                inlineCandidates);
  inline_scope_detail::RunInlinerCanonicalizer(result.module);
  ValidateGenericModule(result.module);
  return result;
}

} // namespace cvub

#endif
