"builtin.module"() ({
  "func.func"() <{function_type = () -> (), sym_name = "used_decl"}> ({
    "func.return"() : () -> ()
  }) : () -> ()
  "func.func"() <{function_type = () -> (), sym_name = "unused_decl"}> : () -> ()
  "func.func"() <{function_type = () -> (), sym_name = "first_mix_aiv"}> ({
    "func.call"() <{callee = @used_decl}> : () -> ()
    "test.first"() : () -> ()
    "test.second"() : () -> ()
    "test.container"() ({
      "test.child"() : () -> ()
    }) : () -> ()
    "func.return"() : () -> ()
  }) {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv"} : () -> ()
  "func.func"() <{function_type = () -> (), sym_name = "second_mix_aiv"}> ({
    "func.return"() : () -> ()
  }) {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv"} : () -> ()
}) : () -> ()
