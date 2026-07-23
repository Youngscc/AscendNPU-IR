"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xf32>, memref<?xi64>, memref<?xf32>, i32, i32, i32) -> (), sym_name = "embedding_kernel"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xf32>, %arg4: memref<?xi64>, %arg5: memref<?xf32>, %arg6: i32, %arg7: i32, %arg8: i32):
    %0 = "arith.constant"() <{value = 1 : index}> : () -> index
    %1 = "arith.constant"() <{value = 5 : index}> : () -> index
    %2 = "arith.constant"() <{value = 0 : index}> : () -> index
    "hivm.hir.set_mask_norm"() : () -> ()
    %3 = "arith.muli"(%arg6, %arg7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %4 = "arith.muli"(%3, %arg8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%4) {logical_block_num} : (i32) -> ()
    %5 = "memref.reinterpret_cast"(%arg4) <{operandSegmentSizes = array<i32: 1, 0, 0, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: 5>, static_strides = array<i64: 1>}> : (memref<?xi64>) -> memref<5xi64, strided<[1]>>
    %6 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<5xi64>
    "hivm.hir.load"(%5, %6) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<5xi64, strided<[1]>>, memref<5xi64>) -> ()
    %7 = "bufferization.to_tensor"(%6) <{restrict, writable}> : (memref<5xi64>) -> tensor<5xi64>
    %8 = "memref.reinterpret_cast"(%arg3) <{operandSegmentSizes = array<i32: 1, 0, 0, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: 10, 8>, static_strides = array<i64: 8, 1>}> : (memref<?xf32>) -> memref<10x8xf32, strided<[8, 1]>>
    %9 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<5x8xf32>
    "scf.for"(%2, %1, %0) ({
    ^bb0(%arg9: index):
      %12 = "tensor.extract"(%7, %arg9) : (tensor<5xi64>, index) -> i64
      %13 = "arith.index_cast"(%12) : (i64) -> index
      %14 = "memref.subview"(%8, %13) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808, 0>, static_sizes = array<i64: 1, 8>, static_strides = array<i64: 1, 1>}> : (memref<10x8xf32, strided<[8, 1]>>, index) -> memref<1x8xf32, strided<[8, 1], offset: ?>>
      %15 = "memref.subview"(%9, %arg9) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808, 0>, static_sizes = array<i64: 1, 8>, static_strides = array<i64: 1, 1>}> : (memref<5x8xf32>, index) -> memref<1x8xf32, strided<[8, 1], offset: ?>>
      "annotation.mark"(%15) {hivm.stride_align_dims = array<i32: 0>, hivm.stride_align_value_in_byte = array<i32: 32>} : (memref<1x8xf32, strided<[8, 1], offset: ?>>) -> ()
      "hivm.hir.load"(%14, %15, %2) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 1, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<1x8xf32, strided<[8, 1], offset: ?>>, memref<1x8xf32, strided<[8, 1], offset: ?>>, index) -> ()
      "scf.yield"() : () -> ()
    }) {hivm.parallel_loop} : (index, index, index) -> ()
    %10 = "bufferization.to_tensor"(%9) <{restrict, writable}> {index_select_simd} : (memref<5x8xf32>) -> tensor<5x8xf32>
    %11 = "memref.reinterpret_cast"(%arg5) <{operandSegmentSizes = array<i32: 1, 0, 0, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: 5, 8>, static_strides = array<i64: 8, 1>}> : (memref<?xf32>) -> memref<5x8xf32, strided<[8, 1]>>
    "hivm.hir.store"(%10, %11) : (tensor<5x8xf32>, memref<5x8xf32, strided<[8, 1]>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, false, false, false]> : vector<9xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

