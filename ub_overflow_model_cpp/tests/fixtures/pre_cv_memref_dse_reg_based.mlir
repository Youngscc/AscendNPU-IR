"builtin.module"() ({
  "func.func"() <{function_type = (memref<4xf32, #hivm.address_space<gm>>) -> (), sym_name = "remove_unused_load"}> ({
  ^bb0(%arg0: memref<4xf32, #hivm.address_space<gm>>):
    %0 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<4xf32, #hivm.address_space<ub>>
    "hivm.hir.load"(%arg0, %0) <{operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>}> : (memref<4xf32, #hivm.address_space<gm>>, memref<4xf32, #hivm.address_space<ub>>) -> ()
    "func.return"() : () -> ()
  }) : () -> ()
  "func.func"() <{function_type = (memref<4xf32, #hivm.address_space<gm>>, memref<4xf32, #hivm.address_space<gm>>) -> (), sym_name = "retain_indirect_load"}> ({
  ^bb0(%arg0: memref<4xf32, #hivm.address_space<gm>>, %arg1: memref<4xf32, #hivm.address_space<gm>>):
    %0 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<4xf32, #hivm.address_space<ub>>
    "hivm.hir.load"(%arg0, %0) <{operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>}> : (memref<4xf32, #hivm.address_space<gm>>, memref<4xf32, #hivm.address_space<ub>>) -> ()
    "hivm.hir.store"(%0, %arg1) : (memref<4xf32, #hivm.address_space<ub>>, memref<4xf32, #hivm.address_space<gm>>) -> ()
    "func.return"() : () -> ()
  }) : () -> ()
}) {hacc.target = #hacc.target<"Ascend950PR_9579">} : () -> ()
