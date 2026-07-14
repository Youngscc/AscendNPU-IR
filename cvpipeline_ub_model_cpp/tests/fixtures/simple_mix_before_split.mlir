"builtin.module"() ({
  "func.func"() <{function_type = () -> (), sym_name = "kernel"}> ({
  ^bb0:
    %lhs = "tensor.empty"() : () -> tensor<4xf32>
    %rhs = "tensor.empty"() : () -> tensor<4xf32>
    %cube_init = "tensor.empty"() : () -> tensor<4xf32>
    %cube = "hivm.hir.mmadL1"(%lhs, %rhs, %cube_init) : (tensor<4xf32>, tensor<4xf32>, tensor<4xf32>) -> tensor<4xf32>
    %workspace = "memref_ext.alloc_workspace"() : () -> memref<4xf32>
    "hivm.hir.fixpipe"(%cube, %workspace) : (tensor<4xf32>, memref<4xf32>) -> ()
    %vector_init = "tensor.empty"() : () -> tensor<4xf32>
    %loaded = "hivm.hir.load"(%workspace, %vector_init) <{tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<4xf32>, tensor<4xf32>) -> tensor<4xf32>
    %sum_init = "tensor.empty"() : () -> tensor<4xf32>
    %sum = "hivm.hir.vadd"(%loaded, %loaded, %sum_init) : (tensor<4xf32>, tensor<4xf32>, tensor<4xf32>) -> tensor<4xf32>
    "func.return"() : () -> ()
  }) {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<MIX>, mix_mode = "mix"} : () -> ()
}) : () -> ()
