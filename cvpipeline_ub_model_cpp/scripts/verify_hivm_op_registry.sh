#!/usr/bin/env bash
set -euo pipefail

MODULE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPO_ROOT="$(cd "${MODULE_DIR}/.." && pwd)"
GENERATED_DIR="${REPO_ROOT}/build/tools/bishengir/bishengir/include/bishengir/Dialect/HIVM/IR"
REGISTRY="${MODULE_DIR}/src/ir/hivm_op_semantics.hpp"
EXTRA_BUFFER_SOURCE="${REPO_ROOT}/bishengir/lib/Dialect/HIVM/IR/ExtraBufferOpInterface/GetExtraBuffers.cpp"
GENERATED_HEADERS=(
  "${GENERATED_DIR}/HIVMOps.h.inc"
  "${GENERATED_DIR}/HIVMDMAOps.h.inc"
  "${GENERATED_DIR}/HIVMMacroOps.h.inc"
  "${GENERATED_DIR}/HIVMVectorOps.h.inc"
  "${GENERATED_DIR}/HIVMSynchronizationOps.h.inc"
)

for header in "${GENERATED_HEADERS[@]}"; do
  [[ -f "${header}" ]] || {
    echo "[ERROR] generated HIVM declaration is missing: ${header}" >&2
    exit 2
  }
done

generated_ops() {
  local trait="$1"
  awk -v trait="${trait}" '
    /^class [A-Za-z0-9_]+Op : public ::mlir::Op</ {
      header = $0
      active = 1
      next
    }
    active && /return ::llvm::StringLiteral\("/ {
      name = $0
      sub(/^.*StringLiteral\("/, "", name)
      sub(/"\).*$/, "", name)
      if (index(header, trait) != 0)
        print name
      active = 0
    }
  ' "${GENERATED_HEADERS[@]}" | LC_ALL=C sort -u
}

registry_ops() {
  local function_name="$1"
  sed -n "/inline bool ${function_name}/,/return operations.count/p" \
    "${REGISTRY}" |
    rg -o '"hivm\.hir\.[^"]+"' |
    tr -d '"' |
    LC_ALL=C sort -u
}

source_scalar_or_broadcast_extra_buffer_ops() {
  awk '
    FILENAME == ARGV[1] &&
        /ENABLE_OP_SHOULD_ALLOC_EXTRA_BUFFER_FOR_SCALAR_OR_OTF_BRC\(/ {
      name = $0
      sub(/^.*\(/, "", name)
      sub(/\).*$/, "", name)
      desired[name] = 1
      next
    }
    FILENAME != ARGV[1] && /^class [A-Za-z0-9_]+Op : public ::mlir::Op</ {
      class_name = $2
      active = desired[class_name]
      next
    }
    active && /return ::llvm::StringLiteral\("/ {
      name = $0
      sub(/^.*StringLiteral\("/, "", name)
      sub(/"\).*$/, "", name)
      print name
      active = 0
    }
  ' "${EXTRA_BUFFER_SOURCE}" "${GENERATED_HEADERS[@]}" | LC_ALL=C sort -u
}

diff -u \
  <(generated_ops 'DestinationStyleOpInterface::Trait') \
  <(registry_ops IsDestinationStyleOp)
diff -u \
  <(generated_ops 'SinglePipeOpTrait') \
  <(registry_ops IsSinglePipeOp)
diff -u \
  <(generated_ops 'OpPipeTrait<PIPE::PIPE_MTE2>') \
  <(registry_ops HasStaticMTE2Pipe)
diff -u \
  <(generated_ops 'OpPipeTrait<PIPE::PIPE_MTE3>') \
  <(registry_ops HasStaticMTE3Pipe)
diff -u \
  <(generated_ops 'ImplByScalarOpInterface::Trait') \
  <(registry_ops HasImplByScalarOpInterface)
diff -u \
  <(generated_ops 'ScalarOnlyHWTrait<1>') \
  <(registry_ops HasScalarOnlyHWOperand1)
if [[ -n "$(generated_ops 'ScalarOnlyHWTrait<0>')" ]]; then
  echo "[ERROR] lightweight scalar operand registry lacks operand index 0" >&2
  exit 1
fi
diff -u \
  <(generated_ops 'ElementwiseNaryOpTrait') \
  <(registry_ops IsElementwiseNaryOp)
diff -u \
  <(generated_ops '::mlir::OpTrait::SameOperandsElementType') \
  <(registry_ops HasSameOperandsElementType)
diff -u \
  <(source_scalar_or_broadcast_extra_buffer_ops) \
  <(registry_ops ShouldAllocExtraBufferForScalarOrOTFBrc)
diff -u \
  <(comm -23 \
      <(generated_ops 'DestinationStyleOpInterface::Trait') \
      <(generated_ops 'HIVMStructuredOp::Trait')) \
  <(printf '%s\n' hivm.hir.gather_load hivm.hir.scatter_store)

echo "HIVM_OP_REGISTRY_VERIFICATION=PASS"
