#ifndef CVPIPELINE_UB_MODEL_CPP_AFTER_MARK_MULTI_BUFFER_HPP
#define CVPIPELINE_UB_MODEL_CPP_AFTER_MARK_MULTI_BUFFER_HPP

#include "../passes/mark_multi_buffer.hpp"

namespace cvub {

struct AfterMarkMultiBufferState {
  AfterInlineLoadCopyState afterInlineLoadCopy;
  MarkMultiBufferOptions options;
  MarkMultiBufferResult markMultiBuffer;
};

inline AfterMarkMultiBufferState BuildAfterMarkMultiBufferState(AfterInlineLoadCopyState afterInlineLoadCopy,
                                      MarkMultiBufferOptions options) {
  AfterMarkMultiBufferState result;
  result.markMultiBuffer = ModelMarkMultiBuffer(afterInlineLoadCopy, options);
  result.options = options;
  result.afterInlineLoadCopy = std::move(afterInlineLoadCopy);
  return result;
}

inline std::string SerializeMarkMultiBufferProjection(
    const AfterMarkMultiBufferState &module) {
  std::ostringstream output;
  output << "MARK_MULTI_BUFFER_PROJECTION\t1\n";
  for (const LocalBufferRecord &buffer : module.afterInlineLoadCopy.buffers) {
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

inline std::string SerializeAfterMarkMultiBufferState(const AfterMarkMultiBufferState &module) {
  return "AFTER_MARK_MULTI_BUFFER_STATE\t1\n" + SerializeAfterInlineLoadCopyState(module.afterInlineLoadCopy) +
         SerializeMarkMultiBufferResult(module.markMultiBuffer) +
         SerializeMarkMultiBufferProjection(module);
}

} // namespace cvub

#endif
