module {
  func.func @test_multi_if_yield_not_alias(%arg0: memref<16xf16, #hivm.address_space<gm>>,
                                           %arg1: i1,
                                           %arg2: memref<16xf16, #hivm.address_space<gm>>,
                                           %arg3: memref<16xf16, #hivm.address_space<gm>>,
                                           %arg4: i1,
                                           %arg5: memref<16xf16, #hivm.address_space<gm>>,
                                           %arg6: memref<16xf16, #hivm.address_space<gm>>,
                                           %arg7: memref<16xf32, #hivm.address_space<gm>>,
                                           %arg8: memref<16xf32, #hivm.address_space<gm>>) attributes {hivm.func_core_type = #hivm.func_core_type<AIV>} {
    %alloc = memref.alloc() : memref<16xf16, #hivm.address_space<ub>>
    hivm.hir.load ins(%arg0 : memref<16xf16, #hivm.address_space<gm>>) outs(%alloc : memref<16xf16, #hivm.address_space<ub>>)
    %0 = scf.if %arg1 -> (memref<16xf16, #hivm.address_space<ub>>) {
      %alloc_0 = memref.alloc() : memref<16xf16, #hivm.address_space<ub>>
      hivm.hir.load ins(%arg2 : memref<16xf16, #hivm.address_space<gm>>) outs(%alloc_0 : memref<16xf16, #hivm.address_space<ub>>)
      %alloc_1 = memref.alloc() : memref<16xf16, #hivm.address_space<ub>>
      hivm.hir.load ins(%arg3 : memref<16xf16, #hivm.address_space<gm>>) outs(%alloc_1 : memref<16xf16, #hivm.address_space<ub>>)
      %alloc_2 = memref.alloc() : memref<16xf16, #hivm.address_space<ub>>
      hivm.hir.vadd ins(%alloc_0, %alloc_1 : memref<16xf16, #hivm.address_space<ub>>, memref<16xf16, #hivm.address_space<ub>>)
        outs(%alloc_2 : memref<16xf16, #hivm.address_space<ub>>)
      scf.yield %alloc_2 : memref<16xf16, #hivm.address_space<ub>>
    } else {
      scf.yield %alloc : memref<16xf16, #hivm.address_space<ub>>
    }
    %1 = scf.if %arg4 -> (memref<16xf16, #hivm.address_space<ub>>) {
      %alloc_3 = memref.alloc() : memref<16xf16, #hivm.address_space<ub>>
      hivm.hir.load ins(%arg5 : memref<16xf16, #hivm.address_space<gm>>) outs(%alloc_3 : memref<16xf16, #hivm.address_space<ub>>)
      %alloc_4 = memref.alloc() : memref<16xf16, #hivm.address_space<ub>>
      hivm.hir.load ins(%arg6 : memref<16xf16, #hivm.address_space<gm>>) outs(%alloc_4 : memref<16xf16, #hivm.address_space<ub>>)
      %alloc_5 = memref.alloc() : memref<16xf16, #hivm.address_space<ub>>
      hivm.hir.vsub ins(%alloc_3, %alloc_4 : memref<16xf16, #hivm.address_space<ub>>, memref<16xf16, #hivm.address_space<ub>>)
        outs(%alloc_5 : memref<16xf16, #hivm.address_space<ub>>)
      scf.yield %alloc_5 : memref<16xf16, #hivm.address_space<ub>>
    } else {
      scf.yield %alloc : memref<16xf16, #hivm.address_space<ub>>
    }
    %alloc_6 = memref.alloc() : memref<16xf32, #hivm.address_space<ub>>
    hivm.hir.vcast ins(%0 : memref<16xf16, #hivm.address_space<ub>>) outs(%alloc_6 : memref<16xf32, #hivm.address_space<ub>>)
    hivm.hir.store ins(%alloc_6 : memref<16xf32, #hivm.address_space<ub>>) outs(%arg7 : memref<16xf32, #hivm.address_space<gm>>)
    %alloc_7 = memref.alloc() : memref<16xf32, #hivm.address_space<ub>>
    hivm.hir.vcast ins(%1 : memref<16xf16, #hivm.address_space<ub>>) outs(%alloc_7 : memref<16xf32, #hivm.address_space<ub>>)
    hivm.hir.store ins(%alloc_7 : memref<16xf32, #hivm.address_space<ub>>) outs(%arg8 : memref<16xf32, #hivm.address_space<gm>>)
    return
  }
}
