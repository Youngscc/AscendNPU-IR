"builtin.module"() ({
  "func.func"() <{function_type = (tensor<4x1xi32>, i32, tensor<4x8xi32>) -> tensor<4x8xi32>, sym_name = "shift_950"}> ({
  ^bb0(%arg0: tensor<4x1xi32>, %arg1: i32, %arg2: tensor<4x8xi32>):
    %0 = "tensor.empty"() : () -> tensor<4x8xi32>
    %1 = "hivm.hir.vbrc"(%arg0, %0) <{broadcast_dims = array<i64: 1>}> : (tensor<4x1xi32>, tensor<4x8xi32>) -> tensor<4x8xi32>
    %2 = "hivm.hir.vshl"(%1, %arg1, %arg2) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<4x8xi32>, i32, tensor<4x8xi32>) -> tensor<4x8xi32>
    "func.return"(%2) : (tensor<4x8xi32>) -> ()
  }) : () -> ()
}) {hacc.target = #hacc.target<"Ascend950PR_9579">} : () -> ()
