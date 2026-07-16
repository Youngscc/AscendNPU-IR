module {
  func.func @b_boundary_align_31_32_33_bytes(%src31: memref<31xi8, #hivm.address_space<gm>>, %dst31: memref<31xi8, #hivm.address_space<gm>>, %src32: memref<32xi8, #hivm.address_space<gm>>, %dst32: memref<32xi8, #hivm.address_space<gm>>, %src33: memref<33xi8, #hivm.address_space<gm>>, %dst33: memref<33xi8, #hivm.address_space<gm>>) attributes {hivm.func_core_type = #hivm.func_core_type<AIV>} {
    %a31 = memref.alloc() : memref<31xi8, #hivm.address_space<ub>>
    %a32 = memref.alloc() : memref<32xi8, #hivm.address_space<ub>>
    %a33 = memref.alloc() : memref<33xi8, #hivm.address_space<ub>>
    hivm.hir.load ins(%src31 : memref<31xi8, #hivm.address_space<gm>>) outs(%a31 : memref<31xi8, #hivm.address_space<ub>>)
    hivm.hir.load ins(%src32 : memref<32xi8, #hivm.address_space<gm>>) outs(%a32 : memref<32xi8, #hivm.address_space<ub>>)
    hivm.hir.load ins(%src33 : memref<33xi8, #hivm.address_space<gm>>) outs(%a33 : memref<33xi8, #hivm.address_space<ub>>)
    hivm.hir.store ins(%a31 : memref<31xi8, #hivm.address_space<ub>>) outs(%dst31 : memref<31xi8, #hivm.address_space<gm>>)
    hivm.hir.store ins(%a32 : memref<32xi8, #hivm.address_space<ub>>) outs(%dst32 : memref<32xi8, #hivm.address_space<gm>>)
    hivm.hir.store ins(%a33 : memref<33xi8, #hivm.address_space<ub>>) outs(%dst33 : memref<33xi8, #hivm.address_space<gm>>)
    return
  }
}
