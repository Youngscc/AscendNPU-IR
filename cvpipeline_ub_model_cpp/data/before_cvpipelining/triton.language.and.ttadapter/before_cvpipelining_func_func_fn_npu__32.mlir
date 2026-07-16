"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xi32>, memref<?xi32>, memref<?xi32>, i32, i32, i32) -> (), sym_name = "fn_npu_"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xi32>, %arg4: memref<?xi32>, %arg5: memref<?xi32>, %arg6: i32, %arg7: i32, %arg8: i32):
    %0 = "arith.constant"() <{value = 16 : index}> : () -> index
    %1 = "arith.constant"() <{value = 8 : i32}> : () -> i32
    %2 = "arith.constant"() <{value = 128 : i32}> : () -> i32
    "hivm.hir.set_mask_norm"() : () -> ()
    %3 = "arith.muli"(%arg6, %arg7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %4 = "arith.muli"(%3, %arg8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%4) {logical_block_num} : (i32) -> ()
    %5 = "hivm.hir.get_block_idx"() : () -> i64
    %6 = "arith.trunci"(%5) : (i64) -> i32
    %7 = "arith.remsi"(%6, %arg8) : (i32, i32) -> i32
    %8 = "arith.divsi"(%6, %arg8) : (i32, i32) -> i32
    %9 = "arith.remsi"(%8, %arg7) : (i32, i32) -> i32
    %10 = "arith.muli"(%arg8, %arg7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %11 = "arith.divsi"(%6, %10) : (i32, i32) -> i32
    %12 = "arith.remsi"(%11, %arg6) : (i32, i32) -> i32
    %13 = "arith.muli"(%9, %1) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %14 = "arith.muli"(%12, %2) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %15 = "arith.index_cast"(%14) : (i32) -> index
    %16 = "arith.index_cast"(%13) : (i32) -> index
    %17 = "arith.muli"(%16, %0) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %18 = "arith.index_cast"(%7) : (i32) -> index
    %19 = "arith.addi"(%15, %18) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %20 = "arith.addi"(%19, %17) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %21 = "memref.reinterpret_cast"(%arg4, %20) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1, 8, 1>, static_strides = array<i64: 8, 16, 1>}> : (memref<?xi32>, index) -> memref<1x8x1xi32, strided<[8, 16, 1], offset: ?>>
    %22 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<1x8x1xi32>
    "hivm.hir.load"(%21, %22) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<1x8x1xi32, strided<[8, 16, 1], offset: ?>>, memref<1x8x1xi32>) -> ()
    %23 = "bufferization.to_tensor"(%22) <{restrict, writable}> : (memref<1x8x1xi32>) -> tensor<1x8x1xi32>
    %24 = "memref.reinterpret_cast"(%arg5, %20) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1, 8, 1>, static_strides = array<i64: 8, 16, 1>}> : (memref<?xi32>, index) -> memref<1x8x1xi32, strided<[8, 16, 1], offset: ?>>
    %25 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<1x8x1xi32>
    "hivm.hir.load"(%24, %25) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<1x8x1xi32, strided<[8, 16, 1], offset: ?>>, memref<1x8x1xi32>) -> ()
    %26 = "bufferization.to_tensor"(%25) <{restrict, writable}> : (memref<1x8x1xi32>) -> tensor<1x8x1xi32>
    %27 = "tensor.empty"() : () -> tensor<1x8x1xi32>
    %28 = "hivm.hir.vand"(%23, %26, %27) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1x8x1xi32>, tensor<1x8x1xi32>, tensor<1x8x1xi32>) -> tensor<1x8x1xi32>
    %29 = "memref.reinterpret_cast"(%arg3, %20) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1, 8, 1>, static_strides = array<i64: 8, 16, 1>}> : (memref<?xi32>, index) -> memref<1x8x1xi32, strided<[8, 16, 1], offset: ?>>
    "hivm.hir.store"(%28, %29) : (tensor<1x8x1xi32>, memref<1x8x1xi32, strided<[8, 16, 1], offset: ?>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, false, false, false]> : vector<9xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

