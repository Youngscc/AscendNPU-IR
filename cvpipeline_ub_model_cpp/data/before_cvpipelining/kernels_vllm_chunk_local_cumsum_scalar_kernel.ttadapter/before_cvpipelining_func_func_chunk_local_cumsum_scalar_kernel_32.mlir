"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xbf16>, memref<?xbf16>, i32, i32, i32, i32) -> (), sym_name = "chunk_local_cumsum_scalar_kernel"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xbf16>, %arg4: memref<?xbf16>, %arg5: i32, %arg6: i32, %arg7: i32, %arg8: i32):
    %0 = "arith.constant"() <{value = 0.000000e+00 : bf16}> : () -> bf16
    %1 = "arith.constant"() <{value = 64 : index}> : () -> index
    %2 = "arith.constant"() <{value = 0 : index}> : () -> index
    %3 = "arith.constant"() <{value = 4 : index}> : () -> index
    %4 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %5 = "arith.constant"() <{value = 4 : i32}> : () -> i32
    %6 = "arith.constant"() <{value = 64 : i32}> : () -> i32
    "hivm.hir.set_mask_norm"() : () -> ()
    %7 = "arith.muli"(%arg6, %arg7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %8 = "arith.muli"(%7, %arg8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%8) {logical_block_num} : (i32) -> ()
    %9 = "hivm.hir.get_block_idx"() : () -> i64
    %10 = "arith.trunci"(%9) : (i64) -> i32
    %11 = "arith.divsi"(%10, %arg8) : (i32, i32) -> i32
    %12 = "arith.remsi"(%11, %arg7) : (i32, i32) -> i32
    %13 = "arith.muli"(%arg8, %arg7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %14 = "arith.divsi"(%10, %13) : (i32, i32) -> i32
    %15 = "arith.remsi"(%14, %arg6) : (i32, i32) -> i32
    %16 = "arith.muli"(%12, %arg5) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %17 = "arith.muli"(%16, %5) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %18 = "arith.index_cast"(%17) : (i32) -> index
    %19 = "arith.muli"(%15, %6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %20 = "arith.maxsi"(%19, %4) : (i32, i32) -> i32
    %21 = "arith.index_cast"(%20) : (i32) -> index
    %22 = "arith.muli"(%21, %3) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %23 = "arith.addi"(%22, %18) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %24 = "arith.index_cast"(%arg5) : (i32) -> index
    %25 = "memref.reinterpret_cast"(%arg3, %23) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64, 4>, static_strides = array<i64: 4, 1>}> : (memref<?xbf16>, index) -> memref<64x4xbf16, strided<[4, 1], offset: ?>>
    %26 = "memref.reinterpret_cast"(%arg4, %23) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64, 4>, static_strides = array<i64: 4, 1>}> : (memref<?xbf16>, index) -> memref<64x4xbf16, strided<[4, 1], offset: ?>>
    %27 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<64x4xbf16>
    %28 = "arith.divsi"(%22, %3) : (index, index) -> index
    %29 = "arith.subi"(%24, %28) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %30 = "arith.maxsi"(%29, %2) : (index, index) -> index
    %31 = "arith.minsi"(%30, %1) : (index, index) -> index
    %32 = "arith.subi"(%4, %19) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %33 = "arith.maxsi"(%32, %4) : (i32, i32) -> i32
    %34 = "arith.index_cast"(%33) : (i32) -> index
    %35 = "arith.minsi"(%34, %31) : (index, index) -> index
    %36 = "arith.subi"(%31, %35) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %37 = "arith.cmpi"(%36, %1) <{predicate = 2 : i64}> : (index, index) -> i1
    %38 = "memref.subview"(%25, %36) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 4>, static_strides = array<i64: 1, 1>}> : (memref<64x4xbf16, strided<[4, 1], offset: ?>>, index) -> memref<?x4xbf16, strided<[4, 1], offset: ?>>
    %39 = "memref.subview"(%27, %35, %36) <{operandSegmentSizes = array<i32: 1, 1, 1, 0>, static_offsets = array<i64: -9223372036854775808, 0>, static_sizes = array<i64: -9223372036854775808, 4>, static_strides = array<i64: 1, 1>}> : (memref<64x4xbf16>, index, index) -> memref<?x4xbf16, strided<[4, 1], offset: ?>>
    "hivm.hir.load"(%38, %39, %0, %2, %37) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x4xbf16, strided<[4, 1], offset: ?>>, memref<?x4xbf16, strided<[4, 1], offset: ?>>, bf16, index, i1) -> ()
    %40 = "bufferization.to_tensor"(%27) <{restrict, writable}> : (memref<64x4xbf16>) -> tensor<64x4xbf16>
    %41 = "tensor.empty"() : () -> tensor<64x4xf32>
    %42 = "hivm.hir.vcast"(%40, %41) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<64x4xbf16>, tensor<64x4xf32>) -> tensor<64x4xf32>
    %43 = "tensor.expand_shape"(%42) <{reassociation = [[0, 1], [2]], static_output_shape = array<i64: 64, 1, 4>}> : (tensor<64x4xf32>) -> tensor<64x1x4xf32>
    %44 = "tensor.empty"() : () -> tensor<64x1x4xf32>
    %45 = "hivm.hir.vcumsum"(%43, %44) <{cum_dims = array<i64: 0>, reverse = false}> : (tensor<64x1x4xf32>, tensor<64x1x4xf32>) -> tensor<64x1x4xf32>
    %46 = "tensor.empty"() : () -> tensor<1x64x4xf32>
    %47 = "hivm.hir.vtranspose"(%45, %46) <{disable_align = false, permutation = array<i64: 1, 0, 2>}> : (tensor<64x1x4xf32>, tensor<1x64x4xf32>) -> tensor<1x64x4xf32>
    %48 = "tensor.collapse_shape"(%47) <{reassociation = [[0, 1], [2]]}> : (tensor<1x64x4xf32>) -> tensor<64x4xf32>
    %49 = "tensor.empty"() : () -> tensor<64x4xbf16>
    %50 = "hivm.hir.vcast"(%48, %49) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<64x4xf32>, tensor<64x4xbf16>) -> tensor<64x4xbf16>
    %51 = "tensor.extract_slice"(%50, %35, %36) <{operandSegmentSizes = array<i32: 1, 1, 1, 0>, static_offsets = array<i64: -9223372036854775808, 0>, static_sizes = array<i64: -9223372036854775808, 4>, static_strides = array<i64: 1, 1>}> : (tensor<64x4xbf16>, index, index) -> tensor<?x4xbf16>
    %52 = "memref.subview"(%26, %36) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 4>, static_strides = array<i64: 1, 1>}> : (memref<64x4xbf16, strided<[4, 1], offset: ?>>, index) -> memref<?x4xbf16, strided<[4, 1], offset: ?>>
    "hivm.hir.store"(%51, %52) : (tensor<?x4xbf16>, memref<?x4xbf16, strided<[4, 1], offset: ?>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, false, false, false, false]> : vector<9xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

