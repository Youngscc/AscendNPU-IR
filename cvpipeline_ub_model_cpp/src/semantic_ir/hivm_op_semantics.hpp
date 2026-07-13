#ifndef CVPIPELINE_UB_MODEL_CPP_HIVM_OP_SEMANTICS_HPP
#define CVPIPELINE_UB_MODEL_CPP_HIVM_OP_SEMANTICS_HPP

#include <set>
#include <string>

namespace cvub {

enum class HIVMPipe { Unassigned, MTE2, MTE3, Other };

// Source of truth: the DestinationStyleOpInterface/HIVMStructuredOp traits in
// the generated declarations for HIVMOps, HIVMDMAOps, HIVMMacroOps and
// HIVMVectorOps. Keeping the registry explicit lets the lightweight model
// reject newly introduced operations until their UB semantics are reviewed.
inline bool IsDestinationStyleOp(const std::string &opName) {
  static const std::set<std::string> operations = {
      "hivm.hir.Conv1dL1",       "hivm.hir.Conv2dL1",
      "hivm.hir.Conv3dL1",       "hivm.hir.atomic_rmw",
      "hivm.hir.batchMmadL1",    "hivm.hir.copy",
      "hivm.hir.custom",         "hivm.hir.fixpipe",
      "hivm.hir.gather_load",    "hivm.hir.indirect_store",
      "hivm.hir.load",           "hivm.hir.matmul",
      "hivm.hir.mix_group_matmul", "hivm.hir.mix_matmul",
      "hivm.hir.mmadL1",         "hivm.hir.nd2nz",
      "hivm.hir.nz2nd",          "hivm.hir.scatter_store",
      "hivm.hir.store",          "hivm.hir.vabs",
      "hivm.hir.vadd",           "hivm.hir.vand",
      "hivm.hir.varange",        "hivm.hir.vbrc",
      "hivm.hir.vcast",          "hivm.hir.vcmp",
      "hivm.hir.vconcat",        "hivm.hir.vcos",
      "hivm.hir.vcumprod",       "hivm.hir.vcumsum",
      "hivm.hir.vdeinterleave",  "hivm.hir.vdiv",
      "hivm.hir.verf",           "hivm.hir.vexp",
      "hivm.hir.vflip",          "hivm.hir.vgather",
      "hivm.hir.vgathermask",    "hivm.hir.vinterleave",
      "hivm.hir.vln",            "hivm.hir.vmax",
      "hivm.hir.vmin",           "hivm.hir.vmod",
      "hivm.hir.vmul",           "hivm.hir.vmulext",
      "hivm.hir.vmulextended",   "hivm.hir.vmulextui",
      "hivm.hir.vnot",           "hivm.hir.vor",
      "hivm.hir.vpad",           "hivm.hir.vpow",
      "hivm.hir.vrec",           "hivm.hir.vreduce",
      "hivm.hir.vrelu",          "hivm.hir.vrsqrt",
      "hivm.hir.vsel",           "hivm.hir.vshl",
      "hivm.hir.vshr",           "hivm.hir.vsin",
      "hivm.hir.vsort",          "hivm.hir.vsqrt",
      "hivm.hir.vsub",           "hivm.hir.vtanh",
      "hivm.hir.vtranspose",     "hivm.hir.vxor"};
  return operations.count(opName) != 0;
}

inline bool IsHIVMStructuredOp(const std::string &opName) {
  return IsDestinationStyleOp(opName) && opName != "hivm.hir.gather_load" &&
         opName != "hivm.hir.scatter_store";
}

inline bool IsSinglePipeOp(const std::string &opName) {
  static const std::set<std::string> operations = {
      "hivm.hir.copy",          "hivm.hir.custom",
      "hivm.hir.fixpipe",       "hivm.hir.indirect_store",
      "hivm.hir.load",          "hivm.hir.nd2nz",
      "hivm.hir.nz2nd",         "hivm.hir.store",
      "hivm.hir.vabs",          "hivm.hir.vadd",
      "hivm.hir.vand",          "hivm.hir.varange",
      "hivm.hir.vbrc",          "hivm.hir.vcast",
      "hivm.hir.vcmp",          "hivm.hir.vconcat",
      "hivm.hir.vcos",          "hivm.hir.vcumprod",
      "hivm.hir.vcumsum",       "hivm.hir.vdeinterleave",
      "hivm.hir.vdiv",          "hivm.hir.verf",
      "hivm.hir.vexp",          "hivm.hir.vflip",
      "hivm.hir.vgather",       "hivm.hir.vgathermask",
      "hivm.hir.vinterleave",   "hivm.hir.vln",
      "hivm.hir.vmax",          "hivm.hir.vmin",
      "hivm.hir.vmod",          "hivm.hir.vmul",
      "hivm.hir.vmulext",       "hivm.hir.vmulextended",
      "hivm.hir.vmulextui",     "hivm.hir.vnot",
      "hivm.hir.vor",           "hivm.hir.vpad",
      "hivm.hir.vpow",          "hivm.hir.vrec",
      "hivm.hir.vreduce",       "hivm.hir.vrelu",
      "hivm.hir.vrsqrt",        "hivm.hir.vsel",
      "hivm.hir.vshl",          "hivm.hir.vshr",
      "hivm.hir.vsin",          "hivm.hir.vsort",
      "hivm.hir.vsqrt",         "hivm.hir.vsub",
      "hivm.hir.vtanh",         "hivm.hir.vtranspose",
      "hivm.hir.vxor"};
  return operations.count(opName) != 0;
}

inline bool HasStaticMTE2Pipe(const std::string &opName) {
  static const std::set<std::string> operations = {
      "hivm.hir.load", "hivm.hir.nd2nz"};
  return operations.count(opName) != 0;
}

inline bool HasStaticMTE3Pipe(const std::string &opName) {
  static const std::set<std::string> operations = {
      "hivm.hir.nz2nd", "hivm.hir.store"};
  return operations.count(opName) != 0;
}

inline HIVMPipe GetStaticPipe(const std::string &opName) {
  if (HasStaticMTE2Pipe(opName))
    return HIVMPipe::MTE2;
  if (HasStaticMTE3Pipe(opName))
    return HIVMPipe::MTE3;
  if (IsSinglePipeOp(opName) && opName != "hivm.hir.copy" &&
      opName != "hivm.hir.custom" && opName != "hivm.hir.vbrc")
    return HIVMPipe::Other;
  return HIVMPipe::Unassigned;
}

inline bool HasImplByScalarOpInterface(const std::string &opName) {
  static const std::set<std::string> operations = {
      "hivm.hir.vabs",         "hivm.hir.vadd",
      "hivm.hir.vcmp",         "hivm.hir.vcumprod",
      "hivm.hir.vcumsum",      "hivm.hir.vdeinterleave",
      "hivm.hir.vdiv",         "hivm.hir.vinterleave",
      "hivm.hir.vmax",         "hivm.hir.vmin",
      "hivm.hir.vmod",         "hivm.hir.vmul",
      "hivm.hir.vmulext",      "hivm.hir.vmulextui",
      "hivm.hir.vreduce",      "hivm.hir.vshl",
      "hivm.hir.vshr",         "hivm.hir.vsub"};
  return operations.count(opName) != 0;
}

inline bool HasScalarOnlyHWOperand1(const std::string &opName) {
  static const std::set<std::string> operations = {
      "hivm.hir.vshl", "hivm.hir.vshr"};
  return operations.count(opName) != 0;
}

// Source of truth: ElementwiseNaryOpTrait in the generated HIVM declarations.
inline bool IsElementwiseNaryOp(const std::string &opName) {
  static const std::set<std::string> operations = {
      "hivm.hir.vabs",     "hivm.hir.vadd",    "hivm.hir.vand",
      "hivm.hir.vcast",    "hivm.hir.vcmp",    "hivm.hir.vcos",
      "hivm.hir.vdiv",     "hivm.hir.verf",    "hivm.hir.vexp",
      "hivm.hir.vln",      "hivm.hir.vmax",    "hivm.hir.vmin",
      "hivm.hir.vmod",     "hivm.hir.vmulext", "hivm.hir.vmulextui",
      "hivm.hir.vmul",     "hivm.hir.vnot",    "hivm.hir.vor",
      "hivm.hir.vpow",     "hivm.hir.vrec",    "hivm.hir.vrelu",
      "hivm.hir.vrsqrt",   "hivm.hir.vsel",    "hivm.hir.vshl",
      "hivm.hir.vshr",     "hivm.hir.vsin",    "hivm.hir.vsqrt",
      "hivm.hir.vsub",     "hivm.hir.vtanh",   "hivm.hir.vxor"};
  return operations.count(opName) != 0;
}

// Source of truth: SameOperandsElementType in the generated declarations.
inline bool HasSameOperandsElementType(const std::string &opName) {
  static const std::set<std::string> operations = {
      "hivm.hir.vabs",          "hivm.hir.vadd",
      "hivm.hir.vand",          "hivm.hir.vbrc",
      "hivm.hir.vconcat",       "hivm.hir.vcos",
      "hivm.hir.vcumprod",      "hivm.hir.vcumsum",
      "hivm.hir.vdeinterleave", "hivm.hir.vdiv",
      "hivm.hir.verf",          "hivm.hir.vexp",
      "hivm.hir.vflip",         "hivm.hir.vinterleave",
      "hivm.hir.vln",           "hivm.hir.vmax",
      "hivm.hir.vmin",          "hivm.hir.vmod",
      "hivm.hir.vmulext",       "hivm.hir.vmulextui",
      "hivm.hir.vmul",          "hivm.hir.vnot",
      "hivm.hir.vor",           "hivm.hir.vpow",
      "hivm.hir.vrec",          "hivm.hir.vrelu",
      "hivm.hir.vrsqrt",        "hivm.hir.vshl",
      "hivm.hir.vshr",          "hivm.hir.vsin",
      "hivm.hir.vsqrt",         "hivm.hir.vsub",
      "hivm.hir.vtanh",         "hivm.hir.vxor"};
  return operations.count(opName) != 0;
}

// Mirrors GetExtraBuffers.cpp. These are precisely the operations for which
// one extra buffer may represent a scalar operand or on-the-fly broadcast.
inline bool ShouldAllocExtraBufferForScalarOrOTFBrc(
    const std::string &opName) {
  static const std::set<std::string> operations = {
      "hivm.hir.vabs",  "hivm.hir.vadd",  "hivm.hir.vand",
      "hivm.hir.vdiv",  "hivm.hir.vexp",  "hivm.hir.vln",
      "hivm.hir.vmax",  "hivm.hir.vmin",  "hivm.hir.vmul",
      "hivm.hir.vnot",  "hivm.hir.vor",   "hivm.hir.vrec",
      "hivm.hir.vrelu", "hivm.hir.vrsqrt", "hivm.hir.vshl",
      "hivm.hir.vshr",  "hivm.hir.vsqrt", "hivm.hir.vsub"};
  return operations.count(opName) != 0;
}

} // namespace cvub

#endif
