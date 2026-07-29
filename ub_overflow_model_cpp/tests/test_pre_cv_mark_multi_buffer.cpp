#include "../src/passes/pre_cv_mark_multi_buffer.hpp"

#include <iostream>
#include <stdexcept>

namespace {

void Check(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}

cvub::GenericOperation *FindFunction(cvub::GenericModule &module,
                                     const std::string &name) {
  for (cvub::GenericOperation &operation : module.operations)
    if (operation.name == "func.func" &&
        cvub::FindDictionaryValue(operation.properties, "sym_name") ==
            '"' + name + '"')
      return &operation;
  return nullptr;
}

std::vector<const cvub::GenericOperation *>
MultiBufferMarks(const cvub::GenericModule &module) {
  std::vector<const cvub::GenericOperation *> result;
  for (const cvub::GenericOperation &operation : module.operations)
    if (operation.name == "annotation.mark" &&
        !cvub::FindDictionaryValue(operation.attributes,
                                   "hivm.multi_buffer")
             .empty())
      result.push_back(&operation);
  return result;
}

const cvub::GenericOperation *Definition(const cvub::GenericModule &module,
                                         int value) {
  for (const cvub::GenericOperation &operation : module.operations)
    if (std::find(operation.results.begin(), operation.results.end(), value) !=
        operation.results.end())
      return &operation;
  return nullptr;
}

std::set<cvub::AddressSpace>
MarkedAddressSpaces(const cvub::GenericModule &module) {
  std::set<cvub::AddressSpace> result;
  for (const cvub::GenericOperation *mark : MultiBufferMarks(module)) {
    if (mark->operands.empty())
      continue;
    const cvub::GenericOperation *allocation =
        Definition(module, mark->operands.front());
    if (!allocation || allocation->resultTypes.empty())
      continue;
    const std::optional<cvub::AddressSpace> address =
        cvub::PreCVExplicitAddressSpace(allocation->resultTypes.front());
    if (address)
      result.insert(*address);
  }
  return result;
}

cvub::PreCVMarkMultiBufferOptions EnabledOptions() {
  cvub::PreCVMarkMultiBufferOptions options;
  options.enableAuto = true;
  options.limitAutoMultiBufferOnlyForLocalBuffer = true;
  options.limitAutoMultiBufferOfLocalBuffer =
      cvub::MultiBufferStrategy::NoLimit;
  options.limitMixAutoMultiBufferBuffer =
      cvub::MultiBufferStrategy::NoLimit;
  return options;
}

void CheckMarkPlacement(const cvub::GenericModule &module) {
  for (const cvub::GenericOperation *mark : MultiBufferMarks(module)) {
    Check(!mark->operands.empty(), "multi-buffer mark must have a source");
    const cvub::GenericOperation *allocation =
        Definition(module, mark->operands.front());
    Check(allocation != nullptr, "multi-buffer mark source must have a def");
    const cvub::GenericBlock &block =
        module.blocks.at(static_cast<size_t>(allocation->blockId));
    const auto allocationPosition =
        std::find(block.operations.begin(), block.operations.end(),
                  allocation->id);
    Check(allocationPosition != block.operations.end() &&
              std::next(allocationPosition) != block.operations.end() &&
              *std::next(allocationPosition) == mark->id,
          "native mark must be inserted immediately after its allocation");
  }
}

} // namespace

int main() {
  const cvub::GenericModule before = cvub::ParseGenericIR(
      "ub_overflow_model_cpp/tests/fixtures/pre_cv_mark_multi_buffer.mlir",
      false);

  cvub::PreCVMarkMultiBufferOptions disabled;
  Check(cvub::SerializeGenericModule(
            cvub::RunPreCVMarkMultiBuffer(before, disabled)) ==
            cvub::SerializeGenericModule(before),
        "enable-auto=false must be an exact identity");
  disabled.enableAuto = true;
  disabled.disableAutoCVWorkSpaceManage = true;
  Check(cvub::SerializeGenericModule(
            cvub::RunPreCVMarkMultiBuffer(before, disabled)) ==
            cvub::SerializeGenericModule(before),
        "workspace-manage-off must omit the complete pre-CV pass");

  const cvub::GenericModule enabled =
      cvub::RunPreCVMarkMultiBuffer(before, EnabledOptions());
  const std::vector<const cvub::GenericOperation *> enabledMarks =
      MultiBufferMarks(enabled);
  Check(enabledMarks.size() == 3,
        "load, fixpipe and preload scope must each mark one allocation");
  size_t localMarks = 0;
  size_t preloadMarks = 0;
  for (const cvub::GenericOperation *mark : enabledMarks) {
    const std::string count = cvub::FindDictionaryValue(
        mark->attributes, "hivm.multi_buffer");
    localMarks += cvub::trim(count).rfind("2", 0) == 0;
    preloadMarks +=
        !cvub::FindDictionaryValue(mark->attributes,
                                   "hivm.preload_local_buffer")
             .empty();
  }
  Check(localMarks == 2 && preloadMarks == 1,
        "local marks must use two buffers and scope preload must use four");
  CheckMarkPlacement(enabled);

  cvub::PreCVMarkMultiBufferOptions noL0C = EnabledOptions();
  noL0C.limitAutoMultiBufferOfLocalBuffer =
      cvub::MultiBufferStrategy::CubeNoL0C;
  const cvub::GenericModule withoutL0C =
      cvub::RunPreCVMarkMultiBuffer(before, noL0C);
  Check(MultiBufferMarks(withoutL0C).size() == 2 &&
            MarkedAddressSpaces(withoutL0C).count(cvub::AddressSpace::L0C) == 0,
        "no-l0c must omit Fixpipe while retaining Load and scope preload");

  cvub::GenericModule mix = before;
  cvub::GenericOperation *localFunction = FindFunction(mix, "local_buffer");
  Check(localFunction != nullptr, "local fixture function is missing");
  std::string attributes = localFunction->attributes;
  Check(!attributes.empty() && attributes.back() == '}',
        "fixture function attributes must be a dictionary");
  attributes.pop_back();
  attributes += ", hivm.part_of_mix}";
  localFunction->attributes = attributes;

  cvub::PreCVMarkMultiBufferOptions onlyCube = EnabledOptions();
  onlyCube.limitMixAutoMultiBufferBuffer =
      cvub::MultiBufferStrategy::OnlyCube;
  const cvub::GenericModule cube =
      cvub::RunPreCVMarkMultiBuffer(mix, onlyCube);
  const std::set<cvub::AddressSpace> cubeSpaces = MarkedAddressSpaces(cube);
  Check(cubeSpaces.count(cvub::AddressSpace::L0C) == 1 &&
            cubeSpaces.count(cvub::AddressSpace::UB) == 1,
        "MIX only-cube must retain Fixpipe and independent scope preload");

  cvub::PreCVMarkMultiBufferOptions onlyVector = EnabledOptions();
  onlyVector.limitMixAutoMultiBufferBuffer =
      cvub::MultiBufferStrategy::OnlyVector;
  const cvub::GenericModule vector =
      cvub::RunPreCVMarkMultiBuffer(mix, onlyVector);
  const std::set<cvub::AddressSpace> vectorSpaces =
      MarkedAddressSpaces(vector);
  Check(vectorSpaces.count(cvub::AddressSpace::L0C) == 0 &&
            vectorSpaces.count(cvub::AddressSpace::UB) == 1,
        "MIX only-vector must retain Load and suppress cube patterns");

  const std::string rerun = cvub::SerializeGenericModule(
      cvub::RunPreCVMarkMultiBuffer(enabled, EnabledOptions()));
  const std::string enabledText = cvub::SerializeGenericModule(enabled);
  Check(rerun == enabledText,
        "existing valid marks must be recognized and skipped");

  cvub::GenericModule invalid = enabled;
  for (cvub::GenericOperation &operation : invalid.operations)
    if (operation.name == "annotation.mark" &&
        !cvub::FindDictionaryValue(operation.attributes,
                                   "hivm.multi_buffer")
             .empty()) {
      operation.attributes =
          "{effects = [\"write\"], hivm.multi_buffer = 0 : i32}";
      break;
    }
  bool rejected = false;
  try {
    (void)cvub::RunPreCVMarkMultiBuffer(invalid, EnabledOptions());
  } catch (const std::runtime_error &) {
    rejected = true;
  }
  Check(rejected, "illegal existing multi-buffer marks must fail fast");

  std::cout << "[PASS] pre-CV MarkMultiBuffer pattern and option parity\n";
  return 0;
}
