"builtin.module"() ({
  "func.func"() <{function_type = () -> (), sym_name = "tile_cube"}> ({
  ^bb0:
    %c0 = "arith.constant"() {value = 0 : index} : () -> index
    %c1 = "arith.constant"() {value = 1 : index} : () -> index
    %true = "arith.constant"() {value = true} : () -> i1
    %lhs = "tensor.empty"() : () -> tensor<128x64xf16>
    %rhs = "tensor.empty"() : () -> tensor<4096x64xf16>
    %gm_out = "memref.alloc"() : () -> memref<128x4096xf32>
    "scf.for"(%c0, %c1, %c1) ({
    ^bb1(%iv: index):
      %acc = "tensor.empty"() : () -> tensor<128x4096xf32>
      %mmad = "hivm.hir.mmadL1"(%lhs, %rhs, %true, %acc) : (tensor<128x64xf16>, tensor<4096x64xf16>, i1, tensor<128x4096xf32>) -> tensor<128x4096xf32>
      %out_view = "memref.subview"(%gm_out) {static_offsets = [0, 0], static_sizes = [128, 4096], static_strides = [1, 1]} : (memref<128x4096xf32>) -> memref<128x4096xf32>
      "hivm.hir.fixpipe"(%mmad, %out_view) : (tensor<128x4096xf32>, memref<128x4096xf32>) -> ()
      "scf.yield"() : () -> ()
    }) {hivm.loop_core_type = #hivm.tcore_type<CUBE>} : (index, index, index) -> ()
    "func.return"() : () -> ()
  }) {hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<MIX>} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>>>} : () -> ()
