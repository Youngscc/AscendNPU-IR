"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, i32, i32, i32) -> (), sym_name = "test_sync_block_all"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: i32, %arg4: i32, %arg5: i32):
    "hivm.hir.set_mask_norm"() : () -> ()
    %0 = "arith.muli"(%arg3, %arg4) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %1 = "arith.muli"(%0, %arg5) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%1) {logical_block_num} : (i32) -> ()
    "hivm.hir.sync_block"() <{flag_id = 8 : index, sync_block_mode = #hivm.sync_block_mode<ALL_CUBE>, tcube_pipe = #hivm.pipe<PIPE_ALL>}> : () -> ()
    "hivm.hir.sync_block"() <{flag_id = 9 : index, sync_block_mode = #hivm.sync_block_mode<ALL_VECTOR>, tvector_pipe = #hivm.pipe<PIPE_ALL>}> : () -> ()
    "hivm.hir.sync_block"() <{flag_id = 10 : index, sync_block_mode = #hivm.sync_block_mode<ALL>, tcube_pipe = #hivm.pipe<PIPE_ALL>, tvector_pipe = #hivm.pipe<PIPE_ALL>}> : () -> ()
    "hivm.hir.sync_block"() <{flag_id = 11 : index, sync_block_mode = #hivm.sync_block_mode<ALL_SUB_VECTOR>, tvector_pipe = #hivm.pipe<PIPE_ALL>}> : () -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, false, false, false]> : vector<6xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

