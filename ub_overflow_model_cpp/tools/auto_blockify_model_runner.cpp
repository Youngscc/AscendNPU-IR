#include "../src/ir/generic_rewriter.hpp"
#include "../src/passes/auto_blockify_parallel_loop.hpp"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

struct Options {
  bool applyModel = false;
  std::filesystem::path input;
};

Options ParseOptions(int argc, char **argv) {
  Options options;
  for (int index = 1; index < argc; ++index) {
    const std::string argument = argv[index];
    if (argument == "--apply-model") {
      options.applyModel = true;
      continue;
    }
    if (argument == "--help" || argument == "-h") {
      std::cout
          << "usage: auto_blockify_model_runner [--apply-model] INPUT.mlir\n"
          << "\n"
          << "Parse Generic MLIR, optionally apply the replicated "
             "AutoBlockifyParallelLoop transform, then emit a compact "
             "structural snapshot.\n";
      std::exit(0);
    }
    if (!options.input.empty())
      throw std::runtime_error("multiple input paths were provided");
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
    if (options.applyModel)
      module = cvub::RunAutoBlockifyParallelLoop(std::move(module));
    else
      module = cvub::CompactGenericModule(std::move(module));
    std::cout << cvub::SerializeGenericModule(
        module, "AUTO_BLOCKIFY_CANONICAL_STRUCTURAL_IR");
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "auto_blockify_model_runner: " << error.what() << '\n';
    return 1;
  }
}
