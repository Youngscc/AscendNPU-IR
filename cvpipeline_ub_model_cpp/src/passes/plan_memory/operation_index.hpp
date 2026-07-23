// Shared, read-only indexes for the finalized PlanMemory operation stream.
#ifndef CVPIPELINE_UB_MODEL_CPP_PLAN_MEMORY_OPERATION_INDEX_HPP
#define CVPIPELINE_UB_MODEL_CPP_PLAN_MEMORY_OPERATION_INDEX_HPP

#include "../../ir/semantic_ir.hpp"

#include <memory>

namespace cvub {

struct PlanMemoryOperationIndexStorage {
  std::vector<std::vector<std::string>> operandsByRecord;
  std::vector<std::vector<std::string>> resultsByRecord;
  std::vector<size_t> positionByLinearIndex;
  std::unordered_map<int, size_t> positionByOperationId;
  std::unordered_map<std::string, size_t> valueIds;
  std::vector<std::vector<size_t>> usersByValueId;
  std::unordered_map<int, std::vector<std::string>> regionArguments;
  std::unordered_map<int, std::vector<std::string>> regionYieldedValues;
};

inline std::shared_ptr<const PlanMemoryOperationIndexStorage>
BuildPlanMemoryOperationIndexStorage(
    const std::vector<OperationRecord> &operations) {
  static constexpr size_t kMissingPosition =
      std::numeric_limits<size_t>::max();
  auto result = std::make_shared<PlanMemoryOperationIndexStorage>();
  result->operandsByRecord.reserve(operations.size());
  result->resultsByRecord.reserve(operations.size());
  result->positionByOperationId.reserve(operations.size());
  result->valueIds.reserve(operations.size() * 2);
  result->usersByValueId.reserve(operations.size() * 2);
  result->regionArguments.reserve(operations.size());
  result->regionYieldedValues.reserve(operations.size());
  auto internValue = [&](const std::string &value) {
    auto found = result->valueIds.find(value);
    if (found != result->valueIds.end())
      return found->second;
    const size_t id = result->valueIds.size();
    result->valueIds.emplace(value, id);
    result->usersByValueId.emplace_back();
    return id;
  };
  size_t maximumLinearIndex = 0;
  for (const OperationRecord &operation : operations) {
    if (operation.index < 0)
      throw std::runtime_error(
          "PlanMemoryOperationIndex: negative linear index");
    maximumLinearIndex =
        std::max(maximumLinearIndex, static_cast<size_t>(operation.index));
  }
  result->positionByLinearIndex.assign(
      operations.empty() ? 0 : maximumLinearIndex + 1, kMissingPosition);
  for (size_t position = 0; position < operations.size(); ++position) {
    const OperationRecord &operation = operations[position];
    const size_t linearIndex = static_cast<size_t>(operation.index);
    if (result->positionByLinearIndex[linearIndex] != kMissingPosition)
      throw std::runtime_error(
          "PlanMemoryOperationIndex: duplicate linear index");
    result->positionByLinearIndex[linearIndex] = position;
    // Match the previous Liveness behavior: the first record carrying a
    // given semantic operation id is authoritative.
    result->positionByOperationId.emplace(operation.operationId, position);
    result->resultsByRecord.push_back(operationResultNames(operation));
    result->operandsByRecord.push_back(operationOperandNames(operation));
    for (const std::string &value : result->resultsByRecord.back())
      (void)internValue(value);
    for (const std::string &operand : result->operandsByRecord.back()) {
      std::vector<size_t> &users =
          result->usersByValueId[internValue(operand)];
      if (users.empty() || users.back() != position)
        users.push_back(position);
    }
    if (!operation.regionPath.empty()) {
      const int region = operation.regionPath.back();
      result->regionArguments.emplace(region, operation.blockArguments);
      if (operation.opName == "scf.yield")
        result->regionYieldedValues.emplace(
            region, result->operandsByRecord.back());
    }
  }
  return result;
}

// This class deliberately derives every entry through the existing semantic
// helpers.  It changes query cost, not PlanMemory semantics: operand/result
// filtering, branch ordering, and user ordering remain exactly the order
// produced by operationOperandNames/operationResultNames and the linear IR.
class PlanMemoryOperationIndex {
public:
  explicit PlanMemoryOperationIndex(
      const std::vector<OperationRecord> &inputOperations,
      std::shared_ptr<const PlanMemoryOperationIndexStorage> inputStorage = {})
      : operations(inputOperations),
        storage(inputStorage ? std::move(inputStorage)
                             : BuildPlanMemoryOperationIndexStorage(
                                   inputOperations)) {}

  const std::vector<std::string> &
  operands(const OperationRecord &operation) const {
    return storage->operandsByRecord.at(position(operation));
  }

  const std::vector<std::string> &
  results(const OperationRecord &operation) const {
    return storage->resultsByRecord.at(position(operation));
  }

  const OperationRecord &operation(int operationId) const {
    return operations.at(positionForOperationId(operationId));
  }

  const std::vector<std::string> &operands(int operationId) const {
    return storage->operandsByRecord.at(positionForOperationId(operationId));
  }

  const std::vector<std::string> &results(int operationId) const {
    return storage->resultsByRecord.at(positionForOperationId(operationId));
  }

  bool isCanonicalRecord(const OperationRecord &operation) const {
    auto found = storage->positionByOperationId.find(operation.operationId);
    return found != storage->positionByOperationId.end() &&
           found->second == position(operation);
  }

  const std::vector<size_t> &users(const std::string &value) const {
    static const std::vector<size_t> empty;
    auto found = storage->valueIds.find(value);
    return found == storage->valueIds.end()
               ? empty
               : storage->usersByValueId[found->second];
  }

  const std::vector<std::string> &argumentsForRegion(int region) const {
    static const std::vector<std::string> empty;
    auto found = storage->regionArguments.find(region);
    return found == storage->regionArguments.end() ? empty : found->second;
  }

  const std::vector<std::string> &yieldedValuesForRegion(int region) const {
    static const std::vector<std::string> empty;
    auto found = storage->regionYieldedValues.find(region);
    return found == storage->regionYieldedValues.end() ? empty
                                                       : found->second;
  }

private:
  static constexpr size_t kMissingPosition =
      std::numeric_limits<size_t>::max();

  size_t position(const OperationRecord &operation) const {
    if (operation.index < 0 ||
        static_cast<size_t>(operation.index) >=
            storage->positionByLinearIndex.size())
      throw std::runtime_error(
          "PlanMemoryOperationIndex: operation is outside indexed stream");
    const size_t position =
        storage->positionByLinearIndex[static_cast<size_t>(operation.index)];
    if (position == kMissingPosition)
      throw std::runtime_error(
          "PlanMemoryOperationIndex: operation is outside indexed stream");
    return position;
  }

  size_t positionForOperationId(int operationId) const {
    auto found = storage->positionByOperationId.find(operationId);
    if (found == storage->positionByOperationId.end())
      throw std::runtime_error(
          "PlanMemoryOperationIndex: unknown operation id");
    return found->second;
  }

  const std::vector<OperationRecord> &operations;
  std::shared_ptr<const PlanMemoryOperationIndexStorage> storage;
};

} // namespace cvub

#endif
