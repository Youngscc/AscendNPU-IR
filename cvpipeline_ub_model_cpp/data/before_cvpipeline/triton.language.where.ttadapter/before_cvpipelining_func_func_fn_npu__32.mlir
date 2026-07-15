"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xf32>, memref<?xf32>, memref<?xf32>, i32, i32, i32) -> (), sym_name = "fn_npu_"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xf32>, %arg4: memref<?xf32>, %arg5: memref<?xf32>, %arg6: i32, %arg7: i32, %arg8: i32):
    %0 = "arith.constant"() <{value = 16 : index}> : () -> index
    %1 = "arith.constant"() <{value = 128 : i32}> : () -> i32
    %2 = "arith.constant"() <{value = 8 : i32}> : () -> i32
    %3 = "arith.constant"() <{value = 1.000000e+00 : f32}> : () -> f32
    "hivm.hir.set_mask_norm"() : () -> ()
    %4 = "arith.muli"(%arg6, %arg7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %5 = "arith.muli"(%4, %arg8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%5) {logical_block_num} : (i32) -> ()
    %6 = "hivm.hir.get_block_idx"() : () -> i64
    %7 = "arith.trunci"(%6) : (i64) -> i32
    %8 = "arith.remsi"(%7, %arg8) : (i32, i32) -> i32
    %9 = "arith.divsi"(%7, %arg8) : (i32, i32) -> i32
    %10 = "arith.remsi"(%9, %arg7) : (i32, i32) -> i32
    %11 = "arith.muli"(%arg8, %arg7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %12 = "arith.divsi"(%7, %11) : (i32, i32) -> i32
    %13 = "arith.remsi"(%12, %arg6) : (i32, i32) -> i32
    %14 = "tensor.empty"() : () -> tensor<1x8x1xf32>
    %15 = "arith.muli"(%10, %2) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %16 = "arith.muli"(%13, %1) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %17 = "arith.index_cast"(%16) : (i32) -> index
    %18 = "arith.index_cast"(%15) : (i32) -> index
    %19 = "arith.muli"(%18, %0) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %20 = "arith.index_cast"(%8) : (i32) -> index
    %21 = "arith.addi"(%17, %20) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %22 = "arith.addi"(%21, %19) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %23 = "memref.reinterpret_cast"(%arg4, %22) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1, 8, 1>, static_strides = array<i64: 8, 16, 1>}> : (memref<?xf32>, index) -> memref<1x8x1xf32, strided<[8, 16, 1], offset: ?>>
    %24 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<1x8x1xf32>
    "hivm.hir.load"(%23, %24) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<1x8x1xf32, strided<[8, 16, 1], offset: ?>>, memref<1x8x1xf32>) -> ()
    %25 = "bufferization.to_tensor"(%24) <{restrict, writable}> : (memref<1x8x1xf32>) -> tensor<1x8x1xf32>
    %26 = "memref.reinterpret_cast"(%arg5, %22) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1, 8, 1>, static_strides = array<i64: 8, 16, 1>}> : (memref<?xf32>, index) -> memref<1x8x1xf32, strided<[8, 16, 1], offset: ?>>
    %27 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<1x8x1xf32>
    "hivm.hir.load"(%26, %27) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<1x8x1xf32, strided<[8, 16, 1], offset: ?>>, memref<1x8x1xf32>) -> ()
    %28 = "bufferization.to_tensor"(%27) <{restrict, writable}> : (memref<1x8x1xf32>) -> tensor<1x8x1xf32>
    %29 = "tensor.empty"() : () -> tensor<1x8x1xi1>
    %30 = "hivm.hir.vcmp"(%25, %28, %29) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<lt>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<1x8x1xf32>, tensor<1x8x1xf32>, tensor<1x8x1xi1>) -> tensor<1x8x1xi1>
    %31 = "hivm.hir.vsel"(%30, %25, %3, %14) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<1x8x1xi1>, tensor<1x8x1xf32>, f32, tensor<1x8x1xf32>) -> tensor<1x8x1xf32>
    %32 = "memref.reinterpret_cast"(%arg3, %22) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1, 8, 1>, static_strides = array<i64: 8, 16, 1>}> : (memref<?xf32>, index) -> memref<1x8x1xf32, strided<[8, 16, 1], offset: ?>>
    "hivm.hir.store"(%31, %32) : (tensor<1x8x1xf32>, memref<1x8x1xf32, strided<[8, 16, 1], offset: ?>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, false, false, false]> : vector<9xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

