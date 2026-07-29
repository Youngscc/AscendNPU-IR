#map = affine_map<(d0, d1) -> (2, d1 - d0)>
#map1 = affine_map<(d0) -> (256, -d0 + 256)>
#map2 = affine_map<(d0) -> (d0 * -64 + 128, 64)>
"builtin.module"() ({
  "func.func"() <{function_type = (memref<i64>) -> (), sym_name = "scf_for_canonicalize_min"}> ({
  ^bb0(%arg0: memref<i64>):
    %0 = "arith.constant"() <{value = 0 : index}> : () -> index
    %1 = "arith.constant"() <{value = 2 : index}> : () -> index
    %2 = "arith.constant"() <{value = 4 : index}> : () -> index
    "scf.for"(%0, %2, %1) ({
    ^bb0(%arg1: index):
      %3 = "affine.min"(%arg1, %2) <{map = #map}> : (index, index) -> index
      %4 = "arith.index_cast"(%3) : (index) -> i64
      "memref.store"(%4, %arg0) : (i64, memref<i64>) -> ()
      "scf.yield"() : () -> ()
    }) : (index, index, index) -> ()
    "func.return"() : () -> ()
  }) : () -> ()
  "func.func"() <{function_type = (memref<i64>) -> (), sym_name = "scf_for_canonicalize_partly"}> ({
  ^bb0(%arg0: memref<i64>):
    %0 = "arith.constant"() <{value = 1 : index}> : () -> index
    %1 = "arith.constant"() <{value = 16 : index}> : () -> index
    %2 = "arith.constant"() <{value = 256 : index}> : () -> index
    "scf.for"(%0, %2, %1) ({
    ^bb0(%arg1: index):
      %3 = "affine.min"(%arg1) <{map = #map1}> : (index) -> index
      %4 = "arith.index_cast"(%3) : (index) -> i64
      "memref.store"(%4, %arg0) : (i64, memref<i64>) -> ()
      "scf.yield"() : () -> ()
    }) : (index, index, index) -> ()
    "func.return"() : () -> ()
  }) : () -> ()
  "func.func"() <{function_type = (tensor<?x?xf32>, tensor<10x10xf32>) -> (index, index), sym_name = "tensor_dim_of_iter_arg_and_result"}> ({
  ^bb0(%arg0: tensor<?x?xf32>, %arg1: tensor<10x10xf32>):
    %0 = "arith.constant"() <{value = 0 : index}> : () -> index
    %1 = "arith.constant"() <{value = 1 : index}> : () -> index
    %2 = "arith.constant"() <{value = 10 : index}> : () -> index
    %3:2 = "scf.for"(%0, %2, %1, %arg0, %0) ({
    ^bb0(%arg2: index, %arg3: tensor<?x?xf32>, %arg4: index):
      %4 = "tensor.dim"(%arg3, %0) : (tensor<?x?xf32>, index) -> index
      %5 = "tensor.insert_slice"(%arg1, %arg3) <{operandSegmentSizes = array<i32: 1, 1, 0, 0, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: 10, 10>, static_strides = array<i64: 1, 1>}> : (tensor<10x10xf32>, tensor<?x?xf32>) -> tensor<?x?xf32>
      "scf.yield"(%5, %4) : (tensor<?x?xf32>, index) -> ()
    }) : (index, index, index, tensor<?x?xf32>, index) -> (tensor<?x?xf32>, index)
    %6 = "tensor.dim"(%3#0, %0) : (tensor<?x?xf32>, index) -> index
    "func.return"(%3#1, %6) : (index, index) -> ()
  }) : () -> ()
  "func.func"() <{function_type = (memref<i64>) -> (), sym_name = "scf_forall_canonicalize_min"}> ({
  ^bb0(%arg0: memref<i64>):
    %0 = "arith.constant"() <{value = 2 : index}> : () -> index
    "scf.forall"(%0) <{operandSegmentSizes = array<i32: 0, 1, 0, 0>, staticLowerBound = array<i64: 0>, staticStep = array<i64: 1>, staticUpperBound = array<i64: -9223372036854775808>}> ({
    ^bb0(%arg1: index):
      %1 = "affine.min"(%arg1) <{map = #map2}> : (index) -> index
      %2 = "arith.index_cast"(%1) : (index) -> i64
      "memref.store"(%2, %arg0) : (i64, memref<i64>) -> ()
      "scf.forall.in_parallel"() ({
      ^bb0:
      }) : () -> ()
    }) : (index) -> ()
    "func.return"() : () -> ()
  }) : () -> ()
}) : () -> ()
