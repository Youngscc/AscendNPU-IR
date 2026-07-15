// Post-split AIV fixture exercising loop-invariant code motion (stage 11).
//
// The function body contains an scf.for loop carrying:
//   * a side-effect-free, loop-invariant HIVM elementwise op (hoist_target)
//     whose operands are all defined outside the loop,
//   * a second pure op (hoist_dependent) that consumes the first op's result,
//     exercising the fixed-point iteration (it only becomes invariant once the
//     first op has been hoisted),
//   * a pure arithmetic op (loop_variant) whose operand is the induction
//     variable, which must remain inside the loop,
//   * a side-effecting store (stays_store) which the real pass never hoists.
//
// The model mirrors upstream createLoopInvariantCodeMotionPass: it hoists the
// two invariant pure definitions to before the loop (changing their gen-kill
// interval from live-across-the-loop to defined-before-the-loop) while leaving
// the variant and effectful operations in place, and reports Exact.
"builtin.module"() ({
  "func.func"() <{function_type = () -> (), sym_name = "code_motion_lifetime_aiv"}> ({
  ^bb0:
    %c0 = "arith.constant"() {value = 0 : index} : () -> index
    %c1 = "arith.constant"() {value = 1 : index} : () -> index
    %c10 = "arith.constant"() {value = 10 : index} : () -> index
    %invariant_source = "tensor.empty"() {case = "invariant_source"} : () -> tensor<16xf16>
    %dst = "memref.alloc"() : () -> memref<16xf16>
    "scf.for"(%c0, %c10, %c1) ({
    ^bb1(%iv: index):
      %hoisted_abs = "hivm.hir.vabs"(%invariant_source, %invariant_source) <{operandSegmentSizes = array<i32: 1, 1>}> {case = "hoist_target"} : (tensor<16xf16>, tensor<16xf16>) -> tensor<16xf16>
      %dependent = "hivm.hir.vabs"(%hoisted_abs, %hoisted_abs) <{operandSegmentSizes = array<i32: 1, 1>}> {case = "hoist_dependent"} : (tensor<16xf16>, tensor<16xf16>) -> tensor<16xf16>
      %variant = "arith.addi"(%iv, %c1) {case = "loop_variant"} : (index, index) -> index
      "hivm.hir.store"(%dependent, %dst) {case = "stays_store"} : (tensor<16xf16>, memref<16xf16>) -> ()
      "scf.yield"() : () -> ()
    }) {case = "lifetime_loop"} : (index, index, index) -> ()
    "func.return"() : () -> ()
  }) {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, hivm.part_of_mix, mix_mode = "aiv"} : () -> ()
}) : () -> ()
