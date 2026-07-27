#map = affine_map<()[s0, s1] -> (s0 * s1)>
#map1 = affine_map<()[s0] -> (s0 * 16)>
#map2 = affine_map<()[s0, s1] -> (s0 + s1)>
#map3 = affine_map<()[s0] -> (s0 + 4)>
#map4 = affine_map<()[s0] -> (s0, 4)>
#map5 = affine_map<()[s0, s1] -> (s0, s1)>
#map6 = affine_map<()[s0, s1] -> (s0 - s1)>
"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, i32, i32, i32, i32) -> (), sym_name = "fused_gdn_gating_kernel"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xbf16>, %arg4: memref<?xbf16>, %arg5: memref<?xbf16>, %arg6: memref<?xbf16>, %arg7: memref<?xbf16>, %arg8: memref<?xbf16>, %arg9: i32, %arg10: i32, %arg11: i32, %arg12: i32):
    %0 = "arith.constant"() <{value = -1.000000e+00 : f32}> : () -> f32
    %1 = "arith.constant"() <{value = 4 : index}> : () -> index
    %2 = "arith.constant"() <{value = 0.000000e+00 : bf16}> : () -> bf16
    %3 = "arith.constant"() <{value = 2.000000e+01 : f32}> : () -> f32
    %4 = "arith.constant"() <{value = 1.000000e+00 : f32}> : () -> f32
    %5 = "arith.constant"() <{value = 4 : i32}> : () -> i32
    %6 = "arith.constant"() <{value = 16 : i32}> : () -> i32
    %7 = "arith.constant"() <{value = 0.000000e+00 : f32}> : () -> f32
    %8 = "arith.constant"() <{value = 0 : index}> : () -> index
    "hivm.hir.set_mask_norm"() : () -> ()
    %9 = "arith.muli"(%arg10, %arg11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %10 = "arith.muli"(%9, %arg12) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%10) <{effects = ["write"]}> {logical_block_num} : (i32) -> ()
    %11 = "hivm.hir.get_block_idx"() : () -> i64
    %12 = "arith.trunci"(%11) : (i64) -> i32
    %13 = "arith.divsi"(%12, %arg12) : (i32, i32) -> i32
    %14 = "arith.remsi"(%13, %arg11) : (i32, i32) -> i32
    %15 = "arith.muli"(%arg12, %arg11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %16 = "arith.divsi"(%12, %15) : (i32, i32) -> i32
    %17 = "arith.remsi"(%16, %arg10) : (i32, i32) -> i32
    %18 = "tensor.empty"() : () -> tensor<16xf32>
    %19 = "tensor.empty"() : () -> tensor<16xf32>
    %20 = "tensor.empty"() : () -> tensor<16xf32>
    %21 = "tensor.empty"() : () -> tensor<16xf32>
    %22 = "tensor.empty"() : () -> tensor<16xf32>
    %23 = "tensor.empty"() : () -> tensor<4x16xf32>
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
    %39 = "hivm.hir.vbrc"(%3, %38) <{broadcast_dims = array<i64>}> : (f32, tensor<4x16xf32>) -> tensor<4x16xf32>
    %40 = "arith.muli"(%17, %5) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %41 = "arith.muli"(%14, %6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %42 = "memref.reinterpret_cast"(%arg5) <{operandSegmentSizes = array<i32: 1, 0, 0, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: 16>, static_strides = array<i64: 1>}> : (memref<?xbf16>) -> memref<16xbf16, strided<[1]>>
    %43 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16xbf16>
    "hivm.hir.load"(%42, %43) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<16xbf16, strided<[1]>>, memref<16xbf16>) -> ()
    %44 = "bufferization.to_tensor"(%43) <{restrict, writable}> : (memref<16xbf16>) -> tensor<16xbf16>
    %45 = "arith.index_cast"(%40) : (i32) -> index
    %46 = "arith.index_cast"(%arg9) : (i32) -> index
    %47 = "affine.apply"(%45, %46) <{map = #map}> : (index, index) -> index
    %48 = "affine.apply"(%47) <{map = #map1}> : (index) -> index
    %49 = "affine.apply"(%46) <{map = #map1}> : (index) -> index
    %50 = "arith.index_cast"(%41) : (i32) -> index
    %51 = "affine.apply"(%48, %50) <{map = #map2}> : (index, index) -> index
    %52 = "memref.reinterpret_cast"(%arg6, %51, %49) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 4, 16>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<4x16xbf16, strided<[?, 1], offset: ?>>
    %53 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<4x16xbf16>
    %54 = "affine.apply"(%45) <{map = #map3}> : (index) -> index
    %55 = "affine.max"(%45) <{map = #map4}> : (index) -> index
    %56 = "affine.min"(%54, %55) <{map = #map5}> : (index, index) -> index
    %57 = "affine.apply"(%56, %45) <{map = #map6}> : (index, index) -> index
    %58 = "affine.min"(%57) <{map = #map4}> : (index) -> index
    %59 = "arith.cmpi"(%58, %1) <{predicate = 2 : i64}> : (index, index) -> i1
    %60 = "memref.subview"(%52, %58) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (memref<4x16xbf16, strided<[?, 1], offset: ?>>, index) -> memref<?x16xbf16, strided<[?, 1], offset: ?>>
    %61 = "memref.subview"(%53, %58) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (memref<4x16xbf16>, index) -> memref<?x16xbf16, strided<[16, 1]>>
    "hivm.hir.load"(%60, %61, %2, %8, %59) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x16xbf16, strided<[?, 1], offset: ?>>, memref<?x16xbf16, strided<[16, 1]>>, bf16, index, i1) -> ()
    %62 = "bufferization.to_tensor"(%53) <{restrict, writable}> : (memref<4x16xbf16>) -> tensor<4x16xbf16>
    %63 = "memref.reinterpret_cast"(%arg7, %51, %49) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 4, 16>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<4x16xbf16, strided<[?, 1], offset: ?>>
    %64 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<4x16xbf16>
    %65 = "memref.subview"(%63, %58) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (memref<4x16xbf16, strided<[?, 1], offset: ?>>, index) -> memref<?x16xbf16, strided<[?, 1], offset: ?>>
    %66 = "memref.subview"(%64, %58) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (memref<4x16xbf16>, index) -> memref<?x16xbf16, strided<[16, 1]>>
    "hivm.hir.load"(%65, %66, %2, %8, %59) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x16xbf16, strided<[?, 1], offset: ?>>, memref<?x16xbf16, strided<[16, 1]>>, bf16, index, i1) -> ()
    %67 = "bufferization.to_tensor"(%64) <{restrict, writable}> : (memref<4x16xbf16>) -> tensor<4x16xbf16>
    %68 = "memref.reinterpret_cast"(%arg8) <{operandSegmentSizes = array<i32: 1, 0, 0, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: 16>, static_strides = array<i64: 1>}> : (memref<?xbf16>) -> memref<16xbf16, strided<[1]>>
    %69 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16xbf16>
    "hivm.hir.load"(%68, %69) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<16xbf16, strided<[1]>>, memref<16xbf16>) -> ()
    %70 = "bufferization.to_tensor"(%69) <{restrict, writable}> : (memref<16xbf16>) -> tensor<16xbf16>
    %71 = "hivm.hir.vcast"(%62, %37) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<4x16xbf16>, tensor<4x16xf32>) -> tensor<4x16xf32>
    %72 = "hivm.hir.vcast"(%70, %22) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16xbf16>, tensor<16xf32>) -> tensor<16xf32>
    %73 = "tensor.expand_shape"(%72) <{reassociation = [[0, 1]], static_output_shape = array<i64: 1, 16>}> : (tensor<16xf32>) -> tensor<1x16xf32>
    %74 = "hivm.hir.vbrc"(%73, %36) <{broadcast_dims = array<i64: 0>}> : (tensor<1x16xf32>, tensor<4x16xf32>) -> tensor<4x16xf32>
    %75 = "hivm.hir.vadd"(%71, %74, %35) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<4x16xf32>, tensor<4x16xf32>, tensor<4x16xf32>) -> tensor<4x16xf32>
    %76 = "tensor.empty"() : () -> tensor<4x16xi1>
    %77 = "hivm.hir.vcmp"(%75, %39, %76) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<le>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<4x16xf32>, tensor<4x16xf32>, tensor<4x16xi1>) -> tensor<4x16xi1>
    %78 = "hivm.hir.vexp"(%75, %34) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<4x16xf32>, tensor<4x16xf32>) -> tensor<4x16xf32>
    %79 = "hivm.hir.vadd"(%78, %4, %33) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<4x16xf32>, f32, tensor<4x16xf32>) -> tensor<4x16xf32>
    %80 = "hivm.hir.vln"(%79, %32) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<4x16xf32>, tensor<4x16xf32>) -> tensor<4x16xf32>
    %81 = "hivm.hir.vsel"(%77, %80, %75, %31) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<4x16xi1>, tensor<4x16xf32>, tensor<4x16xf32>, tensor<4x16xf32>) -> tensor<4x16xf32>
    %82 = "hivm.hir.vcast"(%44, %21) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16xbf16>, tensor<16xf32>) -> tensor<16xf32>
    %83 = "hivm.hir.vexp"(%82, %20) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
    %84 = "hivm.hir.vmul"(%83, %0, %19) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, f32, tensor<16xf32>) -> tensor<16xf32>
    %85 = "hivm.hir.vadd"(%84, %7, %18) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, f32, tensor<16xf32>) -> tensor<16xf32>
    %86 = "tensor.expand_shape"(%85) <{reassociation = [[0, 1]], static_output_shape = array<i64: 1, 16>}> : (tensor<16xf32>) -> tensor<1x16xf32>
    %87 = "hivm.hir.vbrc"(%86, %30) <{broadcast_dims = array<i64: 0>}> : (tensor<1x16xf32>, tensor<4x16xf32>) -> tensor<4x16xf32>
    %88 = "hivm.hir.vmul"(%87, %81, %29) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<4x16xf32>, tensor<4x16xf32>, tensor<4x16xf32>) -> tensor<4x16xf32>
    %89 = "memref.reinterpret_cast"(%arg3, %51, %49) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 4, 16>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<4x16xbf16, strided<[?, 1], offset: ?>>
    %90 = "tensor.empty"() : () -> tensor<4x16xbf16>
    %91 = "tensor.empty"() : () -> tensor<4x16xbf16>
    %92 = "hivm.hir.vcast"(%88, %91) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<4x16xf32>, tensor<4x16xbf16>) -> tensor<4x16xbf16>
    %93 = "tensor.extract_slice"(%92, %58) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (tensor<4x16xbf16>, index) -> tensor<?x16xbf16>
    %94 = "memref.subview"(%89, %58) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (memref<4x16xbf16, strided<[?, 1], offset: ?>>, index) -> memref<?x16xbf16, strided<[?, 1], offset: ?>>
    "hivm.hir.store"(%93, %94) : (tensor<?x16xbf16>, memref<?x16xbf16, strided<[?, 1], offset: ?>>) -> ()
    %95 = "hivm.hir.vcast"(%67, %28) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<4x16xbf16>, tensor<4x16xf32>) -> tensor<4x16xf32>
    %96 = "hivm.hir.vmul"(%95, %0, %27) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<4x16xf32>, f32, tensor<4x16xf32>) -> tensor<4x16xf32>
    %97 = "hivm.hir.vadd"(%96, %7, %26) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<4x16xf32>, f32, tensor<4x16xf32>) -> tensor<4x16xf32>
    %98 = "hivm.hir.vexp"(%97, %25) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<4x16xf32>, tensor<4x16xf32>) -> tensor<4x16xf32>
    %99 = "hivm.hir.vadd"(%98, %4, %24) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<4x16xf32>, f32, tensor<4x16xf32>) -> tensor<4x16xf32>
    %100 = "tensor.empty"() : () -> tensor<4x16xf32>
    %101 = "hivm.hir.vbrc"(%4, %100) <{broadcast_dims = array<i64>}> : (f32, tensor<4x16xf32>) -> tensor<4x16xf32>
    %102 = "hivm.hir.vdiv"(%101, %99, %23) <{broadcast = array<i64>, isHP = false, isSigned = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<4x16xf32>, tensor<4x16xf32>, tensor<4x16xf32>) -> tensor<4x16xf32>
    %103 = "memref.reinterpret_cast"(%arg4, %51, %49) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 4, 16>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<4x16xbf16, strided<[?, 1], offset: ?>>
    %104 = "hivm.hir.vcast"(%102, %90) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<4x16xf32>, tensor<4x16xbf16>) -> tensor<4x16xbf16>
    %105 = "tensor.extract_slice"(%104, %58) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (tensor<4x16xbf16>, index) -> tensor<?x16xbf16>
    %106 = "memref.subview"(%103, %58) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (memref<4x16xbf16, strided<[?, 1], offset: ?>>, index) -> memref<?x16xbf16, strided<[?, 1], offset: ?>>
    "hivm.hir.store"(%105, %106) : (tensor<?x16xbf16>, memref<?x16xbf16, strided<[?, 1], offset: ?>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, true, true, true, false, false, false, false]> : vector<13xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hacc.target = #hacc.target<"Ascend910B1">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

