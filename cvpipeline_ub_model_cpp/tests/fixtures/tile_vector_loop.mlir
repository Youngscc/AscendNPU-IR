"builtin.module"() ({
  "func.func"() <{function_type = () -> (), sym_name = "tile_vector"}> ({
  ^bb0:
    %c0 = "arith.constant"() {value = 0 : index} : () -> index
    %c1 = "arith.constant"() {value = 1 : index} : () -> index
    %src = "tensor.empty"() : () -> tensor<1x128xf16>
    %vector_dst = "memref.alloc"() : () -> memref<1x128xf16>
    %dst_view = "memref.subview"(%vector_dst) {static_offsets = [0, 0], static_sizes = [1, 128], static_strides = [1, 1]} : (memref<1x128xf16>) -> memref<1x128xf16>
    "scf.for"(%c0, %c1, %c1) ({
    ^bb1(%iv: index):
      %slice = "tensor.extract_slice"(%src) {static_offsets = [0, 0], static_sizes = [1, 128], static_strides = [1, 1]} : (tensor<1x128xf16>) -> tensor<1x128xf16>
      %init = "tensor.empty"() : () -> tensor<1x128xf16>
      %loaded = "hivm.hir.load"(%slice, %init) : (tensor<1x128xf16>, tensor<1x128xf16>) -> tensor<1x128xf16>
      %sum_init = "tensor.empty"() : () -> tensor<1x128xf16>
      %sum = "hivm.hir.vadd"(%loaded, %loaded, %sum_init) : (tensor<1x128xf16>, tensor<1x128xf16>, tensor<1x128xf16>) -> tensor<1x128xf16>
      "hivm.hir.store"(%sum, %dst_view) : (tensor<1x128xf16>, memref<1x128xf16>) -> ()
      "scf.yield"() : () -> ()
    }) {hivm.loop_core_type = #hivm.tcore_type<VECTOR>} : (index, index, index) -> ()
    %unrelated = "tensor.empty"() {case = "unrelated"} : () -> tensor<1x128xf16>
    "func.return"() : () -> ()
  }) {hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<MIX>} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>>>} : () -> ()
