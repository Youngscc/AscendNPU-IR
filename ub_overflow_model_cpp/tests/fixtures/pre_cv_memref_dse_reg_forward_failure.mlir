"builtin.module"() ({
  "func.func"() <{function_type = (memref<4xf32>, f32) -> f32, sym_name = "reg_forward_failure"}> ({
  ^bb0(%arg0: memref<4xf32>, %arg1: f32):
    %0 = "arith.constant"() <{value = 0 : index}> : () -> index
    "memref.store"(%arg1, %arg0, %0) : (f32, memref<4xf32>, index) -> ()
    %1 = "memref.load"(%arg0, %0) : (memref<4xf32>, index) -> f32
    "func.return"(%1) : (f32) -> ()
  }) : () -> ()
}) {hacc.target = #hacc.target<"Ascend950PR_9579">} : () -> ()
