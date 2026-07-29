"builtin.module"() ({
  "func.func"() <{function_type = (memref<1xui64>, memref<1xui64>, memref<1xui64>) -> (), sym_name = "single_point_unsigned_failure"}> ({
  ^bb0(%arg0: memref<1xui64>, %arg1: memref<1xui64>, %arg2: memref<1xui64>):
    "hivm.hir.vmax"(%arg0, %arg1, %arg2) <{broadcast = array<i64>, is_signed = false, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (memref<1xui64>, memref<1xui64>, memref<1xui64>) -> ()
    "func.return"() : () -> ()
  }) {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>} : () -> ()
}) : () -> ()
