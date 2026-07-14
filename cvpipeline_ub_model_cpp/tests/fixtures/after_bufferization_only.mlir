"builtin.module"() ({
  "func.func"() <{function_type = () -> (), sym_name = "bufferized_aiv"}> ({
    %buffer = "memref.alloc"() : () -> memref<4xf32, #hivm.mem_scope<ub>>
    "test.consume_memref"(%buffer) : (memref<4xf32, #hivm.mem_scope<ub>>) -> ()
    "func.return"() : () -> ()
  }) {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv"} : () -> ()
}) : () -> ()
