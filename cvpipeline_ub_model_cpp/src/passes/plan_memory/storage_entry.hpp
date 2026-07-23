// BufferLife and StorageEntry construction/merge semantics.
#ifndef CVPIPELINE_UB_MODEL_CPP_STORAGE_ENTRY_HPP
#define CVPIPELINE_UB_MODEL_CPP_STORAGE_ENTRY_HPP

#include "mem_liveness_analysis.hpp"

namespace cvub {

struct BufferLifeModel {
  std::string buffer;
  int allocTime = -1;
  int freeTime = -1;
};

struct StorageEntryModel {
  int id = -1;
  int conflictProfileId = -1;
  std::vector<std::string> inplaceBuffers;
  std::vector<BufferLifeModel> bufferLifeVec;
  uint64_t constBits = 0;
  uint64_t alignedConstBits = 0;
  uint64_t bitsOffset = 0;
  int childIdx = -1;
  uint32_t multiBufferNum = 1;
  std::vector<int> otherBufferRelationEntries;
  int parentLoop = -1;
  std::set<std::string> dmaBuffers;
  std::set<std::string> scalarBuffers;
};

inline bool CompareBufferLifeModel(const BufferLifeModel &lhs,
                                   const BufferLifeModel &rhs) {
  if (lhs.allocTime == rhs.allocTime)
    return lhs.freeTime < rhs.freeTime;
  return lhs.allocTime < rhs.allocTime;
}

inline void MergeBufferVec(std::vector<BufferLifeModel> &bufferLife) {
  if (bufferLife.empty())
    return;
  std::sort(bufferLife.begin(), bufferLife.end(), CompareBufferLifeModel);
  std::vector<BufferLifeModel> mergedLife;
  BufferLifeModel current = bufferLife.front();
  for (size_t i = 1; i < bufferLife.size(); ++i) {
    const BufferLifeModel &life = bufferLife[i];
    if (life.allocTime <= current.freeTime + 1) {
      current.freeTime = std::max(current.freeTime, life.freeTime);
    } else {
      mergedLife.push_back(current);
      current = life;
    }
  }
  mergedLife.push_back(current);
  bufferLife.swap(mergedLife);
}

inline bool IsBufferLifeVecConflict(
    const std::vector<BufferLifeModel> &lhs,
    const std::vector<BufferLifeModel> &rhs) {
  size_t i = 0;
  size_t j = 0;
  while (i < lhs.size() && j < rhs.size()) {
    int lo = std::max(lhs[i].allocTime, rhs[j].allocTime);
    int hi = std::min(lhs[i].freeTime, rhs[j].freeTime);
    if (lo <= hi)
      return true;
    if (lhs[i].freeTime < rhs[j].freeTime)
      ++i;
    else
      ++j;
  }
  return false;
}

inline bool IsInplacableBufferPairMatched(
    const StorageEntryModel &buffer, const StorageEntryModel &conflictBuffer,
    const std::vector<std::pair<std::string, std::string>>
        &inplacableBufferPairs) {
  for (const std::string &inplaceBuffer : buffer.inplaceBuffers) {
    for (const std::string &conflictInplaceBuffer :
         conflictBuffer.inplaceBuffers) {
      for (const auto &pair : inplacableBufferPairs) {
        if ((inplaceBuffer == pair.first &&
             conflictInplaceBuffer == pair.second) ||
            (inplaceBuffer == pair.second &&
             conflictInplaceBuffer == pair.first))
          return true;
      }
    }
  }
  return false;
}

struct PreparedStorageEntryAnalysis {
  std::unordered_map<std::string, BufferInfoRecord> bufferInfoByName;
  std::unordered_set<std::string> dmaBuffers;
  std::unordered_set<std::string> scalarBuffers;
  std::unordered_map<std::string, int> parentLoopByBuffer;

  PreparedStorageEntryAnalysis(
      const std::vector<BufferInfoRecord> &bufferInfos,
      const LifetimeAnalysis &liveness) {
    bufferInfoByName.reserve(bufferInfos.size());
    dmaBuffers.reserve(bufferInfos.size());
    scalarBuffers.reserve(bufferInfos.size());
    parentLoopByBuffer.reserve(bufferInfos.size());
    for (const BufferInfoRecord &info : bufferInfos)
      bufferInfoByName[info.name] = info;
    const StrideAlignmentMap strideAlignments =
        CollectStrideAlignmentMarks(liveness.operations);
    std::set<int> loopRegions;
    for (size_t i = 0; i < liveness.operations.size(); ++i) {
      const OperationRecord &operation = liveness.operations[i];
      if (operation.opName != "scf.for")
        continue;
      for (size_t j = i + 1; j < liveness.operations.size(); ++j) {
        const OperationRecord &candidate = liveness.operations[j];
        if (candidate.regionPath.size() == operation.regionPath.size() + 1 &&
            pathPrefix(operation.regionPath, candidate.regionPath)) {
          loopRegions.insert(candidate.regionPath.back());
          break;
        }
        if (candidate.regionPath.size() <= operation.regionPath.size())
          break;
      }
    }
    auto recordCanonical = [&](const std::vector<std::string> &values,
                               std::unordered_set<std::string> &destination) {
      for (const std::string &value : values) {
        auto canonical = liveness.canonicalAllocByValue.find(value);
        if (canonical != liveness.canonicalAllocByValue.end())
          destination.insert(canonical->second);
      }
    };
    for (const OperationRecord &operation : liveness.operations) {
      if (operation.opName == "memref.alloc") {
        std::vector<std::string> results = operationResultNames(operation);
        if (!results.empty()) {
          int parentLoop = -1;
          for (int region : operation.regionPath)
            if (loopRegions.count(region))
              parentLoop = region;
          parentLoopByBuffer[results.front()] = parentLoop;
        }
      }
      if (IsHIVMStructuredOp(operation.opName) &&
          IsSinglePipeOp(operation.opName)) {
        const HIVMPipe pipe = GetPipe(operation);
        if (pipe == HIVMPipe::Unassigned)
          throw std::runtime_error(
              "PlanMemory exact blocker: unrecognized HIVM pipe: " +
              operation.opName);
        if (pipe == HIVMPipe::MTE2)
          recordCanonical(extractGroupSSAs(operation.text, "outs"),
                          dmaBuffers);
        else if (pipe == HIVMPipe::MTE3)
          recordCanonical(extractGroupSSAs(operation.text, "ins"),
                          dmaBuffers);
      }
      if (shouldLowerToScalarLoops(operation, strideAlignments))
        recordCanonical(operationOperandNames(operation), scalarBuffers);
      else if (operation.opName == "memref.load" ||
               operation.opName == "memref.store")
        recordCanonical(operationOperandNames(operation), scalarBuffers);
    }
  }
};

inline std::vector<StorageEntryModel>
GenerateStorageEntry(const PreparedStorageEntryAnalysis &prepared,
                     const LifetimeAnalysis &liveness) {
  std::unordered_map<std::string, LifetimeRecord> lifeByName;
  lifeByName.reserve(liveness.records.size());
  for (const LifetimeRecord &life : liveness.records)
    lifeByName[life.name] = life;

  std::vector<StorageEntryModel> entries;
  for (const std::string &name : liveness.bufferGenOrder) {
    auto lifeIt = lifeByName.find(name);
    if (lifeIt == lifeByName.end() || lifeIt->second.directAllocTime < 0)
      continue;
    auto info = prepared.bufferInfoByName.find(name);
    if (info == prepared.bufferInfoByName.end())
      continue;
    StorageEntryModel entry;
    entry.id = static_cast<int>(entries.size());
    entry.inplaceBuffers.push_back(name);
    entry.constBits = info->second.constBits;
    if (prepared.dmaBuffers.count(name) != 0)
      entry.dmaBuffers.insert(name);
    if (prepared.scalarBuffers.count(name) != 0)
      entry.scalarBuffers.insert(name);
    auto parentLoop = prepared.parentLoopByBuffer.find(name);
    entry.parentLoop = parentLoop == prepared.parentLoopByBuffer.end()
                           ? -1
                           : parentLoop->second;
    auto multiBuffer = liveness.buffer2MultiNum.find(name);
    if (multiBuffer != liveness.buffer2MultiNum.end())
      entry.multiBufferNum = multiBuffer->second;
    entry.bufferLifeVec.push_back(
        {name, lifeIt->second.directAllocTime, lifeIt->second.directFreeTime});
    entries.push_back(std::move(entry));
  }
  return entries;
}

inline std::vector<StorageEntryModel>
GenerateStorageEntry(const std::vector<BufferInfoRecord> &bufferInfos,
                     const LifetimeAnalysis &liveness) {
  return GenerateStorageEntry(
      PreparedStorageEntryAnalysis(bufferInfos, liveness), liveness);
}

inline void MergeInplaceSE(
    std::vector<StorageEntryModel> &entries,
    const std::vector<std::pair<std::string, std::string>> &inplacePairList) {
  std::vector<std::optional<StorageEntryModel>> storageEntries;
  storageEntries.reserve(entries.size());
  std::unordered_map<std::string, size_t> buffer2storageEntry;
  buffer2storageEntry.reserve(entries.size());
  for (StorageEntryModel &entry : entries) {
    size_t index = storageEntries.size();
    for (const std::string &buffer : entry.inplaceBuffers)
      buffer2storageEntry[buffer] = index;
    storageEntries.emplace_back(std::move(entry));
  }
  for (const auto &pair : inplacePairList) {
    auto genIt = buffer2storageEntry.find(pair.first);
    auto killIt = buffer2storageEntry.find(pair.second);
    if (genIt == buffer2storageEntry.end() ||
        killIt == buffer2storageEntry.end())
      throw std::runtime_error("MergeInplaceSE: missing storage entry");
    size_t genIndex = genIt->second;
    size_t killIndex = killIt->second;
    if (genIndex == killIndex)
      continue;
    StorageEntryModel &genSE = *storageEntries[genIndex];
    StorageEntryModel &killSE = *storageEntries[killIndex];
    std::vector<BufferLifeModel> mergedBufferLifeVec = genSE.bufferLifeVec;
    mergedBufferLifeVec.insert(mergedBufferLifeVec.end(),
                               killSE.bufferLifeVec.begin(),
                               killSE.bufferLifeVec.end());
    MergeBufferVec(mergedBufferLifeVec);
    killSE.bufferLifeVec.swap(mergedBufferLifeVec);
    killSE.inplaceBuffers.insert(killSE.inplaceBuffers.begin(),
                                 genSE.inplaceBuffers.begin(),
                                 genSE.inplaceBuffers.end());
    killSE.multiBufferNum =
        std::max(genSE.multiBufferNum, killSE.multiBufferNum);
    // GetBufferParentLoop() examines every buffer in the merged StorageEntry.
    // A merged entry has a parent loop only when all defining buffers have the
    // same non-null parent loop.
    if (genSE.parentLoop < 0 || killSE.parentLoop < 0 ||
        genSE.parentLoop != killSE.parentLoop)
      killSE.parentLoop = -1;
    killSE.dmaBuffers.insert(genSE.dmaBuffers.begin(), genSE.dmaBuffers.end());
    killSE.scalarBuffers.insert(genSE.scalarBuffers.begin(),
                                genSE.scalarBuffers.end());
    for (const std::string &buffer : genSE.inplaceBuffers)
      buffer2storageEntry[buffer] = killIndex;
    storageEntries[genIndex].reset();
  }
  entries.clear();
  for (std::optional<StorageEntryModel> &entry : storageEntries)
    if (entry)
      entries.push_back(std::move(*entry));
}

inline void
ExpandMultiBufferStorageEntry(std::vector<StorageEntryModel> &entries) {
  const size_t originalSize = entries.size();
  int nextId = 0;
  for (StorageEntryModel &entry : entries) {
    entry.id = nextId++;
    entry.otherBufferRelationEntries.clear();
  }
  for (size_t i = 0; i < originalSize; ++i) {
    if (entries[i].multiBufferNum <= 1)
      continue;
    for (uint32_t j = 1; j < entries[i].multiBufferNum; ++j) {
      StorageEntryModel relationEntry = entries[i];
      relationEntry.id = nextId++;
      relationEntry.otherBufferRelationEntries.clear();
      entries[i].otherBufferRelationEntries.push_back(relationEntry.id);
      entries.push_back(std::move(relationEntry));
    }
  }
}

inline void GetReorderRootStorageEntry(std::vector<StorageEntryModel> &entries) {
  enum class PipeClass { Dma, Other, Scalar };
  auto getPipeClass = [](const StorageEntryModel &entry) {
    for (const std::string &buffer : entry.inplaceBuffers) {
      if (entry.dmaBuffers.count(buffer) != 0)
        return PipeClass::Dma;
      if (entry.scalarBuffers.count(buffer) != 0)
        return PipeClass::Scalar;
    }
    return PipeClass::Other;
  };

  std::vector<int> classified;
  for (PipeClass pipeClass :
       {PipeClass::Dma, PipeClass::Other, PipeClass::Scalar})
    for (const StorageEntryModel &entry : entries)
      if (getPipeClass(entry) == pipeClass)
        classified.push_back(entry.id);

  std::map<int, StorageEntryModel> byId;
  for (StorageEntryModel &entry : entries)
    byId.emplace(entry.id, std::move(entry));
  std::vector<StorageEntryModel> reordered;
  std::set<int> added;
  for (int id : classified) {
    if (!added.insert(id).second)
      continue;
    reordered.push_back(std::move(byId.at(id)));
    for (int relationId : reordered.back().otherBufferRelationEntries) {
      if (added.insert(relationId).second)
        reordered.push_back(std::move(byId.at(relationId)));
    }
  }
  entries.swap(reordered);
}

inline uint64_t PlanBuffersWithoutReuse(std::vector<StorageEntryModel> &entries,
                                        uint64_t alignUnit) {
  uint64_t offset = 0;
  for (StorageEntryModel &entry : entries) {
    entry.alignedConstBits = AlignUp(entry.constBits, alignUnit);
    entry.bitsOffset = offset;
    offset = CheckedAdd(offset, entry.alignedConstBits,
                        "non-reuse storage placement");
  }
  return offset;
}

constexpr int SPEC_LEVEL_0 = 0;
constexpr int SPEC_LEVEL_1 = 1;
constexpr int SPEC_LEVEL_2 = 2;
constexpr int SPEC_LEVEL_3 = 3;


} // namespace cvub

#endif
