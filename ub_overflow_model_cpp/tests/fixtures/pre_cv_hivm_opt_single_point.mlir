"builtin.module"() ({
  "func.func"() <{function_type = (memref<1xf32>, memref<1xf32>, memref<1xf32>, memref<1xf32>, memref<1xf32, #hivm.address_space<ub>>, memref<1xf32, #hivm.address_space<ub>>) -> (), sym_name = "single_point_success"}> ({
  ^bb0(%arg0: memref<1xf32>, %arg1: memref<1xf32>, %arg2: memref<1xf32>, %arg3: memref<1xf32>, %arg4: memref<1xf32, #hivm.address_space<ub>>, %arg5: memref<1xf32, #hivm.address_space<ub>>):
    "hivm.hir.vadd"(%arg0, %arg1, %arg2) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (memref<1xf32>, memref<1xf32>, memref<1xf32>) -> ()
    "hivm.hir.vsub"(%arg0, %arg1, %arg2) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (memref<1xf32>, memref<1xf32>, memref<1xf32>) -> ()
    "hivm.hir.vmul"(%arg0, %arg1, %arg2) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (memref<1xf32>, memref<1xf32>, memref<1xf32>) -> ()
    "hivm.hir.vdiv"(%arg0, %arg1, %arg2) <{broadcast = array<i64>, isHP = false, isSigned = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (memref<1xf32>, memref<1xf32>, memref<1xf32>) -> ()
    "hivm.hir.vabs"(%arg0, %arg2) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (memref<1xf32>, memref<1xf32>) -> ()
    "hivm.hir.vsqrt"(%arg0, %arg2) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (memref<1xf32>, memref<1xf32>) -> ()
    "hivm.hir.vmax"(%arg0, %arg1, %arg2) <{broadcast = array<i64>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (memref<1xf32>, memref<1xf32>, memref<1xf32>) -> ()
    "hivm.hir.vmin"(%arg0, %arg1, %arg2) <{broadcast = array<i64>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (memref<1xf32>, memref<1xf32>, memref<1xf32>) -> ()
    "hivm.hir.vbrc"(%arg2, %arg3) <{broadcast_dims = array<i64: 0>}> : (memref<1xf32>, memref<1xf32>) -> ()
    %0 = "arith.constant"() <{value = 1.000000e+00 : f32}> : () -> f32
    "hivm.hir.vbrc"(%0, %arg3) <{broadcast_dims = array<i64>}> : (f32, memref<1xf32>) -> ()
    "hivm.hir.copy"(%arg4, %arg5) : (memref<1xf32, #hivm.address_space<ub>>, memref<1xf32, #hivm.address_space<ub>>) -> ()
    "func.return"() : () -> ()
  }) {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>} : () -> ()
  "func.func"() <{function_type = (memref<2xf32>, memref<2xf32>, memref<2xf32>, tensor<1xf32>, tensor<1xf32>, tensor<1xf32>) -> (), sym_name = "single_point_no_match"}> ({
  ^bb0(%arg0: memref<2xf32>, %arg1: memref<2xf32>, %arg2: memref<2xf32>, %arg3: tensor<1xf32>, %arg4: tensor<1xf32>, %arg5: tensor<1xf32>):
    "hivm.hir.vadd"(%arg0, %arg1, %arg2) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (memref<2xf32>, memref<2xf32>, memref<2xf32>) -> ()
    %0 = "hivm.hir.vadd"(%arg3, %arg4, %arg5) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1xf32>, tensor<1xf32>, tensor<1xf32>) -> tensor<1xf32>
    "annotation.mark"(%0) <{effects = ["write"]}> : (tensor<1xf32>) -> ()
    "func.return"() : () -> ()
  }) {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>} : () -> ()
  "func.func"() <{function_type = (memref<1xf16>, memref<1xf16>, memref<1xf16>, memref<1xf32>, memref<1xf32>) -> (), sym_name = "single_point_type_and_space_no_match"}> ({
  ^bb0(%arg0: memref<1xf16>, %arg1: memref<1xf16>, %arg2: memref<1xf16>, %arg3: memref<1xf32>, %arg4: memref<1xf32>):
    "hivm.hir.vadd"(%arg0, %arg1, %arg2) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (memref<1xf16>, memref<1xf16>, memref<1xf16>) -> ()
    "hivm.hir.copy"(%arg3, %arg4) : (memref<1xf32>, memref<1xf32>) -> ()
    "func.return"() : () -> ()
  }) {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>} : () -> ()
  "func.func"() <{function_type = (memref<1xi64>, memref<1xi64>, memref<1xi64>) -> (), sym_name = "single_point_integer"}> ({
  ^bb0(%arg0: memref<1xi64>, %arg1: memref<1xi64>, %arg2: memref<1xi64>):
    "hivm.hir.vadd"(%arg0, %arg1, %arg2) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (memref<1xi64>, memref<1xi64>, memref<1xi64>) -> ()
    "hivm.hir.vsub"(%arg0, %arg1, %arg2) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (memref<1xi64>, memref<1xi64>, memref<1xi64>) -> ()
    "hivm.hir.vmul"(%arg0, %arg1, %arg2) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (memref<1xi64>, memref<1xi64>, memref<1xi64>) -> ()
    "hivm.hir.vdiv"(%arg0, %arg1, %arg2) <{broadcast = array<i64>, isHP = false, isSigned = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (memref<1xi64>, memref<1xi64>, memref<1xi64>) -> ()
    "hivm.hir.vabs"(%arg0, %arg2) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (memref<1xi64>, memref<1xi64>) -> ()
    "hivm.hir.vmax"(%arg0, %arg1, %arg2) <{broadcast = array<i64>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (memref<1xi64>, memref<1xi64>, memref<1xi64>) -> ()
    "hivm.hir.vmin"(%arg0, %arg1, %arg2) <{broadcast = array<i64>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (memref<1xi64>, memref<1xi64>, memref<1xi64>) -> ()
    "func.return"() : () -> ()
  }) {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>} : () -> ()
  "func.func"() <{function_type = (memref<1xf32, #hivm.address_space<gm>>, memref<1xf32, #hivm.address_space<ub>>) -> (), sym_name = "single_point_load"}> ({
  ^bb0(%arg0: memref<1xf32, #hivm.address_space<gm>>, %arg1: memref<1xf32, #hivm.address_space<ub>>):
    "hivm.hir.load"(%arg0, %arg1) <{operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>}> : (memref<1xf32, #hivm.address_space<gm>>, memref<1xf32, #hivm.address_space<ub>>) -> ()
    "func.return"() : () -> ()
  }) {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hacc.no_io_alias, hivm.func_core_type = #hivm.func_core_type<AIV>} : () -> ()
  "func.func"() <{function_type = (memref<1xf32, #hivm.address_space<gm>>, memref<1xf32, #hivm.address_space<ub>>) -> (), sym_name = "single_point_load_without_no_alias"}> ({
  ^bb0(%arg0: memref<1xf32, #hivm.address_space<gm>>, %arg1: memref<1xf32, #hivm.address_space<ub>>):
    "hivm.hir.load"(%arg0, %arg1) <{operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>}> : (memref<1xf32, #hivm.address_space<gm>>, memref<1xf32, #hivm.address_space<ub>>) -> ()
    "func.return"() : () -> ()
  }) {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>} : () -> ()
  "func.func"() <{function_type = (memref<1xf32, #hivm.address_space<gm>>, memref<1xf32, #hivm.address_space<ub>>) -> (), sym_name = "single_point_invalid_memory_user"}> ({
  ^bb0(%arg0: memref<1xf32, #hivm.address_space<gm>>, %arg1: memref<1xf32, #hivm.address_space<ub>>):
    "hivm.hir.load"(%arg0, %arg1) <{operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>}> : (memref<1xf32, #hivm.address_space<gm>>, memref<1xf32, #hivm.address_space<ub>>) -> ()
    "hivm.hir.store"(%arg1, %arg0) : (memref<1xf32, #hivm.address_space<ub>>, memref<1xf32, #hivm.address_space<gm>>) -> ()
    "func.return"() : () -> ()
  }) {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hacc.no_io_alias, hivm.func_core_type = #hivm.func_core_type<AIV>} : () -> ()
  "func.func"() <{function_type = (memref<1xf32>, memref<1xf32>, memref<1xf32>) -> (), sym_name = "single_point_host"}> ({
  ^bb0(%arg0: memref<1xf32>, %arg1: memref<1xf32>, %arg2: memref<1xf32>):
    "hivm.hir.vadd"(%arg0, %arg1, %arg2) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (memref<1xf32>, memref<1xf32>, memref<1xf32>) -> ()
    "func.return"() : () -> ()
  }) {hacc.function_kind = #hacc.function_kind<HOST>} : () -> ()
}) : () -> ()
