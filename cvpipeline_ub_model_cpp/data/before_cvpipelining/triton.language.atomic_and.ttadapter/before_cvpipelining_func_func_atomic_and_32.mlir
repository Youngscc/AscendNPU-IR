"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 2 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xi32>, memref<?xi32>, memref<?xi32>, i32, i32, i32, i32) -> (), sym_name = "atomic_and"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xi32>, %arg4: memref<?xi32>, %arg5: memref<?xi32>, %arg6: i32, %arg7: i32, %arg8: i32, %arg9: i32):
    %0 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = 512 : index}> : () -> index
    %2 = "arith.constant"() <{value = 512 : i32}> : () -> i32
    %3 = "arith.constant"() <{value = 0 : index}> : () -> index
    "hivm.hir.set_mask_norm"() : () -> ()
    %4 = "arith.muli"(%arg7, %arg8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %5 = "arith.muli"(%4, %arg9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%5) {logical_block_num} : (i32) -> ()
    %6 = "hivm.hir.get_block_idx"() : () -> i64
    %7 = "arith.trunci"(%6) : (i64) -> i32
    %8 = "arith.muli"(%arg9, %arg8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %9 = "arith.divsi"(%7, %8) : (i32, i32) -> i32
    %10 = "arith.remsi"(%9, %arg7) : (i32, i32) -> i32
    %11 = "arith.muli"(%10, %2) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %12 = "arith.index_cast"(%11) : (i32) -> index
    %13 = "memref.reinterpret_cast"(%arg3, %12) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 512>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<512xi32, strided<[1], offset: ?>>
    %14 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<512xi32>
    %15 = "arith.addi"(%12, %1) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %16 = "arith.index_cast"(%arg6) : (i32) -> index
    %17 = "arith.maxsi"(%12, %16) : (index, index) -> index
    %18 = "arith.minsi"(%15, %17) : (index, index) -> index
    %19 = "arith.subi"(%18, %12) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %20 = "arith.cmpi"(%19, %1) <{predicate = 2 : i64}> : (index, index) -> i1
    %21 = "memref.subview"(%13, %19) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<512xi32, strided<[1], offset: ?>>, index) -> memref<?xi32, strided<[1], offset: ?>>
    %22 = "memref.subview"(%14, %19) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<512xi32>, index) -> memref<?xi32, strided<[1]>>
    "hivm.hir.load"(%21, %22, %0, %3, %20) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xi32, strided<[1], offset: ?>>, memref<?xi32, strided<[1]>>, i32, index, i1) -> ()
    %23 = "memref.reinterpret_cast"(%arg4) <{operandSegmentSizes = array<i32: 1, 0, 0, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: 512>, static_strides = array<i64: 1>}> : (memref<?xi32>) -> memref<512xi32, strided<[1]>>
    %24 = "memref.subview"(%23, %19) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<512xi32, strided<[1]>>, index) -> memref<?xi32, strided<[1]>>
    %25 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<512xi32>
    "hivm.hir.load"(%23, %25) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<512xi32, strided<[1]>>, memref<512xi32>) -> ()
    %26 = "bufferization.to_tensor"(%25) <{restrict, writable}> : (memref<512xi32>) -> tensor<512xi32>
    "hivm.hir.atomic_rmw"(%22, %24) <{atomic_kind = #hivm.atomic_kind<and>}> : (memref<?xi32, strided<[1]>>, memref<?xi32, strided<[1]>>) -> ()
    %27 = "memref.reinterpret_cast"(%arg5) <{operandSegmentSizes = array<i32: 1, 0, 0, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: 512>, static_strides = array<i64: 1>}> : (memref<?xi32>) -> memref<512xi32, strided<[1]>>
    %28 = "tensor.extract_slice"(%26, %19) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<512xi32>, index) -> tensor<?xi32>
    %29 = "memref.subview"(%27, %19) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<512xi32, strided<[1]>>, index) -> memref<?xi32, strided<[1]>>
    "hivm.hir.store"(%28, %29) : (tensor<?xi32>, memref<?xi32, strided<[1]>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, false, false, false, false]> : vector<10xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

