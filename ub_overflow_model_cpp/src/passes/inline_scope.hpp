#ifndef CVPIPELINE_UB_MODEL_CPP_INLINE_SCOPE_HPP
#define CVPIPELINE_UB_MODEL_CPP_INLINE_SCOPE_HPP

#include "canonicalization_hivm_pipeline.hpp"

namespace cvub {

inline bool ScopeHasNoInline(const GenericOperation &operation) {
  const std::string value =
      FindDictionaryValue(operation.properties, "no_inline").empty()
          ? FindDictionaryValue(operation.attributes, "no_inline")
          : FindDictionaryValue(operation.properties, "no_inline");
  return !value.empty() && value != "false" && value != "0";
}

// Mirrors ExtractOpsFromBodyPattern<scope::ScopeOp>. createInlinerPass is a
// separate generic function-call transformation and is not conflated with the
// scope body extraction.
inline GenericModule RunInlineScope(GenericModule module) {
  bool changed = true;
  while (changed) {
    changed = false;
    for (auto iterator = module.operations.rbegin();
         iterator != module.operations.rend(); ++iterator) {
      const GenericOperation scopeSnapshot = *iterator;
      if (scopeSnapshot.name != "scope.scope" ||
          ScopeHasNoInline(scopeSnapshot) || scopeSnapshot.regions.size() != 1 ||
          scopeSnapshot.blockId < 0)
        continue;
      const GenericRegion &region = module.regions.at(
          static_cast<size_t>(scopeSnapshot.regions.front()));
      if (region.blocks.size() != 1)
        throw std::runtime_error(
            "InlineScope: scope.scope must contain one block");
      const GenericBlock body =
          module.blocks.at(static_cast<size_t>(region.blocks.front()));
      if (body.operations.empty())
        throw std::runtime_error("InlineScope: scope.scope has no terminator");
      const GenericOperation &terminator = module.operations.at(
          static_cast<size_t>(body.operations.back()));
      if (terminator.name != "scope.return" ||
          terminator.operands.size() != scopeSnapshot.results.size())
        throw std::runtime_error(
            "InlineScope: malformed scope.return/result contract");

      for (size_t index = 0; index < scopeSnapshot.results.size(); ++index)
        ReplaceCanonicalizedValue(module, scopeSnapshot.results[index],
                                  terminator.operands[index]);

      GenericRewriter rewriter(module);
      const GenericBlock &parent = module.blocks.at(
          static_cast<size_t>(scopeSnapshot.blockId));
      const auto scopePosition = std::find(parent.operations.begin(),
                                           parent.operations.end(),
                                           scopeSnapshot.id);
      if (scopePosition == parent.operations.end())
        continue;
      size_t insertion =
          static_cast<size_t>(scopePosition - parent.operations.begin());
      for (size_t index = 0; index + 1 < body.operations.size(); ++index) {
        const int child = body.operations[index];
        rewriter.removeFromBlock(body.id, child);
        rewriter.insertToBlock(scopeSnapshot.blockId, insertion++, child);
      }
      rewriter.removeFromBlock(scopeSnapshot.blockId, scopeSnapshot.id);
      changed = true;
      break;
    }
  }
  return CompactGenericModule(std::move(module));
}

} // namespace cvub

#endif
