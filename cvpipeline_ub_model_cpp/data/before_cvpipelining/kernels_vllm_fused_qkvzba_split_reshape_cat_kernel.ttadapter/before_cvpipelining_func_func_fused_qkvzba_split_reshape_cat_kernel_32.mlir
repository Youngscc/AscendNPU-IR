"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, i32, i32, i32) -> (), sym_name = "fused_qkvzba_split_reshape_cat_kernel"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xbf16>, %arg4: memref<?xbf16>, %arg5: memref<?xbf16>, %arg6: memref<?xbf16>, %arg7: memref<?xbf16>, %arg8: memref<?xbf16>, %arg9: i32, %arg10: i32, %arg11: i32):
    %0 = "arith.constant"() <{value = 1 : index}> : () -> index
    %1 = "arith.constant"() <{value = 0 : index}> : () -> index
    %2 = "arith.constant"() <{value = 512 : index}> : () -> index
    %3 = "arith.constant"() <{value = 256 : index}> : () -> index
    %4 = "arith.constant"() <{value = 192 : index}> : () -> index
    %5 = "arith.constant"() <{value = 128 : index}> : () -> index
    %6 = "arith.constant"() <{value = 64 : index}> : () -> index
    %7 = "arith.constant"() <{value = 4 : i32}> : () -> i32
    %8 = "arith.constant"() <{value = 256 : i32}> : () -> i32
    %9 = "arith.constant"() <{value = 64 : i32}> : () -> i32
    %10 = "arith.constant"() <{value = 2 : i32}> : () -> i32
    %11 = "arith.constant"() <{value = 1024 : i32}> : () -> i32
    %12 = "arith.constant"() <{value = 768 : i32}> : () -> i32
    %13 = "arith.constant"() <{value = 8 : i32}> : () -> i32
    "hivm.hir.set_mask_norm"() : () -> ()
    %14 = "arith.muli"(%arg9, %arg10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %15 = "arith.muli"(%14, %arg11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%15) {logical_block_num} : (i32) -> ()
    %16 = "hivm.hir.get_block_idx"() : () -> i64
    %17 = "arith.trunci"(%16) : (i64) -> i32
    %18 = "arith.divsi"(%17, %arg11) : (i32, i32) -> i32
    %19 = "arith.remsi"(%18, %arg10) : (i32, i32) -> i32
    %20 = "arith.muli"(%arg11, %arg10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %21 = "arith.divsi"(%17, %20) : (i32, i32) -> i32
    %22 = "arith.remsi"(%21, %arg9) : (i32, i32) -> i32
    %23 = "arith.muli"(%22, %11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %24 = "arith.index_cast"(%23) : (i32) -> index
    %25 = "arith.muli"(%19, %8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %26 = "arith.index_cast"(%25) : (i32) -> index
    %27 = "arith.addi"(%24, %26) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %28 = "memref.reinterpret_cast"(%arg7, %27) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64>, static_strides = array<i64: 1>}> : (memref<?xbf16>, index) -> memref<64xbf16, strided<[1], offset: ?>>
    %29 = "arith.addi"(%27, %6) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %30 = "memref.reinterpret_cast"(%arg7, %29) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64>, static_strides = array<i64: 1>}> : (memref<?xbf16>, index) -> memref<64xbf16, strided<[1], offset: ?>>
    %31 = "arith.addi"(%27, %5) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %32 = "memref.reinterpret_cast"(%arg7, %31) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64>, static_strides = array<i64: 1>}> : (memref<?xbf16>, index) -> memref<64xbf16, strided<[1], offset: ?>>
    %33 = "arith.addi"(%27, %4) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %34 = "memref.reinterpret_cast"(%arg7, %33) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64>, static_strides = array<i64: 1>}> : (memref<?xbf16>, index) -> memref<64xbf16, strided<[1], offset: ?>>
    %35 = "arith.muli"(%22, %12) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %36 = "arith.index_cast"(%35) : (i32) -> index
    %37 = "arith.muli"(%19, %9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %38 = "arith.index_cast"(%37) : (i32) -> index
    %39 = "arith.addi"(%36, %38) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %40 = "memref.reinterpret_cast"(%arg3, %39) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64>, static_strides = array<i64: 1>}> : (memref<?xbf16>, index) -> memref<64xbf16, strided<[1], offset: ?>>
    %41 = "arith.addi"(%36, %3) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %42 = "arith.addi"(%41, %38) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %43 = "memref.reinterpret_cast"(%arg3, %42) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64>, static_strides = array<i64: 1>}> : (memref<?xbf16>, index) -> memref<64xbf16, strided<[1], offset: ?>>
    %44 = "arith.addi"(%36, %2) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %45 = "arith.divsi"(%25, %7) : (i32, i32) -> i32
    %46 = "arith.index_cast"(%45) : (i32) -> index
    %47 = "arith.addi"(%44, %46) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %48 = "memref.reinterpret_cast"(%arg3, %47) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64>, static_strides = array<i64: 1>}> : (memref<?xbf16>, index) -> memref<64xbf16, strided<[1], offset: ?>>
    %49 = "arith.muli"(%22, %8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %50 = "arith.index_cast"(%49) : (i32) -> index
    %51 = "arith.addi"(%50, %46) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %52 = "memref.reinterpret_cast"(%arg4, %51) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64>, static_strides = array<i64: 1>}> : (memref<?xbf16>, index) -> memref<64xbf16, strided<[1], offset: ?>>
    %53 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<64xbf16>
    "hivm.hir.load"(%28, %53) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<64xbf16, strided<[1], offset: ?>>, memref<64xbf16>) -> ()
    %54 = "bufferization.to_tensor"(%53) <{restrict, writable}> : (memref<64xbf16>) -> tensor<64xbf16>
    "hivm.hir.store"(%54, %40) : (tensor<64xbf16>, memref<64xbf16, strided<[1], offset: ?>>) -> ()
    %55 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<64xbf16>
    "hivm.hir.load"(%30, %55) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<64xbf16, strided<[1], offset: ?>>, memref<64xbf16>) -> ()
    %56 = "bufferization.to_tensor"(%55) <{restrict, writable}> : (memref<64xbf16>) -> tensor<64xbf16>
    "hivm.hir.store"(%56, %43) : (tensor<64xbf16>, memref<64xbf16, strided<[1], offset: ?>>) -> ()
    %57 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<64xbf16>
    "hivm.hir.load"(%32, %57) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<64xbf16, strided<[1], offset: ?>>, memref<64xbf16>) -> ()
    %58 = "bufferization.to_tensor"(%57) <{restrict, writable}> : (memref<64xbf16>) -> tensor<64xbf16>
    "hivm.hir.store"(%58, %48) : (tensor<64xbf16>, memref<64xbf16, strided<[1], offset: ?>>) -> ()
    %59 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<64xbf16>
    "hivm.hir.load"(%34, %59) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<64xbf16, strided<[1], offset: ?>>, memref<64xbf16>) -> ()
    %60 = "bufferization.to_tensor"(%59) <{restrict, writable}> : (memref<64xbf16>) -> tensor<64xbf16>
    "hivm.hir.store"(%60, %52) : (tensor<64xbf16>, memref<64xbf16, strided<[1], offset: ?>>) -> ()
    %61 = "arith.muli"(%22, %13) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %62 = "arith.index_cast"(%61) : (i32) -> index
    %63 = "arith.muli"(%19, %10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %64 = "arith.index_cast"(%63) : (i32) -> index
    %65 = "arith.addi"(%62, %64) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %66 = "memref.reinterpret_cast"(%arg8, %65) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xbf16>, index) -> memref<1xbf16, strided<[1], offset: ?>>
    %67 = "arith.muli"(%22, %7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %68 = "arith.index_cast"(%67) : (i32) -> index
    %69 = "arith.muli"(%19, %7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %70 = "arith.divsi"(%69, %7) : (i32, i32) -> i32
    %71 = "arith.index_cast"(%70) : (i32) -> index
    %72 = "arith.addi"(%68, %71) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %73 = "memref.load"(%66, %1) : (memref<1xbf16, strided<[1], offset: ?>>, index) -> bf16
    %74 = "tensor.empty"() : () -> tensor<1xbf16>
    %75 = "tensor.empty"() : () -> tensor<1xbf16>
    %76 = "tensor.insert"(%73, %75, %1) : (bf16, tensor<1xbf16>, index) -> tensor<1xbf16>
    %77 = "memref.reinterpret_cast"(%arg5, %72) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xbf16>, index) -> memref<1xbf16, strided<[1], offset: ?>>
    "hivm.hir.store"(%76, %77) : (tensor<1xbf16>, memref<1xbf16, strided<[1], offset: ?>>) -> ()
    %78 = "arith.addi"(%65, %0) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %79 = "memref.reinterpret_cast"(%arg8, %78) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xbf16>, index) -> memref<1xbf16, strided<[1], offset: ?>>
    %80 = "memref.load"(%79, %1) : (memref<1xbf16, strided<[1], offset: ?>>, index) -> bf16
    %81 = "tensor.insert"(%80, %74, %1) : (bf16, tensor<1xbf16>, index) -> tensor<1xbf16>
    %82 = "memref.reinterpret_cast"(%arg6, %72) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xbf16>, index) -> memref<1xbf16, strided<[1], offset: ?>>
    "hivm.hir.store"(%81, %82) : (tensor<1xbf16>, memref<1xbf16, strided<[1], offset: ?>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, true, true, true, false, false, false]> : vector<12xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

