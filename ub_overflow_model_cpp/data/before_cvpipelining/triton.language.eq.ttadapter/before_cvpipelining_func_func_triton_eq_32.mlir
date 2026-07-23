"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xi32>, memref<?xi32>, memref<?xi8>, i32, i32, i32) -> (), sym_name = "triton_eq"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xi32>, %arg4: memref<?xi32>, %arg5: memref<?xi8>, %arg6: i32, %arg7: i32, %arg8: i32):
    %0 = "arith.constant"() <{value = 1 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = 128 : index}> : () -> index
    %2 = "arith.constant"() <{value = 32 : index}> : () -> index
    %3 = "arith.constant"() <{value = 128 : i32}> : () -> i32
    %4 = "arith.constant"() <{value = 32 : i32}> : () -> i32
    %5 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %6 = "arith.constant"() <{value = 4 : i32}> : () -> i32
    %7 = "arith.constant"() <{value = 0 : index}> : () -> index
    "hivm.hir.set_mask_norm"() : () -> ()
    %8 = "arith.muli"(%arg6, %arg7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %9 = "arith.muli"(%8, %arg8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%9) {logical_block_num} : (i32) -> ()
    %10 = "hivm.hir.get_block_idx"() : () -> i64
    %11 = "arith.trunci"(%10) : (i64) -> i32
    %12 = "arith.muli"(%arg8, %arg7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %13 = "arith.divsi"(%11, %12) : (i32, i32) -> i32
    %14 = "arith.remsi"(%13, %arg6) : (i32, i32) -> i32
    %15 = "arith.muli"(%14, %3) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "scf.for"(%5, %6, %0) ({
    ^bb0(%arg9: i32):
      %16 = "arith.muli"(%arg9, %4) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %17 = "arith.addi"(%15, %16) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %18 = "arith.index_cast"(%17) : (i32) -> index
      %19 = "memref.reinterpret_cast"(%arg3, %18) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 32>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<32xi32, strided<[1], offset: ?>>
      %20 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<32xi32>
      %21 = "arith.addi"(%18, %2) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %22 = "arith.maxsi"(%18, %1) : (index, index) -> index
      %23 = "arith.minsi"(%21, %22) : (index, index) -> index
      %24 = "arith.subi"(%23, %18) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %25 = "arith.cmpi"(%24, %2) <{predicate = 2 : i64}> : (index, index) -> i1
      %26 = "memref.subview"(%19, %24) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<32xi32, strided<[1], offset: ?>>, index) -> memref<?xi32, strided<[1], offset: ?>>
      %27 = "memref.subview"(%20, %24) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<32xi32>, index) -> memref<?xi32, strided<[1]>>
      "hivm.hir.load"(%26, %27, %5, %7, %25) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xi32, strided<[1], offset: ?>>, memref<?xi32, strided<[1]>>, i32, index, i1) -> ()
      %28 = "bufferization.to_tensor"(%20) <{restrict, writable}> : (memref<32xi32>) -> tensor<32xi32>
      %29 = "memref.reinterpret_cast"(%arg4, %18) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 32>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<32xi32, strided<[1], offset: ?>>
      %30 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<32xi32>
      %31 = "memref.subview"(%29, %24) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<32xi32, strided<[1], offset: ?>>, index) -> memref<?xi32, strided<[1], offset: ?>>
      %32 = "memref.subview"(%30, %24) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<32xi32>, index) -> memref<?xi32, strided<[1]>>
      "hivm.hir.load"(%31, %32, %5, %7, %25) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xi32, strided<[1], offset: ?>>, memref<?xi32, strided<[1]>>, i32, index, i1) -> ()
      %33 = "bufferization.to_tensor"(%30) <{restrict, writable}> : (memref<32xi32>) -> tensor<32xi32>
      %34 = "tensor.empty"() : () -> tensor<32xi1>
      %35 = "hivm.hir.vcmp"(%28, %33, %34) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<32xi32>, tensor<32xi32>, tensor<32xi1>) -> tensor<32xi1>
      %36 = "memref.reinterpret_cast"(%arg5, %18) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 32>, static_strides = array<i64: 1>}> : (memref<?xi8>, index) -> memref<32xi8, strided<[1], offset: ?>>
      %37 = "tensor.empty"() : () -> tensor<32xi8>
      %38 = "hivm.hir.vcast"(%35, %37) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<32xi1>, tensor<32xi8>) -> tensor<32xi8>
      %39 = "tensor.extract_slice"(%38, %24) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<32xi8>, index) -> tensor<?xi8>
      %40 = "memref.subview"(%36, %24) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<32xi8, strided<[1], offset: ?>>, index) -> memref<?xi8, strided<[1], offset: ?>>
      "hivm.hir.store"(%39, %40) : (tensor<?xi8>, memref<?xi8, strided<[1], offset: ?>>) -> ()
      "scf.yield"() : () -> ()
    }) : (i32, i32, i32) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, false, false, false]> : vector<9xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

