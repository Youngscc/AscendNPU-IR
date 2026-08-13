// RUN: bishengir-opt -allow-unregistered-dialect %s \
// RUN:   -pass-pipeline="builtin.module(func.func(hivm-mark-multi-buffer{enable-auto=true set-local-multibuffer=1}),cse)" \
// RUN:   | FileCheck %s --check-prefix=ONE
// RUN: bishengir-opt -allow-unregistered-dialect %s \
// RUN:   -pass-pipeline="builtin.module(func.func(hivm-mark-multi-buffer{enable-auto=true set-local-multibuffer=3}),cse)" \
// RUN:   | FileCheck %s --check-prefix=THREE
// RUN: bishengir-opt -allow-unregistered-dialect %s \
// RUN:   -pass-pipeline="builtin.module(func.func(hivm-mark-multi-buffer{enable-auto=true set-local-multibuffer=4}),cse)" \
// RUN:   | FileCheck %s --check-prefix=FOUR

module {
  // ONE-LABEL: func.func @ordinary_local
  // THREE-LABEL: func.func @ordinary_local
  // FOUR-LABEL: func.func @ordinary_local
  func.func @ordinary_local(
      %input: memref<16xf16, #hivm.address_space<gm>>) {
    %c0 = arith.constant 0 : index
    %c1 = arith.constant 1 : index
    %c4 = arith.constant 4 : index
    scf.for %i = %c0 to %c4 step %c1 {
      %buffer = memref.alloc() : memref<16xf16, #hivm.address_space<ub>>
      // ONE: annotation.mark %{{.*}} {hivm.multi_buffer = 1 : i32}
      // THREE: annotation.mark %{{.*}} {hivm.multi_buffer = 3 : i32}
      // FOUR: annotation.mark %{{.*}} {hivm.multi_buffer = 4 : i32}
      hivm.hir.load
          ins(%input : memref<16xf16, #hivm.address_space<gm>>)
          outs(%buffer : memref<16xf16, #hivm.address_space<ub>>)
    }
    return
  }

  // The ordinary local count must not replace the independently inferred
  // preload-local count, even though the preload buffer is also the
  // destination of a load. Here, producer preload 2 and consumer preload 0
  // require three buffers.
  // ONE-LABEL: func.func @preload_local
  // THREE-LABEL: func.func @preload_local
  // FOUR-LABEL: func.func @preload_local
  func.func @preload_local(
      %input: memref<16xf16, #hivm.address_space<gm>>) {
    %c0 = arith.constant 0 : index
    %c1 = arith.constant 1 : index
    %c4 = arith.constant 4 : index
    scf.for %i = %c0 to %c4 step %c1 {
      %preloaded = scope.scope : () -> memref<16xf16, #hivm.address_space<ub>> {
        %buffer = memref.alloc() : memref<16xf16, #hivm.address_space<ub>>
        // ONE: annotation.mark %{{.*}} {hivm.multi_buffer = 3 : i32, hivm.preload_local_buffer = 1 : i32}
        // THREE: annotation.mark %{{.*}} {hivm.multi_buffer = 3 : i32, hivm.preload_local_buffer = 1 : i32}
        // FOUR: annotation.mark %{{.*}} {hivm.multi_buffer = 3 : i32, hivm.preload_local_buffer = 1 : i32}
        hivm.hir.load
            ins(%input : memref<16xf16, #hivm.address_space<gm>>)
            outs(%buffer : memref<16xf16, #hivm.address_space<ub>>)
        scope.return %buffer : memref<16xf16, #hivm.address_space<ub>>
      } {hivm.loop_core_type = #hivm.tcore_type<VECTOR>,
         hivm.max_preload_num = 4 : i32, hivm.preload_num = 2 : i32,
         no_inline}
      %consumed = scope.scope : () -> memref<16xf16, #hivm.address_space<ub>> {
        %output = memref.alloc() : memref<16xf16, #hivm.address_space<ub>>
        hivm.hir.vexp
            ins(%preloaded : memref<16xf16, #hivm.address_space<ub>>)
            outs(%output : memref<16xf16, #hivm.address_space<ub>>)
        scope.return %output : memref<16xf16, #hivm.address_space<ub>>
      } {hivm.loop_core_type = #hivm.tcore_type<VECTOR>,
         hivm.max_preload_num = 4 : i32, hivm.preload_num = 0 : i32,
         no_inline}
    }
    return
  }
}
