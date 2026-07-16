"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xf32>, memref<?xi32>, memref<?xf32>, i32, i32, i32, i32) -> (), sym_name = "get_element_test_kernel"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xf32>, %arg4: memref<?xi32>, %arg5: memref<?xf32>, %arg6: i32, %arg7: i32, %arg8: i32, %arg9: i32):
    %0 = "arith.constant"() <{value = 1 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = 16 : index}> : () -> index
    %2 = "arith.constant"() <{value = 1024 : index}> : () -> index
    %3 = "arith.constant"() <{value = 326 : index}> : () -> index
    %4 = "arith.constant"() <{value = 326 : i32}> : () -> i32
    %5 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %6 = "arith.constant"() <{value = 26 : i32}> : () -> i32
    %7 = "arith.constant"() <{value = 16 : i32}> : () -> i32
    %8 = "arith.constant"() <{value = 0 : index}> : () -> index
    "hivm.hir.set_mask_norm"() : () -> ()
    %9 = "arith.muli"(%arg7, %arg8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %10 = "arith.muli"(%9, %arg9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%10) {logical_block_num} : (i32) -> ()
    %11 = "hivm.hir.get_block_idx"() : () -> i64
    %12 = "arith.trunci"(%11) : (i64) -> i32
    %13 = "arith.muli"(%arg9, %arg8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %14 = "arith.divsi"(%12, %13) : (i32, i32) -> i32
    %15 = "arith.remsi"(%14, %arg7) : (i32, i32) -> i32
    %16 = "arith.muli"(%15, %6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %17 = "arith.index_cast"(%16) : (i32) -> index
    %18 = "memref.reinterpret_cast"(%arg4, %17) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 326>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<326xi32, strided<[1], offset: ?>>
    %19 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<326xi32>
    %20 = "arith.addi"(%17, %3) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %21 = "arith.maxsi"(%17, %2) : (index, index) -> index
    %22 = "arith.minsi"(%20, %21) : (index, index) -> index
    %23 = "arith.subi"(%22, %17) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %24 = "arith.cmpi"(%23, %3) <{predicate = 2 : i64}> : (index, index) -> i1
    %25 = "memref.subview"(%18, %23) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<326xi32, strided<[1], offset: ?>>, index) -> memref<?xi32, strided<[1], offset: ?>>
    %26 = "memref.subview"(%19, %23) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<326xi32>, index) -> memref<?xi32, strided<[1]>>
    "hivm.hir.load"(%25, %26, %5, %8, %24) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xi32, strided<[1], offset: ?>>, memref<?xi32, strided<[1]>>, i32, index, i1) -> ()
    %27 = "bufferization.to_tensor"(%19) <{restrict, writable}> : (memref<326xi32>) -> tensor<326xi32>
    %28 = "tensor.empty"() : () -> tensor<326x16xf32>
    %29 = "scf.for"(%5, %4, %0, %28) ({
    ^bb0(%arg10: i32, %arg11: tensor<326x16xf32>):
      %35 = "arith.index_cast"(%arg10) : (i32) -> index
      %36 = "tensor.extract"(%27, %35) : (tensor<326xi32>, index) -> i32
      %37 = "arith.muli"(%36, %7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %38 = "arith.index_cast"(%37) : (i32) -> index
      %39 = "memref.reinterpret_cast"(%arg3, %38) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1, 16>, static_strides = array<i64: 16, 1>}> : (memref<?xf32>, index) -> memref<1x16xf32, strided<[16, 1], offset: ?>>
      %40 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<1x16xf32>
      "hivm.hir.load"(%39, %40) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<1x16xf32, strided<[16, 1], offset: ?>>, memref<1x16xf32>) -> ()
      %41 = "bufferization.to_tensor"(%40) <{restrict, writable}> : (memref<1x16xf32>) -> tensor<1x16xf32>
      %42 = "tensor.insert_slice"(%41, %arg11, %35) <{operandSegmentSizes = array<i32: 1, 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808, 0>, static_sizes = array<i64: 1, 16>, static_strides = array<i64: 1, 1>}> : (tensor<1x16xf32>, tensor<326x16xf32>, index) -> tensor<326x16xf32>
      "scf.yield"(%42) : (tensor<326x16xf32>) -> ()
    }) : (i32, i32, i32, tensor<326x16xf32>) -> tensor<326x16xf32>
    %30 = "arith.muli"(%17, %1) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %31 = "memref.reinterpret_cast"(%arg5, %30) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 326, 16>, static_strides = array<i64: 16, 1>}> : (memref<?xf32>, index) -> memref<326x16xf32, strided<[16, 1], offset: ?>>
    %32 = "arith.minsi"(%23, %3) : (index, index) -> index
    %33 = "tensor.extract_slice"(%29, %32) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (tensor<326x16xf32>, index) -> tensor<?x16xf32>
    %34 = "memref.subview"(%31, %32) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (memref<326x16xf32, strided<[16, 1], offset: ?>>, index) -> memref<?x16xf32, strided<[16, 1], offset: ?>>
    "hivm.hir.store"(%33, %34) : (tensor<?x16xf32>, memref<?x16xf32, strided<[16, 1], offset: ?>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, false, false, false, false]> : vector<10xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

