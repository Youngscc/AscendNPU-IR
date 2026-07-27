#map = affine_map<()[s0] -> (s0 * 128)>
"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xbf16>, memref<?xf32>, memref<?xf32>, memref<?xbf16>, memref<?xf32>, memref<?xf32>, memref<?xbf16>, memref<?xf32>, memref<?xf32>, i32, i32, i32) -> (), sym_name = "tl_fn_forward_update"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xbf16>, %arg4: memref<?xf32>, %arg5: memref<?xf32>, %arg6: memref<?xbf16>, %arg7: memref<?xf32>, %arg8: memref<?xf32>, %arg9: memref<?xbf16>, %arg10: memref<?xf32>, %arg11: memref<?xf32>, %arg12: i32, %arg13: i32, %arg14: i32):
    %0 = "arith.constant"() <{value = 256 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = 32 : i32}> : () -> i32
    "hivm.hir.set_mask_norm"() : () -> ()
    %2 = "arith.muli"(%arg12, %arg13) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %3 = "arith.muli"(%2, %arg14) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%3) <{effects = ["write"]}> {logical_block_num} : (i32) -> ()
    %4 = "hivm.hir.get_block_idx"() : () -> i64
    %5 = "arith.trunci"(%4) : (i64) -> i32
    %6 = "arith.muli"(%arg14, %arg13) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %7 = "arith.divsi"(%5, %6) : (i32, i32) -> i32
    %8 = "arith.remsi"(%7, %arg12) : (i32, i32) -> i32
    %9 = "arith.remsi"(%8, %1) : (i32, i32) -> i32
    %10 = "arith.muli"(%9, %1) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %11 = "arith.muli"(%9, %0) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %12 = "arith.index_cast"(%11) : (i32) -> index
    %13 = "memref.reinterpret_cast"(%arg7, %12) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64, 8>, static_strides = array<i64: 8, 1>}> : (memref<?xf32>, index) -> memref<64x8xf32, strided<[8, 1], offset: ?>>
    %14 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<64x8xf32>
    "hivm.hir.load"(%13, %14) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<64x8xf32, strided<[8, 1], offset: ?>>, memref<64x8xf32>) -> ()
    %15 = "bufferization.to_tensor"(%14) <{restrict, writable}> : (memref<64x8xf32>) -> tensor<64x8xf32>
    %16 = "memref.reinterpret_cast"(%arg10, %12) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64, 8>, static_strides = array<i64: 8, 1>}> : (memref<?xf32>, index) -> memref<64x8xf32, strided<[8, 1], offset: ?>>
    %17 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<64x8xf32>
    "hivm.hir.load"(%16, %17) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<64x8xf32, strided<[8, 1], offset: ?>>, memref<64x8xf32>) -> ()
    %18 = "bufferization.to_tensor"(%17) <{restrict, writable}> : (memref<64x8xf32>) -> tensor<64x8xf32>
    %19 = "tensor.empty"() : () -> tensor<64x8xf32>
    %20 = "tensor.empty"() : () -> tensor<64x8xf32>
    %21 = "tensor.empty"() : () -> tensor<64x8xf32>
    %22 = "tensor.empty"() : () -> tensor<64x8xf32>
    %23 = "tensor.empty"() : () -> tensor<64x8xf32>
    %24 = "tensor.empty"() : () -> tensor<64x8xf32>
    %25 = "tensor.empty"() : () -> tensor<64x8xf32>
    %26 = "tensor.empty"() : () -> tensor<64x8xf32>
    %27 = "tensor.empty"() : () -> tensor<64x8xf32>
    %28 = "tensor.empty"() : () -> tensor<64x8xf32>
    %29 = "tensor.empty"() : () -> tensor<64x8xf32>
    %30 = "tensor.empty"() : () -> tensor<64x8xf32>
    %31 = "tensor.empty"() : () -> tensor<64x8xi1>
    %32 = "tensor.empty"() : () -> tensor<64x8xi1>
    %33 = "tensor.empty"() : () -> tensor<64x8xi1>
    %34 = "tensor.empty"() : () -> tensor<64x8xi1>
    %35 = "hivm.hir.vcmp"(%15, %15, %34) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x8xf32>, tensor<64x8xf32>, tensor<64x8xi1>) -> tensor<64x8xi1>
    %36 = "hivm.hir.vnot"(%35, %33) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<64x8xi1>, tensor<64x8xi1>) -> tensor<64x8xi1>
    %37 = "hivm.hir.vcmp"(%18, %18, %32) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x8xf32>, tensor<64x8xf32>, tensor<64x8xi1>) -> tensor<64x8xi1>
    %38 = "hivm.hir.vnot"(%37, %31) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<64x8xi1>, tensor<64x8xi1>) -> tensor<64x8xi1>
    %39 = "hivm.hir.vmax"(%15, %18, %30) <{broadcast = array<i64>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x8xf32>, tensor<64x8xf32>, tensor<64x8xf32>) -> tensor<64x8xf32>
    %40 = "hivm.hir.vsel"(%36, %18, %39, %29) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<64x8xi1>, tensor<64x8xf32>, tensor<64x8xf32>, tensor<64x8xf32>) -> tensor<64x8xf32>
    %41 = "hivm.hir.vsel"(%38, %15, %40, %28) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<64x8xi1>, tensor<64x8xf32>, tensor<64x8xf32>, tensor<64x8xf32>) -> tensor<64x8xf32>
    %42 = "hivm.hir.vsub"(%15, %41, %27) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x8xf32>, tensor<64x8xf32>, tensor<64x8xf32>) -> tensor<64x8xf32>
    %43 = "hivm.hir.vexp"(%42, %26) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<64x8xf32>, tensor<64x8xf32>) -> tensor<64x8xf32>
    %44 = "hivm.hir.vsub"(%18, %41, %25) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x8xf32>, tensor<64x8xf32>, tensor<64x8xf32>) -> tensor<64x8xf32>
    %45 = "hivm.hir.vexp"(%44, %24) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<64x8xf32>, tensor<64x8xf32>) -> tensor<64x8xf32>
    %46 = "memref.reinterpret_cast"(%arg8, %12) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64, 8>, static_strides = array<i64: 8, 1>}> : (memref<?xf32>, index) -> memref<64x8xf32, strided<[8, 1], offset: ?>>
    %47 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<64x8xf32>
    "hivm.hir.load"(%46, %47) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<64x8xf32, strided<[8, 1], offset: ?>>, memref<64x8xf32>) -> ()
    %48 = "bufferization.to_tensor"(%47) <{restrict, writable}> : (memref<64x8xf32>) -> tensor<64x8xf32>
    %49 = "memref.reinterpret_cast"(%arg11, %12) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64, 8>, static_strides = array<i64: 8, 1>}> : (memref<?xf32>, index) -> memref<64x8xf32, strided<[8, 1], offset: ?>>
    %50 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<64x8xf32>
    "hivm.hir.load"(%49, %50) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<64x8xf32, strided<[8, 1], offset: ?>>, memref<64x8xf32>) -> ()
    %51 = "bufferization.to_tensor"(%50) <{restrict, writable}> : (memref<64x8xf32>) -> tensor<64x8xf32>
    %52 = "hivm.hir.vmul"(%48, %43, %23) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x8xf32>, tensor<64x8xf32>, tensor<64x8xf32>) -> tensor<64x8xf32>
    %53 = "hivm.hir.vmul"(%51, %45, %22) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x8xf32>, tensor<64x8xf32>, tensor<64x8xf32>) -> tensor<64x8xf32>
    %54 = "hivm.hir.vadd"(%52, %53, %21) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x8xf32>, tensor<64x8xf32>, tensor<64x8xf32>) -> tensor<64x8xf32>
    %55 = "hivm.hir.vdiv"(%52, %54, %20) <{broadcast = array<i64>, isHP = false, isSigned = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x8xf32>, tensor<64x8xf32>, tensor<64x8xf32>) -> tensor<64x8xf32>
    %56 = "hivm.hir.vdiv"(%53, %54, %19) <{broadcast = array<i64>, isHP = false, isSigned = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x8xf32>, tensor<64x8xf32>, tensor<64x8xf32>) -> tensor<64x8xf32>
    %57 = "memref.reinterpret_cast"(%arg4, %12) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64, 8>, static_strides = array<i64: 8, 1>}> : (memref<?xf32>, index) -> memref<64x8xf32, strided<[8, 1], offset: ?>>
    "hivm.hir.store"(%41, %57) : (tensor<64x8xf32>, memref<64x8xf32, strided<[8, 1], offset: ?>>) -> ()
    %58 = "memref.reinterpret_cast"(%arg5, %12) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64, 8>, static_strides = array<i64: 8, 1>}> : (memref<?xf32>, index) -> memref<64x8xf32, strided<[8, 1], offset: ?>>
    "hivm.hir.store"(%54, %58) : (tensor<64x8xf32>, memref<64x8xf32, strided<[8, 1], offset: ?>>) -> ()
    %59 = "arith.index_cast"(%10) : (i32) -> index
    %60 = "affine.apply"(%59) <{map = #map}> : (index) -> index
    %61 = "memref.reinterpret_cast"(%arg6, %60) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64, 8, 8>, static_strides = array<i64: 128, 8, 1>}> : (memref<?xbf16>, index) -> memref<64x8x8xbf16, strided<[128, 8, 1], offset: ?>>
    %62 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<64x8x8xbf16>
    "hivm.hir.load"(%61, %62) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<64x8x8xbf16, strided<[128, 8, 1], offset: ?>>, memref<64x8x8xbf16>) -> ()
    %63 = "bufferization.to_tensor"(%62) <{restrict, writable}> : (memref<64x8x8xbf16>) -> tensor<64x8x8xbf16>
    %64 = "memref.reinterpret_cast"(%arg9, %60) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64, 8, 8>, static_strides = array<i64: 128, 8, 1>}> : (memref<?xbf16>, index) -> memref<64x8x8xbf16, strided<[128, 8, 1], offset: ?>>
    %65 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<64x8x8xbf16>
    "hivm.hir.load"(%64, %65) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<64x8x8xbf16, strided<[128, 8, 1], offset: ?>>, memref<64x8x8xbf16>) -> ()
    %66 = "bufferization.to_tensor"(%65) <{restrict, writable}> : (memref<64x8x8xbf16>) -> tensor<64x8x8xbf16>
    %67 = "tensor.empty"() : () -> tensor<64x8x8xf32>
    %68 = "tensor.empty"() : () -> tensor<64x8x8xf32>
    %69 = "tensor.empty"() : () -> tensor<64x8x8xf32>
    %70 = "tensor.empty"() : () -> tensor<64x8x8xf32>
    %71 = "tensor.empty"() : () -> tensor<64x8x8xf32>
    %72 = "tensor.empty"() : () -> tensor<64x8x8xf32>
    %73 = "tensor.empty"() : () -> tensor<64x8x8xf32>
    %74 = "hivm.hir.vcast"(%63, %73) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<64x8x8xbf16>, tensor<64x8x8xf32>) -> tensor<64x8x8xf32>
    %75 = "hivm.hir.vcast"(%66, %72) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<64x8x8xbf16>, tensor<64x8x8xf32>) -> tensor<64x8x8xf32>
    %76 = "tensor.expand_shape"(%55) <{reassociation = [[0, 1], [2]], static_output_shape = array<i64: 64, 1, 8>}> : (tensor<64x8xf32>) -> tensor<64x1x8xf32>
    %77 = "hivm.hir.vbrc"(%76, %71) <{broadcast_dims = array<i64: 1>}> : (tensor<64x1x8xf32>, tensor<64x8x8xf32>) -> tensor<64x8x8xf32>
    %78 = "tensor.expand_shape"(%56) <{reassociation = [[0, 1], [2]], static_output_shape = array<i64: 64, 1, 8>}> : (tensor<64x8xf32>) -> tensor<64x1x8xf32>
    %79 = "hivm.hir.vbrc"(%78, %70) <{broadcast_dims = array<i64: 1>}> : (tensor<64x1x8xf32>, tensor<64x8x8xf32>) -> tensor<64x8x8xf32>
    %80 = "hivm.hir.vmul"(%74, %77, %69) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x8x8xf32>, tensor<64x8x8xf32>, tensor<64x8x8xf32>) -> tensor<64x8x8xf32>
    %81 = "hivm.hir.vmul"(%75, %79, %68) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x8x8xf32>, tensor<64x8x8xf32>, tensor<64x8x8xf32>) -> tensor<64x8x8xf32>
    %82 = "hivm.hir.vadd"(%80, %81, %67) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x8x8xf32>, tensor<64x8x8xf32>, tensor<64x8x8xf32>) -> tensor<64x8x8xf32>
    %83 = "tensor.empty"() : () -> tensor<64x8x8xbf16>
    %84 = "hivm.hir.vcast"(%82, %83) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<64x8x8xf32>, tensor<64x8x8xbf16>) -> tensor<64x8x8xbf16>
    %85 = "memref.reinterpret_cast"(%arg3, %60) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64, 8, 8>, static_strides = array<i64: 128, 8, 1>}> : (memref<?xbf16>, index) -> memref<64x8x8xbf16, strided<[128, 8, 1], offset: ?>>
    "hivm.hir.store"(%84, %85) : (tensor<64x8x8xbf16>, memref<64x8x8xbf16, strided<[128, 8, 1], offset: ?>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, true, true, true, true, true, true, false, false, false]> : vector<15xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hacc.target = #hacc.target<"Ascend910B1">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

