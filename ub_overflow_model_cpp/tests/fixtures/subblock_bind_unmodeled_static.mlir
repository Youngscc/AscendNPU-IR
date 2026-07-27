// A non-bubble strategy user keeps the marked store slice on %sub.  The real
// strict verifier rejects the TileAndBind candidate and falls back to the
// sub-block-0 guard path.
"builtin.module"() ({
  "func.func"() <{function_type = () -> (), sym_name = "subblock_unmodeled_aiv"}> ({
  ^bb0:
    %load_src = "tensor.empty"() : () -> tensor<16x16xf16>
    %load_out = "tensor.empty"() : () -> tensor<16x16xf16>
    %loaded = "hivm.hir.load"(%load_src, %load_out) <{operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>}> : (tensor<16x16xf16>, tensor<16x16xf16>) -> tensor<16x16xf16>
    %sub_init = "tensor.empty"() : () -> tensor<16x16xf16>
    %sub = "hivm.hir.vsub"(%loaded, %loaded, %sub_init) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x16xf16>, tensor<16x16xf16>, tensor<16x16xf16>) -> tensor<16x16xf16>
    %other_init = "tensor.empty"() : () -> tensor<16x16xf16>
    %kept = "tensor.insert_slice"(%sub, %other_init) <{operandSegmentSizes = array<i32: 1, 1, 0, 0, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 1, 1>}> : (tensor<16x16xf16>, tensor<16x16xf16>) -> tensor<16x16xf16>
    %store_dst = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x16xf16>
    "hivm.hir.store"(%sub, %store_dst) {case = "unmodeled_store"} : (tensor<16x16xf16>, memref<16x16xf16>) -> ()
    "func.return"() : () -> ()
  }) {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, hivm.part_of_mix, mix_mode = "aiv"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>>>} : () -> ()
