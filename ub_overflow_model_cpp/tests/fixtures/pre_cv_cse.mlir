"builtin.module"() ({
  "func.func"() <{function_type = (memref<4xi32>, i1, index, index, index) -> (i32, i32), sym_name = "pre_cv_cse"}> ({
  ^bb0(%arg0: memref<4xi32>, %arg1: i1, %arg2: index, %arg3: index, %arg4: index):
    %0 = "arith.constant"() <{value = 0 : index}> : () -> index
    %1 = "arith.constant"() <{value = 7 : i32}> : () -> i32
    %2 = "arith.constant"() <{value = 9 : i32}> : () -> i32
    %3 = "arith.addi"(%1, %2) : (i32, i32) -> i32
    %4 = "arith.addi"(%2, %1) : (i32, i32) -> i32
    "memref.store"(%4, %arg0, %0) : (i32, memref<4xi32>, index) -> ()
    %5 = "memref.load"(%arg0, %0) : (memref<4xi32>, index) -> i32
    %6 = "memref.load"(%arg0, %0) : (memref<4xi32>, index) -> i32
    "memref.store"(%6, %arg0, %0) : (i32, memref<4xi32>, index) -> ()
    %7 = "memref.load"(%arg0, %0) : (memref<4xi32>, index) -> i32
    %8 = "arith.addi"(%1, %2) : (i32, i32) -> i32
    "scf.for"(%arg2, %arg3, %arg4) ({
    ^bb0(%arg5: index):
      %12 = "arith.addi"(%2, %1) : (i32, i32) -> i32
      "memref.store"(%12, %arg0, %arg5) : (i32, memref<4xi32>, index) -> ()
      "scf.yield"() : () -> ()
    }) : (index, index, index) -> ()
    "scf.for"(%arg2, %arg3, %arg4) ({
    ^bb0(%arg5: index):
      %12 = "arith.addi"(%arg5, %arg5) : (index, index) -> index
      %13 = "arith.index_cast"(%12) : (index) -> i32
      "memref.store"(%13, %arg0, %arg5) : (i32, memref<4xi32>, index) -> ()
      "scf.yield"() : () -> ()
    }) : (index, index, index) -> ()
    "scf.for"(%arg2, %arg3, %arg4) ({
    ^bb0(%arg5: index):
      %12 = "arith.addi"(%arg5, %arg5) : (index, index) -> index
      %13 = "arith.index_cast"(%12) : (index) -> i32
      "memref.store"(%13, %arg0, %arg5) : (i32, memref<4xi32>, index) -> ()
      "scf.yield"() : () -> ()
    }) : (index, index, index) -> ()
    %9 = "scf.if"(%arg1) ({
      %12 = "arith.constant"() <{value = 3 : i32}> : () -> i32
      "scf.yield"(%12) : (i32) -> ()
    }, {
      %12 = "arith.constant"() <{value = 4 : i32}> : () -> i32
      "scf.yield"(%12) : (i32) -> ()
    }) : (i1) -> i32
    %10 = "scf.if"(%arg1) ({
      %12 = "arith.constant"() <{value = 3 : i32}> : () -> i32
      "scf.yield"(%12) : (i32) -> ()
    }, {
      %12 = "arith.constant"() <{value = 4 : i32}> : () -> i32
      "scf.yield"(%12) : (i32) -> ()
    }) : (i1) -> i32
    %11 = "arith.addi"(%9, %10) : (i32, i32) -> i32
    "memref.store"(%11, %arg0, %0) : (i32, memref<4xi32>, index) -> ()
    %dead = "arith.addi"(%1, %1) : (i32, i32) -> i32
    "func.return"(%5, %7) : (i32, i32) -> ()
  }) : () -> ()
}) : () -> ()
