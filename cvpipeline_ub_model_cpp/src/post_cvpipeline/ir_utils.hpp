#ifndef CVPIPELINE_UB_MODEL_CPP_POST_CVPIPELINE_IR_UTILS_HPP
#define CVPIPELINE_UB_MODEL_CPP_POST_CVPIPELINE_IR_UTILS_HPP

#include "../semantic_ir/generic_rewriter.hpp"

#include <regex>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace cvub {

inline void ReplaceAllUses(GenericModule &module, int oldValue, int newValue) {
  const std::map<int, int> replacement = {{oldValue, newValue}};
  for (GenericOperation &operation : module.operations) {
    RemapValues(operation.operands, replacement);
    RemapValues(operation.dpsInputs, replacement);
    RemapValues(operation.dpsInits, replacement);
  }
}

inline bool ModuleReferencesValue(const GenericModule &module, int value) {
  const auto contains = [value](const std::vector<int> &values) {
    return std::find(values.begin(), values.end(), value) != values.end();
  };
  for (const GenericOperation &operation : module.operations)
    if (contains(operation.operands) || contains(operation.dpsInputs) ||
        contains(operation.dpsInits))
      return true;
  return false;
}

inline void EraseOperationTree(GenericModule &module, int operationId) {
  if (operationId <= 0 ||
      static_cast<size_t>(operationId) >= module.operations.size())
    throw std::runtime_error("generic IR erase: invalid operation id " +
                             std::to_string(operationId));
  const GenericOperation &operation =
      module.operations.at(static_cast<size_t>(operationId));
  if (operation.blockId < 0 ||
      static_cast<size_t>(operation.blockId) >= module.blocks.size())
    throw std::runtime_error("generic IR erase: operation is not attached");
  GenericRewriter(module).removeFromBlock(operation.blockId, operationId);
}

inline void MoveOperationBefore(GenericModule &module, int operationId,
                                int beforeOperationId) {
  if (operationId <= 0 || beforeOperationId <= 0 ||
      static_cast<size_t>(operationId) >= module.operations.size() ||
      static_cast<size_t>(beforeOperationId) >= module.operations.size())
    throw std::runtime_error("generic IR move: invalid operation id");
  if (operationId == beforeOperationId)
    return;

  const int sourceBlock =
      module.operations.at(static_cast<size_t>(operationId)).blockId;
  const int destinationBlock =
      module.operations.at(static_cast<size_t>(beforeOperationId)).blockId;
  if (sourceBlock < 0 || destinationBlock < 0)
    throw std::runtime_error("generic IR move: operation is not attached");

  GenericRewriter rewriter(module);
  rewriter.removeFromBlock(sourceBlock, operationId);
  const GenericBlock &destination =
      module.blocks.at(static_cast<size_t>(destinationBlock));
  const auto before = std::find(destination.operations.begin(),
                                destination.operations.end(), beforeOperationId);
  if (before == destination.operations.end())
    throw std::runtime_error("generic IR move: destination is not attached");
  const size_t position =
      static_cast<size_t>(std::distance(destination.operations.begin(), before));
  rewriter.insertToBlock(destinationBlock, position, operationId);
}

template <typename = void>
inline std::string SetDictionaryValue(const std::string &dictionary,
                                      const std::string &name,
                                      const std::string &value) {
  std::vector<std::string> entries;
  bool replaced = false;
  if (dictionary.size() >= 2 && dictionary.front() == '{' &&
      dictionary.back() == '}') {
    entries = splitTopLevel(dictionary.substr(1, dictionary.size() - 2));
    for (std::string &entry : entries) {
      const size_t equal = entry.find('=');
      if (equal != std::string::npos && trim(entry.substr(0, equal)) == name) {
        entry = name + " = " + value;
        replaced = true;
      }
    }
  }
  if (!replaced)
    entries.push_back(name + " = " + value);
  std::string result = "{";
  for (size_t index = 0; index < entries.size(); ++index) {
    if (index != 0)
      result += ", ";
    result += trim(entries[index]);
  }
  return result + "}";
}

inline std::string IRDictionaryValue(const std::string &dictionary,
                                     const std::string &name) {
  if (dictionary.size() < 2 || dictionary.front() != '{' ||
      dictionary.back() != '}')
    return "";
  for (const std::string &entry :
       splitTopLevel(dictionary.substr(1, dictionary.size() - 2))) {
    const size_t equal = entry.find('=');
    if (equal != std::string::npos && trim(entry.substr(0, equal)) == name)
      return trim(entry.substr(equal + 1));
  }
  return "";
}

inline std::string UnquoteIRString(std::string value) {
  value = trim(std::move(value));
  if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
    return value.substr(1, value.size() - 2);
  return value;
}

inline std::string FunctionSymbolName(const GenericOperation &function) {
  std::string name = IRDictionaryValue(function.properties, "sym_name");
  if (name.empty())
    name = IRDictionaryValue(function.attributes, "sym_name");
  return UnquoteIRString(std::move(name));
}

inline std::vector<std::string>
FunctionSymbolNames(const GenericModule &module) {
  std::vector<std::string> names;
  for (const GenericOperation &operation : module.operations) {
    if (operation.name != "func.func")
      continue;
    const std::string name = FunctionSymbolName(operation);
    if (!name.empty())
      names.push_back(name);
  }
  return names;
}

inline std::vector<std::string>
DeviceFunctionNames(const GenericModule &module) {
  std::vector<std::string> names;
  for (const GenericOperation &operation : module.operations) {
    if (operation.name != "func.func" || operation.regions.empty())
      continue;
    const std::string kind =
        IRDictionaryValue(operation.attributes, "hacc.function_kind");
    if (kind.find("DEVICE") == std::string::npos)
      continue;
    const std::string name = FunctionSymbolName(operation);
    if (!name.empty())
      names.push_back(name);
  }
  return names;
}

inline std::map<int, std::string>
GenericValueTypes(const GenericModule &module) {
  std::map<int, std::string> types;
  for (const GenericBlock &block : module.blocks) {
    if (block.arguments.size() != block.argumentTypes.size())
      throw std::runtime_error("generic IR validation: block argument/type mismatch");
    for (size_t index = 0; index < block.arguments.size(); ++index)
      if (!types.emplace(block.arguments[index], block.argumentTypes[index]).second)
        throw std::runtime_error("generic IR validation: duplicate value id " +
                                 std::to_string(block.arguments[index]));
  }
  for (const GenericOperation &operation : module.operations) {
    if (operation.results.size() != operation.resultTypes.size())
      throw std::runtime_error("generic IR validation: result/type mismatch at " +
                               std::to_string(operation.id));
    for (size_t index = 0; index < operation.results.size(); ++index)
      if (!types.emplace(operation.results[index], operation.resultTypes[index])
               .second)
        throw std::runtime_error("generic IR validation: duplicate value id " +
                                 std::to_string(operation.results[index]));
  }
  return types;
}

inline void ValidateGenericModule(const GenericModule &module) {
  if (module.operations.empty())
    throw std::runtime_error("generic IR validation: missing module root");
  const std::map<int, std::string> valueTypes = GenericValueTypes(module);

  for (size_t index = 0; index < module.operations.size(); ++index)
    if (module.operations[index].id != static_cast<int>(index))
      throw std::runtime_error("generic IR validation: non-unique operation ids");
  for (size_t index = 0; index < module.regions.size(); ++index)
    if (module.regions[index].id != static_cast<int>(index))
      throw std::runtime_error("generic IR validation: non-unique region ids");
  for (size_t index = 0; index < module.blocks.size(); ++index)
    if (module.blocks[index].id != static_cast<int>(index))
      throw std::runtime_error("generic IR validation: non-unique block ids");

  for (const GenericRegion &region : module.regions) {
    if (region.parentOperation < 0 ||
        static_cast<size_t>(region.parentOperation) >= module.operations.size())
      throw std::runtime_error("generic IR validation: invalid region parent");
    const GenericOperation &parent =
        module.operations.at(static_cast<size_t>(region.parentOperation));
    if (region.ordinal < 0 ||
        static_cast<size_t>(region.ordinal) >= parent.regions.size() ||
        parent.regions[static_cast<size_t>(region.ordinal)] != region.id)
      throw std::runtime_error("generic IR validation: invalid region ordinal");
    for (size_t ordinal = 0; ordinal < region.blocks.size(); ++ordinal) {
      const int blockId = region.blocks[ordinal];
      if (blockId < 0 || static_cast<size_t>(blockId) >= module.blocks.size())
        throw std::runtime_error("generic IR validation: invalid block link");
      const GenericBlock &block =
          module.blocks.at(static_cast<size_t>(blockId));
      if (block.regionId != region.id ||
          block.ordinal != static_cast<int>(ordinal))
        throw std::runtime_error("generic IR validation: invalid block parent");
    }
  }

  for (const GenericBlock &block : module.blocks) {
    if (block.regionId < 0 ||
        static_cast<size_t>(block.regionId) >= module.regions.size())
      throw std::runtime_error("generic IR validation: invalid block region");
    for (size_t ordinal = 0; ordinal < block.operations.size(); ++ordinal) {
      const int operationId = block.operations[ordinal];
      if (operationId < 0 ||
          static_cast<size_t>(operationId) >= module.operations.size())
        throw std::runtime_error("generic IR validation: invalid operation link");
      const GenericOperation &operation =
          module.operations.at(static_cast<size_t>(operationId));
      const GenericRegion &region =
          module.regions.at(static_cast<size_t>(block.regionId));
      if (operation.blockId != block.id || operation.regionId != block.regionId ||
          operation.parentId != region.parentOperation ||
          operation.ordinal != static_cast<int>(ordinal))
        throw std::runtime_error("generic IR validation: invalid operation parent");
    }
  }

  for (const GenericOperation &operation : module.operations) {
    if (operation.id == 0) {
      if (operation.parentId != -1 || operation.regionId != -1 ||
          operation.blockId != -1)
        throw std::runtime_error("generic IR validation: invalid module root");
    } else if (operation.blockId < 0 ||
               static_cast<size_t>(operation.blockId) >= module.blocks.size()) {
      throw std::runtime_error("generic IR validation: detached operation");
    }
    for (size_t ordinal = 0; ordinal < operation.regions.size(); ++ordinal) {
      const int regionId = operation.regions[ordinal];
      if (regionId < 0 || static_cast<size_t>(regionId) >= module.regions.size())
        throw std::runtime_error("generic IR validation: invalid region link");
      const GenericRegion &region =
          module.regions.at(static_cast<size_t>(regionId));
      if (region.parentOperation != operation.id ||
          region.ordinal != static_cast<int>(ordinal))
        throw std::runtime_error("generic IR validation: invalid region parent");
    }
    for (int operand : operation.operands)
      if (valueTypes.count(operand) == 0)
        throw std::runtime_error("generic IR validation: undefined operand " +
                                 std::to_string(operand));
    for (int successor : operation.successors) {
      if (successor < 0 ||
          static_cast<size_t>(successor) >= module.blocks.size() ||
          module.blocks.at(static_cast<size_t>(successor)).regionId !=
              operation.regionId)
        throw std::runtime_error("generic IR validation: invalid successor");
    }

    const auto validateDps = [&](const std::vector<int> &values) {
      for (int value : values) {
        const auto operand =
            std::find(operation.operands.begin(), operation.operands.end(), value);
        if (operand == operation.operands.end())
          throw std::runtime_error("generic IR validation: DPS value is not an operand");
        const size_t operandIndex = static_cast<size_t>(
            std::distance(operation.operands.begin(), operand));
        const auto type = valueTypes.find(value);
        if (type == valueTypes.end())
          throw std::runtime_error("generic IR validation: undefined DPS value");
        if (operandIndex >= operation.operandTypes.size() ||
            operation.operandTypes[operandIndex] != type->second)
          throw std::runtime_error("generic IR validation: DPS value type mismatch");
      }
    };
    validateDps(operation.dpsInputs);
    validateDps(operation.dpsInits);
  }
}

inline std::set<std::string>
ReferencedSymbols(const GenericModule &module, int operationId) {
  std::set<std::string> result;
  static const std::regex reference(R"(@([A-Za-z_.$][A-Za-z0-9_.$-]*))");
  std::function<void(int)> visit = [&](int id) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(id));
    for (const std::string *dictionary :
         {&operation.properties, &operation.attributes})
      for (std::sregex_iterator found(dictionary->begin(), dictionary->end(),
                                      reference),
           end;
           found != end; ++found)
        result.insert((*found)[1].str());
    for (int regionId : operation.regions)
      for (int blockId :
           module.regions.at(static_cast<size_t>(regionId)).blocks)
        for (int child :
             module.blocks.at(static_cast<size_t>(blockId)).operations)
          visit(child);
  };
  visit(operationId);
  return result;
}

inline GenericModule ExtractFunctionModule(const GenericModule &module,
                                           const std::string &functionName) {
  ValidateGenericModule(module);
  std::map<std::string, int> functions;
  for (const GenericOperation &operation : module.operations)
    if (operation.name == "func.func") {
      const std::string name = FunctionSymbolName(operation);
      if (!name.empty() && !functions.emplace(name, operation.id).second)
        throw std::runtime_error("generic IR extraction: duplicate function symbol " +
                                 name);
    }
  const auto selected = functions.find(functionName);
  if (selected == functions.end())
    throw std::runtime_error("generic IR extraction: unknown function " +
                             functionName);

  std::set<int> keep = {selected->second};
  std::vector<int> pending = {selected->second};
  while (!pending.empty()) {
    const int operationId = pending.back();
    pending.pop_back();
    for (const std::string &symbol : ReferencedSymbols(module, operationId)) {
      const auto referenced = functions.find(symbol);
      if (referenced != functions.end() && keep.insert(referenced->second).second)
        pending.push_back(referenced->second);
    }
  }

  GenericModule isolated = module;
  if (isolated.operations.front().regions.size() != 1)
    throw std::runtime_error("generic IR extraction: expected one module region");
  const int rootRegion = isolated.operations.front().regions.front();
  GenericRegion &region =
      isolated.regions.at(static_cast<size_t>(rootRegion));
  if (region.blocks.size() != 1)
    throw std::runtime_error("generic IR extraction: expected one module block");
  GenericBlock &block = isolated.blocks.at(static_cast<size_t>(region.blocks.front()));
  block.operations.erase(
      std::remove_if(block.operations.begin(), block.operations.end(),
                     [&](int operationId) { return keep.count(operationId) == 0; }),
      block.operations.end());
  for (size_t ordinal = 0; ordinal < block.operations.size(); ++ordinal)
    isolated.operations.at(static_cast<size_t>(block.operations[ordinal])).ordinal =
        static_cast<int>(ordinal);
  isolated = CompactGenericModule(std::move(isolated));
  ValidateGenericModule(isolated);
  return isolated;
}

inline bool IsPostCVPipelineTensorType(const std::string &type) {
  return startsWith(trim(type), "tensor<");
}

inline void ValidateBeforeCVPipelineBoundary(const GenericModule &module) {
  ValidateGenericModule(module);
  bool foundDeviceFunction = false;
  bool foundTensorLevel = false;
  for (const GenericOperation &function : module.operations) {
    if (function.name != "func.func" || function.regions.empty())
      continue;
    const std::string kind =
        IRDictionaryValue(function.attributes, "hacc.function_kind");
    if (kind.find("DEVICE") == std::string::npos)
      continue;
    const std::string core =
        IRDictionaryValue(function.attributes, "hivm.func_core_type");
    if (core.find("AIV") == std::string::npos &&
        core.find("MIX") == std::string::npos)
      continue;
    foundDeviceFunction = true;
    std::function<void(int)> visit = [&](int operationId) {
      const GenericOperation &operation =
          module.operations.at(static_cast<size_t>(operationId));
      foundTensorLevel = foundTensorLevel ||
          std::any_of(operation.resultTypes.begin(), operation.resultTypes.end(),
                      IsPostCVPipelineTensorType) ||
          std::any_of(operation.operandTypes.begin(), operation.operandTypes.end(),
                      IsPostCVPipelineTensorType) ||
          operation.properties.find("tensor<") != std::string::npos;
      for (int regionId : operation.regions)
        for (int blockId :
             module.regions.at(static_cast<size_t>(regionId)).blocks) {
          const GenericBlock &block =
              module.blocks.at(static_cast<size_t>(blockId));
          foundTensorLevel = foundTensorLevel ||
              std::any_of(block.argumentTypes.begin(), block.argumentTypes.end(),
                          IsPostCVPipelineTensorType);
          for (int child : block.operations)
            visit(child);
        }
    };
    visit(function.id);
  }
  if (!foundDeviceFunction)
    throw std::runtime_error(
        "generic IR boundary: expected non-host MIX or AIV device function with core type");
  if (!foundTensorLevel)
    throw std::runtime_error(
        "generic IR boundary: expected tensor-level before-CVPipeline device function");
}

} // namespace cvub

#endif
