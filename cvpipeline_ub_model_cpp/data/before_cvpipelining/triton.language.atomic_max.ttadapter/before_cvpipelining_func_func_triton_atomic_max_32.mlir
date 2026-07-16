"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 2 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xi32>, memref<?xi32>, i32, i32, i32) -> (), sym_name = "triton_atomic_max"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xi32>, %arg4: memref<?xi32>, %arg5: i32, %arg6: i32, %arg7: i32):
    "hivm.hir.set_mask_norm"() : () -> ()
    %0 = "arith.muli"(%arg5, %arg6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %1 = "arith.muli"(%0, %arg7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%1) {logical_block_num} : (i32) -> ()
    %2 = "hivm.hir.get_block_idx"() : () -> i64
    %3 = "arith.trunci"(%2) : (i64) -> i32
    %4 = "arith.muli"(%arg7, %arg6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %5 = "arith.divsi"(%3, %4) : (i32, i32) -> i32
    %6 = "arith.remsi"(%5, %arg5) : (i32, i32) -> i32
    %7 = "arith.index_cast"(%6) : (i32) -> index
    %8 = "memref.reinterpret_cast"(%arg3, %7) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<1xi32, strided<[1], offset: ?>>
    %9 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<1xi32>
    "hivm.hir.load"(%8, %9) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<1xi32, strided<[1], offset: ?>>, memref<1xi32>) -> ()
    %10 = "bufferization.to_tensor"(%9) <{restrict, writable}> : (memref<1xi32>) -> tensor<1xi32>
    %11 = "memref.reinterpret_cast"(%arg4, %7) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<1xi32, strided<[1], offset: ?>>
    "hivm.hir.store"(%10, %11) <{atomic_kind = #hivm.atomic_kind<max>}> : (tensor<1xi32>, memref<1xi32, strided<[1], offset: ?>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, false, false, false]> : vector<8xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

