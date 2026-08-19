// REQUIRES: hivmc-a5
// RUN: bishengir-opt %s --convert-ssbuffer-to-legacy-llvm > %t.mlir
// RUN: not grep -E "address_space<ssbuf>|hivm.hir.pointer_cast|memref_ext.volatile" %t.mlir
// RUN: hivmc-a5 %t.mlir -o %t.o --only-run-hivm-pipeline=false
// RUN: test -s %t.o

module attributes {hivm.module_core_type = #hivm.module_core_type<AIV>} {
  func.func @lower_ssbuffer_for_hivmc(%address: i64, %value: i32)
      attributes {
        global_kernel = "local",
        hacc.entry,
        hacc.function_kind = #hacc.function_kind<DEVICE>,
        hivm.func_core_type = #hivm.func_core_type<AIV>,
        mix_mode = "aiv",
        parallel_mode = "simd"
      } {
    %ptr = hivm.hir.pointer_cast(%address)
        : memref<i32, #hivm.address_space<ssbuf>>
    memref.store %value, %ptr[]
        : memref<i32, #hivm.address_space<ssbuf>>
    %loaded = memref.load %ptr[]
        : memref<i32, #hivm.address_space<ssbuf>>
    annotation.mark %loaded {memref_ext.volatile} : i32
    return
  }
}
