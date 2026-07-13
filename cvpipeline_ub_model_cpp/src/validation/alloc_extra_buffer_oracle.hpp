#ifndef CVPIPELINE_UB_MODEL_CPP_ALLOC_EXTRA_BUFFER_ORACLE_HPP
#define CVPIPELINE_UB_MODEL_CPP_ALLOC_EXTRA_BUFFER_ORACLE_HPP

#include "bufferized_semantic_ir_oracle.hpp"
#include "../suffix/after_alloc_extra_buffer.hpp"

namespace cvub {

inline std::string AllocExtraBufferOracleFunctionName(const GenericOperation &function) {
  std::string name = FindDictionaryValue(function.properties, "sym_name");
  if (name.size() >= 2 && name.front() == '"' && name.back() == '"')
    name = name.substr(1, name.size() - 2);
  return name;
}

inline std::string AllocExtraBufferOperationKey(const GenericModule &module,
                                  const GenericOperation &operation) {
  const GenericOperation *function = EnclosingFunction(module, operation);
  if (!function)
    throw std::runtime_error("AllocExtraBuffer oracle: operation outside function");
  size_t occurrence = 0;
  for (const GenericOperation &candidate : module.operations) {
    if (candidate.id == operation.id)
      break;
    const GenericOperation *owner = EnclosingFunction(module, candidate);
    if (owner && owner->id == function->id && candidate.name == operation.name)
      ++occurrence;
  }
  return AllocExtraBufferOracleFunctionName(*function) + "\t" + operation.name + "\t" +
         std::to_string(occurrence);
}

inline std::set<std::string> CollectAllocExtraBufferKeys(
    const std::vector<std::pair<GenericModule, GenericModule>> &pairs) {
  std::set<std::string> result;
  for (const auto &[before, after] : pairs)
    for (const ExtraBufferAllocation &allocation :
         CollectAllocExtraBufferOracle(before, after))
      result.insert(AllocExtraBufferOperationKey(
          after, after.operations.at(static_cast<size_t>(allocation.ownerOperation))));
  return result;
}

inline std::vector<LocalBufferRecord> CollectAllocExtraBufferProjectionOracle(
    const GenericModule &beforeAllocExtraBuffer,
    const GenericModule &afterAllocExtraBuffer,
    const std::set<std::string> &extraBufferKeys) {
  const std::vector<BufferAllocation> before =
      CollectBufferAllocationOracle(beforeAllocExtraBuffer);
  const std::vector<BufferAllocation> after =
      CollectBufferAllocationOracle(afterAllocExtraBuffer);
  AfterBufferRootResolver roots(afterAllocExtraBuffer);
  const std::map<int, const GenericOperation *> definitions =
      DefiningOperations(afterAllocExtraBuffer);
  std::map<size_t, std::string> extraOwners;
  for (const GenericOperation &operation : afterAllocExtraBuffer.operations) {
    if (!HasAllocExtraBufferInterface(operation.name) ||
        operation.operands.empty() ||
        extraBufferKeys.count(
            AllocExtraBufferOperationKey(afterAllocExtraBuffer, operation)) == 0)
      continue;
    std::vector<size_t> segments = OperandSegmentSizes(operation.properties);
    if (segments.empty())
      segments = OperandSegmentSizes(operation.attributes);
    size_t tempOperand = operation.operands.size();
    if (segments.size() >= 3 && segments[2] != 0)
      tempOperand = segments[0] + segments[1];
    if (tempOperand >= operation.operands.size()) {
      const int candidate = operation.operands.back();
      auto definition = definitions.find(candidate);
      if (definition == definitions.end() ||
          definition->second->name != "memref.alloc" ||
          definition->second->id + 1 != operation.id)
        throw std::runtime_error("AllocExtraBuffer oracle: expected extra buffer is missing");
      tempOperand = operation.operands.size() - 1;
    }
    const std::string root = roots.root(operation.operands[tempOperand]);
    if (!startsWith(root, "local:"))
      throw std::runtime_error("AllocExtraBuffer oracle: temp buffer is not an allocation");
    extraOwners[static_cast<size_t>(std::stoull(root.substr(6)))] =
        operation.name;
  }

  std::vector<LocalBufferRecord> result;
  size_t baseOrdinal = 0;
  std::map<std::string, size_t> extraOrdinals;
  for (size_t ordinal = 0; ordinal < after.size(); ++ordinal) {
    const BufferAllocation &allocation = after[ordinal];
    const std::optional<MemRefTypeModel> type = ParseMemRefType(allocation.type);
    if (!type)
      throw std::runtime_error("AllocExtraBuffer oracle: malformed allocation type");
    const uint64_t bits = StaticBufferBits(allocation.type);
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
          "AllocExtraBuffer oracle: too many base allocations; detected extras=" +
          std::to_string(extraOwners.size()) + " before=" +
          std::to_string(before.size()) + " after=" +
          std::to_string(after.size()));
    const std::string identity = "base:" + std::to_string(baseOrdinal++);
    result.push_back({identity, identity, "",
                      allocation.type, type->addressSpace, bits, false, {}});
  }
  if (baseOrdinal != before.size())
    throw std::runtime_error("AllocExtraBuffer oracle: base allocation count mismatch " +
                             std::to_string(baseOrdinal) + " != " +
                             std::to_string(before.size()));
  return result;
}

inline std::string SerializeAllocExtraBufferProjection(
    const std::vector<LocalBufferRecord> &buffers) {
  std::ostringstream output;
  output << "ALLOC_EXTRA_BUFFER_PROJECTION\t1\n";
  for (const LocalBufferRecord &buffer : buffers)
    output << "BUFFER\t" << HexEncode(buffer.identity) << '\t'
           << HexEncode(buffer.extraBuffer ? buffer.ownerName : "") << '\t'
           << AddressSpaceName(buffer.addressSpace) << '\t'
           << buffer.constBits << '\t' << (buffer.extraBuffer ? 1 : 0) << '\n';
  return output.str();
}

} // namespace cvub

#endif
