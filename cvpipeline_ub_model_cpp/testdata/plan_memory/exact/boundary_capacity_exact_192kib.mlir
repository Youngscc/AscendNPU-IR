module {
  func.func @b_boundary_capacity_exact_192kib(%src: memref<196608xi8, #hivm.address_space<gm>>, %dst: memref<196608xi8, #hivm.address_space<gm>>) attributes {hivm.func_core_type = #hivm.func_core_type<AIV>} {
    %alloc = memref.alloc() : memref<196608xi8, #hivm.address_space<ub>>
    hivm.hir.load ins(%src : memref<196608xi8, #hivm.address_space<gm>>) outs(%alloc : memref<196608xi8, #hivm.address_space<ub>>)
    hivm.hir.store ins(%alloc : memref<196608xi8, #hivm.address_space<ub>>) outs(%dst : memref<196608xi8, #hivm.address_space<gm>>)
    return
  }
}
