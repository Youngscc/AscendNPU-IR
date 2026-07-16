"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {}, {}, {}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xi32>, memref<?xi32>, memref<?xi8>, i32, i32, i32, i32, i32, i32) -> (), sym_name = "triton_logical_or_3d"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xi32>, %arg4: memref<?xi32>, %arg5: memref<?xi8>, %arg6: i32, %arg7: i32, %arg8: i32, %arg9: i32, %arg10: i32, %arg11: i32):
    %0 = "arith.constant"() <{value = 0.000000e+00 : f32}> : () -> f32
    %1 = "arith.constant"() <{value = 32 : index}> : () -> index
    %2 = "arith.constant"() <{value = 8 : index}> : () -> index
    "hivm.hir.set_mask_norm"() : () -> ()
    %3 = "arith.muli"(%arg9, %arg10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %4 = "arith.muli"(%3, %arg11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%4) {logical_block_num} : (i32) -> ()
    %5 = "hivm.hir.get_block_idx"() : () -> i64
    %6 = "arith.trunci"(%5) : (i64) -> i32
    %7 = "arith.remsi"(%6, %arg11) : (i32, i32) -> i32
    %8 = "arith.divsi"(%6, %arg11) : (i32, i32) -> i32
    %9 = "arith.remsi"(%8, %arg10) : (i32, i32) -> i32
    %10 = "arith.muli"(%arg11, %arg10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %11 = "arith.divsi"(%6, %10) : (i32, i32) -> i32
    %12 = "arith.remsi"(%11, %arg9) : (i32, i32) -> i32
    %13 = "arith.muli"(%12, %arg6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %14 = "arith.muli"(%9, %arg7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %15 = "arith.muli"(%7, %arg8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %16 = "arith.index_cast"(%13) : (i32) -> index
    %17 = "arith.muli"(%16, %1) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %18 = "arith.index_cast"(%14) : (i32) -> index
    %19 = "arith.muli"(%18, %2) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %20 = "arith.index_cast"(%15) : (i32) -> index
    %21 = "arith.addi"(%17, %19) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %22 = "arith.addi"(%21, %20) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %23 = "memref.reinterpret_cast"(%arg3, %22) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 2, 4, 8>, static_strides = array<i64: 32, 8, 1>}> : (memref<?xi32>, index) -> memref<2x4x8xi32, strided<[32, 8, 1], offset: ?>>
    %24 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<2x4x8xi32>
    "hivm.hir.load"(%23, %24) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<2x4x8xi32, strided<[32, 8, 1], offset: ?>>, memref<2x4x8xi32>) -> ()
    %25 = "bufferization.to_tensor"(%24) <{restrict, writable}> : (memref<2x4x8xi32>) -> tensor<2x4x8xi32>
    %26 = "memref.reinterpret_cast"(%arg4, %22) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 2, 4, 8>, static_strides = array<i64: 32, 8, 1>}> : (memref<?xi32>, index) -> memref<2x4x8xi32, strided<[32, 8, 1], offset: ?>>
    %27 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<2x4x8xi32>
    "hivm.hir.load"(%26, %27) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<2x4x8xi32, strided<[32, 8, 1], offset: ?>>, memref<2x4x8xi32>) -> ()
    %28 = "bufferization.to_tensor"(%27) <{restrict, writable}> : (memref<2x4x8xi32>) -> tensor<2x4x8xi32>
    %29 = "tensor.empty"() : () -> tensor<2x4x8xi1>
    %30 = "tensor.empty"() : () -> tensor<2x4x8xi1>
    %31 = "tensor.empty"() : () -> tensor<2x4x8xi1>
    %32 = "tensor.empty"() : () -> tensor<2x4x8xi1>
    %33 = "tensor.empty"() : () -> tensor<2x4x8xi1>
    %34 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %35 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %36 = "hivm.hir.vcast"(%25, %35) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<2x4x8xi32>, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %37 = "hivm.hir.vcmp"(%36, %0, %33) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<2x4x8xf32>, f32, tensor<2x4x8xi1>) -> tensor<2x4x8xi1>
    %38 = "hivm.hir.vnot"(%37, %32) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xi1>, tensor<2x4x8xi1>) -> tensor<2x4x8xi1>
    %39 = "hivm.hir.vcast"(%28, %34) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<2x4x8xi32>, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %40 = "hivm.hir.vcmp"(%39, %0, %31) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<2x4x8xf32>, f32, tensor<2x4x8xi1>) -> tensor<2x4x8xi1>
    %41 = "hivm.hir.vnot"(%40, %30) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xi1>, tensor<2x4x8xi1>) -> tensor<2x4x8xi1>
    %42 = "hivm.hir.vor"(%38, %41, %29) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xi1>, tensor<2x4x8xi1>, tensor<2x4x8xi1>) -> tensor<2x4x8xi1>
    %43 = "memref.reinterpret_cast"(%arg5, %22) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 2, 4, 8>, static_strides = array<i64: 32, 8, 1>}> : (memref<?xi8>, index) -> memref<2x4x8xi8, strided<[32, 8, 1], offset: ?>>
    %44 = "tensor.empty"() : () -> tensor<2x4x8xi8>
    %45 = "hivm.hir.vcast"(%42, %44) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<2x4x8xi1>, tensor<2x4x8xi8>) -> tensor<2x4x8xi8>
    "hivm.hir.store"(%45, %43) : (tensor<2x4x8xi8>, memref<2x4x8xi8, strided<[32, 8, 1], offset: ?>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, false, false, false, false, false, false]> : vector<12xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

