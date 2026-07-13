#include "../src/suffix/inline_load_copy.hpp"

#include <iostream>
#include <stdexcept>

namespace {

cvub::C1OperationRecord operation(int id, int parent, int block, int ordinal,
                                  const std::string &name,
                                  std::vector<int> operands = {}) {
  cvub::C1OperationRecord result;
  result.id = id;
  result.parentId = parent;
  result.blockId = block;
  result.ordinal = ordinal;
  result.name = name;
  result.operands = std::move(operands);
  return result;
}

void addAccess(cvub::C3SemanticIR &c3, int operationId, int operand,
               const std::string &buffer) {
  c3.bufferized.accesses.push_back({operationId, operand, buffer});
}

cvub::C4SemanticIR makeCase(bool copyBeforeLoad, bool middleRead,
                            bool sourceWrite, bool differentBlock) {
  cvub::C4SemanticIR c4;
  cvub::C3SemanticIR &c3 = c4.c3;
  c3.bufferized.logicalModule.operations.push_back(
      operation(0, -1, -1, 0, "builtin.module"));
  cvub::C1OperationRecord function = operation(1, 0, 0, 0, "func.func");
  function.properties = "{sym_name = \"kernel\"}";
  c3.bufferized.logicalModule.operations.push_back(function);

  const int loadOrdinal = copyBeforeLoad ? 4 : 1;
  cvub::C1OperationRecord load =
      operation(2, 1, 0, loadOrdinal, "hivm.hir.load", {10, 20});
  load.operandTypes = {"memref<8xf32>", "memref<8xf32>"};
  c3.bufferized.logicalModule.operations.push_back(load);
  cvub::C1OperationRecord toTensor =
      operation(3, 1, 0, 2, "bufferization.to_tensor", {20});
  toTensor.results = {21};
  toTensor.operandTypes = {"memref<8xf32>"};
  toTensor.resultTypes = {"tensor<8xf32>"};
  c3.bufferized.logicalModule.operations.push_back(toTensor);

  int copyId = 4;
  if (middleRead) {
    c3.bufferized.logicalModule.operations.push_back(
        operation(copyId++, 1, 0, 3, "memref.load", {20}));
  }
  if (sourceWrite) {
    c3.bufferized.logicalModule.operations.push_back(
        operation(copyId++, 1, 0, 3, "memref.store", {30, 10}));
  }
  cvub::C1OperationRecord copy = operation(
      copyId, 1, differentBlock ? 1 : 0, copyBeforeLoad ? 1 : 5,
      "tensor.insert_slice", {21, 40});
  copy.operandTypes = {"tensor<8xf32>", "tensor<8xf32>"};
  c3.bufferized.logicalModule.operations.push_back(copy);

  addAccess(c3, load.id, 0, "arg:1:0");
  addAccess(c3, load.id, 1, "local:0");
  addAccess(c3, toTensor.id, 0, "local:0");
  if (middleRead)
    addAccess(c3, 4, 0, "local:0");
  if (sourceWrite)
    addAccess(c3, middleRead ? 5 : 4, 1, "arg:1:0");
  addAccess(c3, copy.id, 0, "local:0");
  addAccess(c3, copy.id, 1, "local:1");
  c3.singlePoint.bufferMapping = {
      {"local:0", "local:0"}, {"local:1", "local:1"}};
  c4.buffers = {
      {"base:0", "base:0", "", "memref<8xf32>",
       cvub::AddressSpace::UB, 256, false, {}},
      {"base:1", "base:1", "", "memref<8xf32>",
       cvub::AddressSpace::UB, 256, false, {}},
  };
  return c4;
}

void expectRewrites(const cvub::C4SemanticIR &c4, size_t expected,
                    const std::string &label) {
  const cvub::InlineLoadCopyResult result = cvub::ModelInlineLoadCopy(c4);
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
