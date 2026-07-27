#map = affine_map<()[s0] -> (s0 * 4)>
#map1 = affine_map<()[s0, s1] -> (s0 + s1)>
#map2 = affine_map<()[s0] -> (s0 floordiv 4)>
#map3 = affine_map<()[s0, s1] -> (s0 - s1)>
#map4 = affine_map<()[s0] -> (s0, 0)>
#map5 = affine_map<()[s0] -> (s0, 64)>
#map6 = affine_map<()[s0, s1] -> (s0, s1)>
"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xbf16>, memref<?xbf16>, i32, i32, i32, i32) -> (), sym_name = "chunk_local_cumsum_scalar_kernel"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xbf16>, %arg4: memref<?xbf16>, %arg5: i32, %arg6: i32, %arg7: i32, %arg8: i32):
    %0 = "arith.constant"() <{value = 0.000000e+00 : bf16}> : () -> bf16
    %1 = "arith.constant"() <{value = 64 : index}> : () -> index
    %2 = "arith.constant"() <{value = 0 : index}> : () -> index
    %3 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %4 = "arith.constant"() <{value = 4 : i32}> : () -> i32
    %5 = "arith.constant"() <{value = 64 : i32}> : () -> i32
    "hivm.hir.set_mask_norm"() : () -> ()
    %6 = "arith.muli"(%arg6, %arg7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %7 = "arith.muli"(%6, %arg8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%7) <{effects = ["write"]}> {logical_block_num} : (i32) -> ()
    %8 = "hivm.hir.get_block_idx"() : () -> i64
    %9 = "arith.trunci"(%8) : (i64) -> i32
    %10 = "arith.divsi"(%9, %arg8) : (i32, i32) -> i32
    %11 = "arith.remsi"(%10, %arg7) : (i32, i32) -> i32
    %12 = "arith.muli"(%arg8, %arg7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %13 = "arith.divsi"(%9, %12) : (i32, i32) -> i32
    %14 = "arith.remsi"(%13, %arg6) : (i32, i32) -> i32
    %15 = "arith.muli"(%11, %arg5) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %16 = "arith.muli"(%15, %4) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %17 = "arith.index_cast"(%16) : (i32) -> index
    %18 = "arith.muli"(%14, %5) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %19 = "arith.maxsi"(%18, %3) : (i32, i32) -> i32
    %20 = "arith.index_cast"(%19) : (i32) -> index
    %21 = "affine.apply"(%20) <{map = #map}> : (index) -> index
    %22 = "affine.apply"(%21, %17) <{map = #map1}> : (index, index) -> index
    %23 = "arith.index_cast"(%arg5) : (i32) -> index
    %24 = "memref.reinterpret_cast"(%arg3, %22) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64, 4>, static_strides = array<i64: 4, 1>}> : (memref<?xbf16>, index) -> memref<64x4xbf16, strided<[4, 1], offset: ?>>
    %25 = "memref.reinterpret_cast"(%arg4, %22) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64, 4>, static_strides = array<i64: 4, 1>}> : (memref<?xbf16>, index) -> memref<64x4xbf16, strided<[4, 1], offset: ?>>
    %26 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<64x4xbf16>
    %27 = "affine.apply"(%21) <{map = #map2}> : (index) -> index
    %28 = "affine.apply"(%23, %27) <{map = #map3}> : (index, index) -> index
    %29 = "affine.max"(%28) <{map = #map4}> : (index) -> index
    %30 = "affine.min"(%29) <{map = #map5}> : (index) -> index
    %31 = "arith.subi"(%3, %18) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %32 = "arith.maxsi"(%31, %3) : (i32, i32) -> i32
    %33 = "arith.index_cast"(%32) : (i32) -> index
    %34 = "affine.min"(%33, %30) <{map = #map6}> : (index, index) -> index
    %35 = "affine.apply"(%30, %34) <{map = #map3}> : (index, index) -> index
    %36 = "arith.cmpi"(%35, %1) <{predicate = 2 : i64}> : (index, index) -> i1
    %37 = "memref.subview"(%24, %35) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 4>, static_strides = array<i64: 1, 1>}> : (memref<64x4xbf16, strided<[4, 1], offset: ?>>, index) -> memref<?x4xbf16, strided<[4, 1], offset: ?>>
    %38 = "memref.subview"(%26, %34, %35) <{operandSegmentSizes = array<i32: 1, 1, 1, 0>, static_offsets = array<i64: -9223372036854775808, 0>, static_sizes = array<i64: -9223372036854775808, 4>, static_strides = array<i64: 1, 1>}> : (memref<64x4xbf16>, index, index) -> memref<?x4xbf16, strided<[4, 1], offset: ?>>
    "hivm.hir.load"(%37, %38, %0, %2, %36) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x4xbf16, strided<[4, 1], offset: ?>>, memref<?x4xbf16, strided<[4, 1], offset: ?>>, bf16, index, i1) -> ()
    %39 = "bufferization.to_tensor"(%26) <{restrict, writable}> : (memref<64x4xbf16>) -> tensor<64x4xbf16>
    %40 = "tensor.empty"() : () -> tensor<64x4xf32>
    %41 = "hivm.hir.vcast"(%39, %40) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<64x4xbf16>, tensor<64x4xf32>) -> tensor<64x4xf32>
    %42 = "tensor.expand_shape"(%41) <{reassociation = [[0, 1], [2]], static_output_shape = array<i64: 64, 1, 4>}> : (tensor<64x4xf32>) -> tensor<64x1x4xf32>
    %43 = "tensor.empty"() : () -> tensor<64x1x4xf32>
    %44 = "hivm.hir.vcumsum"(%42, %43) <{cum_dims = array<i64: 0>, reverse = false}> : (tensor<64x1x4xf32>, tensor<64x1x4xf32>) -> tensor<64x1x4xf32>
    %45 = "tensor.empty"() : () -> tensor<1x64x4xf32>
    %46 = "hivm.hir.vtranspose"(%44, %45) <{disable_align = false, permutation = array<i64: 1, 0, 2>}> : (tensor<64x1x4xf32>, tensor<1x64x4xf32>) -> tensor<1x64x4xf32>
    %47 = "tensor.collapse_shape"(%46) <{reassociation = [[0, 1], [2]]}> : (tensor<1x64x4xf32>) -> tensor<64x4xf32>
    %48 = "tensor.empty"() : () -> tensor<64x4xbf16>
    %49 = "hivm.hir.vcast"(%47, %48) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<64x4xf32>, tensor<64x4xbf16>) -> tensor<64x4xbf16>
    %50 = "tensor.extract_slice"(%49, %34, %35) <{operandSegmentSizes = array<i32: 1, 1, 1, 0>, static_offsets = array<i64: -9223372036854775808, 0>, static_sizes = array<i64: -9223372036854775808, 4>, static_strides = array<i64: 1, 1>}> : (tensor<64x4xbf16>, index, index) -> tensor<?x4xbf16>
    %51 = "memref.subview"(%25, %35) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 4>, static_strides = array<i64: 1, 1>}> : (memref<64x4xbf16, strided<[4, 1], offset: ?>>, index) -> memref<?x4xbf16, strided<[4, 1], offset: ?>>
    "hivm.hir.store"(%50, %51) : (tensor<?x4xbf16>, memref<?x4xbf16, strided<[4, 1], offset: ?>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, false, false, false, false]> : vector<9xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hacc.target = #hacc.target<"Ascend910B1">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

