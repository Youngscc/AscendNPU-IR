// AIV function whose `hivm.hir.store` carries a static-shaped tensor source
// produced by a vector op.  The real TileAndBindSubBlock pass derives a tiling
// dimension for the store source and would wrap the body in a sub-block loop,
// slicing the store operands in half.  The lightweight model recognizes the
// eligibility but cannot reproduce the bubble-up-extract-slice transformation
// exactly, so the stage reports the applicable form as incomplete.
"builtin.module"() ({
  "func.func"() <{function_type = () -> (), sym_name = "subblock_bind_success_aiv"}> ({
  ^bb0:
    %load_src = "memref.alloc"() : () -> memref<16x16xf16>
    %load_out = "tensor.empty"() : () -> tensor<16x16xf16>
    %loaded = "hivm.hir.load"(%load_src, %load_out) : (memref<16x16xf16>, tensor<16x16xf16>) -> tensor<16x16xf16>
    %sum_init = "tensor.empty"() : () -> tensor<16x16xf16>
    %sum = "hivm.hir.vadd"(%loaded, %loaded, %sum_init) : (tensor<16x16xf16>, tensor<16x16xf16>, tensor<16x16xf16>) -> tensor<16x16xf16>
    %store_dst = "memref.alloc"() : () -> memref<16x16xf16>
    "hivm.hir.store"(%sum, %store_dst) {case = "tileable_store"} : (tensor<16x16xf16>, memref<16x16xf16>) -> ()
    "func.return"() : () -> ()
  }) {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, hivm.part_of_mix, mix_mode = "aiv"} : () -> ()
}) : () -> ()
