module attributes {hacc.target = #hacc.target<"Ascend910_9382">} {
  func.func @_attn_fwd(
      %q: memref<65536xf16, #hivm.address_space<gm>>,
      %k: memref<65536xf16, #hivm.address_space<gm>>,
      %out: memref<65536xf16, #hivm.address_space<gm>>,
      %program_num_x: i32,
      %program_num_y: i32,
      %program_num_z: i32,
      %program_id_x: i32,
      %program_id_y: i32,
      %program_id_z: i32)
      attributes {
        hacc.entry,
        hacc.function_kind = #hacc.function_kind<DEVICE>,
        hivm.func_core_type = #hivm.func_core_type<AIV>
      } {
    %q_ub = memref.alloc() : memref<65536xf16, #hivm.address_space<ub>>
    %k_ub = memref.alloc() : memref<65536xf16, #hivm.address_space<ub>>
    %acc_ub = memref.alloc() : memref<65536xf16, #hivm.address_space<ub>>

    hivm.hir.load
      ins(%q : memref<65536xf16, #hivm.address_space<gm>>)
      outs(%q_ub : memref<65536xf16, #hivm.address_space<ub>>)

    hivm.hir.load
      ins(%k : memref<65536xf16, #hivm.address_space<gm>>)
      outs(%k_ub : memref<65536xf16, #hivm.address_space<ub>>)

    hivm.hir.vadd
      ins(%q_ub, %k_ub : memref<65536xf16, #hivm.address_space<ub>>,
                         memref<65536xf16, #hivm.address_space<ub>>)
      outs(%acc_ub : memref<65536xf16, #hivm.address_space<ub>>)

    hivm.hir.store
      ins(%acc_ub : memref<65536xf16, #hivm.address_space<ub>>)
      outs(%out : memref<65536xf16, #hivm.address_space<gm>>)

    return
  }
}
