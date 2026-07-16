"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {}, {tt.divisibility = 16 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xbf16>, memref<?xbf16>, f32, i32, i32, i32, i32) -> (), sym_name = "l2norm_fwd_kernel2_loop"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xbf16>, %arg4: memref<?xbf16>, %arg5: f32, %arg6: i32, %arg7: i32, %arg8: i32, %arg9: i32):
    %0 = "arith.constant"() <{value = 0.000000e+00 : bf16}> : () -> bf16
    %1 = "arith.constant"() <{value = 16 : index}> : () -> index
    %2 = "arith.constant"() <{value = 64 : index}> : () -> index
    %3 = "arith.constant"() <{value = 16 : i32}> : () -> i32
    %4 = "arith.constant"() <{value = 0 : index}> : () -> index
    "hivm.hir.set_mask_norm"() : () -> ()
    %5 = "arith.muli"(%arg7, %arg8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %6 = "arith.muli"(%5, %arg9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%6) {logical_block_num} : (i32) -> ()
    %7 = "hivm.hir.get_block_idx"() : () -> i64
    %8 = "arith.trunci"(%7) : (i64) -> i32
    %9 = "arith.muli"(%arg9, %arg8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %10 = "arith.divsi"(%8, %9) : (i32, i32) -> i32
    %11 = "arith.remsi"(%10, %arg7) : (i32, i32) -> i32
    %12 = "arith.muli"(%11, %3) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %13 = "arith.index_cast"(%12) : (i32) -> index
    %14 = "arith.muli"(%13, %2) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %15 = "memref.reinterpret_cast"(%arg3, %14) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 64>, static_strides = array<i64: 64, 1>}> : (memref<?xbf16>, index) -> memref<16x64xbf16, strided<[64, 1], offset: ?>>
    %16 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x64xbf16>
    %17 = "arith.addi"(%13, %1) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %18 = "arith.index_cast"(%arg6) : (i32) -> index
    %19 = "arith.maxsi"(%13, %18) : (index, index) -> index
    %20 = "arith.minsi"(%17, %19) : (index, index) -> index
    %21 = "arith.subi"(%20, %13) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %22 = "arith.cmpi"(%21, %1) <{predicate = 2 : i64}> : (index, index) -> i1
    %23 = "memref.subview"(%15, %21) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 64>, static_strides = array<i64: 1, 1>}> : (memref<16x64xbf16, strided<[64, 1], offset: ?>>, index) -> memref<?x64xbf16, strided<[64, 1], offset: ?>>
    %24 = "memref.subview"(%16, %21) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 64>, static_strides = array<i64: 1, 1>}> : (memref<16x64xbf16>, index) -> memref<?x64xbf16, strided<[64, 1]>>
    "hivm.hir.load"(%23, %24, %0, %4, %22) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x64xbf16, strided<[64, 1], offset: ?>>, memref<?x64xbf16, strided<[64, 1]>>, bf16, index, i1) -> ()
    %25 = "bufferization.to_tensor"(%16) <{restrict, writable}> : (memref<16x64xbf16>) -> tensor<16x64xbf16>
    %26 = "tensor.empty"() : () -> tensor<16x64xf32>
    %27 = "tensor.empty"() : () -> tensor<16x64xf32>
    %28 = "tensor.empty"() : () -> tensor<16x64xf32>
    %29 = "tensor.empty"() : () -> tensor<16x64xf32>
    %30 = "hivm.hir.vcast"(%25, %29) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x64xbf16>, tensor<16x64xf32>) -> tensor<16x64xf32>
    %31 = "hivm.hir.vmul"(%30, %30, %28) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x64xf32>, tensor<16x64xf32>, tensor<16x64xf32>) -> tensor<16x64xf32>
    %32 = "tensor.empty"() : () -> tensor<16x1xf32>
    %33 = "hivm.hir.vreduce"(%31, %32) <{arith = #hivm.reduce_op<sum>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 1>}> : (tensor<16x64xf32>, tensor<16x1xf32>) -> tensor<16x1xf32>
    %34 = "tensor.empty"() : () -> tensor<16x1xf32>
    %35 = "tensor.empty"() : () -> tensor<16x1xf32>
    %36 = "tensor.empty"() : () -> tensor<16x1xf32>
    %37 = "hivm.hir.vadd"(%33, %arg5, %36) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x1xf32>, f32, tensor<16x1xf32>) -> tensor<16x1xf32>
    %38 = "hivm.hir.vsqrt"(%37, %35) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16x1xf32>, tensor<16x1xf32>) -> tensor<16x1xf32>
    %39 = "hivm.hir.vrec"(%38, %34) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16x1xf32>, tensor<16x1xf32>) -> tensor<16x1xf32>
    %40 = "memref.reinterpret_cast"(%arg4, %14) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 64>, static_strides = array<i64: 64, 1>}> : (memref<?xbf16>, index) -> memref<16x64xbf16, strided<[64, 1], offset: ?>>
    %41 = "hivm.hir.vbrc"(%39, %27) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xf32>, tensor<16x64xf32>) -> tensor<16x64xf32>
    %42 = "hivm.hir.vmul"(%30, %41, %26) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x64xf32>, tensor<16x64xf32>, tensor<16x64xf32>) -> tensor<16x64xf32>
    %43 = "tensor.empty"() : () -> tensor<16x64xbf16>
    %44 = "hivm.hir.vcast"(%42, %43) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x64xf32>, tensor<16x64xbf16>) -> tensor<16x64xbf16>
    %45 = "tensor.extract_slice"(%44, %21) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 64>, static_strides = array<i64: 1, 1>}> : (tensor<16x64xbf16>, index) -> tensor<?x64xbf16>
    %46 = "memref.subview"(%40, %21) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 64>, static_strides = array<i64: 1, 1>}> : (memref<16x64xbf16, strided<[64, 1], offset: ?>>, index) -> memref<?x64xbf16, strided<[64, 1], offset: ?>>
    "hivm.hir.store"(%45, %46) : (tensor<?x64xbf16>, memref<?x64xbf16, strided<[64, 1], offset: ?>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, false, false, false, false, false]> : vector<10xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

