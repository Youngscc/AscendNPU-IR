"builtin.module"() ({
  "func.func"() <{function_type = (tensor<4xf32>) -> (tensor<4xf32>, tensor<4xf32>), sym_name = "private_pair", sym_visibility = "private"}> ({
  ^bb0(%arg: tensor<4xf32>):
    %pair:2 = "scope.scope"() ({
      %first = "tensor.expand_shape"(%arg) {case = "nested_scope_first"} : (tensor<4xf32>) -> tensor<4xf32>
      "scope.return"(%first, %arg) : (tensor<4xf32>, tensor<4xf32>) -> ()
    }) {case = "nested_callee_scope"} : () -> (tensor<4xf32>, tensor<4xf32>)
    "func.return"(%pair#0, %pair#1) : (tensor<4xf32>, tensor<4xf32>) -> ()
  }) {sym_visibility = "private"} : () -> ()
  "func.func"() <{function_type = () -> (), sym_name = "public_decl"}> : () -> ()
  "func.func"() <{function_type = () -> (), sym_name = "noinline_helper", sym_visibility = "private"}> ({
    "func.return"() : () -> ()
  }) {no_inline} : () -> ()
  "func.func"() <{function_type = () -> (), sym_name = "caller"}> ({
    %empty = "tensor.empty"() {case = "inline_input"} : () -> tensor<4xf32>
    %called:2 = "func.call"(%empty) <{callee = @private_pair}> {case = "inline_call"} : (tensor<4xf32>) -> (tensor<4xf32>, tensor<4xf32>)
    %use = "hivm.hir.vadd"(%called#0, %called#1, %empty) <{operandSegmentSizes = array<i32: 2, 1>}> {case = "inline_user"} : (tensor<4xf32>, tensor<4xf32>, tensor<4xf32>) -> tensor<4xf32>
    "func.call"() <{callee = @public_decl}> {case = "public_call"} : () -> ()
    "func.call"() <{callee = @noinline_helper}> {case = "noinline_call", no_inline} : () -> ()
    "func.return"() : () -> ()
  }) : () -> ()
}) : () -> ()
