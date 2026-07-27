#map = affine_map<()[s0] -> (s0 * 16)>
#map1 = affine_map<()[s0, s1] -> (s0 + s1)>
"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xi32>, memref<?xi32>, memref<?xi32>, i32, i32, i32) -> (), sym_name = "fn_npu_"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xi32>, %arg4: memref<?xi32>, %arg5: memref<?xi32>, %arg6: i32, %arg7: i32, %arg8: i32):
    %0 = "arith.constant"() <{value = 8 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = 128 : i32}> : () -> i32
    "hivm.hir.set_mask_norm"() : () -> ()
    %2 = "arith.muli"(%arg6, %arg7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %3 = "arith.muli"(%2, %arg8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%3) <{effects = ["write"]}> {logical_block_num} : (i32) -> ()
    %4 = "hivm.hir.get_block_idx"() : () -> i64
    %5 = "arith.trunci"(%4) : (i64) -> i32
    %6 = "arith.remsi"(%5, %arg8) : (i32, i32) -> i32
    %7 = "arith.divsi"(%5, %arg8) : (i32, i32) -> i32
    %8 = "arith.remsi"(%7, %arg7) : (i32, i32) -> i32
    %9 = "arith.muli"(%arg8, %arg7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %10 = "arith.divsi"(%5, %9) : (i32, i32) -> i32
    %11 = "arith.remsi"(%10, %arg6) : (i32, i32) -> i32
    %12 = "arith.muli"(%8, %0) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %13 = "arith.muli"(%11, %1) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %14 = "arith.index_cast"(%13) : (i32) -> index
    %15 = "arith.index_cast"(%12) : (i32) -> index
    %16 = "affine.apply"(%15) <{map = #map}> : (index) -> index
    %17 = "arith.index_cast"(%6) : (i32) -> index
    %18 = "affine.apply"(%14, %17) <{map = #map1}> : (index, index) -> index
    %19 = "affine.apply"(%18, %16) <{map = #map1}> : (index, index) -> index
    %20 = "memref.reinterpret_cast"(%arg4, %19) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1, 8, 1>, static_strides = array<i64: 8, 16, 1>}> : (memref<?xi32>, index) -> memref<1x8x1xi32, strided<[8, 16, 1], offset: ?>>
    %21 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<1x8x1xi32>
    "hivm.hir.load"(%20, %21) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<1x8x1xi32, strided<[8, 16, 1], offset: ?>>, memref<1x8x1xi32>) -> ()
    %22 = "bufferization.to_tensor"(%21) <{restrict, writable}> : (memref<1x8x1xi32>) -> tensor<1x8x1xi32>
    %23 = "memref.reinterpret_cast"(%arg5, %19) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1, 8, 1>, static_strides = array<i64: 8, 16, 1>}> : (memref<?xi32>, index) -> memref<1x8x1xi32, strided<[8, 16, 1], offset: ?>>
    %24 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<1x8x1xi32>
    "hivm.hir.load"(%23, %24) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<1x8x1xi32, strided<[8, 16, 1], offset: ?>>, memref<1x8x1xi32>) -> ()
    %25 = "bufferization.to_tensor"(%24) <{restrict, writable}> : (memref<1x8x1xi32>) -> tensor<1x8x1xi32>
    %26 = "tensor.empty"() : () -> tensor<1x8x1xi32>
    %27 = "hivm.hir.vand"(%22, %25, %26) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1x8x1xi32>, tensor<1x8x1xi32>, tensor<1x8x1xi32>) -> tensor<1x8x1xi32>
    %28 = "memref.reinterpret_cast"(%arg3, %19) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1, 8, 1>, static_strides = array<i64: 8, 16, 1>}> : (memref<?xi32>, index) -> memref<1x8x1xi32, strided<[8, 16, 1], offset: ?>>
    "hivm.hir.store"(%27, %28) : (tensor<1x8x1xi32>, memref<1x8x1xi32, strided<[8, 16, 1], offset: ?>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, false, false, false]> : vector<9xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hacc.target = #hacc.target<"Ascend910B1">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

