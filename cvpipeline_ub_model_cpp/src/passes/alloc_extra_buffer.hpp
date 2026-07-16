#ifndef CVPIPELINE_UB_MODEL_CPP_ALLOC_EXTRA_BUFFER_HPP
#define CVPIPELINE_UB_MODEL_CPP_ALLOC_EXTRA_BUFFER_HPP

#include "one_shot_bufferize.hpp"

namespace cvub {

struct ExtraBufferAllocation {
  int ownerOperation = -1;
  std::string ownerName;
  std::string type;
};

inline bool HasAllocExtraBufferInterface(const std::string &name) {
  static const std::set<std::string> operations = {
      "hivm.hir.vreduce", "hivm.hir.vsel", "hivm.hir.vbrc",
      "hivm.hir.vcast", "hivm.hir.vxor", "hivm.hir.vpow",
      "hivm.hir.vinterleave", "hivm.hir.vmulextui", "hivm.hir.vgather",
      "hivm.hir.vtranspose", "hivm.hir.vsort", "hivm.hir.vsub",
      "hivm.hir.vdiv", "hivm.hir.vmul", "hivm.hir.vadd",
      "hivm.hir.vmax", "hivm.hir.vmin", "hivm.hir.vand",
      "hivm.hir.vor", "hivm.hir.vshl", "hivm.hir.vshr",
      "hivm.hir.vnot", "hivm.hir.vabs", "hivm.hir.vln",
      "hivm.hir.vrelu", "hivm.hir.vexp", "hivm.hir.vrsqrt",
      "hivm.hir.vsqrt", "hivm.hir.vrec", "hivm.hir.custom"};
  return operations.count(name) != 0;
}

inline int64_t CeilFactor(int64_t value, int64_t factor) {
  if (value < 0 || factor <= 0)
    throw std::runtime_error("AllocExtraBuffer: invalid ceil-factor operand");
  if (value > std::numeric_limits<int64_t>::max() - factor + 1)
    throw std::overflow_error("AllocExtraBuffer: ceil-factor overflow");
  return (value + factor - 1) / factor * factor;
}

inline int64_t StaticElementCount(const MemRefTypeModel &type) {
  int64_t result = 1;
  for (const std::optional<int64_t> &dimension : type.shape) {
    if (!dimension)
      throw std::runtime_error("AllocExtraBuffer: dynamic shape is unsupported");
    result = detail::CheckedShapeProduct(result, *dimension);
  }
  return result;
}

inline std::vector<int64_t> ParseI64Array(const std::string &text) {
  std::vector<int64_t> result;
  size_t colon = text.find(':');
  size_t close = text.rfind('>');
  if (colon == std::string::npos || close == std::string::npos || colon >= close)
    return result;
  for (const std::string &item : splitTopLevel(text.substr(colon + 1,
                                                            close - colon - 1)))
    result.push_back(std::stoll(trim(item)));
  return result;
}

inline std::string ParseEnumValue(const std::string &text) {
  size_t open = text.rfind('<');
  size_t close = text.rfind('>');
  return open == std::string::npos || close == std::string::npos || open >= close
             ? text
             : text.substr(open + 1, close - open - 1);
}

class AllocExtraBufferModel {
public:
  explicit AllocExtraBufferModel(const GenericModule &inputModule,
                                 std::string functionName = "")
      : module(inputModule), valueTypes(ValueTypes(inputModule)),
        definitions(DefiningOperations(inputModule)),
        function(std::move(functionName)) {}

  std::vector<ExtraBufferAllocation> Run() const {
    std::vector<ExtraBufferAllocation> result;
    for (const GenericOperation &operation : module.operations) {
      const GenericOperation *owner = enclosingFunction(operation);
      if (!owner || (!function.empty() && functionName(*owner) != function) ||
          ParseEnumValue(FindDictionaryValue(owner->attributes,
                                             "hacc.function_kind")) == "HOST")
        continue;
      std::optional<std::pair<int64_t, std::string>> allocation =
          getExtraBuffer(operation);
      if (!allocation)
        continue;
      result.push_back({operation.id, operation.name,
                        "memref<" + std::to_string(allocation->first) + "x" +
                            allocation->second + ">"});
    }
    return result;
  }

private:
  const GenericOperation *
  enclosingFunction(const GenericOperation &operation) const {
    const GenericOperation *current = &operation;
    while (current && current->name != "func.func") {
      if (current->parentId < 0)
        return nullptr;
      current = &module.operations.at(static_cast<size_t>(current->parentId));
    }
    return current;
  }

  std::string functionName(const GenericOperation &operation) const {
    std::string name = FindDictionaryValue(operation.properties, "sym_name");
    if (name.size() >= 2 && name.front() == '"' && name.back() == '"')
      name = name.substr(1, name.size() - 2);
    return name;
  }

  const MemRefTypeModel &memrefOperand(const GenericOperation &operation,
                                      size_t index,
                                      MemRefTypeModel &storage) const {
    if (index >= operation.operandTypes.size())
      throw std::runtime_error("AllocExtraBuffer: missing operand type for " +
                               operation.name);
    std::optional<MemRefTypeModel> parsed =
        ParseMemRefType(operation.operandTypes[index]);
    if (!parsed)
      throw std::runtime_error("AllocExtraBuffer: expected memref operand for " +
                               operation.name);
    storage = *parsed;
    return storage;
  }

  bool hasTempBuffer(const GenericOperation &operation) const {
    const std::vector<int64_t> segments = ParseI64Array(
        FindDictionaryValue(operation.attributes, "operandSegmentSizes"));
    return segments.size() >= 3 && segments.back() != 0;
  }

  int64_t traceToAllocMaxSize(const GenericOperation &operation,
                              size_t operand) const {
    if (operand >= operation.operands.size())
      throw std::runtime_error("AllocExtraBuffer: missing traced operand");
    int value = operation.operands[operand];
    std::set<int> seen;
    while (seen.insert(value).second) {
      auto found = definitions.find(value);
      if (found == definitions.end())
        break;
      const GenericOperation &definition = *found->second;
      if (definition.name == "memref.alloc") {
        auto type = valueTypes.find(value);
        if (type == valueTypes.end())
          break;
        std::optional<MemRefTypeModel> parsed = ParseMemRefType(type->second);
        if (parsed)
          return StaticElementCount(*parsed);
      }
      static const std::set<std::string> aliases = {
          "memref.cast", "memref.collapse_shape", "memref.expand_shape",
          "memref.extract_strided_metadata", "memref.reinterpret_cast",
          "memref.subview", "memref.view"};
      if (aliases.count(definition.name) == 0 || definition.operands.empty())
        break;
      value = definition.operands.front();
    }
    std::optional<MemRefTypeModel> parsed =
        ParseMemRefType(operation.operandTypes.at(operand));
    if (!parsed)
      throw std::runtime_error("AllocExtraBuffer: cannot trace alloc type");
    return StaticElementCount(*parsed);
  }

  std::optional<std::pair<int64_t, std::string>>
  getExtraBuffer(const GenericOperation &operation) const {
    if (hasTempBuffer(operation))
      return std::nullopt;
    if (operation.name == "hivm.hir.vcast")
      return getVCastExtraBuffer(operation);
    if (operation.name == "hivm.hir.vinterleave")
      return getVInterleaveExtraBuffer(operation);
    if (operation.name == "hivm.hir.vpow")
      return getVPowExtraBuffer(operation);
    if (operation.name == "hivm.hir.vsel")
      return getVSelExtraBuffer(operation);
    if (operation.name == "hivm.hir.vsort")
      return getVSortExtraBuffer(operation);
    if (operation.name == "hivm.hir.vxor") {
      MemRefTypeModel source;
      memrefOperand(operation, 0, source);
      return std::pair<int64_t, std::string>{traceToAllocMaxSize(operation, 0),
                                             source.elementType};
    }
    if (operation.name == "hivm.hir.vdiv")
      return getVDivExtraBuffer(operation);
    if (operation.name == "hivm.hir.vbrc")
      return getVBrcExtraBuffer(operation);
    if (operation.name == "hivm.hir.vreduce")
      return getVReduceExtraBuffer(operation);
    return std::nullopt;
  }

  std::optional<std::pair<int64_t, std::string>>
  getVCastExtraBuffer(const GenericOperation &operation) const {
    MemRefTypeModel source, destination;
    memrefOperand(operation, 0, source);
    memrefOperand(operation, 1, destination);
    const uint64_t srcWidth = GetElementBitWidth(source);
    const uint64_t dstWidth = GetElementBitWidth(destination);
    if (srcWidth == 1)
      return std::pair<int64_t, std::string>{
          static_cast<int64_t>(32U * 3U * 8U / dstWidth),
          destination.elementType};
    if ((source.elementType == "i32" && destination.elementType == "i16") ||
        (source.elementType == "i64" && destination.elementType == "i32")) {
      if (ParseEnumValue(FindDictionaryValue(operation.attributes,
                                             "round_mode")) !=
          "truncwithoverflow")
        return std::nullopt;
      return std::pair<int64_t, std::string>{0, source.elementType};
    }
    if ((source.elementType == "i32" || source.elementType == "i16") &&
        destination.elementType == "i8")
      return std::pair<int64_t, std::string>{
          traceToAllocMaxSize(operation, 0) * 2, source.elementType};
    return std::nullopt;
  }

  std::optional<std::pair<int64_t, std::string>>
  getVInterleaveExtraBuffer(const GenericOperation &operation) const {
    MemRefTypeModel source;
    memrefOperand(operation, 0, source);
    if (source.shape.empty() || !source.shape.back())
      throw std::runtime_error("AllocExtraBuffer: dynamic vinterleave axis");
    const int64_t perBlock = static_cast<int64_t>(32U * 8U /
                                                  GetElementBitWidth(source));
    const int64_t axis = *source.shape.back();
    const int64_t sourceAligned = CeilFactor(axis, 8);
    const int64_t destinationAligned = CeilFactor(axis * 2, perBlock);
    return std::pair<int64_t, std::string>{
        sourceAligned * perBlock + destinationAligned, source.elementType};
  }

  std::optional<std::pair<int64_t, std::string>>
  getVPowExtraBuffer(const GenericOperation &operation) const {
    MemRefTypeModel source;
    memrefOperand(operation, 0, source);
    if (source.elementType != "i32")
      return std::nullopt;
    const int64_t shapeSize = traceToAllocMaxSize(operation, 0);
    const int64_t boolAligned = CeilFactor(shapeSize, 256) / 32;
    const int64_t reduceAligned = shapeSize <= 64
                                      ? CeilFactor(3 * shapeSize / 2, 8)
                                      : 72;
    return std::pair<int64_t, std::string>{
        2 * shapeSize + 8 + 2 * boolAligned + reduceAligned,
        source.elementType};
  }

  std::optional<std::pair<int64_t, std::string>>
  getVSelExtraBuffer(const GenericOperation &operation) const {
    MemRefTypeModel destination;
    memrefOperand(operation, operation.operandTypes.size() - 1, destination);
    const bool src0Scalar = !IsMemRefType(operation.operandTypes.at(1));
    const bool src1Scalar = !IsMemRefType(operation.operandTypes.at(2));
    const std::string sourceElement =
        src0Scalar ? ParseMemRefType(operation.operandTypes.at(2))->elementType
                   : ParseMemRefType(operation.operandTypes.at(1))->elementType;
    MemRefTypeModel condition;
    const std::string conditionElement =
        ParseMemRefType(operation.operandTypes.at(0))->elementType;
    (void)condition;
    if (sourceElement == "i64" &&
        (conditionElement == "i1" || src0Scalar || src1Scalar))
      return std::nullopt;
    if (sourceElement != "i64") {
      const int64_t perStride = static_cast<int64_t>(256U /
          GetElementBitWidth(destination));
      return std::pair<int64_t, std::string>{
          perStride * (1 + static_cast<int>(src0Scalar) +
                       static_cast<int>(src1Scalar)),
          destination.elementType};
    }
    if (destination.shape.empty() || !destination.shape.back())
      throw std::runtime_error("AllocExtraBuffer: dynamic vsel destination");
    const int64_t size = *destination.shape.back();
    return std::pair<int64_t, std::string>{CeilFactor(size, 8) / 2 +
                                               CeilFactor(size, 16) / 4,
                                           destination.elementType};
  }

  std::optional<std::pair<int64_t, std::string>>
  getVSortExtraBuffer(const GenericOperation &operation) const {
    MemRefTypeModel source;
    memrefOperand(operation, 0, source);
    if (source.shape.empty() || !source.shape.back())
      throw std::runtime_error("AllocExtraBuffer: dynamic vsort axis");
    const int64_t axis = *source.shape.back() == 1 ? 0 : *source.shape.back();
    const int64_t aligned = CeilFactor(axis, 32);
    const bool descending =
        FindDictionaryValue(operation.attributes, "descending") == "true";
    const bool needIndex = operation.dpsInits.size() == 2;
    int64_t factor = 0;
    if (source.elementType == "f32")
      factor = needIndex ? (descending ? 4 : 5) : (descending ? 5 : 6);
    else if (source.elementType == "f16")
      factor = needIndex ? (descending ? 8 : 9) : (descending ? 10 : 11);
    return std::pair<int64_t, std::string>{aligned * factor,
                                           source.elementType};
  }

  std::optional<std::pair<int64_t, std::string>>
  getVDivExtraBuffer(const GenericOperation &operation) const {
    if (operation.operandTypes.size() < 3)
      throw std::runtime_error("AllocExtraBuffer: malformed vdiv");
    const bool src0Scalar = !IsMemRefType(operation.operandTypes[0]);
    const bool src1Scalar = !IsMemRefType(operation.operandTypes[1]);
    MemRefTypeModel destination;
    memrefOperand(operation, 2, destination);
    if (!src0Scalar && !src1Scalar)
      return std::nullopt;
    return std::pair<int64_t, std::string>{
        static_cast<int64_t>(32U * 8U / GetElementBitWidth(destination)),
        destination.elementType};
  }

  std::optional<std::pair<int64_t, std::string>>
  getVBrcExtraBuffer(const GenericOperation &operation) const {
    std::vector<int64_t> dimensions = ParseI64Array(
        FindDictionaryValue(operation.attributes, "broadcast_dims"));
    if (dimensions.empty())
      return std::nullopt;
    if (dimensions.size() != 1)
      throw std::runtime_error("AllocExtraBuffer: vbrc was not decomposed");
    MemRefTypeModel source, destination;
    memrefOperand(operation, 0, source);
    memrefOperand(operation, 1, destination);
    const int64_t rank = static_cast<int64_t>(destination.shape.size());
    if (rank <= 1)
      return std::nullopt;
    int64_t dimension = dimensions.front();
    const bool first = dimension + 3 - rank <= 0;
    const bool last = dimension + 3 - rank == 2;
    if (!first && !last)
      return std::nullopt;
    if (last && std::all_of(source.shape.begin(), source.shape.end(),
                           [](const std::optional<int64_t> &value) {
                             return value && *value == 1;
                           }))
      return std::nullopt;
    const uint64_t width = GetElementBitWidth(destination);
    const int64_t perBlock = static_cast<int64_t>(256U / width);
    bool aligned = false;
    if (destination.strides.size() >= 2 &&
        destination.strides[destination.strides.size() - 2])
      aligned = (*destination.strides[destination.strides.size() - 2] *
                 static_cast<int64_t>(width / 8U)) % 32 == 0;
    if (first && aligned)
      return std::nullopt;
    if (!std::all_of(destination.shape.begin(), destination.shape.end(),
                     [](const std::optional<int64_t> &value) {
                       return value.has_value();
                     }))
      throw std::runtime_error("AllocExtraBuffer: dynamic vbrc shape");
    const int64_t sourceSize = traceToAllocMaxSize(operation, 0);
    const int64_t destinationSize = traceToAllocMaxSize(operation, 1);
    const std::vector<std::optional<int64_t>> &shape = destination.shape;
    int64_t size = 0;
    if (first) {
      int64_t b = *shape.front();
      int64_t c = *shape.back();
      if (rank > 2)
        c *= *shape[shape.size() - 2];
      size = b * CeilFactor(c, perBlock);
    } else if (aligned) {
      int64_t a = *shape[shape.size() - 2];
      int64_t b = *shape.back();
      const bool needs = (a % 8 != 0 || b != perBlock) && width != 64;
      if (!needs)
        size = 0;
      else {
        a = CeilFactor(a, 8);
        size = width == 1 ? (a + 2) * perBlock + a * 16
                          : a * perBlock;
      }
    } else {
      int64_t a = *shape[shape.size() - 2];
      int64_t b = *shape.back();
      (void)sourceSize;
      (void)destinationSize;
      size = width == 64 ? a * CeilFactor(b, perBlock)
                         : CeilFactor(a, 8) * CeilFactor(b, perBlock);
    }
    return std::pair<int64_t, std::string>{size, destination.elementType};
  }

  bool functionHasAttribute(const GenericOperation &operation,
                            const std::string &name) const {
    int parent = operation.parentId;
    while (parent >= 0) {
      const GenericOperation &owner =
          module.operations.at(static_cast<size_t>(parent));
      if (!FindDictionaryValue(owner.attributes, name).empty())
        return true;
      parent = owner.parentId;
    }
    return false;
  }

  std::optional<std::pair<int64_t, std::string>>
  getVReduceExtraBuffer(const GenericOperation &operation) const {
    MemRefTypeModel source, destination;
    memrefOperand(operation, 0, source);
    memrefOperand(operation, 1, destination);
    if (!std::all_of(source.shape.begin(), source.shape.end(),
                     [](const std::optional<int64_t> &value) {
                       return value.has_value();
                     }))
      throw std::runtime_error("AllocExtraBuffer: dynamic vreduce shape");
    const std::vector<int64_t> dimensions = ParseI64Array(
        FindDictionaryValue(operation.attributes, "reduce_dims"));
    const std::string arithmetic = ParseEnumValue(
        FindDictionaryValue(operation.attributes, "arith"));
    const int64_t rank = static_cast<int64_t>(source.shape.size());
    const int64_t allocSize = traceToAllocMaxSize(operation, 0);
    const int64_t width = static_cast<int64_t>(GetElementBitWidth(source));
    const int64_t perBlock = 256 / width;
    const int64_t perRepeat = 2048 / width;
    const bool saveUb = functionHasAttribute(operation, "enable_saving_ub");
    std::optional<int64_t> maximum;
    for (int64_t dimension : dimensions) {
      if (dimension < 0 || dimension >= rank)
        throw std::runtime_error("AllocExtraBuffer: invalid reduction axis");
      const int64_t r = *source.shape[static_cast<size_t>(dimension)];
      int64_t a = dimension == 0
                      ? 1
                      : *source.shape[static_cast<size_t>(dimension - 1)];
      if (rank >= 3 && dimension == rank - 1) {
        const size_t a0Index = static_cast<size_t>(rank - 3);
        const size_t a1Index = static_cast<size_t>(rank - 2);
        const int64_t a0 = *source.shape[a0Index];
        const int64_t a1 = *source.shape[a1Index];
        bool fused = false;
        if (source.strides.size() == source.shape.size() &&
            destination.strides.size() == destination.shape.size()) {
          const auto &ss = source.strides;
          const auto &ds = destination.strides;
          if (ss[a0Index] && ss[a1Index] && ss.back() && ds[a0Index] &&
              ds[a1Index] && ds.back() && *ss[a1Index] != 0 &&
              *ds[a1Index] != 0 && *ss[a0Index] % *ss[a1Index] == 0 &&
              *ds[a0Index] % *ds[a1Index] == 0 && *ss.back() == 1 &&
              *ds.back() == 1 &&
              *ss[a0Index] / *ss[a1Index] == *ds[a0Index] / *ds[a1Index]) {
            a = a0 * (*ss[a0Index] / *ss[a1Index]);
            fused = true;
          }
          if (!fused) {
            const bool loopOverA0 =
                *ds[a1Index] == 1 || (*ds[a0Index] != 1 && a0 <= a1);
            a = loopOverA0 ? a1 : a0;
          }
        } else {
          a = a0 * a1;
        }
      }
      std::optional<int64_t> current;
      const bool integer = HasIntegerElementType(source);
      const bool sumMaxMin = arithmetic == "sum" || arithmetic == "max" ||
                             arithmetic == "min";
      const bool indexReduction =
          arithmetic.find("with_index") != std::string::npos;
      if (indexReduction) {
        if (dimension == rank - 1)
          current = perBlock;
        else {
          int64_t aDimension = 1;
          for (size_t index = 0; index < source.shape.size(); ++index)
            if (static_cast<int64_t>(index) != dimension)
              aDimension *= *source.shape[index];
          const int64_t elementBytes = (width + 7) / 8;
          const int64_t elements =
              ((aDimension * 4 + perRepeat - 1) / perRepeat) * perRepeat /
              elementBytes;
          const int64_t mask =
              ((aDimension + 8 * perBlock - 1) / (8 * perBlock)) * perBlock;
          current = elements + mask + perBlock;
        }
      } else if (dimension != rank - 1) {
        // The full ARA rule is needed only for rank-3 middle-axis cases.
        if (rank != 3)
          current = allocSize + (arithmetic == "xori" ? perBlock : 0);
        else {
          const int64_t s0 = *source.shape[0];
          const int64_t s1 = *source.shape[1];
          const int64_t s2 = *source.shape[2];
          int64_t main = 1;
          while (main <= s1 / 2)
            main *= 2;
          int64_t log = 0;
          for (int64_t value = main; value > 1; value >>= 1)
            ++log;
          const int64_t repeats = log + s1 - main;
          const bool isXor = arithmetic == "xori";
          const int64_t baseline = allocSize + (isXor ? perBlock : 0);
          if (repeats <= s0) {
            const int64_t half = main / 2 * s0 * CeilFactor(s2, perBlock);
            current = isXor
                          ? std::max(baseline, CeilFactor(half, perBlock) + half)
                          : std::max(baseline, half);
          } else if (isXor) {
            const int64_t alignedS2 = CeilFactor(s2, perBlock);
            const int64_t xorFront =
                CeilFactor(s1 * alignedS2 / 2, perBlock);
            const int64_t stride1 = source.strides[1].value_or(s2);
            current = std::max(s1 * s2 + perBlock,
                               xorFront + main / 2 * stride1);
          } else {
            const int64_t stride1 = source.strides[1].value_or(s2);
            current = std::max(s1 * s2, main / 2 * stride1);
          }
        }
      } else if (integer || arithmetic == "prod" || arithmetic == "ori" ||
                 arithmetic == "xori") {
        const int64_t multiplier = arithmetic == "xori" ? 2 : 1;
        current = r > perRepeat
                      ? a * perRepeat * multiplier + a * perBlock
                      : (arithmetic == "xori" ? 3 : 2) * allocSize;
      } else if (sumMaxMin && saveUb) {
        if (r > perRepeat)
          current = CeilFactor(allocSize, perBlock) / perBlock;
      } else if ((source.elementType == "f16" ||
                  source.elementType == "f32")) {
        if ((arithmetic == "max" || arithmetic == "min") && dimension == 0 &&
            rank == 1)
          current = r <= perRepeat ? std::nullopt
                                   : std::optional<int64_t>(perRepeat);
        else if (r < perBlock)
          current = r % 2 == 0 ? std::optional<int64_t>(allocSize / 2)
                               : std::nullopt;
        else if (r <= perRepeat)
          current = std::nullopt;
        else if (r <= perRepeat * 2)
          current = a * perRepeat;
        else
          current = allocSize / 2;
      } else {
        current = allocSize;
      }
      if (current)
        maximum = maximum ? std::max(*maximum, *current) : current;
    }
    if (!maximum)
      return std::nullopt;
    return std::pair<int64_t, std::string>{*maximum, source.elementType};
  }

  const GenericModule &module;
  std::map<int, std::string> valueTypes;
  std::map<int, const GenericOperation *> definitions;
  std::string function;
};

inline std::vector<ExtraBufferAllocation>
ModelAllocExtraBuffer(const GenericModule &module,
                      const std::string &function = "") {
  return AllocExtraBufferModel(module, function).Run();
}

inline std::vector<ExtraBufferAllocation>
CollectAllocExtraBufferOracle(const GenericModule &before,
                              const GenericModule &after) {
  std::vector<ExtraBufferAllocation> result;
  size_t beforeIndex = 0;
  for (size_t afterIndex = 0; afterIndex < after.operations.size(); ++afterIndex) {
    const GenericOperation &operation = after.operations[afterIndex];
    if (beforeIndex < before.operations.size() &&
        operation.name == before.operations[beforeIndex].name) {
      ++beforeIndex;
      continue;
    }
    if (operation.name != "memref.alloc" || afterIndex + 1 >= after.operations.size())
      throw std::runtime_error("AllocExtraBuffer oracle: unexpected pass delta");
    const GenericOperation &owner = after.operations[afterIndex + 1];
    if (operation.resultTypes.size() != 1)
      throw std::runtime_error("AllocExtraBuffer oracle: malformed allocation");
    result.push_back({owner.id, owner.name, operation.resultTypes.front()});
  }
  if (beforeIndex != before.operations.size())
    throw std::runtime_error("AllocExtraBuffer oracle: operation was removed");
  return result;
}

inline std::string SerializeExtraBufferAllocations(
    const std::vector<ExtraBufferAllocation> &allocations) {
  std::ostringstream output;
  output << "ALLOC_EXTRA_BUFFER\t1\n";
  for (size_t index = 0; index < allocations.size(); ++index)
    output << "ALLOC\t" << index << '\t'
           << HexEncode(allocations[index].ownerName) << '\t'
           << HexEncode(allocations[index].type) << '\n';
  return output.str();
}

} // namespace cvub

#endif
