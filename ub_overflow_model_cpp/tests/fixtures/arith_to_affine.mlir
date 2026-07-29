"builtin.module"() ({
  "func.func"() <{function_type = (index, index, i32, i32) -> (index, index, index, index, index, index, index, index, index, index, i32), sym_name = "arith_to_affine"}> ({
  ^bb0(%arg0: index, %arg1: index, %arg2: i32, %arg3: i32):
    %add = "arith.addi"(%arg0, %arg1) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %sub = "arith.subi"(%arg0, %arg1) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %mul = "arith.muli"(%arg0, %arg1) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %ceil = "arith.ceildivsi"(%arg0, %arg1) : (index, index) -> index
    %div = "arith.divsi"(%arg0, %arg1) : (index, index) -> index
    %rem = "arith.remsi"(%arg0, %arg1) : (index, index) -> index
    %maxs = "arith.maxsi"(%arg0, %arg1) : (index, index) -> index
    %maxu = "arith.maxui"(%arg0, %arg1) : (index, index) -> index
    %mins = "arith.minsi"(%arg0, %arg1) : (index, index) -> index
    %minu = "arith.minui"(%arg0, %arg1) : (index, index) -> index
    %i32_add = "arith.addi"(%arg2, %arg3) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "func.return"(%add, %sub, %mul, %ceil, %div, %rem, %maxs, %maxu, %mins, %minu, %i32_add) : (index, index, index, index, index, index, index, index, index, index, i32) -> ()
  }) {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>} : () -> ()
}) : () -> ()
