module {
  func.func @test_dead_after_op_nested_if_in_for(
      %src: memref<16x16xf16, #hivm.address_space<gm>>,
      %dst1: memref<16x16xf16, #hivm.address_space<gm>>,
      %dst2: memref<256xf16, #hivm.address_space<gm>>,
      %cond: i1) attributes {hivm.func_core_type = #hivm.func_core_type<AIV>} {
    %alloc = memref.alloc() : memref<16x16xf16, #hivm.address_space<ub>>
    %collapse = memref.collapse_shape %alloc [[0, 1]] :
        memref<16x16xf16, #hivm.address_space<ub>> into memref<256xf16, #hivm.address_space<ub>>
    hivm.hir.load ins(%src : memref<16x16xf16, #hivm.address_space<gm>>)
                  outs(%alloc : memref<16x16xf16, #hivm.address_space<ub>>)

    %if0 = scf.if %cond -> (memref<16x16xf16, #hivm.address_space<ub>>) {
      %tmp0 = memref.alloc() : memref<16x16xf16, #hivm.address_space<ub>>
      hivm.hir.vadd ins(%alloc, %alloc : memref<16x16xf16, #hivm.address_space<ub>>,
                        memref<16x16xf16, #hivm.address_space<ub>>)
                  outs(%tmp0 : memref<16x16xf16, #hivm.address_space<ub>>)
      scf.yield %tmp0 : memref<16x16xf16, #hivm.address_space<ub>>
    } else {
      scf.yield %alloc : memref<16x16xf16, #hivm.address_space<ub>>
    }

    %c0 = arith.constant 0 : index
    %c1024 = arith.constant 1024 : index
    %c128 = arith.constant 128 : index

    scf.for %iv = %c0 to %c1024 step %c128 {
      %if1 = scf.if %cond -> (memref<16x16xf16, #hivm.address_space<ub>>) {
        scf.yield %alloc : memref<16x16xf16, #hivm.address_space<ub>>
      } else {
        %local = memref.alloc() : memref<16x16xf16, #hivm.address_space<ub>>
        hivm.hir.vadd ins(%if0, %if0 : memref<16x16xf16, #hivm.address_space<ub>>,
                          memref<16x16xf16, #hivm.address_space<ub>>)
                    outs(%local : memref<16x16xf16, #hivm.address_space<ub>>)
        scf.yield %local : memref<16x16xf16, #hivm.address_space<ub>>
      }
      hivm.hir.vadd ins(%if1, %if1 : memref<16x16xf16, #hivm.address_space<ub>>,
                        memref<16x16xf16, #hivm.address_space<ub>>)
                  outs(%alloc : memref<16x16xf16, #hivm.address_space<ub>>)
    }

    %if2 = scf.if %cond -> (memref<16x16xf16, #hivm.address_space<ub>>) {
      %tmp2 = memref.alloc() : memref<16x16xf16, #hivm.address_space<ub>>
      hivm.hir.vadd ins(%alloc, %alloc : memref<16x16xf16, #hivm.address_space<ub>>,
                        memref<16x16xf16, #hivm.address_space<ub>>)
                  outs(%tmp2 : memref<16x16xf16, #hivm.address_space<ub>>)
      scf.yield %tmp2 : memref<16x16xf16, #hivm.address_space<ub>>
    } else {
      scf.yield %alloc : memref<16x16xf16, #hivm.address_space<ub>>
    }

    scf.for %iv2 = %c0 to %c1024 step %c128 {
      hivm.hir.vadd ins(%if2, %if2 : memref<16x16xf16, #hivm.address_space<ub>>,
                        memref<16x16xf16, #hivm.address_space<ub>>)
                  outs(%alloc : memref<16x16xf16, #hivm.address_space<ub>>)
    }

    hivm.hir.store ins(%alloc : memref<16x16xf16, #hivm.address_space<ub>>)
                   outs(%dst1 : memref<16x16xf16, #hivm.address_space<gm>>)
    hivm.hir.store ins(%collapse : memref<256xf16, #hivm.address_space<ub>>)
                   outs(%dst2 : memref<256xf16, #hivm.address_space<gm>>)
    return
  }
}
