"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, i32, i32, i32, i32) -> (), sym_name = "fused_gdn_gating_kernel"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xbf16>, %arg4: memref<?xbf16>, %arg5: memref<?xbf16>, %arg6: memref<?xbf16>, %arg7: memref<?xbf16>, %arg8: memref<?xbf16>, %arg9: i32, %arg10: i32, %arg11: i32, %arg12: i32):
    %0 = "arith.constant"() <{value = -1.000000e+00 : f32}> : () -> f32
    %1 = "arith.constant"() <{value = 4 : index}> : () -> index
    %2 = "arith.constant"() <{value = 16 : index}> : () -> index
    %3 = "arith.constant"() <{value = 0.000000e+00 : bf16}> : () -> bf16
    %4 = "arith.constant"() <{value = 2.000000e+01 : f32}> : () -> f32
    %5 = "arith.constant"() <{value = 1.000000e+00 : f32}> : () -> f32
    %6 = "arith.constant"() <{value = 4 : i32}> : () -> i32
    %7 = "arith.constant"() <{value = 16 : i32}> : () -> i32
    %8 = "arith.constant"() <{value = 0.000000e+00 : f32}> : () -> f32
    %9 = "arith.constant"() <{value = 0 : index}> : () -> index
    "hivm.hir.set_mask_norm"() : () -> ()
    %10 = "arith.muli"(%arg10, %arg11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %11 = "arith.muli"(%10, %arg12) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%11) {logical_block_num} : (i32) -> ()
    %12 = "hivm.hir.get_block_idx"() : () -> i64
    %13 = "arith.trunci"(%12) : (i64) -> i32
    %14 = "arith.divsi"(%13, %arg12) : (i32, i32) -> i32
    %15 = "arith.remsi"(%14, %arg11) : (i32, i32) -> i32
    %16 = "arith.muli"(%arg12, %arg11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %17 = "arith.divsi"(%13, %16) : (i32, i32) -> i32
    %18 = "arith.remsi"(%17, %arg10) : (i32, i32) -> i32
    %19 = "tensor.empty"() : () -> tensor<16xf32>
    %20 = "tensor.empty"() : () -> tensor<16xf32>
    %21 = "tensor.empty"() : () -> tensor<16xf32>
    %22 = "tensor.empty"() : () -> tensor<16xf32>
    %23 = "tensor.empty"() : () -> tensor<16xf32>
    %24 = "tensor.empty"() : () -> tensor<4x16xf32>
    %25 = "tensor.empty"() : () -> tensor<4x16xf32>
    %26 = "tensor.empty"() : () -> tensor<4x16xf32>
    %27 = "tensor.empty"() : () -> tensor<4x16xf32>
    %28 = "tensor.empty"() : () -> tensor<4x16xf32>
    %29 = "tensor.empty"() : () -> tensor<4x16xf32>
    %30 = "tensor.empty"() : () -> tensor<4x16xf32>
    %31 = "tensor.empty"() : () -> tensor<4x16xf32>
    %32 = "tensor.empty"() : () -> tensor<4x16xf32>
    %33 = "tensor.empty"() : () -> tensor<4x16xf32>
    %34 = "tensor.empty"() : () -> tensor<4x16xf32>
    %35 = "tensor.empty"() : () -> tensor<4x16xf32>
    %36 = "tensor.empty"() : () -> tensor<4x16xf32>
    %37 = "tensor.empty"() : () -> tensor<4x16xf32>
    %38 = "tensor.empty"() : () -> tensor<4x16xf32>
    %39 = "tensor.empty"() : () -> tensor<4x16xf32>
    %40 = "hivm.hir.vbrc"(%4, %39) <{broadcast_dims = array<i64>}> : (f32, tensor<4x16xf32>) -> tensor<4x16xf32>
    %41 = "arith.muli"(%18, %6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %42 = "arith.muli"(%15, %7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %43 = "memref.reinterpret_cast"(%arg5) <{operandSegmentSizes = array<i32: 1, 0, 0, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: 16>, static_strides = array<i64: 1>}> : (memref<?xbf16>) -> memref<16xbf16, strided<[1]>>
    %44 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16xbf16>
    "hivm.hir.load"(%43, %44) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<16xbf16, strided<[1]>>, memref<16xbf16>) -> ()
    %45 = "bufferization.to_tensor"(%44) <{restrict, writable}> : (memref<16xbf16>) -> tensor<16xbf16>
    %46 = "arith.index_cast"(%41) : (i32) -> index
    %47 = "arith.index_cast"(%arg9) : (i32) -> index
    %48 = "arith.muli"(%46, %47) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %49 = "arith.muli"(%48, %2) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %50 = "arith.muli"(%47, %2) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %51 = "arith.index_cast"(%42) : (i32) -> index
    %52 = "arith.addi"(%49, %51) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %53 = "memref.reinterpret_cast"(%arg6, %52, %50) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 4, 16>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<4x16xbf16, strided<[?, 1], offset: ?>>
    %54 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<4x16xbf16>
    %55 = "arith.addi"(%46, %1) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %56 = "arith.maxsi"(%46, %1) : (index, index) -> index
    %57 = "arith.minsi"(%55, %56) : (index, index) -> index
    %58 = "arith.subi"(%57, %46) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %59 = "arith.minsi"(%58, %1) : (index, index) -> index
    %60 = "arith.cmpi"(%59, %1) <{predicate = 2 : i64}> : (index, index) -> i1
    %61 = "memref.subview"(%53, %59) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (memref<4x16xbf16, strided<[?, 1], offset: ?>>, index) -> memref<?x16xbf16, strided<[?, 1], offset: ?>>
    %62 = "memref.subview"(%54, %59) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (memref<4x16xbf16>, index) -> memref<?x16xbf16, strided<[16, 1]>>
    "hivm.hir.load"(%61, %62, %3, %9, %60) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x16xbf16, strided<[?, 1], offset: ?>>, memref<?x16xbf16, strided<[16, 1]>>, bf16, index, i1) -> ()
    %63 = "bufferization.to_tensor"(%54) <{restrict, writable}> : (memref<4x16xbf16>) -> tensor<4x16xbf16>
    %64 = "memref.reinterpret_cast"(%arg7, %52, %50) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 4, 16>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<4x16xbf16, strided<[?, 1], offset: ?>>
    %65 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<4x16xbf16>
    %66 = "memref.subview"(%64, %59) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (memref<4x16xbf16, strided<[?, 1], offset: ?>>, index) -> memref<?x16xbf16, strided<[?, 1], offset: ?>>
    %67 = "memref.subview"(%65, %59) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (memref<4x16xbf16>, index) -> memref<?x16xbf16, strided<[16, 1]>>
    "hivm.hir.load"(%66, %67, %3, %9, %60) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x16xbf16, strided<[?, 1], offset: ?>>, memref<?x16xbf16, strided<[16, 1]>>, bf16, index, i1) -> ()
    %68 = "bufferization.to_tensor"(%65) <{restrict, writable}> : (memref<4x16xbf16>) -> tensor<4x16xbf16>
    %69 = "memref.reinterpret_cast"(%arg8) <{operandSegmentSizes = array<i32: 1, 0, 0, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: 16>, static_strides = array<i64: 1>}> : (memref<?xbf16>) -> memref<16xbf16, strided<[1]>>
    %70 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16xbf16>
    "hivm.hir.load"(%69, %70) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<16xbf16, strided<[1]>>, memref<16xbf16>) -> ()
    %71 = "bufferization.to_tensor"(%70) <{restrict, writable}> : (memref<16xbf16>) -> tensor<16xbf16>
    %72 = "hivm.hir.vcast"(%63, %38) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<4x16xbf16>, tensor<4x16xf32>) -> tensor<4x16xf32>
    %73 = "hivm.hir.vcast"(%71, %23) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16xbf16>, tensor<16xf32>) -> tensor<16xf32>
    %74 = "tensor.expand_shape"(%73) <{reassociation = [[0, 1]], static_output_shape = array<i64: 1, 16>}> : (tensor<16xf32>) -> tensor<1x16xf32>
    %75 = "hivm.hir.vbrc"(%74, %37) <{broadcast_dims = array<i64: 0>}> : (tensor<1x16xf32>, tensor<4x16xf32>) -> tensor<4x16xf32>
    %76 = "hivm.hir.vadd"(%72, %75, %36) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<4x16xf32>, tensor<4x16xf32>, tensor<4x16xf32>) -> tensor<4x16xf32>
    %77 = "tensor.empty"() : () -> tensor<4x16xi1>
    %78 = "hivm.hir.vcmp"(%76, %40, %77) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<le>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<4x16xf32>, tensor<4x16xf32>, tensor<4x16xi1>) -> tensor<4x16xi1>
    %79 = "hivm.hir.vexp"(%76, %35) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<4x16xf32>, tensor<4x16xf32>) -> tensor<4x16xf32>
    %80 = "hivm.hir.vadd"(%79, %5, %34) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<4x16xf32>, f32, tensor<4x16xf32>) -> tensor<4x16xf32>
    %81 = "hivm.hir.vln"(%80, %33) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<4x16xf32>, tensor<4x16xf32>) -> tensor<4x16xf32>
    %82 = "hivm.hir.vsel"(%78, %81, %76, %32) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<4x16xi1>, tensor<4x16xf32>, tensor<4x16xf32>, tensor<4x16xf32>) -> tensor<4x16xf32>
    %83 = "hivm.hir.vcast"(%45, %22) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16xbf16>, tensor<16xf32>) -> tensor<16xf32>
    %84 = "hivm.hir.vexp"(%83, %21) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
    %85 = "hivm.hir.vmul"(%84, %0, %20) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, f32, tensor<16xf32>) -> tensor<16xf32>
    %86 = "hivm.hir.vadd"(%85, %8, %19) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, f32, tensor<16xf32>) -> tensor<16xf32>
    %87 = "tensor.expand_shape"(%86) <{reassociation = [[0, 1]], static_output_shape = array<i64: 1, 16>}> : (tensor<16xf32>) -> tensor<1x16xf32>
    %88 = "hivm.hir.vbrc"(%87, %31) <{broadcast_dims = array<i64: 0>}> : (tensor<1x16xf32>, tensor<4x16xf32>) -> tensor<4x16xf32>
    %89 = "hivm.hir.vmul"(%88, %82, %30) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<4x16xf32>, tensor<4x16xf32>, tensor<4x16xf32>) -> tensor<4x16xf32>
    %90 = "memref.reinterpret_cast"(%arg3, %52, %50) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 4, 16>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<4x16xbf16, strided<[?, 1], offset: ?>>
    %91 = "tensor.empty"() : () -> tensor<4x16xbf16>
    %92 = "tensor.empty"() : () -> tensor<4x16xbf16>
    %93 = "hivm.hir.vcast"(%89, %92) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<4x16xf32>, tensor<4x16xbf16>) -> tensor<4x16xbf16>
    %94 = "tensor.extract_slice"(%93, %59) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (tensor<4x16xbf16>, index) -> tensor<?x16xbf16>
    %95 = "memref.subview"(%90, %59) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (memref<4x16xbf16, strided<[?, 1], offset: ?>>, index) -> memref<?x16xbf16, strided<[?, 1], offset: ?>>
    "hivm.hir.store"(%94, %95) : (tensor<?x16xbf16>, memref<?x16xbf16, strided<[?, 1], offset: ?>>) -> ()
    %96 = "hivm.hir.vcast"(%68, %29) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<4x16xbf16>, tensor<4x16xf32>) -> tensor<4x16xf32>
    %97 = "hivm.hir.vmul"(%96, %0, %28) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<4x16xf32>, f32, tensor<4x16xf32>) -> tensor<4x16xf32>
    %98 = "hivm.hir.vadd"(%97, %8, %27) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<4x16xf32>, f32, tensor<4x16xf32>) -> tensor<4x16xf32>
    %99 = "hivm.hir.vexp"(%98, %26) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<4x16xf32>, tensor<4x16xf32>) -> tensor<4x16xf32>
    %100 = "hivm.hir.vadd"(%99, %5, %25) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<4x16xf32>, f32, tensor<4x16xf32>) -> tensor<4x16xf32>
    %101 = "tensor.empty"() : () -> tensor<4x16xf32>
    %102 = "hivm.hir.vbrc"(%5, %101) <{broadcast_dims = array<i64>}> : (f32, tensor<4x16xf32>) -> tensor<4x16xf32>
    %103 = "hivm.hir.vdiv"(%102, %100, %24) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<4x16xf32>, tensor<4x16xf32>, tensor<4x16xf32>) -> tensor<4x16xf32>
    %104 = "memref.reinterpret_cast"(%arg4, %52, %50) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 4, 16>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<4x16xbf16, strided<[?, 1], offset: ?>>
    %105 = "hivm.hir.vcast"(%103, %91) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<4x16xf32>, tensor<4x16xbf16>) -> tensor<4x16xbf16>
    %106 = "tensor.extract_slice"(%105, %59) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (tensor<4x16xbf16>, index) -> tensor<?x16xbf16>
    %107 = "memref.subview"(%104, %59) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (memref<4x16xbf16, strided<[?, 1], offset: ?>>, index) -> memref<?x16xbf16, strided<[?, 1], offset: ?>>
    "hivm.hir.store"(%106, %107) : (tensor<?x16xbf16>, memref<?x16xbf16, strided<[?, 1], offset: ?>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, true, true, true, false, false, false, false]> : vector<13xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

