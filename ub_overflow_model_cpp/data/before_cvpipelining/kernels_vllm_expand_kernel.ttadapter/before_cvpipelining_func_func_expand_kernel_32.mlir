"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {}, {}, {}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xi32>, memref<?xi32>, memref<?xi32>, i32, i32, i32, i32, i32, i32) -> (), sym_name = "expand_kernel"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xi32>, %arg4: memref<?xi32>, %arg5: memref<?xi32>, %arg6: i32, %arg7: i32, %arg8: i32, %arg9: i32, %arg10: i32, %arg11: i32):
    %0 = "arith.constant"() <{value = 1 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = 16 : index}> : () -> index
    %2 = "arith.constant"() <{value = -1 : index}> : () -> index
    %3 = "arith.constant"() <{value = 1 : index}> : () -> index
    %4 = "arith.constant"() <{value = 4 : index}> : () -> index
    %5 = "arith.constant"() <{value = 0 : index}> : () -> index
    %6 = "arith.constant"() <{value = 4 : i32}> : () -> i32
    %7 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    "hivm.hir.set_mask_norm"() : () -> ()
    %8 = "arith.muli"(%arg9, %arg10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %9 = "arith.muli"(%8, %arg11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%9) {logical_block_num} : (i32) -> ()
    %10 = "hivm.hir.get_block_idx"() : () -> i64
    %11 = "arith.trunci"(%10) : (i64) -> i32
    %12 = "arith.muli"(%arg11, %arg10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %13 = "arith.divsi"(%11, %12) : (i32, i32) -> i32
    %14 = "arith.remsi"(%13, %arg9) : (i32, i32) -> i32
    %15 = "tensor.empty"() : () -> tensor<4xi32>
    %16 = "tensor.empty"() : () -> tensor<4xi32>
    %17 = "tensor.empty"() : () -> tensor<4xi32>
    %18 = "hivm.hir.vbrc"(%7, %17) <{broadcast_dims = array<i64>}> : (i32, tensor<4xi32>) -> tensor<4xi32>
    %19 = "arith.muli"(%14, %6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %20 = "arith.index_cast"(%19) : (i32) -> index
    %21 = "memref.reinterpret_cast"(%arg5, %20) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 4>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<4xi32, strided<[1], offset: ?>>
    %22 = "arith.addi"(%20, %2) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %23 = "memref.reinterpret_cast"(%arg5, %22) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 4>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<4xi32, strided<[1], offset: ?>>
    %24 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<4xi32>
    %25 = "arith.addi"(%20, %4) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %26 = "arith.index_cast"(%arg8) : (i32) -> index
    %27 = "arith.maxsi"(%20, %26) : (index, index) -> index
    %28 = "arith.minsi"(%25, %27) : (index, index) -> index
    %29 = "arith.subi"(%28, %20) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %30 = "arith.cmpi"(%29, %4) <{predicate = 2 : i64}> : (index, index) -> i1
    %31 = "memref.subview"(%23, %29) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<4xi32, strided<[1], offset: ?>>, index) -> memref<?xi32, strided<[1], offset: ?>>
    %32 = "memref.subview"(%24, %29) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<4xi32>, index) -> memref<?xi32, strided<[1]>>
    "hivm.hir.load"(%31, %32, %7, %5, %30) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xi32, strided<[1], offset: ?>>, memref<?xi32, strided<[1]>>, i32, index, i1) -> ()
    %33 = "bufferization.to_tensor"(%24) <{restrict, writable}> : (memref<4xi32>) -> tensor<4xi32>
    %34 = "arith.subi"(%5, %20) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %35 = "arith.subi"(%3, %20) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %36 = "arith.cmpi"(%34, %5) <{predicate = 2 : i64}> : (index, index) -> i1
    %37 = "arith.cmpi"(%34, %4) <{predicate = 5 : i64}> : (index, index) -> i1
    %38 = "arith.ori"(%36, %37) : (i1, i1) -> i1
    %39 = "scf.if"(%38) ({
      "scf.yield"(%33) : (tensor<4xi32>) -> ()
    }, {
      %68 = "tensor.extract_slice"(%33, %34) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<4xi32>, index) -> tensor<?xi32>
      %69 = "tensor.insert_slice"(%68, %18, %34) <{operandSegmentSizes = array<i32: 1, 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<?xi32>, tensor<4xi32>, index) -> tensor<4xi32>
      %70 = "tensor.extract_slice"(%69, %35) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<4xi32>, index) -> tensor<?xi32>
      %71 = "tensor.insert_slice"(%70, %33, %35) <{operandSegmentSizes = array<i32: 1, 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<?xi32>, tensor<4xi32>, index) -> tensor<4xi32>
      "scf.yield"(%71) : (tensor<4xi32>) -> ()
    }) : (i1) -> tensor<4xi32>
    %40 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<4xi32>
    %41 = "memref.subview"(%21, %29) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<4xi32, strided<[1], offset: ?>>, index) -> memref<?xi32, strided<[1], offset: ?>>
    %42 = "memref.subview"(%40, %29) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<4xi32>, index) -> memref<?xi32, strided<[1]>>
    "hivm.hir.load"(%41, %42, %7, %5, %30) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xi32, strided<[1], offset: ?>>, memref<?xi32, strided<[1]>>, i32, index, i1) -> ()
    %43 = "bufferization.to_tensor"(%40) <{restrict, writable}> : (memref<4xi32>) -> tensor<4xi32>
    %44 = "memref.reinterpret_cast"(%arg4, %20) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 4>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<4xi32, strided<[1], offset: ?>>
    %45 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<4xi32>
    %46 = "memref.subview"(%44, %29) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<4xi32, strided<[1], offset: ?>>, index) -> memref<?xi32, strided<[1], offset: ?>>
    %47 = "memref.subview"(%45, %29) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<4xi32>, index) -> memref<?xi32, strided<[1]>>
    "hivm.hir.load"(%46, %47, %7, %5, %30) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xi32, strided<[1], offset: ?>>, memref<?xi32, strided<[1]>>, i32, index, i1) -> ()
    %48 = "bufferization.to_tensor"(%45) <{restrict, writable}> : (memref<4xi32>) -> tensor<4xi32>
    %49 = "hivm.hir.vbrc"(%arg6, %16) <{broadcast_dims = array<i64>}> : (i32, tensor<4xi32>) -> tensor<4xi32>
    %50 = "tensor.empty"() : () -> tensor<4xi1>
    %51 = "hivm.hir.vcmp"(%48, %49, %50) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<4xi32>, tensor<4xi32>, tensor<4xi1>) -> tensor<4xi1>
    %52 = "hivm.hir.vsel"(%51, %arg7, %48, %15) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<4xi1>, i32, tensor<4xi32>, tensor<4xi32>) -> tensor<4xi32>
    %53 = "tensor.empty"() : () -> tensor<16xi32>
    "scf.for"(%7, %6, %0) ({
    ^bb0(%arg12: i32):
      %54 = "arith.index_cast"(%arg12) : (i32) -> index
      %55 = "tensor.extract"(%43, %54) {DiscreteMemAccess} : (tensor<4xi32>, index) -> i32
      %56 = "tensor.extract"(%39, %54) {DiscreteMemAccess} : (tensor<4xi32>, index) -> i32
      %57 = "arith.subi"(%55, %56) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %58 = "tensor.extract"(%39, %54) : (tensor<4xi32>, index) -> i32
      %59 = "tensor.extract"(%52, %54) : (tensor<4xi32>, index) -> i32
      %60 = "arith.index_cast"(%58) : (i32) -> index
      %61 = "memref.reinterpret_cast"(%arg3, %60) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<16xi32, strided<[1], offset: ?>>
      %62 = "hivm.hir.vbrc"(%59, %53) <{broadcast_dims = array<i64>}> : (i32, tensor<16xi32>) -> tensor<16xi32>
      %63 = "arith.index_cast"(%57) : (i32) -> index
      %64 = "arith.maxsi"(%63, %5) : (index, index) -> index
      %65 = "arith.minsi"(%64, %1) : (index, index) -> index
      %66 = "tensor.extract_slice"(%62, %65) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<16xi32>, index) -> tensor<?xi32>
      %67 = "memref.subview"(%61, %65) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<16xi32, strided<[1], offset: ?>>, index) -> memref<?xi32, strided<[1], offset: ?>>
      "hivm.hir.store"(%66, %67) : (tensor<?xi32>, memref<?xi32, strided<[1], offset: ?>>) -> ()
      "scf.yield"() : () -> ()
    }) : (i32, i32, i32) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, false, false, false, false, false, false]> : vector<12xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

