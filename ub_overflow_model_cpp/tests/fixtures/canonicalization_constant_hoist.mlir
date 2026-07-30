"builtin.module"() ({
  "func.func"() <{function_type = (i1) -> (), sym_name = "constant_hoist"}> ({
  ^bb0(%arg0: i1):
    %c0 = "arith.constant"() <{value = 0 : index}> : () -> index
    "scf.if"(%arg0) ({
      %c7 = "arith.constant"() <{value = 7 : i32}> : () -> i32
      %nested_c0 = "arith.constant"() <{value = 0 : index}> : () -> index
      "annotation.mark"(%c7) {case = "nested_seven"} : (i32) -> ()
      "annotation.mark"(%nested_c0) {case = "nested_zero"} : (index) -> ()
      "scf.yield"() : () -> ()
    }) : (i1) -> ()
    %c9 = "arith.constant"() <{value = 9 : i32}> : () -> i32
    "annotation.mark"(%c9) {case = "entry_after_region"} : (i32) -> ()
    "annotation.mark"(%c0) {case = "entry_zero"} : (index) -> ()
    "func.return"() : () -> ()
  }) : () -> ()
}) : () -> ()
