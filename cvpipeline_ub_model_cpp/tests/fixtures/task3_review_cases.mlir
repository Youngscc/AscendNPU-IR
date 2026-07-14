"builtin.module"() ({
  "func.func"() <{function_type = () -> (), sym_name = "review"}> ({
  ^bb0:
    %base = "tensor.empty"() : () -> tensor<4xf32>
    %other = "tensor.empty"() : () -> tensor<4xf32>
    %zero = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %lb = "arith.constant"() <{value = 0 : index}> : () -> index
    %ub = "arith.constant"() <{value = 1 : index}> : () -> index
    %step = "arith.constant"() <{value = 1 : index}> : () -> index
    %loop0, %loop1 = "scf.for"(%lb, %ub, %step, %base, %other) ({
    ^bb0(%iv: index, %iter0: tensor<4xf32>, %iter1: tensor<4xf32>):
      "scf.yield"(%iter0, %iter1) : (tensor<4xf32>, tensor<4xf32>) -> ()
    }) {hivm.loop_core_type = #hivm.tcore_type<CUBE>} : (index, index, index, tensor<4xf32>, tensor<4xf32>) -> (tensor<4xf32>, tensor<4xf32>)
    %loop_use0 = "hivm.hir.vabs"(%loop0, %base) <{case = "loop0"}> : (tensor<4xf32>, tensor<4xf32>) -> tensor<4xf32>
    %loop_use1 = "hivm.hir.vabs"(%loop1, %other) <{case = "loop1"}> : (tensor<4xf32>, tensor<4xf32>) -> tensor<4xf32>
    %dps0, %dps1 = "hivm.hir.custom"(%base, %base, %other) <{case = "dps_multi", tcoretype = #hivm.tcore_type<CUBE>}> : (tensor<4xf32>, tensor<4xf32>, tensor<4xf32>) -> (tensor<4xf32>, tensor<4xf32>)
    %dps_use0 = "hivm.hir.vabs"(%dps0, %base) <{case = "dps0"}> : (tensor<4xf32>, tensor<4xf32>) -> tensor<4xf32>
    %dps_use1 = "hivm.hir.vabs"(%dps1, %other) <{case = "dps1"}> : (tensor<4xf32>, tensor<4xf32>) -> tensor<4xf32>
    %scope_tensor, %scope_int, %scope_external, %scope_unused = "scope.scope"() ({
      %internal_tensor = "tensor.empty"() : () -> tensor<4xf32>
      %internal_int = "arith.constant"() <{value = 7 : i32}> : () -> i32
      "scope.yield"(%internal_tensor, %internal_int, %base, %internal_int) : (tensor<4xf32>, i32, tensor<4xf32>, i32) -> ()
    }) {hivm.loop_core_type = #hivm.tcore_type<CUBE>} : () -> (tensor<4xf32>, i32, tensor<4xf32>, i32)
    %scope_tensor_use = "hivm.hir.vabs"(%scope_tensor, %base) <{case = "scope_tensor"}> : (tensor<4xf32>, tensor<4xf32>) -> tensor<4xf32>
    %scope_int_use = "arith.addi"(%scope_int, %zero) <{case = "scope_int"}> : (i32, i32) -> i32
    %scope_external_use = "hivm.hir.vabs"(%scope_external, %base) <{case = "scope_external"}> : (tensor<4xf32>, tensor<4xf32>) -> tensor<4xf32>
    %condition = "arith.constant"() <{value = true}> : () -> i1
    %if_value = "scf.if"(%condition) ({
      "scf.yield"(%base) : (tensor<4xf32>) -> ()
    }, {
      "scf.yield"(%other) : (tensor<4xf32>) -> ()
    }) {tcoretype = #hivm.tcore_type<CUBE>} : (i1) -> tensor<4xf32>
    %if_use = "hivm.hir.vabs"(%if_value, %base) <{case = "if_mismatch"}> : (tensor<4xf32>, tensor<4xf32>) -> tensor<4xf32>
    %gm = "memref.alloc"() : () -> memref<4xf32, #hivm.address_space<gm>>
    %ubuf = "memref.alloc"() : () -> memref<4xf32, #hivm.address_space<ub>>
    %l1 = "memref.alloc"() : () -> memref<4xf32, #hivm.address_space<l1>>
    "hivm.hir.load"(%gm, %ubuf) <{case = "load_vector"}> : (memref<4xf32, #hivm.address_space<gm>>, memref<4xf32, #hivm.address_space<ub>>) -> ()
    "hivm.hir.load"(%gm, %l1) <{case = "load_cube"}> : (memref<4xf32, #hivm.address_space<gm>>, memref<4xf32, #hivm.address_space<l1>>) -> ()
    "hivm.hir.copy"(%ubuf, %gm) <{case = "copy_vector"}> : (memref<4xf32, #hivm.address_space<ub>>, memref<4xf32, #hivm.address_space<gm>>) -> ()
    "hivm.hir.copy"(%gm, %l1) <{case = "copy_cube"}> : (memref<4xf32, #hivm.address_space<gm>>, memref<4xf32, #hivm.address_space<l1>>) -> ()
    "hivm.hir.vbrc"(%zero, %ubuf) <{case = "vbrc_vector"}> : (i32, memref<4xf32, #hivm.address_space<ub>>) -> ()
    "hivm.hir.vbrc"(%zero, %l1) <{case = "vbrc_cube"}> : (i32, memref<4xf32, #hivm.address_space<l1>>) -> ()
    %matmul = "hivm.hir.matmul"(%base, %other, %base) : (tensor<4xf32>, tensor<4xf32>, tensor<4xf32>) -> tensor<4xf32>
    %matmul_use = "hivm.hir.vabs"(%matmul, %base) <{case = "matmul"}> : (tensor<4xf32>, tensor<4xf32>) -> tensor<4xf32>
    %context_unknown = "hivm.hir.custom"(%base, %base) <{case = "context_unknown"}> : (tensor<4xf32>, tensor<4xf32>) -> tensor<4xf32>
    "func.return"() : () -> ()
  }) {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<MIX>, mix_mode = "mix"} : () -> ()
}) : () -> ()
