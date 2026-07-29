"builtin.module"() ({
  "func.func"() <{function_type = (i32) -> (), sym_name = "outer_canonicalizer"}> ({
  ^bb0(%arg0: i32):
    %c1 = "arith.constant"() <{value = 1 : i32}> : () -> i32
    %mul = "arith.muli"(%arg0, %c1) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %ext = "arith.extsi"(%mul) : (i32) -> i64
    %trunc = "arith.trunci"(%ext) : (i64) -> i32
    "annotation.mark"(%trunc) <{effects = ["write"]}> {case = "folded"} : (i32) -> ()
    %brc_src = "tensor.empty"() : () -> tensor<4x1xf32>
    %brc_dst = "tensor.empty"() : () -> tensor<4x1xf32>
    %brc = "hivm.hir.vbrc"(%brc_src, %brc_dst) <{broadcast_dims = array<i64: 1>}> : (tensor<4x1xf32>, tensor<4x1xf32>) -> tensor<4x1xf32>
    "annotation.mark"(%brc) <{effects = ["write"]}> {case = "redundant_vbrc"} : (tensor<4x1xf32>) -> ()
    %neg_inf = "arith.constant"() <{value = 0xFF800000 : f32}> : () -> f32
    %reduce_src = "tensor.empty"() : () -> tensor<4x8xf32>
    %reduce_init_empty = "tensor.empty"() : () -> tensor<4x1xf32>
    %reduce_init = "hivm.hir.vbrc"(%neg_inf, %reduce_init_empty) <{broadcast_dims = array<i64>}> : (f32, tensor<4x1xf32>) -> tensor<4x1xf32>
    %reduce = "hivm.hir.vreduce"(%reduce_src, %reduce_init) <{arith = #hivm.reduce_op<max>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 1>, tie_break_left = true, unsigned_src = false}> : (tensor<4x8xf32>, tensor<4x1xf32>) -> tensor<4x1xf32>
    "annotation.mark"(%reduce) <{effects = ["write"]}> {case = "vreduce_init"} : (tensor<4x1xf32>) -> ()
    "func.return"() : () -> ()
  }) {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>} : () -> ()
}) : () -> ()
