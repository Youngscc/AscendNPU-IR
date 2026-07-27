#map = affine_map<()[s0] -> (s0 + 1)>
#map1 = affine_map<()[s0] -> (s0 * 64)>
#map2 = affine_map<()[s0, s1] -> (s0 + s1)>
#map3 = affine_map<()[s0] -> (s0 + 16)>
#map4 = affine_map<()[s0, s1] -> (s0, s1)>
#map5 = affine_map<()[s0, s1] -> (s0 - s1)>
#map6 = affine_map<()[s0] -> (s0 * 256)>
#map7 = affine_map<()[s0] -> (s0, 16)>
"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, memref<?xf32>, memref<?xi64>, memref<?xi64>, i32, i32, i32, i32) -> (), sym_name = "recompute_w_u_fwd_kernel"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xbf16>, %arg4: memref<?xbf16>, %arg5: memref<?xbf16>, %arg6: memref<?xbf16>, %arg7: memref<?xbf16>, %arg8: memref<?xbf16>, %arg9: memref<?xf32>, %arg10: memref<?xi64>, %arg11: memref<?xi64>, %arg12: i32, %arg13: i32, %arg14: i32, %arg15: i32):
    %0 = "arith.constant"() <{value = 1 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = true}> : () -> i1
    %2 = "arith.constant"() <{value = 0.000000e+00 : bf16}> : () -> bf16
    %3 = "arith.constant"() <{value = 16 : index}> : () -> index
    %4 = "arith.constant"() <{value = 64 : index}> : () -> index
    %5 = "arith.constant"() <{value = 0 : index}> : () -> index
    %6 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %7 = "arith.constant"() <{value = 4 : i32}> : () -> i32
    %8 = "arith.constant"() <{value = 2 : i32}> : () -> i32
    %9 = "arith.constant"() <{value = 16 : i32}> : () -> i32
    %10 = "arith.constant"() <{value = 64 : i32}> : () -> i32
    %11 = "arith.constant"() <{value = 0.000000e+00 : f32}> : () -> f32
    "hivm.hir.set_mask_norm"() : () -> ()
    %12 = "arith.muli"(%arg13, %arg14) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %13 = "arith.muli"(%12, %arg15) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%13) <{effects = ["write"]}> {logical_block_num} : (i32) -> ()
    %14 = "hivm.hir.get_block_idx"() : () -> i64
    %15 = "arith.trunci"(%14) : (i64) -> i32
    %16 = "arith.muli"(%arg15, %arg14) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %17 = "arith.divsi"(%15, %16) : (i32, i32) -> i32
    %18 = "arith.remsi"(%17, %arg13) : (i32, i32) -> i32
    %19 = "tensor.empty"() : () -> tensor<16x64xf32>
    %20 = "arith.muli"(%18, %8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %21 = "arith.index_cast"(%20) : (i32) -> index
    %22 = "memref.reinterpret_cast"(%arg11, %21) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi64>, index) -> memref<1xi64, strided<[1], offset: ?>>
    %23 = "affine.apply"(%21) <{map = #map}> : (index) -> index
    %24 = "memref.reinterpret_cast"(%arg11, %23) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi64>, index) -> memref<1xi64, strided<[1], offset: ?>>
    "scf.for"(%6, %7, %0) ({
    ^bb0(%arg16: i32):
      %25 = "arith.remsi"(%arg16, %7) : (i32, i32) -> i32
      %26 = "memref.load"(%22, %5) : (memref<1xi64, strided<[1], offset: ?>>, index) -> i64
      %27 = "arith.trunci"(%26) : (i64) -> i32
      %28 = "memref.load"(%24, %5) : (memref<1xi64, strided<[1], offset: ?>>, index) -> i64
      %29 = "arith.trunci"(%28) : (i64) -> i32
      %30 = "arith.index_cast"(%27) : (i32) -> index
      %31 = "memref.reinterpret_cast"(%arg10, %30) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi64>, index) -> memref<1xi64, strided<[1], offset: ?>>
      %32 = "memref.load"(%31, %5) : (memref<1xi64, strided<[1], offset: ?>>, index) -> i64
      %33 = "arith.trunci"(%32) : (i64) -> i32
      %34 = "affine.apply"(%30) <{map = #map}> : (index) -> index
      %35 = "memref.reinterpret_cast"(%arg10, %34) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi64>, index) -> memref<1xi64, strided<[1], offset: ?>>
      %36 = "memref.load"(%35, %5) : (memref<1xi64, strided<[1], offset: ?>>, index) -> i64
      %37 = "arith.trunci"(%36) : (i64) -> i32
      %38 = "arith.subi"(%37, %33) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %39 = "arith.muli"(%29, %9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %40 = "arith.muli"(%33, %7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %41 = "arith.addi"(%40, %25) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %42 = "arith.muli"(%41, %9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %43 = "arith.index_cast"(%42) : (i32) -> index
      %44 = "arith.index_cast"(%39) : (i32) -> index
      %45 = "affine.apply"(%44) <{map = #map1}> : (index) -> index
      %46 = "affine.apply"(%43, %45) <{map = #map2}> : (index, index) -> index
      %47 = "memref.reinterpret_cast"(%arg8, %46) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 64, 1>}> : (memref<?xbf16>, index) -> memref<16x16xbf16, strided<[64, 1], offset: ?>>
      %48 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x16xbf16>
      %49 = "affine.apply"(%44) <{map = #map3}> : (index) -> index
      %50 = "arith.index_cast"(%38) : (i32) -> index
      %51 = "affine.max"(%44, %50) <{map = #map4}> : (index, index) -> index
      %52 = "affine.min"(%49, %51) <{map = #map4}> : (index, index) -> index
      %53 = "affine.apply"(%52, %44) <{map = #map5}> : (index, index) -> index
      %54 = "arith.cmpi"(%53, %3) <{predicate = 2 : i64}> : (index, index) -> i1
      %55 = "memref.subview"(%47, %53) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16, strided<[64, 1], offset: ?>>, index) -> memref<?x16xbf16, strided<[64, 1], offset: ?>>
      %56 = "memref.subview"(%48, %53) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16>, index) -> memref<?x16xbf16, strided<[16, 1]>>
      "hivm.hir.load"(%55, %56, %2, %5, %54) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x16xbf16, strided<[64, 1], offset: ?>>, memref<?x16xbf16, strided<[16, 1]>>, bf16, index, i1) -> ()
      %57 = "bufferization.to_tensor"(%48) <{restrict, writable}> : (memref<16x16xbf16>) -> tensor<16x16xbf16>
      %58 = "tensor.empty"() : () -> tensor<16x16xf32>
      %59 = "hivm.hir.vcast"(%57, %58) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x16xbf16>, tensor<16x16xf32>) -> tensor<16x16xf32>
      %60 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xf32>
      %61 = "bufferization.to_tensor"(%60) <{restrict, writable}> : (memref<16x16xf32>) -> tensor<16x16xf32>
      %62 = "hivm.hir.store"(%59, %61) {"inserted-store"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
      %63 = "hivm.hir.load"(%62, %58) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
      %64 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xf32>
      %65 = "bufferization.to_tensor"(%64) <{restrict, writable}> : (memref<16x16xf32>) -> tensor<16x16xf32>
      %66 = "hivm.hir.store"(%59, %65) {"inserted-store"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
      %67 = "hivm.hir.load"(%66, %58) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
      %68 = "arith.index_cast"(%33) : (i32) -> index
      %69 = "arith.muli"(%25, %arg12) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %70 = "arith.index_cast"(%69) : (i32) -> index
      %71 = "affine.apply"(%68, %70) <{map = #map2}> : (index, index) -> index
      %72 = "affine.apply"(%71, %44) <{map = #map2}> : (index, index) -> index
      %73 = "memref.reinterpret_cast"(%arg9, %72) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<16xf32, strided<[1], offset: ?>>
      %74 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16xf32>
      %75 = "memref.subview"(%73, %53) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<16xf32, strided<[1], offset: ?>>, index) -> memref<?xf32, strided<[1], offset: ?>>
      %76 = "memref.subview"(%74, %53) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<16xf32>, index) -> memref<?xf32, strided<[1]>>
      "hivm.hir.load"(%75, %76, %11, %5, %54) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xf32, strided<[1], offset: ?>>, memref<?xf32, strided<[1]>>, f32, index, i1) -> ()
      %77 = "bufferization.to_tensor"(%74) <{restrict, writable}> : (memref<16xf32>) -> tensor<16xf32>
      %78 = "tensor.empty"() : () -> tensor<16xf32>
      %79 = "hivm.hir.vexp"(%77, %78) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
      %80 = "memref.reinterpret_cast"(%arg5, %72) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16>, static_strides = array<i64: 1>}> : (memref<?xbf16>, index) -> memref<16xbf16, strided<[1], offset: ?>>
      %81 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16xbf16>
      %82 = "memref.subview"(%80, %53) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<16xbf16, strided<[1], offset: ?>>, index) -> memref<?xbf16, strided<[1], offset: ?>>
      %83 = "memref.subview"(%81, %53) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<16xbf16>, index) -> memref<?xbf16, strided<[1]>>
      "hivm.hir.load"(%82, %83, %2, %5, %54) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xbf16, strided<[1], offset: ?>>, memref<?xbf16, strided<[1]>>, bf16, index, i1) -> ()
      %84 = "bufferization.to_tensor"(%81) <{restrict, writable}> : (memref<16xbf16>) -> tensor<16xbf16>
      %85 = "hivm.hir.vcast"(%84, %78) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16xbf16>, tensor<16xf32>) -> tensor<16xf32>
      %86 = "arith.muli"(%41, %10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %87 = "arith.index_cast"(%86) : (i32) -> index
      %88 = "affine.apply"(%44) <{map = #map6}> : (index) -> index
      %89 = "affine.apply"(%87, %88) <{map = #map2}> : (index, index) -> index
      %90 = "memref.reinterpret_cast"(%arg4, %89) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 64>, static_strides = array<i64: 256, 1>}> : (memref<?xbf16>, index) -> memref<16x64xbf16, strided<[256, 1], offset: ?>>
      %91 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x64xbf16>
      %92 = "affine.min"(%53) <{map = #map7}> : (index) -> index
      %93 = "arith.cmpi"(%92, %3) <{predicate = 2 : i64}> : (index, index) -> i1
      %94 = "memref.subview"(%90, %92) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 64>, static_strides = array<i64: 1, 1>}> : (memref<16x64xbf16, strided<[256, 1], offset: ?>>, index) -> memref<?x64xbf16, strided<[256, 1], offset: ?>>
      %95 = "memref.subview"(%91, %92) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 64>, static_strides = array<i64: 1, 1>}> : (memref<16x64xbf16>, index) -> memref<?x64xbf16, strided<[64, 1]>>
      "hivm.hir.load"(%94, %95, %2, %5, %93) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x64xbf16, strided<[256, 1], offset: ?>>, memref<?x64xbf16, strided<[64, 1]>>, bf16, index, i1) -> ()
      %96 = "bufferization.to_tensor"(%91) <{restrict, writable}> : (memref<16x64xbf16>) -> tensor<16x64xbf16>
      %97 = "hivm.hir.vcast"(%96, %19) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x64xbf16>, tensor<16x64xf32>) -> tensor<16x64xf32>
      %98 = "tensor.expand_shape"(%85) <{reassociation = [[0, 1]], static_output_shape = array<i64: 16, 1>}> : (tensor<16xf32>) -> tensor<16x1xf32>
      %99 = "hivm.hir.vbrc"(%98, %19) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xf32>, tensor<16x64xf32>) -> tensor<16x64xf32>
      %100 = "hivm.hir.vmul"(%97, %99, %19) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x64xf32>, tensor<16x64xf32>, tensor<16x64xf32>) -> tensor<16x64xf32>
      %101 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x64xf32>
      %102 = "bufferization.to_tensor"(%101) <{restrict, writable}> : (memref<16x64xf32>) -> tensor<16x64xf32>
      %103 = "hivm.hir.store"(%100, %102) {"inserted-store"} : (tensor<16x64xf32>, tensor<16x64xf32>) -> tensor<16x64xf32>
      %104 = "hivm.hir.load"(%103, %19) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x64xf32>, tensor<16x64xf32>) -> tensor<16x64xf32>
      %105 = "hivm.hir.mmadL1"(%63, %104, %1, %3, %3, %4, %19) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_for_result_already_inserted = true} : (tensor<16x16xf32>, tensor<16x64xf32>, i1, index, index, index, tensor<16x64xf32>) -> tensor<16x64xf32>
      %106 = "memref.reinterpret_cast"(%arg7, %89) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 64>, static_strides = array<i64: 256, 1>}> : (memref<?xbf16>, index) -> memref<16x64xbf16, strided<[256, 1], offset: ?>>
      %107 = "tensor.extract_slice"(%105, %92) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 64>, static_strides = array<i64: 1, 1>}> : (tensor<16x64xf32>, index) -> tensor<?x64xf32>
      %108 = "memref.subview"(%106, %92) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 64>, static_strides = array<i64: 1, 1>}> : (memref<16x64xbf16, strided<[256, 1], offset: ?>>, index) -> memref<?x64xbf16, strided<[256, 1], offset: ?>>
      "hivm.hir.fixpipe"(%107, %108) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, pre_quant = #hivm.fixpipe_pre_quant_mode<F322BF16>}> : (tensor<?x64xf32>, memref<?x64xbf16, strided<[256, 1], offset: ?>>) -> ()
      %109 = "memref.reinterpret_cast"(%arg3, %89) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 64>, static_strides = array<i64: 256, 1>}> : (memref<?xbf16>, index) -> memref<16x64xbf16, strided<[256, 1], offset: ?>>
      %110 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x64xbf16>
      %111 = "memref.subview"(%109, %92) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 64>, static_strides = array<i64: 1, 1>}> : (memref<16x64xbf16, strided<[256, 1], offset: ?>>, index) -> memref<?x64xbf16, strided<[256, 1], offset: ?>>
      %112 = "memref.subview"(%110, %92) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 64>, static_strides = array<i64: 1, 1>}> : (memref<16x64xbf16>, index) -> memref<?x64xbf16, strided<[64, 1]>>
      "hivm.hir.load"(%111, %112, %2, %5, %93) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x64xbf16, strided<[256, 1], offset: ?>>, memref<?x64xbf16, strided<[64, 1]>>, bf16, index, i1) -> ()
      %113 = "bufferization.to_tensor"(%110) <{restrict, writable}> : (memref<16x64xbf16>) -> tensor<16x64xbf16>
      %114 = "hivm.hir.vcast"(%113, %19) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x64xbf16>, tensor<16x64xf32>) -> tensor<16x64xf32>
      %115 = "hivm.hir.vmul"(%114, %99, %19) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x64xf32>, tensor<16x64xf32>, tensor<16x64xf32>) -> tensor<16x64xf32>
      %116 = "tensor.expand_shape"(%79) <{reassociation = [[0, 1]], static_output_shape = array<i64: 16, 1>}> : (tensor<16xf32>) -> tensor<16x1xf32>
      %117 = "hivm.hir.vbrc"(%116, %19) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xf32>, tensor<16x64xf32>) -> tensor<16x64xf32>
      %118 = "hivm.hir.vmul"(%115, %117, %19) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x64xf32>, tensor<16x64xf32>, tensor<16x64xf32>) -> tensor<16x64xf32>
      %119 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x64xf32>
      %120 = "bufferization.to_tensor"(%119) <{restrict, writable}> : (memref<16x64xf32>) -> tensor<16x64xf32>
      %121 = "hivm.hir.store"(%118, %120) {"inserted-store"} : (tensor<16x64xf32>, tensor<16x64xf32>) -> tensor<16x64xf32>
      %122 = "hivm.hir.load"(%121, %19) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x64xf32>, tensor<16x64xf32>) -> tensor<16x64xf32>
      %123 = "hivm.hir.mmadL1"(%67, %122, %1, %3, %3, %4, %19) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_for_result_already_inserted = true} : (tensor<16x16xf32>, tensor<16x64xf32>, i1, index, index, index, tensor<16x64xf32>) -> tensor<16x64xf32>
      %124 = "memref.reinterpret_cast"(%arg6, %89) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 64>, static_strides = array<i64: 256, 1>}> : (memref<?xbf16>, index) -> memref<16x64xbf16, strided<[256, 1], offset: ?>>
      %125 = "tensor.extract_slice"(%123, %92) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 64>, static_strides = array<i64: 1, 1>}> : (tensor<16x64xf32>, index) -> tensor<?x64xf32>
      %126 = "memref.subview"(%124, %92) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 64>, static_strides = array<i64: 1, 1>}> : (memref<16x64xbf16, strided<[256, 1], offset: ?>>, index) -> memref<?x64xbf16, strided<[256, 1], offset: ?>>
      "hivm.hir.fixpipe"(%125, %126) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, pre_quant = #hivm.fixpipe_pre_quant_mode<F322BF16>}> : (tensor<?x64xf32>, memref<?x64xbf16, strided<[256, 1], offset: ?>>) -> ()
      "scf.yield"() : () -> ()
    }) {fixpipe_for_mmad_result_already_inserted = true} : (i32, i32, i32) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false]> : vector<16xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<MIX>, mix_mode = "mix", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hacc.target = #hacc.target<"Ascend910B1">, hivm.module_core_type = #hivm.module_core_type<MIX>} : () -> ()

