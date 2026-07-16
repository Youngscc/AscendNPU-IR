module {
  func.func @test_iter_arg_use_in_if_yield_outside(%arg0: memref<16x16x16xf16, #hivm.address_space<gm>>,
                                                   %arg1: memref<16x16x16xf16, #hivm.address_space<gm>>,
                                                   %arg2: memref<16x16x16xf16, #hivm.address_space<gm>>) attributes {hivm.func_core_type = #hivm.func_core_type<AIV>} {
    %c128 = arith.constant 128 : index
    %c1024 = arith.constant 1024 : index
    %c0 = arith.constant 0 : index
    %true = arith.constant true
    %alloc = memref.alloc() : memref<16x16x16xf16, #hivm.address_space<ub>>
    %alloc_0 = memref.alloc() : memref<16x16x16xf16, #hivm.address_space<ub>>
    %alloc_1 = memref.alloc() : memref<16x16x16xf16, #hivm.address_space<ub>>
    hivm.hir.load ins(%arg0 : memref<16x16x16xf16, #hivm.address_space<gm>>) outs(%alloc : memref<16x16x16xf16, #hivm.address_space<ub>>)
    hivm.hir.load ins(%arg1 : memref<16x16x16xf16, #hivm.address_space<gm>>) outs(%alloc_0 : memref<16x16x16xf16, #hivm.address_space<ub>>)
    %0 = scf.for %arg3 = %c0 to %c1024 step %c128 iter_args(%arg4 = %alloc_0) -> (memref<16x16x16xf16, #hivm.address_space<ub>>) {
      %alloc_2 = memref.alloc() : memref<16x16x16xf16, #hivm.address_space<ub>>
      scf.if %true {
        hivm.hir.vadd ins(%alloc, %arg4 : memref<16x16x16xf16, #hivm.address_space<ub>>, memref<16x16x16xf16, #hivm.address_space<ub>>)
          outs(%alloc_2 : memref<16x16x16xf16, #hivm.address_space<ub>>)
      }
      %alloc_3 = memref.alloc() : memref<16x16x16xf16, #hivm.address_space<ub>>
      hivm.hir.vadd ins(%alloc_1, %alloc_2 : memref<16x16x16xf16, #hivm.address_space<ub>>, memref<16x16x16xf16, #hivm.address_space<ub>>)
                    outs(%alloc_3 : memref<16x16x16xf16, #hivm.address_space<ub>>)
      scf.yield %alloc_3 : memref<16x16x16xf16, #hivm.address_space<ub>>
    }
    hivm.hir.store ins(%0 : memref<16x16x16xf16, #hivm.address_space<ub>>) outs(%arg2 : memref<16x16x16xf16, #hivm.address_space<gm>>)
    return
  }
}
