"builtin.module"() ({
  "func.func"() <{function_type = () -> (), sym_name = "dynamic_sizes"}> ({
  ^bb0:
    %d = "arith.constant"() {value = 8 : index} : () -> index
    %seed = "memref.alloc"(%d) : (index) -> memref<?xf16, #hivm.address_space<UB>>
    "annotation.mark"(%seed) {buffer_size_in_byte = 64 : i64} : (memref<?xf16, #hivm.address_space<UB>>) -> ()
    %inferred = "memref.alloc"(%d) : (index) -> memref<?xf32, #hivm.address_space<UB>>
    %static = "memref.alloc"() : () -> memref<8xf16, #hivm.address_space<UB>>
    "annotation.mark"(%static) {buffer_size_in_byte = 16 : i64} : (memref<8xf16, #hivm.address_space<UB>>) -> ()
    "test.consume"(%seed, %inferred, %static) : (memref<?xf16, #hivm.address_space<UB>>, memref<?xf32, #hivm.address_space<UB>>, memref<8xf16, #hivm.address_space<UB>>) -> ()
    "func.return"() : () -> ()
  }) {enable_auto_mark_buffer_size, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv"} : () -> ()
  "func.func"() <{function_type = () -> (), sym_name = "conflicting_sizes"}> ({
  ^bb0:
    %d = "arith.constant"() {value = 8 : index} : () -> index
    %alloc = "memref.alloc"(%d) : (index) -> memref<?xf16, #hivm.address_space<UB>>
    "annotation.mark"(%alloc) {buffer_size_in_byte = 64 : i64} : (memref<?xf16, #hivm.address_space<UB>>) -> ()
    "annotation.mark"(%alloc) {buffer_size_in_byte = 128 : i64} : (memref<?xf16, #hivm.address_space<UB>>) -> ()
    "test.consume"(%alloc) : (memref<?xf16, #hivm.address_space<UB>>) -> ()
    "func.return"() : () -> ()
  }) {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv"} : () -> ()
  "func.func"() <{function_type = () -> (), sym_name = "canonicalize"}> ({
  ^bb0:
    %zero = "arith.constant"() {value = 0 : index} : () -> index
    %one = "arith.constant"() {value = 1 : index} : () -> index
    %two = "arith.constant"() {value = 2 : i32} : () -> i32
    %sum0 = "arith.addi"(%two, %two) : (i32, i32) -> i32
    %sum1 = "arith.addi"(%two, %two) : (i32, i32) -> i32
    %empty = "tensor.empty"() : () -> tensor<4xf32>
    "annotation.mark"(%empty) : (tensor<4xf32>) -> ()
    %loop = "scf.for"(%zero, %one, %one, %sum1) ({
    ^bb0(%iv: index, %iter: i32):
      "scf.yield"(%iter) : (i32) -> ()
    }) : (index, index, index, i32) -> i32
    "test.consume"(%sum0, %loop) : (i32, i32) -> ()
    "func.return"() : () -> ()
  }) {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv"} : () -> ()
  "func.func"() <{function_type = () -> (), sym_name = "constant_bounds"}> ({
  ^bb0:
    %seven = "arith.constant"() {value = 7 : index} : () -> index
    %four = "arith.constant"() {value = 4 : index} : () -> index
    %two = "arith.constant"() {value = 20 : index} : () -> index
    %ceil = "arith.ceildivui"(%seven, %four) : (index, index) -> index
    %max = "arith.maxui"(%ceil, %two) : (index, index) -> index
    %alloc = "memref.alloc"(%max) : (index) -> memref<?xf16, strided<[2], offset: 1>, #hivm.address_space<UB>>
    "annotation.mark"(%alloc) {buffer_size_in_byte = 32 : i64, multi_buffer = 2 : i64} : (memref<?xf16, strided<[2], offset: 1>, #hivm.address_space<UB>>) -> ()
    %stack = "memref.alloca"(%max) : (index) -> memref<?xf32, #hivm.address_space<UB>>
    "test.consume"(%alloc, %stack) : (memref<?xf16, strided<[2], offset: 1>, #hivm.address_space<UB>>, memref<?xf32, #hivm.address_space<UB>>) -> ()
    "func.return"() : () -> ()
  }) {enable_auto_mark_buffer_size, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv"} : () -> ()
  "func.func"() <{function_type = () -> (), sym_name = "affine_loop_vreduce"}> ({
  ^bb0:
    %lb = "arith.constant"() {value = 0 : index} : () -> index
    %ub = "arith.constant"() {value = 4 : index} : () -> index
    %zero = "arith.constant"() {value = 0.0 : f32} : () -> f32
    %src = "tensor.empty"() : () -> tensor<4xf32>
    %init_empty = "tensor.empty"() : () -> tensor<1xf32>
    %filled = "hivm.hir.vbrc"(%zero, %init_empty) <{operandSegmentSizes = array<i32: 1, 1, 0>, broadcast_dims = array<i64>}> : (f32, tensor<1xf32>) -> tensor<1xf32>
    %loop = "affine.for"(%lb, %ub, %filled) <{lowerBoundMap = affine_map<(d0) -> (d0)>, upperBoundMap = affine_map<(d0) -> (d0)>, step = 1 : i64, operandSegmentSizes = array<i32: 1, 1, 1>}> ({
    ^bb0(%iv: index, %iter: tensor<1xf32>):
      %reduced = "hivm.hir.vreduce"(%src, %iter) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 0>, arith = #hivm.reduce_op<sum>}> : (tensor<4xf32>, tensor<1xf32>) -> tensor<1xf32>
      "affine.yield"(%iter) : (tensor<1xf32>) -> ()
    }) : (index, index, tensor<1xf32>) -> tensor<1xf32>
    "test.consume"(%loop) : (tensor<1xf32>) -> ()
    "func.return"() : () -> ()
  }) {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv"} : () -> ()
}) : () -> ()
