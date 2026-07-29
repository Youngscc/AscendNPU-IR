#include "../src/ir/generic_rewriter.hpp"
#include "../src/passes/auto_blockify_parallel_loop.hpp"
#include "../src/passes/pre_cv_mark_multi_buffer.hpp"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

struct Options {
  bool applyModel = false;
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
    if (options.applyModel) {
      module = cvub::RunAutoBlockifyPrefixStage(
          std::move(module), options.autoBlockify);
      module = cvub::RunPreCVMarkMultiBuffer(
          std::move(module), options.markMultiBuffer);
      cvub::ApplyOperationSemanticsToAll(module.operations);
    } else {
      cvub::ApplyOperationSemanticsToAll(module.operations);
      module = cvub::CompactGenericModule(std::move(module));
    }
    std::cout << cvub::SerializeGenericModule(
        module, "PRE_CV_PREFIX_CANONICAL_STRUCTURAL_IR");
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "pre_cv_prefix_model_runner: " << error.what() << '\n';
    return 1;
  }
}
