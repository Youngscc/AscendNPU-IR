#include "../src/ir/generic_rewriter.hpp"
#include "../src/passes/auto_blockify_parallel_loop.hpp"
#include "../src/passes/canonicalization_hivm_pipeline.hpp"
#include "../src/passes/module_extended_canonicalizer.hpp"
#include "../src/passes/outer_extended_canonicalizer.hpp"
#include "../src/passes/pre_cv_mark_multi_buffer.hpp"
#include "../src/passes/pre_cv_cse.hpp"
#include "../src/passes/scf_for_loop_canonicalization.hpp"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

struct Options {
  bool applyModel = false;
  bool applyOuterCanonicalizer = false;
  bool applyArithToAffine = false;
  bool applyCanonicalizeIterArg = false;
  bool applyModuleCanonicalizer = false;
  bool applySCFForLoopCanonicalization = false;
  bool applyCSE = false;
  cvub::AutoBlockifyPrefixOptions autoBlockify;
  cvub::PreCVMarkMultiBufferOptions markMultiBuffer;
  std::filesystem::path input;
};

cvub::MultiBufferStrategy ParseStrategy(const std::string &value) {
  if (value == "no-limit")
    return cvub::MultiBufferStrategy::NoLimit;
  if (value == "only-cube")
    return cvub::MultiBufferStrategy::OnlyCube;
  if (value == "only-vector")
    return cvub::MultiBufferStrategy::OnlyVector;
  if (value == "no-l0c")
    return cvub::MultiBufferStrategy::CubeNoL0C;
  throw std::runtime_error("unknown multi-buffer strategy: " + value);
}

Options ParseOptions(int argc, char **argv) {
  Options options;
  for (int index = 1; index < argc; ++index) {
    const std::string argument = argv[index];
    if (argument == "--apply-model")
      options.applyModel = true;
    else if (argument == "--apply-outer-canonicalizer")
      options.applyOuterCanonicalizer = true;
    else if (argument == "--apply-arith-to-affine")
      options.applyArithToAffine = true;
    else if (argument == "--apply-canonicalize-iter-arg")
      options.applyCanonicalizeIterArg = true;
    else if (argument == "--apply-module-canonicalizer")
      options.applyModuleCanonicalizer = true;
    else if (argument == "--apply-scf-for-loop-canonicalization")
      options.applySCFForLoopCanonicalization = true;
    else if (argument == "--apply-cse")
      options.applyCSE = true;
    else if (argument == "--enable-auto-multi-buffer")
      options.markMultiBuffer.enableAuto = true;
    else if (argument == "--disable-auto-cv-workspace-manage")
      options.markMultiBuffer.disableAutoCVWorkSpaceManage = true;
    else if (argument == "--local-only")
      options.markMultiBuffer.limitAutoMultiBufferOnlyForLocalBuffer = true;
    else if (argument == "--disable-auto-blockify-loop")
      options.autoBlockify.enableAutoBlockifyLoop = false;
    else if (argument.rfind("--local-strategy=", 0) == 0)
      options.markMultiBuffer.limitAutoMultiBufferOfLocalBuffer =
          ParseStrategy(argument.substr(std::string("--local-strategy=").size()));
    else if (argument.rfind("--mix-strategy=", 0) == 0)
      options.markMultiBuffer.limitMixAutoMultiBufferBuffer =
          ParseStrategy(argument.substr(std::string("--mix-strategy=").size()));
    else if (argument.rfind("--workspace-multi-buffer=", 0) == 0)
      options.markMultiBuffer.workspaceMultiBufferNum =
          static_cast<unsigned>(std::stoul(argument.substr(
              std::string("--workspace-multi-buffer=").size())));
    else if (argument == "--help" || argument == "-h") {
      std::cout << "usage: pre_cv_prefix_model_runner [options] INPUT.mlir\n";
      std::exit(0);
    } else if (!options.input.empty())
      throw std::runtime_error("multiple input paths were provided");
    else
      options.input = argument;
  }
  if (options.input.empty())
    throw std::runtime_error("an input MLIR path is required");
  return options;
}

} // namespace

int main(int argc, char **argv) {
  try {
    const Options options = ParseOptions(argc, argv);
    cvub::GenericModule module = cvub::ParseGenericIR(options.input, false);
    // Parsed GenericOperation records intentionally start with conservative
    // placeholder semantics.  Every standalone pass entry must see the same
    // reviewed effects/types as a cumulative lightweight prefix; otherwise
    // CanonicalizeIterArg's internal CSE can incorrectly merge reads or
    // allocations only on the single-pass verification path.
    cvub::ApplyOperationSemanticsToAll(module.operations);
    if (options.applyModel) {
      module = cvub::RunAutoBlockifyPrefixStage(
          std::move(module), options.autoBlockify);
      module = cvub::RunPreCVMarkMultiBuffer(
          std::move(module), options.markMultiBuffer);
    }
    if (options.applyOuterCanonicalizer)
      module = cvub::RunOuterExtendedCanonicalizer(std::move(module));
    if (options.applyArithToAffine)
      module = cvub::RunArithToAffineConversionPass(std::move(module));
    if (options.applyCanonicalizeIterArg)
      module = cvub::RunCanonicalizationHIVMAfterArithToAffine(
          std::move(module));
    if (options.applyModuleCanonicalizer)
      module = cvub::RunModuleExtendedCanonicalizer(std::move(module));
    if (options.applySCFForLoopCanonicalization)
      module = cvub::RunSCFForLoopCanonicalization(std::move(module));
    if (options.applyCSE)
      module = cvub::RunPreCVCSE(std::move(module));
    if (!options.applyModel && !options.applyOuterCanonicalizer &&
        !options.applyArithToAffine && !options.applyCanonicalizeIterArg &&
        !options.applyModuleCanonicalizer &&
        !options.applySCFForLoopCanonicalization && !options.applyCSE) {
      cvub::ApplyOperationSemanticsToAll(module.operations);
      module = cvub::CompactGenericModule(std::move(module));
    }
    cvub::ApplyOperationSemanticsToAll(module.operations);
    std::cout << cvub::SerializeGenericModule(
        module, "PRE_CV_PREFIX_CANONICAL_STRUCTURAL_IR");
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "pre_cv_prefix_model_runner: " << error.what() << '\n';
    return 1;
  }
}
