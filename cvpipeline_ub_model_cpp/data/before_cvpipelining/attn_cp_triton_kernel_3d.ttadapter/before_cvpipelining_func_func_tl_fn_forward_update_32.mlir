"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xbf16>, memref<?xf32>, memref<?xf32>, memref<?xbf16>, memref<?xf32>, memref<?xf32>, memref<?xbf16>, memref<?xf32>, memref<?xf32>, i32, i32, i32) -> (), sym_name = "tl_fn_forward_update"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xbf16>, %arg4: memref<?xf32>, %arg5: memref<?xf32>, %arg6: memref<?xbf16>, %arg7: memref<?xf32>, %arg8: memref<?xf32>, %arg9: memref<?xbf16>, %arg10: memref<?xf32>, %arg11: memref<?xf32>, %arg12: i32, %arg13: i32, %arg14: i32):
    %0 = "arith.constant"() <{value = 256 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = 32 : i32}> : () -> i32
    %2 = "arith.constant"() <{value = 128 : index}> : () -> index
    "hivm.hir.set_mask_norm"() : () -> ()
    %3 = "arith.muli"(%arg12, %arg13) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %4 = "arith.muli"(%3, %arg14) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%4) {logical_block_num} : (i32) -> ()
    %5 = "hivm.hir.get_block_idx"() : () -> i64
    %6 = "arith.trunci"(%5) : (i64) -> i32
    %7 = "arith.muli"(%arg14, %arg13) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %8 = "arith.divsi"(%6, %7) : (i32, i32) -> i32
    %9 = "arith.remsi"(%8, %arg12) : (i32, i32) -> i32
    %10 = "arith.remsi"(%9, %1) : (i32, i32) -> i32
    %11 = "arith.muli"(%10, %1) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %12 = "arith.muli"(%10, %0) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %13 = "arith.index_cast"(%12) : (i32) -> index
    %14 = "memref.reinterpret_cast"(%arg7, %13) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64, 8>, static_strides = array<i64: 8, 1>}> : (memref<?xf32>, index) -> memref<64x8xf32, strided<[8, 1], offset: ?>>
    %15 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<64x8xf32>
    "hivm.hir.load"(%14, %15) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<64x8xf32, strided<[8, 1], offset: ?>>, memref<64x8xf32>) -> ()
    %16 = "bufferization.to_tensor"(%15) <{restrict, writable}> : (memref<64x8xf32>) -> tensor<64x8xf32>
    %17 = "memref.reinterpret_cast"(%arg10, %13) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64, 8>, static_strides = array<i64: 8, 1>}> : (memref<?xf32>, index) -> memref<64x8xf32, strided<[8, 1], offset: ?>>
    %18 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<64x8xf32>
    "hivm.hir.load"(%17, %18) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<64x8xf32, strided<[8, 1], offset: ?>>, memref<64x8xf32>) -> ()
    %19 = "bufferization.to_tensor"(%18) <{restrict, writable}> : (memref<64x8xf32>) -> tensor<64x8xf32>
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
    %31 = "tensor.empty"() : () -> tensor<64x8xf32>
    %32 = "tensor.empty"() : () -> tensor<64x8xi1>
    %33 = "tensor.empty"() : () -> tensor<64x8xi1>
    %34 = "tensor.empty"() : () -> tensor<64x8xi1>
    %35 = "tensor.empty"() : () -> tensor<64x8xi1>
    %36 = "hivm.hir.vcmp"(%16, %16, %35) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<64x8xf32>, tensor<64x8xf32>, tensor<64x8xi1>) -> tensor<64x8xi1>
    %37 = "hivm.hir.vnot"(%36, %34) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<64x8xi1>, tensor<64x8xi1>) -> tensor<64x8xi1>
    %38 = "hivm.hir.vcmp"(%19, %19, %33) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<64x8xf32>, tensor<64x8xf32>, tensor<64x8xi1>) -> tensor<64x8xi1>
    %39 = "hivm.hir.vnot"(%38, %32) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<64x8xi1>, tensor<64x8xi1>) -> tensor<64x8xi1>
    %40 = "hivm.hir.vmax"(%16, %19, %31) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x8xf32>, tensor<64x8xf32>, tensor<64x8xf32>) -> tensor<64x8xf32>
    %41 = "hivm.hir.vsel"(%37, %19, %40, %30) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<64x8xi1>, tensor<64x8xf32>, tensor<64x8xf32>, tensor<64x8xf32>) -> tensor<64x8xf32>
    %42 = "hivm.hir.vsel"(%39, %16, %41, %29) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<64x8xi1>, tensor<64x8xf32>, tensor<64x8xf32>, tensor<64x8xf32>) -> tensor<64x8xf32>
    %43 = "hivm.hir.vsub"(%16, %42, %28) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x8xf32>, tensor<64x8xf32>, tensor<64x8xf32>) -> tensor<64x8xf32>
    %44 = "hivm.hir.vexp"(%43, %27) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<64x8xf32>, tensor<64x8xf32>) -> tensor<64x8xf32>
    %45 = "hivm.hir.vsub"(%19, %42, %26) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x8xf32>, tensor<64x8xf32>, tensor<64x8xf32>) -> tensor<64x8xf32>
    %46 = "hivm.hir.vexp"(%45, %25) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<64x8xf32>, tensor<64x8xf32>) -> tensor<64x8xf32>
    %47 = "memref.reinterpret_cast"(%arg8, %13) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64, 8>, static_strides = array<i64: 8, 1>}> : (memref<?xf32>, index) -> memref<64x8xf32, strided<[8, 1], offset: ?>>
    %48 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<64x8xf32>
    "hivm.hir.load"(%47, %48) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<64x8xf32, strided<[8, 1], offset: ?>>, memref<64x8xf32>) -> ()
    %49 = "bufferization.to_tensor"(%48) <{restrict, writable}> : (memref<64x8xf32>) -> tensor<64x8xf32>
    %50 = "memref.reinterpret_cast"(%arg11, %13) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64, 8>, static_strides = array<i64: 8, 1>}> : (memref<?xf32>, index) -> memref<64x8xf32, strided<[8, 1], offset: ?>>
    %51 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<64x8xf32>
    "hivm.hir.load"(%50, %51) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<64x8xf32, strided<[8, 1], offset: ?>>, memref<64x8xf32>) -> ()
    %52 = "bufferization.to_tensor"(%51) <{restrict, writable}> : (memref<64x8xf32>) -> tensor<64x8xf32>
    %53 = "hivm.hir.vmul"(%49, %44, %24) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x8xf32>, tensor<64x8xf32>, tensor<64x8xf32>) -> tensor<64x8xf32>
    %54 = "hivm.hir.vmul"(%52, %46, %23) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x8xf32>, tensor<64x8xf32>, tensor<64x8xf32>) -> tensor<64x8xf32>
    %55 = "hivm.hir.vadd"(%53, %54, %22) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x8xf32>, tensor<64x8xf32>, tensor<64x8xf32>) -> tensor<64x8xf32>
    %56 = "hivm.hir.vdiv"(%53, %55, %21) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x8xf32>, tensor<64x8xf32>, tensor<64x8xf32>) -> tensor<64x8xf32>
    %57 = "hivm.hir.vdiv"(%54, %55, %20) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x8xf32>, tensor<64x8xf32>, tensor<64x8xf32>) -> tensor<64x8xf32>
    %58 = "memref.reinterpret_cast"(%arg4, %13) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64, 8>, static_strides = array<i64: 8, 1>}> : (memref<?xf32>, index) -> memref<64x8xf32, strided<[8, 1], offset: ?>>
    "hivm.hir.store"(%42, %58) : (tensor<64x8xf32>, memref<64x8xf32, strided<[8, 1], offset: ?>>) -> ()
    %59 = "memref.reinterpret_cast"(%arg5, %13) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64, 8>, static_strides = array<i64: 8, 1>}> : (memref<?xf32>, index) -> memref<64x8xf32, strided<[8, 1], offset: ?>>
    "hivm.hir.store"(%55, %59) : (tensor<64x8xf32>, memref<64x8xf32, strided<[8, 1], offset: ?>>) -> ()
    %60 = "arith.index_cast"(%11) : (i32) -> index
    %61 = "arith.muli"(%60, %2) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %62 = "memref.reinterpret_cast"(%arg6, %61) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64, 8, 8>, static_strides = array<i64: 128, 8, 1>}> : (memref<?xbf16>, index) -> memref<64x8x8xbf16, strided<[128, 8, 1], offset: ?>>
    %63 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<64x8x8xbf16>
    "hivm.hir.load"(%62, %63) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<64x8x8xbf16, strided<[128, 8, 1], offset: ?>>, memref<64x8x8xbf16>) -> ()
    %64 = "bufferization.to_tensor"(%63) <{restrict, writable}> : (memref<64x8x8xbf16>) -> tensor<64x8x8xbf16>
    %65 = "memref.reinterpret_cast"(%arg9, %61) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64, 8, 8>, static_strides = array<i64: 128, 8, 1>}> : (memref<?xbf16>, index) -> memref<64x8x8xbf16, strided<[128, 8, 1], offset: ?>>
    %66 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<64x8x8xbf16>
    "hivm.hir.load"(%65, %66) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<64x8x8xbf16, strided<[128, 8, 1], offset: ?>>, memref<64x8x8xbf16>) -> ()
    %67 = "bufferization.to_tensor"(%66) <{restrict, writable}> : (memref<64x8x8xbf16>) -> tensor<64x8x8xbf16>
    %68 = "tensor.empty"() : () -> tensor<64x8x8xf32>
    %69 = "tensor.empty"() : () -> tensor<64x8x8xf32>
    %70 = "tensor.empty"() : () -> tensor<64x8x8xf32>
    %71 = "tensor.empty"() : () -> tensor<64x8x8xf32>
    %72 = "tensor.empty"() : () -> tensor<64x8x8xf32>
    %73 = "tensor.empty"() : () -> tensor<64x8x8xf32>
    %74 = "tensor.empty"() : () -> tensor<64x8x8xf32>
    %75 = "hivm.hir.vcast"(%64, %74) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<64x8x8xbf16>, tensor<64x8x8xf32>) -> tensor<64x8x8xf32>
    %76 = "hivm.hir.vcast"(%67, %73) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<64x8x8xbf16>, tensor<64x8x8xf32>) -> tensor<64x8x8xf32>
    %77 = "tensor.expand_shape"(%56) <{reassociation = [[0, 1], [2]], static_output_shape = array<i64: 64, 1, 8>}> : (tensor<64x8xf32>) -> tensor<64x1x8xf32>
    %78 = "hivm.hir.vbrc"(%77, %72) <{broadcast_dims = array<i64: 1>}> : (tensor<64x1x8xf32>, tensor<64x8x8xf32>) -> tensor<64x8x8xf32>
    %79 = "tensor.expand_shape"(%57) <{reassociation = [[0, 1], [2]], static_output_shape = array<i64: 64, 1, 8>}> : (tensor<64x8xf32>) -> tensor<64x1x8xf32>
    %80 = "hivm.hir.vbrc"(%79, %71) <{broadcast_dims = array<i64: 1>}> : (tensor<64x1x8xf32>, tensor<64x8x8xf32>) -> tensor<64x8x8xf32>
    %81 = "hivm.hir.vmul"(%75, %78, %70) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x8x8xf32>, tensor<64x8x8xf32>, tensor<64x8x8xf32>) -> tensor<64x8x8xf32>
    %82 = "hivm.hir.vmul"(%76, %80, %69) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x8x8xf32>, tensor<64x8x8xf32>, tensor<64x8x8xf32>) -> tensor<64x8x8xf32>
    %83 = "hivm.hir.vadd"(%81, %82, %68) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x8x8xf32>, tensor<64x8x8xf32>, tensor<64x8x8xf32>) -> tensor<64x8x8xf32>
    %84 = "tensor.empty"() : () -> tensor<64x8x8xbf16>
    %85 = "hivm.hir.vcast"(%83, %84) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<64x8x8xf32>, tensor<64x8x8xbf16>) -> tensor<64x8x8xbf16>
    %86 = "memref.reinterpret_cast"(%arg3, %61) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64, 8, 8>, static_strides = array<i64: 128, 8, 1>}> : (memref<?xbf16>, index) -> memref<64x8x8xbf16, strided<[128, 8, 1], offset: ?>>
    "hivm.hir.store"(%85, %86) : (tensor<64x8x8xbf16>, memref<64x8x8xbf16, strided<[128, 8, 1], offset: ?>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, true, true, true, true, true, true, false, false, false]> : vector<15xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

