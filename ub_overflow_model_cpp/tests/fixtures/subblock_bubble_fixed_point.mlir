// Two structured users consume the same loop result. BubbleUpPattern may
// visit one marked slice while the other user still blocks it; the greedy
// driver must revisit the slice after both users have been cloned.
"builtin.module"() ({
  "func.func"() <{function_type = (tensor<64xf32>, memref<64xf32, #hivm.address_space<gm>>) -> (), sym_name = "subblock_bubble_fixed_point_aiv"}> ({
  ^bb0(%source: tensor<64xf32>, %destination: memref<64xf32, #hivm.address_space<gm>>):
    %c0 = "arith.constant"() <{value = 0 : index}> : () -> index
    %c1 = "arith.constant"() <{value = 1 : index}> : () -> index
    %init = "tensor.empty"() : () -> tensor<64xf32>
    %loop = "scf.for"(%c0, %c1, %c1, %init) ({
    ^bb0(%iv: index, %iter: tensor<64xf32>):
      %body_init = "tensor.empty"() : () -> tensor<64xf32>
      %body = "hivm.hir.vadd"(%iter, %source, %body_init) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
      "scf.yield"(%body) : (tensor<64xf32>) -> ()
    }) : (index, index, index, tensor<64xf32>) -> tensor<64xf32>
    %left = "hivm.hir.vadd"(%loop, %loop, %init) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
    %right = "hivm.hir.vadd"(%loop, %loop, %init) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
    %out = "hivm.hir.vadd"(%left, %right, %init) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
    "hivm.hir.store"(%out, %destination) : (tensor<64xf32>, memref<64xf32, #hivm.address_space<gm>>) -> ()
    "func.return"() : () -> ()
  }) {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, hivm.part_of_mix, mix_mode = "aiv"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>>>} : () -> ()
