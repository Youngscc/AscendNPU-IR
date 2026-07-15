"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xf32>, memref<?xf32>, memref<?xf32>, i32, i32, f32, i32, i32, i32) -> (), sym_name = "rmsnorm_kernel"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xf32>, %arg4: memref<?xf32>, %arg5: memref<?xf32>, %arg6: i32, %arg7: i32, %arg8: f32, %arg9: i32, %arg10: i32, %arg11: i32):
    %0 = "arith.constant"() <{value = 0.000000e+00 : f32}> : () -> f32
    %1 = "arith.constant"() <{value = 2048 : index}> : () -> index
    %2 = "arith.constant"() <{value = 0 : index}> : () -> index
    %3 = "arith.constant"() <{value = 1.000000e+00 : f32}> : () -> f32
    "hivm.hir.set_mask_norm"() : () -> ()
    %4 = "arith.muli"(%arg9, %arg10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %5 = "arith.muli"(%4, %arg11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%5) {logical_block_num} : (i32) -> ()
    %6 = "hivm.hir.get_block_idx"() : () -> i64
    %7 = "arith.trunci"(%6) : (i64) -> i32
    %8 = "arith.muli"(%arg11, %arg10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %9 = "arith.divsi"(%7, %8) : (i32, i32) -> i32
    %10 = "arith.remsi"(%9, %arg9) : (i32, i32) -> i32
    %11 = "arith.muli"(%10, %arg6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %12 = "arith.index_cast"(%11) : (i32) -> index
    %13 = "memref.reinterpret_cast"(%arg3, %12) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 2048>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<2048xf32, strided<[1], offset: ?>>
    %14 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<2048xf32>
    %15 = "arith.index_cast"(%arg7) : (i32) -> index
    %16 = "arith.maxsi"(%15, %2) : (index, index) -> index
    %17 = "arith.minsi"(%16, %1) : (index, index) -> index
    %18 = "arith.cmpi"(%17, %1) <{predicate = 2 : i64}> : (index, index) -> i1
    %19 = "memref.subview"(%13, %17) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<2048xf32, strided<[1], offset: ?>>, index) -> memref<?xf32, strided<[1], offset: ?>>
    %20 = "memref.subview"(%14, %17) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<2048xf32>, index) -> memref<?xf32, strided<[1]>>
    "hivm.hir.load"(%19, %20, %0, %2, %18) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xf32, strided<[1], offset: ?>>, memref<?xf32, strided<[1]>>, f32, index, i1) -> ()
    %21 = "bufferization.to_tensor"(%14) <{restrict, writable}> : (memref<2048xf32>) -> tensor<2048xf32>
    %22 = "tensor.empty"() : () -> tensor<2048xf32>
    %23 = "tensor.empty"() : () -> tensor<2048xf32>
    %24 = "tensor.empty"() : () -> tensor<2048xf32>
    %25 = "hivm.hir.vmul"(%21, %21, %24) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2048xf32>, tensor<2048xf32>, tensor<2048xf32>) -> tensor<2048xf32>
    %26 = "bufferization.alloc_tensor"() <{operandSegmentSizes = array<i32: 0, 0, 0>}> : () -> tensor<f32>
    %27 = "tensor.empty"() : () -> tensor<1xf32>
    %28 = "hivm.hir.vreduce"(%25, %27) <{arith = #hivm.reduce_op<sum>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 0>}> : (tensor<2048xf32>, tensor<1xf32>) -> tensor<1xf32>
    %29 = "tensor.extract"(%28, %2) : (tensor<1xf32>, index) -> f32
    %30 = "arith.sitofp"(%arg7) : (i32) -> f32
    %31 = "arith.divf"(%29, %30) <{fastmath = #arith.fastmath<none>}> : (f32, f32) -> f32
    %32 = "tensor.empty"() : () -> tensor<1xf32>
    %33 = "tensor.empty"() : () -> tensor<1xf32>
    %34 = "tensor.empty"() : () -> tensor<1xf32>
    %35 = "tensor.empty"() : () -> tensor<1xf32>
    %36 = "tensor.empty"() : () -> tensor<1xf32>
    %37 = "tensor.insert"(%31, %36, %2) : (f32, tensor<1xf32>, index) -> tensor<1xf32>
    %38 = "tensor.insert"(%arg8, %35, %2) : (f32, tensor<1xf32>, index) -> tensor<1xf32>
    %39 = "hivm.hir.vadd"(%37, %38, %33) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1xf32>, tensor<1xf32>, tensor<1xf32>) -> tensor<1xf32>
    %40 = "tensor.extract"(%39, %2) : (tensor<1xf32>, index) -> f32
    %41 = "tensor.insert"(%40, %34, %2) : (f32, tensor<1xf32>, index) -> tensor<1xf32>
    %42 = "hivm.hir.vsqrt"(%41, %32) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<1xf32>, tensor<1xf32>) -> tensor<1xf32>
    %43 = "tensor.extract"(%42, %2) : (tensor<1xf32>, index) -> f32
    %44 = "arith.divf"(%3, %43) <{fastmath = #arith.fastmath<none>}> : (f32, f32) -> f32
    %45 = "memref.reinterpret_cast"(%arg5) <{operandSegmentSizes = array<i32: 1, 0, 0, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: 2048>, static_strides = array<i64: 1>}> : (memref<?xf32>) -> memref<2048xf32, strided<[1]>>
    %46 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<2048xf32>
    %47 = "memref.subview"(%45, %17) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<2048xf32, strided<[1]>>, index) -> memref<?xf32, strided<[1]>>
    %48 = "memref.subview"(%46, %17) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<2048xf32>, index) -> memref<?xf32, strided<[1]>>
    "hivm.hir.load"(%47, %48, %0, %2, %18) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xf32, strided<[1]>>, memref<?xf32, strided<[1]>>, f32, index, i1) -> ()
    %49 = "bufferization.to_tensor"(%46) <{restrict, writable}> : (memref<2048xf32>) -> tensor<2048xf32>
    %50 = "memref.reinterpret_cast"(%arg4, %12) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 2048>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<2048xf32, strided<[1], offset: ?>>
    %51 = "hivm.hir.vmul"(%21, %44, %23) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2048xf32>, f32, tensor<2048xf32>) -> tensor<2048xf32>
    %52 = "hivm.hir.vmul"(%51, %49, %22) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2048xf32>, tensor<2048xf32>, tensor<2048xf32>) -> tensor<2048xf32>
    %53 = "tensor.extract_slice"(%52, %17) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<2048xf32>, index) -> tensor<?xf32>
    %54 = "memref.subview"(%50, %17) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<2048xf32, strided<[1], offset: ?>>, index) -> memref<?xf32, strided<[1], offset: ?>>
    "hivm.hir.store"(%53, %54) : (tensor<?xf32>, memref<?xf32, strided<[1], offset: ?>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, false, false, false, false, false, false]> : vector<12xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

