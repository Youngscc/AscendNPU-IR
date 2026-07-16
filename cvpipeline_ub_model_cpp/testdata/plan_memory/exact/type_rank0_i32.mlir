module {
  func.func @b_type_rank0_i32(%src: memref<i32, #hivm.address_space<gm>>, %dst: memref<i32, #hivm.address_space<gm>>) attributes {hivm.func_core_type = #hivm.func_core_type<AIV>} {
    %alloc = memref.alloc() : memref<i32, #hivm.address_space<ub>>
    hivm.hir.load ins(%src : memref<i32, #hivm.address_space<gm>>) outs(%alloc : memref<i32, #hivm.address_space<ub>>)
    hivm.hir.store ins(%alloc : memref<i32, #hivm.address_space<ub>>) outs(%dst : memref<i32, #hivm.address_space<gm>>)
    return
  }
}
