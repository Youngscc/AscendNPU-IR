module {
  func.func @test_mem_inplace_mb_and_mb(%src_gm: memref<16xf16, #hivm.address_space<gm>>,
                                        %dst_gm: memref<16xf16, #hivm.address_space<gm>>) attributes {hivm.func_core_type = #hivm.func_core_type<AIV>} {
    %c0 = arith.constant 0 : index
    %c4 = arith.constant 4 : index
    %c16 = arith.constant 16 : index
    scf.for %i0 = %c0 to %c16 step %c4 {
      %src_ub = memref.alloc() : memref<16xf16, #hivm.address_space<ub>>
      annotation.mark %src_ub {hivm.multi_buffer = 4 : i32} : memref<16xf16, #hivm.address_space<ub>>
      %dst_ub = memref.alloc() : memref<16xf16, #hivm.address_space<ub>>
      annotation.mark %dst_ub {hivm.multi_buffer = 4 : i32} : memref<16xf16, #hivm.address_space<ub>>
      hivm.hir.load ins(%src_gm : memref<16xf16, #hivm.address_space<gm>>)
                    outs(%src_ub : memref<16xf16, #hivm.address_space<ub>>)
      hivm.hir.vadd ins(%src_ub, %src_ub : memref<16xf16, #hivm.address_space<ub>>,
                        memref<16xf16, #hivm.address_space<ub>>)
                    outs(%dst_ub : memref<16xf16, #hivm.address_space<ub>>)
      hivm.hir.store ins(%dst_ub : memref<16xf16,#hivm.address_space<ub>>)
                     outs(%dst_gm: memref<16xf16,#hivm.address_space<gm>>)
    }
    return
  }
}
