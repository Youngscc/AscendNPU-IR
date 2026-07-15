"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xi32>, memref<?xi32>, memref<?xi32>, i32, i32, i32) -> (), sym_name = "cdiv_kernel"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xi32>, %arg4: memref<?xi32>, %arg5: memref<?xi32>, %arg6: i32, %arg7: i32, %arg8: i32):
    %0 = "arith.constant"() <{value = 1 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = 32 : index}> : () -> index
    %2 = "arith.constant"() <{value = 8 : index}> : () -> index
    %3 = "arith.constant"() <{value = 2 : i32}> : () -> i32
    %4 = "arith.constant"() <{value = 4 : i32}> : () -> i32
    %5 = "arith.constant"() <{value = 8 : i32}> : () -> i32
    "hivm.hir.set_mask_norm"() : () -> ()
    %6 = "arith.muli"(%arg6, %arg7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %7 = "arith.muli"(%6, %arg8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%7) {logical_block_num} : (i32) -> ()
    %8 = "hivm.hir.get_block_idx"() : () -> i64
    %9 = "arith.trunci"(%8) : (i64) -> i32
    %10 = "arith.remsi"(%9, %arg8) : (i32, i32) -> i32
    %11 = "arith.divsi"(%9, %arg8) : (i32, i32) -> i32
    %12 = "arith.remsi"(%11, %arg7) : (i32, i32) -> i32
    %13 = "arith.muli"(%arg8, %arg7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %14 = "arith.divsi"(%9, %13) : (i32, i32) -> i32
    %15 = "arith.remsi"(%14, %arg6) : (i32, i32) -> i32
    %16 = "tensor.empty"() : () -> tensor<2x4x8xi32>
    %17 = "tensor.empty"() : () -> tensor<2x4x8xi32>
    %18 = "tensor.empty"() : () -> tensor<2x4x8xi32>
    %19 = "arith.muli"(%15, %3) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %20 = "arith.muli"(%12, %4) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %21 = "arith.muli"(%10, %5) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %22 = "arith.index_cast"(%19) : (i32) -> index
    %23 = "arith.muli"(%22, %1) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %24 = "arith.index_cast"(%20) : (i32) -> index
    %25 = "arith.muli"(%24, %2) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %26 = "arith.index_cast"(%21) : (i32) -> index
    %27 = "arith.addi"(%23, %25) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %28 = "arith.addi"(%27, %26) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %29 = "memref.reinterpret_cast"(%arg4, %28) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 2, 4, 8>, static_strides = array<i64: 32, 8, 1>}> : (memref<?xi32>, index) -> memref<2x4x8xi32, strided<[32, 8, 1], offset: ?>>
    %30 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<2x4x8xi32>
    "hivm.hir.load"(%29, %30) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<2x4x8xi32, strided<[32, 8, 1], offset: ?>>, memref<2x4x8xi32>) -> ()
    %31 = "bufferization.to_tensor"(%30) <{restrict, writable}> : (memref<2x4x8xi32>) -> tensor<2x4x8xi32>
    %32 = "memref.reinterpret_cast"(%arg5, %28) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 2, 4, 8>, static_strides = array<i64: 32, 8, 1>}> : (memref<?xi32>, index) -> memref<2x4x8xi32, strided<[32, 8, 1], offset: ?>>
    %33 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<2x4x8xi32>
    "hivm.hir.load"(%32, %33) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<2x4x8xi32, strided<[32, 8, 1], offset: ?>>, memref<2x4x8xi32>) -> ()
    %34 = "bufferization.to_tensor"(%33) <{restrict, writable}> : (memref<2x4x8xi32>) -> tensor<2x4x8xi32>
    %35 = "hivm.hir.vsub"(%34, %0, %18) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xi32>, i32, tensor<2x4x8xi32>) -> tensor<2x4x8xi32>
    %36 = "hivm.hir.vadd"(%31, %35, %17) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xi32>, tensor<2x4x8xi32>, tensor<2x4x8xi32>) -> tensor<2x4x8xi32>
    %37 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %38 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %39 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %40 = "hivm.hir.vcast"(%36, %39) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<2x4x8xi32>, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %41 = "hivm.hir.vcast"(%34, %38) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<2x4x8xi32>, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %42 = "hivm.hir.vdiv"(%40, %41, %37) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, tensor<2x4x8xf32>, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %43 = "hivm.hir.vcast"(%42, %16) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<trunc>, transpose = array<i64>}> : (tensor<2x4x8xf32>, tensor<2x4x8xi32>) -> tensor<2x4x8xi32>
    %44 = "memref.reinterpret_cast"(%arg3, %28) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 2, 4, 8>, static_strides = array<i64: 32, 8, 1>}> : (memref<?xi32>, index) -> memref<2x4x8xi32, strided<[32, 8, 1], offset: ?>>
    "hivm.hir.store"(%43, %44) : (tensor<2x4x8xi32>, memref<2x4x8xi32, strided<[32, 8, 1], offset: ?>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, false, false, false]> : vector<9xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

