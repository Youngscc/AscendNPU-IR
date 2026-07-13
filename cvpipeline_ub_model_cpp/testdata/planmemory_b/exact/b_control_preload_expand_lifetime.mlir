module {
  func.func @test_preload_buffer_expand_lifetime(%arg0: memref<16384xi8, #hivm.address_space<gm>>,
                                                 %arg1: memref<16384xi32, #hivm.address_space<gm>>,
                                                 %arg2: memref<16384xi32, #hivm.address_space<gm>>,
                                                 %arg3: memref<4096xi32, #hivm.address_space<gm>>,
                                                 %arg4: memref<4096xi32, #hivm.address_space<gm>>,
                                                 %arg5: memref<4096xi32, #hivm.address_space<gm>>) attributes {hivm.func_core_type = #hivm.func_core_type<AIV>} {
    %c4 = arith.constant 4 : index
    %c0 = arith.constant 0 : index
    %c1 = arith.constant 1 : index
    %c0_i32 = arith.constant 0 : i32
    %c1_i32 = arith.constant 1 : i32
    %c16_i32 = arith.constant 16 : i32
    %c128_i32 = arith.constant 128 : i32
    %cst_0 = arith.constant 0xFF8000 : i32
    %alloc = memref.alloc() : memref<4096xi32, #hivm.address_space<ub>>
    hivm.hir.vbrc ins(%cst_0 : i32) outs(%alloc : memref<4096xi32, #hivm.address_space<ub>>)
    scf.for %arg6 = %c0_i32 to %c16_i32 step %c1_i32  : i32 {
      %alloc_0 = memref.alloc() : memref<4096xi32, #hivm.address_space<ub>>
      hivm.hir.copy ins(%alloc : memref<4096xi32, #hivm.address_space<ub>>) outs(%alloc_0 : memref<4096xi32, #hivm.address_space<ub>>)
      %0 = scf.for %arg7 = %c0_i32 to %c128_i32 step %c16_i32 iter_args(%arg8 = %alloc_0) -> (memref<4096xi32, #hivm.address_space<ub>>)  : i32 {
          %1 = scope.scope : () -> (memref<4096xi32, #hivm.address_space<ub>>) {
          %alloc_1 = memref.alloc() : memref<16384xi8, #hivm.address_space<ub>>
          hivm.hir.load ins(%arg0 : memref<16384xi8, #hivm.address_space<gm>>) outs(%alloc_1 : memref<16384xi8, #hivm.address_space<ub>>)
          %alloc_2 = memref.alloc() : memref<16384xi16, #hivm.address_space<ub>>
          hivm.hir.vcast ins(%alloc_1 : memref<16384xi8, #hivm.address_space<ub>>) outs(%alloc_2 : memref<16384xi16, #hivm.address_space<ub>>)
          %alloc_3 = memref.alloc() : memref<16384xi32, #hivm.address_space<ub>>
          hivm.hir.vcast ins(%alloc_2 : memref<16384xi16, #hivm.address_space<ub>>) outs(%alloc_3 : memref<16384xi32, #hivm.address_space<ub>>)
          %alloc_4 = memref.alloc() : memref<16384xi32, #hivm.address_space<ub>>
          hivm.hir.load ins(%arg1 : memref<16384xi32, #hivm.address_space<gm>>) outs(%alloc_4 : memref<16384xi32, #hivm.address_space<ub>>)
          %alloc_5 = memref.alloc() : memref<16384xi32, #hivm.address_space<ub>>
          hivm.hir.vadd ins(%alloc_3, %alloc_4 : memref<16384xi32, #hivm.address_space<ub>>, memref<16384xi32, #hivm.address_space<ub>>) outs(%alloc_5 : memref<16384xi32, #hivm.address_space<ub>>)
          hivm.hir.store ins(%alloc_5 : memref<16384xi32, #hivm.address_space<ub>>) outs(%arg2 : memref<16384xi32, #hivm.address_space<gm>>)
          %alloc_6 = memref.alloc() : memref<4096xi32, #hivm.address_space<ub>>
          annotation.mark %alloc_6 {hivm.multi_buffer = 4 : i32, hivm.preload_local_buffer = 1 : i32} : memref<4096xi32, #hivm.address_space<ub>>
          scf.for %arg9 = %c0 to %c4 step %c1 {
            %alloc_7 = memref.alloc() : memref<4096xi32, #hivm.address_space<ub>>
            hivm.hir.load ins(%arg3 : memref<4096xi32, #hivm.address_space<gm>>) outs(%alloc_7 : memref<4096xi32, #hivm.address_space<ub>>)
            hivm.hir.vmax ins(%arg8, %alloc_7 : memref<4096xi32, #hivm.address_space<ub>>, memref<4096xi32, #hivm.address_space<ub>>) outs(%alloc_6 : memref<4096xi32, #hivm.address_space<ub>>)
          }
          scope.return %alloc_6 : memref<4096xi32, #hivm.address_space<ub>>
        } {hivm.loop_core_type = #hivm.tcore_type<VECTOR>, hivm.preload_num = 2 : i32, no_inline}
        %2 = scope.scope : () -> memref<4096xi32, #hivm.address_space<ub>> {
          %alloc_1 = memref.alloc() : memref<4096xi32, #hivm.address_space<ub>>
          hivm.hir.load ins(%arg4 : memref<4096xi32, #hivm.address_space<gm>>) outs(%alloc_1 : memref<4096xi32, #hivm.address_space<ub>>)
          %alloc_2 = memref.alloc() : memref<4096xi32, #hivm.address_space<ub>>
          hivm.hir.vadd ins(%1, %alloc_1 : memref<4096xi32, #hivm.address_space<ub>>, memref<4096xi32, #hivm.address_space<ub>>) outs(%alloc_2 : memref<4096xi32, #hivm.address_space<ub>>)
          scope.return %alloc_2 : memref<4096xi32, #hivm.address_space<ub>>
        } {hivm.loop_core_type = #hivm.tcore_type<VECTOR>, hivm.preload_num = 0 : i32, no_inline}
        scf.yield %2 : memref<4096xi32, #hivm.address_space<ub>>
      }
      hivm.hir.store ins(%0 : memref<4096xi32, #hivm.address_space<ub>>) outs(%arg5 : memref<4096xi32, #hivm.address_space<gm>>)
    }
    return
  }
}
