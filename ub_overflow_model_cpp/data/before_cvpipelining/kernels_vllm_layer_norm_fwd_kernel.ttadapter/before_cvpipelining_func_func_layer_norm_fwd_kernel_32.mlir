#map = affine_map<()[s0, s1] -> (s0 + s1)>
#map1 = affine_map<()[s0] -> (s0, 0)>
#map2 = affine_map<()[s0] -> (s0, 128)>
"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, memref<?xf32>, memref<?xf32>, i32, i32, i32, i32, i32, f32, i32, i32, i32) -> (), sym_name = "layer_norm_fwd_kernel"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xbf16>, %arg4: memref<?xbf16>, %arg5: memref<?xbf16>, %arg6: memref<?xbf16>, %arg7: memref<?xbf16>, %arg8: memref<?xf32>, %arg9: memref<?xf32>, %arg10: i32, %arg11: i32, %arg12: i32, %arg13: i32, %arg14: i32, %arg15: f32, %arg16: i32, %arg17: i32, %arg18: i32):
    %0 = "arith.constant"() <{value = 1 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = -1.000000e+00 : f32}> : () -> f32
    %2 = "arith.constant"() <{value = 0.000000e+00 : bf16}> : () -> bf16
    %3 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %4 = "arith.constant"() <{value = 1.000000e+00 : f32}> : () -> f32
    %5 = "arith.constant"() <{value = 128 : index}> : () -> index
    %6 = "arith.constant"() <{value = 0 : index}> : () -> index
    %7 = "arith.constant"() <{value = 0.000000e+00 : f32}> : () -> f32
    "hivm.hir.set_mask_norm"() : () -> ()
    %8 = "arith.muli"(%arg16, %arg17) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %9 = "arith.muli"(%8, %arg18) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%9) <{effects = ["write"]}> {logical_block_num} : (i32) -> ()
    %10 = "hivm.hir.get_block_idx"() : () -> i64
    %11 = "arith.trunci"(%10) : (i64) -> i32
    %12 = "arith.divsi"(%11, %arg18) : (i32, i32) -> i32
    %13 = "arith.remsi"(%12, %arg17) : (i32, i32) -> i32
    %14 = "arith.muli"(%arg18, %arg17) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %15 = "arith.divsi"(%11, %14) : (i32, i32) -> i32
    %16 = "arith.remsi"(%15, %arg16) : (i32, i32) -> i32
    %17 = "tensor.empty"() : () -> tensor<128xf32>
    %18 = "hivm.hir.vbrc"(%7, %17) <{broadcast_dims = array<i64>}> : (f32, tensor<128xf32>) -> tensor<128xf32>
    %19 = "arith.cmpi"(%arg13, %0) <{predicate = 2 : i64}> : (i32, i32) -> i1
    %20 = "arith.select"(%19, %arg13, %0) : (i1, i32, i32) -> i32
    %21 = "arith.divsi"(%arg13, %20) : (i32, i32) -> i32
    %22 = "arith.remsi"(%arg13, %20) : (i32, i32) -> i32
    %23 = "arith.cmpi"(%16, %22) <{predicate = 2 : i64}> : (i32, i32) -> i1
    %24 = "scf.if"(%23) ({
      %120 = "arith.addi"(%21, %0) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      "scf.yield"(%120) : (i32) -> ()
    }, {
      "scf.yield"(%21) : (i32) -> ()
    }) : (i1) -> i32
    %25 = "arith.muli"(%16, %arg10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %26 = "arith.muli"(%13, %arg14) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %27 = "arith.muli"(%16, %arg11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %28 = "arith.muli"(%16, %arg12) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %29 = "arith.muli"(%13, %arg13) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %30 = "arith.index_cast"(%26) : (i32) -> index
    %31 = "arith.sitofp"(%arg14) : (i32) -> f32
    %32 = "memref.reinterpret_cast"(%arg5, %30) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 128>, static_strides = array<i64: 1>}> : (memref<?xbf16>, index) -> memref<128xbf16, strided<[1], offset: ?>>
    %33 = "memref.reinterpret_cast"(%arg6, %30) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 128>, static_strides = array<i64: 1>}> : (memref<?xbf16>, index) -> memref<128xbf16, strided<[1], offset: ?>>
    "scf.for"(%3, %24, %0) ({
    ^bb0(%arg19: i32):
      %34 = "arith.muli"(%arg19, %20) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %35 = "arith.muli"(%34, %arg10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %36 = "arith.index_cast"(%35) : (i32) -> index
      %37 = "arith.index_cast"(%25) : (i32) -> index
      %38 = "affine.apply"(%36, %37) <{map = #map}> : (index, index) -> index
      %39 = "affine.apply"(%38, %30) <{map = #map}> : (index, index) -> index
      %40 = "arith.muli"(%34, %arg11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %41 = "arith.index_cast"(%40) : (i32) -> index
      %42 = "arith.index_cast"(%27) : (i32) -> index
      %43 = "affine.apply"(%41, %42) <{map = #map}> : (index, index) -> index
      %44 = "affine.apply"(%43, %30) <{map = #map}> : (index, index) -> index
      %45 = "arith.muli"(%34, %arg12) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %46 = "arith.index_cast"(%45) : (i32) -> index
      %47 = "arith.index_cast"(%28) : (i32) -> index
      %48 = "affine.apply"(%46, %47) <{map = #map}> : (index, index) -> index
      %49 = "affine.apply"(%48, %30) <{map = #map}> : (index, index) -> index
      %50 = "arith.index_cast"(%34) : (i32) -> index
      %51 = "arith.index_cast"(%29) : (i32) -> index
      %52 = "affine.apply"(%50, %51) <{map = #map}> : (index, index) -> index
      %53 = "memref.reinterpret_cast"(%arg3, %39) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 128>, static_strides = array<i64: 1>}> : (memref<?xbf16>, index) -> memref<128xbf16, strided<[1], offset: ?>>
      %54 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<128xbf16>
      %55 = "arith.index_cast"(%arg14) : (i32) -> index
      %56 = "affine.max"(%55) <{map = #map1}> : (index) -> index
      %57 = "affine.min"(%56) <{map = #map2}> : (index) -> index
      %58 = "arith.cmpi"(%57, %5) <{predicate = 2 : i64}> : (index, index) -> i1
      %59 = "memref.subview"(%53, %57) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<128xbf16, strided<[1], offset: ?>>, index) -> memref<?xbf16, strided<[1], offset: ?>>
      %60 = "memref.subview"(%54, %57) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<128xbf16>, index) -> memref<?xbf16, strided<[1]>>
      "hivm.hir.load"(%59, %60, %2, %6, %58) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xbf16, strided<[1], offset: ?>>, memref<?xbf16, strided<[1]>>, bf16, index, i1) -> ()
      %61 = "bufferization.to_tensor"(%54) <{restrict, writable}> : (memref<128xbf16>) -> tensor<128xbf16>
      %62 = "hivm.hir.vcast"(%61, %17) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<128xbf16>, tensor<128xf32>) -> tensor<128xf32>
      %63 = "bufferization.alloc_tensor"() <{operandSegmentSizes = array<i32: 0, 0, 0>}> : () -> tensor<f32>
      %64 = "tensor.empty"() : () -> tensor<1xf32>
      %65 = "hivm.hir.vreduce"(%62, %64) <{arith = #hivm.reduce_op<sum>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 0>, tie_break_left = true, unsigned_src = false}> : (tensor<128xf32>, tensor<1xf32>) -> tensor<1xf32>
      %66 = "tensor.extract"(%65, %6) : (tensor<1xf32>, index) -> f32
      %67 = "arith.divf"(%66, %31) <{fastmath = #arith.fastmath<none>}> : (f32, f32) -> f32
      %68 = "arith.index_cast"(%16) : (i32) -> index
      %69 = "affine.apply"(%52, %68) <{map = #map}> : (index, index) -> index
      %70 = "tensor.insert"(%67, %64, %6) : (f32, tensor<1xf32>, index) -> tensor<1xf32>
      %71 = "memref.reinterpret_cast"(%arg8, %69) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<1xf32, strided<[1], offset: ?>>
      "hivm.hir.store"(%70, %71) : (tensor<1xf32>, memref<1xf32, strided<[1], offset: ?>>) -> ()
      %72 = "hivm.hir.vsub"(%62, %67, %17) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, f32, tensor<128xf32>) -> tensor<128xf32>
      %73 = "tensor.extract_slice"(%72, %57) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<128xf32>, index) -> tensor<?xf32>
      %74 = "tensor.insert_slice"(%73, %18, %57) <{operandSegmentSizes = array<i32: 1, 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<?xf32>, tensor<128xf32>, index) -> tensor<128xf32>
      %75 = "hivm.hir.vmul"(%74, %74, %17) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, tensor<128xf32>, tensor<128xf32>) -> tensor<128xf32>
      %76 = "bufferization.alloc_tensor"() <{operandSegmentSizes = array<i32: 0, 0, 0>}> : () -> tensor<f32>
      %77 = "hivm.hir.vreduce"(%75, %64) <{arith = #hivm.reduce_op<sum>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 0>, tie_break_left = true, unsigned_src = false}> : (tensor<128xf32>, tensor<1xf32>) -> tensor<1xf32>
      %78 = "tensor.extract"(%77, %6) : (tensor<1xf32>, index) -> f32
      %79 = "arith.divf"(%78, %31) <{fastmath = #arith.fastmath<none>}> : (f32, f32) -> f32
      %80 = "tensor.insert"(%79, %64, %6) : (f32, tensor<1xf32>, index) -> tensor<1xf32>
      %81 = "tensor.insert"(%arg15, %64, %6) : (f32, tensor<1xf32>, index) -> tensor<1xf32>
      %82 = "hivm.hir.vadd"(%80, %81, %64) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1xf32>, tensor<1xf32>, tensor<1xf32>) -> tensor<1xf32>
      %83 = "tensor.extract"(%82, %6) : (tensor<1xf32>, index) -> f32
      %84 = "tensor.insert"(%83, %64, %6) : (f32, tensor<1xf32>, index) -> tensor<1xf32>
      %85 = "hivm.hir.vsqrt"(%84, %64) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<1xf32>, tensor<1xf32>) -> tensor<1xf32>
      %86 = "tensor.extract"(%85, %6) : (tensor<1xf32>, index) -> f32
      %87 = "arith.divf"(%4, %86) <{fastmath = #arith.fastmath<none>}> : (f32, f32) -> f32
      %88 = "tensor.insert"(%87, %64, %6) : (f32, tensor<1xf32>, index) -> tensor<1xf32>
      %89 = "memref.reinterpret_cast"(%arg9, %69) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<1xf32, strided<[1], offset: ?>>
      "hivm.hir.store"(%88, %89) : (tensor<1xf32>, memref<1xf32, strided<[1], offset: ?>>) -> ()
      %90 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<128xbf16>
      %91 = "memref.subview"(%32, %57) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<128xbf16, strided<[1], offset: ?>>, index) -> memref<?xbf16, strided<[1], offset: ?>>
      %92 = "memref.subview"(%90, %57) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<128xbf16>, index) -> memref<?xbf16, strided<[1]>>
      "hivm.hir.load"(%91, %92, %2, %6, %58) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xbf16, strided<[1], offset: ?>>, memref<?xbf16, strided<[1]>>, bf16, index, i1) -> ()
      %93 = "bufferization.to_tensor"(%90) <{restrict, writable}> : (memref<128xbf16>) -> tensor<128xbf16>
      %94 = "hivm.hir.vcast"(%93, %17) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<128xbf16>, tensor<128xf32>) -> tensor<128xf32>
      %95 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<128xbf16>
      %96 = "memref.subview"(%33, %57) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<128xbf16, strided<[1], offset: ?>>, index) -> memref<?xbf16, strided<[1], offset: ?>>
      %97 = "memref.subview"(%95, %57) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<128xbf16>, index) -> memref<?xbf16, strided<[1]>>
      "hivm.hir.load"(%96, %97, %2, %6, %58) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xbf16, strided<[1], offset: ?>>, memref<?xbf16, strided<[1]>>, bf16, index, i1) -> ()
      %98 = "bufferization.to_tensor"(%95) <{restrict, writable}> : (memref<128xbf16>) -> tensor<128xbf16>
      %99 = "hivm.hir.vcast"(%98, %17) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<128xbf16>, tensor<128xf32>) -> tensor<128xf32>
      %100 = "hivm.hir.vmul"(%72, %87, %17) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, f32, tensor<128xf32>) -> tensor<128xf32>
      %101 = "hivm.hir.vmul"(%100, %94, %17) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, tensor<128xf32>, tensor<128xf32>) -> tensor<128xf32>
      %102 = "hivm.hir.vadd"(%101, %99, %17) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, tensor<128xf32>, tensor<128xf32>) -> tensor<128xf32>
      %103 = "memref.reinterpret_cast"(%arg7, %49) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 128>, static_strides = array<i64: 1>}> : (memref<?xbf16>, index) -> memref<128xbf16, strided<[1], offset: ?>>
      %104 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<128xbf16>
      %105 = "memref.subview"(%103, %57) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<128xbf16, strided<[1], offset: ?>>, index) -> memref<?xbf16, strided<[1], offset: ?>>
      %106 = "memref.subview"(%104, %57) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<128xbf16>, index) -> memref<?xbf16, strided<[1]>>
      "hivm.hir.load"(%105, %106, %2, %6, %58) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xbf16, strided<[1], offset: ?>>, memref<?xbf16, strided<[1]>>, bf16, index, i1) -> ()
      %107 = "bufferization.to_tensor"(%104) <{restrict, writable}> : (memref<128xbf16>) -> tensor<128xbf16>
      %108 = "hivm.hir.vcast"(%107, %17) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<128xbf16>, tensor<128xf32>) -> tensor<128xf32>
      %109 = "hivm.hir.vmul"(%108, %1, %17) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, f32, tensor<128xf32>) -> tensor<128xf32>
      %110 = "hivm.hir.vadd"(%109, %7, %17) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, f32, tensor<128xf32>) -> tensor<128xf32>
      %111 = "hivm.hir.vexp"(%110, %17) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, tensor<128xf32>) -> tensor<128xf32>
      %112 = "hivm.hir.vadd"(%111, %4, %17) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, f32, tensor<128xf32>) -> tensor<128xf32>
      %113 = "hivm.hir.vdiv"(%108, %112, %17) <{broadcast = array<i64>, isHP = false, isSigned = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, tensor<128xf32>, tensor<128xf32>) -> tensor<128xf32>
      %114 = "hivm.hir.vmul"(%102, %113, %17) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, tensor<128xf32>, tensor<128xf32>) -> tensor<128xf32>
      %115 = "memref.reinterpret_cast"(%arg4, %44) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 128>, static_strides = array<i64: 1>}> : (memref<?xbf16>, index) -> memref<128xbf16, strided<[1], offset: ?>>
      %116 = "tensor.empty"() : () -> tensor<128xbf16>
      %117 = "hivm.hir.vcast"(%114, %116) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<128xf32>, tensor<128xbf16>) -> tensor<128xbf16>
      %118 = "tensor.extract_slice"(%117, %57) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<128xbf16>, index) -> tensor<?xbf16>
      %119 = "memref.subview"(%115, %57) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<128xbf16, strided<[1], offset: ?>>, index) -> memref<?xbf16, strided<[1], offset: ?>>
      "hivm.hir.store"(%118, %119) : (tensor<?xbf16>, memref<?xbf16, strided<[1], offset: ?>>) -> ()
      "scf.yield"() : () -> ()
    }) : (i32, i32, i32) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false]> : vector<19xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hacc.target = #hacc.target<"Ascend910B1">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

