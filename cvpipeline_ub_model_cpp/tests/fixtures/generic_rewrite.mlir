"builtin.module"() ({
  "func.func"() <{function_type = () -> (), sym_name = "rewrite"}> ({
  ^bb0:
    %dead = "test.dead"() : () -> tensor<4xf32>
    %old = "test.old"() : () -> tensor<4xf32>
    %new = "test.new"() : () -> tensor<4xf32>
    %dps = "test.dps"(%old, %old) : (tensor<4xf32>, tensor<4xf32>) -> tensor<4xf32>
    "cf.br"(%old) [^bb1] : (tensor<4xf32>) -> ()
  ^bb1(%forwarded: tensor<4xf32>):
    "test.use"(%forwarded) : (tensor<4xf32>) -> ()
    "func.return"() : () -> ()
  }) {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv"} : () -> ()
}) : () -> ()
