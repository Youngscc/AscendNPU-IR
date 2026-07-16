module {
  func.func @test_scope_copy_arg_used_after_yield_init(%arg0: memref<64xf32, #hivm.address_space<gm>>,
                                                       %arg1: memref<64xf32, #hivm.address_space<gm>>,
                                                       %arg2: memref<64xf32, #hivm.address_space<gm>>,
                                                       %arg3: memref<64xf32, #hivm.address_space<gm>>,
                                                       %arg4: memref<64xf32, #hivm.address_space<gm>>,
                                                       %arg5: memref<64xf32, #hivm.address_space<gm>>) attributes {hivm.func_core_type = #hivm.func_core_type<AIV>} {
    %c4 = arith.constant 4 : index
    %c0 = arith.constant 0 : index
    %c1 = arith.constant 1 : index
    %c0_i32 = arith.constant 0 : i32
    %c1_i32 = arith.constant 1 : i32
    %c16_i32 = arith.constant 16 : i32
    %c128_i32 = arith.constant 128 : i32
    %cst_0 = arith.constant 0xFF800000 : f32
    %alloc = memref.alloc() : memref<64xf32, #hivm.address_space<ub>>
    hivm.hir.vbrc ins(%cst_0 : f32) outs(%alloc : memref<64xf32, #hivm.address_space<ub>>)
    scf.for %arg6 = %c0_i32 to %c16_i32 step %c1_i32  : i32 {
      %alloc_0 = memref.alloc() : memref<64xf32, #hivm.address_space<ub>>
      hivm.hir.copy ins(%alloc : memref<64xf32, #hivm.address_space<ub>>) outs(%alloc_0 : memref<64xf32, #hivm.address_space<ub>>)
      %0 = scf.for %arg7 = %c0_i32 to %c128_i32 step %c16_i32 iter_args(%arg8 = %alloc_0) -> (memref<64xf32, #hivm.address_space<ub>>)  : i32 {
        %1 = scope.scope : () -> (memref<64xf32, #hivm.address_space<ub>>) {
          %alloc_1 = memref.alloc() : memref<64xf32, #hivm.address_space<ub>>
          scf.for %arg9 = %c0 to %c4 step %c1 {
            %alloc_2 = memref.alloc() : memref<64xf32, #hivm.address_space<ub>>
            hivm.hir.load ins(%arg0 : memref<64xf32, #hivm.address_space<gm>>) outs(%alloc_2 : memref<64xf32, #hivm.address_space<ub>>)
            hivm.hir.vmax ins(%arg8, %alloc_2 : memref<64xf32, #hivm.address_space<ub>>, memref<64xf32, #hivm.address_space<ub>>) outs(%alloc_1 : memref<64xf32, #hivm.address_space<ub>>)
            %alloc_3 = memref.alloc() : memref<64xf32, #hivm.address_space<ub>>
            hivm.hir.vsub ins(%arg8, %alloc_1 : memref<64xf32, #hivm.address_space<ub>>, memref<64xf32, #hivm.address_space<ub>>) outs(%alloc_3 : memref<64xf32, #hivm.address_space<ub>>)
            hivm.hir.store ins(%alloc_3 : memref<64xf32, #hivm.address_space<ub>>) outs(%arg1 : memref<64xf32, #hivm.address_space<gm>>)
          }
          scope.return %alloc_1 : memref<64xf32, #hivm.address_space<ub>>
        } {hivm.loop_core_type = #hivm.tcore_type<VECTOR>, hivm.preload_num = 2 : i32, no_inline}
        %2 = scope.scope : () -> memref<64xf32, #hivm.address_space<ub>> {
          %alloc_1 = memref.alloc() : memref<64xf32, #hivm.address_space<ub>>
          hivm.hir.load ins(%arg2 : memref<64xf32, #hivm.address_space<gm>>) outs(%alloc_1 : memref<64xf32, #hivm.address_space<ub>>)
          %alloc_2 = memref.alloc() : memref<64xf32, #hivm.address_space<ub>>
          hivm.hir.load ins(%arg3 : memref<64xf32, #hivm.address_space<gm>>) outs(%alloc_2 : memref<64xf32, #hivm.address_space<ub>>)
          %alloc_3 = memref.alloc() : memref<64xf32, #hivm.address_space<ub>>
          hivm.hir.vadd ins(%alloc_2, %alloc_1 : memref<64xf32, #hivm.address_space<ub>>, memref<64xf32, #hivm.address_space<ub>>) outs(%alloc_3 : memref<64xf32, #hivm.address_space<ub>>)
          scope.return %alloc_3 : memref<64xf32, #hivm.address_space<ub>>
        } {hivm.loop_core_type = #hivm.tcore_type<VECTOR>, hivm.preload_num = 0 : i32, no_inline}
        scf.yield %1 : memref<64xf32, #hivm.address_space<ub>>
      }
      %alloc_4 = memref.alloc() : memref<64xf32, #hivm.address_space<ub>>
      hivm.hir.load ins(%arg4 : memref<64xf32, #hivm.address_space<gm>>) outs(%alloc_4 : memref<64xf32, #hivm.address_space<ub>>)
      %alloc_5 = memref.alloc() : memref<64xf32, #hivm.address_space<ub>>
      hivm.hir.vadd ins(%0, %alloc_4 : memref<64xf32, #hivm.address_space<ub>>, memref<64xf32, #hivm.address_space<ub>>) outs(%alloc_5 : memref<64xf32, #hivm.address_space<ub>>)
      hivm.hir.store ins(%alloc_5 : memref<64xf32, #hivm.address_space<ub>>) outs(%arg5 : memref<64xf32, #hivm.address_space<gm>>)
    }
    return
  }
}
