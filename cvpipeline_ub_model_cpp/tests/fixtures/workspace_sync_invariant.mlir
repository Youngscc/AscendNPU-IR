"builtin.module"() ({
  "func.func"() <{function_type = () -> (), sym_name = "workspace_sync"}> ({
  ^bb0:
    %workspace = "memref_ext.alloc_workspace"() {alias_source = "arg0", case = "workspace", workspace_offset = 0 : i64} : () -> memref<256xi8, #hivm.address_space<GM>>
    %load = "hivm.hir.load"(%workspace) {case = "aiv_load", tcoretype = #hivm.tcore_type<VECTOR>} : (memref<256xi8, #hivm.address_space<GM>>) -> tensor<64xf32>
    "hivm.hir.sync_block"() {case = "operandless_sync"} : () -> ()
    "test.consume"(%load) : (tensor<64xf32>) -> ()
    "func.return"() : () -> ()
  }) {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv"} : () -> ()
}) : () -> ()
