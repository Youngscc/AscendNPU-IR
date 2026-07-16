module {
  func.func @b_control_scope_two_ub_results(%src0: memref<32xf32, #hivm.address_space<gm>>, %src1: memref<32xf32, #hivm.address_space<gm>>, %dst0: memref<32xf32, #hivm.address_space<gm>>, %dst1: memref<32xf32, #hivm.address_space<gm>>) attributes {hivm.func_core_type = #hivm.func_core_type<AIV>} {
    %result:2 = scope.scope : () -> (memref<32xf32, #hivm.address_space<ub>>, memref<32xf32, #hivm.address_space<ub>>) {
      %a = memref.alloc() : memref<32xf32, #hivm.address_space<ub>>
      %b = memref.alloc() : memref<32xf32, #hivm.address_space<ub>>
      hivm.hir.load ins(%src0 : memref<32xf32, #hivm.address_space<gm>>) outs(%a : memref<32xf32, #hivm.address_space<ub>>)
      hivm.hir.load ins(%src1 : memref<32xf32, #hivm.address_space<gm>>) outs(%b : memref<32xf32, #hivm.address_space<ub>>)
      scope.return %a, %b : memref<32xf32, #hivm.address_space<ub>>, memref<32xf32, #hivm.address_space<ub>>
    } {no_inline}
    hivm.hir.store ins(%result#0 : memref<32xf32, #hivm.address_space<ub>>) outs(%dst0 : memref<32xf32, #hivm.address_space<gm>>)
    hivm.hir.store ins(%result#1 : memref<32xf32, #hivm.address_space<ub>>) outs(%dst1 : memref<32xf32, #hivm.address_space<gm>>)
    return
  }
}
