module {
  func.func @test_vabs_offset_unequal_not_inplace(%arg0: memref<8x16x32xf16, #hivm.address_space<gm>>, %arg1: memref<7x13x13xf16, #hivm.address_space<gm>>) attributes {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, hivm.storage_aligned} {
    hivm.hir.set_mask_norm

    %alloc = memref.alloc() : memref<8x16x32xf16, #hivm.address_space<ub>>
    hivm.hir.load ins(%arg0 : memref<8x16x32xf16, #hivm.address_space<gm>>) outs(%alloc : memref<8x16x32xf16, #hivm.address_space<ub>>)
    %subview = memref.subview %alloc[0, 1, 4] [7, 13, 13] [1, 1, 2] : memref<8x16x32xf16, #hivm.address_space<ub>> to memref<7x13x13xf16, strided<[512, 32, 2], offset: 36>, #hivm.address_space<ub>>

    %alloc_1 = memref.alloc() : memref<8x16x32xf16, #hivm.address_space<ub>>
    %subview_2 = memref.subview %alloc_1[1, 3, 6] [7, 13, 13] [1, 1, 2] : memref<8x16x32xf16, #hivm.address_space<ub>> to memref<7x13x13xf16, strided<[512, 32, 2], offset: 614>, #hivm.address_space<ub>>
    hivm.hir.vabs ins(%subview : memref<7x13x13xf16, strided<[512, 32, 2], offset: 36>, #hivm.address_space<ub>>) outs(%subview_2 : memref<7x13x13xf16, strided<[512, 32, 2], offset: 614>, #hivm.address_space<ub>>)
    hivm.hir.store ins(%subview_2 : memref<7x13x13xf16, strided<[512, 32, 2], offset: 614>, #hivm.address_space<ub>>) outs(%arg1 : memref<7x13x13xf16, #hivm.address_space<gm>>)
    return
  }
}
