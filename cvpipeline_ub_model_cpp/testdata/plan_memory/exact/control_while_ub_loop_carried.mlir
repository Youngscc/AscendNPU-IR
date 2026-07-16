module {
  func.func @test_scfwhileop_yield_inplace(%arg0 : memref<16xf32, #hivm.address_space<gm>>,
                                           %arg1 : memref<16xf32, #hivm.address_space<gm>>,
                                           %arg2 : memref<16xf32, #hivm.address_space<gm>>,
                                           %arg3 : memref<16xf32, #hivm.address_space<gm>>,
                                           %arg4 : i32) attributes {hivm.func_core_type = #hivm.func_core_type<AIV>} {
    %c0 = arith.constant 0 : i32
    %c1 = arith.constant 1 : i32
    %c100 = arith.constant 100 : i32
    %alloc = memref.alloc() : memref<16xf32, #hivm.address_space<ub>>
    hivm.hir.load ins(%arg0 : memref<16xf32, #hivm.address_space<gm>>) outs(%alloc : memref<16xf32, #hivm.address_space<ub>>)
    %alloc_0 = memref.alloc() : memref<16xf32, #hivm.address_space<ub>>
    hivm.hir.load ins(%arg1 : memref<16xf32, #hivm.address_space<gm>>) outs(%alloc_0 : memref<16xf32, #hivm.address_space<ub>>)
    %0 = scf.while (%arg5 = %alloc) : (memref<16xf32, #hivm.address_space<ub>>) -> memref<16xf32, #hivm.address_space<ub>> {
      %1 = arith.cmpi eq, %arg4, %c100 : i32
      scf.condition(%1) %alloc_0 : memref<16xf32, #hivm.address_space<ub>>
    } do {
    ^bb0(%arg6: memref<16xf32, #hivm.address_space<ub>>):
      %alloc_1 = memref.alloc() : memref<16xf32, #hivm.address_space<ub>>
      hivm.hir.load ins(%arg2 : memref<16xf32, #hivm.address_space<gm>>) outs(%alloc_1 : memref<16xf32, #hivm.address_space<ub>>)
      %alloc_2 = memref.alloc() : memref<16xf32, #hivm.address_space<ub>>
      hivm.hir.vadd ins(%arg6, %alloc_1 : memref<16xf32, #hivm.address_space<ub>>, memref<16xf32, #hivm.address_space<ub>>) outs(%alloc_2 : memref<16xf32, #hivm.address_space<ub>>)
      %alloc_3 = memref.alloc() : memref<16xf32, #hivm.address_space<ub>>
      hivm.hir.vadd ins(%arg6, %alloc_2 : memref<16xf32, #hivm.address_space<ub>>, memref<16xf32, #hivm.address_space<ub>>) outs(%alloc_3 : memref<16xf32, #hivm.address_space<ub>>)
      scf.yield %alloc_3 : memref<16xf32, #hivm.address_space<ub>>
    }
    hivm.hir.store ins(%0#0 : memref<16xf32, #hivm.address_space<ub>>) outs(%arg3 : memref<16xf32, #hivm.address_space<gm>>)
    return
  }
}
