module {
  func.func @b_liveness_five_live_buffers(%src: memref<32xf32, #hivm.address_space<gm>>, %dst: memref<32xf32, #hivm.address_space<gm>>) attributes {hivm.func_core_type = #hivm.func_core_type<AIV>} {
    %a0 = memref.alloc() : memref<32xf32, #hivm.address_space<ub>>
    %a1 = memref.alloc() : memref<32xf32, #hivm.address_space<ub>>
    %a2 = memref.alloc() : memref<32xf32, #hivm.address_space<ub>>
    %a3 = memref.alloc() : memref<32xf32, #hivm.address_space<ub>>
    %a4 = memref.alloc() : memref<32xf32, #hivm.address_space<ub>>
    hivm.hir.load ins(%src : memref<32xf32, #hivm.address_space<gm>>) outs(%a0 : memref<32xf32, #hivm.address_space<ub>>)
    hivm.hir.load ins(%src : memref<32xf32, #hivm.address_space<gm>>) outs(%a1 : memref<32xf32, #hivm.address_space<ub>>)
    hivm.hir.load ins(%src : memref<32xf32, #hivm.address_space<gm>>) outs(%a2 : memref<32xf32, #hivm.address_space<ub>>)
    hivm.hir.load ins(%src : memref<32xf32, #hivm.address_space<gm>>) outs(%a3 : memref<32xf32, #hivm.address_space<ub>>)
    hivm.hir.load ins(%src : memref<32xf32, #hivm.address_space<gm>>) outs(%a4 : memref<32xf32, #hivm.address_space<ub>>)
    %r0 = memref.alloc() : memref<32xf32, #hivm.address_space<ub>>
    %r1 = memref.alloc() : memref<32xf32, #hivm.address_space<ub>>
    %r2 = memref.alloc() : memref<32xf32, #hivm.address_space<ub>>
    %r3 = memref.alloc() : memref<32xf32, #hivm.address_space<ub>>
    hivm.hir.vadd ins(%a0, %a1 : memref<32xf32, #hivm.address_space<ub>>, memref<32xf32, #hivm.address_space<ub>>) outs(%r0 : memref<32xf32, #hivm.address_space<ub>>)
    hivm.hir.vadd ins(%r0, %a2 : memref<32xf32, #hivm.address_space<ub>>, memref<32xf32, #hivm.address_space<ub>>) outs(%r1 : memref<32xf32, #hivm.address_space<ub>>)
    hivm.hir.vadd ins(%r1, %a3 : memref<32xf32, #hivm.address_space<ub>>, memref<32xf32, #hivm.address_space<ub>>) outs(%r2 : memref<32xf32, #hivm.address_space<ub>>)
    hivm.hir.vadd ins(%r2, %a4 : memref<32xf32, #hivm.address_space<ub>>, memref<32xf32, #hivm.address_space<ub>>) outs(%r3 : memref<32xf32, #hivm.address_space<ub>>)
    hivm.hir.store ins(%r3 : memref<32xf32, #hivm.address_space<ub>>) outs(%dst : memref<32xf32, #hivm.address_space<gm>>)
    return
  }
}
