module {
  func.func @b_control_scope_two_preload_buffers(%src0: memref<32xf32, #hivm.address_space<gm>>, %src1: memref<32xf32, #hivm.address_space<gm>>, %dst0: memref<32xf32, #hivm.address_space<gm>>, %dst1: memref<32xf32, #hivm.address_space<gm>>) attributes {hivm.func_core_type = #hivm.func_core_type<AIV>} {
    %c0 = arith.constant 0 : index
    %c1 = arith.constant 1 : index
    %c4 = arith.constant 4 : index
    scf.for %iv = %c0 to %c4 step %c1 {
      %result:2 = scope.scope : () -> (memref<32xf32, #hivm.address_space<ub>>, memref<32xf32, #hivm.address_space<ub>>) {
        %a = memref.alloc() : memref<32xf32, #hivm.address_space<ub>>
        annotation.mark %a {hivm.multi_buffer = 2 : i32, hivm.preload_local_buffer = 1 : i32} : memref<32xf32, #hivm.address_space<ub>>
        %b = memref.alloc() : memref<32xf32, #hivm.address_space<ub>>
        annotation.mark %b {hivm.multi_buffer = 4 : i32, hivm.preload_local_buffer = 1 : i32} : memref<32xf32, #hivm.address_space<ub>>
        hivm.hir.load ins(%src0 : memref<32xf32, #hivm.address_space<gm>>) outs(%a : memref<32xf32, #hivm.address_space<ub>>)
        hivm.hir.load ins(%src1 : memref<32xf32, #hivm.address_space<gm>>) outs(%b : memref<32xf32, #hivm.address_space<ub>>)
        scope.return %a, %b : memref<32xf32, #hivm.address_space<ub>>, memref<32xf32, #hivm.address_space<ub>>
      } {hivm.loop_core_type = #hivm.tcore_type<VECTOR>, hivm.preload_num = 2 : i32, no_inline}
      hivm.hir.store ins(%result#0 : memref<32xf32, #hivm.address_space<ub>>) outs(%dst0 : memref<32xf32, #hivm.address_space<gm>>)
      hivm.hir.store ins(%result#1 : memref<32xf32, #hivm.address_space<ub>>) outs(%dst1 : memref<32xf32, #hivm.address_space<gm>>)
    }
    return
  }
}
