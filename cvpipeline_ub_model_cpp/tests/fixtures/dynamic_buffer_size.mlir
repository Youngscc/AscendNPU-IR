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
}) : () -> ()
