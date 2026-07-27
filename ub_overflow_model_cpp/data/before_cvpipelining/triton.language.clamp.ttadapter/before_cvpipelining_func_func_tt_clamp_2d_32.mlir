#map = affine_map<()[s0] -> (s0 * 16)>
#map1 = affine_map<()[s0, s1] -> (s0 + s1)>
"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xf32>, memref<?xf32>, memref<?xf32>, memref<?xf32>, i32, i32, i32) -> (), sym_name = "tt_clamp_2d"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xf32>, %arg4: memref<?xf32>, %arg5: memref<?xf32>, %arg6: memref<?xf32>, %arg7: i32, %arg8: i32, %arg9: i32):
    %0 = "arith.constant"() <{value = 4 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = 8 : i32}> : () -> i32
    "hivm.hir.set_mask_norm"() : () -> ()
    %2 = "arith.muli"(%arg7, %arg8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %3 = "arith.muli"(%2, %arg9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%3) <{effects = ["write"]}> {logical_block_num} : (i32) -> ()
    %4 = "hivm.hir.get_block_idx"() : () -> i64
    %5 = "arith.trunci"(%4) : (i64) -> i32
    %6 = "arith.divsi"(%5, %arg9) : (i32, i32) -> i32
    %7 = "arith.remsi"(%6, %arg8) : (i32, i32) -> i32
    %8 = "arith.muli"(%arg9, %arg8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %9 = "arith.divsi"(%5, %8) : (i32, i32) -> i32
    %10 = "arith.remsi"(%9, %arg7) : (i32, i32) -> i32
    %11 = "arith.muli"(%10, %0) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %12 = "arith.muli"(%7, %1) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %13 = "arith.index_cast"(%11) : (i32) -> index
    %14 = "affine.apply"(%13) <{map = #map}> : (index) -> index
    %15 = "arith.index_cast"(%12) : (i32) -> index
    %16 = "affine.apply"(%14, %15) <{map = #map1}> : (index, index) -> index
    %17 = "memref.reinterpret_cast"(%arg3, %16) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 4, 8>, static_strides = array<i64: 16, 1>}> : (memref<?xf32>, index) -> memref<4x8xf32, strided<[16, 1], offset: ?>>
    %18 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<4x8xf32>
    "hivm.hir.load"(%17, %18) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<4x8xf32, strided<[16, 1], offset: ?>>, memref<4x8xf32>) -> ()
    %19 = "bufferization.to_tensor"(%18) <{restrict, writable}> : (memref<4x8xf32>) -> tensor<4x8xf32>
    %20 = "memref.reinterpret_cast"(%arg5, %16) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 4, 8>, static_strides = array<i64: 16, 1>}> : (memref<?xf32>, index) -> memref<4x8xf32, strided<[16, 1], offset: ?>>
    %21 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<4x8xf32>
    "hivm.hir.load"(%20, %21) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<4x8xf32, strided<[16, 1], offset: ?>>, memref<4x8xf32>) -> ()
    %22 = "bufferization.to_tensor"(%21) <{restrict, writable}> : (memref<4x8xf32>) -> tensor<4x8xf32>
    %23 = "memref.reinterpret_cast"(%arg6, %16) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 4, 8>, static_strides = array<i64: 16, 1>}> : (memref<?xf32>, index) -> memref<4x8xf32, strided<[16, 1], offset: ?>>
    %24 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<4x8xf32>
    "hivm.hir.load"(%23, %24) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<4x8xf32, strided<[16, 1], offset: ?>>, memref<4x8xf32>) -> ()
    %25 = "bufferization.to_tensor"(%24) <{restrict, writable}> : (memref<4x8xf32>) -> tensor<4x8xf32>
    %26 = "tensor.empty"() : () -> tensor<4x8xi1>
    %27 = "tensor.empty"() : () -> tensor<4x8xi1>
    %28 = "tensor.empty"() : () -> tensor<4x8xi1>
    %29 = "tensor.empty"() : () -> tensor<4x8xi1>
    %30 = "tensor.empty"() : () -> tensor<4x8xi1>
    %31 = "tensor.empty"() : () -> tensor<4x8xi1>
    %32 = "tensor.empty"() : () -> tensor<4x8xi1>
    %33 = "tensor.empty"() : () -> tensor<4x8xi1>
    %34 = "hivm.hir.vcmp"(%19, %19, %33) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<4x8xf32>, tensor<4x8xf32>, tensor<4x8xi1>) -> tensor<4x8xi1>
    %35 = "hivm.hir.vnot"(%34, %32) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<4x8xi1>, tensor<4x8xi1>) -> tensor<4x8xi1>
    %36 = "hivm.hir.vcmp"(%25, %25, %31) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<4x8xf32>, tensor<4x8xf32>, tensor<4x8xi1>) -> tensor<4x8xi1>
    %37 = "hivm.hir.vnot"(%36, %30) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<4x8xi1>, tensor<4x8xi1>) -> tensor<4x8xi1>
    %38 = "tensor.empty"() : () -> tensor<4x8xf32>
    %39 = "tensor.empty"() : () -> tensor<4x8xf32>
    %40 = "tensor.empty"() : () -> tensor<4x8xf32>
    %41 = "tensor.empty"() : () -> tensor<4x8xf32>
    %42 = "tensor.empty"() : () -> tensor<4x8xf32>
    %43 = "tensor.empty"() : () -> tensor<4x8xf32>
    %44 = "hivm.hir.vmin"(%19, %25, %43) <{broadcast = array<i64>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<4x8xf32>, tensor<4x8xf32>, tensor<4x8xf32>) -> tensor<4x8xf32>
    %45 = "hivm.hir.vsel"(%35, %25, %44, %42) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<4x8xi1>, tensor<4x8xf32>, tensor<4x8xf32>, tensor<4x8xf32>) -> tensor<4x8xf32>
    %46 = "hivm.hir.vsel"(%37, %19, %45, %41) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<4x8xi1>, tensor<4x8xf32>, tensor<4x8xf32>, tensor<4x8xf32>) -> tensor<4x8xf32>
    %47 = "hivm.hir.vcmp"(%22, %22, %29) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<4x8xf32>, tensor<4x8xf32>, tensor<4x8xi1>) -> tensor<4x8xi1>
    %48 = "hivm.hir.vnot"(%47, %28) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<4x8xi1>, tensor<4x8xi1>) -> tensor<4x8xi1>
    %49 = "hivm.hir.vcmp"(%46, %46, %27) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<4x8xf32>, tensor<4x8xf32>, tensor<4x8xi1>) -> tensor<4x8xi1>
    %50 = "hivm.hir.vnot"(%49, %26) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<4x8xi1>, tensor<4x8xi1>) -> tensor<4x8xi1>
    %51 = "hivm.hir.vmax"(%22, %46, %40) <{broadcast = array<i64>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<4x8xf32>, tensor<4x8xf32>, tensor<4x8xf32>) -> tensor<4x8xf32>
    %52 = "hivm.hir.vsel"(%48, %46, %51, %39) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<4x8xi1>, tensor<4x8xf32>, tensor<4x8xf32>, tensor<4x8xf32>) -> tensor<4x8xf32>
    %53 = "hivm.hir.vsel"(%50, %22, %52, %38) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<4x8xi1>, tensor<4x8xf32>, tensor<4x8xf32>, tensor<4x8xf32>) -> tensor<4x8xf32>
    %54 = "memref.reinterpret_cast"(%arg4, %16) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 4, 8>, static_strides = array<i64: 16, 1>}> : (memref<?xf32>, index) -> memref<4x8xf32, strided<[16, 1], offset: ?>>
    "hivm.hir.store"(%53, %54) : (tensor<4x8xf32>, memref<4x8xf32, strided<[16, 1], offset: ?>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, true, false, false, false]> : vector<10xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hacc.target = #hacc.target<"Ascend910B1">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

