"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xf32>, memref<?xf32>, i32, i32, i32) -> (), sym_name = "exp2_kernel"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xf32>, %arg4: memref<?xf32>, %arg5: i32, %arg6: i32, %arg7: i32):
    %0 = "arith.constant"() <{value = 0.693147182 : f32}> : () -> f32
    %1 = "arith.constant"() <{value = 32 : index}> : () -> index
    %2 = "arith.constant"() <{value = 8 : index}> : () -> index
    %3 = "arith.constant"() <{value = 2 : i32}> : () -> i32
    %4 = "arith.constant"() <{value = 4 : i32}> : () -> i32
    %5 = "arith.constant"() <{value = 8 : i32}> : () -> i32
    "hivm.hir.set_mask_norm"() : () -> ()
    %6 = "arith.muli"(%arg5, %arg6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %7 = "arith.muli"(%6, %arg7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%7) {logical_block_num} : (i32) -> ()
    %8 = "hivm.hir.get_block_idx"() : () -> i64
    %9 = "arith.trunci"(%8) : (i64) -> i32
    %10 = "arith.remsi"(%9, %arg7) : (i32, i32) -> i32
    %11 = "arith.divsi"(%9, %arg7) : (i32, i32) -> i32
    %12 = "arith.remsi"(%11, %arg6) : (i32, i32) -> i32
    %13 = "arith.muli"(%arg7, %arg6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %14 = "arith.divsi"(%9, %13) : (i32, i32) -> i32
    %15 = "arith.remsi"(%14, %arg5) : (i32, i32) -> i32
    %16 = "arith.muli"(%15, %3) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %17 = "arith.muli"(%12, %4) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %18 = "arith.muli"(%10, %5) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %19 = "arith.index_cast"(%16) : (i32) -> index
    %20 = "arith.muli"(%19, %1) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %21 = "arith.index_cast"(%17) : (i32) -> index
    %22 = "arith.muli"(%21, %2) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %23 = "arith.index_cast"(%18) : (i32) -> index
    %24 = "arith.addi"(%20, %22) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %25 = "arith.addi"(%24, %23) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %26 = "memref.reinterpret_cast"(%arg4, %25) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 2, 4, 8>, static_strides = array<i64: 32, 8, 1>}> : (memref<?xf32>, index) -> memref<2x4x8xf32, strided<[32, 8, 1], offset: ?>>
    %27 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<2x4x8xf32>
    "hivm.hir.load"(%26, %27) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<2x4x8xf32, strided<[32, 8, 1], offset: ?>>, memref<2x4x8xf32>) -> ()
    %28 = "bufferization.to_tensor"(%27) <{restrict, writable}> : (memref<2x4x8xf32>) -> tensor<2x4x8xf32>
    %29 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %30 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %31 = "hivm.hir.vmul"(%28, %0, %30) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, f32, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %32 = "hivm.hir.vexp"(%31, %29) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %33 = "memref.reinterpret_cast"(%arg3, %25) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 2, 4, 8>, static_strides = array<i64: 32, 8, 1>}> : (memref<?xf32>, index) -> memref<2x4x8xf32, strided<[32, 8, 1], offset: ?>>
    "hivm.hir.store"(%32, %33) : (tensor<2x4x8xf32>, memref<2x4x8xf32, strided<[32, 8, 1], offset: ?>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, false, false, false]> : vector<8xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

