"builtin.module"() ({
  "func.func"() <{function_type = (index, index, index, i1) -> (), sym_name = "canonicalize_iter_arg"}> ({
  ^bb0(%lb: index, %ub: index, %step: index, %condition: i1):
    %c0 = "arith.constant"() <{value = 0 : index}> : () -> index
    %c1 = "arith.constant"() <{value = 1 : index}> : () -> index
    %outside = "tensor.empty"() : () -> tensor<4xf32>
    %for:3 = "scf.for"(%lb, %ub, %step, %c0, %c0, %outside) ({
    ^bb0(%iv: index, %unchanged: index, %dead: index, %tensor: tensor<4xf32>):
      %dead_next = "arith.addi"(%dead, %c1) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      "scf.yield"(%unchanged, %dead_next, %outside) : (index, index, tensor<4xf32>) -> ()
    }) : (index, index, index, index, index, tensor<4xf32>) -> (index, index, tensor<4xf32>)
    "annotation.mark"(%for#0) <{effects = ["write"]}> {case = "for_unchanged"} : (index) -> ()
    "annotation.mark"(%for#2) <{effects = ["write"]}> {case = "for_external_yield"} : (tensor<4xf32>) -> ()
    %nested = "scf.for"(%lb, %ub, %step, %c0) ({
    ^bb0(%nested_iv: index, %nested_iter: index):
      %selected = "scf.if"(%condition) ({
        "scf.yield"(%nested_iter) : (index) -> ()
      }, {
        "scf.yield"(%nested_iter) : (index) -> ()
      }) : (i1) -> index
      "scf.yield"(%selected) : (index) -> ()
    }) : (index, index, index, index) -> index
    "annotation.mark"(%nested) <{effects = ["write"]}> {case = "for_nested_unchanged"} : (index) -> ()
    %while:2 = "scf.while"(%c0, %c0) ({
    ^bb0(%before_keep: index, %before_dead: index):
      "scf.condition"(%condition, %before_keep, %before_dead) : (i1, index, index) -> ()
    }, {
    ^bb0(%after_keep: index, %after_dead: index):
      %dead_while_next = "arith.addi"(%after_dead, %c1) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      "scf.yield"(%after_keep, %dead_while_next) : (index, index) -> ()
    }) : (index, index) -> (index, index)
    "annotation.mark"(%while#0) <{effects = ["write"]}> {case = "while_unchanged"} : (index) -> ()
    "func.return"() : () -> ()
  }) : () -> ()
  "func.func"() <{function_type = (i1) -> (), sym_name = "vector_backward_disabled"}> ({
  ^bb0(%condition: i1):
    %c0 = "arith.constant"() <{value = 0 : index}> : () -> index
    %c1 = "arith.constant"() <{value = 1 : index}> : () -> index
    %while:2 = "scf.while"(%c0, %c0) ({
    ^bb0(%before_keep: index, %before_dead: index):
      "scf.condition"(%condition, %before_keep, %before_dead) : (i1, index, index) -> ()
    }, {
    ^bb0(%after_keep: index, %after_dead: index):
      %dead_next = "arith.addi"(%after_dead, %c1) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      "scf.yield"(%after_keep, %dead_next) : (index, index) -> ()
    }) : (index, index) -> (index, index)
    "annotation.mark"(%while#0) <{effects = ["write"]}> {case = "vector_while_live"} : (index) -> ()
    "func.return"() : () -> ()
  }) {hivm.vector_function} : () -> ()
}) : () -> ()
