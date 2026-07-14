#ifndef CVPIPELINE_UB_MODEL_CPP_POST_CVPIPELINE_INLINE_SCOPE_HPP
#define CVPIPELINE_UB_MODEL_CPP_POST_CVPIPELINE_INLINE_SCOPE_HPP

#include "ir_utils.hpp"
#include "types.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace cvub {
namespace inline_scope_detail {

inline bool HasUnitAttribute(const std::string &dictionary,
                             const std::string &name) {
  if (dictionary.size() < 2 || dictionary.front() != '{' ||
      dictionary.back() != '}')
    return false;
  for (const std::string &entry :
       splitTopLevel(dictionary.substr(1, dictionary.size() - 2)))
    if (trim(entry) == name)
      return true;
  return false;
}

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

} // namespace inline_scope_detail

inline StageResult RunInlineScope(GenericModule module) {
  StageResult result;
  result.module = std::move(module);
  ValidateGenericModule(result.module);

  // The compiler asserts on these shapes.  The lightweight model must retain
  // the input and report incomplete instead of partially inlining a module.
  for (const GenericOperation &operation : result.module.operations) {
    if (operation.name != "scope.scope" ||
        inline_scope_detail::HasUnitAttribute(operation.attributes,
                                              "no_inline"))
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

  while (true) {
    const auto scope = std::find_if(
        result.module.operations.begin(), result.module.operations.end(),
        [](const GenericOperation &operation) {
          return operation.name == "scope.scope" && operation.blockId >= 0 &&
                 !inline_scope_detail::HasUnitAttribute(operation.attributes,
                                                        "no_inline");
        });
    if (scope == result.module.operations.end())
      break;
    inline_scope_detail::InlineOneScope(result.module, scope->id);
  }
  ValidateGenericModule(result.module);
  return result;
}

} // namespace cvub

#endif
