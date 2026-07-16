module {
  func.func @b_control_cf_branches_with_scf_for(%src: memref<32xf32, #hivm.address_space<gm>>, %dst0: memref<32xf32, #hivm.address_space<gm>>, %dst1: memref<32xf32, #hivm.address_space<gm>>, %cond: i1) attributes {hivm.func_core_type = #hivm.func_core_type<AIV>} {
    %c0 = arith.constant 0 : index
    %c1 = arith.constant 1 : index
    %c2 = arith.constant 2 : index
    %alloc = memref.alloc() : memref<32xf32, #hivm.address_space<ub>>
    hivm.hir.load ins(%src : memref<32xf32, #hivm.address_space<gm>>) outs(%alloc : memref<32xf32, #hivm.address_space<ub>>)
    cf.cond_br %cond, ^bb1(%alloc : memref<32xf32, #hivm.address_space<ub>>), ^bb2(%alloc : memref<32xf32, #hivm.address_space<ub>>)
  ^bb1(%left: memref<32xf32, #hivm.address_space<ub>>):
    scf.for %iv = %c0 to %c2 step %c1 {
      %tmp = memref.alloc() : memref<32xf32, #hivm.address_space<ub>>
      hivm.hir.copy ins(%left : memref<32xf32, #hivm.address_space<ub>>) outs(%tmp : memref<32xf32, #hivm.address_space<ub>>)
      hivm.hir.store ins(%tmp : memref<32xf32, #hivm.address_space<ub>>) outs(%dst0 : memref<32xf32, #hivm.address_space<gm>>)
    }
    cf.br ^exit(%left : memref<32xf32, #hivm.address_space<ub>>)
  ^bb2(%right: memref<32xf32, #hivm.address_space<ub>>):
    scf.for %iv = %c0 to %c2 step %c1 {
      %tmp = memref.alloc() : memref<32xf32, #hivm.address_space<ub>>
      hivm.hir.copy ins(%right : memref<32xf32, #hivm.address_space<ub>>) outs(%tmp : memref<32xf32, #hivm.address_space<ub>>)
      hivm.hir.store ins(%tmp : memref<32xf32, #hivm.address_space<ub>>) outs(%dst0 : memref<32xf32, #hivm.address_space<gm>>)
    }
    cf.br ^exit(%right : memref<32xf32, #hivm.address_space<ub>>)
  ^exit(%joined: memref<32xf32, #hivm.address_space<ub>>):
    hivm.hir.store ins(%joined : memref<32xf32, #hivm.address_space<ub>>) outs(%dst1 : memref<32xf32, #hivm.address_space<gm>>)
    return
  }
}
