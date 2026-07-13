#include "../src/suffix/mark_multi_buffer.hpp"

#include <iostream>
#include <stdexcept>

namespace {

cvub::AfterInlineLoadCopyState makeCopyCase(const std::string &name,
                                cvub::AddressSpace space, bool mix,
                                const std::string &loopName = "scf.for",
                                bool mappedLoop = false,
                                bool existingMark = false) {
  cvub::AfterInlineLoadCopyState afterInlineLoadCopy;
  auto &module = afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.bufferized.logicalModule;
  module.operations.resize(existingMark ? 6 : 5);
  module.operations[0].id = 0;
  module.operations[0].name = "builtin.module";
  module.operations[0].parentId = -1;
  module.operations[1].id = 1;
  module.operations[1].name = "func.func";
  module.operations[1].parentId = 0;
  module.operations[1].properties = "{sym_name = \"kernel\"}";
  module.operations[1].attributes =
      mix ? "{hivm.func_core_type = #hivm.func_core_type<MIX>}"
          : "{hivm.func_core_type = #hivm.func_core_type<AIV>}";
  module.operations[2].id = 2;
  module.operations[2].name = loopName;
  module.operations[2].parentId = 1;
  if (mappedLoop)
    module.operations[2].attributes = "{map_for_to_forall}";
  module.operations[3].id = 3;
  module.operations[3].name = "memref.alloc";
  module.operations[3].parentId = 2;
  module.operations[3].results = {30};
  module.operations[4].id = 4;
  module.operations[4].name = name;
  module.operations[4].parentId = 2;
  module.operations[4].operands = {40, 30};
  module.operations[4].operandTypes = {"memref<8xf32>", "memref<8xf32>"};
  module.operations[4].properties = "{operandSegmentSizes = array<i32: 1, 1>}";
  cvub::ApplyOperationSemantics(module.operations[4]);
  afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.bufferized.accesses = {{4, 0, "arg:1:0"}, {4, 1, "local:0"}};
  afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.singlePoint.allocations.push_back(
      {"memref<8xf32>", "", 0, "result:3:0", {}});
  afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.singlePoint.bufferMapping["local:0"] = "local:0";
  afterInlineLoadCopy.afterAllocExtraBuffer.buffers = {{"base:0", "base:0", "result:3:0", "memref<8xf32>",
                    space, 256, false, {8}}};
  afterInlineLoadCopy.buffers = afterInlineLoadCopy.afterAllocExtraBuffer.buffers;
  if (existingMark) {
    module.operations[5].id = 5;
    module.operations[5].name = "annotation.mark";
    module.operations[5].parentId = 2;
    module.operations[5].operands = {30};
    module.operations[5].attributes = "{hivm.multi_buffer = 4 : i32}";
    afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.bufferized.accesses.push_back({5, 0, "local:0"});
  }
  return afterInlineLoadCopy;
}

cvub::MarkMultiBufferOptions options(bool enabled,
                                     cvub::MultiBufferStrategy local,
                                     cvub::MultiBufferStrategy mix) {
  cvub::MarkMultiBufferOptions result;
  result.enableAuto = enabled;
  result.limitAutoMultiBufferOfLocalBuffer = local;
  result.limitMixAutoMultiBufferBuffer = mix;
  return result;
}

void requireCount(const cvub::AfterInlineLoadCopyState &afterInlineLoadCopy,
                  const cvub::MarkMultiBufferOptions &opts, unsigned expected,
                  const std::string &label) {
  const cvub::MarkMultiBufferResult result =
      cvub::ModelMarkMultiBuffer(afterInlineLoadCopy, opts);
  const auto found = result.buffer2MultiNum.find("base:0");
  const unsigned actual =
      found == result.buffer2MultiNum.end() ? 1 : found->second;
  if (actual != expected)
    throw std::runtime_error(label + ": expected " +
                             std::to_string(expected) + ", got " +
                             std::to_string(actual));
}

cvub::AfterInlineLoadCopyState makePreloadCase() {
  cvub::AfterInlineLoadCopyState afterInlineLoadCopy;
  auto &module = afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.bufferized.logicalModule;
  module.operations.resize(7);
  module.operations[0].id = 0;
  module.operations[0].name = "builtin.module";
  module.operations[0].parentId = -1;
  module.operations[1].id = 1;
  module.operations[1].name = "func.func";
  module.operations[1].parentId = 0;
  module.operations[1].properties = "{sym_name = \"kernel\"}";
  module.operations[2].id = 2;
  module.operations[2].name = "scope.scope";
  module.operations[2].parentId = 1;
  module.operations[2].results = {20};
  module.operations[2].attributes =
      "{hivm.preload_num = 1 : i64, "
      "hivm.loop_core_type = #hivm.tcore_type<VECTOR>}";
  module.operations[2].regions = {0};
  module.operations[3].id = 3;
  module.operations[3].name = "memref.alloc";
  module.operations[3].parentId = 2;
  module.operations[3].results = {30};
  module.operations[4].id = 4;
  module.operations[4].name = "scope.return";
  module.operations[4].parentId = 2;
  module.operations[4].operands = {30};
  module.operations[5].id = 5;
  module.operations[5].name = "scope.scope";
  module.operations[5].parentId = 1;
  module.operations[5].operands = {20};
  module.operations[6].id = 6;
  module.operations[6].name = "func.return";
  module.operations[6].parentId = 1;
  module.regions = {{0, 2, 0, {0}}};
  module.blocks = {{0, 0, 0, {}, {}, {3, 4}}};
  afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.bufferized.values.push_back(
      {30, "local:0", "memref<8xf32>", "result:3:0"});
  afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.singlePoint.allocations.push_back(
      {"memref<8xf32>", "", 0, "result:3:0", {}});
  afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.singlePoint.bufferMapping["local:0"] = "local:0";
  afterInlineLoadCopy.afterAllocExtraBuffer.buffers = {{"base:0", "base:0", "result:3:0", "memref<8xf32>",
                    cvub::AddressSpace::UB, 256, false, {8}}};
  afterInlineLoadCopy.buffers = afterInlineLoadCopy.afterAllocExtraBuffer.buffers;
  return afterInlineLoadCopy;
}

} // namespace

int main() {
  try {
    using S = cvub::MultiBufferStrategy;
    requireCount(makeCopyCase("hivm.hir.load", cvub::AddressSpace::UB, false),
                 options(false, S::CubeNoL0C, S::OnlyCube), 1, "disabled");
    requireCount(makeCopyCase("hivm.hir.load", cvub::AddressSpace::UB, false),
                 options(true, S::CubeNoL0C, S::OnlyCube), 2, "non-mix load");
    requireCount(makeCopyCase("hivm.hir.load", cvub::AddressSpace::UB, true),
                 options(true, S::CubeNoL0C, S::OnlyCube), 1, "mix cube");
    requireCount(makeCopyCase("hivm.hir.load", cvub::AddressSpace::UB, true),
                 options(true, S::CubeNoL0C, S::OnlyVector), 2,
                 "mix vector");
    requireCount(makeCopyCase("hivm.hir.load", cvub::AddressSpace::L1, true),
                 options(true, S::CubeNoL0C, S::OnlyCube), 2, "ND2NZ fold");
    requireCount(makeCopyCase("hivm.hir.fixpipe", cvub::AddressSpace::L0C,
                              false),
                 options(true, S::CubeNoL0C, S::NoLimit), 1,
                 "no-l0c fixpipe");
    requireCount(makeCopyCase("hivm.hir.fixpipe", cvub::AddressSpace::L0C,
                              false),
                 options(true, S::NoLimit, S::NoLimit), 2, "fixpipe");
    requireCount(makeCopyCase("hivm.hir.load", cvub::AddressSpace::UB, false,
                              "scf.for", true),
                 options(true, S::NoLimit, S::NoLimit), 1, "mapped loop");
    requireCount(makeCopyCase("hivm.hir.load", cvub::AddressSpace::UB, false,
                              "scf.while"),
                 options(true, S::NoLimit, S::NoLimit), 1, "non-for loop");
    requireCount(makeCopyCase("hivm.hir.load", cvub::AddressSpace::UB, false,
                              "scf.for", false, true),
                 options(true, S::NoLimit, S::NoLimit), 4, "existing mark");
    const cvub::MarkMultiBufferResult preload = cvub::ModelMarkMultiBuffer(
        makePreloadCase(), options(true, S::CubeNoL0C, S::OnlyCube));
    if (preload.buffer2MultiNum.at("base:0") != 4 ||
        preload.preloadLocalBuffers.count("base:0") == 0)
      throw std::runtime_error("scope preload mark mismatch");
    std::cout << "MARK_MULTI_BUFFER_TESTS=PASS cases=11\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "[FAIL] " << error.what() << '\n';
    return 1;
  }
}
