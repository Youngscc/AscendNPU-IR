#include "../src/passes/inline_load_copy.hpp"

#include <iostream>
#include <stdexcept>

namespace {

cvub::GenericOperation operation(int id, int parent, int block, int ordinal,
                                  const std::string &name,
                                  std::vector<int> operands = {}) {
  cvub::GenericOperation result;
  result.id = id;
  result.parentId = parent;
  result.blockId = block;
  result.ordinal = ordinal;
  result.name = name;
  result.operands = std::move(operands);
  return result;
}

void addAccess(cvub::PostBufferizationRewriteState &postBufferization, int operationId, int operand,
               const std::string &buffer) {
  postBufferization.bufferized.accesses.push_back({operationId, operand, buffer});
}

cvub::AfterAllocExtraBufferState makeCase(bool copyBeforeLoad, bool middleRead,
                            bool sourceWrite, bool differentBlock) {
  cvub::AfterAllocExtraBufferState afterAllocExtraBuffer;
  cvub::PostBufferizationRewriteState &postBufferization = afterAllocExtraBuffer.postBufferization;
  postBufferization.bufferized.logicalModule.operations.push_back(
      operation(0, -1, -1, 0, "builtin.module"));
  cvub::GenericOperation function = operation(1, 0, 0, 0, "func.func");
  function.properties = "{sym_name = \"kernel\"}";
  postBufferization.bufferized.logicalModule.operations.push_back(function);

  const int loadOrdinal = copyBeforeLoad ? 4 : 1;
  cvub::GenericOperation load =
      operation(2, 1, 0, loadOrdinal, "hivm.hir.load", {10, 20});
  load.operandTypes = {"memref<8xf32>", "memref<8xf32>"};
  postBufferization.bufferized.logicalModule.operations.push_back(load);
  cvub::GenericOperation toTensor =
      operation(3, 1, 0, 2, "bufferization.to_tensor", {20});
  toTensor.results = {21};
  toTensor.operandTypes = {"memref<8xf32>"};
  toTensor.resultTypes = {"tensor<8xf32>"};
  postBufferization.bufferized.logicalModule.operations.push_back(toTensor);

  int copyId = 4;
  if (middleRead) {
    postBufferization.bufferized.logicalModule.operations.push_back(
        operation(copyId++, 1, 0, 3, "memref.load", {20}));
  }
  if (sourceWrite) {
    postBufferization.bufferized.logicalModule.operations.push_back(
        operation(copyId++, 1, 0, 3, "memref.store", {30, 10}));
  }
  cvub::GenericOperation copy = operation(
      copyId, 1, differentBlock ? 1 : 0, copyBeforeLoad ? 1 : 5,
      "tensor.insert_slice", {21, 40});
  copy.operandTypes = {"tensor<8xf32>", "tensor<8xf32>"};
  postBufferization.bufferized.logicalModule.operations.push_back(copy);

  addAccess(postBufferization, load.id, 0, "arg:1:0");
  addAccess(postBufferization, load.id, 1, "local:0");
  addAccess(postBufferization, toTensor.id, 0, "local:0");
  if (middleRead)
    addAccess(postBufferization, 4, 0, "local:0");
  if (sourceWrite)
    addAccess(postBufferization, middleRead ? 5 : 4, 1, "arg:1:0");
  addAccess(postBufferization, copy.id, 0, "local:0");
  addAccess(postBufferization, copy.id, 1, "local:1");
  postBufferization.singlePoint.bufferMapping = {
      {"local:0", "local:0"}, {"local:1", "local:1"}};
  afterAllocExtraBuffer.buffers = {
      {"base:0", "base:0", "", "memref<8xf32>",
       cvub::AddressSpace::UB, 256, false, {}},
      {"base:1", "base:1", "", "memref<8xf32>",
       cvub::AddressSpace::UB, 256, false, {}},
  };
  return afterAllocExtraBuffer;
}

void expectRewrites(const cvub::AfterAllocExtraBufferState &afterAllocExtraBuffer, size_t expected,
                    const std::string &label) {
  const cvub::InlineLoadCopyResult result = cvub::ModelInlineLoadCopy(afterAllocExtraBuffer);
  if (result.rewrites.size() != expected ||
      result.erasedBuffers.size() != expected)
    throw std::runtime_error(label + " rewrite mismatch");
}

} // namespace

int main() {
  try {
    expectRewrites(makeCase(false, false, false, false), 1, "positive");
    expectRewrites(makeCase(true, false, false, false), 0,
                   "copy-before-load");
    expectRewrites(makeCase(false, true, false, false), 0, "middle-read");
    expectRewrites(makeCase(false, false, true, false), 0, "source-write");
    expectRewrites(makeCase(false, false, false, true), 0, "cross-block");
    std::cout << "INLINE_LOAD_COPY_TESTS=PASS cases=5\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "[FAIL] " << error.what() << '\n';
    return 1;
  }
}
