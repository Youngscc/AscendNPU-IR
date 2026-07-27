#map = affine_map<()[s0, s1] -> (s0 + s1)>
#map1 = affine_map<()[s0] -> (s0 + 8192)>
#map2 = affine_map<()[s0, s1] -> (s0, s1)>
#map3 = affine_map<()[s0, s1] -> (s0 - s1)>
"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xf16>, memref<?xf16>, memref<?xf16>, memref<?xf16>, memref<?xf32>, memref<?xf32>, i32, i32, f32, i32, i32, i32) -> (), sym_name = "_layer_norm_fwd_fused"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xf16>, %arg4: memref<?xf16>, %arg5: memref<?xf16>, %arg6: memref<?xf16>, %arg7: memref<?xf32>, %arg8: memref<?xf32>, %arg9: i32, %arg10: i32, %arg11: f32, %arg12: i32, %arg13: i32, %arg14: i32):
    %0 = "arith.constant"() <{value = 0.000000e+00 : f16}> : () -> f16
    %1 = "arith.constant"() <{value = 8192 : i32}> : () -> i32
    %2 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %3 = "arith.constant"() <{value = 1.000000e+00 : f32}> : () -> f32
    %4 = "arith.constant"() <{value = 0 : index}> : () -> index
    %5 = "arith.constant"() <{value = 8192 : index}> : () -> index
    %6 = "arith.constant"() <{value = 0.000000e+00 : f32}> : () -> f32
    "hivm.hir.set_mask_norm"() : () -> ()
    %7 = "arith.muli"(%arg12, %arg13) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %8 = "arith.muli"(%7, %arg14) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%8) <{effects = ["write"]}> {logical_block_num} : (i32) -> ()
    %9 = "hivm.hir.get_block_idx"() : () -> i64
    %10 = "arith.trunci"(%9) : (i64) -> i32
    %11 = "arith.muli"(%arg14, %arg13) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %12 = "arith.divsi"(%10, %11) : (i32, i32) -> i32
    %13 = "arith.remsi"(%12, %arg12) : (i32, i32) -> i32
    %14 = "tensor.empty"() : () -> tensor<8192xf32>
    %15 = "hivm.hir.vbrc"(%6, %14) <{broadcast_dims = array<i64>}> : (f32, tensor<8192xf32>) -> tensor<8192xf32>
    %16 = "arith.muli"(%13, %arg9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %17 = "arith.index_cast"(%16) : (i32) -> index
    %18 = "scf.for"(%2, %arg10, %1, %15) ({
    ^bb0(%arg18: i32, %arg19: tensor<8192xf32>):
      %97 = "arith.index_cast"(%arg18) : (i32) -> index
      %98 = "affine.apply"(%17, %97) <{map = #map}> : (index, index) -> index
      %99 = "memref.reinterpret_cast"(%arg3, %98) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 8192>, static_strides = array<i64: 1>}> : (memref<?xf16>, index) -> memref<8192xf16, strided<[1], offset: ?>>
      %100 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<8192xf16>
      %101 = "affine.apply"(%97) <{map = #map1}> : (index) -> index
      %102 = "arith.index_cast"(%arg10) : (i32) -> index
      %103 = "affine.max"(%97, %102) <{map = #map2}> : (index, index) -> index
      %104 = "affine.min"(%101, %103) <{map = #map2}> : (index, index) -> index
      %105 = "affine.apply"(%104, %97) <{map = #map3}> : (index, index) -> index
      %106 = "arith.cmpi"(%105, %5) <{predicate = 2 : i64}> : (index, index) -> i1
      %107 = "memref.subview"(%99, %105) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<8192xf16, strided<[1], offset: ?>>, index) -> memref<?xf16, strided<[1], offset: ?>>
      %108 = "memref.subview"(%100, %105) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<8192xf16>, index) -> memref<?xf16, strided<[1]>>
      "hivm.hir.load"(%107, %108, %0, %4, %106) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xf16, strided<[1], offset: ?>>, memref<?xf16, strided<[1]>>, f16, index, i1) -> ()
      %109 = "bufferization.to_tensor"(%100) <{restrict, writable}> : (memref<8192xf16>) -> tensor<8192xf16>
      %110 = "hivm.hir.vcast"(%109, %14) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<8192xf16>, tensor<8192xf32>) -> tensor<8192xf32>
      %111 = "hivm.hir.vadd"(%arg19, %110, %14) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<8192xf32>, tensor<8192xf32>, tensor<8192xf32>) -> tensor<8192xf32>
      "scf.yield"(%111) : (tensor<8192xf32>) -> ()
    }) : (i32, i32, i32, tensor<8192xf32>) -> tensor<8192xf32>
    %19 = "bufferization.alloc_tensor"() <{operandSegmentSizes = array<i32: 0, 0, 0>}> : () -> tensor<f32>
    %20 = "tensor.empty"() : () -> tensor<1xf32>
    %21 = "hivm.hir.vreduce"(%18, %20) <{arith = #hivm.reduce_op<sum>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 0>, tie_break_left = true, unsigned_src = false}> : (tensor<8192xf32>, tensor<1xf32>) -> tensor<1xf32>
    %22 = "tensor.extract"(%21, %4) : (tensor<1xf32>, index) -> f32
    %23 = "arith.sitofp"(%arg10) : (i32) -> f32
    %24 = "arith.divf"(%22, %23) <{fastmath = #arith.fastmath<none>}> : (f32, f32) -> f32
    %25 = "scf.for"(%2, %arg10, %1, %15) ({
    ^bb0(%arg16: i32, %arg17: tensor<8192xf32>):
      %78 = "arith.index_cast"(%arg16) : (i32) -> index
      %79 = "affine.apply"(%17, %78) <{map = #map}> : (index, index) -> index
      %80 = "memref.reinterpret_cast"(%arg3, %79) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 8192>, static_strides = array<i64: 1>}> : (memref<?xf16>, index) -> memref<8192xf16, strided<[1], offset: ?>>
      %81 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<8192xf16>
      %82 = "affine.apply"(%78) <{map = #map1}> : (index) -> index
      %83 = "arith.index_cast"(%arg10) : (i32) -> index
      %84 = "affine.max"(%78, %83) <{map = #map2}> : (index, index) -> index
      %85 = "affine.min"(%82, %84) <{map = #map2}> : (index, index) -> index
      %86 = "affine.apply"(%85, %78) <{map = #map3}> : (index, index) -> index
      %87 = "arith.cmpi"(%86, %5) <{predicate = 2 : i64}> : (index, index) -> i1
      %88 = "memref.subview"(%80, %86) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<8192xf16, strided<[1], offset: ?>>, index) -> memref<?xf16, strided<[1], offset: ?>>
      %89 = "memref.subview"(%81, %86) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<8192xf16>, index) -> memref<?xf16, strided<[1]>>
      "hivm.hir.load"(%88, %89, %0, %4, %87) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xf16, strided<[1], offset: ?>>, memref<?xf16, strided<[1]>>, f16, index, i1) -> ()
      %90 = "bufferization.to_tensor"(%81) <{restrict, writable}> : (memref<8192xf16>) -> tensor<8192xf16>
      %91 = "hivm.hir.vcast"(%90, %14) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<8192xf16>, tensor<8192xf32>) -> tensor<8192xf32>
      %92 = "hivm.hir.vsub"(%91, %24, %14) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<8192xf32>, f32, tensor<8192xf32>) -> tensor<8192xf32>
      %93 = "tensor.extract_slice"(%92, %86) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<8192xf32>, index) -> tensor<?xf32>
      %94 = "tensor.insert_slice"(%93, %15, %86) <{operandSegmentSizes = array<i32: 1, 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<?xf32>, tensor<8192xf32>, index) -> tensor<8192xf32>
      %95 = "hivm.hir.vmul"(%94, %94, %14) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<8192xf32>, tensor<8192xf32>, tensor<8192xf32>) -> tensor<8192xf32>
      %96 = "hivm.hir.vadd"(%arg17, %95, %14) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<8192xf32>, tensor<8192xf32>, tensor<8192xf32>) -> tensor<8192xf32>
      "scf.yield"(%96) : (tensor<8192xf32>) -> ()
    }) : (i32, i32, i32, tensor<8192xf32>) -> tensor<8192xf32>
    %26 = "bufferization.alloc_tensor"() <{operandSegmentSizes = array<i32: 0, 0, 0>}> : () -> tensor<f32>
    %27 = "hivm.hir.vreduce"(%25, %20) <{arith = #hivm.reduce_op<sum>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 0>, tie_break_left = true, unsigned_src = false}> : (tensor<8192xf32>, tensor<1xf32>) -> tensor<1xf32>
    %28 = "tensor.extract"(%27, %4) : (tensor<1xf32>, index) -> f32
    %29 = "arith.divf"(%28, %23) <{fastmath = #arith.fastmath<none>}> : (f32, f32) -> f32
    %30 = "tensor.insert"(%29, %20, %4) : (f32, tensor<1xf32>, index) -> tensor<1xf32>
    %31 = "tensor.insert"(%arg11, %20, %4) : (f32, tensor<1xf32>, index) -> tensor<1xf32>
    %32 = "hivm.hir.vadd"(%30, %31, %20) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1xf32>, tensor<1xf32>, tensor<1xf32>) -> tensor<1xf32>
    %33 = "tensor.extract"(%32, %4) : (tensor<1xf32>, index) -> f32
    %34 = "tensor.insert"(%33, %20, %4) : (f32, tensor<1xf32>, index) -> tensor<1xf32>
    %35 = "hivm.hir.vsqrt"(%34, %20) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<1xf32>, tensor<1xf32>) -> tensor<1xf32>
    %36 = "tensor.extract"(%35, %4) : (tensor<1xf32>, index) -> f32
    %37 = "arith.divf"(%3, %36) <{fastmath = #arith.fastmath<none>}> : (f32, f32) -> f32
    %38 = "arith.index_cast"(%13) : (i32) -> index
    %39 = "tensor.insert"(%24, %20, %4) : (f32, tensor<1xf32>, index) -> tensor<1xf32>
    %40 = "memref.reinterpret_cast"(%arg7, %38) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<1xf32, strided<[1], offset: ?>>
    "hivm.hir.store"(%39, %40) : (tensor<1xf32>, memref<1xf32, strided<[1], offset: ?>>) -> ()
    %41 = "tensor.insert"(%37, %20, %4) : (f32, tensor<1xf32>, index) -> tensor<1xf32>
    %42 = "memref.reinterpret_cast"(%arg8, %38) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<1xf32, strided<[1], offset: ?>>
    "hivm.hir.store"(%41, %42) : (tensor<1xf32>, memref<1xf32, strided<[1], offset: ?>>) -> ()
    "scf.for"(%2, %arg10, %1) ({
    ^bb0(%arg15: i32):
      %43 = "arith.index_cast"(%arg15) : (i32) -> index
      %44 = "memref.reinterpret_cast"(%arg5, %43) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 8192>, static_strides = array<i64: 1>}> : (memref<?xf16>, index) -> memref<8192xf16, strided<[1], offset: ?>>
      %45 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<8192xf16>
      %46 = "affine.apply"(%43) <{map = #map1}> : (index) -> index
      %47 = "arith.index_cast"(%arg10) : (i32) -> index
      %48 = "affine.max"(%43, %47) <{map = #map2}> : (index, index) -> index
      %49 = "affine.min"(%46, %48) <{map = #map2}> : (index, index) -> index
      %50 = "affine.apply"(%49, %43) <{map = #map3}> : (index, index) -> index
      %51 = "arith.cmpi"(%50, %5) <{predicate = 2 : i64}> : (index, index) -> i1
      %52 = "memref.subview"(%44, %50) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<8192xf16, strided<[1], offset: ?>>, index) -> memref<?xf16, strided<[1], offset: ?>>
      %53 = "memref.subview"(%45, %50) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<8192xf16>, index) -> memref<?xf16, strided<[1]>>
      "hivm.hir.load"(%52, %53, %0, %4, %51) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xf16, strided<[1], offset: ?>>, memref<?xf16, strided<[1]>>, f16, index, i1) -> ()
      %54 = "bufferization.to_tensor"(%45) <{restrict, writable}> : (memref<8192xf16>) -> tensor<8192xf16>
      %55 = "memref.reinterpret_cast"(%arg6, %43) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 8192>, static_strides = array<i64: 1>}> : (memref<?xf16>, index) -> memref<8192xf16, strided<[1], offset: ?>>
      %56 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<8192xf16>
      %57 = "memref.subview"(%55, %50) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<8192xf16, strided<[1], offset: ?>>, index) -> memref<?xf16, strided<[1], offset: ?>>
      %58 = "memref.subview"(%56, %50) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<8192xf16>, index) -> memref<?xf16, strided<[1]>>
      "hivm.hir.load"(%57, %58, %0, %4, %51) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xf16, strided<[1], offset: ?>>, memref<?xf16, strided<[1]>>, f16, index, i1) -> ()
      %59 = "bufferization.to_tensor"(%56) <{restrict, writable}> : (memref<8192xf16>) -> tensor<8192xf16>
      %60 = "affine.apply"(%17, %43) <{map = #map}> : (index, index) -> index
      %61 = "memref.reinterpret_cast"(%arg3, %60) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 8192>, static_strides = array<i64: 1>}> : (memref<?xf16>, index) -> memref<8192xf16, strided<[1], offset: ?>>
      %62 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<8192xf16>
      %63 = "memref.subview"(%61, %50) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<8192xf16, strided<[1], offset: ?>>, index) -> memref<?xf16, strided<[1], offset: ?>>
      %64 = "memref.subview"(%62, %50) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<8192xf16>, index) -> memref<?xf16, strided<[1]>>
      "hivm.hir.load"(%63, %64, %0, %4, %51) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xf16, strided<[1], offset: ?>>, memref<?xf16, strided<[1]>>, f16, index, i1) -> ()
      %65 = "bufferization.to_tensor"(%62) <{restrict, writable}> : (memref<8192xf16>) -> tensor<8192xf16>
      %66 = "hivm.hir.vcast"(%65, %14) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<8192xf16>, tensor<8192xf32>) -> tensor<8192xf32>
      %67 = "hivm.hir.vsub"(%66, %24, %14) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<8192xf32>, f32, tensor<8192xf32>) -> tensor<8192xf32>
      %68 = "hivm.hir.vmul"(%67, %37, %14) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<8192xf32>, f32, tensor<8192xf32>) -> tensor<8192xf32>
      %69 = "hivm.hir.vcast"(%54, %14) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<8192xf16>, tensor<8192xf32>) -> tensor<8192xf32>
      %70 = "hivm.hir.vmul"(%68, %69, %14) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<8192xf32>, tensor<8192xf32>, tensor<8192xf32>) -> tensor<8192xf32>
      %71 = "hivm.hir.vcast"(%59, %14) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<8192xf16>, tensor<8192xf32>) -> tensor<8192xf32>
      %72 = "hivm.hir.vadd"(%70, %71, %14) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<8192xf32>, tensor<8192xf32>, tensor<8192xf32>) -> tensor<8192xf32>
      %73 = "memref.reinterpret_cast"(%arg4, %60) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 8192>, static_strides = array<i64: 1>}> : (memref<?xf16>, index) -> memref<8192xf16, strided<[1], offset: ?>>
      %74 = "tensor.empty"() : () -> tensor<8192xf16>
      %75 = "hivm.hir.vcast"(%72, %74) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<8192xf32>, tensor<8192xf16>) -> tensor<8192xf16>
      %76 = "tensor.extract_slice"(%75, %50) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<8192xf16>, index) -> tensor<?xf16>
      %77 = "memref.subview"(%73, %50) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<8192xf16, strided<[1], offset: ?>>, index) -> memref<?xf16, strided<[1], offset: ?>>
      "hivm.hir.store"(%76, %77) : (tensor<?xf16>, memref<?xf16, strided<[1], offset: ?>>) -> ()
      "scf.yield"() : () -> ()
    }) : (i32, i32, i32) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, true, true, true, false, false, false, false, false, false]> : vector<15xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hacc.target = #hacc.target<"Ascend910B1">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

