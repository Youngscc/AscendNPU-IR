"builtin.module"() ({
  "func.func"() <{function_type = (index, index) -> (), sym_name = "constantize_raw_affine_bound"}> ({
  ^bb0(%base: index, %limit: index):
    %tile_end = "affine.apply"(%base) <{map = affine_map<()[s0] -> (s0 + 32)>}> : (index) -> index
    %clamped_limit = "affine.max"(%base, %limit) <{map = affine_map<()[s0, s1] -> (s0, s1)>}> : (index, index) -> index
    %end = "affine.min"(%tile_end, %clamped_limit) <{map = affine_map<()[s0, s1] -> (s0, s1)>}> : (index, index) -> index
    %extent = "affine.apply"(%end, %base) <{map = affine_map<()[s0, s1] -> (s0 - s1)>}> : (index, index) -> index
    "test.consume"(%extent) : (index) -> ()
    "func.return"() : () -> ()
  }) {hacc.function_kind = #hacc.function_kind<DEVICE>} : () -> ()
}) : () -> ()
