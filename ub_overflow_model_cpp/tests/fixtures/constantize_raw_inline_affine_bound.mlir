"builtin.module"() ({
  "func.func"() <{function_type = (index, index) -> (), sym_name = "constantize_raw_inline_affine_bound"}> ({
  ^bb0(%other: index, %base: index):
    %end = "affine.min"(%other, %base) <{map = affine_map<()[s0, s1] -> (s1 + 32, s0)>}> : (index, index) -> index
    %extent = "affine.apply"(%end, %base) <{map = affine_map<()[s0, s1] -> (s0 - s1)>}> : (index, index) -> index
    "test.consume"(%extent) : (index) -> ()
    "func.return"() : () -> ()
  }) {hacc.function_kind = #hacc.function_kind<DEVICE>} : () -> ()
}) : () -> ()
