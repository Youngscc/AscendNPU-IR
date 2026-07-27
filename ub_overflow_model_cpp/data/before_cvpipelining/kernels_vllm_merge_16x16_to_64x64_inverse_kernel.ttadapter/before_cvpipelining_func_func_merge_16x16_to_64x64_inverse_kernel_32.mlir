#map = affine_map<()[s0] -> (s0 * 64)>
#map1 = affine_map<()[s0, s1] -> (s0 + s1)>
#map2 = affine_map<()[s0] -> (s0 + 16)>
#map3 = affine_map<()[s0, s1] -> (s0, s1)>
#map4 = affine_map<()[s0, s1] -> (s0 - s1)>
#map5 = affine_map<()[s0] -> (s0, 16)>
#map6 = affine_map<()[s0] -> (s0 * 256)>
#map7 = affine_map<()[s0] -> (s0 + 32)>
#map8 = affine_map<()[s0] -> (s0, 32)>
"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, i32, i32, i32, i32) -> (), sym_name = "merge_16x16_to_64x64_inverse_kernel"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xbf16>, %arg4: memref<?xbf16>, %arg5: memref<?xbf16>, %arg6: i32, %arg7: i32, %arg8: i32, %arg9: i32):
    %0 = "arith.constant"() <{value = 0 : index}> : () -> index
    %1 = "arith.constant"() <{value = true}> : () -> i1
    %2 = "arith.constant"() <{value = -1.000000e+00 : f32}> : () -> f32
    %3 = "arith.constant"() <{value = 32 : index}> : () -> index
    %4 = "arith.constant"() <{value = 16 : index}> : () -> index
    %5 = "arith.constant"() <{value = 0.000000e+00 : bf16}> : () -> bf16
    %6 = "arith.constant"() <{value = 32 : i32}> : () -> i32
    %7 = "arith.constant"() <{value = 48 : i32}> : () -> i32
    %8 = "arith.constant"() <{value = 4 : i32}> : () -> i32
    %9 = "arith.constant"() <{value = 64 : i32}> : () -> i32
    %10 = "arith.constant"() <{value = 16 : i32}> : () -> i32
    %11 = "arith.constant"() <{value = 0.000000e+00 : f32}> : () -> f32
    "hivm.hir.set_mask_norm"() : () -> ()
    %12 = "arith.muli"(%arg7, %arg8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %13 = "arith.muli"(%12, %arg9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%13) <{effects = ["write"]}> {logical_block_num} : (i32) -> ()
    %14 = "hivm.hir.get_block_idx"() : () -> i64
    %15 = "arith.trunci"(%14) : (i64) -> i32
    %16 = "arith.divsi"(%15, %arg9) : (i32, i32) -> i32
    %17 = "arith.remsi"(%16, %arg8) : (i32, i32) -> i32
    %18 = "arith.muli"(%arg9, %arg8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %19 = "arith.divsi"(%15, %18) : (i32, i32) -> i32
    %20 = "arith.remsi"(%19, %arg7) : (i32, i32) -> i32
    %21 = "tensor.empty"() : () -> tensor<16x16xf32>
    %22 = "tensor.empty"() : () -> tensor<16x16xf32>
    %23 = "tensor.empty"() : () -> tensor<16x16xf32>
    %24 = "tensor.empty"() : () -> tensor<16x16xf32>
    %25 = "tensor.empty"() : () -> tensor<16x16xf32>
    %26 = "tensor.empty"() : () -> tensor<16x16xf32>
    %27 = "tensor.empty"() : () -> tensor<16x16xf32>
    %28 = "tensor.empty"() : () -> tensor<16x16xf32>
    %29 = "tensor.empty"() : () -> tensor<16x16xf32>
    %30 = "tensor.empty"() : () -> tensor<16x16xf32>
    %31 = "tensor.empty"() : () -> tensor<32x32xf32>
    %32 = "tensor.empty"() : () -> tensor<32x32xf32>
    %33 = "tensor.empty"() : () -> tensor<32x32xf32>
    %34 = "tensor.empty"() : () -> tensor<32x32xf32>
    %35 = "hivm.hir.vbrc"(%11, %34) <{broadcast_dims = array<i64>}> : (f32, tensor<32x32xf32>) -> tensor<32x32xf32>
    %36 = "tensor.empty"() : () -> tensor<32x32xbf16>
    %37 = "tensor.empty"() : () -> tensor<32x32xbf16>
    %38 = "tensor.empty"() : () -> tensor<32x32xbf16>
    %39 = "tensor.empty"() : () -> tensor<32x32xbf16>
    %40 = "hivm.hir.vbrc"(%5, %39) <{broadcast_dims = array<i64>}> : (bf16, tensor<32x32xbf16>) -> tensor<32x32xbf16>
    %41 = "arith.divsi"(%17, %8) : (i32, i32) -> i32
    %42 = "arith.remsi"(%17, %8) : (i32, i32) -> i32
    %43 = "arith.muli"(%41, %arg6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %44 = "arith.muli"(%43, %8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %45 = "arith.addi"(%44, %42) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %46 = "arith.muli"(%45, %9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %47 = "arith.index_cast"(%46) : (i32) -> index
    %48 = "arith.muli"(%45, %10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %49 = "arith.index_cast"(%48) : (i32) -> index
    %50 = "arith.muli"(%20, %9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %51 = "arith.addi"(%50, %10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %52 = "arith.index_cast"(%51) : (i32) -> index
    %53 = "affine.apply"(%52) <{map = #map}> : (index) -> index
    %54 = "affine.apply"(%49, %53) <{map = #map1}> : (index, index) -> index
    %55 = "memref.reinterpret_cast"(%arg4, %54) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 64, 1>}> : (memref<?xbf16>, index) -> memref<16x16xbf16, strided<[64, 1], offset: ?>>
    %56 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x16xbf16>
    %57 = "affine.apply"(%52) <{map = #map2}> : (index) -> index
    %58 = "arith.index_cast"(%arg6) : (i32) -> index
    %59 = "affine.max"(%52, %58) <{map = #map3}> : (index, index) -> index
    %60 = "affine.min"(%57, %59) <{map = #map3}> : (index, index) -> index
    %61 = "affine.apply"(%60, %52) <{map = #map4}> : (index, index) -> index
    %62 = "affine.min"(%61) <{map = #map5}> : (index) -> index
    %63 = "arith.cmpi"(%62, %4) <{predicate = 2 : i64}> : (index, index) -> i1
    %64 = "memref.subview"(%55, %62) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16, strided<[64, 1], offset: ?>>, index) -> memref<?x16xbf16, strided<[64, 1], offset: ?>>
    %65 = "memref.subview"(%56, %62) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16>, index) -> memref<?x16xbf16, strided<[16, 1]>>
    "hivm.hir.load"(%64, %65, %5, %0, %63) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x16xbf16, strided<[64, 1], offset: ?>>, memref<?x16xbf16, strided<[16, 1]>>, bf16, index, i1) -> ()
    %66 = "bufferization.to_tensor"(%56) <{restrict, writable}> : (memref<16x16xbf16>) -> tensor<16x16xbf16>
    %67 = "hivm.hir.vcast"(%66, %30) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x16xbf16>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %68 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xf32>
    %69 = "bufferization.to_tensor"(%68) <{restrict, writable}> : (memref<16x16xf32>) -> tensor<16x16xf32>
    %70 = "hivm.hir.store"(%67, %69) {"inserted-store"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %71 = "tensor.empty"() : () -> tensor<16x16xf32>
    %72 = "hivm.hir.load"(%70, %71) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %73 = "affine.apply"(%52) <{map = #map6}> : (index) -> index
    %74 = "affine.apply"(%47, %73) <{map = #map1}> : (index, index) -> index
    %75 = "memref.reinterpret_cast"(%arg3, %74) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 256, 1>}> : (memref<?xbf16>, index) -> memref<16x16xbf16, strided<[256, 1], offset: ?>>
    %76 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x16xbf16>
    %77 = "memref.subview"(%75, %62) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16, strided<[256, 1], offset: ?>>, index) -> memref<?x16xbf16, strided<[256, 1], offset: ?>>
    %78 = "memref.subview"(%76, %62) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16>, index) -> memref<?x16xbf16, strided<[16, 1]>>
    "hivm.hir.load"(%77, %78, %5, %0, %63) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x16xbf16, strided<[256, 1], offset: ?>>, memref<?x16xbf16, strided<[16, 1]>>, bf16, index, i1) -> ()
    %79 = "bufferization.to_tensor"(%76) <{restrict, writable}> : (memref<16x16xbf16>) -> tensor<16x16xbf16>
    %80 = "hivm.hir.vcast"(%79, %29) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x16xbf16>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %81 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xf32>
    %82 = "bufferization.to_tensor"(%81) <{restrict, writable}> : (memref<16x16xf32>) -> tensor<16x16xf32>
    %83 = "hivm.hir.store"(%80, %82) {"inserted-store"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %84 = "tensor.empty"() : () -> tensor<16x16xf32>
    %85 = "hivm.hir.load"(%83, %84) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %86 = "tensor.empty"() : () -> tensor<16x16xf32>
    %87 = "hivm.hir.mmadL1"(%72, %85, %1, %4, %4, %4, %86) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_for_result_already_inserted = true} : (tensor<16x16xf32>, tensor<16x16xf32>, i1, index, index, index, tensor<16x16xf32>) -> tensor<16x16xf32>
    %88 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xf32>
    %89 = "bufferization.to_tensor"(%88) <{restrict, writable}> : (memref<16x16xf32>) -> tensor<16x16xf32>
    %90 = "hivm.hir.fixpipe"(%87, %89) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %91 = "tensor.empty"() : () -> tensor<16x16xf32>
    %92 = "hivm.hir.load"(%90, %91) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %93 = "arith.index_cast"(%50) : (i32) -> index
    %94 = "affine.apply"(%93) <{map = #map}> : (index) -> index
    %95 = "affine.apply"(%49, %94) <{map = #map1}> : (index, index) -> index
    %96 = "memref.reinterpret_cast"(%arg4, %95) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 64, 1>}> : (memref<?xbf16>, index) -> memref<16x16xbf16, strided<[64, 1], offset: ?>>
    %97 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x16xbf16>
    %98 = "affine.apply"(%93) <{map = #map2}> : (index) -> index
    %99 = "affine.max"(%93, %58) <{map = #map3}> : (index, index) -> index
    %100 = "affine.min"(%98, %99) <{map = #map3}> : (index, index) -> index
    %101 = "affine.apply"(%100, %93) <{map = #map4}> : (index, index) -> index
    %102 = "affine.min"(%101) <{map = #map5}> : (index) -> index
    %103 = "arith.cmpi"(%102, %4) <{predicate = 2 : i64}> : (index, index) -> i1
    %104 = "memref.subview"(%96, %102) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16, strided<[64, 1], offset: ?>>, index) -> memref<?x16xbf16, strided<[64, 1], offset: ?>>
    %105 = "memref.subview"(%97, %102) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16>, index) -> memref<?x16xbf16, strided<[16, 1]>>
    "hivm.hir.load"(%104, %105, %5, %0, %103) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x16xbf16, strided<[64, 1], offset: ?>>, memref<?x16xbf16, strided<[16, 1]>>, bf16, index, i1) -> ()
    %106 = "bufferization.to_tensor"(%97) <{restrict, writable}> : (memref<16x16xbf16>) -> tensor<16x16xbf16>
    %107 = "hivm.hir.vcast"(%106, %28) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x16xbf16>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %108 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xf32>
    %109 = "bufferization.to_tensor"(%108) <{restrict, writable}> : (memref<16x16xf32>) -> tensor<16x16xf32>
    %110 = "hivm.hir.store"(%107, %109) {"inserted-store"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %111 = "tensor.empty"() : () -> tensor<16x16xf32>
    %112 = "hivm.hir.load"(%110, %111) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %113 = "tensor.empty"() : () -> tensor<16x16xf32>
    %114 = "hivm.hir.mmadL1"(%92, %112, %1, %4, %4, %4, %113) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_for_result_already_inserted = true} : (tensor<16x16xf32>, tensor<16x16xf32>, i1, index, index, index, tensor<16x16xf32>) -> tensor<16x16xf32>
    %115 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xf32>
    %116 = "bufferization.to_tensor"(%115) <{restrict, writable}> : (memref<16x16xf32>) -> tensor<16x16xf32>
    %117 = "hivm.hir.fixpipe"(%114, %116) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %118 = "tensor.empty"() : () -> tensor<16x16xf32>
    %119 = "hivm.hir.load"(%117, %118) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %120 = "hivm.hir.vmul"(%119, %2, %27) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x16xf32>, f32, tensor<16x16xf32>) -> tensor<16x16xf32>
    %121 = "hivm.hir.vadd"(%120, %11, %26) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x16xf32>, f32, tensor<16x16xf32>) -> tensor<16x16xf32>
    %122 = "arith.addi"(%50, %7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %123 = "arith.index_cast"(%122) : (i32) -> index
    %124 = "affine.apply"(%123) <{map = #map}> : (index) -> index
    %125 = "affine.apply"(%49, %124) <{map = #map1}> : (index, index) -> index
    %126 = "memref.reinterpret_cast"(%arg4, %125) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 64, 1>}> : (memref<?xbf16>, index) -> memref<16x16xbf16, strided<[64, 1], offset: ?>>
    %127 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x16xbf16>
    %128 = "affine.apply"(%123) <{map = #map2}> : (index) -> index
    %129 = "affine.max"(%123, %58) <{map = #map3}> : (index, index) -> index
    %130 = "affine.min"(%128, %129) <{map = #map3}> : (index, index) -> index
    %131 = "affine.apply"(%130, %123) <{map = #map4}> : (index, index) -> index
    %132 = "affine.min"(%131) <{map = #map5}> : (index) -> index
    %133 = "arith.cmpi"(%132, %4) <{predicate = 2 : i64}> : (index, index) -> i1
    %134 = "memref.subview"(%126, %132) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16, strided<[64, 1], offset: ?>>, index) -> memref<?x16xbf16, strided<[64, 1], offset: ?>>
    %135 = "memref.subview"(%127, %132) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16>, index) -> memref<?x16xbf16, strided<[16, 1]>>
    "hivm.hir.load"(%134, %135, %5, %0, %133) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x16xbf16, strided<[64, 1], offset: ?>>, memref<?x16xbf16, strided<[16, 1]>>, bf16, index, i1) -> ()
    %136 = "bufferization.to_tensor"(%127) <{restrict, writable}> : (memref<16x16xbf16>) -> tensor<16x16xbf16>
    %137 = "hivm.hir.vcast"(%136, %25) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x16xbf16>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %138 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xf32>
    %139 = "bufferization.to_tensor"(%138) <{restrict, writable}> : (memref<16x16xf32>) -> tensor<16x16xf32>
    %140 = "hivm.hir.store"(%137, %139) {"inserted-store"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %141 = "tensor.empty"() : () -> tensor<16x16xf32>
    %142 = "hivm.hir.load"(%140, %141) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %143 = "affine.apply"(%123) <{map = #map6}> : (index) -> index
    %144 = "affine.apply"(%47, %143) <{map = #map1}> : (index, index) -> index
    %145 = "affine.apply"(%144) <{map = #map7}> : (index) -> index
    %146 = "memref.reinterpret_cast"(%arg3, %145) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 256, 1>}> : (memref<?xbf16>, index) -> memref<16x16xbf16, strided<[256, 1], offset: ?>>
    %147 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x16xbf16>
    %148 = "memref.subview"(%146, %132) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16, strided<[256, 1], offset: ?>>, index) -> memref<?x16xbf16, strided<[256, 1], offset: ?>>
    %149 = "memref.subview"(%147, %132) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16>, index) -> memref<?x16xbf16, strided<[16, 1]>>
    "hivm.hir.load"(%148, %149, %5, %0, %133) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x16xbf16, strided<[256, 1], offset: ?>>, memref<?x16xbf16, strided<[16, 1]>>, bf16, index, i1) -> ()
    %150 = "bufferization.to_tensor"(%147) <{restrict, writable}> : (memref<16x16xbf16>) -> tensor<16x16xbf16>
    %151 = "hivm.hir.vcast"(%150, %24) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x16xbf16>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %152 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xf32>
    %153 = "bufferization.to_tensor"(%152) <{restrict, writable}> : (memref<16x16xf32>) -> tensor<16x16xf32>
    %154 = "hivm.hir.store"(%151, %153) {"inserted-store"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %155 = "tensor.empty"() : () -> tensor<16x16xf32>
    %156 = "hivm.hir.load"(%154, %155) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %157 = "tensor.empty"() : () -> tensor<16x16xf32>
    %158 = "hivm.hir.mmadL1"(%142, %156, %1, %4, %4, %4, %157) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_for_result_already_inserted = true} : (tensor<16x16xf32>, tensor<16x16xf32>, i1, index, index, index, tensor<16x16xf32>) -> tensor<16x16xf32>
    %159 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xf32>
    %160 = "bufferization.to_tensor"(%159) <{restrict, writable}> : (memref<16x16xf32>) -> tensor<16x16xf32>
    %161 = "hivm.hir.fixpipe"(%158, %160) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %162 = "tensor.empty"() : () -> tensor<16x16xf32>
    %163 = "hivm.hir.load"(%161, %162) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %164 = "arith.addi"(%50, %6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %165 = "arith.index_cast"(%164) : (i32) -> index
    %166 = "affine.apply"(%165) <{map = #map}> : (index) -> index
    %167 = "affine.apply"(%49, %166) <{map = #map1}> : (index, index) -> index
    %168 = "memref.reinterpret_cast"(%arg4, %167) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 64, 1>}> : (memref<?xbf16>, index) -> memref<16x16xbf16, strided<[64, 1], offset: ?>>
    %169 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x16xbf16>
    %170 = "affine.apply"(%165) <{map = #map2}> : (index) -> index
    %171 = "affine.max"(%165, %58) <{map = #map3}> : (index, index) -> index
    %172 = "affine.min"(%170, %171) <{map = #map3}> : (index, index) -> index
    %173 = "affine.apply"(%172, %165) <{map = #map4}> : (index, index) -> index
    %174 = "affine.min"(%173) <{map = #map5}> : (index) -> index
    %175 = "arith.cmpi"(%174, %4) <{predicate = 2 : i64}> : (index, index) -> i1
    %176 = "memref.subview"(%168, %174) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16, strided<[64, 1], offset: ?>>, index) -> memref<?x16xbf16, strided<[64, 1], offset: ?>>
    %177 = "memref.subview"(%169, %174) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16>, index) -> memref<?x16xbf16, strided<[16, 1]>>
    "hivm.hir.load"(%176, %177, %5, %0, %175) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x16xbf16, strided<[64, 1], offset: ?>>, memref<?x16xbf16, strided<[16, 1]>>, bf16, index, i1) -> ()
    %178 = "bufferization.to_tensor"(%169) <{restrict, writable}> : (memref<16x16xbf16>) -> tensor<16x16xbf16>
    %179 = "hivm.hir.vcast"(%178, %23) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x16xbf16>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %180 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xf32>
    %181 = "bufferization.to_tensor"(%180) <{restrict, writable}> : (memref<16x16xf32>) -> tensor<16x16xf32>
    %182 = "hivm.hir.store"(%179, %181) {"inserted-store"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %183 = "tensor.empty"() : () -> tensor<16x16xf32>
    %184 = "hivm.hir.load"(%182, %183) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %185 = "tensor.empty"() : () -> tensor<16x16xf32>
    %186 = "hivm.hir.mmadL1"(%163, %184, %1, %4, %4, %4, %185) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_for_result_already_inserted = true} : (tensor<16x16xf32>, tensor<16x16xf32>, i1, index, index, index, tensor<16x16xf32>) -> tensor<16x16xf32>
    %187 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xf32>
    %188 = "bufferization.to_tensor"(%187) <{restrict, writable}> : (memref<16x16xf32>) -> tensor<16x16xf32>
    %189 = "hivm.hir.fixpipe"(%186, %188) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %190 = "tensor.empty"() : () -> tensor<16x16xf32>
    %191 = "hivm.hir.load"(%189, %190) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %192 = "hivm.hir.vmul"(%191, %2, %22) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x16xf32>, f32, tensor<16x16xf32>) -> tensor<16x16xf32>
    %193 = "hivm.hir.vadd"(%192, %11, %21) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x16xf32>, f32, tensor<16x16xf32>) -> tensor<16x16xf32>
    %194 = "tensor.insert_slice"(%179, %35) <{operandSegmentSizes = array<i32: 1, 1, 0, 0, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 1, 1>}> : (tensor<16x16xf32>, tensor<32x32xf32>) -> tensor<32x32xf32>
    %195 = "tensor.insert_slice"(%137, %194) <{operandSegmentSizes = array<i32: 1, 1, 0, 0, 0>, static_offsets = array<i64: 16, 16>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 1, 1>}> : (tensor<16x16xf32>, tensor<32x32xf32>) -> tensor<32x32xf32>
    %196 = "tensor.insert_slice"(%193, %195) <{operandSegmentSizes = array<i32: 1, 1, 0, 0, 0>, static_offsets = array<i64: 16, 0>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 1, 1>}> : (tensor<16x16xf32>, tensor<32x32xf32>) -> tensor<32x32xf32>
    %197 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<32x32xf32>
    %198 = "bufferization.to_tensor"(%197) <{restrict, writable}> : (memref<32x32xf32>) -> tensor<32x32xf32>
    %199 = "hivm.hir.store"(%196, %198) {"inserted-store"} : (tensor<32x32xf32>, tensor<32x32xf32>) -> tensor<32x32xf32>
    %200 = "tensor.empty"() : () -> tensor<32x32xf32>
    %201 = "hivm.hir.load"(%199, %200) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<32x32xf32>, tensor<32x32xf32>) -> tensor<32x32xf32>
    %202 = "affine.apply"(%165) <{map = #map6}> : (index) -> index
    %203 = "affine.apply"(%47, %202) <{map = #map1}> : (index, index) -> index
    %204 = "memref.reinterpret_cast"(%arg3, %203) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 32, 32>, static_strides = array<i64: 256, 1>}> : (memref<?xbf16>, index) -> memref<32x32xbf16, strided<[256, 1], offset: ?>>
    %205 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<32x32xbf16>
    %206 = "affine.apply"(%165) <{map = #map7}> : (index) -> index
    %207 = "affine.min"(%206, %171) <{map = #map3}> : (index, index) -> index
    %208 = "affine.apply"(%207, %165) <{map = #map4}> : (index, index) -> index
    %209 = "affine.min"(%208) <{map = #map8}> : (index) -> index
    %210 = "arith.cmpi"(%209, %3) <{predicate = 2 : i64}> : (index, index) -> i1
    %211 = "memref.subview"(%204, %209) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 32>, static_strides = array<i64: 1, 1>}> : (memref<32x32xbf16, strided<[256, 1], offset: ?>>, index) -> memref<?x32xbf16, strided<[256, 1], offset: ?>>
    %212 = "memref.subview"(%205, %209) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 32>, static_strides = array<i64: 1, 1>}> : (memref<32x32xbf16>, index) -> memref<?x32xbf16, strided<[32, 1]>>
    "hivm.hir.load"(%211, %212, %5, %0, %210) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x32xbf16, strided<[256, 1], offset: ?>>, memref<?x32xbf16, strided<[32, 1]>>, bf16, index, i1) -> ()
    %213 = "bufferization.to_tensor"(%205) <{restrict, writable}> : (memref<32x32xbf16>) -> tensor<32x32xbf16>
    %214 = "hivm.hir.vcast"(%213, %33) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<32x32xbf16>, tensor<32x32xf32>) -> tensor<32x32xf32>
    %215 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<32x32xf32>
    %216 = "bufferization.to_tensor"(%215) <{restrict, writable}> : (memref<32x32xf32>) -> tensor<32x32xf32>
    %217 = "hivm.hir.store"(%214, %216) {"inserted-store"} : (tensor<32x32xf32>, tensor<32x32xf32>) -> tensor<32x32xf32>
    %218 = "tensor.empty"() : () -> tensor<32x32xf32>
    %219 = "hivm.hir.load"(%217, %218) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<32x32xf32>, tensor<32x32xf32>) -> tensor<32x32xf32>
    %220 = "tensor.empty"() : () -> tensor<32x32xf32>
    %221 = "hivm.hir.mmadL1"(%201, %219, %1, %3, %3, %3, %220) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_for_result_already_inserted = true} : (tensor<32x32xf32>, tensor<32x32xf32>, i1, index, index, index, tensor<32x32xf32>) -> tensor<32x32xf32>
    %222 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<32x32xf32>
    %223 = "bufferization.to_tensor"(%222) <{restrict, writable}> : (memref<32x32xf32>) -> tensor<32x32xf32>
    %224 = "hivm.hir.fixpipe"(%221, %223) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<32x32xf32>, tensor<32x32xf32>) -> tensor<32x32xf32>
    %225 = "tensor.empty"() : () -> tensor<32x32xf32>
    %226 = "hivm.hir.load"(%224, %225) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<32x32xf32>, tensor<32x32xf32>) -> tensor<32x32xf32>
    %227 = "tensor.insert_slice"(%107, %35) <{operandSegmentSizes = array<i32: 1, 1, 0, 0, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 1, 1>}> : (tensor<16x16xf32>, tensor<32x32xf32>) -> tensor<32x32xf32>
    %228 = "tensor.insert_slice"(%67, %227) <{operandSegmentSizes = array<i32: 1, 1, 0, 0, 0>, static_offsets = array<i64: 16, 16>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 1, 1>}> : (tensor<16x16xf32>, tensor<32x32xf32>) -> tensor<32x32xf32>
    %229 = "tensor.insert_slice"(%121, %228) <{operandSegmentSizes = array<i32: 1, 1, 0, 0, 0>, static_offsets = array<i64: 16, 0>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 1, 1>}> : (tensor<16x16xf32>, tensor<32x32xf32>) -> tensor<32x32xf32>
    %230 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<32x32xf32>
    %231 = "bufferization.to_tensor"(%230) <{restrict, writable}> : (memref<32x32xf32>) -> tensor<32x32xf32>
    %232 = "hivm.hir.store"(%229, %231) {"inserted-store"} : (tensor<32x32xf32>, tensor<32x32xf32>) -> tensor<32x32xf32>
    %233 = "tensor.empty"() : () -> tensor<32x32xf32>
    %234 = "hivm.hir.load"(%232, %233) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<32x32xf32>, tensor<32x32xf32>) -> tensor<32x32xf32>
    %235 = "tensor.empty"() : () -> tensor<32x32xf32>
    %236 = "hivm.hir.mmadL1"(%226, %234, %1, %3, %3, %3, %235) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_for_result_already_inserted = true} : (tensor<32x32xf32>, tensor<32x32xf32>, i1, index, index, index, tensor<32x32xf32>) -> tensor<32x32xf32>
    %237 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<32x32xf32>
    %238 = "bufferization.to_tensor"(%237) <{restrict, writable}> : (memref<32x32xf32>) -> tensor<32x32xf32>
    %239 = "hivm.hir.fixpipe"(%236, %238) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<32x32xf32>, tensor<32x32xf32>) -> tensor<32x32xf32>
    %240 = "tensor.empty"() : () -> tensor<32x32xf32>
    %241 = "hivm.hir.load"(%239, %240) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<32x32xf32>, tensor<32x32xf32>) -> tensor<32x32xf32>
    %242 = "hivm.hir.vmul"(%241, %2, %32) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32x32xf32>, f32, tensor<32x32xf32>) -> tensor<32x32xf32>
    %243 = "hivm.hir.vadd"(%242, %11, %31) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32x32xf32>, f32, tensor<32x32xf32>) -> tensor<32x32xf32>
    %244 = "affine.apply"(%93) <{map = #map6}> : (index) -> index
    %245 = "affine.apply"(%47, %244) <{map = #map1}> : (index, index) -> index
    %246 = "memref.reinterpret_cast"(%arg5, %245) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 32, 32>, static_strides = array<i64: 256, 1>}> : (memref<?xbf16>, index) -> memref<32x32xbf16, strided<[256, 1], offset: ?>>
    %247 = "hivm.hir.vcast"(%229, %38) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<32x32xf32>, tensor<32x32xbf16>) -> tensor<32x32xbf16>
    %248 = "affine.apply"(%93) <{map = #map7}> : (index) -> index
    %249 = "affine.min"(%248, %99) <{map = #map3}> : (index, index) -> index
    %250 = "affine.apply"(%249, %93) <{map = #map4}> : (index, index) -> index
    %251 = "affine.min"(%250) <{map = #map8}> : (index) -> index
    %252 = "tensor.extract_slice"(%247, %251) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 32>, static_strides = array<i64: 1, 1>}> : (tensor<32x32xbf16>, index) -> tensor<?x32xbf16>
    %253 = "memref.subview"(%246, %251) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 32>, static_strides = array<i64: 1, 1>}> : (memref<32x32xbf16, strided<[256, 1], offset: ?>>, index) -> memref<?x32xbf16, strided<[256, 1], offset: ?>>
    "hivm.hir.store"(%252, %253) : (tensor<?x32xbf16>, memref<?x32xbf16, strided<[256, 1], offset: ?>>) -> ()
    %254 = "affine.apply"(%203) <{map = #map7}> : (index) -> index
    %255 = "memref.reinterpret_cast"(%arg5, %254) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 32, 32>, static_strides = array<i64: 256, 1>}> : (memref<?xbf16>, index) -> memref<32x32xbf16, strided<[256, 1], offset: ?>>
    %256 = "hivm.hir.vcast"(%196, %37) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<32x32xf32>, tensor<32x32xbf16>) -> tensor<32x32xbf16>
    %257 = "tensor.extract_slice"(%256, %209) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 32>, static_strides = array<i64: 1, 1>}> : (tensor<32x32xbf16>, index) -> tensor<?x32xbf16>
    %258 = "memref.subview"(%255, %209) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 32>, static_strides = array<i64: 1, 1>}> : (memref<32x32xbf16, strided<[256, 1], offset: ?>>, index) -> memref<?x32xbf16, strided<[256, 1], offset: ?>>
    "hivm.hir.store"(%257, %258) : (tensor<?x32xbf16>, memref<?x32xbf16, strided<[256, 1], offset: ?>>) -> ()
    %259 = "memref.reinterpret_cast"(%arg5, %203) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 32, 32>, static_strides = array<i64: 256, 1>}> : (memref<?xbf16>, index) -> memref<32x32xbf16, strided<[256, 1], offset: ?>>
    %260 = "hivm.hir.vcast"(%243, %36) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<32x32xf32>, tensor<32x32xbf16>) -> tensor<32x32xbf16>
    %261 = "tensor.extract_slice"(%260, %209) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 32>, static_strides = array<i64: 1, 1>}> : (tensor<32x32xbf16>, index) -> tensor<?x32xbf16>
    %262 = "memref.subview"(%259, %209) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 32>, static_strides = array<i64: 1, 1>}> : (memref<32x32xbf16, strided<[256, 1], offset: ?>>, index) -> memref<?x32xbf16, strided<[256, 1], offset: ?>>
    "hivm.hir.store"(%261, %262) : (tensor<?x32xbf16>, memref<?x32xbf16, strided<[256, 1], offset: ?>>) -> ()
    %263 = "affine.apply"(%245) <{map = #map7}> : (index) -> index
    %264 = "memref.reinterpret_cast"(%arg5, %263) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 32, 32>, static_strides = array<i64: 256, 1>}> : (memref<?xbf16>, index) -> memref<32x32xbf16, strided<[256, 1], offset: ?>>
    %265 = "tensor.extract_slice"(%40, %251) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 32>, static_strides = array<i64: 1, 1>}> : (tensor<32x32xbf16>, index) -> tensor<?x32xbf16>
    %266 = "memref.subview"(%264, %251) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 32>, static_strides = array<i64: 1, 1>}> : (memref<32x32xbf16, strided<[256, 1], offset: ?>>, index) -> memref<?x32xbf16, strided<[256, 1], offset: ?>>
    "hivm.hir.store"(%265, %266) : (tensor<?x32xbf16>, memref<?x32xbf16, strided<[256, 1], offset: ?>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, false, false, false, false]> : vector<10xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<MIX>, mix_mode = "mix", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hacc.target = #hacc.target<"Ascend910B1">, hivm.module_core_type = #hivm.module_core_type<MIX>} : () -> ()

