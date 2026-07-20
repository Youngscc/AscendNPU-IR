// MemoryBound speculative address planning and public PlanMemory entry.
#ifndef CVPIPELINE_UB_MODEL_CPP_MEM_PLAN_HPP
#define CVPIPELINE_UB_MODEL_CPP_MEM_PLAN_HPP

#include "storage_entry.hpp"

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
};

inline StorageEntryModel *FindStorageEntry(
    std::vector<StorageEntryModel> &entries, MemPlanStateModel &state, int id) {
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
      if (lo != hi ||
          !IsInplacableBufferPairMatched(lhs.buffer, rhs.buffer, entries,
                                         inplacableBufferPairs))
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
  return planned &&
         IsBufferLifeVecConflict(planned->bufferLifeVec,
                                 entry.bufferLifeVec);
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
      [](const StorageEntryModel &lhs, const StorageEntryModel &rhs) {
        return BufferPipeConflict(lhs, rhs);
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
          VerifyConflictStage0(entry, **end, entries,
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
    std::optional<uint32_t> randomSeed) {
  std::vector<BufferInfoRecord> bufferInfos = BuildBufferInfos(input);
  PlanMemoryModelResult finalResult;
  if (randomSeed && *randomSeed >= 20)
    throw std::runtime_error("PlanMemory random seed must be in [0, 19]");
  const uint32_t firstAttempt = randomSeed.value_or(0);
  const uint32_t attemptEnd = randomSeed ? firstAttempt + 1 : 20;
  for (uint32_t attempt = firstAttempt; attempt < attemptEnd; ++attempt) {
    LifetimeAnalysis liveness =
        analyzeLifetimes(input, attempt, restrictInplaceAsISA);
    std::vector<StorageEntryModel> entries =
        GenerateStorageEntry(bufferInfos, liveness);
    MergeInplaceSE(entries, liveness.inplacePairList);
    ExpandMultiBufferStorageEntry(entries);
    uint64_t noReuseBits = 0;
    for (const StorageEntryModel &entry : entries)
      noReuseBits = CheckedAdd(noReuseBits, AlignUp(entry.constBits, 256),
                               "total non-reuse UB size");
    uint64_t requiredBits = 0;
    bool success = false;
    MemPlanStateModel planState;
    if (noReuseBits <= kUBCapacityBits) {
      requiredBits = PlanBuffersWithoutReuse(entries, 256);
      success = true;
    } else {
      success = PlanMemAddressOfWholeLocalBuffer(
          entries, kUBCapacityBits, 256, requiredBits, planState,
          liveness.inplacableBufferPairs);
    }

    PlanMemoryModelResult result;
    result.success = success;
    result.overflow = !success;
    result.selectedSeed = attempt;
    result.requiredBits = requiredBits;
    result.peakBits = requiredBits;
    result.capacityBits = kUBCapacityBits;
    result.inplacePairs = liveness.inplacePairList;
    result.multiBufferNums = liveness.buffer2MultiNum;
    std::map<std::string, LifetimeRecord> lifeByName;
    for (const LifetimeRecord &life : liveness.records)
      lifeByName[life.name] = life;
    std::map<std::string, BufferInfoRecord> infoByName;
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
                                    std::move(item.second), life.directAllocTime,
                                    life.directFreeTime});
    }
    for (auto &item : plannedByName)
      result.buffers.push_back(std::move(item.second));
    // PlanMemory.cpp::DumpDebugState reports PLANMEM_PEAK from each buffer's
    // byte offset plus its original byte-aligned extent for both successful
    // and level-0 fallback plans. The 256-bit aligned StorageEntry size is
    // reported separately as the required capacity when planning overflows.
    result.peakBits = 0;
    for (const PlannedBufferRecord &buffer : result.buffers) {
      const uint64_t extentBits = AlignUp(buffer.extentBits, kBitsPerByte);
      for (uint64_t offsetBytes : buffer.offsetsBytes) {
        const uint64_t offsetBits =
            CheckedMul(offsetBytes, kBitsPerByte, "planned UB offset");
        result.peakBits =
            std::max(result.peakBits,
                     CheckedAdd(offsetBits, extentBits, "planned UB peak"));
      }
    }
    finalResult = std::move(result);
    if (success)
      return finalResult;
  }
  return finalResult;
}

inline PlanMemoryModelResult PlanLocalMemoryImpl(
    const fs::path &beforeIR, bool restrictInplaceAsISA,
    std::optional<uint32_t> randomSeed) {
  if (!fs::exists(beforeIR) || !fs::is_regular_file(beforeIR))
    throw std::runtime_error("PlanMemory-before IR does not exist: " +
                             beforeIR.string());
  return PlanLocalMemoryImpl(ParsePlanMemoryInput(beforeIR, "AIV"),
                             restrictInplaceAsISA, randomSeed);
}

inline PlanMemoryModelResult
PlanLocalMemory(const fs::path &beforeIR, bool restrictInplaceAsISA = false) {
  return PlanLocalMemoryImpl(beforeIR, restrictInplaceAsISA, std::nullopt);
}

inline PlanMemoryModelResult
PlanLocalMemoryForSeed(const fs::path &beforeIR, uint32_t randomSeed,
                       bool restrictInplaceAsISA = false) {
  return PlanLocalMemoryImpl(beforeIR, restrictInplaceAsISA, randomSeed);
}

inline PlanMemoryModelResult
PlanLocalMemory(const PlanMemoryInput &input,
                bool restrictInplaceAsISA = false) {
  return PlanLocalMemoryImpl(input, restrictInplaceAsISA, std::nullopt);
}

inline PlanMemoryModelResult
PlanLocalMemoryForSeed(const PlanMemoryInput &input, uint32_t randomSeed,
                       bool restrictInplaceAsISA = false) {
  return PlanLocalMemoryImpl(input, restrictInplaceAsISA, randomSeed);
}



} // namespace cvub

#endif
