#map = affine_map<()[s0] -> (s0 * 32)>
#map1 = affine_map<()[s0] -> (s0 * 8)>
#map2 = affine_map<()[s0, s1] -> (s0 + s1)>
"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xi32>, memref<?xi32>, memref<?xi32>, i32, i32, i32) -> (), sym_name = "cdiv_kernel"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xi32>, %arg4: memref<?xi32>, %arg5: memref<?xi32>, %arg6: i32, %arg7: i32, %arg8: i32):
    %0 = "arith.constant"() <{value = 1 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = 2 : i32}> : () -> i32
    %2 = "arith.constant"() <{value = 4 : i32}> : () -> i32
    %3 = "arith.constant"() <{value = 8 : i32}> : () -> i32
    "hivm.hir.set_mask_norm"() : () -> ()
    %4 = "arith.muli"(%arg6, %arg7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %5 = "arith.muli"(%4, %arg8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%5) <{effects = ["write"]}> {logical_block_num} : (i32) -> ()
    %6 = "hivm.hir.get_block_idx"() : () -> i64
    %7 = "arith.trunci"(%6) : (i64) -> i32
    %8 = "arith.remsi"(%7, %arg8) : (i32, i32) -> i32
    %9 = "arith.divsi"(%7, %arg8) : (i32, i32) -> i32
    %10 = "arith.remsi"(%9, %arg7) : (i32, i32) -> i32
    %11 = "arith.muli"(%arg8, %arg7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %12 = "arith.divsi"(%7, %11) : (i32, i32) -> i32
    %13 = "arith.remsi"(%12, %arg6) : (i32, i32) -> i32
    %14 = "tensor.empty"() : () -> tensor<2x4x8xi32>
    %15 = "tensor.empty"() : () -> tensor<2x4x8xi32>
    %16 = "tensor.empty"() : () -> tensor<2x4x8xi32>
    %17 = "arith.muli"(%13, %1) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %18 = "arith.muli"(%10, %2) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %19 = "arith.muli"(%8, %3) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %20 = "arith.index_cast"(%17) : (i32) -> index
    %21 = "affine.apply"(%20) <{map = #map}> : (index) -> index
    %22 = "arith.index_cast"(%18) : (i32) -> index
    %23 = "affine.apply"(%22) <{map = #map1}> : (index) -> index
    %24 = "arith.index_cast"(%19) : (i32) -> index
    %25 = "affine.apply"(%21, %23) <{map = #map2}> : (index, index) -> index
    %26 = "affine.apply"(%25, %24) <{map = #map2}> : (index, index) -> index
    %27 = "memref.reinterpret_cast"(%arg4, %26) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 2, 4, 8>, static_strides = array<i64: 32, 8, 1>}> : (memref<?xi32>, index) -> memref<2x4x8xi32, strided<[32, 8, 1], offset: ?>>
    %28 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<2x4x8xi32>
    "hivm.hir.load"(%27, %28) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<2x4x8xi32, strided<[32, 8, 1], offset: ?>>, memref<2x4x8xi32>) -> ()
    %29 = "bufferization.to_tensor"(%28) <{restrict, writable}> : (memref<2x4x8xi32>) -> tensor<2x4x8xi32>
    %30 = "memref.reinterpret_cast"(%arg5, %26) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 2, 4, 8>, static_strides = array<i64: 32, 8, 1>}> : (memref<?xi32>, index) -> memref<2x4x8xi32, strided<[32, 8, 1], offset: ?>>
    %31 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<2x4x8xi32>
    "hivm.hir.load"(%30, %31) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<2x4x8xi32, strided<[32, 8, 1], offset: ?>>, memref<2x4x8xi32>) -> ()
    %32 = "bufferization.to_tensor"(%31) <{restrict, writable}> : (memref<2x4x8xi32>) -> tensor<2x4x8xi32>
    %33 = "hivm.hir.vsub"(%32, %0, %16) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xi32>, i32, tensor<2x4x8xi32>) -> tensor<2x4x8xi32>
    %34 = "hivm.hir.vadd"(%29, %33, %15) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xi32>, tensor<2x4x8xi32>, tensor<2x4x8xi32>) -> tensor<2x4x8xi32>
    %35 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %36 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %37 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %38 = "hivm.hir.vcast"(%34, %37) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<2x4x8xi32>, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %39 = "hivm.hir.vcast"(%32, %36) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<2x4x8xi32>, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %40 = "hivm.hir.vdiv"(%38, %39, %35) <{broadcast = array<i64>, isHP = false, isSigned = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, tensor<2x4x8xf32>, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %41 = "hivm.hir.vcast"(%40, %14) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<trunc>, transpose = array<i64>}> : (tensor<2x4x8xf32>, tensor<2x4x8xi32>) -> tensor<2x4x8xi32>
    %42 = "memref.reinterpret_cast"(%arg3, %26) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 2, 4, 8>, static_strides = array<i64: 32, 8, 1>}> : (memref<?xi32>, index) -> memref<2x4x8xi32, strided<[32, 8, 1], offset: ?>>
    "hivm.hir.store"(%41, %42) : (tensor<2x4x8xi32>, memref<2x4x8xi32, strided<[32, 8, 1], offset: ?>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, false, false, false]> : vector<9xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hacc.target = #hacc.target<"Ascend910B1">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

