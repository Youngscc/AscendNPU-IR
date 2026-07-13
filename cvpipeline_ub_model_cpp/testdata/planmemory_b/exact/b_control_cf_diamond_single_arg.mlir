module {
  func.func @test_branchop(%arg0: memref<16xf16, #hivm.address_space<gm>>,
                           %arg1: memref<16xf16, #hivm.address_space<gm>>,
                           %arg2: memref<16xf16, #hivm.address_space<gm>>,
                           %arg3: i1) attributes {hivm.func_core_type = #hivm.func_core_type<AIV>} {
    %alloc = memref.alloc() : memref<16xf16, #hivm.address_space<ub>>
    hivm.hir.load ins(%arg0 : memref<16xf16, #hivm.address_space<gm>>) outs(%alloc : memref<16xf16, #hivm.address_space<ub>>)
    cf.cond_br %arg3, ^bb1(%alloc : memref<16xf16, #hivm.address_space<ub>>), ^bb2(%alloc : memref<16xf16, #hivm.address_space<ub>>)
    ^bb1(%arg10 : memref<16xf16, #hivm.address_space<ub>>):
      %alloc_0 = memref.alloc() : memref<16xf16, #hivm.address_space<ub>>
      hivm.hir.load ins(%arg1 : memref<16xf16, #hivm.address_space<gm>>) outs(%alloc_0 : memref<16xf16, #hivm.address_space<ub>>)
      %alloc_1 = memref.alloc() : memref<16xf16, #hivm.address_space<ub>>
      hivm.hir.vadd ins(%arg10, %alloc_0 : memref<16xf16, #hivm.address_space<ub>>, memref<16xf16, #hivm.address_space<ub>>)
        outs(%alloc_1 : memref<16xf16, #hivm.address_space<ub>>)
      cf.br ^bb3(%alloc_1 : memref<16xf16, #hivm.address_space<ub>>)
    ^bb2(%arg11 : memref<16xf16, #hivm.address_space<ub>>):
      %alloc_2 = memref.alloc() : memref<16xf16, #hivm.address_space<ub>>
      hivm.hir.load ins(%arg1 : memref<16xf16, #hivm.address_space<gm>>) outs(%alloc_2 : memref<16xf16, #hivm.address_space<ub>>)
      %alloc_3 = memref.alloc() : memref<16xf16, #hivm.address_space<ub>>
      hivm.hir.vsub ins(%arg11, %alloc_2 : memref<16xf16, #hivm.address_space<ub>>, memref<16xf16, #hivm.address_space<ub>>)
        outs(%alloc_3 : memref<16xf16, #hivm.address_space<ub>>)
      cf.br ^bb3(%alloc_3 : memref<16xf16, #hivm.address_space<ub>>)
    ^bb3(%arg12 : memref<16xf16, #hivm.address_space<ub>>):
      %alloc_4 = memref.alloc() : memref<16xf16, #hivm.address_space<ub>>
      hivm.hir.vadd ins(%arg12, %arg12 : memref<16xf16, #hivm.address_space<ub>>, memref<16xf16, #hivm.address_space<ub>>)
                  outs(%alloc_4 : memref<16xf16, #hivm.address_space<ub>>)
    hivm.hir.store ins(%alloc_4 : memref<16xf16, #hivm.address_space<ub>>) outs(%arg2 : memref<16xf16, #hivm.address_space<gm>>)
    return
  }
}
