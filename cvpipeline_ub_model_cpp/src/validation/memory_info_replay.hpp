#ifndef CVPIPELINE_UB_MODEL_CPP_MEMORY_INFO_REPLAY_HPP
#define CVPIPELINE_UB_MODEL_CPP_MEMORY_INFO_REPLAY_HPP

#include "../passes/plan_memory/plan_memory_model.hpp"

namespace cvub {

struct MemoryRecord {
  std::string name;
  std::string buffer;
  uint64_t extentBits = 0;
  std::vector<uint64_t> offsetsBytes;
  bool isTmpBuf = false;
  int allocTimeInIR = 0;
  std::pair<int, int> lifeTimeInIR = {0, 0};
};

struct ReplayResult {
  uint64_t peakBytes = 0;
  uint64_t peakBits = 0;
  uint64_t capacityBits = kUBCapacityBits;
  bool overflow = false;
  size_t allocCount = 0;
};

inline std::string jsonStringAt(const std::string &text, size_t keyPos,
                                const std::string &key) {
  size_t colon = text.find(':', keyPos + key.size());
  if (colon == std::string::npos)
    return "";
  size_t quote = text.find('"', colon + 1);
  if (quote == std::string::npos)
    return "";
  std::string out;
  bool escape = false;
  for (size_t i = quote + 1; i < text.size(); ++i) {
    char c = text[i];
    if (escape) {
      out.push_back(c);
      escape = false;
      continue;
    }
    if (c == '\\') {
      escape = true;
      continue;
    }
    if (c == '"')
      return out;
    out.push_back(c);
  }
  return "";
}

inline size_t findJsonObjectEnd(const std::string &text, size_t objectBegin) {
  bool inString = false;
  bool escape = false;
  int depth = 0;
  for (size_t i = objectBegin; i < text.size(); ++i) {
    char c = text[i];
    if (escape) {
      escape = false;
      continue;
    }
    if (inString && c == '\\') {
      escape = true;
      continue;
    }
    if (c == '"') {
      inString = !inString;
      continue;
    }
    if (inString)
      continue;
    if (c == '{') {
      ++depth;
    } else if (c == '}') {
      --depth;
      if (depth == 0)
        return i;
    }
  }
  return std::string::npos;
}

inline uint64_t jsonUIntAt(const std::string &text, size_t keyPos,
                           const std::string &key) {
  size_t colon = text.find(':', keyPos + key.size());
  if (colon == std::string::npos)
    return 0;
  size_t begin = text.find_first_of("0123456789", colon + 1);
  if (begin == std::string::npos)
    return 0;
  size_t end = begin;
  while (end < text.size() &&
         std::isdigit(static_cast<unsigned char>(text[end])))
    ++end;
  return std::stoull(text.substr(begin, end - begin));
}

inline bool jsonBoolAt(const std::string &text, size_t keyPos,
                       const std::string &key) {
  size_t colon = text.find(':', keyPos + key.size());
  if (colon == std::string::npos)
    return false;
  size_t value = text.find_first_not_of(" \t\r\n", colon + 1);
  return value != std::string::npos && text.compare(value, 4, "true") == 0;
}

inline std::vector<uint64_t> jsonUIntArrayAt(const std::string &text,
                                             size_t keyPos,
                                             const std::string &key) {
  std::vector<uint64_t> values;
  size_t colon = text.find(':', keyPos + key.size());
  if (colon == std::string::npos)
    return values;
  size_t begin = text.find('[', colon + 1);
  size_t end = text.find(']', begin + 1);
  if (begin == std::string::npos || end == std::string::npos)
    return values;
  size_t pos = begin + 1;
  while (pos < end) {
    pos = text.find_first_of("0123456789", pos);
    if (pos == std::string::npos || pos >= end)
      break;
    size_t numberEnd = pos;
    while (numberEnd < end &&
           std::isdigit(static_cast<unsigned char>(text[numberEnd])))
      ++numberEnd;
    values.push_back(std::stoull(text.substr(pos, numberEnd - pos)));
    pos = numberEnd;
  }
  return values;
}

inline std::string bufferName(const std::string &buffer) {
  size_t equal = buffer.find('=');
  return trim(equal == std::string::npos ? buffer : buffer.substr(0, equal));
}

inline std::vector<MemoryRecord> loadMemoryInfo(const fs::path &path) {
  std::string text = readFile(path);
  std::vector<MemoryRecord> records;
  const std::string bufferKey = "\"buffer\"";
  size_t pos = 0;
  while ((pos = text.find(bufferKey, pos)) != std::string::npos) {
    std::string buffer = jsonStringAt(text, pos, bufferKey);
    if (buffer.find("#hivm.address_space<ub>") == std::string::npos) {
      pos += bufferKey.size();
      continue;
    }
    size_t objectBegin = text.rfind('{', pos);
    size_t objectEnd = findJsonObjectEnd(text, objectBegin);
    if (objectBegin == std::string::npos || objectEnd == std::string::npos) {
      pos += bufferKey.size();
      continue;
    }
    std::string object = text.substr(objectBegin, objectEnd - objectBegin + 1);
    MemoryRecord record;
    record.buffer = buffer;
    record.name = bufferName(buffer);
    if (size_t extent = object.find("\"extent\"");
        extent != std::string::npos)
      record.extentBits = jsonUIntAt(object, extent, "\"extent\"");
    if (size_t tmp = object.find("\"is_tmpbuf\"");
        tmp != std::string::npos)
      record.isTmpBuf = jsonBoolAt(object, tmp, "\"is_tmpbuf\"");
    if (size_t alloc = object.find("\"alloc_time_in_ir\"");
        alloc != std::string::npos)
      record.allocTimeInIR = static_cast<int>(
          jsonUIntAt(object, alloc, "\"alloc_time_in_ir\""));
    if (size_t life = object.find("\"life_time_in_ir\"");
        life != std::string::npos) {
      std::vector<uint64_t> values =
          jsonUIntArrayAt(object, life, "\"life_time_in_ir\"");
      if (values.size() >= 2)
        record.lifeTimeInIR = {static_cast<int>(values[0]),
                               static_cast<int>(values[1])};
    }
    if (size_t offset = object.find("\"offset\"");
        offset != std::string::npos)
      record.offsetsBytes = jsonUIntArrayAt(object, offset, "\"offset\"");
    if (record.offsetsBytes.empty())
      record.offsetsBytes.push_back(0);
    records.push_back(std::move(record));
    pos = objectEnd + 1;
  }
  return records;
}

inline ReplayResult replayMemoryInfoPeak(
    const std::vector<MemoryRecord> &records) {
  ReplayResult result;
  result.allocCount = records.size();
  for (const MemoryRecord &record : records) {
    const uint64_t extentBytes = BitsToBytes(record.extentBits);
    for (uint64_t offset : record.offsetsBytes)
      result.peakBytes =
          std::max(result.peakBytes,
                   CheckedAdd(offset, extentBytes, "memory-info peak"));
  }
  result.peakBits =
      CheckedMul(result.peakBytes, kBitsPerByte, "memory-info peak bits");
  result.overflow = result.peakBits > result.capacityBits;
  return result;
}

inline std::set<std::string>
memoryNameSet(const std::vector<MemoryRecord> &records) {
  std::set<std::string> names;
  for (const MemoryRecord &record : records)
    names.insert(record.name);
  return names;
}

inline std::set<std::string>
memoryTmpbufSet(const std::vector<MemoryRecord> &records) {
  std::set<std::string> names;
  for (const MemoryRecord &record : records)
    if (record.isTmpBuf)
      names.insert(record.name);
  return names;
}

inline std::set<std::string>
irNameSet(const std::vector<IRAllocRecord> &records) {
  std::set<std::string> names;
  for (const IRAllocRecord &record : records)
    names.insert(record.name);
  return names;
}

inline std::set<std::string>
irTmpbufSet(const std::vector<IRAllocRecord> &records) {
  std::set<std::string> names;
  for (const IRAllocRecord &record : records)
    if (record.isTmpBufHint)
      names.insert(record.name);
  return names;
}

} // namespace cvub

#endif
