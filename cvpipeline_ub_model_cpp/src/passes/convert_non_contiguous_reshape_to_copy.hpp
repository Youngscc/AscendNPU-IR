#ifndef CVPIPELINE_UB_MODEL_CPP_CONVERT_NON_CONTIGUOUS_RESHAPE_TO_COPY_HPP
#define CVPIPELINE_UB_MODEL_CPP_CONVERT_NON_CONTIGUOUS_RESHAPE_TO_COPY_HPP

#include "hivm_decompose_op.hpp"

namespace cvub {

struct NonContiguousReshapeCopy {
  int collapseOperation = -1;
  std::string type;
};

inline const GenericOperation *EnclosingFunctionForNonContiguousReshape(
    const GenericModule &module, const GenericOperation &operation) {
  const GenericOperation *current = &operation;
  while (current && current->name != "func.func") {
    if (current->parentId < 0)
      return nullptr;
    current = &module.operations.at(static_cast<size_t>(current->parentId));
  }
  return current;
}

inline bool HasDictionaryEntry(const std::string &dictionary,
                               const std::string &name) {
  if (dictionary.size() < 2)
    return false;
  for (const std::string &entry :
       splitTopLevel(dictionary.substr(1, dictionary.size() - 2))) {
    const size_t equal = entry.find('=');
    if (trim(entry.substr(0, equal)) == name)
      return true;
  }
  return false;
}

inline std::vector<NonContiguousReshapeCopy>
ModelConvertNonContiguousReshapeToCopy(const GenericModule &module) {
  const std::map<int, const GenericOperation *> definitions =
      DefiningOperations(module);
  std::vector<NonContiguousReshapeCopy> result;
  for (const GenericOperation &mark : module.operations) {
    if (mark.name != "annotation.mark" || mark.operands.empty() ||
        !HasDictionaryEntry(mark.attributes, "maybeUnCollapsibleReshape"))
      continue;
    auto definition = definitions.find(mark.operands.front());
    if (definition == definitions.end() ||
        definition->second->name != "memref.collapse_shape")
      throw std::runtime_error(
          "ConvertNonContiguousReshapeToCopy: marker is not on collapse_shape");
    const GenericOperation &collapse = *definition->second;
    if (collapse.operandTypes.empty())
      throw std::runtime_error(
          "ConvertNonContiguousReshapeToCopy: collapse source is missing");
    const GenericOperation *function =
        EnclosingFunctionForNonContiguousReshape(module, collapse);
    if (!function)
      continue;
    const std::string coreType = DecomposeEnumValue(
        FindDictionaryValue(function->attributes, "hivm.func_core_type"));
    if (coreType == "AIC")
      continue;
    const std::string &sourceType = collapse.operandTypes.front();
    result.push_back(
        {collapse.id,
         MemRefWithElementType(sourceType, ShapedElementType(sourceType))});
  }
  return result;
}

inline std::string SerializeNonContiguousReshapeCopies(
    const std::vector<NonContiguousReshapeCopy> &copies) {
  std::ostringstream output;
  output << "NON_CONTIGUOUS_RESHAPE_TO_COPY\t1\n";
  for (const NonContiguousReshapeCopy &copy : copies)
    output << "COPY\t" << HexEncode(copy.type) << '\n';
  return output.str();
}

} // namespace cvub

#endif
