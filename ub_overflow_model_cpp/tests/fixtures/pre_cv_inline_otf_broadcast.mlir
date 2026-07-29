"builtin.module"() ({
  "func.func"() <{function_type = (tensor<4x1xf32>, tensor<4x8xf32>, tensor<4x8xf32>) -> tensor<4x8xf32>, sym_name = "last_binary"}> ({
  ^bb0(%arg0: tensor<4x1xf32>, %arg1: tensor<4x8xf32>, %arg2: tensor<4x8xf32>):
    %0 = "tensor.empty"() : () -> tensor<4x8xf32>
    %1 = "hivm.hir.vbrc"(%arg0, %0) <{broadcast_dims = array<i64: 1>}> : (tensor<4x1xf32>, tensor<4x8xf32>) -> tensor<4x8xf32>
    %2 = "hivm.hir.vadd"(%1, %arg1, %arg2) <{broadcast = array<i64: 0>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<4x8xf32>, tensor<4x8xf32>, tensor<4x8xf32>) -> tensor<4x8xf32>
    "func.return"(%2) : (tensor<4x8xf32>) -> ()
  }) : () -> ()
  "func.func"() <{function_type = (tensor<4x1xf32>, tensor<4x8xf32>) -> tensor<4x8xf32>, sym_name = "last_unary"}> ({
  ^bb0(%arg0: tensor<4x1xf32>, %arg1: tensor<4x8xf32>):
    %0 = "tensor.empty"() : () -> tensor<4x8xf32>
    %1 = "hivm.hir.vbrc"(%arg0, %0) <{broadcast_dims = array<i64: 1>}> : (tensor<4x1xf32>, tensor<4x8xf32>) -> tensor<4x8xf32>
    %2 = "hivm.hir.vexp"(%1, %arg1) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<4x8xf32>, tensor<4x8xf32>) -> tensor<4x8xf32>
    "func.return"(%2) : (tensor<4x8xf32>) -> ()
  }) : () -> ()
  "func.func"() <{function_type = (tensor<4x1xf32>, tensor<4x8xf32>) -> tensor<4x8xf32>, sym_name = "last_relu"}> ({
  ^bb0(%arg0: tensor<4x1xf32>, %arg1: tensor<4x8xf32>):
    %0 = "tensor.empty"() : () -> tensor<4x8xf32>
    %1 = "hivm.hir.vbrc"(%arg0, %0) <{broadcast_dims = array<i64: 1>}> : (tensor<4x1xf32>, tensor<4x8xf32>) -> tensor<4x8xf32>
    %2 = "hivm.hir.vrelu"(%1, %arg1) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<4x8xf32>, tensor<4x8xf32>) -> tensor<4x8xf32>
    "func.return"(%2) : (tensor<4x8xf32>) -> ()
  }) : () -> ()
  "func.func"() <{function_type = (tensor<4x1xf32>, tensor<4x8xf32>) -> tensor<4x8xf32>, sym_name = "last_not_whitelisted"}> ({
  ^bb0(%arg0: tensor<4x1xf32>, %arg1: tensor<4x8xf32>):
    %0 = "tensor.empty"() : () -> tensor<4x8xf32>
    %1 = "hivm.hir.vbrc"(%arg0, %0) <{broadcast_dims = array<i64: 1>}> : (tensor<4x1xf32>, tensor<4x8xf32>) -> tensor<4x8xf32>
    %2 = "hivm.hir.vrec"(%1, %arg1) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<4x8xf32>, tensor<4x8xf32>) -> tensor<4x8xf32>
    "func.return"(%2) : (tensor<4x8xf32>) -> ()
  }) : () -> ()
  "func.func"() <{function_type = (tensor<4x1xf32>, tensor<4x8xi1>) -> tensor<4x8xi1>, sym_name = "last_visinf_not_whitelisted"}> ({
  ^bb0(%arg0: tensor<4x1xf32>, %arg1: tensor<4x8xi1>):
    %0 = "tensor.empty"() : () -> tensor<4x8xf32>
    %1 = "hivm.hir.vbrc"(%arg0, %0) <{broadcast_dims = array<i64: 1>}> : (tensor<4x1xf32>, tensor<4x8xf32>) -> tensor<4x8xf32>
    %2 = "hivm.hir.visinf"(%1, %arg1) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1>, transpose = array<i64>}> : (tensor<4x8xf32>, tensor<4x8xi1>) -> tensor<4x8xi1>
    "func.return"(%2) : (tensor<4x8xi1>) -> ()
  }) : () -> ()
  "func.func"() <{function_type = (tensor<4x1x8xf32>, tensor<4x8x8xf32>, tensor<4x8x8xf32>) -> tensor<4x8x8xf32>, sym_name = "middle_binary"}> ({
  ^bb0(%arg0: tensor<4x1x8xf32>, %arg1: tensor<4x8x8xf32>, %arg2: tensor<4x8x8xf32>):
    %0 = "tensor.empty"() : () -> tensor<4x8x8xf32>
    %1 = "hivm.hir.vbrc"(%arg0, %0) <{broadcast_dims = array<i64: 1>}> : (tensor<4x1x8xf32>, tensor<4x8x8xf32>) -> tensor<4x8x8xf32>
    %2 = "hivm.hir.vadd"(%1, %arg1, %arg2) <{broadcast = array<i64: 2>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<4x8x8xf32>, tensor<4x8x8xf32>, tensor<4x8x8xf32>) -> tensor<4x8x8xf32>
    "func.return"(%2) : (tensor<4x8x8xf32>) -> ()
  }) : () -> ()
  "func.func"() <{function_type = (tensor<4x1x8xf32>, tensor<4x8x8xf32>) -> tensor<4x8x8xf32>, sym_name = "middle_unary"}> ({
  ^bb0(%arg0: tensor<4x1x8xf32>, %arg1: tensor<4x8x8xf32>):
    %0 = "tensor.empty"() : () -> tensor<4x8x8xf32>
    %1 = "hivm.hir.vbrc"(%arg0, %0) <{broadcast_dims = array<i64: 1>}> : (tensor<4x1x8xf32>, tensor<4x8x8xf32>) -> tensor<4x8x8xf32>
    %2 = "hivm.hir.vexp"(%1, %arg1) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<4x8x8xf32>, tensor<4x8x8xf32>) -> tensor<4x8x8xf32>
    "func.return"(%2) : (tensor<4x8x8xf32>) -> ()
  }) : () -> ()
  "func.func"() <{function_type = (tensor<4x1xi16>, tensor<4x8xi16>) -> tensor<4x8xi16>, sym_name = "last_vabs_i16"}> ({
  ^bb0(%arg0: tensor<4x1xi16>, %arg1: tensor<4x8xi16>):
    %0 = "tensor.empty"() : () -> tensor<4x8xi16>
    %1 = "hivm.hir.vbrc"(%arg0, %0) <{broadcast_dims = array<i64: 1>}> : (tensor<4x1xi16>, tensor<4x8xi16>) -> tensor<4x8xi16>
    %2 = "hivm.hir.vabs"(%1, %arg1) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<4x8xi16>, tensor<4x8xi16>) -> tensor<4x8xi16>
    "func.return"(%2) : (tensor<4x8xi16>) -> ()
  }) : () -> ()
  "func.func"() <{function_type = (tensor<4x1xi64>, tensor<4x8xi64>, tensor<4x8xi64>) -> tensor<4x8xi64>, sym_name = "i64_no_inline"}> ({
  ^bb0(%arg0: tensor<4x1xi64>, %arg1: tensor<4x8xi64>, %arg2: tensor<4x8xi64>):
    %0 = "tensor.empty"() : () -> tensor<4x8xi64>
    %1 = "hivm.hir.vbrc"(%arg0, %0) <{broadcast_dims = array<i64: 1>}> : (tensor<4x1xi64>, tensor<4x8xi64>) -> tensor<4x8xi64>
    %2 = "hivm.hir.vadd"(%1, %arg1, %arg2) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<4x8xi64>, tensor<4x8xi64>, tensor<4x8xi64>) -> tensor<4x8xi64>
    "func.return"(%2) : (tensor<4x8xi64>) -> ()
  }) : () -> ()
  "func.func"() <{function_type = (tensor<1x1x8xf32>, tensor<4x8x8xf32>, tensor<4x8x8xf32>) -> tensor<4x8x8xf32>, sym_name = "multi_axis_no_inline"}> ({
  ^bb0(%arg0: tensor<1x1x8xf32>, %arg1: tensor<4x8x8xf32>, %arg2: tensor<4x8x8xf32>):
    %0 = "tensor.empty"() : () -> tensor<4x8x8xf32>
    %1 = "hivm.hir.vbrc"(%arg0, %0) <{broadcast_dims = array<i64: 0, 1>}> : (tensor<1x1x8xf32>, tensor<4x8x8xf32>) -> tensor<4x8x8xf32>
    %2 = "hivm.hir.vadd"(%1, %arg1, %arg2) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<4x8x8xf32>, tensor<4x8x8xf32>, tensor<4x8x8xf32>) -> tensor<4x8x8xf32>
    "func.return"(%2) : (tensor<4x8x8xf32>) -> ()
  }) : () -> ()
  "func.func"() <{function_type = (tensor<4x1xf32>, tensor<4x8xf32>, tensor<4x8xf32>) -> tensor<4x8xf32>, sym_name = "partial_inline"}> ({
  ^bb0(%arg0: tensor<4x1xf32>, %arg1: tensor<4x8xf32>, %arg2: tensor<4x8xf32>):
    %0 = "tensor.empty"() : () -> tensor<4x8xf32>
    %1 = "hivm.hir.vbrc"(%arg0, %0) <{broadcast_dims = array<i64: 1>}> : (tensor<4x1xf32>, tensor<4x8xf32>) -> tensor<4x8xf32>
    %2 = "hivm.hir.vadd"(%1, %1, %arg2) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<4x8xf32>, tensor<4x8xf32>, tensor<4x8xf32>) -> tensor<4x8xf32>
    "annotation.mark"(%1) <{effects = ["write"]}> {case = "retain_vbrc"} : (tensor<4x8xf32>) -> ()
    "func.return"(%2) : (tensor<4x8xf32>) -> ()
  }) : () -> ()
  "func.func"() <{function_type = (memref<4x1xf32>, memref<4x8xf32>, memref<4x8xf32>) -> (), sym_name = "buffer_no_inline"}> ({
  ^bb0(%arg0: memref<4x1xf32>, %arg1: memref<4x8xf32>, %arg2: memref<4x8xf32>):
    "hivm.hir.vbrc"(%arg0, %arg2) <{broadcast_dims = array<i64: 1>}> : (memref<4x1xf32>, memref<4x8xf32>) -> ()
    "hivm.hir.vadd"(%arg2, %arg1, %arg2) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (memref<4x8xf32>, memref<4x8xf32>, memref<4x8xf32>) -> ()
    "func.return"() : () -> ()
  }) : () -> ()
  "func.func"() <{function_type = (tensor<4x1xi32>, i32, tensor<4x8xi32>) -> tensor<4x8xi32>, sym_name = "shift_non950"}> ({
  ^bb0(%arg0: tensor<4x1xi32>, %arg1: i32, %arg2: tensor<4x8xi32>):
    %0 = "tensor.empty"() : () -> tensor<4x8xi32>
    %1 = "hivm.hir.vbrc"(%arg0, %0) <{broadcast_dims = array<i64: 1>}> : (tensor<4x1xi32>, tensor<4x8xi32>) -> tensor<4x8xi32>
    %2 = "hivm.hir.vshl"(%1, %arg1, %arg2) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<4x8xi32>, i32, tensor<4x8xi32>) -> tensor<4x8xi32>
    "func.return"(%2) : (tensor<4x8xi32>) -> ()
  }) : () -> ()
}) : () -> ()
