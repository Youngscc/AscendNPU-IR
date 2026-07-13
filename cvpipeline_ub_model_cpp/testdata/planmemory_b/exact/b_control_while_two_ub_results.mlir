module {
  func.func @b_control_while_two_ub_results(%src0: memref<32xf32, #hivm.address_space<gm>>, %src1: memref<32xf32, #hivm.address_space<gm>>, %dst0: memref<32xf32, #hivm.address_space<gm>>, %dst1: memref<32xf32, #hivm.address_space<gm>>, %cond: i1) attributes {hivm.func_core_type = #hivm.func_core_type<AIV>} {
    %a = memref.alloc() : memref<32xf32, #hivm.address_space<ub>>
    %b = memref.alloc() : memref<32xf32, #hivm.address_space<ub>>
    hivm.hir.load ins(%src0 : memref<32xf32, #hivm.address_space<gm>>) outs(%a : memref<32xf32, #hivm.address_space<ub>>)
    hivm.hir.load ins(%src1 : memref<32xf32, #hivm.address_space<gm>>) outs(%b : memref<32xf32, #hivm.address_space<ub>>)
    %result:2 = scf.while (%iter0 = %a, %iter1 = %b) : (memref<32xf32, #hivm.address_space<ub>>, memref<32xf32, #hivm.address_space<ub>>) -> (memref<32xf32, #hivm.address_space<ub>>, memref<32xf32, #hivm.address_space<ub>>) {
      scf.condition(%cond) %iter0, %iter1 : memref<32xf32, #hivm.address_space<ub>>, memref<32xf32, #hivm.address_space<ub>>
    } do {
    ^bb0(%after0: memref<32xf32, #hivm.address_space<ub>>, %after1: memref<32xf32, #hivm.address_space<ub>>):
      scf.yield %after1, %after0 : memref<32xf32, #hivm.address_space<ub>>, memref<32xf32, #hivm.address_space<ub>>
    }
    hivm.hir.store ins(%result#0 : memref<32xf32, #hivm.address_space<ub>>) outs(%dst0 : memref<32xf32, #hivm.address_space<gm>>)
    hivm.hir.store ins(%result#1 : memref<32xf32, #hivm.address_space<ub>>) outs(%dst1 : memref<32xf32, #hivm.address_space<gm>>)
    return
  }
}
