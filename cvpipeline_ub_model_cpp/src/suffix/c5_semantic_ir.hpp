#ifndef CVPIPELINE_UB_MODEL_CPP_C5_SEMANTIC_IR_HPP
#define CVPIPELINE_UB_MODEL_CPP_C5_SEMANTIC_IR_HPP

#include "inline_load_copy.hpp"

namespace cvub {

struct C5SemanticIR {
  C4SemanticIR c4;
  InlineLoadCopyResult inlineLoadCopy;
  std::vector<C4BufferRecord> buffers;
};

inline C5SemanticIR BuildC5SemanticIR(C4SemanticIR c4) {
  C5SemanticIR result;
  result.inlineLoadCopy = ModelInlineLoadCopy(c4);
  for (const C4BufferRecord &buffer : c4.buffers)
    if (result.inlineLoadCopy.erasedBuffers.count(buffer.sourceIdentity) == 0)
      result.buffers.push_back(buffer);
  result.c4 = std::move(c4);
  return result;
}

inline std::string
SerializeC5BufferProjection(const std::vector<C4BufferRecord> &buffers) {
  std::ostringstream output;
  output << "C5_BUFFER_PROJECTION\t1\n";
  for (const C4BufferRecord &buffer : buffers)
    output << "BUFFER\t" << HexEncode(buffer.identity) << '\t'
           << HexEncode(buffer.extraBuffer ? buffer.ownerName : "") << '\t'
           << C4AddressSpaceName(buffer.addressSpace) << '\t'
           << buffer.constBits << '\t' << (buffer.extraBuffer ? 1 : 0) << '\n';
  return output.str();
}

inline std::string SerializeC5SemanticIR(const C5SemanticIR &module) {
  return "C5_SEMANTIC_IR\t1\n" + SerializeC4SemanticIR(module.c4) +
         SerializeInlineLoadCopyResult(module.inlineLoadCopy) +
         SerializeC5BufferProjection(module.buffers);
}

} // namespace cvub

#endif
