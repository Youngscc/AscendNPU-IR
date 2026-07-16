// Post-split AIV fixture exercising the fail-close for non-speculatable
// loop-invariant integer division/remainder in LICM (stage 11).
//
// The scf.for body carries a loop-invariant arith.divsi whose operands (%num,
// a constant, and %arg0, the function argument) are both defined outside the
// loop.  Upstream MLIR classifies arith.divsi as ConditionallySpeculatable
// (ArithOps.td/.cpp): it is NotSpeculatable unless the divisor is provably
// non-zero (and, for signed division, not -1).  The divisor %arg0 is not a
// constant, so upstream LICM -- whose hoist predicate is
// isMemoryEffectFree(op) && isSpeculatable(op) -- LEAVES the divsi in the loop.
//
// The model has no integer-range analysis and cannot prove speculatability, so
// it must fail closed (Incomplete, IR untouched) rather than hoisting the op
// and reporting a wrong Exact.
"builtin.module"() ({
  "func.func"() <{function_type = (i32) -> (), sym_name = "code_motion_nonspec_aiv"}> ({
  ^bb0(%arg0: i32):
    %c0 = "arith.constant"() {value = 0 : index} : () -> index
    %c1 = "arith.constant"() {value = 1 : index} : () -> index
    %c10 = "arith.constant"() {value = 10 : index} : () -> index
    %num = "arith.constant"() {value = 100 : i32} : () -> i32
    "scf.for"(%c0, %c10, %c1) ({
    ^bb1(%iv: index):
      %q = "arith.divsi"(%num, %arg0) {case = "nonspec_divsi"} : (i32, i32) -> i32
      "scf.yield"() : () -> ()
    }) {case = "nonspec_loop"} : (index, index, index) -> ()
    "func.return"() : () -> ()
  }) {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, hivm.part_of_mix, mix_mode = "aiv"} : () -> ()
}) : () -> ()
