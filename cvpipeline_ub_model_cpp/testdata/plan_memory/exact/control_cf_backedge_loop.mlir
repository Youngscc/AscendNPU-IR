module {
  func.func @b_control_cf_backedge_loop(%src: memref<32xf32, #hivm.address_space<gm>>, %dst: memref<32xf32, #hivm.address_space<gm>>, %cond: i1) attributes {hivm.func_core_type = #hivm.func_core_type<AIV>} {
    %alloc = memref.alloc() : memref<32xf32, #hivm.address_space<ub>>
    hivm.hir.load ins(%src : memref<32xf32, #hivm.address_space<gm>>) outs(%alloc : memref<32xf32, #hivm.address_space<ub>>)
    cf.br ^loop(%alloc : memref<32xf32, #hivm.address_space<ub>>)
  ^loop(%iter: memref<32xf32, #hivm.address_space<ub>>):
    cf.cond_br %cond, ^loop(%iter : memref<32xf32, #hivm.address_space<ub>>), ^exit(%iter : memref<32xf32, #hivm.address_space<ub>>)
  ^exit(%result: memref<32xf32, #hivm.address_space<ub>>):
    hivm.hir.store ins(%result : memref<32xf32, #hivm.address_space<ub>>) outs(%dst : memref<32xf32, #hivm.address_space<gm>>)
    return
  }
}
