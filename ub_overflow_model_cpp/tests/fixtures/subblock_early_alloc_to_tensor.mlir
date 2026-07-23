"builtin.module"() ({
  "func.func"() <{function_type = () -> (), sym_name = "subblock_early_aiv"}> ({
  ^bb0:
    %alloc = "memref.alloc"() : () -> memref<16x16xf16>
    %tensor = "bufferization.to_tensor"(%alloc) : (memref<16x16xf16>) -> tensor<16x16xf16>
    "test.consume"(%tensor) : (tensor<16x16xf16>) -> ()
    "func.return"() : () -> ()
  }) {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, hivm.part_of_mix, mix_mode = "aiv"} : () -> ()
}) : () -> ()
