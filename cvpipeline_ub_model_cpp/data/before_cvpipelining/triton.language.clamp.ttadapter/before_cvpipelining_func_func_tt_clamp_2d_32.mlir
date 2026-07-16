"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xf32>, memref<?xf32>, memref<?xf32>, memref<?xf32>, i32, i32, i32) -> (), sym_name = "tt_clamp_2d"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xf32>, %arg4: memref<?xf32>, %arg5: memref<?xf32>, %arg6: memref<?xf32>, %arg7: i32, %arg8: i32, %arg9: i32):
    %0 = "arith.constant"() <{value = 16 : index}> : () -> index
    %1 = "arith.constant"() <{value = 4 : i32}> : () -> i32
    %2 = "arith.constant"() <{value = 8 : i32}> : () -> i32
    "hivm.hir.set_mask_norm"() : () -> ()
    %3 = "arith.muli"(%arg7, %arg8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %4 = "arith.muli"(%3, %arg9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%4) {logical_block_num} : (i32) -> ()
    %5 = "hivm.hir.get_block_idx"() : () -> i64
    %6 = "arith.trunci"(%5) : (i64) -> i32
    %7 = "arith.divsi"(%6, %arg9) : (i32, i32) -> i32
    %8 = "arith.remsi"(%7, %arg8) : (i32, i32) -> i32
    %9 = "arith.muli"(%arg9, %arg8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %10 = "arith.divsi"(%6, %9) : (i32, i32) -> i32
    %11 = "arith.remsi"(%10, %arg7) : (i32, i32) -> i32
    %12 = "arith.muli"(%11, %1) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %13 = "arith.muli"(%8, %2) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %14 = "arith.index_cast"(%12) : (i32) -> index
    %15 = "arith.muli"(%14, %0) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %16 = "arith.index_cast"(%13) : (i32) -> index
    %17 = "arith.addi"(%15, %16) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %18 = "memref.reinterpret_cast"(%arg3, %17) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 4, 8>, static_strides = array<i64: 16, 1>}> : (memref<?xf32>, index) -> memref<4x8xf32, strided<[16, 1], offset: ?>>
    %19 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<4x8xf32>
    "hivm.hir.load"(%18, %19) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<4x8xf32, strided<[16, 1], offset: ?>>, memref<4x8xf32>) -> ()
    %20 = "bufferization.to_tensor"(%19) <{restrict, writable}> : (memref<4x8xf32>) -> tensor<4x8xf32>
    %21 = "memref.reinterpret_cast"(%arg5, %17) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 4, 8>, static_strides = array<i64: 16, 1>}> : (memref<?xf32>, index) -> memref<4x8xf32, strided<[16, 1], offset: ?>>
    %22 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<4x8xf32>
    "hivm.hir.load"(%21, %22) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<4x8xf32, strided<[16, 1], offset: ?>>, memref<4x8xf32>) -> ()
    %23 = "bufferization.to_tensor"(%22) <{restrict, writable}> : (memref<4x8xf32>) -> tensor<4x8xf32>
    %24 = "memref.reinterpret_cast"(%arg6, %17) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 4, 8>, static_strides = array<i64: 16, 1>}> : (memref<?xf32>, index) -> memref<4x8xf32, strided<[16, 1], offset: ?>>
    %25 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<4x8xf32>
    "hivm.hir.load"(%24, %25) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<4x8xf32, strided<[16, 1], offset: ?>>, memref<4x8xf32>) -> ()
    %26 = "bufferization.to_tensor"(%25) <{restrict, writable}> : (memref<4x8xf32>) -> tensor<4x8xf32>
    %27 = "tensor.empty"() : () -> tensor<4x8xi1>
    %28 = "tensor.empty"() : () -> tensor<4x8xi1>
    %29 = "tensor.empty"() : () -> tensor<4x8xi1>
    %30 = "tensor.empty"() : () -> tensor<4x8xi1>
    %31 = "tensor.empty"() : () -> tensor<4x8xi1>
    %32 = "tensor.empty"() : () -> tensor<4x8xi1>
    %33 = "tensor.empty"() : () -> tensor<4x8xi1>
    %34 = "tensor.empty"() : () -> tensor<4x8xi1>
    %35 = "hivm.hir.vcmp"(%20, %20, %34) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<4x8xf32>, tensor<4x8xf32>, tensor<4x8xi1>) -> tensor<4x8xi1>
    %36 = "hivm.hir.vnot"(%35, %33) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<4x8xi1>, tensor<4x8xi1>) -> tensor<4x8xi1>
    %37 = "hivm.hir.vcmp"(%26, %26, %32) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<4x8xf32>, tensor<4x8xf32>, tensor<4x8xi1>) -> tensor<4x8xi1>
    %38 = "hivm.hir.vnot"(%37, %31) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<4x8xi1>, tensor<4x8xi1>) -> tensor<4x8xi1>
    %39 = "tensor.empty"() : () -> tensor<4x8xf32>
    %40 = "tensor.empty"() : () -> tensor<4x8xf32>
    %41 = "tensor.empty"() : () -> tensor<4x8xf32>
    %42 = "tensor.empty"() : () -> tensor<4x8xf32>
    %43 = "tensor.empty"() : () -> tensor<4x8xf32>
    %44 = "tensor.empty"() : () -> tensor<4x8xf32>
    %45 = "hivm.hir.vmin"(%20, %26, %44) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<4x8xf32>, tensor<4x8xf32>, tensor<4x8xf32>) -> tensor<4x8xf32>
    %46 = "hivm.hir.vsel"(%36, %26, %45, %43) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<4x8xi1>, tensor<4x8xf32>, tensor<4x8xf32>, tensor<4x8xf32>) -> tensor<4x8xf32>
    %47 = "hivm.hir.vsel"(%38, %20, %46, %42) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<4x8xi1>, tensor<4x8xf32>, tensor<4x8xf32>, tensor<4x8xf32>) -> tensor<4x8xf32>
    %48 = "hivm.hir.vcmp"(%23, %23, %30) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<4x8xf32>, tensor<4x8xf32>, tensor<4x8xi1>) -> tensor<4x8xi1>
    %49 = "hivm.hir.vnot"(%48, %29) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<4x8xi1>, tensor<4x8xi1>) -> tensor<4x8xi1>
    %50 = "hivm.hir.vcmp"(%47, %47, %28) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<4x8xf32>, tensor<4x8xf32>, tensor<4x8xi1>) -> tensor<4x8xi1>
    %51 = "hivm.hir.vnot"(%50, %27) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<4x8xi1>, tensor<4x8xi1>) -> tensor<4x8xi1>
    %52 = "hivm.hir.vmax"(%23, %47, %41) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<4x8xf32>, tensor<4x8xf32>, tensor<4x8xf32>) -> tensor<4x8xf32>
    %53 = "hivm.hir.vsel"(%49, %47, %52, %40) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<4x8xi1>, tensor<4x8xf32>, tensor<4x8xf32>, tensor<4x8xf32>) -> tensor<4x8xf32>
    %54 = "hivm.hir.vsel"(%51, %23, %53, %39) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<4x8xi1>, tensor<4x8xf32>, tensor<4x8xf32>, tensor<4x8xf32>) -> tensor<4x8xf32>
    %55 = "memref.reinterpret_cast"(%arg4, %17) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 4, 8>, static_strides = array<i64: 16, 1>}> : (memref<?xf32>, index) -> memref<4x8xf32, strided<[16, 1], offset: ?>>
    "hivm.hir.store"(%54, %55) : (tensor<4x8xf32>, memref<4x8xf32, strided<[16, 1], offset: ?>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, true, false, false, false]> : vector<10xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

