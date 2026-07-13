#ifndef CVPIPELINE_UB_MODEL_CPP_C5_SEMANTIC_IR_ORACLE_HPP
#define CVPIPELINE_UB_MODEL_CPP_C5_SEMANTIC_IR_ORACLE_HPP

#include "bufferized_semantic_ir_oracle.hpp"

namespace cvub {

inline std::string CollectInlineLoadCopyCandidates(
    const C1SemanticModule &module) {
  AfterBufferRootResolver roots(module);
  std::ostringstream output;
  output << "INLINE_LOAD_COPY_CANDIDATES\t1\n";
  for (const C1OperationRecord &copy : module.operations) {
    if (copy.name != "hivm.hir.copy" || copy.operands.size() < 2)
      continue;
    const C1OperationRecord *load = nullptr;
    for (const C1OperationRecord &candidate : module.operations)
      if (candidate.name == "hivm.hir.load" &&
          candidate.operands.size() > 1 &&
          candidate.operands[1] == copy.operands[0]) {
        load = &candidate;
        break;
      }
    if (!load)
      continue;
    const C1OperationRecord *function = C1EnclosingFunction(module, copy);
    if (!function)
      throw std::runtime_error("C5 oracle: copy outside function");
    output << "CANDIDATE\t" << HexEncode(C1FunctionName(*function)) << '\t'
           << HexEncode(roots.root(load->operands[0])) << '\t'
           << HexEncode(roots.root(copy.operands[0])) << '\t'
           << HexEncode(roots.root(copy.operands[1])) << '\n';
  }
  return output.str();
}

} // namespace cvub

#endif
