#ifndef CVPIPELINE_UB_MODEL_CPP_PLANMEMORY_INPUT_SEMANTIC_IR_ORACLE_HPP
#define CVPIPELINE_UB_MODEL_CPP_PLANMEMORY_INPUT_SEMANTIC_IR_ORACLE_HPP

#include "c6_semantic_ir_oracle.hpp"
#include "../suffix/planmemory_input_semantic_ir.hpp"

namespace cvub {

inline std::string CollectPlanMemoryInputBufferOracle(
    const C1SemanticModule &module) {
  AfterBufferRootResolver roots(module);
  std::map<size_t, std::pair<uint32_t, bool>> marks;
  for (const C1OperationRecord &operation : module.operations) {
    if (operation.name != "annotation.mark" || operation.operands.empty())
      continue;
    std::string count =
        FindDictionaryValue(operation.attributes, "hivm.multi_buffer");
    if (count.empty())
      count = FindDictionaryValue(operation.properties, "hivm.multi_buffer");
    if (count.empty())
      continue;
    const std::string root = roots.root(operation.operands.front());
    if (!startsWith(root, "local:"))
      continue;
    std::string preload = FindDictionaryValue(
        operation.attributes, "hivm.preload_local_buffer");
    if (preload.empty())
      preload = FindDictionaryValue(operation.properties,
                                    "hivm.preload_local_buffer");
    marks[static_cast<size_t>(std::stoull(root.substr(6)))] =
        {C6OracleInteger(count),
         !preload.empty() && C6OracleInteger(preload) != 0};
  }

  std::ostringstream output;
  output << "PLANMEMORY_INPUT_BUFFER_ORACLE\t1\n";
  for (const C1OperationRecord &operation : module.operations) {
    if (operation.name != "memref.alloc" || operation.results.empty() ||
        operation.resultTypes.empty())
      continue;
    const std::string root = roots.root(operation.results.front());
    if (!startsWith(root, "local:"))
      throw std::runtime_error("C7 oracle: allocation has no local root");
    const size_t ordinal = static_cast<size_t>(std::stoull(root.substr(6)));
    const std::optional<MemRefTypeModel> type =
        ParseMemRefType(operation.resultTypes.front());
    const C1OperationRecord *function = C1EnclosingFunction(module, operation);
    if (!type || !function || type->addressSpace != AddressSpace::UB ||
        !C7IsAIVFunction(*function))
      continue;
    auto mark = marks.find(ordinal);
    output << "BUFFER\t" << ordinal << '\t'
           << HexEncode(C1FunctionName(*function)) << '\t'
           << HexEncode(C7PhysicalTypeSignature(operation.resultTypes.front()))
           << '\t' << GetBufferConstBits(operation.resultTypes.front()) << '\t'
           << (mark == marks.end() ? 1U : mark->second.first) << '\t'
           << (mark != marks.end() && mark->second.second ? 1 : 0) << '\n';
  }
  return output.str();
}

inline std::string CollectPlanMemoryInputAccessOracle(
    const C1SemanticModule &module) {
  AfterBufferRootResolver roots(module);
  std::set<size_t> targetOrdinals;
  for (const C1OperationRecord &operation : module.operations) {
    if (operation.name != "memref.alloc" || operation.results.empty() ||
        operation.resultTypes.empty())
      continue;
    const std::optional<MemRefTypeModel> type =
        ParseMemRefType(operation.resultTypes.front());
    const C1OperationRecord *function = C1EnclosingFunction(module, operation);
    if (!type || !function || type->addressSpace != AddressSpace::UB ||
        !C7IsAIVFunction(*function))
      continue;
    const std::string root = roots.root(operation.results.front());
    if (startsWith(root, "local:"))
      targetOrdinals.insert(static_cast<size_t>(std::stoull(root.substr(6))));
  }

  std::ostringstream output;
  output << "PLANMEMORY_INPUT_ACCESS_ORACLE\t1\n";
  std::map<std::string, size_t> functionOrdinals;
  for (const C1OperationRecord &operation : module.operations) {
    const C1OperationRecord *function = C1EnclosingFunction(module, operation);
    if (!function || !C7IsAIVFunction(*function) ||
        C7IsElidedOperation(operation.name))
      continue;
    std::vector<std::pair<std::string, std::string>> accesses;
    for (size_t operand = 0; operand < operation.operands.size(); ++operand) {
      const std::string root = roots.root(operation.operands[operand]);
      if (!startsWith(root, "local:"))
        continue;
      const size_t ordinal = static_cast<size_t>(std::stoull(root.substr(6)));
      if (targetOrdinals.count(ordinal) == 0)
        continue;
      accesses.push_back(
          {root, C7AccessEffect(operation.name, operand,
                                operation.operands.size(),
                                operation.properties)});
    }
    if (accesses.empty())
      continue;
    const std::string functionName = C1FunctionName(*function);
    output << "EVENT\t" << HexEncode(functionName) << '\t'
           << functionOrdinals[functionName]++ << '\t'
           << HexEncode(operation.name);
    for (const auto &[root, effect] : accesses)
      output << '\t' << HexEncode(root) << ':' << effect;
    output << '\n';
  }
  return output.str();
}

inline std::string CollectPlanMemoryOperationSequenceOracle(
    const C1SemanticModule &module) {
  std::ostringstream output;
  output << "PLANMEMORY_OPERATION_SEQUENCE\t1\n";
  auto emitName = [&](const std::string &name) {
    output << "OP\t" << HexEncode(name) << '\n';
  };
  std::function<void(const C1BlockRecord &)> emitBlock;
  std::function<void(const C1OperationRecord &)> emitOperation;
  emitOperation = [&](const C1OperationRecord &operation) {
    emitName(operation.name);
    if (operation.name != "scf.for" && operation.name != "scf.if" &&
        operation.name != "scf.while" && operation.name != "scope.scope")
      return;
    for (size_t index = 0; index < operation.regions.size(); ++index) {
      const C1RegionRecord &region = module.regions.at(
          static_cast<size_t>(operation.regions[index]));
      for (int block : region.blocks)
        emitBlock(module.blocks.at(static_cast<size_t>(block)));
      if (operation.name == "scf.if" && index + 1 < operation.regions.size())
        emitName("scf.if.else");
    }
    emitName(operation.name == "scope.scope" ? "scope.scope.end"
                                               : operation.name + ".end");
  };
  emitBlock = [&](const C1BlockRecord &block) {
    for (int operation : block.operations)
      emitOperation(module.operations.at(static_cast<size_t>(operation)));
  };
  for (const C1OperationRecord &operation : module.operations) {
    if (operation.name != "func.func" || !C7IsAIVFunction(operation))
      continue;
    for (int regionId : operation.regions) {
      const C1RegionRecord &region =
          module.regions.at(static_cast<size_t>(regionId));
      for (int block : region.blocks)
        emitBlock(module.blocks.at(static_cast<size_t>(block)));
    }
  }
  return output.str();
}

} // namespace cvub

#endif
