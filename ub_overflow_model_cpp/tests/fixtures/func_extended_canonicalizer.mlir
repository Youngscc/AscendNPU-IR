#map = affine_map<()[s0, s1] -> (s0 + s1)>
#map1 = affine_map<()[s0] -> (s0)>
#map2 = affine_map<()[] -> (64)>
"builtin.module"() ({
  "func.func"() <{function_type = (index, index) -> (), sym_name = "func_canonicalizer_a"}> ({
  ^bb0(%arg0: index, %arg1: index):
    %0 = "affine.apply"(%arg0, %arg1) <{map = #map}> : (index, index) -> index
    %1 = "affine.apply"(%0) <{map = #map1}> : (index) -> index
    %2 = "affine.apply"() <{map = #map2}> : () -> index
    "annotation.mark"(%1, %2) <{effects = ["write"]}> : (index, index) -> ()
    "func.return"() : () -> ()
  }) {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>} : () -> ()
  "func.func"() <{function_type = (index, index) -> (), sym_name = "func_canonicalizer_b"}> ({
  ^bb0(%arg0: index, %arg1: index):
    %0 = "affine.apply"(%arg0, %arg1) <{map = #map}> : (index, index) -> index
    %1 = "affine.apply"(%0) <{map = #map1}> : (index) -> index
    %2 = "affine.apply"() <{map = #map2}> : () -> index
    "annotation.mark"(%1, %2) <{effects = ["write"]}> : (index, index) -> ()
    "func.return"() : () -> ()
  }) {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>} : () -> ()
}) : () -> ()
