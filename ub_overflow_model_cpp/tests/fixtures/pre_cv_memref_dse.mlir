"builtin.module"() ({
  "func.func"() <{function_type = (memref<4xf32>, memref<1xf32>, memref<4xf32>, f32) -> (f32, f32, f32, f32), sym_name = "forward_and_barrier"}> ({
  ^bb0(%arg0: memref<4xf32>, %arg1: memref<1xf32>, %arg2: memref<4xf32>, %arg3: f32):
    %0 = "arith.constant"() <{value = 0 : index}> : () -> index
    %1 = "arith.constant"() <{value = 1 : index}> : () -> index
    %2 = "arith.constant"() <{value = 4.200000e+01 : f32}> : () -> f32
    %3 = "arith.constant"() <{value = 8.400000e+01 : f32}> : () -> f32
    "memref.store"(%2, %arg0, %0) : (f32, memref<4xf32>, index) -> ()
    %4 = "memref.load"(%arg0, %0) : (memref<4xf32>, index) -> f32
    "memref.store"(%3, %arg0, %1) : (f32, memref<4xf32>, index) -> ()
    %5 = "memref.load"(%arg0, %0) : (memref<4xf32>, index) -> f32
    "memref.store"(%2, %arg1, %0) : (f32, memref<1xf32>, index) -> ()
    %6 = "memref.load"(%arg1, %1) : (memref<1xf32>, index) -> f32
    "memref.store"(%2, %arg2, %0) : (f32, memref<4xf32>, index) -> ()
    "hivm.hir.vbrc"(%arg3, %arg2) <{broadcast_dims = array<i64>}> : (f32, memref<4xf32>) -> ()
    %7 = "memref.load"(%arg2, %0) : (memref<4xf32>, index) -> f32
    "func.return"(%4, %5, %6, %7) : (f32, f32, f32, f32) -> ()
  }) : () -> ()
  "func.func"() <{function_type = (memref<4xf32>, i1, f32) -> (f32, f32), sym_name = "nested_level"}> ({
  ^bb0(%arg0: memref<4xf32>, %arg1: i1, %arg2: f32):
    %0 = "arith.constant"() <{value = 0 : index}> : () -> index
    "memref.store"(%arg2, %arg0, %0) : (f32, memref<4xf32>, index) -> ()
    %1 = "scf.if"(%arg1) ({
      %3 = "arith.constant"() <{value = 1.000000e+00 : f32}> : () -> f32
      "memref.store"(%3, %arg0, %0) : (f32, memref<4xf32>, index) -> ()
      %4 = "memref.load"(%arg0, %0) : (memref<4xf32>, index) -> f32
      "scf.yield"(%4) : (f32) -> ()
    }, {
      %3 = "arith.constant"() <{value = 2.000000e+00 : f32}> : () -> f32
      "memref.store"(%3, %arg0, %0) : (f32, memref<4xf32>, index) -> ()
      %4 = "memref.load"(%arg0, %0) : (memref<4xf32>, index) -> f32
      "scf.yield"(%4) : (f32) -> ()
    }) : (i1) -> f32
    %2 = "memref.load"(%arg0, %0) : (memref<4xf32>, index) -> f32
    "func.return"(%1, %2) : (f32, f32) -> ()
  }) : () -> ()
  "func.func"() <{function_type = (memref<4xf32>, f32) -> f32, sym_name = "view_alias"}> ({
  ^bb0(%arg0: memref<4xf32>, %arg1: f32):
    %0 = "arith.constant"() <{value = 0 : index}> : () -> index
    %1 = "memref.cast"(%arg0) : (memref<4xf32>) -> memref<?xf32>
    "memref.store"(%arg1, %1, %0) : (f32, memref<?xf32>, index) -> ()
    %2 = "memref.load"(%arg0, %0) : (memref<4xf32>, index) -> f32
    "func.return"(%2) : (f32) -> ()
  }) : () -> ()
  "func.func"() <{function_type = (f32) -> (), sym_name = "dead_alloc"}> ({
  ^bb0(%arg0: f32):
    %0 = "arith.constant"() <{value = 0 : index}> : () -> index
    %1 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<4xf32>
    "memref.store"(%arg0, %1, %0) : (f32, memref<4xf32>, index) -> ()
    "func.return"() : () -> ()
  }) : () -> ()
  "func.func"() <{function_type = (f32) -> (), sym_name = "dead_alloc_subview"}> ({
  ^bb0(%arg0: f32):
    %0 = "arith.constant"() <{value = 0 : index}> : () -> index
    %1 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<4xf32>
    %2 = "memref.subview"(%1) <{operandSegmentSizes = array<i32: 1, 0, 0, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: 4>, static_strides = array<i64: 1>}> : (memref<4xf32>) -> memref<4xf32, strided<[1]>>
    "memref.store"(%arg0, %2, %0) : (f32, memref<4xf32, strided<[1]>>, index) -> ()
    "func.return"() : () -> ()
  }) : () -> ()
  "func.func"() <{function_type = (memref<4xf32>) -> (), sym_name = "call_sink", sym_visibility = "private"}> ({
  ^bb0(%arg0: memref<4xf32>):
    "func.return"() : () -> ()
  }) : () -> ()
  "func.func"() <{function_type = (memref<4xf32>, f32) -> f32, sym_name = "call_barrier"}> ({
  ^bb0(%arg0: memref<4xf32>, %arg1: f32):
    %0 = "arith.constant"() <{value = 0 : index}> : () -> index
    "memref.store"(%arg1, %arg0, %0) : (f32, memref<4xf32>, index) -> ()
    "func.call"(%arg0) <{callee = @call_sink}> : (memref<4xf32>) -> ()
    %1 = "memref.load"(%arg0, %0) : (memref<4xf32>, index) -> f32
    "func.return"(%1) : (f32) -> ()
  }) : () -> ()
}) : () -> ()
