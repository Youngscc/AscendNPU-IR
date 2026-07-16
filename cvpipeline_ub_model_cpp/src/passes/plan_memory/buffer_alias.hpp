// Buffer alias state shared by MemLivenessAnalysis.
#ifndef CVPIPELINE_UB_MODEL_CPP_BUFFER_ALIAS_HPP
#define CVPIPELINE_UB_MODEL_CPP_BUFFER_ALIAS_HPP

#include "normalize_loop_iterator.hpp"

namespace cvub {

struct OpInfo {
  OperationRecord operation;
  int index = 0;
  std::vector<std::string> gen;
  std::vector<std::string> kill;
};

// Keep the original spelling from PlanMemory.h for source-to-model lookup.
enum class BufferStatus { UNDEFFINED = 0, DEFFINED, GENED, KILLED };

class BufferAliasMap {
public:
  using AliasPair = std::pair<std::string, bool>;

  const std::vector<AliasPair> &GetAliasBufferCondPairs(
      const std::string &value) const {
    static const std::vector<AliasPair> empty;
    auto it = buffer2AliasVec.find(value);
    return it == buffer2AliasVec.end() ? empty : it->second;
  }

  std::vector<std::string> GetAliasBuffers(const std::string &value) const {
    std::vector<std::string> result;
    for (const AliasPair &pair : GetAliasBufferCondPairs(value))
      result.push_back(pair.first);
    return result;
  }

  void UpdateBufferAlias(const std::string &value,
                         const std::string &aliasValue, bool conditional) {
    if (value.empty() || aliasValue.empty())
      return;
    std::vector<std::string> left = GetAliasBuffers(value);
    std::vector<std::string> right = GetAliasBuffers(aliasValue);
    left.push_back(value);
    right.push_back(aliasValue);
    UpdateBuffer2AliasVec(value, aliasValue, left, right, conditional);
    UpdateBuffer2AliasVec(aliasValue, value, right, left, conditional);
  }

private:
  std::optional<bool> FindBufferCondPair(const std::string &from,
                                         const std::string &to) const {
    auto it = buffer2AliasVec.find(from);
    if (it == buffer2AliasVec.end())
      return std::nullopt;
    for (const AliasPair &pair : it->second)
      if (pair.first == to)
        return pair.second;
    return std::nullopt;
  }

  void insertOrUpdate(const std::string &from, const std::string &to,
                      bool conditional) {
    for (AliasPair &pair : buffer2AliasVec[from]) {
      if (pair.first == to) {
        pair.second = pair.second || conditional;
        return;
      }
    }
    buffer2AliasVec[from].push_back({to, conditional});
  }

  void UpdateBuffer2AliasVec(const std::string &value,
                             const std::string &aliasValue,
                             const std::vector<std::string> &left,
                             const std::vector<std::string> &right,
                             bool conditional) {
    for (const std::string &leftValue : left) {
      bool leftCond = FindBufferCondPair(value, leftValue).value_or(false);
      for (const std::string &rightValue : right) {
        bool rightCond =
            FindBufferCondPair(aliasValue, rightValue).value_or(false);
        insertOrUpdate(leftValue, rightValue,
                       conditional || leftCond || rightCond);
      }
    }
  }

  std::map<std::string, std::vector<AliasPair>> buffer2AliasVec;
};


} // namespace cvub

#endif
