#ifndef CVPIPELINE_UB_MODEL_CPP_C4_BUFFER_RECORD_HPP
#define CVPIPELINE_UB_MODEL_CPP_C4_BUFFER_RECORD_HPP

#include "infer_hivm_mem_scope.hpp"

namespace cvub {

struct C4BufferRecord {
  std::string identity;
  std::string sourceIdentity;
  std::string ownerName;
  std::string type;
  AddressSpace addressSpace = AddressSpace::Unknown;
  uint64_t constBits = 0;
  bool extraBuffer = false;
  std::vector<std::optional<int64_t>> upperBounds;
};

inline const C4BufferRecord *C4FindSourceBuffer(
    const std::vector<C4BufferRecord> &buffers, const std::string &identity) {
  for (const C4BufferRecord &buffer : buffers)
    if (buffer.sourceIdentity == identity)
      return &buffer;
  return nullptr;
}

inline std::string C4MappedBufferIdentity(
    const std::string &buffer,
    const std::map<std::string, std::string> &mapping) {
  if (startsWith(buffer, "choice("))
    return buffer;
  auto found = mapping.find(buffer);
  if (found != mapping.end())
    return "base:" + found->second.substr(6);
  if (startsWith(buffer, "local:"))
    return "base:" + buffer.substr(6);
  return buffer;
}

inline std::string C4BufferType(
    const std::string &buffer, const std::vector<C4BufferRecord> &buffers,
    const std::string &fallback) {
  const std::string identity = startsWith(buffer, "local:")
                                   ? "base:" + buffer.substr(6)
                                   : buffer;
  if (const C4BufferRecord *record = C4FindSourceBuffer(buffers, identity))
    return record->type;
  return IsTensorType(fallback) ? ConvertTensorToMemRefType(fallback) : fallback;
}

} // namespace cvub

#endif
