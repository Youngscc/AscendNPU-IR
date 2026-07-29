#map = affine_map<()[s0, s1] -> (s0 + s1)>
#map1 = affine_map<()[s0] -> (s0)>
#map2 = affine_map<()[] -> (64)>
#map3 = affine_map<()[s0, s1, s2, s3] -> (s0 - (s3 * s1 + s2) floordiv s1, 0)>
"builtin.module"() ({
  "func.func"() <{function_type = (index, index, index, index) -> (), sym_name = "module_extended_canonicalizer"}> ({
  ^bb0(%arg0: index, %arg1: index, %arg2: index, %arg3: index):
    %sum = "affine.apply"(%arg0, %arg1) <{map = #map}> : (index, index) -> index
    %identity = "affine.apply"(%sum) <{map = #map1}> : (index) -> index
    %constant = "affine.apply"() <{map = #map2}> : () -> index
    %semi = "affine.max"(%arg0, %arg1, %arg2, %arg3) <{map = #map3}> : (index, index, index, index) -> index
    "annotation.mark"(%identity, %constant, %semi) <{effects = ["write"]}> {case = "results"} : (index, index, index) -> ()
    "func.return"() : () -> ()
  }) {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>} : () -> ()
}) : () -> ()
