"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 2 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xf32>, i32, i32, i32, i32, i32) -> (), sym_name = "inplace_abs"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xf32>, %arg4: i32, %arg5: i32, %arg6: i32, %arg7: i32, %arg8: i32):
    %0 = "arith.constant"() <{value = 0.000000e+00 : f32}> : () -> f32
    %1 = "arith.constant"() <{value = 32 : index}> : () -> index
    %2 = "arith.constant"() <{value = 0 : index}> : () -> index
    %3 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %4 = "arith.constant"() <{value = 32 : i32}> : () -> i32
    "hivm.hir.set_mask_norm"() : () -> ()
    %5 = "arith.muli"(%arg6, %arg7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %6 = "arith.muli"(%5, %arg8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%6) {logical_block_num} : (i32) -> ()
    %7 = "hivm.hir.get_block_idx"() : () -> i64
    %8 = "arith.trunci"(%7) : (i64) -> i32
    %9 = "arith.divsi"(%8, %arg8) : (i32, i32) -> i32
    %10 = "arith.remsi"(%9, %arg7) : (i32, i32) -> i32
    %11 = "arith.muli"(%arg8, %arg7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %12 = "arith.divsi"(%8, %11) : (i32, i32) -> i32
    %13 = "arith.remsi"(%12, %arg6) : (i32, i32) -> i32
    %14 = "arith.muli"(%13, %4) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %15 = "arith.muli"(%10, %4) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %16 = "arith.maxsi"(%14, %3) : (i32, i32) -> i32
    %17 = "arith.index_cast"(%16) : (i32) -> index
    %18 = "arith.maxsi"(%15, %3) : (i32, i32) -> i32
    %19 = "arith.index_cast"(%18) : (i32) -> index
    %20 = "arith.index_cast"(%arg5) : (i32) -> index
    %21 = "arith.muli"(%17, %20) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %22 = "arith.index_cast"(%arg4) : (i32) -> index
    %23 = "arith.addi"(%21, %19) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %24 = "memref.reinterpret_cast"(%arg3, %23, %20) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 32, 32>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xf32>, index, index) -> memref<32x32xf32, strided<[?, 1], offset: ?>>
    %25 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<32x32xf32>
    %26 = "arith.divsi"(%23, %20) : (index, index) -> index
    %27 = "arith.subi"(%22, %26) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %28 = "arith.maxsi"(%27, %2) : (index, index) -> index
    %29 = "arith.minsi"(%28, %1) : (index, index) -> index
    %30 = "arith.remsi"(%23, %20) : (index, index) -> index
    %31 = "arith.subi"(%20, %30) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %32 = "arith.maxsi"(%31, %2) : (index, index) -> index
    %33 = "arith.minsi"(%32, %1) : (index, index) -> index
    %34 = "arith.subi"(%3, %14) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %35 = "arith.maxsi"(%34, %3) : (i32, i32) -> i32
    %36 = "arith.index_cast"(%35) : (i32) -> index
    %37 = "arith.minsi"(%36, %29) : (index, index) -> index
    %38 = "arith.subi"(%29, %37) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %39 = "arith.subi"(%3, %15) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %40 = "arith.maxsi"(%39, %3) : (i32, i32) -> i32
    %41 = "arith.index_cast"(%40) : (i32) -> index
    %42 = "arith.minsi"(%41, %33) : (index, index) -> index
    %43 = "arith.subi"(%33, %42) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %44 = "arith.cmpi"(%38, %1) <{predicate = 2 : i64}> : (index, index) -> i1
    %45 = "arith.cmpi"(%43, %1) <{predicate = 2 : i64}> : (index, index) -> i1
    %46 = "arith.ori"(%44, %45) : (i1, i1) -> i1
    %47 = "memref.subview"(%24, %38, %43) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<32x32xf32, strided<[?, 1], offset: ?>>, index, index) -> memref<?x?xf32, strided<[?, 1], offset: ?>>
    %48 = "memref.subview"(%25, %37, %42, %38, %43) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<32x32xf32>, index, index, index, index) -> memref<?x?xf32, strided<[32, 1], offset: ?>>
    "hivm.hir.load"(%47, %48, %0, %42, %46) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x?xf32, strided<[?, 1], offset: ?>>, memref<?x?xf32, strided<[32, 1], offset: ?>>, f32, index, i1) -> ()
    %49 = "bufferization.to_tensor"(%25) <{restrict, writable}> : (memref<32x32xf32>) -> tensor<32x32xf32>
    %50 = "tensor.empty"() : () -> tensor<32x32xf32>
    %51 = "hivm.hir.vabs"(%49, %50) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<32x32xf32>, tensor<32x32xf32>) -> tensor<32x32xf32>
    %52 = "tensor.extract_slice"(%51, %37, %42, %38, %43) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<32x32xf32>, index, index, index, index) -> tensor<?x?xf32>
    "hivm.hir.store"(%52, %47) : (tensor<?x?xf32>, memref<?x?xf32, strided<[?, 1], offset: ?>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, false, false, false, false, false]> : vector<9xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

