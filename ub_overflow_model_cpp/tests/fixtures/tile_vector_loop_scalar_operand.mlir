"builtin.module"() ({
  "func.func"() <{function_type = () -> (), sym_name = "tile_vector_scalar_operand"}> ({
  ^bb0:
    %c0 = "arith.constant"() {value = 0 : index} : () -> index
    %c1 = "arith.constant"() {value = 1 : index} : () -> index
    %src = "tensor.empty"() : () -> tensor<1x128xi32>
    %vector_dst = "memref.alloc"() : () -> memref<1x128xi32>
    %dst_view = "memref.subview"(%vector_dst) {static_offsets = [0, 0], static_sizes = [1, 128], static_strides = [1, 1]} : (memref<1x128xi32>) -> memref<1x128xi32>
    "scf.for"(%c0, %c1, %c1) ({
    ^bb1(%iv: index):
      %offset = "arith.index_cast"(%iv) : (index) -> i32
      %slice = "tensor.extract_slice"(%src) {static_offsets = [0, 0], static_sizes = [1, 128], static_strides = [1, 1]} : (tensor<1x128xi32>) -> tensor<1x128xi32>
      %sum_init = "tensor.empty"() : () -> tensor<1x128xi32>
      %sum = "hivm.hir.vadd"(%slice, %offset, %sum_init) : (tensor<1x128xi32>, i32, tensor<1x128xi32>) -> tensor<1x128xi32>
      "hivm.hir.store"(%sum, %dst_view) : (tensor<1x128xi32>, memref<1x128xi32>) -> ()
      "scf.yield"() : () -> ()
    }) {hivm.loop_core_type = #hivm.tcore_type<VECTOR>} : (index, index, index) -> ()
    "func.return"() : () -> ()
  }) {hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<MIX>} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>>>} : () -> ()
