#ifndef CVPIPELINE_UB_MODEL_CPP_C6_SEMANTIC_IR_HPP
#define CVPIPELINE_UB_MODEL_CPP_C6_SEMANTIC_IR_HPP

#include "mark_multi_buffer.hpp"

namespace cvub {

struct C6SemanticIR {
  C5SemanticIR c5;
  MarkMultiBufferOptions options;
  MarkMultiBufferResult markMultiBuffer;
};

inline C6SemanticIR BuildC6SemanticIR(C5SemanticIR c5,
                                      MarkMultiBufferOptions options) {
  C6SemanticIR result;
  result.markMultiBuffer = ModelMarkMultiBuffer(c5, options);
  result.options = options;
  result.c5 = std::move(c5);
  return result;
}

inline std::string SerializeC6MultiBufferProjection(
    const C6SemanticIR &module) {
  std::ostringstream output;
  output << "C6_MULTI_BUFFER_PROJECTION\t1\n";
  for (const C4BufferRecord &buffer : module.c5.buffers) {
    auto count =
        module.markMultiBuffer.buffer2MultiNum.find(buffer.sourceIdentity);
    output << "BUFFER\t" << HexEncode(buffer.identity) << '\t'
           << (count == module.markMultiBuffer.buffer2MultiNum.end()
                   ? 1
                   : count->second)
           << '\t'
           << (module.markMultiBuffer.preloadLocalBuffers.count(
                       buffer.sourceIdentity)
                   ? 1
                   : 0)
           << '\n';
  }
  return output.str();
}

inline std::string SerializeC6SemanticIR(const C6SemanticIR &module) {
  return "C6_SEMANTIC_IR\t1\n" + SerializeC5SemanticIR(module.c5) +
         SerializeMarkMultiBufferResult(module.markMultiBuffer) +
         SerializeC6MultiBufferProjection(module);
}

} // namespace cvub

#endif
