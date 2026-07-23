#ifndef CVPIPELINE_UB_MODEL_CPP_AFTER_INLINE_LOAD_COPY_HPP
#define CVPIPELINE_UB_MODEL_CPP_AFTER_INLINE_LOAD_COPY_HPP

#include "../passes/inline_load_copy.hpp"

namespace cvub {

struct AfterInlineLoadCopyState {
  AfterAllocExtraBufferState afterAllocExtraBuffer;
  InlineLoadCopyResult inlineLoadCopy;
  std::vector<LocalBufferRecord> buffers;
};

inline AfterInlineLoadCopyState BuildAfterInlineLoadCopyState(
    AfterAllocExtraBufferState afterAllocExtraBuffer,
    bool requireExactDynamicMiddleValue = false) {
  AfterInlineLoadCopyState result;
  result.inlineLoadCopy = ModelInlineLoadCopy(
      afterAllocExtraBuffer, requireExactDynamicMiddleValue);
  for (const LocalBufferRecord &buffer : afterAllocExtraBuffer.buffers)
    if (result.inlineLoadCopy.erasedBuffers.count(buffer.sourceIdentity) == 0)
      result.buffers.push_back(buffer);
  result.afterAllocExtraBuffer = std::move(afterAllocExtraBuffer);
  return result;
}

inline std::string
SerializeInlineLoadCopyBufferProjection(const std::vector<LocalBufferRecord> &buffers) {
  std::ostringstream output;
  output << "INLINE_LOAD_COPY_BUFFER_PROJECTION\t1\n";
  for (const LocalBufferRecord &buffer : buffers)
    output << "BUFFER\t" << HexEncode(buffer.identity) << '\t'
           << HexEncode(buffer.extraBuffer ? buffer.ownerName : "") << '\t'
           << AddressSpaceName(buffer.addressSpace) << '\t'
           << buffer.constBits << '\t' << (buffer.extraBuffer ? 1 : 0) << '\n';
  return output.str();
}

inline std::string SerializeAfterInlineLoadCopyState(const AfterInlineLoadCopyState &module) {
  return "AFTER_INLINE_LOAD_COPY_STATE\t1\n" + SerializeAfterAllocExtraBufferState(module.afterAllocExtraBuffer) +
         SerializeInlineLoadCopyResult(module.inlineLoadCopy) +
         SerializeInlineLoadCopyBufferProjection(module.buffers);
}

} // namespace cvub

#endif
