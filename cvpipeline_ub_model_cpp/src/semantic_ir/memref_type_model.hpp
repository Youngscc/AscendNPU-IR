#ifndef CVPIPELINE_UB_MODEL_CPP_MEMREF_TYPE_MODEL_HPP
#define CVPIPELINE_UB_MODEL_CPP_MEMREF_TYPE_MODEL_HPP

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace cvub {

enum class AddressSpace { Unknown, Zero, GM, L1, L0A, L0B, L0C, UB };

struct MemRefTypeModel {
  std::vector<std::optional<int64_t>> shape;
  std::vector<std::optional<int64_t>> strides;
  std::string elementType;
  AddressSpace addressSpace = AddressSpace::Unknown;
  int64_t offset = 0;
  bool hasStridedLayout = false;
};

namespace detail {

inline std::string TrimTypeText(std::string value) {
  value.erase(value.begin(),
              std::find_if(value.begin(), value.end(), [](char c) {
                return !std::isspace(static_cast<unsigned char>(c));
              }));
  value.erase(std::find_if(value.rbegin(), value.rend(), [](char c) {
                return !std::isspace(static_cast<unsigned char>(c));
              }).base(),
              value.end());
  return value;
}

inline std::vector<std::string> SplitTypeTextAtTopLevel(
    const std::string &value, char delimiter) {
  std::vector<std::string> pieces;
  size_t begin = 0;
  int angleDepth = 0;
  int squareDepth = 0;
  int parenDepth = 0;
  for (size_t i = 0; i < value.size(); ++i) {
    switch (value[i]) {
    case '<':
      ++angleDepth;
      break;
    case '>':
      --angleDepth;
      break;
    case '[':
      ++squareDepth;
      break;
    case ']':
      --squareDepth;
      break;
    case '(':
      ++parenDepth;
      break;
    case ')':
      --parenDepth;
      break;
    default:
      break;
    }
    if (value[i] == delimiter && angleDepth == 0 && squareDepth == 0 &&
        parenDepth == 0) {
      pieces.push_back(TrimTypeText(value.substr(begin, i - begin)));
      begin = i + 1;
    }
  }
  pieces.push_back(TrimTypeText(value.substr(begin)));
  return pieces;
}

inline std::optional<int64_t> ParseStaticDimension(const std::string &text) {
  std::string value = TrimTypeText(text);
  if (value == "?")
    return std::nullopt;
  if (value.empty())
    throw std::runtime_error("empty memref dimension");
  size_t digit = value.front() == '-' ? 1U : 0U;
  if (digit == value.size() ||
      !std::all_of(value.begin() + static_cast<std::ptrdiff_t>(digit),
                   value.end(), [](char c) {
                     return std::isdigit(static_cast<unsigned char>(c));
                   }))
    throw std::runtime_error("invalid memref dimension: " + value);
  return std::stoll(value);
}

inline int64_t CheckedShapeProduct(int64_t lhs, int64_t rhs) {
  if (lhs < 0 || rhs < 0)
    throw std::runtime_error("negative static memref dimension");
  if (lhs != 0 && rhs > std::numeric_limits<int64_t>::max() / lhs)
    throw std::overflow_error("collapsed memref shape overflow");
  return lhs * rhs;
}

inline int64_t CheckedLcm(int64_t lhs, int64_t rhs) {
  if (lhs <= 0 || rhs <= 0)
    throw std::runtime_error("stride alignment must be positive");
  const int64_t divisor = std::gcd(lhs, rhs);
  const int64_t reduced = lhs / divisor;
  if (reduced > std::numeric_limits<int64_t>::max() / rhs)
    throw std::overflow_error("stride alignment lcm overflow");
  return reduced * rhs;
}

inline int64_t AlignStride(int64_t stride, int64_t alignment) {
  if (stride < 0 || alignment <= 0)
    throw std::runtime_error("invalid static stride alignment");
  if (stride > std::numeric_limits<int64_t>::max() - (alignment - 1))
    throw std::overflow_error("stride alignment overflow");
  return ((stride + alignment - 1) / alignment) * alignment;
}

} // namespace detail

inline std::optional<MemRefTypeModel>
ParseMemRefType(const std::string &text) {
  const size_t memref = text.find("memref<");
  if (memref == std::string::npos)
    return std::nullopt;
  const size_t bodyBegin = memref + 7;
  int depth = 1;
  size_t bodyEnd = bodyBegin;
  for (; bodyEnd < text.size() && depth > 0; ++bodyEnd) {
    if (text[bodyEnd] == '<')
      ++depth;
    else if (text[bodyEnd] == '>')
      --depth;
  }
  if (depth != 0 || bodyEnd <= bodyBegin)
    throw std::runtime_error("unterminated memref type: " + text);

  const std::string body =
      text.substr(bodyBegin, bodyEnd - bodyBegin - 1);
  std::vector<std::string> fields =
      detail::SplitTypeTextAtTopLevel(body, ',');
  if (fields.empty())
    throw std::runtime_error("empty memref type: " + text);
  std::vector<std::string> shapeAndElement =
      detail::SplitTypeTextAtTopLevel(fields.front(), 'x');
  if (shapeAndElement.empty())
    throw std::runtime_error("missing memref element type: " + text);

  MemRefTypeModel type;
  type.elementType = shapeAndElement.back();
  shapeAndElement.pop_back();
  for (const std::string &dimension : shapeAndElement)
    type.shape.push_back(detail::ParseStaticDimension(dimension));

  for (size_t i = 1; i < fields.size(); ++i) {
    const std::string &field = fields[i];
    if (field.find("#hivm.address_space<zero>") != std::string::npos)
      type.addressSpace = AddressSpace::Zero;
    else if (field.find("#hivm.address_space<gm>") != std::string::npos)
      type.addressSpace = AddressSpace::GM;
    else if (field.find("#hivm.address_space<cbuf>") != std::string::npos)
      type.addressSpace = AddressSpace::L1;
    else if (field.find("#hivm.address_space<ca>") != std::string::npos)
      type.addressSpace = AddressSpace::L0A;
    else if (field.find("#hivm.address_space<cb>") != std::string::npos)
      type.addressSpace = AddressSpace::L0B;
    else if (field.find("#hivm.address_space<cc>") != std::string::npos)
      type.addressSpace = AddressSpace::L0C;
    else if (field.find("#hivm.address_space<ub>") != std::string::npos)
      type.addressSpace = AddressSpace::UB;

    const size_t strided = field.find("strided<[");
    if (strided == std::string::npos)
      continue;
    type.hasStridedLayout = true;
    const size_t stridesBegin = field.find('[', strided);
    const size_t stridesEnd = field.find(']', stridesBegin);
    if (stridesBegin == std::string::npos || stridesEnd == std::string::npos)
      throw std::runtime_error("invalid strided memref layout: " + text);
    std::vector<std::string> strideTokens = detail::SplitTypeTextAtTopLevel(
        field.substr(stridesBegin + 1, stridesEnd - stridesBegin - 1), ',');
    for (const std::string &stride : strideTokens)
      type.strides.push_back(detail::ParseStaticDimension(stride));
    const size_t offsetKey = field.find("offset:", stridesEnd);
    if (offsetKey != std::string::npos) {
      size_t offsetEnd = field.find('>', offsetKey);
      std::string offsetText = field.substr(offsetKey + 7,
                                            offsetEnd - (offsetKey + 7));
      std::optional<int64_t> offset =
          detail::ParseStaticDimension(offsetText);
      type.offset = offset.value_or(std::numeric_limits<int64_t>::min());
    }
  }

  if (!type.hasStridedLayout) {
    type.strides.resize(type.shape.size());
    std::optional<int64_t> stride = 1;
    for (size_t i = type.shape.size(); i > 0; --i) {
      type.strides[i - 1] = stride;
      if (!stride || !type.shape[i - 1]) {
        stride = std::nullopt;
      } else {
        stride = detail::CheckedShapeProduct(*stride, *type.shape[i - 1]);
      }
    }
  }
  if (type.strides.size() != type.shape.size())
    throw std::runtime_error("memref shape/stride rank mismatch: " + text);
  return type;
}

inline uint64_t GetElementBitWidth(const MemRefTypeModel &type) {
  const std::string &element = type.elementType;
  if (element == "bf16" || element == "f16")
    return 16;
  if (element == "f32")
    return 32;
  if (element == "f64")
    return 64;
  if (element.size() > 1 && element.front() == 'i' &&
      std::all_of(element.begin() + 1, element.end(), [](char c) {
        return std::isdigit(static_cast<unsigned char>(c));
      }))
    return static_cast<uint64_t>(std::stoull(element.substr(1)));
  return 0;
}

inline bool HasIntegerElementType(const MemRefTypeModel &type) {
  return type.elementType.size() > 1 && type.elementType.front() == 'i' &&
         std::all_of(type.elementType.begin() + 1, type.elementType.end(),
                     [](char c) {
                       return std::isdigit(static_cast<unsigned char>(c));
                     });
}

inline bool HasFloatingElementType(const MemRefTypeModel &type) {
  return type.elementType == "bf16" || type.elementType == "f16" ||
         type.elementType == "f32" || type.elementType == "f64";
}

inline bool IsIdentityStrides(const MemRefTypeModel &type) {
  std::optional<int64_t> expected = 1;
  for (size_t i = type.shape.size(); i > 0; --i) {
    if (type.strides[i - 1] != expected)
      return false;
    if (!expected || !type.shape[i - 1])
      expected = std::nullopt;
    else
      expected = detail::CheckedShapeProduct(*expected, *type.shape[i - 1]);
  }
  return true;
}

inline MemRefTypeModel ApplyStrideAlignment(
    MemRefTypeModel type,
    const std::vector<std::pair<int64_t, int64_t>> &alignments) {
  std::vector<int64_t> strideAlignment(type.shape.size(), 1);
  const uint64_t bitWidth = GetElementBitWidth(type);
  if (bitWidth == 0)
    throw std::runtime_error("unsupported stride-aligned element type");
  for (const auto &alignment : alignments) {
    if (alignment.first < 0 ||
        static_cast<size_t>(alignment.first) >= strideAlignment.size() ||
        alignment.second <= 0)
      throw std::runtime_error("invalid stride alignment attribute");
    if (static_cast<uint64_t>(alignment.second) >
        std::numeric_limits<uint64_t>::max() / 8U)
      throw std::overflow_error("byte stride alignment overflow");
    const uint64_t alignmentBits =
        static_cast<uint64_t>(alignment.second) * 8U;
    if (alignmentBits % bitWidth != 0)
      throw std::runtime_error("byte alignment is not element-aligned");
    const uint64_t elementAlignment = alignmentBits / bitWidth;
    if (elementAlignment >
        static_cast<uint64_t>(std::numeric_limits<int64_t>::max()))
      throw std::overflow_error("stride element alignment overflow");
    int64_t &current =
        strideAlignment[static_cast<size_t>(alignment.first)];
    current = detail::CheckedLcm(
        current, static_cast<int64_t>(elementAlignment));
  }
  for (size_t i = strideAlignment.size(); i > 1; --i)
    strideAlignment[i - 2] = detail::CheckedLcm(strideAlignment[i - 2],
                                                strideAlignment[i - 1]);
  bool changed = false;
  for (size_t i = 0; i < type.strides.size(); ++i) {
    if (!type.strides[i]) {
      if (strideAlignment[i] != 1)
        changed = true;
      continue;
    }
    const int64_t aligned =
        detail::AlignStride(*type.strides[i], strideAlignment[i]);
    changed = changed || aligned != *type.strides[i];
    type.strides[i] = aligned;
  }
  if (changed)
    type.hasStridedLayout = true;
  return type;
}

inline MemRefTypeModel CollapseMemRefType(
    const MemRefTypeModel &type,
    const std::vector<std::vector<size_t>> &reassociation) {
  MemRefTypeModel collapsed = type;
  collapsed.shape.clear();
  collapsed.strides.clear();
  for (const std::vector<size_t> &group : reassociation) {
    if (group.empty())
      continue;
    std::optional<int64_t> dimension = 1;
    for (size_t axis : group) {
      if (axis >= type.shape.size())
        throw std::runtime_error("flatten reassociation axis out of range");
      if (!dimension || !type.shape[axis])
        dimension = std::nullopt;
      else
        dimension = detail::CheckedShapeProduct(*dimension, *type.shape[axis]);
    }
    collapsed.shape.push_back(dimension);
    size_t stridePosition = group.size() - 1;
    while (stridePosition > 0 && type.shape[group[stridePosition]] &&
           *type.shape[group[stridePosition]] == 1)
      --stridePosition;
    const size_t strideAxis = group[stridePosition];
    bool allUnitPreceding = true;
    for (size_t i = 0; i < stridePosition; ++i)
      allUnitPreceding = allUnitPreceding && type.shape[group[i]] &&
                         *type.shape[group[i]] == 1;
    if (!type.shape[strideAxis] && stridePosition > 0 &&
        !allUnitPreceding)
      collapsed.strides.push_back(std::nullopt);
    else
      collapsed.strides.push_back(type.strides[strideAxis]);
  }
  if (!type.hasStridedLayout) {
    std::optional<int64_t> stride = 1;
    for (size_t i = collapsed.shape.size(); i > 0; --i) {
      collapsed.strides[i - 1] = stride;
      if (!stride || !collapsed.shape[i - 1])
        stride = std::nullopt;
      else
        stride = detail::CheckedShapeProduct(*stride,
                                             *collapsed.shape[i - 1]);
    }
  }
  return collapsed;
}

inline std::vector<bool>
GetContiguousAxes(const std::vector<MemRefTypeModel> &types) {
  if (types.empty())
    return {};
  std::vector<bool> contiguous(types.front().shape.size(), true);
  for (const MemRefTypeModel &type : types) {
    if (type.shape.size() != contiguous.size())
      throw std::runtime_error("flatten operand rank mismatch");
    if (!type.hasStridedLayout || IsIdentityStrides(type))
      continue;
    for (size_t axis = 1; axis < type.shape.size(); ++axis) {
      if (!type.strides[axis] || !type.strides[axis - 1] ||
          !type.shape[axis]) {
        contiguous[axis] = false;
        continue;
      }
      if (detail::CheckedShapeProduct(*type.strides[axis],
                                      *type.shape[axis]) !=
          *type.strides[axis - 1])
        contiguous[axis] = false;
    }
  }
  if (!contiguous.empty())
    contiguous.front() = true;
  return contiguous;
}

} // namespace cvub

#endif
