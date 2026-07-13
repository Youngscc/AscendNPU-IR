#ifndef CVPIPELINE_UB_MODEL_CPP_INLINE_LOAD_COPY_ORACLE_HPP
#define CVPIPELINE_UB_MODEL_CPP_INLINE_LOAD_COPY_ORACLE_HPP

#include "bufferized_semantic_ir_oracle.hpp"

namespace cvub {

inline std::string CollectInlineLoadCopyCandidates(
    const GenericModule &module) {
  AfterBufferRootResolver roots(module);
  std::ostringstream output;
  output << "INLINE_LOAD_COPY_CANDIDATES\t1\n";
  for (const GenericOperation &copy : module.operations) {
    if (copy.name != "hivm.hir.copy" || copy.operands.size() < 2)
      continue;
    const GenericOperation *load = nullptr;
    for (const GenericOperation &candidate : module.operations)
      if (candidate.name == "hivm.hir.load" &&
          candidate.operands.size() > 1 &&
          candidate.operands[1] == copy.operands[0]) {
        load = &candidate;
        break;
      }
    if (!load)
      continue;
    const GenericOperation *function = EnclosingFunction(module, copy);
    if (!function)
      throw std::runtime_error("InlineLoadCopy oracle: copy outside function");
    output << "CANDIDATE\t" << HexEncode(FunctionSymName(*function)) << '\t'
           << HexEncode(roots.root(load->operands[0])) << '\t'
           << HexEncode(roots.root(copy.operands[0])) << '\t'
           << HexEncode(roots.root(copy.operands[1])) << '\n';
  }
  return output.str();
}

} // namespace cvub

#endif
