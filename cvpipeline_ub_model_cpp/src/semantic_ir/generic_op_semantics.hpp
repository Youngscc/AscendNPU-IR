#ifndef CVPIPELINE_UB_MODEL_CPP_GENERIC_OP_SEMANTICS_HPP
#define CVPIPELINE_UB_MODEL_CPP_GENERIC_OP_SEMANTICS_HPP

#include "hivm_op_semantics.hpp"

#include <set>

namespace cvub {

inline bool HasModeledOperationSemantics(const std::string &name) {
  static const std::set<std::string> operations = {
      "affine.apply", "affine.min", "annotation.mark",
      "bufferization.alloc_tensor",
      "bufferization.to_tensor", "builtin.module", "cf.br", "cf.cond_br",
      "func.func", "func.return",
      "hivm.hir.atomic_cas", "hivm.hir.atomic_rmw", "hivm.hir.atomic_xchg",
      "hivm.hir.bitcast", "hivm.hir.debug", "hivm.hir.fixpipe",
      "hivm.hir.get_block_idx", "hivm.hir.load", "hivm.hir.mmadL1",
      "hivm.hir.pipe_barrier", "hivm.hir.pointer_cast",
      "hivm.hir.set_mask_norm", "hivm.hir.store", "hivm.hir.sync_block",
      "hivm.hir.vabs", "hivm.hir.vadd", "hivm.hir.vand",
      "hivm.hir.varange", "hivm.hir.vbrc", "hivm.hir.vcast",
      "hivm.hir.vcmp", "hivm.hir.vconcat", "hivm.hir.vcumprod",
      "hivm.hir.vcumsum", "hivm.hir.vdeinterleave", "hivm.hir.vdiv",
      "hivm.hir.vexp", "hivm.hir.vinterleave", "hivm.hir.vln",
      "hivm.hir.vmax", "hivm.hir.vmin", "hivm.hir.vmod", "hivm.hir.vmul",
      "hivm.hir.vmulextui", "hivm.hir.vnot", "hivm.hir.vor",
      "hivm.hir.vpow", "hivm.hir.vrec", "hivm.hir.vreduce",
      "hivm.hir.vsel", "hivm.hir.vshr", "hivm.hir.vsort",
      "hivm.hir.vsqrt", "hivm.hir.vsub", "hivm.hir.vtranspose",
      "llvm.inline_asm", "llvm.intr.assume", "memref.alloc",
      "memref.collapse_shape", "memref.load", "memref.reinterpret_cast",
      "memref.subview", "memref_ext.alloc_workspace", "scf.condition",
      "scf.for", "scf.if", "scf.while", "scf.yield",
      "tensor.collapse_shape", "tensor.empty", "tensor.expand_shape",
      "tensor.extract", "tensor.extract_slice", "tensor.from_elements",
      "tensor.insert", "tensor.insert_slice"};
  return startsWith(name, "arith.") || operations.count(name) != 0;
}

inline std::string JoinDelimited(const std::vector<std::string> &values,
                          const std::string &separator) {
  std::string result;
  for (size_t index = 0; index < values.size(); ++index) {
    if (index != 0)
      result += separator;
    result += values[index];
  }
  return result;
}

inline std::vector<size_t> OperandSegmentSizes(const std::string &properties) {
  const std::string marker = "operandSegmentSizes = array<i32:";
  size_t begin = properties.find(marker);
  if (begin == std::string::npos)
    return {};
  begin += marker.size();
  size_t end = properties.find('>', begin);
  if (end == std::string::npos)
    throw std::runtime_error("generic operation semantics: malformed operandSegmentSizes");
  std::vector<size_t> sizes;
  for (const std::string &item : split(properties.substr(begin, end - begin), ','))
    sizes.push_back(static_cast<size_t>(std::stoull(trim(item))));
  return sizes;
}

inline bool HasNoMemoryEffect(const std::string &name) {
  static const std::set<std::string> operations = {
      "affine.apply", "affine.min", "cf.br", "cf.cond_br", "func.return",
      "llvm.inline_asm",
      "memref.collapse_shape", "memref.reinterpret_cast", "memref.subview",
      "scf.condition", "scf.yield", "tensor.collapse_shape", "tensor.empty",
      "tensor.expand_shape", "tensor.extract", "tensor.extract_slice",
      "tensor.from_elements", "tensor.insert", "tensor.insert_slice",
      "hivm.hir.bitcast", "hivm.hir.get_block_idx"};
  if (operations.count(name) != 0)
    return true;
  return startsWith(name, "arith.");
}

inline std::vector<size_t> DpsInitOperandIndices(
    const std::string &name, size_t operandCount,
    const std::string &properties) {
  if (name == "tensor.insert" || name == "tensor.insert_slice")
    return operandCount > 1 ? std::vector<size_t>{1} : std::vector<size_t>{};
  if (!IsDestinationStyleOp(name))
    return {};

  size_t destinationSegment = 1;
  if (name == "hivm.hir.varange")
    destinationSegment = 0;
  else if (name == "hivm.hir.mmadL1")
    destinationSegment = 6;
  else if (name == "hivm.hir.vconcat")
    return operandCount == 0 ? std::vector<size_t>{}
                             : std::vector<size_t>{operandCount - 1};

  const std::vector<size_t> segmentSizes = OperandSegmentSizes(properties);
  if (segmentSizes.empty())
    return destinationSegment < operandCount
               ? std::vector<size_t>{destinationSegment}
               : std::vector<size_t>{};
  if (destinationSegment >= segmentSizes.size())
    throw std::runtime_error("generic operation semantics: destination segment is missing for " +
                             name);
  size_t begin = 0;
  for (size_t index = 0; index < destinationSegment; ++index)
    begin += segmentSizes[index];
  std::vector<size_t> result;
  for (size_t index = 0; index < segmentSizes[destinationSegment]; ++index)
    result.push_back(begin + index);
  return result;
}

inline std::string GenericMemoryEffect(const std::string &kind,
                            const std::string &value, bool fullRegion) {
  return kind + "@" + value + "@3c44656661756c743e@0@" +
         (fullRegion ? "1@" : "0@");
}

inline bool GenericIsMemRefType(const std::string &type) {
  return startsWith(type, "memref<");
}

template <typename Operation>
inline void ApplyOperationSemantics(Operation &operation) {
  if (!HasModeledOperationSemantics(operation.name))
    throw std::runtime_error("generic operation semantics: unreviewed operation " +
                             operation.name);
  const std::vector<size_t> initIndices = DpsInitOperandIndices(
      operation.name, operation.operands.size(), operation.properties);
  std::set<size_t> initSet(initIndices.begin(), initIndices.end());
  if (IsDestinationStyleOp(operation.name) ||
      operation.name == "tensor.insert" ||
      operation.name == "tensor.insert_slice") {
    for (size_t index = 0; index < operation.operands.size(); ++index) {
      if (initSet.count(index) != 0)
        operation.dpsInits.push_back(operation.operands[index]);
      else
        operation.dpsInputs.push_back(operation.operands[index]);
    }
  }

  if (IsHIVMStructuredOp(operation.name)) {
    std::vector<std::string> effects;
    for (size_t index = 0; index < operation.operands.size(); ++index) {
      if (index >= operation.operandTypes.size() ||
          !GenericIsMemRefType(operation.operandTypes[index]))
        continue;
      const std::string value = std::to_string(operation.operands[index]);
      effects.push_back(GenericMemoryEffect(initSet.count(index) != 0 ? "write" : "read",
                                 value, false));
    }
    operation.effects = JoinDelimited(effects, ", ");
    return;
  }
  if (HasNoMemoryEffect(operation.name)) {
    operation.effects.clear();
    return;
  }
  if (operation.name == "annotation.mark") {
    operation.effects = GenericMemoryEffect("write", "-", false);
  } else if (operation.name == "hivm.hir.debug") {
    operation.effects = GenericMemoryEffect("read", "-", false) + ", " +
                        GenericMemoryEffect("write", "-", false);
  } else if (operation.name == "bufferization.to_tensor" &&
             !operation.operands.empty()) {
    operation.effects = GenericMemoryEffect(
        "read", std::to_string(operation.operands.front()), true);
  } else if ((operation.name == "memref.alloc" ||
              operation.name == "memref_ext.alloc_workspace") &&
             !operation.results.empty()) {
    operation.effects = GenericMemoryEffect(
        "allocate", std::to_string(operation.results.front()), true);
  } else if (operation.name == "hivm.hir.pointer_cast" &&
             !operation.results.empty()) {
    operation.effects = GenericMemoryEffect(
        "allocate", std::to_string(operation.results.front()), false);
  } else if (operation.name == "memref.load" &&
             !operation.operands.empty()) {
    operation.effects = GenericMemoryEffect(
        "read", std::to_string(operation.operands.front()), false);
  } else if (operation.name == "hivm.hir.atomic_cas") {
    std::vector<std::string> effects;
    for (size_t index = 0; index < operation.operands.size(); ++index) {
      if (index < operation.operandTypes.size() &&
          GenericIsMemRefType(operation.operandTypes[index]))
        effects.push_back(GenericMemoryEffect(
            "read", std::to_string(operation.operands[index]), false));
    }
    if (!operation.operands.empty() && !operation.operandTypes.empty() &&
        GenericIsMemRefType(operation.operandTypes.back()))
      effects.push_back(GenericMemoryEffect(
          "write", std::to_string(operation.operands.back()), false));
    operation.effects = JoinDelimited(effects, ", ");
  }
}

} // namespace cvub

#endif
