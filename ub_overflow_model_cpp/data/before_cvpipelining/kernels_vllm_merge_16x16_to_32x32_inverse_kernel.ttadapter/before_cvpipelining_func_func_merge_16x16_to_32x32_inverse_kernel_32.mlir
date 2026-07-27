#map = affine_map<()[s0] -> (s0 * 128)>
#map1 = affine_map<()[s0, s1] -> (s0 + s1)>
#map2 = affine_map<()[s0] -> (s0 * 64)>
#map3 = affine_map<()[s0] -> (s0 + 16)>
#map4 = affine_map<()[s0] -> (s0 floordiv 128)>
#map5 = affine_map<()[s0, s1] -> (s0 - s1)>
#map6 = affine_map<()[s0] -> (s0, 0)>
#map7 = affine_map<()[s0] -> (s0, 16)>
#map8 = affine_map<()[s0] -> (s0 mod 128)>
#map9 = affine_map<()[s0] -> (-s0 + 32)>
#map10 = affine_map<()[s0, s1] -> (s0, s1)>
#map11 = affine_map<()[s0] -> (s0 floordiv 64)>
#map12 = affine_map<()[s0] -> (s0 mod 64)>
#map13 = affine_map<()[s0] -> (-s0 + 16)>
"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, i32, i32, i32, i32) -> (), sym_name = "merge_16x16_to_32x32_inverse_kernel"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xbf16>, %arg4: memref<?xbf16>, %arg5: memref<?xbf16>, %arg6: i32, %arg7: i32, %arg8: i32, %arg9: i32):
    %0 = "arith.constant"() <{value = true}> : () -> i1
    %1 = "arith.constant"() <{value = -1.000000e+00 : f32}> : () -> f32
    %2 = "arith.constant"() <{value = -16 : i32}> : () -> i32
    %3 = "arith.constant"() <{value = 0.000000e+00 : bf16}> : () -> bf16
    %4 = "arith.constant"() <{value = 16 : index}> : () -> index
    %5 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %6 = "arith.constant"() <{value = 4 : i32}> : () -> i32
    %7 = "arith.constant"() <{value = 32 : i32}> : () -> i32
    %8 = "arith.constant"() <{value = 16 : i32}> : () -> i32
    %9 = "arith.constant"() <{value = 0.000000e+00 : f32}> : () -> f32
    "hivm.hir.set_mask_norm"() : () -> ()
    %10 = "arith.muli"(%arg7, %arg8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %11 = "arith.muli"(%10, %arg9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%11) <{effects = ["write"]}> {logical_block_num} : (i32) -> ()
    %12 = "hivm.hir.get_block_idx"() : () -> i64
    %13 = "arith.trunci"(%12) : (i64) -> i32
    %14 = "arith.divsi"(%13, %arg9) : (i32, i32) -> i32
    %15 = "arith.remsi"(%14, %arg8) : (i32, i32) -> i32
    %16 = "arith.muli"(%arg9, %arg8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %17 = "arith.divsi"(%13, %16) : (i32, i32) -> i32
    %18 = "arith.remsi"(%17, %arg7) : (i32, i32) -> i32
    %19 = "tensor.empty"() : () -> tensor<16x16xf32>
    %20 = "tensor.empty"() : () -> tensor<16x16xf32>
    %21 = "tensor.empty"() : () -> tensor<16x16xf32>
    %22 = "tensor.empty"() : () -> tensor<16x16xf32>
    %23 = "tensor.empty"() : () -> tensor<16x16xf32>
    %24 = "arith.divsi"(%15, %6) : (i32, i32) -> i32
    %25 = "arith.remsi"(%15, %6) : (i32, i32) -> i32
    %26 = "arith.muli"(%24, %arg6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %27 = "arith.muli"(%26, %6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %28 = "arith.addi"(%27, %25) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %29 = "arith.muli"(%28, %7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %30 = "arith.index_cast"(%29) : (i32) -> index
    %31 = "arith.muli"(%28, %8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %32 = "arith.index_cast"(%31) : (i32) -> index
    %33 = "arith.muli"(%18, %7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %34 = "arith.addi"(%33, %8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %35 = "arith.maxsi"(%34, %5) : (i32, i32) -> i32
    %36 = "arith.index_cast"(%35) : (i32) -> index
    %37 = "affine.apply"(%36) <{map = #map}> : (index) -> index
    %38 = "affine.apply"(%37, %30) <{map = #map1}> : (index, index) -> index
    %39 = "arith.index_cast"(%arg6) : (i32) -> index
    %40 = "memref.reinterpret_cast"(%arg3, %38) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 128, 1>}> : (memref<?xbf16>, index) -> memref<16x16xbf16, strided<[128, 1], offset: ?>>
    %41 = "arith.maxsi"(%33, %5) : (i32, i32) -> i32
    %42 = "arith.index_cast"(%41) : (i32) -> index
    %43 = "affine.apply"(%42) <{map = #map2}> : (index) -> index
    %44 = "affine.apply"(%43, %32) <{map = #map1}> : (index, index) -> index
    %45 = "memref.reinterpret_cast"(%arg4, %44) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 64, 1>}> : (memref<?xbf16>, index) -> memref<16x16xbf16, strided<[64, 1], offset: ?>>
    %46 = "affine.apply"(%36) <{map = #map2}> : (index) -> index
    %47 = "affine.apply"(%46, %32) <{map = #map1}> : (index, index) -> index
    %48 = "memref.reinterpret_cast"(%arg4, %47) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 64, 1>}> : (memref<?xbf16>, index) -> memref<16x16xbf16, strided<[64, 1], offset: ?>>
    %49 = "affine.apply"(%42) <{map = #map}> : (index) -> index
    %50 = "affine.apply"(%49, %30) <{map = #map1}> : (index, index) -> index
    %51 = "memref.reinterpret_cast"(%arg5, %50) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 128, 1>}> : (memref<?xbf16>, index) -> memref<16x16xbf16, strided<[128, 1], offset: ?>>
    %52 = "affine.apply"(%38) <{map = #map3}> : (index) -> index
    %53 = "memref.reinterpret_cast"(%arg5, %52) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 128, 1>}> : (memref<?xbf16>, index) -> memref<16x16xbf16, strided<[128, 1], offset: ?>>
    %54 = "memref.reinterpret_cast"(%arg5, %38) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 128, 1>}> : (memref<?xbf16>, index) -> memref<16x16xbf16, strided<[128, 1], offset: ?>>
    %55 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x16xbf16>
    %56 = "affine.apply"(%37) <{map = #map4}> : (index) -> index
    %57 = "affine.apply"(%39, %56) <{map = #map5}> : (index, index) -> index
    %58 = "affine.max"(%57) <{map = #map6}> : (index) -> index
    %59 = "affine.min"(%58) <{map = #map7}> : (index) -> index
    %60 = "affine.apply"(%37) <{map = #map8}> : (index) -> index
    %61 = "affine.apply"(%60) <{map = #map9}> : (index) -> index
    %62 = "affine.max"(%61) <{map = #map6}> : (index) -> index
    %63 = "affine.min"(%62) <{map = #map7}> : (index) -> index
    %64 = "arith.subi"(%2, %33) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %65 = "arith.maxsi"(%64, %5) : (i32, i32) -> i32
    %66 = "arith.index_cast"(%65) : (i32) -> index
    %67 = "affine.min"(%66, %59) <{map = #map10}> : (index, index) -> index
    %68 = "affine.apply"(%59, %67) <{map = #map5}> : (index, index) -> index
    %69 = "affine.min"(%63) <{map = #map6}> : (index) -> index
    %70 = "affine.apply"(%63, %69) <{map = #map5}> : (index, index) -> index
    %71 = "arith.cmpi"(%68, %4) <{predicate = 2 : i64}> : (index, index) -> i1
    %72 = "arith.cmpi"(%70, %4) <{predicate = 2 : i64}> : (index, index) -> i1
    %73 = "arith.ori"(%71, %72) : (i1, i1) -> i1
    %74 = "memref.subview"(%40, %68, %70) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16, strided<[128, 1], offset: ?>>, index, index) -> memref<?x?xbf16, strided<[128, 1], offset: ?>>
    %75 = "memref.subview"(%55, %67, %69, %68, %70) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16>, index, index, index, index) -> memref<?x?xbf16, strided<[16, 1], offset: ?>>
    %76 = "arith.remui"(%69, %4) : (index, index) -> index
    "hivm.hir.load"(%74, %75, %3, %76, %73) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x?xbf16, strided<[128, 1], offset: ?>>, memref<?x?xbf16, strided<[16, 1], offset: ?>>, bf16, index, i1) -> ()
    %77 = "bufferization.to_tensor"(%55) <{restrict, writable}> : (memref<16x16xbf16>) -> tensor<16x16xbf16>
    %78 = "hivm.hir.vcast"(%77, %23) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x16xbf16>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %79 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xf32>
    %80 = "bufferization.to_tensor"(%79) <{restrict, writable}> : (memref<16x16xf32>) -> tensor<16x16xf32>
    %81 = "hivm.hir.store"(%78, %80) {"inserted-store"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %82 = "tensor.empty"() : () -> tensor<16x16xf32>
    %83 = "hivm.hir.load"(%81, %82) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %84 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x16xbf16>
    %85 = "affine.apply"(%43) <{map = #map11}> : (index) -> index
    %86 = "affine.apply"(%39, %85) <{map = #map5}> : (index, index) -> index
    %87 = "affine.max"(%86) <{map = #map6}> : (index) -> index
    %88 = "affine.min"(%87) <{map = #map7}> : (index) -> index
    %89 = "affine.apply"(%43) <{map = #map12}> : (index) -> index
    %90 = "affine.apply"(%89) <{map = #map13}> : (index) -> index
    %91 = "affine.max"(%90) <{map = #map6}> : (index) -> index
    %92 = "affine.min"(%91) <{map = #map7}> : (index) -> index
    %93 = "arith.subi"(%5, %33) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %94 = "arith.maxsi"(%93, %5) : (i32, i32) -> i32
    %95 = "arith.index_cast"(%94) : (i32) -> index
    %96 = "affine.min"(%95, %88) <{map = #map10}> : (index, index) -> index
    %97 = "affine.apply"(%88, %96) <{map = #map5}> : (index, index) -> index
    %98 = "affine.min"(%92) <{map = #map6}> : (index) -> index
    %99 = "affine.apply"(%92, %98) <{map = #map5}> : (index, index) -> index
    %100 = "arith.cmpi"(%97, %4) <{predicate = 2 : i64}> : (index, index) -> i1
    %101 = "arith.cmpi"(%99, %4) <{predicate = 2 : i64}> : (index, index) -> i1
    %102 = "arith.ori"(%100, %101) : (i1, i1) -> i1
    %103 = "memref.subview"(%45, %97, %99) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16, strided<[64, 1], offset: ?>>, index, index) -> memref<?x?xbf16, strided<[64, 1], offset: ?>>
    %104 = "memref.subview"(%84, %96, %98, %97, %99) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16>, index, index, index, index) -> memref<?x?xbf16, strided<[16, 1], offset: ?>>
    %105 = "arith.remui"(%98, %4) : (index, index) -> index
    "hivm.hir.load"(%103, %104, %3, %105, %102) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x?xbf16, strided<[64, 1], offset: ?>>, memref<?x?xbf16, strided<[16, 1], offset: ?>>, bf16, index, i1) -> ()
    %106 = "bufferization.to_tensor"(%84) <{restrict, writable}> : (memref<16x16xbf16>) -> tensor<16x16xbf16>
    %107 = "hivm.hir.vcast"(%106, %22) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x16xbf16>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %108 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xf32>
    %109 = "bufferization.to_tensor"(%108) <{restrict, writable}> : (memref<16x16xf32>) -> tensor<16x16xf32>
    %110 = "hivm.hir.store"(%107, %109) {"inserted-store"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %111 = "tensor.empty"() : () -> tensor<16x16xf32>
    %112 = "hivm.hir.load"(%110, %111) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %113 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x16xbf16>
    %114 = "affine.apply"(%46) <{map = #map11}> : (index) -> index
    %115 = "affine.apply"(%39, %114) <{map = #map5}> : (index, index) -> index
    %116 = "affine.max"(%115) <{map = #map6}> : (index) -> index
    %117 = "affine.min"(%116) <{map = #map7}> : (index) -> index
    %118 = "affine.apply"(%46) <{map = #map12}> : (index) -> index
    %119 = "affine.apply"(%118) <{map = #map13}> : (index) -> index
    %120 = "affine.max"(%119) <{map = #map6}> : (index) -> index
    %121 = "affine.min"(%120) <{map = #map7}> : (index) -> index
    %122 = "affine.min"(%66, %117) <{map = #map10}> : (index, index) -> index
    %123 = "affine.apply"(%117, %122) <{map = #map5}> : (index, index) -> index
    %124 = "affine.min"(%121) <{map = #map6}> : (index) -> index
    %125 = "affine.apply"(%121, %124) <{map = #map5}> : (index, index) -> index
    %126 = "arith.cmpi"(%123, %4) <{predicate = 2 : i64}> : (index, index) -> i1
    %127 = "arith.cmpi"(%125, %4) <{predicate = 2 : i64}> : (index, index) -> i1
    %128 = "arith.ori"(%126, %127) : (i1, i1) -> i1
    %129 = "memref.subview"(%48, %123, %125) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16, strided<[64, 1], offset: ?>>, index, index) -> memref<?x?xbf16, strided<[64, 1], offset: ?>>
    %130 = "memref.subview"(%113, %122, %124, %123, %125) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16>, index, index, index, index) -> memref<?x?xbf16, strided<[16, 1], offset: ?>>
    %131 = "arith.remui"(%124, %4) : (index, index) -> index
    "hivm.hir.load"(%129, %130, %3, %131, %128) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x?xbf16, strided<[64, 1], offset: ?>>, memref<?x?xbf16, strided<[16, 1], offset: ?>>, bf16, index, i1) -> ()
    %132 = "bufferization.to_tensor"(%113) <{restrict, writable}> : (memref<16x16xbf16>) -> tensor<16x16xbf16>
    %133 = "hivm.hir.vcast"(%132, %21) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x16xbf16>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %134 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xf32>
    %135 = "bufferization.to_tensor"(%134) <{restrict, writable}> : (memref<16x16xf32>) -> tensor<16x16xf32>
    %136 = "hivm.hir.store"(%133, %135) {"inserted-store"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %137 = "tensor.empty"() : () -> tensor<16x16xf32>
    %138 = "hivm.hir.load"(%136, %137) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %139 = "tensor.empty"() : () -> tensor<16x16xf32>
    %140 = "hivm.hir.mmadL1"(%138, %83, %0, %4, %4, %4, %139) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_for_result_already_inserted = true} : (tensor<16x16xf32>, tensor<16x16xf32>, i1, index, index, index, tensor<16x16xf32>) -> tensor<16x16xf32>
    %141 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xf32>
    %142 = "bufferization.to_tensor"(%141) <{restrict, writable}> : (memref<16x16xf32>) -> tensor<16x16xf32>
    %143 = "hivm.hir.fixpipe"(%140, %142) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %144 = "tensor.empty"() : () -> tensor<16x16xf32>
    %145 = "hivm.hir.load"(%143, %144) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %146 = "tensor.empty"() : () -> tensor<16x16xf32>
    %147 = "hivm.hir.mmadL1"(%145, %112, %0, %4, %4, %4, %146) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_for_result_already_inserted = true} : (tensor<16x16xf32>, tensor<16x16xf32>, i1, index, index, index, tensor<16x16xf32>) -> tensor<16x16xf32>
    %148 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xf32>
    %149 = "bufferization.to_tensor"(%148) <{restrict, writable}> : (memref<16x16xf32>) -> tensor<16x16xf32>
    %150 = "hivm.hir.fixpipe"(%147, %149) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %151 = "tensor.empty"() : () -> tensor<16x16xf32>
    %152 = "hivm.hir.load"(%150, %151) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %153 = "hivm.hir.vmul"(%152, %1, %20) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x16xf32>, f32, tensor<16x16xf32>) -> tensor<16x16xf32>
    %154 = "hivm.hir.vadd"(%153, %9, %19) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x16xf32>, f32, tensor<16x16xf32>) -> tensor<16x16xf32>
    %155 = "affine.apply"(%49) <{map = #map4}> : (index) -> index
    %156 = "affine.apply"(%39, %155) <{map = #map5}> : (index, index) -> index
    %157 = "affine.max"(%156) <{map = #map6}> : (index) -> index
    %158 = "affine.min"(%157) <{map = #map7}> : (index) -> index
    %159 = "affine.apply"(%49) <{map = #map8}> : (index) -> index
    %160 = "affine.apply"(%159) <{map = #map9}> : (index) -> index
    %161 = "affine.max"(%160) <{map = #map6}> : (index) -> index
    %162 = "affine.min"(%161) <{map = #map7}> : (index) -> index
    %163 = "affine.min"(%95, %158) <{map = #map10}> : (index, index) -> index
    %164 = "affine.apply"(%158, %163) <{map = #map5}> : (index, index) -> index
    %165 = "affine.min"(%162) <{map = #map6}> : (index) -> index
    %166 = "affine.apply"(%162, %165) <{map = #map5}> : (index, index) -> index
    %167 = "tensor.extract_slice"(%106, %163, %165, %164, %166) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<16x16xbf16>, index, index, index, index) -> tensor<?x?xbf16>
    %168 = "memref.subview"(%51, %164, %166) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16, strided<[128, 1], offset: ?>>, index, index) -> memref<?x?xbf16, strided<[128, 1], offset: ?>>
    "hivm.hir.store"(%167, %168) : (tensor<?x?xbf16>, memref<?x?xbf16, strided<[128, 1], offset: ?>>) -> ()
    %169 = "affine.apply"(%52, %30) <{map = #map5}> : (index, index) -> index
    %170 = "affine.apply"(%169) <{map = #map4}> : (index) -> index
    %171 = "affine.apply"(%39, %170) <{map = #map5}> : (index, index) -> index
    %172 = "affine.max"(%171) <{map = #map6}> : (index) -> index
    %173 = "affine.min"(%172) <{map = #map7}> : (index) -> index
    %174 = "affine.apply"(%169) <{map = #map8}> : (index) -> index
    %175 = "affine.apply"(%174) <{map = #map9}> : (index) -> index
    %176 = "affine.max"(%175) <{map = #map6}> : (index) -> index
    %177 = "affine.min"(%176) <{map = #map7}> : (index) -> index
    %178 = "affine.min"(%66, %173) <{map = #map10}> : (index, index) -> index
    %179 = "affine.apply"(%173, %178) <{map = #map5}> : (index, index) -> index
    %180 = "affine.min"(%177) <{map = #map6}> : (index) -> index
    %181 = "affine.apply"(%177, %180) <{map = #map5}> : (index, index) -> index
    %182 = "tensor.extract_slice"(%132, %178, %180, %179, %181) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<16x16xbf16>, index, index, index, index) -> tensor<?x?xbf16>
    %183 = "memref.subview"(%53, %179, %181) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16, strided<[128, 1], offset: ?>>, index, index) -> memref<?x?xbf16, strided<[128, 1], offset: ?>>
    "hivm.hir.store"(%182, %183) : (tensor<?x?xbf16>, memref<?x?xbf16, strided<[128, 1], offset: ?>>) -> ()
    %184 = "tensor.empty"() : () -> tensor<16x16xbf16>
    %185 = "hivm.hir.vcast"(%154, %184) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x16xf32>, tensor<16x16xbf16>) -> tensor<16x16xbf16>
    %186 = "tensor.extract_slice"(%185, %67, %69, %68, %70) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<16x16xbf16>, index, index, index, index) -> tensor<?x?xbf16>
    %187 = "memref.subview"(%54, %68, %70) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16, strided<[128, 1], offset: ?>>, index, index) -> memref<?x?xbf16, strided<[128, 1], offset: ?>>
    "hivm.hir.store"(%186, %187) : (tensor<?x?xbf16>, memref<?x?xbf16, strided<[128, 1], offset: ?>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, false, false, false, false]> : vector<10xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<MIX>, mix_mode = "mix", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hacc.target = #hacc.target<"Ascend910B1">, hivm.module_core_type = #hivm.module_core_type<MIX>} : () -> ()

