"builtin.module"() ({
  "func.func"() <{function_type = (tensor<8xf16>) -> (), sym_name = "sink_op_use_order_aiv"}> ({
  ^bb0(%external: tensor<8xf16>):
    %c0 = "arith.constant"() {value = 0 : index} : () -> index
    %c1 = "arith.constant"() {value = 1 : index} : () -> index
    %value = "arith.constant"() {value = 0.0 : f16} : () -> f16
    %true = "arith.constant"() {value = true} : () -> i1
    %first_init = "tensor.empty"() : () -> tensor<8xf16>
    %first = "hivm.hir.vbrc"(%value, %first_init) <{broadcast_dims = array<i64>}> {case = "first_fill"} : (f16, tensor<8xf16>) -> tensor<8xf16>
    %second_init = "tensor.empty"() : () -> tensor<8xf16>
    %second = "hivm.hir.vbrc"(%value, %second_init) <{broadcast_dims = array<i64>}> {case = "second_fill"} : (f16, tensor<8xf16>) -> tensor<8xf16>
    "scf.for"(%c0, %c1, %c1) ({
    ^bb1(%iv: index):
      %sum = "hivm.hir.vadd"(%first, %first, %second) : (tensor<8xf16>, tensor<8xf16>, tensor<8xf16>) -> tensor<8xf16>
      "scf.yield"() : () -> ()
    }) : (index, index, index) -> ()
    "scf.for"(%c0, %c1, %c1) ({
    ^bb2(%iv2: index):
      %other_loop_sum = "hivm.hir.vadd"(%second, %external, %external) : (tensor<8xf16>, tensor<8xf16>, tensor<8xf16>) -> tensor<8xf16>
      "scf.yield"() : () -> ()
    }) : (index, index, index) -> ()
    %if_init = "tensor.empty"() : () -> tensor<8xf16>
    %if_only = "hivm.hir.vbrc"(%value, %if_init) <{broadcast_dims = array<i64>}> {case = "if_only_fill"} : (f16, tensor<8xf16>) -> tensor<8xf16>
    "scf.if"(%true) ({
    ^bb3:
      %if_sum = "hivm.hir.vadd"(%if_only, %external, %external) : (tensor<8xf16>, tensor<8xf16>, tensor<8xf16>) -> tensor<8xf16>
      "scf.yield"() : () -> ()
    }) : (i1) -> ()
    "func.return"() : () -> ()
  }) {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>} : () -> ()
}) : () -> ()
