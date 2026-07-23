#ifndef CVPIPELINE_UB_MODEL_CPP_HIVM_INLINE_OTF_LOAD_STORE_HPP
#define CVPIPELINE_UB_MODEL_CPP_HIVM_INLINE_OTF_LOAD_STORE_HPP

#include "canonicalization_hivm_pipeline.hpp"
#include "one_shot_bufferize.hpp"

namespace cvub {

inline std::optional<MemRefTypeModel>
ParseHIVMInlineShapedType(const std::string &type) {
  if (startsWith(type, "tensor<"))
    return ParseMemRefType(ConvertTensorToMemRefType(type));
  return ParseMemRefType(type);
}

inline std::optional<uint64_t>
HIVMInlineElementBytes(const std::string &elementType) {
  if (elementType == "i1" || elementType == "i8")
    return 1;
  if (elementType == "bf16" || elementType == "f16" ||
      elementType == "i16")
    return 2;
  if (elementType == "f32" || elementType == "i32")
    return 4;
  if (elementType == "f64" || elementType == "i64")
    return 8;
  if (elementType.size() > 1 &&
      (elementType.front() == 'i' || elementType.front() == 'f')) {
    try {
      const uint64_t bits = std::stoull(elementType.substr(1));
      return bits / 8;
    } catch (const std::exception &) {
    }
  }
  return std::nullopt;
}

inline bool HIVMInlineUsesValue(const GenericOperation &operation, int value) {
  const auto contains = [&](const std::vector<int> &values) {
    return std::find(values.begin(), values.end(), value) != values.end();
  };
  return contains(operation.operands) || contains(operation.dpsInputs) ||
         contains(operation.dpsInits);
}

inline bool HIVMInlineIsBufferSizeMark(const GenericOperation &operation) {
  return operation.name == "annotation.mark" &&
         (operation.attributes.find("buffer_size_in_byte") !=
              std::string::npos ||
          operation.properties.find("buffer_size_in_byte") !=
              std::string::npos);
}

inline int64_t HIVMInlineConcatDim(const GenericOperation &operation) {
  std::string value = FindDictionaryValue(operation.properties, "dim");
  if (value.empty())
    value = FindDictionaryValue(operation.attributes, "dim");
  const size_t typed = value.find(" : ");
  if (typed != std::string::npos)
    value = value.substr(0, typed);
  return std::stoll(trim(value));
}

inline std::string HIVMInlineStaticArray(
    const std::vector<int64_t> &values) {
  std::ostringstream output;
  output << "array<i64:";
  for (size_t index = 0; index < values.size(); ++index)
    output << (index == 0 ? " " : ", ") << values[index];
  output << '>';
  return output.str();
}

inline std::string HIVMInlineInsertSliceProperties(
    const std::vector<int64_t> &offsets,
    const std::vector<int64_t> &sizes) {
  std::vector<int64_t> strides(sizes.size(), 1);
  return "{operandSegmentSizes = array<i32: 1, 1, 0, 0, 0>, "
         "static_offsets = " +
         HIVMInlineStaticArray(offsets) + ", static_sizes = " +
         HIVMInlineStaticArray(sizes) + ", static_strides = " +
         HIVMInlineStaticArray(strides) + "}";
}

// Mirrors UnalignedLastDimConcatStorePattern.
inline GenericModule RunHIVMInlineOTFLoadStore(GenericModule module) {
  GenericModule original = module;
  ApplyOperationSemanticsToAll(module.operations);
  bool rewritten = false;
  const auto definitions = DefiningOperations(module);
  for (const GenericOperation &storeSnapshot : module.operations) {
    if (storeSnapshot.name != "hivm.hir.store" ||
        storeSnapshot.dpsInputs.empty())
      continue;
    const auto concatDefinition =
        definitions.find(storeSnapshot.dpsInputs.front());
    if (concatDefinition == definitions.end() ||
        concatDefinition->second->name != "hivm.hir.vconcat")
      continue;
    const GenericOperation concat = *concatDefinition->second;
    if (concat.dpsInputs.empty() || concat.dpsInits.size() != 1 ||
        std::any_of(concat.operandTypes.begin(), concat.operandTypes.end(),
                    [](const std::string &type) {
                      return GenericIsMemRefType(type);
                    }))
      continue;
    const MemRefTypeModel output = ParseHIVMInlineShapedType(
        concat.resultTypes.front()).value_or(MemRefTypeModel{});
    const int64_t dim = HIVMInlineConcatDim(concat);
    if (output.shape.empty() || dim != static_cast<int64_t>(output.shape.size()) - 1)
      continue;
    const std::optional<uint64_t> elementBytes =
        HIVMInlineElementBytes(output.elementType);
    if (!elementBytes)
      throw std::runtime_error(
          "HIVMInlineOTFLoadStore: unsupported vconcat element type");

    bool aligned = output.shape.back().has_value();
    uint64_t accumulated = 0;
    std::vector<MemRefTypeModel> inputTypes;
    for (size_t index = 0; index < concat.dpsInputs.size(); ++index) {
      const auto input =
          ParseHIVMInlineShapedType(concat.operandTypes.at(index));
      if (!input || dim >= static_cast<int64_t>(input->shape.size()) ||
          !input->shape[static_cast<size_t>(dim)]) {
        aligned = false;
        inputTypes.push_back(input.value_or(MemRefTypeModel{}));
        continue;
      }
      inputTypes.push_back(*input);
      accumulated += static_cast<uint64_t>(
          *input->shape[static_cast<size_t>(dim)]);
      aligned &= (accumulated * *elementBytes) % 32 == 0;
    }
    if (aligned)
      continue;
    for (const GenericOperation &user : module.operations) {
      if (user.id == storeSnapshot.id ||
          !HIVMInlineUsesValue(user, concat.results.front()) ||
          HIVMInlineIsBufferSizeMark(user))
        continue;
      throw std::runtime_error(
          "HIVMInlineOTFLoadStore: vconcat result has users beyond the store "
          "and size mark");
    }
    if (std::any_of(inputTypes.begin(), inputTypes.end(),
                    [](const MemRefTypeModel &type) {
                      return type.shape.empty() ||
                             std::any_of(type.shape.begin(), type.shape.end(),
                                         [](const auto &extent) {
                                           return !extent.has_value();
                                         });
                    }))
      throw std::runtime_error(
          "HIVMInlineOTFLoadStore: dynamic unaligned vconcat is unsupported");

    GenericRewriter rewriter(module);
    int accumulator = concat.dpsInits.front();
    int64_t offset = 0;
    const GenericBlock &block = module.blocks.at(
        static_cast<size_t>(storeSnapshot.blockId));
    auto position = std::find(block.operations.begin(), block.operations.end(),
                              storeSnapshot.id);
    size_t insertion = static_cast<size_t>(position - block.operations.begin());
    for (size_t index = 0; index < concat.dpsInputs.size(); ++index) {
      std::vector<int64_t> offsets(inputTypes[index].shape.size(), 0);
      offsets[static_cast<size_t>(dim)] = offset;
      std::vector<int64_t> sizes;
      for (const auto &extent : inputTypes[index].shape)
        sizes.push_back(*extent);
      const int insert = rewriter.createOperation(
          storeSnapshot.parentId, storeSnapshot.regionId,
          storeSnapshot.blockId, "tensor.insert_slice",
          {concat.resultTypes.front()},
          {concat.dpsInputs[index], accumulator},
          {concat.operandTypes[index], concat.resultTypes.front()},
          HIVMInlineInsertSliceProperties(offsets, sizes));
      ApplyOperationSemantics(
          module.operations.at(static_cast<size_t>(insert)));
      rewriter.insertToBlock(storeSnapshot.blockId, insertion++, insert);
      accumulator =
          module.operations.at(static_cast<size_t>(insert)).results.front();
      offset += sizes[static_cast<size_t>(dim)];
    }
    GenericOperation &store =
        module.operations.at(static_cast<size_t>(storeSnapshot.id));
    for (int &operand : store.operands)
      if (operand == concat.results.front())
        operand = accumulator;
    for (int &operand : store.dpsInputs)
      if (operand == concat.results.front())
        operand = accumulator;
    rewritten = true;
  }
  if (!rewritten)
    return original;
  PipelineAnalysisContext useLists(module, kGenericAnalysisUsers);
  while (EliminateCanonicalizationDeadCode(module, useLists)) {
  }
  return CompactGenericModule(std::move(module));
}

} // namespace cvub

#endif
