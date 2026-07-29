#ifndef UB_OVERFLOW_MODEL_CPP_PRE_CV_INLINE_OTF_BROADCAST_HPP
#define UB_OVERFLOW_MODEL_CPP_PRE_CV_INLINE_OTF_BROADCAST_HPP

#include "../ir/post_pipeline_ir_utils.hpp"
#include "canonicalization_hivm_pipeline.hpp"

#include <set>

namespace cvub {

enum class PreCVBroadcastAxisKind { First, Middle, Last };

inline std::optional<MemRefTypeModel>
PreCVInlineBroadcastShapedType(const std::string &type) {
  if (GenericIsTensorType(type))
    return ParseMemRefType(ConvertTensorToMemRefType(type));
  return ParseMemRefType(type);
}

inline PreCVBroadcastAxisKind PreCVGetBroadcastAxisKind(int64_t dimension,
                                                        int64_t rank) {
  if (dimension == rank - 1)
    return PreCVBroadcastAxisKind::Last;
  if (dimension <= 0)
    return PreCVBroadcastAxisKind::First;
  return PreCVBroadcastAxisKind::Middle;
}

inline bool PreCVInlineBroadcastIsAscend950(const GenericModule &module) {
  if (module.operations.empty())
    return false;
  const GenericOperation &root = module.operations.front();
  const std::string target = root.properties + " " + root.attributes;
  return target.find("Ascend910_95") != std::string::npos ||
         target.find("Ascend950PR_") != std::string::npos ||
         target.find("Ascend950DT_") != std::string::npos;
}

inline bool PreCVInlineBroadcastHasPureTensorSemantics(
    const GenericOperation &operation) {
  bool hasTensor = false;
  for (const std::string &type : operation.operandTypes) {
    if (GenericIsMemRefType(type))
      return false;
    hasTensor |= GenericIsTensorType(type);
  }
  return hasTensor;
}

inline bool PreCVInlineBroadcastHasTrait(const std::string &name) {
  // Source of truth: BroadcastableOTF occurrences in HIVMVectorOps.td.
  static const std::set<std::string> names = {
      "hivm.hir.vexp",   "hivm.hir.vabs",  "hivm.hir.vln",
      "hivm.hir.vrelu",  "hivm.hir.vrsqrt", "hivm.hir.vsqrt",
      "hivm.hir.vrec",   "hivm.hir.vnot",  "hivm.hir.vcast",
      "hivm.hir.vadd",   "hivm.hir.vmul",  "hivm.hir.vsub",
      "hivm.hir.vdiv",   "hivm.hir.vmax",  "hivm.hir.vmin",
      "hivm.hir.vor",    "hivm.hir.vand",  "hivm.hir.vshl",
      "hivm.hir.vshr",   "hivm.hir.vsel",  "hivm.hir.visinf",
      "hivm.hir.visnan"};
  return names.count(name) != 0;
}

inline bool PreCVInlineBroadcastIsBinary(const std::string &name) {
  // Source of truth: HIVM_ElementwiseBinaryOp derives
  // ElementwiseNaryOpTrait<2> in HIVMVectorOps.td.
  static const std::set<std::string> names = {
      "hivm.hir.vadd", "hivm.hir.vmul", "hivm.hir.vsub",
      "hivm.hir.vdiv", "hivm.hir.vmax", "hivm.hir.vmin",
      "hivm.hir.vor",  "hivm.hir.vand", "hivm.hir.vshl",
      "hivm.hir.vshr"};
  return names.count(name) != 0;
}

inline bool PreCVInlineBroadcastIsStructured(const std::string &name) {
  return IsHIVMStructuredOp(name);
}

inline std::optional<std::string>
PreCVInlineBroadcastElementType(const std::string &type) {
  const std::optional<MemRefTypeModel> shaped =
      PreCVInlineBroadcastShapedType(type);
  if (!shaped)
    return std::nullopt;
  return shaped->elementType;
}

inline bool PreCVInlineBroadcastValidUser(const GenericModule &module,
                                          const GenericOperation &user,
                                          PreCVBroadcastAxisKind axisKind) {
  static const std::set<std::string> lastAxisWhitelist = {
      "hivm.hir.vadd",  "hivm.hir.vmul",  "hivm.hir.vmax",
      "hivm.hir.vmin",  "hivm.hir.vsub",  "hivm.hir.vdiv",
      "hivm.hir.vand",  "hivm.hir.vor",   "hivm.hir.vnot",
      "hivm.hir.vabs",  "hivm.hir.vln",   "hivm.hir.vrelu",
      "hivm.hir.vexp",  "hivm.hir.vrsqrt", "hivm.hir.vsqrt"};

  if (axisKind != PreCVBroadcastAxisKind::Last)
    return PreCVInlineBroadcastHasTrait(user.name) &&
           PreCVInlineBroadcastIsBinary(user.name) &&
           PreCVInlineBroadcastIsStructured(user.name);

  bool valid = lastAxisWhitelist.count(user.name) != 0;
  if (user.name == "hivm.hir.vshl" || user.name == "hivm.hir.vshr")
    valid = PreCVInlineBroadcastIsAscend950(module);
  if (user.name == "hivm.hir.vabs") {
    if (user.operandTypes.empty())
      throw std::runtime_error(
          "pre-CV InlineOTFBroadcast: malformed VAbs source");
    const std::optional<std::string> element =
        PreCVInlineBroadcastElementType(user.operandTypes.front());
    if (!element)
      throw std::runtime_error(
          "pre-CV InlineOTFBroadcast: unreviewed VAbs source type");
    if (*element == "i16" || *element == "i32")
      valid = false;
  }
  return valid;
}

inline std::vector<int64_t>
PreCVInlineBroadcastArray(const GenericOperation &operation,
                          const std::string &name) {
  std::string value = FindDictionaryValue(operation.properties, name);
  if (value.empty())
    value = FindDictionaryValue(operation.attributes, name);
  const size_t colon = value.find(':');
  const size_t close = value.rfind('>');
  if (close == std::string::npos)
    throw std::runtime_error(
        "pre-CV InlineOTFBroadcast: malformed dense i64 array");
  if (colon == std::string::npos)
    return {};
  std::vector<int64_t> result;
  for (const std::string &entry :
       splitTopLevel(value.substr(colon + 1, close - colon - 1)))
    result.push_back(std::stoll(trim(entry)));
  return result;
}

inline std::string
PreCVInlineBroadcastArrayText(const std::set<int64_t> &values) {
  std::ostringstream output;
  output << "array<i64:";
  size_t index = 0;
  for (int64_t value : values)
    output << (index++ == 0 ? " " : ", ") << value;
  return output.str() + ">";
}

inline void PreCVInlineBroadcastUpdateUser(GenericOperation &user,
                                           int destination, int source,
                                           const std::string &sourceType,
                                           int64_t dimension) {
  std::set<int64_t> dimensions = {dimension};
  for (int64_t existing : PreCVInlineBroadcastArray(user, "broadcast"))
    dimensions.insert(existing);
  SetCanonicalizationProperty(user, "broadcast",
                              PreCVInlineBroadcastArrayText(dimensions));

  const std::vector<size_t> initIndices = DpsInitOperandIndices(
      user.name, user.operands.size(), user.properties);
  const std::set<size_t> initSet(initIndices.begin(), initIndices.end());
  for (size_t operand = 0; operand < user.operands.size(); ++operand) {
    if (initSet.count(operand) != 0 || user.operands[operand] != destination)
      continue;
    user.operands[operand] = source;
    if (operand >= user.operandTypes.size())
      throw std::runtime_error(
          "pre-CV InlineOTFBroadcast: missing operand type");
    user.operandTypes[operand] = sourceType;
  }
  ApplyOperationSemantics(user);
}

inline GenericModule RunPreCVInlineOTFBroadcast(GenericModule module) {
  ApplyOperationSemanticsToAll(module.operations);
  const GenericModuleAnalysisIndexes analysis(
      module, kGenericAnalysisUsers | kGenericAnalysisValueTypes);

  for (const GenericOperation &snapshot : module.operations) {
    if (snapshot.name != "hivm.hir.vbrc")
      continue;
    if (!PreCVInlineBroadcastHasPureTensorSemantics(snapshot))
      continue;
    if (snapshot.operands.size() < 2 || snapshot.results.empty() ||
        snapshot.resultTypes.empty())
      throw std::runtime_error(
          "pre-CV InlineOTFBroadcast: malformed tensor VBrc");
    const int source = snapshot.operands.front();
    const int destination = snapshot.results.front();
    const std::string *sourceType = analysis.valueType(source);
    if (!sourceType || !GenericIsTensorType(*sourceType))
      continue;
    const std::optional<MemRefTypeModel> destinationType =
        PreCVInlineBroadcastShapedType(snapshot.resultTypes.front());
    if (!destinationType)
      throw std::runtime_error(
          "pre-CV InlineOTFBroadcast: unreviewed destination type");
    const std::vector<int64_t> dimensions =
        PreCVInlineBroadcastArray(snapshot, "broadcast_dims");
    if (dimensions.empty())
      throw std::runtime_error(
          "pre-CV InlineOTFBroadcast: tensor VBrc has empty broadcast dims");
    if (destinationType->elementType == "i64" ||
        destinationType->elementType == "i1" || dimensions.size() != 1)
      continue;
    const PreCVBroadcastAxisKind axisKind = PreCVGetBroadcastAxisKind(
        dimensions.front(), static_cast<int64_t>(destinationType->shape.size()));

    bool rewritten = false;
    const std::vector<int> users = analysis.users(destination);
    for (int userId : users) {
      GenericOperation &user =
          module.operations.at(static_cast<size_t>(userId));
      if (!PreCVInlineBroadcastValidUser(module, user, axisKind))
        continue;
      rewritten = true;
      PreCVInlineBroadcastUpdateUser(user, destination, source, *sourceType,
                                     dimensions.front());
    }
    (void)rewritten;
  }

  PipelineAnalysisContext activeUses(module, kGenericAnalysisUsers);
  while (EliminateCanonicalizationDeadCode(module, activeUses)) {
  }
  ApplyOperationSemanticsToAll(module.operations);
  module = CompactGenericModule(std::move(module));
  ValidateGenericModule(module);
  return module;
}

} // namespace cvub

#endif
