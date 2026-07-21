// AIV function whose `hivm.hir.store` carries a static-shaped tensor source
// produced by a vector op.  The real TileAndBindSubBlock pass derives a tiling
// dimension for the store source and wraps the body in a sub-block loop,
// slicing the store operands in half.  GM arguments keep the computation
// externally observable so the real suffix cannot erase the UB buffers.
"builtin.module"() ({
  "func.func"() <{function_type = (tensor<16x16xf16>, memref<16x16xf16, #hivm.address_space<gm>>, tensor<1xf32>, memref<1xf32, #hivm.address_space<gm>>) -> (), sym_name = "subblock_bind_success_aiv"}> ({
  ^bb0(%load_src: tensor<16x16xf16>, %store_dst: memref<16x16xf16, #hivm.address_space<gm>>, %scalar: tensor<1xf32>, %scalar_dst: memref<1xf32, #hivm.address_space<gm>>):
    %load_out = "tensor.empty"() : () -> tensor<16x16xf16>
    %loaded = "hivm.hir.load"(%load_src, %load_out) <{operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>}> : (tensor<16x16xf16>, tensor<16x16xf16>) -> tensor<16x16xf16>
    %sum_init = "tensor.empty"() : () -> tensor<16x16xf16>
    %sum = "hivm.hir.vadd"(%loaded, %loaded, %sum_init) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x16xf16>, tensor<16x16xf16>, tensor<16x16xf16>) -> tensor<16x16xf16>
    "hivm.hir.store"(%sum, %store_dst) {case = "tileable_store"} : (tensor<16x16xf16>, memref<16x16xf16, #hivm.address_space<gm>>) -> ()
    // This store has no selected tiling dimension.  A successful bind must
    // still guard it so that the two sub-blocks do not both write it.
    "hivm.hir.store"(%scalar, %scalar_dst) {case = "untiled_store"} : (tensor<1xf32>, memref<1xf32, #hivm.address_space<gm>>) -> ()
    "func.return"() : () -> ()
  }) {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, hivm.part_of_mix, mix_mode = "aiv"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>>>} : () -> ()
