module {
  func.func @test_if_else(%arg1 : memref<11520xi32, #hivm.address_space<gm>>,
                          %arg2 : memref<11520xi32, #hivm.address_space<gm>>,
                          %arg3 : memref<11520xf32, #hivm.address_space<gm>>,
                          %arg4: i8 {tt.divisibility = 16 : i32},
                          %arg5 : memref<11520xi64, #hivm.address_space<gm>>,
                          %arg6 : memref<11520xi64, #hivm.address_space<gm>>) attributes {hivm.func_core_type = #hivm.func_core_type<AIV>} {
    %cst_1 = arith.constant 4.6566126E-10 : f32
    %1 = arith.trunci %arg4 : i8 to i1
    %alloc_0 = memref.alloc() : memref<11520xi32, #hivm.address_space<ub>>
    %alloc_1 = memref.alloc() : memref<11520xi32, #hivm.address_space<ub>>
    hivm.hir.load ins(%arg1 : memref<11520xi32, #hivm.address_space<gm>>) outs(%alloc_0 : memref<11520xi32, #hivm.address_space<ub>>)
    hivm.hir.load ins(%arg2 : memref<11520xi32, #hivm.address_space<gm>>) outs(%alloc_1 : memref<11520xi32, #hivm.address_space<ub>>)
    %alloc_2 = memref.alloc() : memref<11520xi32, #hivm.address_space<ub>>
    hivm.hir.vand ins(%alloc_0, %alloc_1 : memref<11520xi32, #hivm.address_space<ub>>, memref<11520xi32, #hivm.address_space<ub>>) outs(%alloc_2 : memref<11520xi32, #hivm.address_space<ub>>)
    scf.if %1 {
      %alloc_3 = memref.alloc() : memref<11520xi64, #hivm.address_space<ub>>
      hivm.hir.vcast ins(%alloc_2 : memref<11520xi32, #hivm.address_space<ub>>) outs(%alloc_3 : memref<11520xi64, #hivm.address_space<ub>>)
      hivm.hir.store ins(%alloc_3 : memref<11520xi64, #hivm.address_space<ub>>) outs(%arg5 : memref<11520xi64, #hivm.address_space<gm>>)
    } else {
      %alloc_4 = memref.alloc() : memref<11520xf32, #hivm.address_space<ub>>
      hivm.hir.load ins(%arg3 : memref<11520xf32, #hivm.address_space<gm>>) outs(%alloc_4 : memref<11520xf32, #hivm.address_space<ub>>)
      %alloc_5 = memref.alloc() : memref<11520xf32, #hivm.address_space<ub>>
      hivm.hir.vmul ins(%alloc_4, %cst_1 : memref<11520xf32, #hivm.address_space<ub>>, f32) outs(%alloc_5 : memref<11520xf32, #hivm.address_space<ub>>)
      %alloc_6 = memref.alloc() : memref<11520xi64, #hivm.address_space<ub>>
      hivm.hir.vcast ins(%alloc_2 : memref<11520xi32, #hivm.address_space<ub>>) outs(%alloc_6 : memref<11520xi64, #hivm.address_space<ub>>)
      hivm.hir.store ins(%alloc_6 : memref<11520xi64, #hivm.address_space<ub>>) outs(%arg6 : memref<11520xi64, #hivm.address_space<gm>>)
    }
    return
  }
}
