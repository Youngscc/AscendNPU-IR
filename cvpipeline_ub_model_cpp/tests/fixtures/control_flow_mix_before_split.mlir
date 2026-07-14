"builtin.module"() ({
  "func.func"() <{function_type = () -> (), sym_name = "control"}> ({
  ^bb0:
    %base = "tensor.empty"() : () -> tensor<4xf32>
    %lhs = "tensor.empty"() : () -> tensor<4xf32>
    %rhs = "tensor.empty"() : () -> tensor<4xf32>
    %cube = "hivm.hir.mmadL1"(%lhs, %rhs, %base) : (tensor<4xf32>, tensor<4xf32>, tensor<4xf32>) -> tensor<4xf32>
    "annotation.mark"(%cube) {"DuplicateTensorExtractForCube::replacementLabel" = 1 : i32} : (tensor<4xf32>) -> ()
    %dps_out = "hivm.hir.vadd"(%cube, %base, %base) <{case = "dps"}> : (tensor<4xf32>, tensor<4xf32>, tensor<4xf32>) -> tensor<4xf32>
    %lb = "arith.constant"() <{value = 0 : index}> : () -> index
    %ub = "arith.constant"() <{value = 1 : index}> : () -> index
    %step = "arith.constant"() <{value = 1 : index}> : () -> index
    %new_extract = "tensor.extract"(%base, %lb) {"DuplicateTensorExtractForCube::newExtractLabel" = 1 : i32} : (tensor<4xf32>, index) -> f32
    %loop = "scf.for"(%lb, %ub, %step, %base) ({
    ^bb0(%iv: index, %iter: tensor<4xf32>):
      "scf.yield"(%iter) : (tensor<4xf32>) -> ()
    }) {hivm.loop_core_type = #hivm.tcore_type<CUBE>, tcoretype = #hivm.tcore_type<VECTOR>} : (index, index, index, tensor<4xf32>) -> tensor<4xf32>
    %loop_out = "hivm.hir.vsub"(%loop, %base, %base) <{case = "loop"}> : (tensor<4xf32>, tensor<4xf32>, tensor<4xf32>) -> tensor<4xf32>
    %condition = "arith.constant"() <{value = true}> : () -> i1
    %if_value = "scf.if"(%condition) ({
      "scf.yield"(%base) : (tensor<4xf32>) -> ()
    }, {
      "scf.yield"(%base) : (tensor<4xf32>) -> ()
    }) {tcoretype = #hivm.tcore_type<CUBE>} : (i1) -> tensor<4xf32>
    %if_out = "hivm.hir.vmul"(%if_value, %base, %base) <{case = "if"}> : (tensor<4xf32>, tensor<4xf32>, tensor<4xf32>) -> tensor<4xf32>
    %scope_value = "scope.scope"() ({
      "scope.yield"(%base) : (tensor<4xf32>) -> ()
    }) {hivm.loop_core_type = #hivm.tcore_type<CUBE>} : () -> tensor<4xf32>
    %scope_out = "hivm.hir.vabs"(%scope_value, %base) <{case = "scope"}> : (tensor<4xf32>, tensor<4xf32>) -> tensor<4xf32>
    %custom = "hivm.hir.custom"(%base, %base) : (tensor<4xf32>, tensor<4xf32>) -> tensor<4xf32>
    %custom_out = "hivm.hir.vadd"(%custom, %base, %base) <{case = "custom"}> : (tensor<4xf32>, tensor<4xf32>, tensor<4xf32>) -> tensor<4xf32>
    "func.return"() : () -> ()
  }) {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<MIX>, mix_mode = "mix"} : () -> ()
  "func.func"() <{function_type = () -> (), sym_name = "second"}> ({
  ^bb0:
    %init = "tensor.empty"() : () -> tensor<4xf32>
    %value = "hivm.hir.vabs"(%init, %init) : (tensor<4xf32>, tensor<4xf32>) -> tensor<4xf32>
    "func.return"() : () -> ()
  }) {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<MIX>, mix_mode = "mix"} : () -> ()
}) : () -> ()
