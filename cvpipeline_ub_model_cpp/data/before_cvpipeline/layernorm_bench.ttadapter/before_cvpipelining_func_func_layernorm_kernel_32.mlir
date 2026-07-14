"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xf32>, memref<?xf32>, memref<?xf32>, memref<?xf32>, i32, i32, f32, i32, i32, i32) -> (), sym_name = "layernorm_kernel"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xf32>, %arg4: memref<?xf32>, %arg5: memref<?xf32>, %arg6: memref<?xf32>, %arg7: i32, %arg8: i32, %arg9: f32, %arg10: i32, %arg11: i32, %arg12: i32):
    %0 = "arith.constant"() <{value = 1.000000e+00 : f32}> : () -> f32
    %1 = "arith.constant"() <{value = 2048 : index}> : () -> index
    %2 = "arith.constant"() <{value = 0 : index}> : () -> index
    %3 = "arith.constant"() <{value = 0.000000e+00 : f32}> : () -> f32
    "hivm.hir.set_mask_norm"() : () -> ()
    %4 = "arith.muli"(%arg10, %arg11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %5 = "arith.muli"(%4, %arg12) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%5) {logical_block_num} : (i32) -> ()
    %6 = "hivm.hir.get_block_idx"() : () -> i64
    %7 = "arith.trunci"(%6) : (i64) -> i32
    %8 = "arith.muli"(%arg12, %arg11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %9 = "arith.divsi"(%7, %8) : (i32, i32) -> i32
    %10 = "arith.remsi"(%9, %arg10) : (i32, i32) -> i32
    %11 = "tensor.empty"() : () -> tensor<2048xf32>
    %12 = "tensor.empty"() : () -> tensor<2048xf32>
    %13 = "tensor.empty"() : () -> tensor<2048xf32>
    %14 = "tensor.empty"() : () -> tensor<2048xf32>
    %15 = "tensor.empty"() : () -> tensor<2048xf32>
    %16 = "tensor.empty"() : () -> tensor<2048xf32>
    %17 = "hivm.hir.vbrc"(%3, %16) <{broadcast_dims = array<i64>}> : (f32, tensor<2048xf32>) -> tensor<2048xf32>
    %18 = "arith.muli"(%10, %arg7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %19 = "arith.index_cast"(%18) : (i32) -> index
    %20 = "memref.reinterpret_cast"(%arg3, %19) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 2048>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<2048xf32, strided<[1], offset: ?>>
    %21 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<2048xf32>
    %22 = "arith.index_cast"(%arg8) : (i32) -> index
    %23 = "arith.maxsi"(%22, %2) : (index, index) -> index
    %24 = "arith.minsi"(%23, %1) : (index, index) -> index
    %25 = "arith.cmpi"(%24, %1) <{predicate = 2 : i64}> : (index, index) -> i1
    %26 = "memref.subview"(%20, %24) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<2048xf32, strided<[1], offset: ?>>, index) -> memref<?xf32, strided<[1], offset: ?>>
    %27 = "memref.subview"(%21, %24) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<2048xf32>, index) -> memref<?xf32, strided<[1]>>
    "hivm.hir.load"(%26, %27, %3, %2, %25) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xf32, strided<[1], offset: ?>>, memref<?xf32, strided<[1]>>, f32, index, i1) -> ()
    %28 = "bufferization.to_tensor"(%21) <{restrict, writable}> : (memref<2048xf32>) -> tensor<2048xf32>
    %29 = "bufferization.alloc_tensor"() <{operandSegmentSizes = array<i32: 0, 0, 0>}> : () -> tensor<f32>
    %30 = "tensor.empty"() : () -> tensor<1xf32>
    %31 = "hivm.hir.vreduce"(%28, %30) <{arith = #hivm.reduce_op<sum>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 0>}> : (tensor<2048xf32>, tensor<1xf32>) -> tensor<1xf32>
    %32 = "tensor.extract"(%31, %2) : (tensor<1xf32>, index) -> f32
    %33 = "arith.sitofp"(%arg8) : (i32) -> f32
    %34 = "arith.divf"(%32, %33) <{fastmath = #arith.fastmath<none>}> : (f32, f32) -> f32
    %35 = "hivm.hir.vsub"(%28, %34, %15) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2048xf32>, f32, tensor<2048xf32>) -> tensor<2048xf32>
    %36 = "tensor.extract_slice"(%35, %24) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<2048xf32>, index) -> tensor<?xf32>
    %37 = "tensor.insert_slice"(%36, %17, %24) <{operandSegmentSizes = array<i32: 1, 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<?xf32>, tensor<2048xf32>, index) -> tensor<2048xf32>
    %38 = "hivm.hir.vmul"(%37, %37, %14) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2048xf32>, tensor<2048xf32>, tensor<2048xf32>) -> tensor<2048xf32>
    %39 = "bufferization.alloc_tensor"() <{operandSegmentSizes = array<i32: 0, 0, 0>}> : () -> tensor<f32>
    %40 = "tensor.empty"() : () -> tensor<1xf32>
    %41 = "hivm.hir.vreduce"(%38, %40) <{arith = #hivm.reduce_op<sum>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 0>}> : (tensor<2048xf32>, tensor<1xf32>) -> tensor<1xf32>
    %42 = "tensor.extract"(%41, %2) : (tensor<1xf32>, index) -> f32
    %43 = "arith.divf"(%42, %33) <{fastmath = #arith.fastmath<none>}> : (f32, f32) -> f32
    %44 = "tensor.empty"() : () -> tensor<1xf32>
    %45 = "tensor.empty"() : () -> tensor<1xf32>
    %46 = "tensor.empty"() : () -> tensor<1xf32>
    %47 = "tensor.empty"() : () -> tensor<1xf32>
    %48 = "tensor.empty"() : () -> tensor<1xf32>
    %49 = "tensor.insert"(%43, %48, %2) : (f32, tensor<1xf32>, index) -> tensor<1xf32>
    %50 = "tensor.insert"(%arg9, %47, %2) : (f32, tensor<1xf32>, index) -> tensor<1xf32>
    %51 = "hivm.hir.vadd"(%49, %50, %45) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1xf32>, tensor<1xf32>, tensor<1xf32>) -> tensor<1xf32>
    %52 = "tensor.extract"(%51, %2) : (tensor<1xf32>, index) -> f32
    %53 = "tensor.insert"(%52, %46, %2) : (f32, tensor<1xf32>, index) -> tensor<1xf32>
    %54 = "hivm.hir.vsqrt"(%53, %44) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<1xf32>, tensor<1xf32>) -> tensor<1xf32>
    %55 = "tensor.extract"(%54, %2) : (tensor<1xf32>, index) -> f32
    %56 = "arith.divf"(%0, %55) <{fastmath = #arith.fastmath<none>}> : (f32, f32) -> f32
    %57 = "memref.reinterpret_cast"(%arg5) <{operandSegmentSizes = array<i32: 1, 0, 0, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: 2048>, static_strides = array<i64: 1>}> : (memref<?xf32>) -> memref<2048xf32, strided<[1]>>
    %58 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<2048xf32>
    %59 = "memref.subview"(%57, %24) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<2048xf32, strided<[1]>>, index) -> memref<?xf32, strided<[1]>>
    %60 = "memref.subview"(%58, %24) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<2048xf32>, index) -> memref<?xf32, strided<[1]>>
    "hivm.hir.load"(%59, %60, %3, %2, %25) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xf32, strided<[1]>>, memref<?xf32, strided<[1]>>, f32, index, i1) -> ()
    %61 = "bufferization.to_tensor"(%58) <{restrict, writable}> : (memref<2048xf32>) -> tensor<2048xf32>
    %62 = "memref.reinterpret_cast"(%arg6) <{operandSegmentSizes = array<i32: 1, 0, 0, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: 2048>, static_strides = array<i64: 1>}> : (memref<?xf32>) -> memref<2048xf32, strided<[1]>>
    %63 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<2048xf32>
    %64 = "memref.subview"(%62, %24) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<2048xf32, strided<[1]>>, index) -> memref<?xf32, strided<[1]>>
    %65 = "memref.subview"(%63, %24) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<2048xf32>, index) -> memref<?xf32, strided<[1]>>
    "hivm.hir.load"(%64, %65, %3, %2, %25) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xf32, strided<[1]>>, memref<?xf32, strided<[1]>>, f32, index, i1) -> ()
    %66 = "bufferization.to_tensor"(%63) <{restrict, writable}> : (memref<2048xf32>) -> tensor<2048xf32>
    %67 = "memref.reinterpret_cast"(%arg4, %19) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 2048>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<2048xf32, strided<[1], offset: ?>>
    %68 = "hivm.hir.vmul"(%37, %56, %13) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2048xf32>, f32, tensor<2048xf32>) -> tensor<2048xf32>
    %69 = "hivm.hir.vmul"(%68, %61, %12) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2048xf32>, tensor<2048xf32>, tensor<2048xf32>) -> tensor<2048xf32>
    %70 = "hivm.hir.vadd"(%69, %66, %11) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2048xf32>, tensor<2048xf32>, tensor<2048xf32>) -> tensor<2048xf32>
    %71 = "tensor.extract_slice"(%70, %24) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<2048xf32>, index) -> tensor<?xf32>
    %72 = "memref.subview"(%67, %24) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<2048xf32, strided<[1], offset: ?>>, index) -> memref<?xf32, strided<[1], offset: ?>>
    "hivm.hir.store"(%71, %72) : (tensor<?xf32>, memref<?xf32, strided<[1], offset: ?>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, true, false, false, false, false, false, false]> : vector<13xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

