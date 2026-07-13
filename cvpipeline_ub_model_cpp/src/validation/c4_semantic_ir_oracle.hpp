#ifndef CVPIPELINE_UB_MODEL_CPP_C4_SEMANTIC_IR_ORACLE_HPP
#define CVPIPELINE_UB_MODEL_CPP_C4_SEMANTIC_IR_ORACLE_HPP

#include "bufferized_semantic_ir_oracle.hpp"
#include "../suffix/c4_semantic_ir.hpp"

namespace cvub {

inline std::string C4FunctionName(const C1OperationRecord &function) {
  std::string name = FindDictionaryValue(function.properties, "sym_name");
  if (name.size() >= 2 && name.front() == '"' && name.back() == '"')
    name = name.substr(1, name.size() - 2);
  return name;
}

inline std::string C4OperationKey(const C1SemanticModule &module,
                                  const C1OperationRecord &operation) {
  const C1OperationRecord *function = C4EnclosingFunction(module, operation);
  if (!function)
    throw std::runtime_error("C4 oracle: operation outside function");
  size_t occurrence = 0;
  for (const C1OperationRecord &candidate : module.operations) {
    if (candidate.id == operation.id)
      break;
    const C1OperationRecord *owner = C4EnclosingFunction(module, candidate);
    if (owner && owner->id == function->id && candidate.name == operation.name)
      ++occurrence;
  }
  return C4FunctionName(*function) + "\t" + operation.name + "\t" +
         std::to_string(occurrence);
}

inline std::set<std::string> CollectC4ExtraBufferKeys(
    const std::vector<std::pair<C1SemanticModule, C1SemanticModule>> &pairs) {
  std::set<std::string> result;
  for (const auto &[before, after] : pairs)
    for (const ExtraBufferAllocation &allocation :
         CollectAllocExtraBufferOracle(before, after))
      result.insert(C4OperationKey(
          after, after.operations.at(static_cast<size_t>(allocation.ownerOperation))));
  return result;
}

inline std::vector<C4BufferRecord> CollectC4BufferOracle(
    const C1SemanticModule &c3Endpoint, const C1SemanticModule &c4Endpoint,
    const std::set<std::string> &extraBufferKeys) {
  const std::vector<BufferAllocation> before =
      CollectBufferAllocationOracle(c3Endpoint);
  const std::vector<BufferAllocation> after =
      CollectBufferAllocationOracle(c4Endpoint);
  AfterBufferRootResolver roots(c4Endpoint);
  const std::map<int, const C1OperationRecord *> definitions =
      C1DefiningOperations(c4Endpoint);
  std::map<size_t, std::string> extraOwners;
  for (const C1OperationRecord &operation : c4Endpoint.operations) {
    if (!HasAllocExtraBufferInterface(operation.name) ||
        operation.operands.empty() ||
        extraBufferKeys.count(C4OperationKey(c4Endpoint, operation)) == 0)
      continue;
    std::vector<size_t> segments = C1OperandSegmentSizes(operation.properties);
    if (segments.empty())
      segments = C1OperandSegmentSizes(operation.attributes);
    size_t tempOperand = operation.operands.size();
    if (segments.size() >= 3 && segments[2] != 0)
      tempOperand = segments[0] + segments[1];
    if (tempOperand >= operation.operands.size()) {
      const int candidate = operation.operands.back();
      auto definition = definitions.find(candidate);
      if (definition == definitions.end() ||
          definition->second->name != "memref.alloc" ||
          definition->second->id + 1 != operation.id)
        throw std::runtime_error("C4 oracle: expected extra buffer is missing");
      tempOperand = operation.operands.size() - 1;
    }
    const std::string root = roots.root(operation.operands[tempOperand]);
    if (!startsWith(root, "local:"))
      throw std::runtime_error("C4 oracle: temp buffer is not an allocation");
    extraOwners[static_cast<size_t>(std::stoull(root.substr(6)))] =
        operation.name;
  }

  std::vector<C4BufferRecord> result;
  size_t baseOrdinal = 0;
  std::map<std::string, size_t> extraOrdinals;
  for (size_t ordinal = 0; ordinal < after.size(); ++ordinal) {
    const BufferAllocation &allocation = after[ordinal];
    const std::optional<MemRefTypeModel> type = ParseMemRefType(allocation.type);
    if (!type)
      throw std::runtime_error("C4 oracle: malformed allocation type");
    const uint64_t bits = C4StaticBufferBits(allocation.type);
    auto extra = extraOwners.find(ordinal);
    if (extra != extraOwners.end()) {
      const size_t ownerOrdinal = extraOrdinals[extra->second]++;
      const std::string identity = "extra:" + extra->second + ":" +
                                   std::to_string(ownerOrdinal);
      result.push_back({identity, identity, extra->second, allocation.type,
                        type->addressSpace,
                        bits, true, {}});
      continue;
    }
    if (baseOrdinal >= before.size())
      throw std::runtime_error(
          "C4 oracle: too many base allocations; detected extras=" +
          std::to_string(extraOwners.size()) + " before=" +
          std::to_string(before.size()) + " after=" +
          std::to_string(after.size()));
    const std::string identity = "base:" + std::to_string(baseOrdinal++);
    result.push_back({identity, identity, "",
                      allocation.type, type->addressSpace, bits, false, {}});
  }
  if (baseOrdinal != before.size())
    throw std::runtime_error("C4 oracle: base allocation count mismatch " +
                             std::to_string(baseOrdinal) + " != " +
                             std::to_string(before.size()));
  return result;
}

inline std::string SerializeC4BufferProjection(
    const std::vector<C4BufferRecord> &buffers) {
  std::ostringstream output;
  output << "C4_BUFFER_PROJECTION\t1\n";
  for (const C4BufferRecord &buffer : buffers)
    output << "BUFFER\t" << HexEncode(buffer.identity) << '\t'
           << HexEncode(buffer.extraBuffer ? buffer.ownerName : "") << '\t'
           << C4AddressSpaceName(buffer.addressSpace) << '\t'
           << buffer.constBits << '\t' << (buffer.extraBuffer ? 1 : 0) << '\n';
  return output.str();
}

} // namespace cvub

#endif
