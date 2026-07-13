module {
  func.func @test_select_cond_alias_not_inplace(%arg0: memref<32xf32, #hivm.address_space<gm>>,
                                                %arg1: memref<32xf32, #hivm.address_space<gm>>,
                                                %arg2: i1,
                                                %arg3: memref<32xf32, #hivm.address_space<gm>>) attributes {hivm.func_core_type = #hivm.func_core_type<AIV>} {
    %alloc_0 = memref.alloc() : memref<32xf32, #hivm.address_space<ub>>
    %alloc_1 = memref.alloc() : memref<32xf32, #hivm.address_space<ub>>
    hivm.hir.load ins(%arg0 : memref<32xf32, #hivm.address_space<gm>>) outs(%alloc_0 : memref<32xf32, #hivm.address_space<ub>>)
    hivm.hir.load ins(%arg1 : memref<32xf32, #hivm.address_space<gm>>) outs(%alloc_1 : memref<32xf32, #hivm.address_space<ub>>)
    %c128 = arith.constant 128 : index
    %c1 = arith.constant 1 : index
    %c0 = arith.constant 0 : index
    %0:2 = scf.for %arg4 = %c0 to %c128 step %c1 iter_args(%arg5 = %alloc_0, %arg6 = %alloc_1) -> (memref<32xf32, #hivm.address_space<ub>>, memref<32xf32, #hivm.address_space<ub>>) {
      %1 = arith.select %arg2, %arg5, %arg6 : memref<32xf32, #hivm.address_space<ub>>
      %alloc_2 = memref.alloc() : memref<32xf32, #hivm.address_space<ub>>
      hivm.hir.load ins(%arg3 : memref<32xf32, #hivm.address_space<gm>>) outs(%alloc_2 : memref<32xf32, #hivm.address_space<ub>>)
      %2 = arith.select %arg2, %alloc_2, %arg5 : memref<32xf32, #hivm.address_space<ub>>
      %3 = arith.select %arg2, %arg6, %alloc_2 : memref<32xf32, #hivm.address_space<ub>>
      %alloc_3 = memref.alloc() : memref<32xf32, #hivm.address_space<ub>>
      hivm.hir.copy ins(%2 : memref<32xf32, #hivm.address_space<ub>>) outs(%alloc_3 : memref<32xf32, #hivm.address_space<ub>>)
      %alloc_4 = memref.alloc() : memref<32xf32, #hivm.address_space<ub>>
      hivm.hir.copy ins(%3 : memref<32xf32, #hivm.address_space<ub>>) outs(%alloc_4 : memref<32xf32, #hivm.address_space<ub>>)
      scf.yield %alloc_3, %alloc_4 : memref<32xf32, #hivm.address_space<ub>>, memref<32xf32, #hivm.address_space<ub>>
    }
    return
  }
}
