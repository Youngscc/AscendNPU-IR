// MemoryBound speculative address planning and public PlanMemory entry.
#ifndef CVPIPELINE_UB_MODEL_CPP_MEM_PLAN_HPP
#define CVPIPELINE_UB_MODEL_CPP_MEM_PLAN_HPP

#include "storage_entry.hpp"
#include "../../support/debug_trace.hpp"

namespace cvub {

struct MemoryBoundModel {
  std::vector<BufferLifeModel> bufferLifeVec;
  uint64_t offset = 0;
  uint64_t extent = 0;
  int lastStorageEntry = -1;
};

using MemBoundListModel = std::list<std::shared_ptr<MemoryBoundModel>>;
using MemBoundListModelIter = MemBoundListModel::iterator;

struct PlanRecordModel {
  int specLevel = SPEC_LEVEL_0;
  int childIdx = -1;
  bool tailed = false;
  bool headed = false;
  size_t splitNums = 0;
  int entry = -1;
  uint64_t allExtent = 0;
  std::shared_ptr<MemoryBoundModel> firstMemBound;
  MemBoundListModel replaced;
  bool isDirectlyRollback = false;
};

using PlanRecHisModel = std::vector<PlanRecordModel>;

struct SpecInfoModel {
  int maxLevel = SPEC_LEVEL_3;
  int minLevel = SPEC_LEVEL_0;
  int specLevel = SPEC_LEVEL_3;
  int childIdx = -1;
  int specStartIdx = 0;
  int rollbackIdx = -1;
  uint64_t rollbackAddr = std::numeric_limits<uint64_t>::max();
};

struct MemPlanStateModel {
  bool splitOutline = false;
  int nextStorageEntryId = 0;
  std::list<StorageEntryModel> relationStorageEntries;
  std::map<int, std::vector<int>> firstBufferEntry2RelationOtherBufferEntry;
  // Storage-entry IDs are dense and stable during one planning attempt.
  // Speculation queried them from nested conflict loops through repeated
  // linear scans; retain direct pointers instead.  The root vector is not
  // resized after this table is built and std::list keeps synthetic-entry
  // addresses stable.
  std::vector<StorageEntryModel *> entryById;
  std::unordered_map<std::string, int> conflictProfileByBuffer;
  size_t conflictProfileCount = 0;
  std::vector<uint8_t> lifeConflict;
  std::vector<uint8_t> pipeConflict;
  std::vector<uint8_t> inplacableConflict;
};

inline void RegisterStorageEntry(MemPlanStateModel &state,
                                 StorageEntryModel &entry) {
  if (entry.id < 0)
    return;
  const size_t id = static_cast<size_t>(entry.id);
  if (state.entryById.size() <= id)
    state.entryById.resize(id + 1, nullptr);
  state.entryById[id] = &entry;
}

inline void RebuildStorageEntryLookup(
    std::vector<StorageEntryModel> &entries, MemPlanStateModel &state) {
  state.entryById.clear();
  for (StorageEntryModel &entry : entries)
    RegisterStorageEntry(state, entry);
  for (StorageEntryModel &entry : state.relationStorageEntries)
    RegisterStorageEntry(state, entry);
}

inline bool BufferPipeConflict(const StorageEntryModel &lhs,
                               const StorageEntryModel &rhs);

inline void BuildStorageEntryConflictTables(
    std::vector<StorageEntryModel> &entries, MemPlanStateModel &state,
    const std::vector<std::pair<std::string, std::string>>
        &inplacableBufferPairs) {
  state.conflictProfileByBuffer.reserve(entries.size() * 2);
  int maximumProfile = -1;
  for (StorageEntryModel &entry : entries) {
    if (entry.conflictProfileId < 0)
      entry.conflictProfileId = entry.id;
    maximumProfile = std::max(maximumProfile, entry.conflictProfileId);
    for (const std::string &buffer : entry.inplaceBuffers)
      state.conflictProfileByBuffer[buffer] = entry.conflictProfileId;
  }
  const size_t count = maximumProfile < 0
                           ? 0
                           : static_cast<size_t>(maximumProfile) + 1;
  state.conflictProfileCount = count;
  const size_t tableSize = count * count;
  state.lifeConflict.assign(tableSize, uint8_t{0});
  state.pipeConflict.assign(tableSize, uint8_t{0});
  state.inplacableConflict.assign(tableSize, uint8_t{0});
  std::unordered_map<std::string, std::unordered_set<std::string>>
      inplacableByBuffer;
  inplacableByBuffer.reserve(inplacableBufferPairs.size() * 2);
  for (const auto &[left, right] : inplacableBufferPairs) {
    inplacableByBuffer[left].insert(right);
    inplacableByBuffer[right].insert(left);
  }
  const auto isInplacable = [&](const StorageEntryModel &left,
                                const StorageEntryModel &right) {
    for (const std::string &leftBuffer : left.inplaceBuffers) {
      auto candidates = inplacableByBuffer.find(leftBuffer);
      if (candidates == inplacableByBuffer.end())
        continue;
      for (const std::string &rightBuffer : right.inplaceBuffers)
        if (candidates->second.count(rightBuffer) != 0)
          return true;
    }
    return false;
  };
  for (size_t leftIndex = 0; leftIndex < entries.size(); ++leftIndex) {
    const StorageEntryModel &left = entries[leftIndex];
    const size_t leftProfile =
        static_cast<size_t>(left.conflictProfileId);
    for (size_t rightIndex = leftIndex; rightIndex < entries.size();
         ++rightIndex) {
      const StorageEntryModel &right = entries[rightIndex];
      const size_t rightProfile =
          static_cast<size_t>(right.conflictProfileId);
      const size_t forward = leftProfile * count + rightProfile;
      const size_t reverse = rightProfile * count + leftProfile;
      const uint8_t life =
          IsBufferLifeVecConflict(left.bufferLifeVec, right.bufferLifeVec);
      const uint8_t pipe = BufferPipeConflict(left, right);
      const uint8_t inplacable = isInplacable(left, right);
      state.lifeConflict[forward] = state.lifeConflict[reverse] = life;
      state.pipeConflict[forward] = state.pipeConflict[reverse] = pipe;
      state.inplacableConflict[forward] =
          state.inplacableConflict[reverse] = inplacable;
    }
  }
}

inline bool CachedConflict(
    const std::vector<uint8_t> &table, size_t profileCount,
    const StorageEntryModel &left, const StorageEntryModel &right) {
  if (left.conflictProfileId < 0 || right.conflictProfileId < 0)
    return false;
  const size_t lhs = static_cast<size_t>(left.conflictProfileId);
  const size_t rhs = static_cast<size_t>(right.conflictProfileId);
  return lhs < profileCount && rhs < profileCount &&
         table[lhs * profileCount + rhs] != 0;
}

inline StorageEntryModel *FindStorageEntry(
    std::vector<StorageEntryModel> &entries, MemPlanStateModel &state, int id) {
  if (id >= 0 && static_cast<size_t>(id) < state.entryById.size()) {
    StorageEntryModel *entry = state.entryById[static_cast<size_t>(id)];
    if (entry)
      return entry;
  }
  for (StorageEntryModel &entry : entries)
    if (entry.id == id)
      return &entry;
  for (StorageEntryModel &entry : state.relationStorageEntries)
    if (entry.id == id)
      return &entry;
  return nullptr;
}

inline const StorageEntryModel *FindStorageEntry(
    const std::vector<StorageEntryModel> &entries,
    const MemPlanStateModel &state, int id) {
  if (id >= 0 && static_cast<size_t>(id) < state.entryById.size()) {
    const StorageEntryModel *entry = state.entryById[static_cast<size_t>(id)];
    if (entry)
      return entry;
  }
  for (const StorageEntryModel &entry : entries)
    if (entry.id == id)
      return &entry;
  for (const StorageEntryModel &entry : state.relationStorageEntries)
    if (entry.id == id)
      return &entry;
  return nullptr;
}

inline void ClearSyntheticRelationEntries(MemPlanStateModel &state,
                                          int firstBufferEntryId) {
  auto relation = state.firstBufferEntry2RelationOtherBufferEntry.find(
      firstBufferEntryId);
  if (relation == state.firstBufferEntry2RelationOtherBufferEntry.end())
    return;
  const std::set<int> ids(relation->second.begin(), relation->second.end());
  for (int id : relation->second)
    if (id >= 0 && static_cast<size_t>(id) < state.entryById.size())
      state.entryById[static_cast<size_t>(id)] = nullptr;
  state.relationStorageEntries.remove_if(
      [&](const StorageEntryModel &entry) { return ids.count(entry.id) != 0; });
  state.firstBufferEntry2RelationOtherBufferEntry.erase(relation);
}

inline void MergeBufferLife(MemBoundListModelIter start,
                            MemBoundListModelIter end,
                            std::vector<BufferLifeModel> &newLife) {
  for (auto it = start; it != end; ++it)
    newLife.insert(newLife.end(), (*it)->bufferLifeVec.begin(),
                   (*it)->bufferLifeVec.end());
  MergeBufferVec(newLife);
}

inline void AddMemBoundInSectionalWay(
    StorageEntryModel &entry, MemBoundListModelIter start,
    MemBoundListModelIter end,
    std::vector<std::shared_ptr<MemoryBoundModel>> &splitBound) {
  for (auto iter = start; iter != end; ++iter) {
    std::vector<BufferLifeModel> life = entry.bufferLifeVec;
    life.insert(life.end(), (*iter)->bufferLifeVec.begin(),
                (*iter)->bufferLifeVec.end());
    MergeBufferVec(life);
    auto next = iter;
    ++next;
    uint64_t size =
        next == end
            ? CheckedAdd(entry.bitsOffset, entry.alignedConstBits,
                         "sectional storage end") -
                  (*iter)->offset
            : (*iter)->extent;
    splitBound.push_back(std::make_shared<MemoryBoundModel>(
        MemoryBoundModel{std::move(life), (*iter)->offset, size, entry.id}));
  }
}

inline void UpdateOutline(MemBoundListModel &outline, PlanRecHisModel &history,
                          StorageEntryModel &entry,
                          MemBoundListModelIter start,
                          MemBoundListModelIter endInclusive, uint64_t size,
                          bool isDirectlyRollback, int localLevel,
                          bool splitOutline) {
  uint64_t res = size - entry.alignedConstBits;
  std::shared_ptr<MemoryBoundModel> last = *endInclusive;
  auto end = endInclusive;
  ++end;
  std::vector<std::shared_ptr<MemoryBoundModel>> splitBound;
  if (splitOutline) {
    AddMemBoundInSectionalWay(entry, start, end, splitBound);
  } else {
    std::vector<BufferLifeModel> life = entry.bufferLifeVec;
    MergeBufferLife(start, end, life);
    splitBound.push_back(std::make_shared<MemoryBoundModel>(MemoryBoundModel{
        std::move(life), entry.bitsOffset, entry.alignedConstBits, entry.id}));
  }

  if (res > 0) {
    auto tail = std::make_shared<MemoryBoundModel>(MemoryBoundModel{
        last->bufferLifeVec,
        CheckedAdd(last->offset, last->extent, "outline tail") - res, res,
        last->lastStorageEntry});
    end = outline.insert(end, std::move(tail));
  }
  for (int i = static_cast<int>(splitBound.size()) - 1; i >= 0; --i)
    end = outline.insert(end, splitBound[static_cast<size_t>(i)]);

  PlanRecordModel record;
  record.specLevel = localLevel;
  record.childIdx = entry.childIdx;
  record.tailed = res > 0;
  record.splitNums = splitBound.size();
  record.entry = entry.id;
  record.allExtent = entry.alignedConstBits;
  record.firstMemBound = splitBound.front();
  record.isDirectlyRollback = isDirectlyRollback;
  record.replaced.splice(record.replaced.begin(), outline, start, end);
  history.push_back(std::move(record));
}

inline bool IsInplacableBufferPairMatched(
    const std::string &buffer, const std::string &conflictBuffer,
    const std::vector<StorageEntryModel> &entries,
    const std::vector<std::pair<std::string, std::string>> &pairs) {
  const StorageEntryModel *entry = nullptr;
  const StorageEntryModel *conflictEntry = nullptr;
  for (const StorageEntryModel &candidate : entries) {
    if (std::find(candidate.inplaceBuffers.begin(),
                  candidate.inplaceBuffers.end(), buffer) !=
        candidate.inplaceBuffers.end())
      entry = &candidate;
    if (std::find(candidate.inplaceBuffers.begin(),
                  candidate.inplaceBuffers.end(), conflictBuffer) !=
        candidate.inplaceBuffers.end())
      conflictEntry = &candidate;
  }
  return entry && conflictEntry &&
         IsInplacableBufferPairMatched(*entry, *conflictEntry, pairs);
}

inline bool VerifyConflictStage0(
    const StorageEntryModel &entry, const MemoryBoundModel &last,
    const std::vector<StorageEntryModel> &entries,
    const MemPlanStateModel &state,
    const std::vector<std::pair<std::string, std::string>>
        &inplacableBufferPairs) {
  size_t i = 0;
  size_t j = 0;
  while (i < entry.bufferLifeVec.size() &&
         j < last.bufferLifeVec.size()) {
    const BufferLifeModel &lhs = entry.bufferLifeVec[i];
    const BufferLifeModel &rhs = last.bufferLifeVec[j];
    int lo = std::max(lhs.allocTime, rhs.allocTime);
    int hi = std::min(lhs.freeTime, rhs.freeTime);
    if (lo <= hi) {
      bool inplacable = false;
      auto leftProfile = state.conflictProfileByBuffer.find(lhs.buffer);
      auto rightProfile = state.conflictProfileByBuffer.find(rhs.buffer);
      if (leftProfile != state.conflictProfileByBuffer.end() &&
          rightProfile != state.conflictProfileByBuffer.end() &&
          leftProfile->second >= 0 && rightProfile->second >= 0) {
        const size_t left = static_cast<size_t>(leftProfile->second);
        const size_t right = static_cast<size_t>(rightProfile->second);
        inplacable = left < state.conflictProfileCount &&
                     right < state.conflictProfileCount &&
                     state.inplacableConflict[
                         left * state.conflictProfileCount + right] != 0;
      } else {
        inplacable = IsInplacableBufferPairMatched(
            lhs.buffer, rhs.buffer, entries, inplacableBufferPairs);
      }
      if (lo != hi || !inplacable)
        return true;
    }
    if (lhs.freeTime < rhs.freeTime)
      ++i;
    else
      ++j;
  }
  return false;
}

inline bool BufferPipeConflict(const StorageEntryModel &lhs,
                               const StorageEntryModel &rhs) {
  for (const std::string &left : lhs.inplaceBuffers) {
    for (const std::string &right : rhs.inplaceBuffers) {
      bool leftScalar = lhs.scalarBuffers.count(left) != 0;
      bool rightScalar = rhs.scalarBuffers.count(right) != 0;
      if (leftScalar && rightScalar)
        continue;
      if (leftScalar || rightScalar)
        return true;
      if (lhs.dmaBuffers.count(left) || rhs.dmaBuffers.count(right))
        return true;
    }
  }
  return false;
}

inline bool PipeConflictInSameLoop(const StorageEntryModel &lhs,
                                   const StorageEntryModel &rhs) {
  return lhs.parentLoop == rhs.parentLoop;
}

inline bool IsBufferLifeVecConflict(const PlanRecordModel &record,
                                    uint64_t offset,
                                    const StorageEntryModel &entry,
                                    const std::vector<StorageEntryModel> &entries,
                                    const MemPlanStateModel &state) {
  if (!(CheckedAdd(record.firstMemBound->offset, record.allExtent,
                   "plan record end") > offset &&
        record.firstMemBound->offset <
            CheckedAdd(offset, entry.alignedConstBits, "candidate end")))
    return false;
  const StorageEntryModel *planned =
      FindStorageEntry(entries, state, record.entry);
  return planned && CachedConflict(state.lifeConflict,
                                   state.conflictProfileCount, *planned,
                                   entry);
}

inline bool VerifyConflictStageCommon(
    PlanRecHisModel &history, const StorageEntryModel &entry,
    MemBoundListModelIter &start, MemBoundListModel &outline,
    const std::vector<StorageEntryModel> &entries,
    const MemPlanStateModel &state,
    const std::function<bool(const StorageEntryModel &,
                             const StorageEntryModel &)> &conflictChecker) {
  bool touchMemCanUse = false;
  auto foundMem = outline.end();
  for (auto iter = start; iter != outline.end(); ++iter) {
    uint64_t offset = (*iter)->offset;
    bool conflict = false;
    for (const PlanRecordModel &record : history) {
      const StorageEntryModel *planned =
          FindStorageEntry(entries, state, record.entry);
      if (planned &&
          CheckedAdd(record.firstMemBound->offset, record.allExtent,
                     "plan record end") > offset &&
          record.firstMemBound->offset <
              CheckedAdd(offset, entry.alignedConstBits, "candidate end") &&
          conflictChecker(*planned, entry)) {
        conflict = true;
        break;
      }
    }
    if (conflict ||
        (std::next(iter) == outline.end() &&
         (*iter)->extent < entry.alignedConstBits))
      continue;
    touchMemCanUse = true;
    foundMem = iter;
    break;
  }
  if (!touchMemCanUse)
    return true;
  bool conflict = foundMem != start;
  if (conflict) {
    start = foundMem;
    --start;
  }
  return conflict;
}

inline bool VerifyConflictStage2(
    PlanRecHisModel &history, const StorageEntryModel &entry, int specLevel,
    MemBoundListModelIter &start, MemBoundListModel &outline,
    const std::vector<StorageEntryModel> &entries,
    const MemPlanStateModel &state) {
  if (specLevel != SPEC_LEVEL_2)
    return false;
  return VerifyConflictStageCommon(
      history, entry, start, outline, entries, state,
      [](const StorageEntryModel &lhs, const StorageEntryModel &rhs) {
        return PipeConflictInSameLoop(lhs, rhs);
      });
}

inline bool VerifyConflictStage3(
    PlanRecHisModel &history, const StorageEntryModel &entry, int specLevel,
    MemBoundListModelIter &start, MemBoundListModel &outline,
    const std::vector<StorageEntryModel> &entries,
    const MemPlanStateModel &state) {
  if (specLevel != SPEC_LEVEL_3)
    return false;
  return VerifyConflictStageCommon(
      history, entry, start, outline, entries, state,
      [&state](const StorageEntryModel &lhs, const StorageEntryModel &rhs) {
        return CachedConflict(state.pipeConflict,
                              state.conflictProfileCount, lhs, rhs);
      });
}

inline std::vector<int> GetRelationOtherBufferEntries(
    const StorageEntryModel &reuseEntry,
    const std::vector<StorageEntryModel> &entries,
    const MemPlanStateModel &state) {
  std::vector<int> result;
  if (reuseEntry.multiBufferNum > 1) {
    for (int id : reuseEntry.otherBufferRelationEntries) {
      const StorageEntryModel *entry = FindStorageEntry(entries, state, id);
      if (entry && entry->bitsOffset != 0)
        result.push_back(id);
    }
  } else {
    auto it = state.firstBufferEntry2RelationOtherBufferEntry.find(
        reuseEntry.id);
    if (it != state.firstBufferEntry2RelationOtherBufferEntry.end()) {
      for (int id : it->second) {
        const StorageEntryModel *entry = FindStorageEntry(entries, state, id);
        if (entry && entry->bitsOffset != 0)
          result.push_back(id);
      }
    }
  }
  return result;
}

inline bool VerifyConflictStage1(
    PlanRecHisModel &history, const StorageEntryModel &entry,
    MemBoundListModelIter start, MemBoundListModelIter end,
    std::vector<uint64_t> &otherBufferOffsets,
    const std::vector<StorageEntryModel> &entries,
    const MemPlanStateModel &state) {
  if (start != end)
    return true;
  const StorageEntryModel *reuseEntry =
      FindStorageEntry(entries, state, (*start)->lastStorageEntry);
  if (!reuseEntry)
    return true;
  std::vector<int> relations =
      GetRelationOtherBufferEntries(*reuseEntry, entries, state);
  if (relations.empty() || entry.parentLoop < 0 ||
      reuseEntry->parentLoop < 0 ||
      entry.parentLoop != reuseEntry->parentLoop)
    return true;
  if (entry.multiBufferNum > 1 &&
      (relations.size() < entry.multiBufferNum - 1 ||
       entry.otherBufferRelationEntries.empty()))
    return true;

  for (int relationId : relations) {
    const StorageEntryModel *relation =
        FindStorageEntry(entries, state, relationId);
    if (!relation)
      return true;
    uint64_t offset = relation->bitsOffset;
    bool conflict = false;
    for (const PlanRecordModel &record : history) {
      if (IsBufferLifeVecConflict(record, offset, entry, entries, state)) {
        conflict = true;
        break;
      }
    }
    if (conflict) {
      otherBufferOffsets.clear();
      return true;
    }
    otherBufferOffsets.push_back(offset);
  }
  return false;
}

inline void PlanRelationOtherBufferEntryAddress(
    const std::vector<uint64_t> &offsets, StorageEntryModel &entry,
    std::vector<StorageEntryModel> &entries, MemPlanStateModel &state) {
  for (size_t i = 1; i < entry.multiBufferNum; ++i) {
    if (i - 1 >= entry.otherBufferRelationEntries.size() ||
        i - 1 >= offsets.size())
      continue;
    StorageEntryModel *relation = FindStorageEntry(
        entries, state, entry.otherBufferRelationEntries[i - 1]);
    if (relation)
      relation->bitsOffset = offsets[i - 1];
  }

  // The compiler owns these entries through unique_ptrs in the map. Its
  // vec.clear() destroys an earlier speculative set before replacement; do
  // the equivalent for the model's stable list storage.
  ClearSyntheticRelationEntries(state, entry.id);
  std::vector<int> &relations =
      state.firstBufferEntry2RelationOtherBufferEntry[entry.id];
  for (size_t i = entry.multiBufferNum - 1; i < offsets.size(); ++i) {
    StorageEntryModel relation = entry;
    relation.id = state.nextStorageEntryId++;
    relation.bitsOffset = offsets[i];
    relation.otherBufferRelationEntries.clear();
    state.relationStorageEntries.push_back(std::move(relation));
    RegisterStorageEntry(state, state.relationStorageEntries.back());
    relations.push_back(state.relationStorageEntries.back().id);
  }
}

inline void SpecAllocRelationOtherBufferEntry(
    MemBoundListModel &outline, PlanRecHisModel &history,
    StorageEntryModel &entry, uint64_t offset,
    std::vector<StorageEntryModel> &entries, MemPlanStateModel &state) {
  for (auto start = outline.begin(); start != outline.end(); ++start) {
    if ((*start)->offset != offset)
      continue;
    uint64_t size = 0;
    for (auto end = start; end != outline.end(); ++end) {
      size = CheckedAdd(size, (*end)->extent,
                        "relation storage candidate extent");
      if (size < entry.alignedConstBits)
        continue;
      StorageEntryModel *other = nullptr;
      auto synthetic = state.firstBufferEntry2RelationOtherBufferEntry.find(
          entry.id);
      if (synthetic !=
          state.firstBufferEntry2RelationOtherBufferEntry.end()) {
        for (int id : synthetic->second) {
          StorageEntryModel *candidate = FindStorageEntry(entries, state, id);
          if (candidate && candidate->bitsOffset == offset) {
            other = candidate;
            break;
          }
        }
      }
      if (!other && entry.multiBufferNum > 1) {
        for (int id : entry.otherBufferRelationEntries) {
          StorageEntryModel *candidate = FindStorageEntry(entries, state, id);
          if (candidate && candidate->bitsOffset == offset) {
            other = candidate;
            break;
          }
        }
      }
      if (!other)
        throw std::runtime_error(
            "SpecAllocRelationOtherBufferEntry: relation not found");
      UpdateOutline(outline, history, *other, start, end, size, true,
                    SPEC_LEVEL_1, state.splitOutline);
      return;
    }
  }
}

inline bool IsSamePlanAsLastRollBack(uint64_t allocOffset, int childIdx,
                                     const SpecInfoModel &specInfo) {
  return childIdx == specInfo.rollbackIdx &&
         allocOffset == specInfo.rollbackAddr;
}

inline bool SpecAlloc(
    MemBoundListModel &outline, PlanRecHisModel &history,
    StorageEntryModel &entry, const SpecInfoModel &specInfo, int localLevel,
    std::vector<StorageEntryModel> &entries, MemPlanStateModel &state,
    const std::vector<std::pair<std::string, std::string>>
        &inplacableBufferPairs) {
  for (const PlanRecordModel &record : history)
    if (record.entry == entry.id)
      return true;
  if (entry.alignedConstBits == 0) {
    entry.bitsOffset = 0;
    return true;
  }

  for (auto start = outline.begin(); start != outline.end(); ++start) {
    uint64_t size = 0;
    uint64_t allocOffset = (*start)->offset;
    for (auto end = start; end != outline.end(); ++end) {
      size = CheckedAdd(size, (*end)->extent,
                        "storage candidate extent");
      if (IsSamePlanAsLastRollBack(allocOffset, entry.childIdx, specInfo) ||
          VerifyConflictStage0(entry, **end, entries, state,
                               inplacableBufferPairs)) {
        start = end;
        break;
      }
      if (size < entry.alignedConstBits)
        continue;

      std::vector<uint64_t> otherBufferOffsets;
      if (localLevel == SPEC_LEVEL_1 &&
          VerifyConflictStage1(history, entry, start, end,
                               otherBufferOffsets, entries, state))
        break;
      if (VerifyConflictStage2(history, entry, localLevel, start, outline,
                               entries, state))
        break;
      if (VerifyConflictStage3(history, entry, localLevel, start, outline,
                               entries, state))
        break;

      entry.bitsOffset = allocOffset;
      UpdateOutline(outline, history, entry, start, end, size, false,
                    localLevel, state.splitOutline);
      if (localLevel == SPEC_LEVEL_1) {
        if (otherBufferOffsets.empty())
          throw std::runtime_error("SPEC_LEVEL_1 missing relation offsets");
        PlanRelationOtherBufferEntryAddress(otherBufferOffsets, entry,
                                            entries, state);
        for (uint64_t offset : otherBufferOffsets)
          SpecAllocRelationOtherBufferEntry(outline, history, entry, offset,
                                            entries, state);
      }
      return true;
    }
  }
  return false;
}

inline bool MultiSpecPlan(
    SpecInfoModel &specInfo, MemBoundListModel &outline,
    PlanRecHisModel &history, StorageEntryModel &entry,
    std::vector<StorageEntryModel> &entries, MemPlanStateModel &state,
    const std::vector<std::pair<std::string, std::string>>
        &inplacableBufferPairs) {
  for (int level = specInfo.specLevel; level >= specInfo.minLevel; --level) {
    if (!SpecAlloc(outline, history, entry, specInfo, level, entries, state,
                   inplacableBufferPairs))
      continue;
    if (specInfo.childIdx == specInfo.specStartIdx)
      specInfo.specLevel = specInfo.maxLevel;
    ++specInfo.childIdx;
    return true;
  }
  return false;
}

inline PlanRecordModel RollbackOutline(PlanRecHisModel &history,
                                       MemBoundListModel &outline) {
  PlanRecordModel record = std::move(history.back());
  auto it = std::find(outline.begin(), outline.end(), record.firstMemBound);
  if (record.headed) {
    --it;
    it = outline.erase(it);
  }
  for (size_t i = 0; i < record.splitNums; ++i)
    it = outline.erase(it);
  if (record.tailed)
    it = outline.erase(it);
  outline.splice(it, record.replaced);
  history.pop_back();
  return record;
}

inline bool RollBackForAllocFail(
    uint64_t failedAlignedBits, SpecInfoModel &specInfo,
    MemBoundListModel &outline, PlanRecHisModel &history,
    const std::vector<StorageEntryModel> &entries, MemPlanStateModel &state,
    uint64_t maxBits) {
  bool hasEnoughRollBackSize = false;
  if (specInfo.childIdx > specInfo.specStartIdx)
    specInfo.specStartIdx = specInfo.childIdx;
  while (!hasEnoughRollBackSize && !history.empty() && !outline.empty()) {
    PlanRecordModel record = RollbackOutline(history, outline);
    ClearSyntheticRelationEntries(state, record.entry);
    const StorageEntryModel *entry =
        FindStorageEntry(entries, state, record.entry);
    if (record.isDirectlyRollback ||
        (entry && entry->multiBufferNum > 1 &&
         entry->otherBufferRelationEntries.empty()))
      continue;
    specInfo.childIdx = record.childIdx;
    specInfo.specLevel = record.specLevel;
    if (specInfo.specLevel <= specInfo.minLevel)
      continue;
    specInfo.rollbackAddr =
        specInfo.childIdx == -1
            ? std::numeric_limits<uint64_t>::max()
            : entries[static_cast<size_t>(specInfo.childIdx + 1)].bitsOffset;
    specInfo.rollbackIdx = specInfo.childIdx;
    if (specInfo.rollbackAddr + failedAlignedBits > maxBits)
      continue;
    hasEnoughRollBackSize = true;
  }
  return hasEnoughRollBackSize;
}

enum class PlanStatusModel { PLAN_SUCCESS, RESTART_NEW_PLAN, CONTINUE_PLAN,
                             PLAN_FAILED };

inline PlanStatusModel ApplyFailStrategy(
    uint64_t failedAlignedBits, SpecInfoModel &specInfo,
    MemBoundListModel &outline, PlanRecHisModel &history,
    const std::vector<StorageEntryModel> &entries, MemPlanStateModel &state,
    uint64_t maxBits) {
  (void)RollBackForAllocFail(failedAlignedBits, specInfo, outline, history,
                             entries, state, maxBits);
  if (specInfo.specLevel > SPEC_LEVEL_0 && specInfo.childIdx >= 0) {
    --specInfo.specLevel;
    return PlanStatusModel::CONTINUE_PLAN;
  }
  if (!state.splitOutline) {
    state.splitOutline = true;
    return PlanStatusModel::RESTART_NEW_PLAN;
  }
  return PlanStatusModel::PLAN_FAILED;
}

inline uint64_t GetMaxAllocatedBits(
    const std::vector<StorageEntryModel> &entries,
    const MemPlanStateModel &state) {
  uint64_t result = 0;
  for (const StorageEntryModel &entry : entries)
    result = std::max(
        result, CheckedAdd(entry.bitsOffset, entry.alignedConstBits,
                           "planned storage end"));
  for (const StorageEntryModel &entry : state.relationStorageEntries)
    result = std::max(
        result, CheckedAdd(entry.bitsOffset, entry.alignedConstBits,
                           "planned relation storage end"));
  return result;
}

inline uint64_t PlanMemAddressForLevel0(
    std::vector<StorageEntryModel> &entries, uint64_t alignUnit,
    MemPlanStateModel &state,
    const std::vector<std::pair<std::string, std::string>>
        &inplacableBufferPairs) {
  const bool splitOutline = state.splitOutline;
  state = MemPlanStateModel{};
  state.splitOutline = splitOutline;
  for (const StorageEntryModel &entry : entries)
    state.nextStorageEntryId =
        std::max(state.nextStorageEntryId, entry.id + 1);
  RebuildStorageEntryLookup(entries, state);
  BuildStorageEntryConflictTables(entries, state, inplacableBufferPairs);
  MemBoundListModel outline;
  outline.push_back(std::make_shared<MemoryBoundModel>(MemoryBoundModel{
      {}, 0, std::numeric_limits<uint64_t>::max(), -1}));
  PlanRecHisModel history;
  SpecInfoModel specInfo;
  specInfo.specLevel = SPEC_LEVEL_0;
  specInfo.maxLevel = SPEC_LEVEL_0;
  while (specInfo.childIdx < static_cast<int>(entries.size()) - 1) {
    StorageEntryModel &entry =
        entries[static_cast<size_t>(specInfo.childIdx + 1)];
    entry.alignedConstBits = AlignUp(entry.constBits, alignUnit);
    entry.childIdx = specInfo.childIdx;
    if (!MultiSpecPlan(specInfo, outline, history, entry, entries, state,
                       inplacableBufferPairs))
      throw std::runtime_error("PlanMemory level-0 allocation failed");
  }
  return GetMaxAllocatedBits(entries, state);
}

inline bool PlanMemAddressOfWholeLocalBuffer(
    std::vector<StorageEntryModel> &entries, uint64_t maxBits,
    uint64_t alignUnit, uint64_t &requiredBits, MemPlanStateModel &state,
    const std::vector<std::pair<std::string, std::string>>
        &inplacableBufferPairs) {
  if (entries.empty()) {
    requiredBits = 0;
    return true;
  }
  if (entries.size() == 1) {
    entries.front().alignedConstBits =
        AlignUp(entries.front().constBits, alignUnit);
    entries.front().bitsOffset = 0;
    requiredBits = entries.front().alignedConstBits;
    return requiredBits <= maxBits;
  }

  GetReorderRootStorageEntry(entries);
  state = MemPlanStateModel{};
  for (const StorageEntryModel &entry : entries)
    state.nextStorageEntryId =
        std::max(state.nextStorageEntryId, entry.id + 1);
  RebuildStorageEntryLookup(entries, state);
  BuildStorageEntryConflictTables(entries, state, inplacableBufferPairs);
  MemBoundListModel outline;
  outline.push_back(std::make_shared<MemoryBoundModel>(
      MemoryBoundModel{{}, 0, maxBits, -1}));
  PlanRecHisModel history;
  SpecInfoModel specInfo;

  while (specInfo.childIdx < static_cast<int>(entries.size()) - 1) {
    StorageEntryModel &entry =
        entries[static_cast<size_t>(specInfo.childIdx + 1)];
    entry.alignedConstBits = AlignUp(entry.constBits, alignUnit);
    entry.childIdx = specInfo.childIdx;
    if (!MultiSpecPlan(specInfo, outline, history, entry, entries, state,
                       inplacableBufferPairs)) {
      PlanStatusModel status = ApplyFailStrategy(
          entry.alignedConstBits, specInfo, outline, history, entries, state,
          maxBits);
      if (status == PlanStatusModel::RESTART_NEW_PLAN) {
        specInfo = SpecInfoModel{};
        continue;
      }
      if (status == PlanStatusModel::PLAN_FAILED) {
        requiredBits = PlanMemAddressForLevel0(
            entries, alignUnit, state, inplacableBufferPairs);
        return false;
      }
    }
  }
  requiredBits = GetMaxAllocatedBits(entries, state);
  return true;
}

inline std::map<std::string, std::vector<uint64_t>> UpdateBuffer2Offsets(
    const std::vector<StorageEntryModel> &entries,
    const MemPlanStateModel &state,
    std::map<std::string, uint64_t> *buffer2StorageExtentBits = nullptr) {
  std::map<std::string, std::vector<uint64_t>> buffer2Offsets;
  auto update = [&](const StorageEntryModel &entry) {
    for (const std::string &buffer : entry.inplaceBuffers)
      buffer2Offsets[buffer].push_back(BitsToBytes(entry.bitsOffset));
    if (buffer2StorageExtentBits)
      for (const std::string &buffer : entry.inplaceBuffers)
        (*buffer2StorageExtentBits)[buffer] =
            AlignUp(entry.constBits, kBitsPerByte);
  };
  for (const StorageEntryModel &entry : entries)
    update(entry);
  for (const StorageEntryModel &entry : state.relationStorageEntries)
    update(entry);
  return buffer2Offsets;
}

inline PlanMemoryModelResult PlanLocalMemoryImpl(
    const PlanMemoryInput &input, bool restrictInplaceAsISA,
    std::optional<uint32_t> randomSeed, DebugTrace *trace = nullptr) {
  std::vector<BufferInfoRecord> bufferInfos = MeasureStage(
      trace, "PlanMemory.PrepareBufferInfos",
      [&] { return BuildBufferInfos(input); });
  PlanMemoryModelResult finalResult;
  if (randomSeed && *randomSeed >= 20)
    throw std::runtime_error("PlanMemory random seed must be in [0, 19]");
  const uint32_t firstAttempt = randomSeed.value_or(0);
  const uint32_t attemptEnd = randomSeed ? firstAttempt + 1 : 20;
  std::unique_ptr<PreparedMemLivenessAnalysis> preparedLiveness =
      MeasureStage(trace, "PlanMemory.PrepareLiveness", [&] {
        return std::make_unique<PreparedMemLivenessAnalysis>(input, trace);
      });
  std::unique_ptr<PreparedStorageEntryAnalysis> preparedStorage;
  for (uint32_t attempt = firstAttempt; attempt < attemptEnd; ++attempt) {
    LifetimeAnalysis liveness = MeasureStage(
        trace, "PlanMemory.AnalyzeLifetimes", [&] {
          return analyzeLifetimes(input, *preparedLiveness, attempt,
                                  restrictInplaceAsISA, !preparedStorage);
        });
    if (!preparedStorage)
      preparedStorage = MeasureStage(
          trace, "PlanMemory.PrepareStorage", [&] {
            return std::make_unique<PreparedStorageEntryAnalysis>(bufferInfos,
                                                                   liveness);
          });
    std::vector<StorageEntryModel> entries = MeasureStage(
        trace, "PlanMemory.GenerateStorageEntry", [&] {
          return GenerateStorageEntry(*preparedStorage, liveness);
        });
    MeasureStage(trace, "PlanMemory.MergeInplace", [&] {
      MergeInplaceSE(entries, liveness.inplacePairList);
    });
    MeasureStage(trace, "PlanMemory.ExpandMultiBuffer", [&] {
      ExpandMultiBufferStorageEntry(entries);
    });
    const uint64_t noReuseBits = MeasureStage(
        trace, "PlanMemory.CalculateNoReuse", [&] {
          uint64_t bits = 0;
          for (const StorageEntryModel &entry : entries)
            bits = CheckedAdd(bits, AlignUp(entry.constBits, 256),
                              "total non-reuse UB size");
          return bits;
        });
    uint64_t requiredBits = 0;
    bool success = false;
    MemPlanStateModel planState;
    MeasureStage(trace, "PlanMemory.PlanAddress", [&] {
      if (noReuseBits <= kUBCapacityBits) {
        requiredBits = PlanBuffersWithoutReuse(entries, 256);
        success = true;
      } else {
        success = PlanMemAddressOfWholeLocalBuffer(
            entries, kUBCapacityBits, 256, requiredBits, planState,
            liveness.inplacableBufferPairs);
      }
    });

    // A failed non-final retry contributes no externally observable state:
    // the next seed rebuilds liveness/storage state from the immutable input,
    // and only a successful attempt or the final failed attempt is returned.
    // Avoid constructing name-keyed lookup maps, planned-buffer records and
    // peak statistics that would immediately be discarded. This mirrors the
    // compiler pass, which only commits GetBuffer2Offsets() on success.
    if (!success && attempt + 1 < attemptEnd)
      continue;

    PlanMemoryModelResult result = MeasureStage(
        trace, "PlanMemory.MaterializeResult", [&] {
          PlanMemoryModelResult materialized;
          materialized.success = success;
          materialized.overflow = !success;
          materialized.selectedSeed = attempt;
          materialized.requiredBits = requiredBits;
          materialized.peakBits = requiredBits;
          materialized.capacityBits = kUBCapacityBits;
          materialized.inplacePairs = liveness.inplacePairList;
          materialized.multiBufferNums = liveness.buffer2MultiNum;
          std::unordered_map<std::string, LifetimeRecord> lifeByName;
          lifeByName.reserve(liveness.records.size());
          for (const LifetimeRecord &life : liveness.records)
            lifeByName[life.name] = life;
          std::unordered_map<std::string, BufferInfoRecord> infoByName;
          infoByName.reserve(bufferInfos.size());
          for (const BufferInfoRecord &info : bufferInfos)
            infoByName[info.name] = info;
          std::map<std::string, PlannedBufferRecord> plannedByName;
          std::map<std::string, std::vector<uint64_t>> buffer2Offsets =
              UpdateBuffer2Offsets(entries, planState);
          for (auto &item : buffer2Offsets) {
            const std::string &name = item.first;
            const BufferInfoRecord &info = infoByName.at(name);
            const LifetimeRecord &life = lifeByName.at(name);
            plannedByName.emplace(
                name, PlannedBufferRecord{name, info.constBits, info.constBits,
                                          std::move(item.second),
                                          life.directAllocTime,
                                          life.directFreeTime});
          }
          for (auto &item : plannedByName)
            materialized.buffers.push_back(std::move(item.second));
          // PlanMemory.cpp::DumpDebugState reports PLANMEM_PEAK from each
          // buffer's byte offset plus its original byte-aligned extent.
          materialized.peakBits = 0;
          for (const PlannedBufferRecord &buffer : materialized.buffers) {
            const uint64_t extentBits =
                AlignUp(buffer.extentBits, kBitsPerByte);
            for (uint64_t offsetBytes : buffer.offsetsBytes) {
              const uint64_t offsetBits = CheckedMul(
                  offsetBytes, kBitsPerByte, "planned UB offset");
              materialized.peakBits = std::max(
                  materialized.peakBits,
                  CheckedAdd(offsetBits, extentBits, "planned UB peak"));
            }
          }
          return materialized;
        });
    finalResult = std::move(result);
    if (success)
      return finalResult;
  }
  return finalResult;
}

inline PlanMemoryModelResult PlanLocalMemoryImpl(
    const fs::path &beforeIR, bool restrictInplaceAsISA,
    std::optional<uint32_t> randomSeed, DebugTrace *trace = nullptr) {
  if (!fs::exists(beforeIR) || !fs::is_regular_file(beforeIR))
    throw std::runtime_error("PlanMemory-before IR does not exist: " +
                             beforeIR.string());
  return PlanLocalMemoryImpl(ParsePlanMemoryInput(beforeIR, "AIV"),
                             restrictInplaceAsISA, randomSeed, trace);
}

inline PlanMemoryModelResult
PlanLocalMemory(const fs::path &beforeIR, bool restrictInplaceAsISA = false,
                DebugTrace *trace = nullptr) {
  return PlanLocalMemoryImpl(beforeIR, restrictInplaceAsISA, std::nullopt,
                             trace);
}

inline PlanMemoryModelResult
PlanLocalMemoryForSeed(const fs::path &beforeIR, uint32_t randomSeed,
                       bool restrictInplaceAsISA = false,
                       DebugTrace *trace = nullptr) {
  return PlanLocalMemoryImpl(beforeIR, restrictInplaceAsISA, randomSeed,
                             trace);
}

inline PlanMemoryModelResult
PlanLocalMemory(const PlanMemoryInput &input,
                bool restrictInplaceAsISA = false,
                DebugTrace *trace = nullptr) {
  return PlanLocalMemoryImpl(input, restrictInplaceAsISA, std::nullopt,
                             trace);
}

inline PlanMemoryModelResult
PlanLocalMemoryForSeed(const PlanMemoryInput &input, uint32_t randomSeed,
                       bool restrictInplaceAsISA = false,
                       DebugTrace *trace = nullptr) {
  return PlanLocalMemoryImpl(input, restrictInplaceAsISA, randomSeed, trace);
}



} // namespace cvub

#endif
