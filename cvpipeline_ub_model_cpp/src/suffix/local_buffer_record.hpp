#ifndef CVPIPELINE_UB_MODEL_CPP_LOCAL_BUFFER_RECORD_HPP
#define CVPIPELINE_UB_MODEL_CPP_LOCAL_BUFFER_RECORD_HPP

#include "infer_hivm_mem_scope.hpp"

namespace cvub {

struct LocalBufferRecord {
  std::string identity;
  std::string sourceIdentity;
  std::string ownerName;
  std::string type;
  AddressSpace addressSpace = AddressSpace::Unknown;
  uint64_t constBits = 0;
  bool extraBuffer = false;
  std::vector<std::optional<int64_t>> upperBounds;
};

inline const LocalBufferRecord *FindSourceBuffer(
    const std::vector<LocalBufferRecord> &buffers, const std::string &identity) {
  for (const LocalBufferRecord &buffer : buffers)
    if (buffer.sourceIdentity == identity)
      return &buffer;
  return nullptr;
}

inline std::string MappedBufferIdentity(
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

inline std::string BufferTypeAfterMapping(
    const std::string &buffer, const std::vector<LocalBufferRecord> &buffers,
    const std::string &fallback) {
  const std::string identity = startsWith(buffer, "local:")
                                   ? "base:" + buffer.substr(6)
                                   : buffer;
  if (const LocalBufferRecord *record = FindSourceBuffer(buffers, identity))
    return record->type;
  return IsTensorType(fallback) ? ConvertTensorToMemRefType(fallback) : fallback;
}

} // namespace cvub

#endif
