"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xi32>, memref<?xi32>, memref<?xi32>, memref<?xi32>, i32, i32, i32, i32) -> (), sym_name = "rejection_greedy_sample_spec_len_1_triton"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xi32>, %arg4: memref<?xi32>, %arg5: memref<?xi32>, %arg6: memref<?xi32>, %arg7: i32, %arg8: i32, %arg9: i32, %arg10: i32):
    %0 = "arith.constant"() <{value = 1 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = 1 : index}> : () -> index
    %2 = "arith.constant"() <{value = 0 : index}> : () -> index
    %3 = "arith.constant"() <{value = 2 : index}> : () -> index
    %4 = "arith.constant"() <{value = 16 : index}> : () -> index
    %5 = "arith.constant"() <{value = 16 : i32}> : () -> i32
    %6 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %7 = "arith.constant"() <{value = 2 : i32}> : () -> i32
    "hivm.hir.set_mask_norm"() : () -> ()
    %8 = "arith.muli"(%arg8, %arg9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %9 = "arith.muli"(%8, %arg10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%9) {logical_block_num} : (i32) -> ()
    %10 = "hivm.hir.get_block_idx"() : () -> i64
    %11 = "arith.trunci"(%10) : (i64) -> i32
    %12 = "arith.muli"(%arg10, %arg9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %13 = "arith.divsi"(%11, %12) : (i32, i32) -> i32
    %14 = "arith.remsi"(%13, %arg8) : (i32, i32) -> i32
    %15 = "arith.muli"(%14, %5) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %16 = "arith.index_cast"(%15) : (i32) -> index
    %17 = "memref.reinterpret_cast"(%arg4, %16) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<16xi32, strided<[1], offset: ?>>
    %18 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16xi32>
    %19 = "arith.addi"(%16, %4) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %20 = "arith.index_cast"(%arg7) : (i32) -> index
    %21 = "arith.maxsi"(%16, %20) : (index, index) -> index
    %22 = "arith.minsi"(%19, %21) : (index, index) -> index
    %23 = "arith.subi"(%22, %16) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %24 = "arith.cmpi"(%23, %4) <{predicate = 2 : i64}> : (index, index) -> i1
    %25 = "memref.subview"(%17, %23) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<16xi32, strided<[1], offset: ?>>, index) -> memref<?xi32, strided<[1], offset: ?>>
    %26 = "memref.subview"(%18, %23) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<16xi32>, index) -> memref<?xi32, strided<[1]>>
    "hivm.hir.load"(%25, %26, %6, %2, %24) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xi32, strided<[1], offset: ?>>, memref<?xi32, strided<[1]>>, i32, index, i1) -> ()
    %27 = "bufferization.to_tensor"(%18) <{restrict, writable}> : (memref<16xi32>) -> tensor<16xi32>
    %28 = "memref.reinterpret_cast"(%arg5, %16) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<16xi32, strided<[1], offset: ?>>
    %29 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16xi32>
    %30 = "memref.subview"(%28, %23) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<16xi32, strided<[1], offset: ?>>, index) -> memref<?xi32, strided<[1], offset: ?>>
    %31 = "memref.subview"(%29, %23) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<16xi32>, index) -> memref<?xi32, strided<[1]>>
    "hivm.hir.load"(%30, %31, %6, %2, %24) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xi32, strided<[1], offset: ?>>, memref<?xi32, strided<[1]>>, i32, index, i1) -> ()
    %32 = "bufferization.to_tensor"(%29) <{restrict, writable}> : (memref<16xi32>) -> tensor<16xi32>
    %33 = "arith.muli"(%16, %3) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %34 = "memref.reinterpret_cast"(%arg3, %33) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16>, static_strides = array<i64: 2>}> : (memref<?xi32>, index) -> memref<16xi32, strided<[2], offset: ?>>
    %35 = "tensor.extract_slice"(%32, %23) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<16xi32>, index) -> tensor<?xi32>
    %36 = "memref.subview"(%34, %23) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<16xi32, strided<[2], offset: ?>>, index) -> memref<?xi32, strided<[2], offset: ?>>
    "hivm.hir.store"(%35, %36) : (tensor<?xi32>, memref<?xi32, strided<[2], offset: ?>>) -> ()
    "scf.for"(%6, %5, %0) ({
    ^bb0(%arg11: i32):
      %37 = "arith.index_cast"(%arg11) : (i32) -> index
      %38 = "tensor.extract"(%27, %37) : (tensor<16xi32>, index) -> i32
      %39 = "tensor.extract"(%32, %37) : (tensor<16xi32>, index) -> i32
      %40 = "arith.addi"(%15, %arg11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %41 = "arith.cmpi"(%38, %39) <{predicate = 0 : i64}> : (i32, i32) -> i1
      "scf.if"(%41) ({
        %42 = "arith.index_cast"(%40) : (i32) -> index
        %43 = "memref.reinterpret_cast"(%arg6, %42) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<1xi32, strided<[1], offset: ?>>
        %44 = "memref.load"(%43, %2) : (memref<1xi32, strided<[1], offset: ?>>, index) -> i32
        %45 = "arith.muli"(%40, %7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
        %46 = "arith.index_cast"(%45) : (i32) -> index
        %47 = "arith.addi"(%46, %1) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
        %48 = "tensor.empty"() : () -> tensor<1xi32>
        %49 = "tensor.insert"(%44, %48, %2) : (i32, tensor<1xi32>, index) -> tensor<1xi32>
        %50 = "memref.reinterpret_cast"(%arg3, %47) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<1xi32, strided<[1], offset: ?>>
        "hivm.hir.store"(%49, %50) : (tensor<1xi32>, memref<1xi32, strided<[1], offset: ?>>) -> ()
        "scf.yield"() : () -> ()
      }, {
      }) : (i1) -> ()
      "scf.yield"() : () -> ()
    }) : (i32, i32, i32) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, true, false, false, false, false]> : vector<11xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

