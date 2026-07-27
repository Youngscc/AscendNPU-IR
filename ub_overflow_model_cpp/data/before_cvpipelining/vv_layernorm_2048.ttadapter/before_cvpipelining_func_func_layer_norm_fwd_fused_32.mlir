#map = affine_map<()[s0, s1] -> (s0 + s1)>
#map1 = affine_map<()[s0] -> (s0 + 2048)>
#map2 = affine_map<()[s0, s1] -> (s0, s1)>
#map3 = affine_map<()[s0, s1] -> (s0 - s1)>
"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xf32>, memref<?xf32>, memref<?xf32>, memref<?xf32>, memref<?xf32>, memref<?xf32>, i32, i32, f32, i32, i32, i32) -> (), sym_name = "_layer_norm_fwd_fused"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xf32>, %arg4: memref<?xf32>, %arg5: memref<?xf32>, %arg6: memref<?xf32>, %arg7: memref<?xf32>, %arg8: memref<?xf32>, %arg9: i32, %arg10: i32, %arg11: f32, %arg12: i32, %arg13: i32, %arg14: i32):
    %0 = "arith.constant"() <{value = 2048 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %2 = "arith.constant"() <{value = 1.000000e+00 : f32}> : () -> f32
    %3 = "arith.constant"() <{value = 0 : index}> : () -> index
    %4 = "arith.constant"() <{value = 2048 : index}> : () -> index
    %5 = "arith.constant"() <{value = 0.000000e+00 : f32}> : () -> f32
    "hivm.hir.set_mask_norm"() : () -> ()
    %6 = "arith.muli"(%arg12, %arg13) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %7 = "arith.muli"(%6, %arg14) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%7) <{effects = ["write"]}> {logical_block_num} : (i32) -> ()
    %8 = "hivm.hir.get_block_idx"() : () -> i64
    %9 = "arith.trunci"(%8) : (i64) -> i32
    %10 = "arith.muli"(%arg14, %arg13) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %11 = "arith.divsi"(%9, %10) : (i32, i32) -> i32
    %12 = "arith.remsi"(%11, %arg12) : (i32, i32) -> i32
    %13 = "tensor.empty"() : () -> tensor<2048xf32>
    %14 = "hivm.hir.vbrc"(%5, %13) <{broadcast_dims = array<i64>}> : (f32, tensor<2048xf32>) -> tensor<2048xf32>
    %15 = "arith.muli"(%12, %arg9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %16 = "scf.for"(%1, %arg10, %0, %14) ({
    ^bb0(%arg18: i32, %arg19: tensor<2048xf32>):
      %91 = "arith.index_cast"(%15) : (i32) -> index
      %92 = "arith.index_cast"(%arg18) : (i32) -> index
      %93 = "affine.apply"(%91, %92) <{map = #map}> : (index, index) -> index
      %94 = "memref.reinterpret_cast"(%arg3, %93) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 2048>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<2048xf32, strided<[1], offset: ?>>
      %95 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<2048xf32>
      %96 = "affine.apply"(%92) <{map = #map1}> : (index) -> index
      %97 = "arith.index_cast"(%arg10) : (i32) -> index
      %98 = "affine.max"(%92, %97) <{map = #map2}> : (index, index) -> index
      %99 = "affine.min"(%96, %98) <{map = #map2}> : (index, index) -> index
      %100 = "affine.apply"(%99, %92) <{map = #map3}> : (index, index) -> index
      %101 = "arith.cmpi"(%100, %4) <{predicate = 2 : i64}> : (index, index) -> i1
      %102 = "memref.subview"(%94, %100) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<2048xf32, strided<[1], offset: ?>>, index) -> memref<?xf32, strided<[1], offset: ?>>
      %103 = "memref.subview"(%95, %100) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<2048xf32>, index) -> memref<?xf32, strided<[1]>>
      "hivm.hir.load"(%102, %103, %5, %3, %101) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xf32, strided<[1], offset: ?>>, memref<?xf32, strided<[1]>>, f32, index, i1) -> ()
      %104 = "bufferization.to_tensor"(%95) <{restrict, writable}> : (memref<2048xf32>) -> tensor<2048xf32>
      %105 = "hivm.hir.vadd"(%arg19, %104, %13) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2048xf32>, tensor<2048xf32>, tensor<2048xf32>) -> tensor<2048xf32>
      "scf.yield"(%105) : (tensor<2048xf32>) -> ()
    }) : (i32, i32, i32, tensor<2048xf32>) -> tensor<2048xf32>
    %17 = "bufferization.alloc_tensor"() <{operandSegmentSizes = array<i32: 0, 0, 0>}> : () -> tensor<f32>
    %18 = "tensor.empty"() : () -> tensor<1xf32>
    %19 = "hivm.hir.vreduce"(%16, %18) <{arith = #hivm.reduce_op<sum>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 0>, tie_break_left = true, unsigned_src = false}> : (tensor<2048xf32>, tensor<1xf32>) -> tensor<1xf32>
    %20 = "tensor.extract"(%19, %3) : (tensor<1xf32>, index) -> f32
    %21 = "arith.sitofp"(%arg10) : (i32) -> f32
    %22 = "arith.divf"(%20, %21) <{fastmath = #arith.fastmath<none>}> : (f32, f32) -> f32
    %23 = "scf.for"(%1, %arg10, %0, %14) ({
    ^bb0(%arg16: i32, %arg17: tensor<2048xf32>):
      %72 = "arith.index_cast"(%15) : (i32) -> index
      %73 = "arith.index_cast"(%arg16) : (i32) -> index
      %74 = "affine.apply"(%72, %73) <{map = #map}> : (index, index) -> index
      %75 = "memref.reinterpret_cast"(%arg3, %74) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 2048>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<2048xf32, strided<[1], offset: ?>>
      %76 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<2048xf32>
      %77 = "affine.apply"(%73) <{map = #map1}> : (index) -> index
      %78 = "arith.index_cast"(%arg10) : (i32) -> index
      %79 = "affine.max"(%73, %78) <{map = #map2}> : (index, index) -> index
      %80 = "affine.min"(%77, %79) <{map = #map2}> : (index, index) -> index
      %81 = "affine.apply"(%80, %73) <{map = #map3}> : (index, index) -> index
      %82 = "arith.cmpi"(%81, %4) <{predicate = 2 : i64}> : (index, index) -> i1
      %83 = "memref.subview"(%75, %81) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<2048xf32, strided<[1], offset: ?>>, index) -> memref<?xf32, strided<[1], offset: ?>>
      %84 = "memref.subview"(%76, %81) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<2048xf32>, index) -> memref<?xf32, strided<[1]>>
      "hivm.hir.load"(%83, %84, %5, %3, %82) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xf32, strided<[1], offset: ?>>, memref<?xf32, strided<[1]>>, f32, index, i1) -> ()
      %85 = "bufferization.to_tensor"(%76) <{restrict, writable}> : (memref<2048xf32>) -> tensor<2048xf32>
      %86 = "hivm.hir.vsub"(%85, %22, %13) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2048xf32>, f32, tensor<2048xf32>) -> tensor<2048xf32>
      %87 = "tensor.extract_slice"(%86, %81) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<2048xf32>, index) -> tensor<?xf32>
      %88 = "tensor.insert_slice"(%87, %14, %81) <{operandSegmentSizes = array<i32: 1, 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<?xf32>, tensor<2048xf32>, index) -> tensor<2048xf32>
      %89 = "hivm.hir.vmul"(%88, %88, %13) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2048xf32>, tensor<2048xf32>, tensor<2048xf32>) -> tensor<2048xf32>
      %90 = "hivm.hir.vadd"(%arg17, %89, %13) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2048xf32>, tensor<2048xf32>, tensor<2048xf32>) -> tensor<2048xf32>
      "scf.yield"(%90) : (tensor<2048xf32>) -> ()
    }) : (i32, i32, i32, tensor<2048xf32>) -> tensor<2048xf32>
    %24 = "bufferization.alloc_tensor"() <{operandSegmentSizes = array<i32: 0, 0, 0>}> : () -> tensor<f32>
    %25 = "hivm.hir.vreduce"(%23, %18) <{arith = #hivm.reduce_op<sum>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 0>, tie_break_left = true, unsigned_src = false}> : (tensor<2048xf32>, tensor<1xf32>) -> tensor<1xf32>
    %26 = "tensor.extract"(%25, %3) : (tensor<1xf32>, index) -> f32
    %27 = "arith.divf"(%26, %21) <{fastmath = #arith.fastmath<none>}> : (f32, f32) -> f32
    %28 = "tensor.insert"(%27, %18, %3) : (f32, tensor<1xf32>, index) -> tensor<1xf32>
    %29 = "tensor.insert"(%arg11, %18, %3) : (f32, tensor<1xf32>, index) -> tensor<1xf32>
    %30 = "hivm.hir.vadd"(%28, %29, %18) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1xf32>, tensor<1xf32>, tensor<1xf32>) -> tensor<1xf32>
    %31 = "tensor.extract"(%30, %3) : (tensor<1xf32>, index) -> f32
    %32 = "tensor.insert"(%31, %18, %3) : (f32, tensor<1xf32>, index) -> tensor<1xf32>
    %33 = "hivm.hir.vsqrt"(%32, %18) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<1xf32>, tensor<1xf32>) -> tensor<1xf32>
    %34 = "tensor.extract"(%33, %3) : (tensor<1xf32>, index) -> f32
    %35 = "arith.divf"(%2, %34) <{fastmath = #arith.fastmath<none>}> : (f32, f32) -> f32
    %36 = "tensor.insert"(%22, %18, %3) : (f32, tensor<1xf32>, index) -> tensor<1xf32>
    %37 = "arith.index_cast"(%12) : (i32) -> index
    %38 = "memref.reinterpret_cast"(%arg7, %37) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<1xf32, strided<[1], offset: ?>>
    "hivm.hir.store"(%36, %38) : (tensor<1xf32>, memref<1xf32, strided<[1], offset: ?>>) -> ()
    %39 = "tensor.insert"(%35, %18, %3) : (f32, tensor<1xf32>, index) -> tensor<1xf32>
    %40 = "memref.reinterpret_cast"(%arg8, %37) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<1xf32, strided<[1], offset: ?>>
    "hivm.hir.store"(%39, %40) : (tensor<1xf32>, memref<1xf32, strided<[1], offset: ?>>) -> ()
    "scf.for"(%1, %arg10, %0) ({
    ^bb0(%arg15: i32):
      %41 = "arith.index_cast"(%arg15) : (i32) -> index
      %42 = "memref.reinterpret_cast"(%arg5, %41) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 2048>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<2048xf32, strided<[1], offset: ?>>
      %43 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<2048xf32>
      %44 = "affine.apply"(%41) <{map = #map1}> : (index) -> index
      %45 = "arith.index_cast"(%arg10) : (i32) -> index
      %46 = "affine.max"(%41, %45) <{map = #map2}> : (index, index) -> index
      %47 = "affine.min"(%44, %46) <{map = #map2}> : (index, index) -> index
      %48 = "affine.apply"(%47, %41) <{map = #map3}> : (index, index) -> index
      %49 = "arith.cmpi"(%48, %4) <{predicate = 2 : i64}> : (index, index) -> i1
      %50 = "memref.subview"(%42, %48) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<2048xf32, strided<[1], offset: ?>>, index) -> memref<?xf32, strided<[1], offset: ?>>
      %51 = "memref.subview"(%43, %48) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<2048xf32>, index) -> memref<?xf32, strided<[1]>>
      "hivm.hir.load"(%50, %51, %5, %3, %49) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xf32, strided<[1], offset: ?>>, memref<?xf32, strided<[1]>>, f32, index, i1) -> ()
      %52 = "bufferization.to_tensor"(%43) <{restrict, writable}> : (memref<2048xf32>) -> tensor<2048xf32>
      %53 = "memref.reinterpret_cast"(%arg6, %41) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 2048>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<2048xf32, strided<[1], offset: ?>>
      %54 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<2048xf32>
      %55 = "memref.subview"(%53, %48) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<2048xf32, strided<[1], offset: ?>>, index) -> memref<?xf32, strided<[1], offset: ?>>
      %56 = "memref.subview"(%54, %48) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<2048xf32>, index) -> memref<?xf32, strided<[1]>>
      "hivm.hir.load"(%55, %56, %5, %3, %49) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xf32, strided<[1], offset: ?>>, memref<?xf32, strided<[1]>>, f32, index, i1) -> ()
      %57 = "bufferization.to_tensor"(%54) <{restrict, writable}> : (memref<2048xf32>) -> tensor<2048xf32>
      %58 = "arith.index_cast"(%15) : (i32) -> index
      %59 = "affine.apply"(%58, %41) <{map = #map}> : (index, index) -> index
      %60 = "memref.reinterpret_cast"(%arg3, %59) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 2048>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<2048xf32, strided<[1], offset: ?>>
      %61 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<2048xf32>
      %62 = "memref.subview"(%60, %48) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<2048xf32, strided<[1], offset: ?>>, index) -> memref<?xf32, strided<[1], offset: ?>>
      %63 = "memref.subview"(%61, %48) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<2048xf32>, index) -> memref<?xf32, strided<[1]>>
      "hivm.hir.load"(%62, %63, %5, %3, %49) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xf32, strided<[1], offset: ?>>, memref<?xf32, strided<[1]>>, f32, index, i1) -> ()
      %64 = "bufferization.to_tensor"(%61) <{restrict, writable}> : (memref<2048xf32>) -> tensor<2048xf32>
      %65 = "hivm.hir.vsub"(%64, %22, %13) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2048xf32>, f32, tensor<2048xf32>) -> tensor<2048xf32>
      %66 = "hivm.hir.vmul"(%65, %35, %13) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2048xf32>, f32, tensor<2048xf32>) -> tensor<2048xf32>
      %67 = "hivm.hir.vmul"(%66, %52, %13) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2048xf32>, tensor<2048xf32>, tensor<2048xf32>) -> tensor<2048xf32>
      %68 = "hivm.hir.vadd"(%67, %57, %13) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2048xf32>, tensor<2048xf32>, tensor<2048xf32>) -> tensor<2048xf32>
      %69 = "memref.reinterpret_cast"(%arg4, %59) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 2048>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<2048xf32, strided<[1], offset: ?>>
      %70 = "tensor.extract_slice"(%68, %48) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<2048xf32>, index) -> tensor<?xf32>
      %71 = "memref.subview"(%69, %48) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<2048xf32, strided<[1], offset: ?>>, index) -> memref<?xf32, strided<[1], offset: ?>>
      "hivm.hir.store"(%70, %71) : (tensor<?xf32>, memref<?xf32, strided<[1], offset: ?>>) -> ()
      "scf.yield"() : () -> ()
    }) : (i32, i32, i32) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, true, true, true, false, false, false, false, false, false]> : vector<15xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hacc.target = #hacc.target<"Ascend910B1">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

