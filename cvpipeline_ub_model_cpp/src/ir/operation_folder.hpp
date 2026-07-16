#ifndef CVPIPELINE_UB_MODEL_CPP_OPERATION_FOLDER_HPP
#define CVPIPELINE_UB_MODEL_CPP_OPERATION_FOLDER_HPP

#include "../pipeline/buffer_topology.hpp"

namespace cvub {

struct ArithIntegerConstant {
  uint64_t bits = 0;
  unsigned width = 0;
};

inline std::optional<unsigned>
ArithIntegerBitWidth(const std::string &type) {
  if (type == "index")
    return 64;
  if (type.size() < 2 || type.front() != 'i')
    return std::nullopt;
  try {
    size_t consumed = 0;
    const unsigned width =
        static_cast<unsigned>(std::stoul(type.substr(1), &consumed));
    return consumed == type.size() - 1 && width >= 1 && width <= 64
               ? std::optional<unsigned>(width)
               : std::nullopt;
  } catch (const std::exception &) {
    return std::nullopt;
  }
}

inline std::optional<ArithIntegerConstant>
ParseArithIntegerConstant(const GenericOperation &operation) {
  if (operation.name != "arith.constant" || operation.results.size() != 1 ||
      operation.resultTypes.size() != 1)
    return std::nullopt;
  const std::optional<unsigned> width =
      ArithIntegerBitWidth(operation.resultTypes.front());
  if (!width)
    return std::nullopt;
  std::string literal =
      FindDictionaryValue(operation.properties, "value");
  if (literal.empty())
    literal = FindDictionaryValue(operation.attributes, "value");
  const size_t separator = literal.find(':');
  if (separator != std::string::npos)
    literal.resize(separator);
  literal = trim(std::move(literal));
  if (literal == "true")
    literal = "1";
  else if (literal == "false")
    literal = "0";
  try {
    size_t consumed = 0;
    uint64_t bits = 0;
    if (!literal.empty() && literal.front() == '-')
      bits = static_cast<uint64_t>(std::stoll(literal, &consumed, 0));
    else
      bits = std::stoull(literal, &consumed, 0);
    if (consumed != literal.size())
      return std::nullopt;
    if (*width < 64)
      bits &= (uint64_t{1} << *width) - 1;
    return ArithIntegerConstant{bits, *width};
  } catch (const std::exception &) {
    return std::nullopt;
  }
}

inline std::optional<int64_t>
ArithCmpIPredicate(const GenericOperation &operation) {
  std::string predicate =
      FindDictionaryValue(operation.properties, "predicate");
  if (predicate.empty())
    predicate = FindDictionaryValue(operation.attributes, "predicate");
  const size_t separator = predicate.find(':');
  if (separator != std::string::npos)
    predicate.resize(separator);
  try {
    size_t consumed = 0;
    const std::string value = trim(std::move(predicate));
    const int64_t result = std::stoll(value, &consumed);
    return consumed == value.size() && result >= 0 && result <= 9
               ? std::optional<int64_t>(result)
               : std::nullopt;
  } catch (const std::exception &) {
    return std::nullopt;
  }
}

inline __int128 SignedArithInteger(const ArithIntegerConstant &value) {
  const __int128 bits = value.bits;
  if (value.width == 64)
    return (value.bits & (uint64_t{1} << 63)) != 0
               ? bits - (static_cast<__int128>(1) << 64)
               : bits;
  return (value.bits & (uint64_t{1} << (value.width - 1))) != 0
             ? bits - (static_cast<__int128>(1) << value.width)
             : bits;
}

// Mirrors arith::applyCmpPredicate for scalar integer constants.
inline std::optional<bool> EvaluateArithCmpI(
    int64_t predicate, const ArithIntegerConstant &lhs,
    const ArithIntegerConstant &rhs) {
  if (lhs.width != rhs.width)
    return std::nullopt;
  switch (predicate) {
  case 0:
    return lhs.bits == rhs.bits;
  case 1:
    return lhs.bits != rhs.bits;
  case 2:
    return SignedArithInteger(lhs) < SignedArithInteger(rhs);
  case 3:
    return SignedArithInteger(lhs) <= SignedArithInteger(rhs);
  case 4:
    return SignedArithInteger(lhs) > SignedArithInteger(rhs);
  case 5:
    return SignedArithInteger(lhs) >= SignedArithInteger(rhs);
  case 6:
    return lhs.bits < rhs.bits;
  case 7:
    return lhs.bits <= rhs.bits;
  case 8:
    return lhs.bits > rhs.bits;
  case 9:
    return lhs.bits >= rhs.bits;
  default:
    return std::nullopt;
  }
}

inline std::optional<bool> FoldArithCmpI(
    const GenericOperation &operation,
    const std::map<int, ArithIntegerConstant> &constants) {
  if (operation.name != "arith.cmpi" || operation.operands.size() != 2 ||
      operation.results.size() != 1)
    return std::nullopt;
  const std::optional<int64_t> predicate = ArithCmpIPredicate(operation);
  if (!predicate)
    return std::nullopt;
  if (operation.operands.front() == operation.operands.back()) {
    switch (*predicate) {
    case 0:
    case 3:
    case 5:
    case 7:
    case 9:
      return true;
    default:
      return false;
    }
  }
  const auto lhs = constants.find(operation.operands.front());
  const auto rhs = constants.find(operation.operands.back());
  if (lhs == constants.end() || rhs == constants.end())
    return std::nullopt;
  return EvaluateArithCmpI(*predicate, lhs->second, rhs->second);
}

inline std::optional<int> FindArithConstantValue(
    const GenericModule &module, int blockId, const std::string &type,
    const std::string &value) {
  while (blockId >= 0) {
    const GenericBlock &block =
        module.blocks.at(static_cast<size_t>(blockId));
    for (int operationId : block.operations) {
      const GenericOperation &operation =
          module.operations.at(static_cast<size_t>(operationId));
      if (operation.name != "arith.constant" ||
          operation.results.size() != 1 || operation.resultTypes.size() != 1 ||
          operation.resultTypes.front() != type)
        continue;
      std::string literal =
          FindDictionaryValue(operation.properties, "value");
      if (literal.empty())
        literal = FindDictionaryValue(operation.attributes, "value");
      const size_t typeSeparator = literal.find(':');
      if (typeSeparator != std::string::npos)
        literal = trim(literal.substr(0, typeSeparator));
      if (literal == value)
        return operation.results.front();
    }

    const int parentOperation =
        module.regions.at(static_cast<size_t>(block.regionId)).parentOperation;
    blockId = parentOperation < 0
                  ? -1
                  : module.operations.at(static_cast<size_t>(parentOperation))
                        .blockId;
  }
  return std::nullopt;
}

inline void ReplaceOperationFolderValue(GenericModule &module, int from,
                                        int to) {
  for (GenericOperation &operation : module.operations) {
    for (int &operand : operation.operands)
      if (operand == from)
        operand = to;
    for (int &operand : operation.dpsInputs)
      if (operand == from)
        operand = to;
    for (int &operand : operation.dpsInits)
      if (operand == from)
        operand = to;
  }
}

// Mirrors the constant CSE/hoisting performed by OperationFolder while
// applyPatternsGreedily walks a function in postorder.
inline void RunGreedyOperationFolder(GenericModule &module, int functionId) {
  const GenericOperation &function =
      module.operations.at(static_cast<size_t>(functionId));
  if (function.regions.size() != 1)
    return;
  const GenericRegion &functionRegion =
      module.regions.at(static_cast<size_t>(function.regions.front()));
  if (functionRegion.blocks.empty())
    return;
  const int insertionBlock = functionRegion.blocks.front();

  std::vector<int> postOrder;
  std::function<void(int)> collect = [&](int operationId) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    for (int regionId : operation.regions)
      for (int blockId :
           module.regions.at(static_cast<size_t>(regionId)).blocks)
        for (int childId :
             module.blocks.at(static_cast<size_t>(blockId)).operations) {
          collect(childId);
          postOrder.push_back(childId);
        }
  };
  collect(functionId);

  GenericRewriter rewriter(module);
  std::set<int> folderOwnedConstants;
  std::map<std::tuple<std::string, std::string, std::string>, int>
      uniquedConstants;
  for (int operationId : postOrder) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    if (operation.name != "arith.constant" || operation.results.size() != 1 ||
        operation.resultTypes.size() != 1 || operation.blockId < 0)
      continue;
    std::string value = FindDictionaryValue(operation.properties, "value");
    if (value.empty())
      value = FindDictionaryValue(operation.attributes, "value");
    const auto key = std::make_tuple(operation.name, trim(std::move(value)),
                                     operation.resultTypes.front());
    auto existing = uniquedConstants.find(key);
    if (existing != uniquedConstants.end()) {
      ReplaceOperationFolderValue(
          module, operation.results.front(),
          module.operations.at(static_cast<size_t>(existing->second))
              .results.front());
      rewriter.removeFromBlock(operation.blockId, operationId);
      continue;
    }
    uniquedConstants.emplace(key, operationId);

    const int sourceBlock = operation.blockId;
    const GenericBlock &block =
        module.blocks.at(static_cast<size_t>(sourceBlock));
    const auto position =
        std::find(block.operations.begin(), block.operations.end(), operationId);
    const bool atFront = position == block.operations.begin();
    const bool followsFolderConstant =
        position != block.operations.begin() &&
        folderOwnedConstants.count(*std::prev(position)) != 0;
    if (sourceBlock != insertionBlock || (!atFront && !followsFolderConstant)) {
      rewriter.removeFromBlock(sourceBlock, operationId);
      rewriter.insertToBlock(insertionBlock, 0, operationId);
    }
    folderOwnedConstants.insert(operationId);
  }
}

} // namespace cvub

#endif
