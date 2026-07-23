#ifndef CVPIPELINE_UB_MODEL_CPP_PIPELINE_METADATA_CACHE_HPP
#define CVPIPELINE_UB_MODEL_CPP_PIPELINE_METADATA_CACHE_HPP

#include "hivm_dimension_analyzer.hpp"
#include "../ir/generic_analysis.hpp"

#include <tuple>

namespace cvub {

// Content-keyed parsing cache shared within one pipeline stage. It owns every
// returned value, so callers can safely retain references while new entries
// are inserted. No global state or third-party interning is required.
class PipelineMetadataCache {
public:
  const std::optional<MemRefTypeModel> &
  memRefType(const std::string &type) {
    auto found = memRefTypes.find(type);
    if (found != memRefTypes.end())
      return found->second;
    return memRefTypes.emplace(type, ParseMemRefType(type)).first->second;
  }

  const std::optional<ShapedTypeModel> &
  shapedType(const std::string &type) {
    auto found = shapedTypes.find(type);
    if (found != shapedTypes.end())
      return found->second;
    return shapedTypes
        .emplace(type, ParseShapedTypeForDimensionAnalysis(type))
        .first->second;
  }

  const std::string &dictionaryValue(const std::string &dictionary,
                                     const std::string &name) {
    auto dictionaryIt = dictionaries.find(dictionary);
    if (dictionaryIt == dictionaries.end())
      dictionaryIt =
          dictionaries.emplace(dictionary, parseDictionary(dictionary)).first;
    auto value = dictionaryIt->second.find(name);
    return value == dictionaryIt->second.end() ? empty : value->second;
  }

  GenericOperationKind operationKind(const std::string &name) {
    auto found = operationKinds.find(name);
    if (found != operationKinds.end())
      return found->second;
    return operationKinds.emplace(name, ClassifyGenericOperation(name))
        .first->second;
  }

  const std::vector<size_t> &
  dpsInitOperandIndices(const std::string &name, size_t operandCount,
                        const std::string &properties) {
    const auto key = std::make_tuple(name, operandCount, properties);
    auto found = dpsInitOperands.find(key);
    if (found != dpsInitOperands.end())
      return found->second;
    return dpsInitOperands
        .emplace(key,
                 DpsInitOperandIndices(name, operandCount, properties))
        .first->second;
  }

private:
  static std::map<std::string, std::string>
  parseDictionary(const std::string &dictionary) {
    std::map<std::string, std::string> result;
    if (dictionary.size() < 2)
      return result;
    for (const std::string &entry :
         splitTopLevel(dictionary.substr(1, dictionary.size() - 2))) {
      const size_t equal = entry.find('=');
      if (equal != std::string::npos)
        result.emplace(trim(entry.substr(0, equal)),
                       trim(entry.substr(equal + 1)));
    }
    return result;
  }

  std::map<std::string, std::optional<MemRefTypeModel>> memRefTypes;
  std::map<std::string, std::optional<ShapedTypeModel>> shapedTypes;
  std::map<std::string, std::map<std::string, std::string>> dictionaries;
  std::map<std::string, GenericOperationKind> operationKinds;
  std::map<std::tuple<std::string, size_t, std::string>,
           std::vector<size_t>>
      dpsInitOperands;
  std::string empty;
};

} // namespace cvub

#endif
