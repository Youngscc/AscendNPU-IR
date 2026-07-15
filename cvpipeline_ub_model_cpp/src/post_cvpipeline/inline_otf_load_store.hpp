#ifndef CVPIPELINE_UB_MODEL_CPP_POST_CVPIPELINE_INLINE_OTF_LOAD_STORE_HPP
#define CVPIPELINE_UB_MODEL_CPP_POST_CVPIPELINE_INLINE_OTF_LOAD_STORE_HPP

#include "ir_utils.hpp"
#include "types.hpp"

#include <cstdint>
#include <regex>
#include <set>
#include <string>
#include <vector>

namespace cvub {
namespace inline_otf_detail {

// Real source: bishengir/lib/Dialect/HIVM/Transforms/HIVMInlineOTFLoadStore.cpp
// pattern UnalignedLastDimConcatStorePattern, plus
// bishengir/include/bishengir/Dialect/Utils/Util.h (INTR_BYTES_PER_BLOCK = 32)
// and bishengir/include/bishengir/Dialect/HIVM/Utils/Utils.h
// (kBufferSizeInByteAttr = "buffer_size_in_byte").
inline constexpr uint64_t kIntrBytesPerBlock = 32;
inline constexpr const char *kBufferSizeInByteAttr = "buffer_size_in_byte";

inline const GenericOperation *Definition(const GenericModule &module,
                                          int value) {
  for (const GenericOperation &operation : module.operations)
    if (std::find(operation.results.begin(), operation.results.end(), value) !=
        operation.results.end())
      return &operation;
  return nullptr;
}

inline bool IsTensorType(const std::string &type) {
  return startsWith(trim(type), "tensor<");
}

inline bool IsStaticShapedTensor(const std::string &type) {
  return IsTensorType(type) && type.find('?') == std::string::npos;
}

// Mirrors CanonicalizationStaticShape: the first comma-separated token inside
// the outermost <...> (dropping trailing memory-space attributes).
inline std::string ShapedInner(const std::string &type) {
  const size_t open = type.find('<');
  const size_t close = type.rfind('>');
  if (open == std::string::npos || close == std::string::npos || close <= open)
    return "";
  const std::string inner = type.substr(open + 1, close - open - 1);
  const std::vector<std::string> tokens = splitTopLevel(inner);
  return tokens.empty() ? inner : tokens.front();
}

inline std::vector<int64_t> StaticShape(const std::string &type) {
  const std::string inner = ShapedInner(type);
  if (inner.empty())
    return {};
  static const std::regex element(R"((?:bf16|[fiu][0-9]+)$)");
  const std::string dimensions = std::regex_replace(inner, element, "");
  if (dimensions.empty())
    return {};
  std::vector<int64_t> shape;
  try {
    for (std::string dimension : split(dimensions, 'x')) {
      dimension = trim(dimension);
      if (dimension.empty())
        continue;
      if (dimension == "?")
        shape.push_back(-1);
      else
        shape.push_back(std::stoll(dimension));
    }
  } catch (const std::exception &) {
    return {};
  }
  return shape;
}

// Mirrors HIVMInlineOTFLoadStore.cpp:53 elemSizeInBytes =
// tensorTypeOut.getElementTypeBitWidth() / 8 (integer division, e.g. f16 -> 2).
inline int64_t ElementBytes(const std::string &type) {
  const std::string inner = ShapedInner(type);
  if (inner.empty())
    return 0;
  const std::vector<std::string> parts = split(inner, 'x');
  const std::string element = trim(parts.back());
  std::string digits;
  for (const char character : element)
    if (std::isdigit(static_cast<unsigned char>(character)) != 0)
      digits += character;
  if (digits.empty())
    return 0;
  try {
    return std::stoll(digits) / 8;
  } catch (const std::exception &) {
    return 0;
  }
}

inline int64_t ParseConcatDim(const GenericOperation &concat) {
  std::string value = IRDictionaryValue(concat.properties, "dim");
  if (value.empty())
    value = IRDictionaryValue(concat.attributes, "dim");
  std::string digits;
  for (const char character : value) {
    if (std::isdigit(static_cast<unsigned char>(character)) != 0)
      digits += character;
    else if (!digits.empty())
      break;
  }
  if (digits.empty())
    return -1;
  try {
    return std::stoll(digits);
  } catch (const std::exception &) {
    return -1;
  }
}

inline std::string FormatArray(const std::vector<int64_t> &values) {
  std::ostringstream output;
  output << "[";
  for (size_t index = 0; index < values.size(); ++index) {
    if (index != 0)
      output << ", ";
    output << values[index];
  }
  output << "]";
  return output.str();
}

inline std::string OperandTypeOf(const GenericOperation &operation, int value) {
  for (size_t index = 0; index < operation.operands.size() &&
                              index < operation.operandTypes.size();
       ++index)
    if (operation.operands[index] == value)
      return operation.operandTypes[index];
  return "";
}

inline bool IsAnnotationWithAttr(const GenericOperation &operation,
                                 const std::string &name) {
  return operation.name == "annotation.mark" &&
         !IRDictionaryValue(operation.attributes, name).empty();
}

// Destination operand index of a vconcat (the $dst, last operand), mirroring
// DpsInitOperandIndices for hivm.hir.vconcat == {operandCount - 1}.  Computed
// from operands so it is correct whether or not DPS metadata is populated.
inline size_t VConcatDestinationIndex(const GenericOperation &concat) {
  return concat.operands.size() - 1;
}

inline int64_t InputExtent(const std::vector<int64_t> &shape, int64_t dim) {
  if (dim < 0 || static_cast<int64_t>(shape.size()) <= dim)
    return -1;
  return shape[static_cast<size_t>(dim)];
}

// Mirrors isLastDimConcatAligned: true iff every cumulative input extent along
// the concat dimension, scaled by the element size, is divisible by the
// 32-byte block.  A dynamic (negative) extent means the alignment cannot be
// decided statically and the caller treats the store as unrewriteable.
inline bool IsLastDimConcatAligned(
    int64_t concatDim, int64_t elementBytes,
    const std::vector<std::vector<int64_t>> &inputShapes) {
  if (elementBytes <= 0)
    return false;
  int64_t accumulated = 0;
  for (const std::vector<int64_t> &shape : inputShapes) {
    const int64_t extent = InputExtent(shape, concatDim);
    if (extent < 0)
      return false;
    accumulated += extent;
    if (static_cast<uint64_t>(accumulated) *
            static_cast<uint64_t>(elementBytes) %
        kIntrBytesPerBlock !=
        0U)
      return false;
  }
  return true;
}

inline bool ValueIsUser(const GenericOperation &operation, int value) {
  const auto contains = [&](const std::vector<int> &values) {
    return std::find(values.begin(), values.end(), value) != values.end();
  };
  return contains(operation.operands) || contains(operation.dpsInputs) ||
         contains(operation.dpsInits);
}

inline bool HasExtraUsers(const GenericModule &module, int concatResult,
                          int storeId) {
  for (const GenericOperation &operation : module.operations) {
    if (operation.id == storeId)
      continue;
    if (!ValueIsUser(operation, concatResult))
      continue;
    if (IsAnnotationWithAttr(operation, kBufferSizeInByteAttr))
      continue; // erased by the rewrite.
    return true;
  }
  return false;
}

inline std::vector<int> BufferSizeMarksOn(const GenericModule &module,
                                          int concatResult) {
  std::vector<int> marks;
  for (const GenericOperation &operation : module.operations)
    if (operation.name == "annotation.mark" && !operation.operands.empty() &&
        operation.operands.front() == concatResult &&
        IsAnnotationWithAttr(operation, kBufferSizeInByteAttr))
      marks.push_back(operation.id);
  return marks;
}

inline void AddDiagnostic(StageResult &result, int operationId,
                          const std::string &operation,
                          const std::string &reason) {
  result.precision = Precision::Incomplete;
  result.diagnostics.push_back(
      {"InlineOTFLoadStore", "", operationId, operation, reason});
}

enum class ConcatStoreKind {
  // Store source is not a last-dim pure-tensor vconcat: the real pattern returns
  // failure and leaves the store unchanged (recognized no-op for this store).
  kNoMatch,
  // Every cumulative input extent in bytes is divisible by the 32-byte block:
  // the real pattern returns failure and leaves the IR unchanged.
  kAlignedNoOp,
  // Static, unaligned: the real pattern rebuilds the store source as an
  // insert_slice accumulator chain that the model reproduces exactly.
  kUnalignedStatic,
  // The pattern would fire but a dynamic extent prevents an exact rebuild here.
  kUnrewriteable,
};

struct ConcatStoreCandidate {
  int storeId;
  int concatId;
  int storeSourceValue;
  ConcatStoreKind kind;
};

// Rebuild one store's source as an insert_slice accumulator chain seeded from
// the (reused) vconcat destination, repoint the store, erase the
// buffer_size_in_byte annotation, and drop the now-dead vconcat.  Mirrors
// UnalignedLastDimConcatStorePattern for fully-static extents.
inline void RewriteUnalignedConcatStore(GenericModule &module,
                                        const ConcatStoreCandidate &candidate) {
  const GenericOperation storeCopy =
      module.operations.at(static_cast<size_t>(candidate.storeId));
  const GenericOperation concat =
      module.operations.at(static_cast<size_t>(candidate.concatId));
  const int concatResult = concat.results.front();
  const size_t dstIndex = VConcatDestinationIndex(concat);
  const int seedValue = concat.operands[dstIndex];
  const std::string outType = OperandTypeOf(concat, seedValue);
  const std::vector<int64_t> outShape = StaticShape(outType);
  const int64_t rank = static_cast<int64_t>(outShape.size());
  const int64_t dim = ParseConcatDim(concat);

  std::vector<std::pair<int, std::string>> inputs; // (value, type)
  for (size_t index = 0; index < concat.operands.size(); ++index) {
    if (index == dstIndex)
      continue;
    inputs.emplace_back(concat.operands[index],
                        OperandTypeOf(concat, concat.operands[index]));
  }

  const int storeBlock = storeCopy.blockId;
  const GenericBlock &block = module.blocks.at(static_cast<size_t>(storeBlock));
  size_t position = static_cast<size_t>(std::distance(
      block.operations.begin(),
      std::find(block.operations.begin(), block.operations.end(),
                candidate.storeId)));

  GenericRewriter rewriter(module);
  int accumulator = seedValue;
  int64_t runningOffset = 0;
  for (size_t index = 0; index < inputs.size(); ++index) {
    const int inputValue = inputs[index].first;
    const std::vector<int64_t> inShape = StaticShape(inputs[index].second);
    std::vector<int64_t> offsets(static_cast<size_t>(rank), 0);
    offsets[static_cast<size_t>(dim)] = runningOffset;
    const std::vector<int64_t> sizes = inShape;
    const std::vector<int64_t> strides(static_cast<size_t>(rank), 1);
    const std::string attributes =
        "{static_offsets = " + FormatArray(offsets) +
        ", static_sizes = " + FormatArray(sizes) +
        ", static_strides = " + FormatArray(strides) + "}";
    const int insertSlice = rewriter.createOperation(
        storeCopy.parentId, storeCopy.regionId, storeBlock,
        "tensor.insert_slice", {outType}, {inputValue, accumulator},
        {inputs[index].second, outType}, "", attributes);
    ApplyOperationSemantics(module.operations.at(static_cast<size_t>(insertSlice)));
    rewriter.insertToBlock(storeBlock, position, insertSlice);
    ++position;
    accumulator = module.operations.at(static_cast<size_t>(insertSlice))
                      .results.front();
    if (index + 1 < inputs.size())
      runningOffset += inShape[static_cast<size_t>(dim)];
  }
  const int finalAccumulator = accumulator;

  GenericOperation &mutableStore =
      module.operations.at(static_cast<size_t>(candidate.storeId));
  const std::string sourceType = OperandTypeOf(mutableStore, concatResult);
  for (int &operand : mutableStore.operands)
    if (operand == concatResult)
      operand = finalAccumulator;
  for (size_t index = 0; index < mutableStore.operands.size() &&
                              index < mutableStore.operandTypes.size();
       ++index)
    if (mutableStore.operands[index] == finalAccumulator)
      mutableStore.operandTypes[index] = sourceType;
  for (int &value : mutableStore.dpsInputs)
    if (value == concatResult)
      value = finalAccumulator;
  for (int &value : mutableStore.dpsInits)
    if (value == concatResult)
      value = finalAccumulator;

  for (int markId : BufferSizeMarksOn(module, concatResult))
    GenericRewriter(module).removeFromBlock(
        module.operations.at(static_cast<size_t>(markId)).blockId, markId);

  GenericRewriter(module).removeFromBlock(
      module.operations.at(static_cast<size_t>(candidate.concatId)).blockId,
      candidate.concatId);
}

} // namespace inline_otf_detail

// Models createHIVMInlineOTFLoadStorePass on a projected AIV module.
//
// Match a hivm.hir.store whose source is a pure-tensor hivm.hir.vconcat along
// the last dimension (HIVMInlineOTFLoadStore.cpp:UnalignedLastDimConcatStorePattern).
//   * aligned static extents -> the pattern returns failure, IR unchanged (Exact)
//   * unaligned static extents -> rebuild the store source as an insert_slice
//     accumulator seeded from the vconcat destination, drop the
//     buffer_size_in_byte mark and the dead vconcat (Exact); this is what
//     prevents a dead concat destination from being counted as a UB allocation.
//   * dynamic extents (the pattern would fire with dynamic arith) -> fail closed
//     to Incomplete, IR untouched.
// Stores whose source is not a last-dim pure-tensor vconcat are left unchanged
// (recognized no-op).  Never gated by enableCodeMotion (the pass always runs).
inline StageResult RunInlineOTFLoadStore(GenericModule module) {
  using namespace inline_otf_detail;
  StageResult result;
  result.module = std::move(module);
  ValidateGenericModule(result.module);

  std::vector<ConcatStoreCandidate> candidates;
  for (const GenericOperation &operation : result.module.operations) {
    if (operation.name != "hivm.hir.store" || operation.blockId < 0)
      continue;
    if (operation.operands.empty())
      continue;
    // hivm.hir.store schema is store(source, dest); the pattern matches the
    // first DPS input == operand 0.
    const int sourceValue = operation.operands[0];
    const GenericOperation *concat = Definition(result.module, sourceValue);
    if (concat == nullptr || concat->name != "hivm.hir.vconcat")
      continue; // No match: recognized no-op for this store.
    if (concat->operands.empty())
      continue;

    bool pureTensor = true;
    for (const std::string &type : concat->operandTypes)
      if (!IsTensorType(type)) {
        pureTensor = false;
        break;
      }
    if (pureTensor)
      for (const std::string &type : concat->resultTypes)
        if (!IsTensorType(type)) {
          pureTensor = false;
          break;
        }
    if (!pureTensor)
      continue; // hasPureTensorSemantics fails: recognized no-op.

    const size_t dstIndex = VConcatDestinationIndex(*concat);
    const int dstValue = concat->operands[dstIndex];
    const std::string outType = OperandTypeOf(*concat, dstValue);
    const std::vector<int64_t> outShape = StaticShape(outType);
    if (outShape.empty()) {
      AddDiagnostic(result, concat->id, "hivm.hir.vconcat",
                    "vconcat destination shape is not statically known");
      continue;
    }
    const int64_t rank = static_cast<int64_t>(outShape.size());
    const int64_t dim = ParseConcatDim(*concat);
    if (dim < 0) {
      AddDiagnostic(result, concat->id, "hivm.hir.vconcat",
                    "vconcat dim attribute is not parseable");
      continue;
    }
    if (dim != rank - 1)
      continue; // Not last dim: recognized no-op.

    const bool lastDynamic = outShape.back() < 0;
    std::vector<std::vector<int64_t>> inputShapes;
    bool staticInputs = true;
    for (size_t index = 0; index < concat->operands.size(); ++index) {
      if (index == dstIndex)
        continue;
      const std::string inType =
          OperandTypeOf(*concat, concat->operands[index]);
      if (!IsStaticShapedTensor(inType)) {
        staticInputs = false;
        break;
      }
      inputShapes.push_back(StaticShape(inType));
    }

    ConcatStoreCandidate candidate{operation.id, concat->id, sourceValue,
                                   ConcatStoreKind::kNoMatch};
    if (lastDynamic || !staticInputs) {
      candidate.kind = ConcatStoreKind::kUnrewriteable;
    } else {
      const int64_t elementBytes = ElementBytes(outType);
      if (elementBytes <= 0) {
        AddDiagnostic(result, concat->id, "hivm.hir.vconcat",
                      "vconcat element bit width is not parseable");
        continue;
      }
      candidate.kind = IsLastDimConcatAligned(dim, elementBytes, inputShapes)
                           ? ConcatStoreKind::kAlignedNoOp
                           : ConcatStoreKind::kUnalignedStatic;
    }
    candidates.push_back(candidate);
  }

  for (const ConcatStoreCandidate &candidate : candidates) {
    if (candidate.kind != ConcatStoreKind::kUnrewriteable)
      continue;
    const GenericOperation &concat =
        result.module.operations.at(static_cast<size_t>(candidate.concatId));
    AddDiagnostic(result, candidate.concatId, concat.name,
                  "unaligned last-dim concat/store rewrite has dynamic extents "
                  "not modeled");
  }

  if (result.precision == Precision::Incomplete) {
    ValidateGenericModule(result.module);
    return result; // Fail closed: legal IR untouched.
  }

  for (const ConcatStoreCandidate &candidate : candidates) {
    if (candidate.kind != ConcatStoreKind::kUnalignedStatic)
      continue;
    if (HasExtraUsers(result.module, candidate.storeSourceValue,
                      candidate.storeId)) {
      AddDiagnostic(result, candidate.concatId, "hivm.hir.vconcat",
                    "vconcat result has users beyond the store and size mark");
      continue;
    }
    RewriteUnalignedConcatStore(result.module, candidate);
  }

  if (!candidates.empty())
    result.module = CompactGenericModule(std::move(result.module));
  ValidateGenericModule(result.module);
  return result;
}

} // namespace cvub

#endif
