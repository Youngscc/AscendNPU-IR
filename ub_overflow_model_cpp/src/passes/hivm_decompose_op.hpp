#ifndef CVPIPELINE_UB_MODEL_CPP_HIVM_DECOMPOSE_OP_HPP
#define CVPIPELINE_UB_MODEL_CPP_HIVM_DECOMPOSE_OP_HPP

#include "../pipeline/bufferized_semantic_ir.hpp"

namespace cvub {

struct DecomposeBufferAllocation {
  int ownerOperation = -1;
  std::string ownerName;
  std::string type;
};

using DecomposeOperationDelta = std::map<std::string, int64_t>;

struct HIVMDecomposeResult {
  std::vector<DecomposeBufferAllocation> allocations;
  DecomposeOperationDelta operationDelta;
};

inline std::string DecomposeEnumValue(const std::string &text) {
  const size_t open = text.rfind('<');
  const size_t close = text.rfind('>');
  return open == std::string::npos || close == std::string::npos || open >= close
             ? text
             : text.substr(open + 1, close - open - 1);
}

inline std::vector<int64_t> DecomposeI64Array(const std::string &text) {
  std::vector<int64_t> result;
  const size_t colon = text.find(':');
  const size_t close = text.rfind('>');
  if (colon == std::string::npos || close == std::string::npos || colon >= close)
    return result;
  for (const std::string &item :
       splitTopLevel(text.substr(colon + 1, close - colon - 1)))
    result.push_back(std::stoll(trim(item)));
  return result;
}

inline std::string ShapedElementType(const std::string &type) {
  std::string memref = IsTensorType(type) ? ConvertTensorToMemRefType(type)
                                          : type;
  std::optional<MemRefTypeModel> parsed = ParseMemRefType(memref);
  return parsed ? parsed->elementType : std::string();
}

inline std::string MemRefWithElementType(const std::string &type,
                                         const std::string &elementType) {
  std::string memref = IsTensorType(type) ? ConvertTensorToMemRefType(type)
                                          : type;
  std::optional<MemRefTypeModel> parsed = ParseMemRefType(memref);
  if (!parsed)
    throw std::runtime_error("HIVMDecomposeOp: expected shaped type");
  std::string result = "memref<";
  for (const std::optional<int64_t> &dimension : parsed->shape)
    result += dimension ? std::to_string(*dimension) + "x" : "?x";
  return result + elementType + ">";
}

class HIVMDecomposeOpModel {
public:
  explicit HIVMDecomposeOpModel(const GenericModule &inputModule,
                                std::string functionName = "")
      : module(inputModule), function(std::move(functionName)) {}

  std::vector<DecomposeBufferAllocation> Run() const {
    return RunCombined().allocations;
  }

  HIVMDecomposeResult RunCombined() const {
    HIVMDecomposeResult result;
    for (const GenericOperation &operation : module.operations) {
      const GenericOperation *owner = enclosingFunction(operation);
      if (!owner || (!function.empty() && functionName(*owner) != function) ||
          isHost(*owner))
        continue;
      appendAllocations(operation, result.allocations);
      appendOperationDelta(operation, result.operationDelta);
    }
    return result;
  }

  DecomposeOperationDelta OperationDelta() const {
    DecomposeOperationDelta result;
    for (const GenericOperation &operation : module.operations) {
      const GenericOperation *owner = enclosingFunction(operation);
      if (!owner || (!function.empty() && functionName(*owner) != function) ||
          isHost(*owner))
        continue;
      appendOperationDelta(operation, result);
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

  bool isHost(const GenericOperation &operation) const {
    return DecomposeEnumValue(FindDictionaryValue(operation.attributes,
                                                   "hacc.function_kind")) ==
           "HOST";
  }

  std::string operandType(const GenericOperation &operation,
                          size_t index) const {
    if (index >= operation.operandTypes.size())
      throw std::runtime_error("HIVMDecomposeOp: missing operand type for " +
                               operation.name);
    return operation.operandTypes[index];
  }

  size_t destinationIndex(const GenericOperation &operation) const {
    const std::vector<size_t> destinations = DpsInitOperandIndices(
        operation.name, operation.operands.size(), operation.properties);
    if (destinations.empty())
      throw std::runtime_error("HIVMDecomposeOp: missing destination for " +
                               operation.name);
    return destinations.front();
  }

  void add(const GenericOperation &operation, const std::string &type,
           std::vector<DecomposeBufferAllocation> &result) const {
    result.push_back({operation.id, operation.name, type});
  }

  void appendVCast(const GenericOperation &operation,
                   std::vector<DecomposeBufferAllocation> &result) const {
    const std::string source = operandType(operation, 0);
    const std::string destination =
        operandType(operation, destinationIndex(operation));
    const std::string sourceElement = ShapedElementType(source);
    const std::string destinationElement = ShapedElementType(destination);
    if ((sourceElement == "f32" || sourceElement == "i4" ||
         sourceElement == "i1") &&
        destinationElement == "i8") {
      add(operation, MemRefWithElementType(source, "f16"), result);
      return;
    }
    if ((sourceElement == "i8" || sourceElement == "i16") &&
        destinationElement == "i1") {
      add(operation, MemRefWithElementType(source, "f16"), result);
      add(operation, MemRefWithElementType(source, "f16"), result);
      return;
    }
    if (sourceElement == "i32" && destinationElement == "i1") {
      add(operation, MemRefWithElementType(source, "f32"), result);
      add(operation, MemRefWithElementType(source, "f32"), result);
    }
  }

  void appendVCmp(const GenericOperation &operation,
                  std::vector<DecomposeBufferAllocation> &result) const {
    const std::string source = operandType(operation, 0);
    const std::string destination =
        operandType(operation, destinationIndex(operation));
    const std::string sourceElement = ShapedElementType(source);
    if (sourceElement.empty() || sourceElement.front() != 'i' ||
        ShapedElementType(destination) != "i1")
      return;
    const std::string compareMode = DecomposeEnumValue(
        FindDictionaryValue(operation.properties, "compare_mode"));
    if (sourceElement == "i32" &&
        (compareMode == "eq" || compareMode == "ne"))
      return;
    add(operation, MemRefWithElementType(source, "i8"), result);
    add(operation, MemRefWithElementType(source, "f16"), result);
    add(operation, MemRefWithElementType(source, "f16"), result);
  }

  void appendVBrc(const GenericOperation &operation,
                  std::vector<DecomposeBufferAllocation> &result) const {
    const std::vector<int64_t> dimensions = DecomposeI64Array(
        FindDictionaryValue(operation.properties, "broadcast_dims"));
    if (dimensions.size() <= 1)
      return;
    const std::string destination =
        operandType(operation, destinationIndex(operation));
    std::optional<MemRefTypeModel> current = ParseMemRefType(
        IsTensorType(operandType(operation, 0))
            ? ConvertTensorToMemRefType(operandType(operation, 0))
            : operandType(operation, 0));
    std::optional<MemRefTypeModel> target = ParseMemRefType(
        IsTensorType(destination) ? ConvertTensorToMemRefType(destination)
                                  : destination);
    if (!current || !target || current->shape.size() != target->shape.size())
      throw std::runtime_error("HIVMDecomposeOp: malformed multi-axis vbrc");
    for (size_t index = dimensions.size(); index > 1; --index) {
      const int64_t dimension = dimensions[index - 1];
      if (dimension < 0 || static_cast<size_t>(dimension) >= current->shape.size())
        throw std::runtime_error("HIVMDecomposeOp: invalid broadcast dimension");
      current->shape[static_cast<size_t>(dimension)] =
          target->shape[static_cast<size_t>(dimension)];
      std::string type = "memref<";
      for (const std::optional<int64_t> &extent : current->shape)
        type += extent ? std::to_string(*extent) + "x" : "?x";
      add(operation, type + current->elementType + ">", result);
    }
  }

  void appendAtomic(const GenericOperation &operation,
                    std::vector<DecomposeBufferAllocation> &result) const {
    const std::string source = operandType(operation, 0);
    if (operation.name == "hivm.hir.atomic_cas") {
      add(operation, MemRefWithElementType(source, ShapedElementType(source)),
          result);
      add(operation, MemRefWithElementType(source, "i1"), result);
      add(operation, MemRefWithElementType(source, ShapedElementType(source)),
          result);
      return;
    }
    if (operation.name == "hivm.hir.atomic_xchg") {
      add(operation, MemRefWithElementType(source, ShapedElementType(source)),
          result);
      const std::vector<size_t> segments =
          OperandSegmentSizes(operation.properties);
      if (segments.size() >= 3 && segments[2] != 0)
        add(operation,
            MemRefWithElementType(source, ShapedElementType(source)), result);
      return;
    }
    if (operation.name == "hivm.hir.atomic_rmw") {
      const std::string kind = DecomposeEnumValue(
          FindDictionaryValue(operation.properties, "atomic_kind"));
      if (!operation.results.empty() || kind == "and" || kind == "or" ||
          kind == "xor") {
        add(operation,
            MemRefWithElementType(source, ShapedElementType(source)), result);
        add(operation,
            MemRefWithElementType(source, ShapedElementType(source)), result);
      }
    }
  }

  void appendAllocations(
      const GenericOperation &operation,
      std::vector<DecomposeBufferAllocation> &result) const {
    if (operation.name == "hivm.hir.vcast")
      appendVCast(operation, result);
    else if (operation.name == "hivm.hir.vcmp")
      appendVCmp(operation, result);
    else if (operation.name == "hivm.hir.vbrc")
      appendVBrc(operation, result);
    else if (operation.name == "hivm.hir.atomic_rmw" ||
             operation.name == "hivm.hir.atomic_cas" ||
             operation.name == "hivm.hir.atomic_xchg")
      appendAtomic(operation, result);
  }

  void change(DecomposeOperationDelta &result, const std::string &name,
              int64_t amount) const {
    result[name] += amount;
    if (result[name] == 0)
      result.erase(name);
  }

  bool lowersVCast(const GenericOperation &operation) const {
    const std::string sourceElement =
        ShapedElementType(operandType(operation, 0));
    const std::string destinationElement = ShapedElementType(
        operandType(operation, destinationIndex(operation)));
    return ((sourceElement == "f32" || sourceElement == "i4" ||
             sourceElement == "i1") &&
            destinationElement == "i8") ||
           ((sourceElement == "i8" || sourceElement == "i16" ||
             sourceElement == "i32") &&
            destinationElement == "i1");
  }

  bool lowersVCmp(const GenericOperation &operation) const {
    const std::string sourceElement =
        ShapedElementType(operandType(operation, 0));
    const std::string destinationElement = ShapedElementType(
        operandType(operation, destinationIndex(operation)));
    if (sourceElement.empty() || sourceElement.front() != 'i' ||
        destinationElement != "i1")
      return false;
    const std::string compareMode = DecomposeEnumValue(
        FindDictionaryValue(operation.properties, "compare_mode"));
    return sourceElement != "i32" ||
           (compareMode != "eq" && compareMode != "ne");
  }

  void appendOperationDelta(const GenericOperation &operation,
                            DecomposeOperationDelta &result) const {
    if (operation.name == "hivm.hir.vcast" && lowersVCast(operation)) {
      const std::string sourceElement =
          ShapedElementType(operandType(operation, 0));
      const std::string destinationElement = ShapedElementType(
          operandType(operation, destinationIndex(operation)));
      if (destinationElement == "i8") {
        change(result, "hivm.hir.vcast", 1);
      } else if (destinationElement == "i1" &&
                 (sourceElement == "i8" || sourceElement == "i16" ||
                  sourceElement == "i32")) {
        change(result, "hivm.hir.vbrc", 1);
        change(result, "hivm.hir.vcmp", 1);
      }
      return;
    }
    if (operation.name == "hivm.hir.vcmp" && lowersVCmp(operation)) {
      change(result, "hivm.hir.vcast", 1);
      change(result, "hivm.hir.vbrc", 1);
      change(result, "hivm.hir.vcmp", 1);
      return;
    }
    if (operation.name == "hivm.hir.vsub" &&
        operation.operandTypes.size() > 1 &&
        !IsTensorType(operation.operandTypes[1]) &&
        !IsMemRefType(operation.operandTypes[1])) {
      change(result, "hivm.hir.vsub", -1);
      change(result, "hivm.hir.vadd", 1);
      return;
    }
    if (operation.name == "hivm.hir.vrec") {
      change(result, "hivm.hir.vrec", -1);
      change(result, "hivm.hir.vbrc", 1);
      change(result, "hivm.hir.vdiv", 1);
      return;
    }
    if (operation.name == "hivm.hir.atomic_cas") {
      change(result, operation.name, -1);
      change(result, "hivm.hir.load", 1);
      change(result, "hivm.hir.vcmp", 1);
      change(result, "hivm.hir.vsel", 1);
      change(result, "hivm.hir.store", 1);
      return;
    }
    if (operation.name == "hivm.hir.atomic_xchg") {
      change(result, operation.name, -1);
      change(result, "hivm.hir.load", 1);
      change(result, "hivm.hir.store", 1);
      const std::vector<size_t> segments =
          OperandSegmentSizes(operation.properties);
      change(result, segments.size() >= 3 && segments[2] != 0
                         ? "hivm.hir.vsel"
                         : "hivm.hir.copy",
             segments.size() >= 3 && segments[2] != 0 ? 2 : 1);
      return;
    }
    if (operation.name == "hivm.hir.atomic_rmw") {
      const std::string kind = DecomposeEnumValue(
          FindDictionaryValue(operation.properties, "atomic_kind"));
      if (!operation.results.empty() || kind == "and" || kind == "or" ||
          kind == "xor") {
        change(result, operation.name, -1);
        change(result, "hivm.hir.load", 1);
        change(result, "hivm.hir.v" + kind, 1);
        change(result, "hivm.hir.store", 1);
      }
    }
  }

  const GenericModule &module;
  std::string function;
};

inline std::vector<DecomposeBufferAllocation>
ModelHIVMDecomposeOp(const GenericModule &module,
                     const std::string &function = "") {
  return HIVMDecomposeOpModel(module, function).Run();
}

inline DecomposeOperationDelta
ModelHIVMDecomposeOperationDelta(const GenericModule &module,
                                 const std::string &function = "") {
  return HIVMDecomposeOpModel(module, function).OperationDelta();
}

inline HIVMDecomposeResult
ModelHIVMDecompose(const GenericModule &module,
                   const std::string &function = "") {
  return HIVMDecomposeOpModel(module, function).RunCombined();
}

inline std::string SerializeDecomposeBufferAllocations(
    const std::vector<DecomposeBufferAllocation> &allocations) {
  std::ostringstream output;
  output << "HIVM_DECOMPOSE_ALLOCATIONS\t1\n";
  for (const DecomposeBufferAllocation &allocation : allocations)
    output << "ALLOC\t" << HexEncode(allocation.type) << '\n';
  return output.str();
}

inline std::string
SerializeDecomposeOperationDelta(const DecomposeOperationDelta &delta) {
  std::ostringstream output;
  output << "HIVM_DECOMPOSE_OPERATION_DELTA\t1\n";
  for (const auto &[name, amount] : delta)
    output << "OP\t" << HexEncode(name) << '\t' << amount << '\n';
  return output.str();
}

} // namespace cvub

#endif
