"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xf32>, memref<?xi32>, memref<?xf32>, i32, i32, i32) -> (), sym_name = "kernel"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xf32>, %arg4: memref<?xi32>, %arg5: memref<?xf32>, %arg6: i32, %arg7: i32, %arg8: i32):
    %0 = "arith.constant"() <{value = 2 : index}> : () -> index
    %1 = "arith.constant"() <{value = 1 : index}> : () -> index
    %2 = "arith.constant"() <{value = 0 : index}> : () -> index
    "hivm.hir.set_mask_norm"() : () -> ()
    %3 = "arith.muli"(%arg6, %arg7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %4 = "arith.muli"(%3, %arg8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%4) {logical_block_num} : (i32) -> ()
    %5 = "memref.reinterpret_cast"(%arg3) <{operandSegmentSizes = array<i32: 1, 0, 0, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: 4, 2>, static_strides = array<i64: 2, 1>}> : (memref<?xf32>) -> memref<4x2xf32, strided<[2, 1]>>
    %6 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<4x2xf32>
    "hivm.hir.load"(%5, %6) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<4x2xf32, strided<[2, 1]>>, memref<4x2xf32>) -> ()
    %7 = "bufferization.to_tensor"(%6) <{restrict, writable}> : (memref<4x2xf32>) -> tensor<4x2xf32>
    %8 = "memref.reinterpret_cast"(%arg4) <{operandSegmentSizes = array<i32: 1, 0, 0, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: 2, 2>, static_strides = array<i64: 2, 1>}> : (memref<?xi32>) -> memref<2x2xi32, strided<[2, 1]>>
    %9 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<2x2xi32>
    "hivm.hir.load"(%8, %9) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<2x2xi32, strided<[2, 1]>>, memref<2x2xi32>) -> ()
    %10 = "bufferization.to_tensor"(%9) <{restrict, writable}> : (memref<2x2xi32>) -> tensor<2x2xi32>
    %11 = "tensor.empty"() : () -> tensor<2x2xf32>
    %12 = "scf.for"(%2, %0, %1, %11) ({
    ^bb0(%arg9: index, %arg10: tensor<2x2xf32>):
      %14 = "scf.for"(%2, %0, %1, %arg10) ({
      ^bb0(%arg11: index, %arg12: tensor<2x2xf32>):
        %15 = "tensor.extract"(%10, %arg9, %arg11) : (tensor<2x2xi32>, index, index) -> i32
        %16 = "arith.index_cast"(%15) : (i32) -> index
        %17 = "tensor.extract"(%7, %16, %arg11) : (tensor<4x2xf32>, index, index) -> f32
        %18 = "tensor.insert"(%17, %arg12, %arg9, %arg11) : (f32, tensor<2x2xf32>, index, index) -> tensor<2x2xf32>
        "scf.yield"(%18) : (tensor<2x2xf32>) -> ()
      }) : (index, index, index, tensor<2x2xf32>) -> tensor<2x2xf32>
      "scf.yield"(%14) : (tensor<2x2xf32>) -> ()
    }) : (index, index, index, tensor<2x2xf32>) -> tensor<2x2xf32>
    %13 = "memref.reinterpret_cast"(%arg5) <{operandSegmentSizes = array<i32: 1, 0, 0, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: 2, 2>, static_strides = array<i64: 2, 1>}> : (memref<?xf32>) -> memref<2x2xf32, strided<[2, 1]>>
    "hivm.hir.store"(%12, %13) : (tensor<2x2xf32>, memref<2x2xf32, strided<[2, 1]>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, false, false, false]> : vector<9xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

