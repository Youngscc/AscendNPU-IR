"builtin.module"() ({
  "func.func"() <{function_type = () -> (), sym_name = "small_aiv"}> ({
  ^bb0:
    %c0 = "arith.constant"() <{value = 0 : f32}> : () -> f32
    %idx = "arith.constant"() <{value = 0 : index}> : () -> index
    %b = "memref.alloc"() : () -> memref<2048xf32, #hivm.address_space<UB>>
    "memref.store"(%c0, %b, %idx) : (f32, memref<2048xf32, #hivm.address_space<UB>>, index) -> ()
    "func.return"() : () -> ()
  }) {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv"} : () -> ()
  "func.func"() <{function_type = () -> (), sym_name = "large_aiv"}> ({
  ^bb0:
    %c0 = "arith.constant"() <{value = 0 : f32}> : () -> f32
    %idx = "arith.constant"() <{value = 0 : index}> : () -> index
    %b = "memref.alloc"() : () -> memref<3072xf32, #hivm.address_space<UB>>
    "memref.store"(%c0, %b, %idx) : (f32, memref<3072xf32, #hivm.address_space<UB>>, index) -> ()
    "func.return"() : () -> ()
  }) {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv"} : () -> ()
}) : () -> ()
