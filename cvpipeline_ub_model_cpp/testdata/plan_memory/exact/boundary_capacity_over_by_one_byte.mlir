module {
  func.func @b_boundary_capacity_over_by_one_byte(%src: memref<196609xi8, #hivm.address_space<gm>>, %dst: memref<196609xi8, #hivm.address_space<gm>>) attributes {hivm.func_core_type = #hivm.func_core_type<AIV>} {
    %alloc = memref.alloc() : memref<196609xi8, #hivm.address_space<ub>>
    hivm.hir.load ins(%src : memref<196609xi8, #hivm.address_space<gm>>) outs(%alloc : memref<196609xi8, #hivm.address_space<ub>>)
    hivm.hir.store ins(%alloc : memref<196609xi8, #hivm.address_space<ub>>) outs(%dst : memref<196609xi8, #hivm.address_space<gm>>)
    return
  }
}
