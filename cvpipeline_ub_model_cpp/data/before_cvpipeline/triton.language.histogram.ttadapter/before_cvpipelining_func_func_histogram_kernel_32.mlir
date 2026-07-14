"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xi32>, memref<?xi32>, i32, i32, i32) -> (), sym_name = "histogram_kernel"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xi32>, %arg4: memref<?xi32>, %arg5: i32, %arg6: i32, %arg7: i32):
    %0 = "arith.constant"() <{value = 1 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = 2048 : index}> : () -> index
    %2 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %3 = "arith.constant"() <{value = 2 : index}> : () -> index
    %4 = "arith.constant"() <{value = 1 : index}> : () -> index
    %5 = "arith.constant"() <{value = 0 : index}> : () -> index
    "hivm.hir.set_mask_norm"() : () -> ()
    %6 = "arith.muli"(%arg5, %arg6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %7 = "arith.muli"(%6, %arg7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%7) {logical_block_num} : (i32) -> ()
    %8 = "memref.reinterpret_cast"(%arg3) <{operandSegmentSizes = array<i32: 1, 0, 0, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: 2048>, static_strides = array<i64: 1>}> : (memref<?xi32>) -> memref<2048xi32, strided<[1]>>
    %9 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<2048xi32>
    "hivm.hir.load"(%8, %9) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<2048xi32, strided<[1]>>, memref<2048xi32>) -> ()
    %10 = "bufferization.to_tensor"(%9) <{restrict, writable}> : (memref<2048xi32>) -> tensor<2048xi32>
    %11 = "tensor.empty"() : () -> tensor<2xi32>
    %12 = "hivm.hir.vbrc"(%2, %11) <{broadcast_dims = array<i64>}> : (i32, tensor<2xi32>) -> tensor<2xi32>
    %13 = "scf.for"(%5, %1, %4, %12) ({
    ^bb0(%arg8: index, %arg9: tensor<2xi32>):
      %15 = "tensor.extract"(%10, %arg8) : (tensor<2048xi32>, index) -> i32
      %16 = "arith.cmpi"(%15, %2) <{predicate = 6 : i64}> : (i32, i32) -> i1
      %17 = "arith.index_castui"(%15) : (i32) -> index
      %18 = "arith.select"(%16, %5, %17) : (i1, index, index) -> index
      %19 = "arith.cmpi"(%18, %3) <{predicate = 9 : i64}> : (index, index) -> i1
      %20 = "arith.select"(%19, %5, %18) : (i1, index, index) -> index
      %21 = "arith.ori"(%16, %19) : (i1, i1) -> i1
      %22 = "tensor.extract"(%arg9, %20) : (tensor<2xi32>, index) -> i32
      %23 = "arith.addi"(%22, %0) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %24 = "tensor.insert"(%23, %arg9, %20) : (i32, tensor<2xi32>, index) -> tensor<2xi32>
      %25 = "arith.select"(%21, %arg9, %24) : (i1, tensor<2xi32>, tensor<2xi32>) -> tensor<2xi32>
      "scf.yield"(%25) : (tensor<2xi32>) -> ()
    }) : (index, index, index, tensor<2xi32>) -> tensor<2xi32>
    %14 = "memref.reinterpret_cast"(%arg4) <{operandSegmentSizes = array<i32: 1, 0, 0, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: 2>, static_strides = array<i64: 1>}> : (memref<?xi32>) -> memref<2xi32, strided<[1]>>
    "hivm.hir.store"(%13, %14) : (tensor<2xi32>, memref<2xi32, strided<[1]>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, false, false, false]> : vector<8xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

