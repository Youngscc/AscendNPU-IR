// Post-split AIV fixture exercising loop-invariant subset hoisting (stage 12).
//
// The scf.for carries one loop iter arg (%t : tensor<8xf32>).  Inside the body
// %t flows through a tensor.extract_slice (the paired extraction) and back into a
// tensor.insert_slice whose result is yielded (the paired insertion), with fully
// loop-invariant indices.  This is exactly the matching extraction/insertion
// pair that upstream createLoopInvariantSubsetHoistingPass rewrites: the
// extraction moves before the loop, the insertion moves after it, and the loop
// gains a carried iter arg/result threading the updated subset.
//
// The lightweight model reproduces this deliberately narrow static form at the
// UB-semantic level and rejects other candidate forms transactionally.
"builtin.module"() ({
  "func.func"() <{function_type = () -> (), sym_name = "subset_hoist_lifetime_aiv"}> ({
  ^bb0:
    %c0 = "arith.constant"() {value = 0 : index} : () -> index
    %c1 = "arith.constant"() {value = 1 : index} : () -> index
    %c8 = "arith.constant"() {value = 8 : index} : () -> index
    %init = "tensor.empty"() {case = "subset_init"} : () -> tensor<8xf32>
    %loop = "scf.for"(%c0, %c8, %c1, %init) ({
    ^bb0(%iv: index, %t: tensor<8xf32>):
      %ext = "tensor.extract_slice"(%t) <{operandSegmentSizes = array<i32: 1, 0, 0, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: 4>, static_strides = array<i64: 1>}> {case = "subset_extract"} : (tensor<8xf32>) -> tensor<4xf32>
      %ins = "tensor.insert_slice"(%ext, %t) <{operandSegmentSizes = array<i32: 1, 1, 0, 0, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: 4>, static_strides = array<i64: 1>}> {case = "subset_insert"} : (tensor<4xf32>, tensor<8xf32>) -> tensor<8xf32>
      "scf.yield"(%ins) : (tensor<8xf32>) -> ()
    }) {case = "subset_loop"} : (index, index, index, tensor<8xf32>) -> tensor<8xf32>
    "func.return"() : () -> ()
  }) {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, hivm.part_of_mix, mix_mode = "aiv"} : () -> ()
}) : () -> ()
