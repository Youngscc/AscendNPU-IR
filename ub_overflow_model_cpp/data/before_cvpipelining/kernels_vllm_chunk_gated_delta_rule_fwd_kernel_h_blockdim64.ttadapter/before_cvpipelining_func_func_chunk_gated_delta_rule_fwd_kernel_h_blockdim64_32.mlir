#map = affine_map<()[s0, s1] -> (s0 + s1)>
#map1 = affine_map<()[s0] -> (s0 + 64)>
#map2 = affine_map<()[s0] -> (s0 * 512)>
#map3 = affine_map<()[s0, s1] -> (s0, s1)>
#map4 = affine_map<()[s0, s1] -> (s0 - s1)>
#map5 = affine_map<()[s0] -> (s0, 64)>
#map6 = affine_map<()[s0] -> (s0 floordiv 512)>
#map7 = affine_map<()[s0] -> (s0, 0)>
#map8 = affine_map<()[s0] -> (s0 mod 512)>
#map9 = affine_map<()[s0] -> (-s0 + 128)>
#map10 = affine_map<()[s0] -> (s0, 128)>
"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, memref<?xf32>, memref<?xbf16>, i32, i32, i32, i32) -> (), sym_name = "chunk_gated_delta_rule_fwd_kernel_h_blockdim64"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xbf16>, %arg4: memref<?xbf16>, %arg5: memref<?xbf16>, %arg6: memref<?xbf16>, %arg7: memref<?xf32>, %arg8: memref<?xbf16>, %arg9: i32, %arg10: i32, %arg11: i32, %arg12: i32):
    %0 = "arith.constant"() <{value = 1 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = true}> : () -> i1
    %2 = "arith.constant"() <{value = -1.000000e+00 : f32}> : () -> f32
    %3 = "arith.constant"() <{value = 0.000000e+00 : bf16}> : () -> bf16
    %4 = "arith.constant"() <{value = 64 : index}> : () -> index
    %5 = "arith.constant"() <{value = 128 : index}> : () -> index
    %6 = "arith.constant"() <{value = 0xFF800000 : f32}> : () -> f32
    %7 = "arith.constant"() <{value = 512 : i32}> : () -> i32
    %8 = "arith.constant"() <{value = 16384 : i32}> : () -> i32
    %9 = "arith.constant"() <{value = 65536 : i32}> : () -> i32
    %10 = "arith.constant"() <{value = 63 : i32}> : () -> i32
    %11 = "arith.constant"() <{value = 0 : index}> : () -> index
    %12 = "arith.constant"() <{value = 4 : i32}> : () -> i32
    %13 = "arith.constant"() <{value = 64 : i32}> : () -> i32
    %14 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %15 = "arith.constant"() <{value = 128 : i32}> : () -> i32
    %16 = "arith.constant"() <{value = 0.000000e+00 : f32}> : () -> f32
    %17 = "arith.constant"() <{value = 16 : index}> : () -> index
    "hivm.hir.set_mask_norm"() : () -> ()
    %18 = "arith.muli"(%arg10, %arg11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %19 = "arith.muli"(%18, %arg12) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%19) <{effects = ["write"]}> {logical_block_num} : (i32) -> ()
    %20 = "hivm.hir.get_block_idx"() : () -> i64
    %21 = "arith.trunci"(%20) : (i64) -> i32
    %22 = "arith.divsi"(%21, %arg12) : (i32, i32) -> i32
    %23 = "arith.remsi"(%22, %arg11) : (i32, i32) -> i32
    %24 = "tensor.empty"() : () -> tensor<64xf32>
    %25 = "hivm.hir.vbrc"(%16, %24) <{broadcast_dims = array<i64>}> : (f32, tensor<64xf32>) -> tensor<64xf32>
    %26 = "tensor.empty"() : () -> tensor<64x64xf32>
    %27 = "tensor.empty"() : () -> tensor<128x64xf32>
    %28 = "hivm.hir.vbrc"(%16, %27) <{broadcast_dims = array<i64>}> : (f32, tensor<128x64xf32>) -> tensor<128x64xf32>
    %29 = "arith.divsi"(%23, %12) : (i32, i32) -> i32
    %30 = "arith.remsi"(%23, %12) : (i32, i32) -> i32
    %31 = "arith.muli"(%29, %arg9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %32 = "arith.addi"(%arg9, %10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %33 = "arith.divsi"(%32, %13) : (i32, i32) -> i32
    %34 = "arith.muli"(%29, %33) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %35 = "arith.muli"(%30, %8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %36 = "arith.muli"(%31, %7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %37 = "arith.index_cast"(%36) : (i32) -> index
    %38 = "arith.muli"(%30, %15) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %39 = "arith.index_cast"(%38) : (i32) -> index
    %40 = "affine.apply"(%37, %39) <{map = #map}> : (index, index) -> index
    %41 = "arith.index_cast"(%31) : (i32) -> index
    %42 = "arith.muli"(%30, %arg9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %43 = "arith.index_cast"(%42) : (i32) -> index
    %44 = "affine.apply"(%41, %43) <{map = #map}> : (index, index) -> index
    %45:2 = "scf.for"(%14, %33, %0, %28, %28) ({
    ^bb0(%arg13: i32, %arg14: tensor<128x64xf32>, %arg15: tensor<128x64xf32>):
      %46 = "arith.addi"(%34, %arg13) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %47 = "arith.muli"(%46, %9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %48 = "arith.index_cast"(%47) : (i32) -> index
      %49 = "arith.index_cast"(%35) : (i32) -> index
      %50 = "affine.apply"(%48, %49) <{map = #map}> : (index, index) -> index
      %51 = "memref.reinterpret_cast"(%arg8, %50) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 128, 64>, static_strides = array<i64: 128, 1>}> : (memref<?xbf16>, index) -> memref<128x64xbf16, strided<[128, 1], offset: ?>>
      %52 = "tensor.empty"() : () -> tensor<128x64xbf16>
      %53 = "hivm.hir.vcast"(%arg14, %52) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<128x64xf32>, tensor<128x64xbf16>) -> tensor<128x64xbf16>
      %54 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<128x64xbf16>
      %55 = "bufferization.to_tensor"(%54) <{restrict, writable}> : (memref<128x64xbf16>) -> tensor<128x64xbf16>
      %56 = "hivm.hir.store"(%53, %55) {"inserted-store"} : (tensor<128x64xbf16>, tensor<128x64xbf16>) -> tensor<128x64xbf16>
      %57 = "hivm.hir.load"(%56, %52) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<128x64xbf16>, tensor<128x64xbf16>) -> tensor<128x64xbf16>
      "hivm.hir.store"(%53, %51) : (tensor<128x64xbf16>, memref<128x64xbf16, strided<[128, 1], offset: ?>>) -> ()
      %58 = "affine.apply"(%50) <{map = #map1}> : (index) -> index
      %59 = "memref.reinterpret_cast"(%arg8, %58) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 128, 64>, static_strides = array<i64: 128, 1>}> : (memref<?xbf16>, index) -> memref<128x64xbf16, strided<[128, 1], offset: ?>>
      %60 = "hivm.hir.vcast"(%arg15, %52) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<128x64xf32>, tensor<128x64xbf16>) -> tensor<128x64xbf16>
      %61 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<128x64xbf16>
      %62 = "bufferization.to_tensor"(%61) <{restrict, writable}> : (memref<128x64xbf16>) -> tensor<128x64xbf16>
      %63 = "hivm.hir.store"(%60, %62) {"inserted-store"} : (tensor<128x64xbf16>, tensor<128x64xbf16>) -> tensor<128x64xbf16>
      %64 = "hivm.hir.load"(%63, %52) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<128x64xbf16>, tensor<128x64xbf16>) -> tensor<128x64xbf16>
      "hivm.hir.store"(%60, %59) : (tensor<128x64xbf16>, memref<128x64xbf16, strided<[128, 1], offset: ?>>) -> ()
      %65 = "arith.muli"(%arg13, %13) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %66 = "arith.index_cast"(%65) : (i32) -> index
      %67 = "affine.apply"(%66) <{map = #map2}> : (index) -> index
      %68 = "affine.apply"(%40, %67) <{map = #map}> : (index, index) -> index
      %69 = "memref.reinterpret_cast"(%arg5, %68) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64, 128>, static_strides = array<i64: 512, 1>}> : (memref<?xbf16>, index) -> memref<64x128xbf16, strided<[512, 1], offset: ?>>
      %70 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<64x128xbf16>
      %71 = "affine.apply"(%66) <{map = #map1}> : (index) -> index
      %72 = "arith.index_cast"(%arg9) : (i32) -> index
      %73 = "affine.max"(%66, %72) <{map = #map3}> : (index, index) -> index
      %74 = "affine.min"(%71, %73) <{map = #map3}> : (index, index) -> index
      %75 = "affine.apply"(%74, %66) <{map = #map4}> : (index, index) -> index
      %76 = "affine.min"(%75) <{map = #map5}> : (index) -> index
      %77 = "arith.cmpi"(%76, %4) <{predicate = 2 : i64}> : (index, index) -> i1
      %78 = "memref.subview"(%69, %76) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 128>, static_strides = array<i64: 1, 1>}> : (memref<64x128xbf16, strided<[512, 1], offset: ?>>, index) -> memref<?x128xbf16, strided<[512, 1], offset: ?>>
      %79 = "memref.subview"(%70, %76) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 128>, static_strides = array<i64: 1, 1>}> : (memref<64x128xbf16>, index) -> memref<?x128xbf16, strided<[128, 1]>>
      "hivm.hir.load"(%78, %79, %3, %11, %77) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<?x128xbf16, strided<[512, 1], offset: ?>>, memref<?x128xbf16, strided<[128, 1]>>, bf16, index, i1) -> ()
      %80 = "bufferization.to_tensor"(%70) <{restrict, writable}> : (memref<64x128xbf16>) -> tensor<64x128xbf16>
      %81 = "arith.maxsi"(%65, %14) : (i32, i32) -> i32
      %82 = "arith.index_cast"(%81) : (i32) -> index
      %83 = "affine.apply"(%82) <{map = #map2}> : (index) -> index
      %84 = "affine.apply"(%83, %40) <{map = #map}> : (index, index) -> index
      %85 = "memref.reinterpret_cast"(%arg3, %84) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64, 128>, static_strides = array<i64: 512, 1>}> : (memref<?xbf16>, index) -> memref<64x128xbf16, strided<[512, 1], offset: ?>>
      %86 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<64x128xbf16>
      %87 = "affine.apply"(%83) <{map = #map6}> : (index) -> index
      %88 = "affine.apply"(%72, %87) <{map = #map4}> : (index, index) -> index
      %89 = "affine.max"(%88) <{map = #map7}> : (index) -> index
      %90 = "affine.min"(%89) <{map = #map5}> : (index) -> index
      %91 = "affine.apply"(%83) <{map = #map8}> : (index) -> index
      %92 = "affine.apply"(%91) <{map = #map9}> : (index) -> index
      %93 = "affine.max"(%92) <{map = #map7}> : (index) -> index
      %94 = "affine.min"(%93) <{map = #map10}> : (index) -> index
      %95 = "arith.subi"(%14, %65) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %96 = "arith.maxsi"(%95, %14) : (i32, i32) -> i32
      %97 = "arith.index_cast"(%96) : (i32) -> index
      %98 = "affine.min"(%97, %90) <{map = #map3}> : (index, index) -> index
      %99 = "affine.apply"(%90, %98) <{map = #map4}> : (index, index) -> index
      %100 = "affine.min"(%94) <{map = #map7}> : (index) -> index
      %101 = "affine.apply"(%94, %100) <{map = #map4}> : (index, index) -> index
      %102 = "arith.cmpi"(%99, %4) <{predicate = 2 : i64}> : (index, index) -> i1
      %103 = "arith.cmpi"(%101, %5) <{predicate = 2 : i64}> : (index, index) -> i1
      %104 = "arith.ori"(%102, %103) : (i1, i1) -> i1
      %105 = "memref.subview"(%85, %99, %101) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<64x128xbf16, strided<[512, 1], offset: ?>>, index, index) -> memref<?x?xbf16, strided<[512, 1], offset: ?>>
      %106 = "memref.subview"(%86, %98, %100, %99, %101) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<64x128xbf16>, index, index, index, index) -> memref<?x?xbf16, strided<[128, 1], offset: ?>>
      %107 = "arith.remui"(%100, %17) : (index, index) -> index
      "hivm.hir.load"(%105, %106, %3, %107, %104) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<?x?xbf16, strided<[512, 1], offset: ?>>, memref<?x?xbf16, strided<[128, 1], offset: ?>>, bf16, index, i1) -> ()
      %108 = "bufferization.to_tensor"(%86) <{restrict, writable}> : (memref<64x128xbf16>) -> tensor<64x128xbf16>
      %109 = "arith.addi"(%arg13, %0) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %110 = "arith.muli"(%109, %13) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %111 = "arith.minsi"(%110, %arg9) : (i32, i32) -> i32
      %112 = "arith.subi"(%111, %0) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %113 = "arith.index_cast"(%112) : (i32) -> index
      %114 = "affine.apply"(%44, %113) <{map = #map}> : (index, index) -> index
      %115 = "memref.reinterpret_cast"(%arg7, %114) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<1xf32, strided<[1], offset: ?>>
      %116 = "memref.load"(%115, %11) : (memref<1xf32, strided<[1], offset: ?>>, index) -> f32
      %117 = "affine.apply"(%44, %66) <{map = #map}> : (index, index) -> index
      %118 = "memref.reinterpret_cast"(%arg7, %117) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<64xf32, strided<[1], offset: ?>>
      %119 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<64xf32>
      %120 = "arith.cmpi"(%75, %4) <{predicate = 2 : i64}> : (index, index) -> i1
      %121 = "memref.subview"(%118, %75) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<64xf32, strided<[1], offset: ?>>, index) -> memref<?xf32, strided<[1], offset: ?>>
      %122 = "memref.subview"(%119, %75) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<64xf32>, index) -> memref<?xf32, strided<[1]>>
      "hivm.hir.load"(%121, %122, %16, %11, %120) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xf32, strided<[1], offset: ?>>, memref<?xf32, strided<[1]>>, f32, index, i1) -> ()
      %123 = "bufferization.to_tensor"(%119) <{restrict, writable}> : (memref<64xf32>) -> tensor<64xf32>
      %124 = "hivm.hir.vmul"(%123, %2, %24) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, f32, tensor<64xf32>) -> tensor<64xf32>
      %125 = "hivm.hir.vadd"(%124, %116, %24) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, f32, tensor<64xf32>) -> tensor<64xf32>
      %126 = "tensor.empty"() : () -> tensor<64xi1>
      %127 = "hivm.hir.vcmp"(%125, %25, %126) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<le>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>, tensor<64xi1>) -> tensor<64xi1>
      %128 = "hivm.hir.vsel"(%127, %125, %6, %24) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<64xi1>, tensor<64xf32>, f32, tensor<64xf32>) -> tensor<64xf32>
      %129 = "hivm.hir.vexp"(%128, %24) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
      %130 = "tensor.empty"() : () -> tensor<1xf32>
      %131 = "tensor.insert"(%116, %130, %11) : (f32, tensor<1xf32>, index) -> tensor<1xf32>
      %132 = "hivm.hir.vexp"(%131, %130) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<1xf32>, tensor<1xf32>) -> tensor<1xf32>
      %133 = "tensor.extract"(%132, %11) {"DuplicateTensorExtractForCube::visitedLabel" = 1 : i32} : (tensor<1xf32>, index) -> f32
      %134 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<1xf32>
      %135 = "bufferization.to_tensor"(%134) <{restrict, writable}> : (memref<1xf32>) -> tensor<1xf32>
      %136 = "hivm.hir.store"(%132, %135) {"inserted-store"} : (tensor<1xf32>, tensor<1xf32>) -> tensor<1xf32>
      "annotation.mark"(%136) <{effects = ["write"]}> {hivm.tcore_type = #hivm.tcore_type<VECTOR>} : (tensor<1xf32>) -> ()
      %137 = "tensor.extract"(%136, %11) {"DuplicateTensorExtractForCube::newExtractLabel" = 1 : i32, "DuplicateTensorExtractForCube::visitedLabel" = 1 : i32} : (tensor<1xf32>, index) -> f32
      "annotation.mark"(%133, %137) <{effects = ["write"], keys = []}> {"DuplicateTensorExtractForCube::replacementLabel" = 1 : i32} : (f32, f32) -> ()
      %138 = "memref.reinterpret_cast"(%arg4, %68) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64, 64>, static_strides = array<i64: 512, 1>}> : (memref<?xbf16>, index) -> memref<64x64xbf16, strided<[512, 1], offset: ?>>
      %139 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<64x64xbf16>
      %140 = "memref.subview"(%138, %76) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 64>, static_strides = array<i64: 1, 1>}> : (memref<64x64xbf16, strided<[512, 1], offset: ?>>, index) -> memref<?x64xbf16, strided<[512, 1], offset: ?>>
      %141 = "memref.subview"(%139, %76) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 64>, static_strides = array<i64: 1, 1>}> : (memref<64x64xbf16>, index) -> memref<?x64xbf16, strided<[64, 1]>>
      "hivm.hir.load"(%140, %141, %3, %11, %77) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x64xbf16, strided<[512, 1], offset: ?>>, memref<?x64xbf16, strided<[64, 1]>>, bf16, index, i1) -> ()
      %142 = "bufferization.to_tensor"(%139) <{restrict, writable}> : (memref<64x64xbf16>) -> tensor<64x64xbf16>
      %143 = "hivm.hir.vcast"(%142, %26) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<64x64xbf16>, tensor<64x64xf32>) -> tensor<64x64xf32>
      %144 = "hivm.hir.mmadL1"(%80, %57, %1, %76, %5, %4, %26) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_for_result_already_inserted = true} : (tensor<64x128xbf16>, tensor<128x64xbf16>, i1, index, index, index, tensor<64x64xf32>) -> tensor<64x64xf32>
      %145 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<64x64xf32>
      %146 = "bufferization.to_tensor"(%145) <{restrict, writable}> : (memref<64x64xf32>) -> tensor<64x64xf32>
      %147 = "hivm.hir.fixpipe"(%144, %146) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<64x64xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
      %148 = "hivm.hir.load"(%147, %26) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<64x64xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
      %149 = "hivm.hir.vsub"(%143, %148, %26) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x64xf32>, tensor<64x64xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
      %150 = "memref.reinterpret_cast"(%arg6, %84) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64, 64>, static_strides = array<i64: 512, 1>}> : (memref<?xbf16>, index) -> memref<64x64xbf16, strided<[512, 1], offset: ?>>
      %151 = "tensor.empty"() : () -> tensor<64x64xbf16>
      %152 = "hivm.hir.vcast"(%149, %151) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<64x64xf32>, tensor<64x64xbf16>) -> tensor<64x64xbf16>
      %153 = "affine.min"(%93) <{map = #map5}> : (index) -> index
      %154 = "affine.min"(%153) <{map = #map7}> : (index) -> index
      %155 = "affine.apply"(%153, %154) <{map = #map4}> : (index, index) -> index
      %156 = "tensor.extract_slice"(%152, %98, %154, %99, %155) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<64x64xbf16>, index, index, index, index) -> tensor<?x?xbf16>
      %157 = "memref.subview"(%150, %99, %155) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<64x64xbf16, strided<[512, 1], offset: ?>>, index, index) -> memref<?x?xbf16, strided<[512, 1], offset: ?>>
      "hivm.hir.store"(%156, %157) : (tensor<?x?xbf16>, memref<?x?xbf16, strided<[512, 1], offset: ?>>) -> ()
      %158 = "tensor.expand_shape"(%129) <{reassociation = [[0, 1]], static_output_shape = array<i64: 64, 1>}> : (tensor<64xf32>) -> tensor<64x1xf32>
      %159 = "hivm.hir.vbrc"(%158, %26) <{broadcast_dims = array<i64: 1>}> : (tensor<64x1xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
      %160 = "hivm.hir.vmul"(%149, %159, %26) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x64xf32>, tensor<64x64xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
      %161 = "hivm.hir.vmul"(%arg14, %133, %27) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128x64xf32>, f32, tensor<128x64xf32>) -> tensor<128x64xf32>
      %162 = "hivm.hir.vcast"(%160, %151) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<64x64xf32>, tensor<64x64xbf16>) -> tensor<64x64xbf16>
      %163 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<64x64xbf16>
      %164 = "bufferization.to_tensor"(%163) <{restrict, writable}> : (memref<64x64xbf16>) -> tensor<64x64xbf16>
      %165 = "hivm.hir.store"(%162, %164) {"inserted-store"} : (tensor<64x64xbf16>, tensor<64x64xbf16>) -> tensor<64x64xbf16>
      %166 = "hivm.hir.load"(%165, %151) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<64x64xbf16>, tensor<64x64xbf16>) -> tensor<64x64xbf16>
      %167 = "hivm.hir.mmadL1"(%108, %166, %1, %101, %99, %4, %27) <{a_transpose, operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_for_result_already_inserted = true} : (tensor<64x128xbf16>, tensor<64x64xbf16>, i1, index, index, index, tensor<128x64xf32>) -> tensor<128x64xf32>
      %168 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<128x64xf32>
      %169 = "bufferization.to_tensor"(%168) <{restrict, writable}> : (memref<128x64xf32>) -> tensor<128x64xf32>
      %170 = "hivm.hir.fixpipe"(%167, %169) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<128x64xf32>, tensor<128x64xf32>) -> tensor<128x64xf32>
      %171 = "hivm.hir.load"(%170, %27) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<128x64xf32>, tensor<128x64xf32>) -> tensor<128x64xf32>
      %172 = "hivm.hir.vadd"(%171, %161, %27) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128x64xf32>, tensor<128x64xf32>, tensor<128x64xf32>) -> tensor<128x64xf32>
      %173 = "affine.apply"(%68) <{map = #map1}> : (index) -> index
      %174 = "memref.reinterpret_cast"(%arg4, %173) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64, 64>, static_strides = array<i64: 512, 1>}> : (memref<?xbf16>, index) -> memref<64x64xbf16, strided<[512, 1], offset: ?>>
      %175 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<64x64xbf16>
      %176 = "memref.subview"(%174, %76) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 64>, static_strides = array<i64: 1, 1>}> : (memref<64x64xbf16, strided<[512, 1], offset: ?>>, index) -> memref<?x64xbf16, strided<[512, 1], offset: ?>>
      %177 = "memref.subview"(%175, %76) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 64>, static_strides = array<i64: 1, 1>}> : (memref<64x64xbf16>, index) -> memref<?x64xbf16, strided<[64, 1]>>
      "hivm.hir.load"(%176, %177, %3, %11, %77) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x64xbf16, strided<[512, 1], offset: ?>>, memref<?x64xbf16, strided<[64, 1]>>, bf16, index, i1) -> ()
      %178 = "bufferization.to_tensor"(%175) <{restrict, writable}> : (memref<64x64xbf16>) -> tensor<64x64xbf16>
      %179 = "hivm.hir.vcast"(%178, %26) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<64x64xbf16>, tensor<64x64xf32>) -> tensor<64x64xf32>
      %180 = "hivm.hir.mmadL1"(%80, %64, %1, %76, %5, %4, %26) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_for_result_already_inserted = true} : (tensor<64x128xbf16>, tensor<128x64xbf16>, i1, index, index, index, tensor<64x64xf32>) -> tensor<64x64xf32>
      %181 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<64x64xf32>
      %182 = "bufferization.to_tensor"(%181) <{restrict, writable}> : (memref<64x64xf32>) -> tensor<64x64xf32>
      %183 = "hivm.hir.fixpipe"(%180, %182) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<64x64xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
      %184 = "hivm.hir.load"(%183, %26) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<64x64xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
      %185 = "hivm.hir.vsub"(%179, %184, %26) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x64xf32>, tensor<64x64xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
      %186 = "affine.apply"(%84) <{map = #map1}> : (index) -> index
      %187 = "memref.reinterpret_cast"(%arg6, %186) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64, 64>, static_strides = array<i64: 512, 1>}> : (memref<?xbf16>, index) -> memref<64x64xbf16, strided<[512, 1], offset: ?>>
      %188 = "hivm.hir.vcast"(%185, %151) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<64x64xf32>, tensor<64x64xbf16>) -> tensor<64x64xbf16>
      %189 = "affine.apply"(%186, %40) <{map = #map4}> : (index, index) -> index
      %190 = "affine.apply"(%189) <{map = #map6}> : (index) -> index
      %191 = "affine.apply"(%72, %190) <{map = #map4}> : (index, index) -> index
      %192 = "affine.max"(%191) <{map = #map7}> : (index) -> index
      %193 = "affine.min"(%192) <{map = #map5}> : (index) -> index
      %194 = "affine.apply"(%189) <{map = #map8}> : (index) -> index
      %195 = "affine.apply"(%194) <{map = #map9}> : (index) -> index
      %196 = "affine.max"(%195) <{map = #map7}> : (index) -> index
      %197 = "affine.min"(%196) <{map = #map5}> : (index) -> index
      %198 = "affine.min"(%97, %193) <{map = #map3}> : (index, index) -> index
      %199 = "affine.apply"(%193, %198) <{map = #map4}> : (index, index) -> index
      %200 = "affine.min"(%197) <{map = #map7}> : (index) -> index
      %201 = "affine.apply"(%197, %200) <{map = #map4}> : (index, index) -> index
      %202 = "tensor.extract_slice"(%188, %198, %200, %199, %201) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<64x64xbf16>, index, index, index, index) -> tensor<?x?xbf16>
      %203 = "memref.subview"(%187, %199, %201) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<64x64xbf16, strided<[512, 1], offset: ?>>, index, index) -> memref<?x?xbf16, strided<[512, 1], offset: ?>>
      "hivm.hir.store"(%202, %203) : (tensor<?x?xbf16>, memref<?x?xbf16, strided<[512, 1], offset: ?>>) -> ()
      %204 = "hivm.hir.vmul"(%185, %159, %26) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x64xf32>, tensor<64x64xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
      %205 = "hivm.hir.vmul"(%arg15, %133, %27) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128x64xf32>, f32, tensor<128x64xf32>) -> tensor<128x64xf32>
      %206 = "hivm.hir.vcast"(%204, %151) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<64x64xf32>, tensor<64x64xbf16>) -> tensor<64x64xbf16>
      %207 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<64x64xbf16>
      %208 = "bufferization.to_tensor"(%207) <{restrict, writable}> : (memref<64x64xbf16>) -> tensor<64x64xbf16>
      %209 = "hivm.hir.store"(%206, %208) {"inserted-store"} : (tensor<64x64xbf16>, tensor<64x64xbf16>) -> tensor<64x64xbf16>
      %210 = "hivm.hir.load"(%209, %151) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<64x64xbf16>, tensor<64x64xbf16>) -> tensor<64x64xbf16>
      %211 = "hivm.hir.mmadL1"(%108, %210, %1, %101, %99, %4, %27) <{a_transpose, operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_for_result_already_inserted = true} : (tensor<64x128xbf16>, tensor<64x64xbf16>, i1, index, index, index, tensor<128x64xf32>) -> tensor<128x64xf32>
      %212 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<128x64xf32>
      %213 = "bufferization.to_tensor"(%212) <{restrict, writable}> : (memref<128x64xf32>) -> tensor<128x64xf32>
      %214 = "hivm.hir.fixpipe"(%211, %213) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<128x64xf32>, tensor<128x64xf32>) -> tensor<128x64xf32>
      %215 = "hivm.hir.load"(%214, %27) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<128x64xf32>, tensor<128x64xf32>) -> tensor<128x64xf32>
      %216 = "hivm.hir.vadd"(%215, %205, %27) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128x64xf32>, tensor<128x64xf32>, tensor<128x64xf32>) -> tensor<128x64xf32>
      "scf.yield"(%172, %216) : (tensor<128x64xf32>, tensor<128x64xf32>) -> ()
    }) {fixpipe_for_mmad_result_already_inserted = true} : (i32, i32, i32, tensor<128x64xf32>, tensor<128x64xf32>) -> (tensor<128x64xf32>, tensor<128x64xf32>)
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, true, true, true, false, false, false, false]> : vector<13xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<MIX>, mix_mode = "mix", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hacc.target = #hacc.target<"Ascend910B1">, hivm.module_core_type = #hivm.module_core_type<MIX>} : () -> ()

