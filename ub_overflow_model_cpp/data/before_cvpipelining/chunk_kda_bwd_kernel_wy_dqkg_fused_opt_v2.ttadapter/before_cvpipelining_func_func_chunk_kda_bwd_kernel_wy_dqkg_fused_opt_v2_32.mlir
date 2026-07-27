#map = affine_map<()[s0, s1] -> (s0 + s1)>
#map1 = affine_map<()[s0] -> (s0 + 16)>
#map2 = affine_map<()[s0, s1] -> (s0, s1)>
#map3 = affine_map<()[s0, s1] -> (s0 - s1)>
#map4 = affine_map<()[s0] -> (s0, 0)>
#map5 = affine_map<()[s0] -> (s0 * 16)>
#map6 = affine_map<()[s0] -> (s0, 16)>
#map7 = affine_map<()[s0] -> (s0 * 64)>
#map8 = affine_map<()[s0] -> (s0 * 128)>
#map9 = affine_map<()[s0] -> (s0 floordiv 128)>
#map10 = affine_map<()[s0] -> (s0 mod 128)>
#map11 = affine_map<()[s0] -> (-s0 + 64)>
#map12 = affine_map<()[s0] -> (s0, 32)>
#map13 = affine_map<()[s0] -> (s0 + 32)>
#map14 = affine_map<()[s0] -> (s0, 64)>
#map15 = affine_map<()[s0] -> (s0 floordiv 64)>
#map16 = affine_map<()[s0] -> (s0 mod 64)>
#map17 = affine_map<()[s0] -> (s0 floordiv 16)>
#map18 = affine_map<()[s0] -> (s0 mod 16)>
#map19 = affine_map<()[s0] -> (-s0 + 16)>
"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {}, {}, {}, {}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, memref<?xf32>, memref<?xf32>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, memref<?xf32>, memref<?xf32>, memref<?xbf16>, memref<?xbf16>, memref<?xf32>, memref<?xf32>, memref<?xf32>, f32, i32, f32, i32, i32, i32, i32) -> (), sym_name = "chunk_kda_bwd_kernel_wy_dqkg_fused_opt_v2"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xbf16>, %arg4: memref<?xbf16>, %arg5: memref<?xbf16>, %arg6: memref<?xbf16>, %arg7: memref<?xf32>, %arg8: memref<?xf32>, %arg9: memref<?xbf16>, %arg10: memref<?xbf16>, %arg11: memref<?xbf16>, %arg12: memref<?xbf16>, %arg13: memref<?xf32>, %arg14: memref<?xf32>, %arg15: memref<?xbf16>, %arg16: memref<?xbf16>, %arg17: memref<?xf32>, %arg18: memref<?xf32>, %arg19: memref<?xf32>, %arg20: f32, %arg21: i32, %arg22: f32, %arg23: i32, %arg24: i32, %arg25: i32, %arg26: i32):
    %0 = "arith.constant"() <{value = 1 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = true}> : () -> i1
    %2 = "arith.constant"() <{value = -1.000000e+00 : f32}> : () -> f32
    %3 = "arith.constant"() <{value = 0.693147182 : f32}> : () -> f32
    %4 = "arith.constant"() <{value = 0.000000e+00 : f16}> : () -> f16
    %5 = "arith.constant"() <{value = 4096 : i64}> : () -> i64
    %6 = "arith.constant"() <{value = 15 : i32}> : () -> i32
    %7 = "arith.constant"() <{value = 0.000000e+00 : bf16}> : () -> bf16
    %8 = "arith.constant"() <{value = 128 : i64}> : () -> i64
    %9 = "arith.constant"() <{value = 64 : i32}> : () -> i32
    %10 = "arith.constant"() <{value = 32 : i32}> : () -> i32
    %11 = "arith.constant"() <{value = 0.000000e+00 : f32}> : () -> f32
    %12 = "arith.constant"() <{value = 128 : i32}> : () -> i32
    %13 = "arith.constant"() <{value = 16 : index}> : () -> index
    %14 = "arith.constant"() <{value = 32 : index}> : () -> index
    %15 = "arith.constant"() <{value = 1 : index}> : () -> index
    %16 = "arith.constant"() <{value = 0 : index}> : () -> index
    %17 = "arith.constant"() <{value = 2 : i32}> : () -> i32
    %18 = "arith.constant"() <{value = 16 : i32}> : () -> i32
    %19 = "arith.constant"() <{value = 64 : i64}> : () -> i64
    %20 = "arith.constant"() <{value = 2 : i64}> : () -> i64
    %21 = "arith.constant"() <{value = 16 : i64}> : () -> i64
    %22 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %23 = "arith.constant"() <{value = 8 : index}> : () -> index
    "hivm.hir.set_mask_norm"() : () -> ()
    %24 = "arith.muli"(%arg24, %arg25) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %25 = "arith.muli"(%24, %arg26) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%25) <{effects = ["write"]}> {logical_block_num} : (i32) -> ()
    %26 = "hivm.hir.get_block_idx"() : () -> i64
    %27 = "arith.trunci"(%26) : (i64) -> i32
    %28 = "arith.divsi"(%27, %arg26) : (i32, i32) -> i32
    %29 = "arith.remsi"(%28, %arg25) : (i32, i32) -> i32
    %30 = "arith.muli"(%arg26, %arg25) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %31 = "arith.divsi"(%27, %30) : (i32, i32) -> i32
    %32 = "arith.remsi"(%31, %arg24) : (i32, i32) -> i32
    %33 = "tensor.empty"() : () -> tensor<16xi32>
    %34 = "tensor.empty"() : () -> tensor<16xf32>
    %35 = "hivm.hir.vbrc"(%11, %34) <{broadcast_dims = array<i64>}> : (f32, tensor<16xf32>) -> tensor<16xf32>
    %36 = "tensor.empty"() : () -> tensor<16x16xf32>
    %37 = "hivm.hir.vbrc"(%11, %36) <{broadcast_dims = array<i64>}> : (f32, tensor<16x16xf32>) -> tensor<16x16xf32>
    %38 = "tensor.empty"() : () -> tensor<32xi32>
    %39 = "tensor.empty"() : () -> tensor<32xf32>
    %40 = "hivm.hir.vbrc"(%11, %39) <{broadcast_dims = array<i64>}> : (f32, tensor<32xf32>) -> tensor<32xf32>
    %41 = "tensor.empty"() : () -> tensor<16x32xf32>
    %42 = "hivm.hir.vbrc"(%11, %41) <{broadcast_dims = array<i64>}> : (f32, tensor<16x32xf32>) -> tensor<16x32xf32>
    %43 = "tensor.empty"() : () -> tensor<16x32xbf16>
    %44 = "arith.divsi"(%29, %17) : (i32, i32) -> i32
    %45 = "arith.remsi"(%29, %17) : (i32, i32) -> i32
    %46 = "arith.addi"(%arg21, %6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %47 = "arith.divsi"(%46, %18) : (i32, i32) -> i32
    %48 = "arith.muli"(%44, %47) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %49 = "arith.addi"(%48, %32) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %50 = "arith.extsi"(%49) : (i32) -> i64
    %51 = "arith.muli"(%44, %arg21) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %52 = "arith.extsi"(%51) : (i32) -> i64
    %53 = "arith.muli"(%32, %18) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %54 = "hivm.hir.varange"(%33, %16, %15) <{operandSegmentSizes = array<i32: 1, 1, 1>}> : (tensor<16xi32>, index, index) -> tensor<16xi32>
    %55 = "hivm.hir.vadd"(%54, %53, %33) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xi32>, i32, tensor<16xi32>) -> tensor<16xi32>
    %56 = "tensor.empty"() : () -> tensor<16xi1>
    %57 = "hivm.hir.vmax"(%55, %arg21, %33) <{broadcast = array<i64>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xi32>, i32, tensor<16xi32>) -> tensor<16xi32>
    %58 = "hivm.hir.vcmp"(%57, %55, %56) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xi32>, tensor<16xi32>, tensor<16xi1>) -> tensor<16xi1>
    %59 = "hivm.hir.vnot"(%58, %56) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xi1>, tensor<16xi1>) -> tensor<16xi1>
    %60 = "arith.addi"(%53, %18) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %61 = "arith.minsi"(%arg21, %60) : (i32, i32) -> i32
    %62 = "arith.subi"(%61, %0) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %63 = "hivm.hir.vbrc"(%62, %33) <{broadcast_dims = array<i64>}> : (i32, tensor<16xi32>) -> tensor<16xi32>
    %64 = "hivm.hir.vcmp"(%55, %63, %56) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xi32>, tensor<16xi32>, tensor<16xi1>) -> tensor<16xi1>
    %65 = "arith.muli"(%45, %arg23) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %66 = "arith.extsi"(%65) : (i32) -> i64
    %67 = "arith.addi"(%66, %52) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
    %68 = "arith.muli"(%67, %19) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
    %69 = "arith.index_cast"(%68) : (i64) -> index
    %70 = "arith.muli"(%52, %20) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
    %71 = "arith.extsi"(%45) : (i32) -> i64
    %72 = "arith.addi"(%70, %71) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
    %73 = "arith.muli"(%72, %19) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
    %74 = "arith.index_cast"(%73) : (i64) -> index
    %75 = "arith.index_cast"(%67) : (i64) -> index
    %76 = "arith.muli"(%67, %21) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
    %77 = "arith.index_cast"(%76) : (i64) -> index
    %78 = "arith.muli"(%50, %20) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
    %79 = "arith.addi"(%78, %71) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
    %80 = "arith.muli"(%79, %5) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
    %81 = "arith.index_cast"(%80) : (i64) -> index
    %82 = "hivm.hir.vmax"(%55, %22, %33) <{broadcast = array<i64>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xi32>, i32, tensor<16xi32>) -> tensor<16xi32>
    %83 = "hivm.hir.vcmp"(%82, %55, %56) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xi32>, tensor<16xi32>, tensor<16xi1>) -> tensor<16xi1>
    %84 = "hivm.hir.vand"(%59, %83, %56) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xi1>, tensor<16xi1>, tensor<16xi1>) -> tensor<16xi1>
    %85 = "arith.index_cast"(%53) : (i32) -> index
    %86 = "affine.apply"(%75, %85) <{map = #map}> : (index, index) -> index
    %87 = "memref.reinterpret_cast"(%arg8, %86) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1, 16>, static_strides = array<i64: 16, 1>}> : (memref<?xf32>, index) -> memref<1x16xf32, strided<[16, 1], offset: ?>>
    %88 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<1x16xf32>
    %89 = "affine.apply"(%85) <{map = #map1}> : (index) -> index
    %90 = "arith.index_cast"(%arg21) : (i32) -> index
    %91 = "affine.max"(%85, %90) <{map = #map2}> : (index, index) -> index
    %92 = "affine.min"(%89, %91) <{map = #map2}> : (index, index) -> index
    %93 = "affine.apply"(%92, %85) <{map = #map3}> : (index, index) -> index
    %94 = "affine.max"(%85) <{map = #map4}> : (index) -> index
    %95 = "affine.min"(%89, %94) <{map = #map2}> : (index, index) -> index
    %96 = "affine.apply"(%95, %85) <{map = #map3}> : (index, index) -> index
    %97 = "affine.apply"(%89, %95) <{map = #map3}> : (index, index) -> index
    %98 = "affine.max"(%96) <{map = #map4}> : (index) -> index
    %99 = "affine.apply"(%96, %97) <{map = #map}> : (index, index) -> index
    %100 = "affine.min"(%93, %99) <{map = #map2}> : (index, index) -> index
    %101 = "affine.apply"(%100, %98) <{map = #map3}> : (index, index) -> index
    %102 = "arith.cmpi"(%101, %13) <{predicate = 2 : i64}> : (index, index) -> i1
    %103 = "memref.subview"(%87, %98, %101) <{operandSegmentSizes = array<i32: 1, 1, 1, 0>, static_offsets = array<i64: 0, -9223372036854775808>, static_sizes = array<i64: 1, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<1x16xf32, strided<[16, 1], offset: ?>>, index, index) -> memref<1x?xf32, strided<[16, 1], offset: ?>>
    %104 = "memref.subview"(%88, %98, %101) <{operandSegmentSizes = array<i32: 1, 1, 1, 0>, static_offsets = array<i64: 0, -9223372036854775808>, static_sizes = array<i64: 1, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<1x16xf32>, index, index) -> memref<1x?xf32, strided<[16, 1], offset: ?>>
    %105 = "arith.remui"(%98, %23) : (index, index) -> index
    "hivm.hir.load"(%103, %104, %11, %105, %102) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<1x?xf32, strided<[16, 1], offset: ?>>, memref<1x?xf32, strided<[16, 1], offset: ?>>, f32, index, i1) -> ()
    %106 = "bufferization.to_tensor"(%88) <{restrict, writable}> : (memref<1x16xf32>) -> tensor<1x16xf32>
    %107 = "affine.apply"(%85) <{map = #map5}> : (index) -> index
    %108 = "affine.apply"(%77, %107) <{map = #map}> : (index, index) -> index
    %109 = "memref.reinterpret_cast"(%arg9, %108) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 1, 16>}> : (memref<?xbf16>, index) -> memref<16x16xbf16, strided<[1, 16], offset: ?>>
    %110 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x16xbf16>
    %111 = "affine.max"(%98) <{map = #map4}> : (index) -> index
    %112 = "affine.min"(%100) <{map = #map6}> : (index) -> index
    %113 = "affine.apply"(%112, %111) <{map = #map3}> : (index, index) -> index
    %114 = "arith.cmpi"(%113, %13) <{predicate = 2 : i64}> : (index, index) -> i1
    %115 = "memref.subview"(%109, %111, %113) <{operandSegmentSizes = array<i32: 1, 1, 1, 0>, static_offsets = array<i64: 0, -9223372036854775808>, static_sizes = array<i64: 16, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16, strided<[1, 16], offset: ?>>, index, index) -> memref<16x?xbf16, strided<[1, 16], offset: ?>>
    %116 = "memref.subview"(%110, %111, %113) <{operandSegmentSizes = array<i32: 1, 1, 1, 0>, static_offsets = array<i64: 0, -9223372036854775808>, static_sizes = array<i64: 16, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16>, index, index) -> memref<16x?xbf16, strided<[16, 1], offset: ?>>
    %117 = "arith.remui"(%111, %13) : (index, index) -> index
    "hivm.hir.load"(%115, %116, %7, %117, %114) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = true, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<16x?xbf16, strided<[1, 16], offset: ?>>, memref<16x?xbf16, strided<[16, 1], offset: ?>>, bf16, index, i1) -> ()
    "annotation.mark"(%110) <{effects = ["write"]}> {MayImplicitTransposeWithLastAxis} : (memref<16x16xbf16>) -> ()
    %118 = "bufferization.to_tensor"(%110) <{restrict, writable}> : (memref<16x16xbf16>) -> tensor<16x16xbf16>
    %119 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xbf16>
    %120 = "bufferization.to_tensor"(%119) <{restrict, writable}> : (memref<16x16xbf16>) -> tensor<16x16xbf16>
    %121 = "hivm.hir.store"(%118, %120) {"inserted-store"} : (tensor<16x16xbf16>, tensor<16x16xbf16>) -> tensor<16x16xbf16>
    %122 = "tensor.empty"() : () -> tensor<16x16xbf16>
    %123 = "hivm.hir.load"(%121, %122) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x16xbf16>, tensor<16x16xbf16>) -> tensor<16x16xbf16>
    %124 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xbf16>
    %125 = "bufferization.to_tensor"(%124) <{restrict, writable}> : (memref<16x16xbf16>) -> tensor<16x16xbf16>
    %126 = "hivm.hir.store"(%118, %125) {"inserted-store"} : (tensor<16x16xbf16>, tensor<16x16xbf16>) -> tensor<16x16xbf16>
    %127 = "hivm.hir.load"(%126, %122) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x16xbf16>, tensor<16x16xbf16>) -> tensor<16x16xbf16>
    %128 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xbf16>
    %129 = "bufferization.to_tensor"(%128) <{restrict, writable}> : (memref<16x16xbf16>) -> tensor<16x16xbf16>
    %130 = "hivm.hir.store"(%118, %129) {"inserted-store"} : (tensor<16x16xbf16>, tensor<16x16xbf16>) -> tensor<16x16xbf16>
    %131 = "hivm.hir.load"(%130, %122) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x16xbf16>, tensor<16x16xbf16>) -> tensor<16x16xbf16>
    %132 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xbf16>
    %133 = "bufferization.to_tensor"(%132) <{restrict, writable}> : (memref<16x16xbf16>) -> tensor<16x16xbf16>
    %134 = "hivm.hir.store"(%118, %133) {"inserted-store"} : (tensor<16x16xbf16>, tensor<16x16xbf16>) -> tensor<16x16xbf16>
    %135 = "hivm.hir.load"(%134, %122) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x16xbf16>, tensor<16x16xbf16>) -> tensor<16x16xbf16>
    "annotation.mark"(%118) <{effects = ["write"]}> {MayImplicitTransposeWithLastAxis} : (tensor<16x16xbf16>) -> ()
    %136 = "hivm.hir.varange"(%38, %16, %15) <{operandSegmentSizes = array<i32: 1, 1, 1>}> : (tensor<32xi32>, index, index) -> tensor<32xi32>
    %137 = "arith.extsi"(%62) : (i32) -> i64
    %138 = "arith.muli"(%137, %8) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
    %139 = "arith.addi"(%73, %138) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
    %140 = "arith.index_cast"(%139) : (i64) -> index
    %141 = "affine.apply"(%85) <{map = #map7}> : (index) -> index
    %142 = "affine.apply"(%69, %141) <{map = #map}> : (index, index) -> index
    %143 = "tensor.empty"() : () -> tensor<16x32xi1>
    %144 = "tensor.empty"() : () -> tensor<16xf16>
    %145 = "hivm.hir.vcast"(%84, %144) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<trunc>, transpose = array<i64>}> : (tensor<16xi1>, tensor<16xf16>) -> tensor<16xf16>
    %146 = "tensor.empty"() : () -> tensor<16x32xf16>
    %147 = "tensor.expand_shape"(%145) <{reassociation = [[0, 1]], static_output_shape = array<i64: 16, 1>}> : (tensor<16xf16>) -> tensor<16x1xf16>
    %148 = "hivm.hir.vbrc"(%147, %146) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xf16>, tensor<16x32xf16>) -> tensor<16x32xf16>
    %149 = "hivm.hir.vcmp"(%148, %4, %143) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf16>, f16, tensor<16x32xi1>) -> tensor<16x32xi1>
    %150 = "hivm.hir.vnot"(%149, %143) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16x32xi1>, tensor<16x32xi1>) -> tensor<16x32xi1>
    %151 = "tensor.expand_shape"(%106) <{reassociation = [[0], [1, 2]], static_output_shape = array<i64: 1, 16, 1>}> : (tensor<1x16xf32>) -> tensor<1x16x1xf32>
    %152 = "tensor.collapse_shape"(%151) <{reassociation = [[0, 1], [2]]}> : (tensor<1x16x1xf32>) -> tensor<16x1xf32>
    %153 = "hivm.hir.vbrc"(%152, %41) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
    %154 = "tensor.expand_shape"(%64) <{reassociation = [[0, 1]], static_output_shape = array<i64: 16, 1>}> : (tensor<16xi1>) -> tensor<16x1xi1>
    %155 = "tensor.empty"() : () -> tensor<16x1xf32>
    %156 = "hivm.hir.vcast"(%154, %155) <{broadcast = array<i64>, cast = #hivm.cast<cast_unsigned>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x1xi1>, tensor<16x1xf32>) -> tensor<16x1xf32>
    %157 = "hivm.hir.vbrc"(%156, %41) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
    %158:2 = "scf.for"(%22, %17, %0, %37, %35) ({
    ^bb0(%arg27: i32, %arg28: tensor<16x16xf32>, %arg29: tensor<16xf32>):
      %232 = "arith.muli"(%arg27, %10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %233 = "arith.maxsi"(%53, %22) : (i32, i32) -> i32
      %234 = "arith.index_cast"(%233) : (i32) -> index
      %235 = "arith.maxsi"(%232, %22) : (i32, i32) -> i32
      %236 = "arith.index_cast"(%235) : (i32) -> index
      %237 = "affine.apply"(%234) <{map = #map8}> : (index) -> index
      %238 = "affine.apply"(%237, %74) <{map = #map}> : (index, index) -> index
      %239 = "affine.apply"(%238, %236) <{map = #map}> : (index, index) -> index
      %240 = "memref.reinterpret_cast"(%arg4, %239) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 32>, static_strides = array<i64: 128, 1>}> : (memref<?xbf16>, index) -> memref<16x32xbf16, strided<[128, 1], offset: ?>>
      %241 = "memref.reinterpret_cast"(%arg7, %239) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 32>, static_strides = array<i64: 128, 1>}> : (memref<?xf32>, index) -> memref<16x32xf32, strided<[128, 1], offset: ?>>
      %242 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x32xbf16>
      %243 = "affine.apply"(%239, %74) <{map = #map3}> : (index, index) -> index
      %244 = "affine.apply"(%243) <{map = #map9}> : (index) -> index
      %245 = "affine.apply"(%90, %244) <{map = #map3}> : (index, index) -> index
      %246 = "affine.max"(%245) <{map = #map4}> : (index) -> index
      %247 = "affine.min"(%246) <{map = #map6}> : (index) -> index
      %248 = "affine.apply"(%243) <{map = #map10}> : (index) -> index
      %249 = "affine.apply"(%248) <{map = #map11}> : (index) -> index
      %250 = "affine.max"(%249) <{map = #map4}> : (index) -> index
      %251 = "affine.min"(%250) <{map = #map12}> : (index) -> index
      %252 = "arith.subi"(%22, %53) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %253 = "arith.maxsi"(%252, %22) : (i32, i32) -> i32
      %254 = "arith.index_cast"(%253) : (i32) -> index
      %255 = "affine.min"(%254, %247) <{map = #map2}> : (index, index) -> index
      %256 = "affine.apply"(%247, %255) <{map = #map3}> : (index, index) -> index
      %257 = "arith.subi"(%22, %232) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %258 = "arith.maxsi"(%257, %22) : (i32, i32) -> i32
      %259 = "arith.index_cast"(%258) : (i32) -> index
      %260 = "affine.min"(%259, %251) <{map = #map2}> : (index, index) -> index
      %261 = "affine.apply"(%251, %260) <{map = #map3}> : (index, index) -> index
      %262 = "arith.cmpi"(%256, %13) <{predicate = 2 : i64}> : (index, index) -> i1
      %263 = "arith.cmpi"(%261, %14) <{predicate = 2 : i64}> : (index, index) -> i1
      %264 = "arith.ori"(%262, %263) : (i1, i1) -> i1
      %265 = "memref.subview"(%240, %256, %261) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x32xbf16, strided<[128, 1], offset: ?>>, index, index) -> memref<?x?xbf16, strided<[128, 1], offset: ?>>
      %266 = "memref.subview"(%242, %255, %260, %256, %261) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x32xbf16>, index, index, index, index) -> memref<?x?xbf16, strided<[32, 1], offset: ?>>
      %267 = "arith.remui"(%260, %13) : (index, index) -> index
      "hivm.hir.load"(%265, %266, %7, %267, %264) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x?xbf16, strided<[128, 1], offset: ?>>, memref<?x?xbf16, strided<[32, 1], offset: ?>>, bf16, index, i1) -> ()
      %268 = "bufferization.to_tensor"(%242) <{restrict, writable}> : (memref<16x32xbf16>) -> tensor<16x32xbf16>
      %269 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x32xf32>
      %270 = "memref.subview"(%241, %256, %261) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x32xf32, strided<[128, 1], offset: ?>>, index, index) -> memref<?x?xf32, strided<[128, 1], offset: ?>>
      %271 = "memref.subview"(%269, %255, %260, %256, %261) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x32xf32>, index, index, index, index) -> memref<?x?xf32, strided<[32, 1], offset: ?>>
      %272 = "arith.remui"(%260, %23) : (index, index) -> index
      "hivm.hir.load"(%270, %271, %11, %272, %264) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x?xf32, strided<[128, 1], offset: ?>>, memref<?x?xf32, strided<[32, 1], offset: ?>>, f32, index, i1) -> ()
      %273 = "bufferization.to_tensor"(%269) <{restrict, writable}> : (memref<16x32xf32>) -> tensor<16x32xf32>
      %274 = "arith.index_cast"(%232) : (i32) -> index
      %275 = "affine.apply"(%140, %274) <{map = #map}> : (index, index) -> index
      %276 = "memref.reinterpret_cast"(%arg7, %275) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 32>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<32xf32, strided<[1], offset: ?>>
      %277 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<32xf32>
      %278 = "affine.apply"(%274) <{map = #map13}> : (index) -> index
      %279 = "affine.max"(%274) <{map = #map14}> : (index) -> index
      %280 = "affine.min"(%278, %279) <{map = #map2}> : (index, index) -> index
      %281 = "affine.apply"(%280, %274) <{map = #map3}> : (index, index) -> index
      %282 = "arith.cmpi"(%281, %14) <{predicate = 2 : i64}> : (index, index) -> i1
      %283 = "memref.subview"(%276, %281) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<32xf32, strided<[1], offset: ?>>, index) -> memref<?xf32, strided<[1], offset: ?>>
      %284 = "memref.subview"(%277, %281) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<32xf32>, index) -> memref<?xf32, strided<[1]>>
      "hivm.hir.load"(%283, %284, %11, %16, %282) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xf32, strided<[1], offset: ?>>, memref<?xf32, strided<[1]>>, f32, index, i1) -> ()
      %285 = "bufferization.to_tensor"(%277) <{restrict, writable}> : (memref<32xf32>) -> tensor<32xf32>
      %286 = "tensor.extract_slice"(%285, %281) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<32xf32>, index) -> tensor<?xf32>
      %287 = "tensor.insert_slice"(%286, %40, %281) <{operandSegmentSizes = array<i32: 1, 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<?xf32>, tensor<32xf32>, index) -> tensor<32xf32>
      %288 = "arith.cmpi"(%arg27, %22) <{predicate = 0 : i64}> : (i32, i32) -> i1
      %289:6 = "scf.for"(%22, %17, %0, %arg28, %arg29, %42, %42, %42, %40) ({
      ^bb0(%arg30: i32, %arg31: tensor<16x16xf32>, %arg32: tensor<16xf32>, %arg33: tensor<16x32xf32>, %arg34: tensor<16x32xf32>, %arg35: tensor<16x32xf32>, %arg36: tensor<32xf32>):
        %392 = "arith.muli"(%arg30, %10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
        %393 = "hivm.hir.vadd"(%136, %392, %38) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32xi32>, i32, tensor<32xi32>) -> tensor<32xi32>
        %394 = "tensor.empty"() : () -> tensor<32xi1>
        %395 = "hivm.hir.vmax"(%393, %9, %38) <{broadcast = array<i64>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32xi32>, i32, tensor<32xi32>) -> tensor<32xi32>
        %396 = "hivm.hir.vcmp"(%395, %393, %394) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32xi32>, tensor<32xi32>, tensor<32xi1>) -> tensor<32xi1>
        %397 = "hivm.hir.vnot"(%396, %394) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<32xi1>, tensor<32xi1>) -> tensor<32xi1>
        %398 = "hivm.hir.vmax"(%393, %22, %38) <{broadcast = array<i64>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32xi32>, i32, tensor<32xi32>) -> tensor<32xi32>
        %399 = "hivm.hir.vcmp"(%398, %393, %394) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32xi32>, tensor<32xi32>, tensor<32xi1>) -> tensor<32xi1>
        %400 = "hivm.hir.vand"(%397, %399, %394) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32xi1>, tensor<32xi1>, tensor<32xi1>) -> tensor<32xi1>
        %401 = "arith.index_cast"(%392) : (i32) -> index
        %402 = "affine.apply"(%142, %401) <{map = #map}> : (index, index) -> index
        %403 = "memref.reinterpret_cast"(%arg6, %402) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 32>, static_strides = array<i64: 64, 1>}> : (memref<?xbf16>, index) -> memref<16x32xbf16, strided<[64, 1], offset: ?>>
        %404 = "memref.reinterpret_cast"(%arg11, %402) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 32>, static_strides = array<i64: 64, 1>}> : (memref<?xbf16>, index) -> memref<16x32xbf16, strided<[64, 1], offset: ?>>
        %405 = "tensor.empty"() : () -> tensor<32xf16>
        %406 = "hivm.hir.vcast"(%400, %405) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<trunc>, transpose = array<i64>}> : (tensor<32xi1>, tensor<32xf16>) -> tensor<32xf16>
        %407 = "tensor.expand_shape"(%406) <{reassociation = [[0, 1]], static_output_shape = array<i64: 1, 32>}> : (tensor<32xf16>) -> tensor<1x32xf16>
        %408 = "hivm.hir.vbrc"(%407, %146) <{broadcast_dims = array<i64: 0>}> : (tensor<1x32xf16>, tensor<16x32xf16>) -> tensor<16x32xf16>
        %409 = "hivm.hir.vcmp"(%408, %4, %143) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf16>, f16, tensor<16x32xi1>) -> tensor<16x32xi1>
        %410 = "hivm.hir.vnot"(%409, %143) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16x32xi1>, tensor<16x32xi1>) -> tensor<16x32xi1>
        %411 = "hivm.hir.vand"(%150, %410, %143) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xi1>, tensor<16x32xi1>, tensor<16x32xi1>) -> tensor<16x32xi1>
        %412 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x32xbf16>
        %413 = "affine.apply"(%401) <{map = #map13}> : (index) -> index
        %414 = "affine.max"(%401) <{map = #map14}> : (index) -> index
        %415 = "affine.min"(%413, %414) <{map = #map2}> : (index, index) -> index
        %416 = "affine.apply"(%415, %401) <{map = #map3}> : (index, index) -> index
        %417 = "affine.max"(%401) <{map = #map4}> : (index) -> index
        %418 = "affine.min"(%413, %417) <{map = #map2}> : (index, index) -> index
        %419 = "affine.apply"(%418, %401) <{map = #map3}> : (index, index) -> index
        %420 = "affine.apply"(%413, %418) <{map = #map3}> : (index, index) -> index
        %421 = "affine.max"(%419) <{map = #map4}> : (index) -> index
        %422 = "affine.apply"(%419, %420) <{map = #map}> : (index, index) -> index
        %423 = "affine.min"(%416, %422) <{map = #map2}> : (index, index) -> index
        %424 = "affine.max"(%421) <{map = #map4}> : (index) -> index
        %425 = "affine.min"(%423) <{map = #map12}> : (index) -> index
        %426 = "affine.apply"(%425, %424) <{map = #map3}> : (index, index) -> index
        %427 = "arith.cmpi"(%426, %14) <{predicate = 2 : i64}> : (index, index) -> i1
        %428 = "arith.ori"(%114, %427) : (i1, i1) -> i1
        %429 = "memref.subview"(%403, %111, %424, %113, %426) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x32xbf16, strided<[64, 1], offset: ?>>, index, index, index, index) -> memref<?x?xbf16, strided<[64, 1], offset: ?>>
        %430 = "memref.subview"(%412, %111, %424, %113, %426) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x32xbf16>, index, index, index, index) -> memref<?x?xbf16, strided<[32, 1], offset: ?>>
        %431 = "arith.remui"(%424, %13) : (index, index) -> index
        "hivm.hir.load"(%429, %430, %7, %431, %428) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x?xbf16, strided<[64, 1], offset: ?>>, memref<?x?xbf16, strided<[32, 1], offset: ?>>, bf16, index, i1) -> ()
        %432 = "bufferization.to_tensor"(%412) <{restrict, writable}> : (memref<16x32xbf16>) -> tensor<16x32xbf16>
        %433 = "hivm.hir.vsel"(%411, %432, %7, %43) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<16x32xi1>, tensor<16x32xbf16>, bf16, tensor<16x32xbf16>) -> tensor<16x32xbf16>
        %434 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x32xbf16>
        %435 = "bufferization.to_tensor"(%434) <{restrict, writable}> : (memref<16x32xbf16>) -> tensor<16x32xbf16>
        %436 = "hivm.hir.store"(%433, %435) {"inserted-store"} : (tensor<16x32xbf16>, tensor<16x32xbf16>) -> tensor<16x32xbf16>
        %437 = "hivm.hir.load"(%436, %43) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x32xbf16>, tensor<16x32xbf16>) -> tensor<16x32xbf16>
        %438 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x32xbf16>
        %439 = "memref.subview"(%404, %111, %424, %113, %426) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x32xbf16, strided<[64, 1], offset: ?>>, index, index, index, index) -> memref<?x?xbf16, strided<[64, 1], offset: ?>>
        %440 = "memref.subview"(%438, %111, %424, %113, %426) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x32xbf16>, index, index, index, index) -> memref<?x?xbf16, strided<[32, 1], offset: ?>>
        "hivm.hir.load"(%439, %440, %7, %431, %428) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x?xbf16, strided<[64, 1], offset: ?>>, memref<?x?xbf16, strided<[32, 1], offset: ?>>, bf16, index, i1) -> ()
        %441 = "bufferization.to_tensor"(%438) <{restrict, writable}> : (memref<16x32xbf16>) -> tensor<16x32xbf16>
        %442 = "hivm.hir.vsel"(%411, %441, %7, %43) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<16x32xi1>, tensor<16x32xbf16>, bf16, tensor<16x32xbf16>) -> tensor<16x32xbf16>
        %443 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x32xbf16>
        %444 = "bufferization.to_tensor"(%443) <{restrict, writable}> : (memref<16x32xbf16>) -> tensor<16x32xbf16>
        %445 = "hivm.hir.store"(%442, %444) {"inserted-store"} : (tensor<16x32xbf16>, tensor<16x32xbf16>) -> tensor<16x32xbf16>
        %446 = "hivm.hir.load"(%445, %43) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x32xbf16>, tensor<16x32xbf16>) -> tensor<16x32xbf16>
        %447 = "tensor.empty"() : () -> tensor<32x32xbf16>
        %448 = "scf.for"(%16, %14, %15, %447) ({
        ^bb0(%arg45: index, %arg46: tensor<32x32xbf16>):
          %595 = "scf.for"(%16, %14, %15, %arg46) ({
          ^bb0(%arg47: index, %arg48: tensor<32x32xbf16>):
            %596 = "arith.index_cast"(%arg45) : (index) -> i64
            %597 = "arith.extsi"(%392) : (i32) -> i64
            %598 = "arith.addi"(%597, %596) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
            %599 = "arith.index_cast"(%arg47) : (index) -> i64
            %600 = "arith.extsi"(%232) : (i32) -> i64
            %601 = "arith.muli"(%600, %19) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
            %602 = "arith.muli"(%599, %19) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
            %603 = "arith.addi"(%598, %601) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
            %604 = "arith.addi"(%603, %602) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
            %605 = "arith.index_cast"(%604) : (i64) -> index
            %606 = "affine.apply"(%81, %605) <{map = #map}> : (index, index) -> index
            %607 = "memref.reinterpret_cast"(%arg10, %606) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xbf16>, index) -> memref<1xbf16, strided<[1], offset: ?>>
            %608 = "memref.load"(%607, %16) : (memref<1xbf16, strided<[1], offset: ?>>, index) -> bf16
            %609 = "tensor.insert"(%608, %arg48, %arg45, %arg47) : (bf16, tensor<32x32xbf16>, index, index) -> tensor<32x32xbf16>
            "scf.yield"(%609) {DiscreteMemAccess} : (tensor<32x32xbf16>) -> ()
          }) {ExtractedLoadOrStore} : (index, index, index, tensor<32x32xbf16>) -> tensor<32x32xbf16>
          "scf.yield"(%595) : (tensor<32x32xbf16>) -> ()
        }) {ExtractedLoadOrStore} : (index, index, index, tensor<32x32xbf16>) -> tensor<32x32xbf16>
        %449 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<32x32xbf16>
        %450 = "bufferization.to_tensor"(%449) <{restrict, writable}> : (memref<32x32xbf16>) -> tensor<32x32xbf16>
        %451 = "hivm.hir.store"(%448, %450) {"inserted-store"} : (tensor<32x32xbf16>, tensor<32x32xbf16>) -> tensor<32x32xbf16>
        %452 = "hivm.hir.load"(%451, %447) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<32x32xbf16>, tensor<32x32xbf16>) -> tensor<32x32xbf16>
        %453 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<32x32xbf16>
        %454 = "bufferization.to_tensor"(%453) <{restrict, writable}> : (memref<32x32xbf16>) -> tensor<32x32xbf16>
        %455 = "hivm.hir.store"(%448, %454) {"inserted-store"} : (tensor<32x32xbf16>, tensor<32x32xbf16>) -> tensor<32x32xbf16>
        %456 = "hivm.hir.load"(%455, %447) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<32x32xbf16>, tensor<32x32xbf16>) -> tensor<32x32xbf16>
        %457 = "scf.for"(%16, %14, %15, %447) ({
        ^bb0(%arg41: index, %arg42: tensor<32x32xbf16>):
          %580 = "scf.for"(%16, %14, %15, %arg42) ({
          ^bb0(%arg43: index, %arg44: tensor<32x32xbf16>):
            %581 = "arith.index_cast"(%arg41) : (index) -> i64
            %582 = "arith.extsi"(%392) : (i32) -> i64
            %583 = "arith.addi"(%582, %581) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
            %584 = "arith.index_cast"(%arg43) : (index) -> i64
            %585 = "arith.extsi"(%232) : (i32) -> i64
            %586 = "arith.muli"(%585, %19) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
            %587 = "arith.muli"(%584, %19) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
            %588 = "arith.addi"(%583, %586) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
            %589 = "arith.addi"(%588, %587) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
            %590 = "arith.index_cast"(%589) : (i64) -> index
            %591 = "affine.apply"(%81, %590) <{map = #map}> : (index, index) -> index
            %592 = "memref.reinterpret_cast"(%arg12, %591) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xbf16>, index) -> memref<1xbf16, strided<[1], offset: ?>>
            %593 = "memref.load"(%592, %16) : (memref<1xbf16, strided<[1], offset: ?>>, index) -> bf16
            %594 = "tensor.insert"(%593, %arg44, %arg41, %arg43) : (bf16, tensor<32x32xbf16>, index, index) -> tensor<32x32xbf16>
            "scf.yield"(%594) {DiscreteMemAccess} : (tensor<32x32xbf16>) -> ()
          }) {ExtractedLoadOrStore} : (index, index, index, tensor<32x32xbf16>) -> tensor<32x32xbf16>
          "scf.yield"(%580) : (tensor<32x32xbf16>) -> ()
        }) {ExtractedLoadOrStore} : (index, index, index, tensor<32x32xbf16>) -> tensor<32x32xbf16>
        %458 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<32x32xbf16>
        %459 = "bufferization.to_tensor"(%458) <{restrict, writable}> : (memref<32x32xbf16>) -> tensor<32x32xbf16>
        %460 = "hivm.hir.store"(%457, %459) {"inserted-store"} : (tensor<32x32xbf16>, tensor<32x32xbf16>) -> tensor<32x32xbf16>
        %461 = "hivm.hir.load"(%460, %447) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<32x32xbf16>, tensor<32x32xbf16>) -> tensor<32x32xbf16>
        %462 = "affine.max"(%111) <{map = #map4}> : (index) -> index
        %463 = "affine.min"(%462) <{map = #map6}> : (index) -> index
        %464 = "affine.apply"(%463, %113) <{map = #map}> : (index, index) -> index
        %465 = "affine.min"(%464) <{map = #map6}> : (index) -> index
        %466 = "scf.for"(%463, %465, %15, %43) ({
        ^bb0(%arg37: index, %arg38: tensor<16x32xbf16>):
          %562 = "affine.max"(%424) <{map = #map4}> : (index) -> index
          %563 = "affine.min"(%562) <{map = #map12}> : (index) -> index
          %564 = "affine.apply"(%563, %426) <{map = #map}> : (index, index) -> index
          %565 = "affine.min"(%564) <{map = #map12}> : (index) -> index
          %566 = "scf.for"(%563, %565, %15, %arg38) ({
          ^bb0(%arg39: index, %arg40: tensor<16x32xbf16>):
            %567 = "arith.index_cast"(%arg37) : (index) -> i32
            %568 = "arith.addi"(%53, %567) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
            %569 = "arith.muli"(%568, %12) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
            %570 = "arith.extsi"(%569) : (i32) -> i64
            %571 = "arith.addi"(%73, %570) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
            %572 = "arith.index_cast"(%arg39) : (index) -> i32
            %573 = "arith.addi"(%392, %572) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
            %574 = "arith.extsi"(%573) : (i32) -> i64
            %575 = "arith.addi"(%571, %574) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
            %576 = "arith.index_cast"(%575) : (i64) -> index
            %577 = "memref.reinterpret_cast"(%arg15, %576) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xbf16>, index) -> memref<1xbf16, strided<[1], offset: ?>>
            %578 = "memref.load"(%577, %16) : (memref<1xbf16, strided<[1], offset: ?>>, index) -> bf16
            %579 = "tensor.insert"(%578, %arg40, %arg37, %arg39) : (bf16, tensor<16x32xbf16>, index, index) -> tensor<16x32xbf16>
            "scf.yield"(%579) {DiscreteMemAccess} : (tensor<16x32xbf16>) -> ()
          }) {ExtractedLoadOrStore} : (index, index, index, tensor<16x32xbf16>) -> tensor<16x32xbf16>
          "scf.yield"(%566) : (tensor<16x32xbf16>) -> ()
        }) {ExtractedLoadOrStore} : (index, index, index, tensor<16x32xbf16>) -> tensor<16x32xbf16>
        %467 = "hivm.hir.vsel"(%411, %466, %7, %43) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<16x32xi1>, tensor<16x32xbf16>, bf16, tensor<16x32xbf16>) -> tensor<16x32xbf16>
        %468 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x32xbf16>
        %469 = "bufferization.to_tensor"(%468) <{restrict, writable}> : (memref<16x32xbf16>) -> tensor<16x32xbf16>
        %470 = "hivm.hir.store"(%467, %469) {"inserted-store"} : (tensor<16x32xbf16>, tensor<16x32xbf16>) -> tensor<16x32xbf16>
        %471 = "hivm.hir.load"(%470, %43) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x32xbf16>, tensor<16x32xbf16>) -> tensor<16x32xbf16>
        %472 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x32xbf16>
        %473 = "bufferization.to_tensor"(%472) <{restrict, writable}> : (memref<16x32xbf16>) -> tensor<16x32xbf16>
        %474 = "hivm.hir.store"(%467, %473) {"inserted-store"} : (tensor<16x32xbf16>, tensor<16x32xbf16>) -> tensor<16x32xbf16>
        %475 = "hivm.hir.load"(%474, %43) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x32xbf16>, tensor<16x32xbf16>) -> tensor<16x32xbf16>
        %476 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x32xbf16>
        %477 = "bufferization.to_tensor"(%476) <{restrict, writable}> : (memref<16x32xbf16>) -> tensor<16x32xbf16>
        %478 = "hivm.hir.store"(%467, %477) {"inserted-store"} : (tensor<16x32xbf16>, tensor<16x32xbf16>) -> tensor<16x32xbf16>
        %479 = "hivm.hir.load"(%478, %43) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x32xbf16>, tensor<16x32xbf16>) -> tensor<16x32xbf16>
        %480 = "tensor.empty"() : () -> tensor<32x32xf32>
        %481 = "hivm.hir.vcast"(%448, %480) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<32x32xbf16>, tensor<32x32xf32>) -> tensor<32x32xf32>
        %482 = "hivm.hir.vcast"(%457, %480) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<32x32xbf16>, tensor<32x32xf32>) -> tensor<32x32xf32>
        %483 = "hivm.hir.vmul"(%481, %482, %480) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32x32xf32>, tensor<32x32xf32>, tensor<32x32xf32>) -> tensor<32x32xf32>
        %484 = "tensor.empty"() : () -> tensor<1x32xf32>
        %485 = "hivm.hir.vreduce"(%483, %484) <{arith = #hivm.reduce_op<sum>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 0>, tie_break_left = true, unsigned_src = false}> : (tensor<32x32xf32>, tensor<1x32xf32>) -> tensor<1x32xf32>
        %486 = "tensor.collapse_shape"(%485) <{reassociation = [[0, 1]]}> : (tensor<1x32xf32>) -> tensor<32xf32>
        %487 = "hivm.hir.vadd"(%arg36, %486, %39) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32xf32>, tensor<32xf32>, tensor<32xf32>) -> tensor<32xf32>
        %488 = "hivm.hir.mmadL1"(%446, %452, %1, %13, %14, %14, %41) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_for_result_already_inserted = true} : (tensor<16x32xbf16>, tensor<32x32xbf16>, i1, index, index, index, tensor<16x32xf32>) -> tensor<16x32xf32>
        %489 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x32xf32>
        %490 = "bufferization.to_tensor"(%489) <{restrict, writable}> : (memref<16x32xf32>) -> tensor<16x32xf32>
        %491 = "hivm.hir.fixpipe"(%488, %490) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
        %492 = "hivm.hir.load"(%491, %41) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
        %493 = "hivm.hir.vmul"(%492, %arg22, %41) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, f32, tensor<16x32xf32>) -> tensor<16x32xf32>
        %494 = "hivm.hir.vadd"(%arg33, %493, %41) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
        %495 = "hivm.hir.mmadL1"(%437, %461, %1, %13, %14, %14, %41) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_for_result_already_inserted = true} : (tensor<16x32xbf16>, tensor<32x32xbf16>, i1, index, index, index, tensor<16x32xf32>) -> tensor<16x32xf32>
        %496 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x32xf32>
        %497 = "bufferization.to_tensor"(%496) <{restrict, writable}> : (memref<16x32xf32>) -> tensor<16x32xf32>
        %498 = "hivm.hir.fixpipe"(%495, %497) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
        %499 = "hivm.hir.load"(%498, %41) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
        %500 = "hivm.hir.vmul"(%499, %arg22, %41) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, f32, tensor<16x32xf32>) -> tensor<16x32xf32>
        %501 = "hivm.hir.vadd"(%arg34, %500, %41) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
        %502 = "hivm.hir.mmadL1"(%471, %456, %1, %13, %14, %14, %41) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_for_result_already_inserted = true} : (tensor<16x32xbf16>, tensor<32x32xbf16>, i1, index, index, index, tensor<16x32xf32>) -> tensor<16x32xf32>
        %503 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x32xf32>
        %504 = "bufferization.to_tensor"(%503) <{restrict, writable}> : (memref<16x32xf32>) -> tensor<16x32xf32>
        %505 = "hivm.hir.fixpipe"(%502, %504) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
        %506 = "hivm.hir.load"(%505, %41) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
        %507 = "hivm.hir.vmul"(%506, %arg22, %41) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, f32, tensor<16x32xf32>) -> tensor<16x32xf32>
        %508 = "hivm.hir.vadd"(%arg35, %507, %41) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
        "hivm.hir.pipe_barrier"() <{pipe = #hivm.pipe<PIPE_ALL>}> : () -> ()
        %509:2 = "scf.if"(%288) ({
          %512 = "affine.apply"(%85) <{map = #map8}> : (index) -> index
          %513 = "affine.apply"(%74, %512) <{map = #map}> : (index, index) -> index
          %514 = "affine.apply"(%513, %401) <{map = #map}> : (index, index) -> index
          %515 = "memref.reinterpret_cast"(%arg5, %514) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 32>, static_strides = array<i64: 128, 1>}> : (memref<?xbf16>, index) -> memref<16x32xbf16, strided<[128, 1], offset: ?>>
          %516 = "arith.maxsi"(%392, %22) : (i32, i32) -> i32
          %517 = "arith.index_cast"(%516) : (i32) -> index
          %518 = "affine.apply"(%238, %517) <{map = #map}> : (index, index) -> index
          %519 = "memref.reinterpret_cast"(%arg16, %518) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 32>, static_strides = array<i64: 128, 1>}> : (memref<?xbf16>, index) -> memref<16x32xbf16, strided<[128, 1], offset: ?>>
          %520 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x32xbf16>
          %521 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x32xbf16>
          %522 = "memref.subview"(%515, %111, %424, %113, %426) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x32xbf16, strided<[128, 1], offset: ?>>, index, index, index, index) -> memref<?x?xbf16, strided<[128, 1], offset: ?>>
          %523 = "memref.subview"(%520, %111, %424, %113, %426) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x32xbf16>, index, index, index, index) -> memref<?x?xbf16, strided<[32, 1], offset: ?>>
          %524 = "memref.subview"(%521, %111, %424, %113, %426) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x32xbf16>, index, index, index, index) -> memref<?x?xbf16, strided<[32, 1], offset: ?>>
          "hivm.hir.load"(%522, %523, %7, %431, %428) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<?x?xbf16, strided<[128, 1], offset: ?>>, memref<?x?xbf16, strided<[32, 1], offset: ?>>, bf16, index, i1) -> ()
          "hivm.hir.load"(%522, %524, %7, %431, %428) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x?xbf16, strided<[128, 1], offset: ?>>, memref<?x?xbf16, strided<[32, 1], offset: ?>>, bf16, index, i1) -> ()
          %525 = "bufferization.to_tensor"(%520) <{restrict, writable}> : (memref<16x32xbf16>) -> tensor<16x32xbf16>
          %526 = "bufferization.to_tensor"(%521) <{restrict, writable}> : (memref<16x32xbf16>) -> tensor<16x32xbf16>
          %527 = "hivm.hir.mmadL1"(%475, %525, %1, %13, %14, %113, %36) <{b_transpose, operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_for_result_already_inserted = true} : (tensor<16x32xbf16>, tensor<16x32xbf16>, i1, index, index, index, tensor<16x16xf32>) -> tensor<16x16xf32>
          %528 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xf32>
          %529 = "bufferization.to_tensor"(%528) <{restrict, writable}> : (memref<16x16xf32>) -> tensor<16x16xf32>
          %530 = "hivm.hir.fixpipe"(%527, %529) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
          %531 = "hivm.hir.load"(%530, %36) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
          %532 = "hivm.hir.vmul"(%531, %arg22, %36) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x16xf32>, f32, tensor<16x16xf32>) -> tensor<16x16xf32>
          %533 = "hivm.hir.mmadL1"(%123, %479, %1, %13, %13, %14, %41) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_for_result_already_inserted = true} : (tensor<16x16xbf16>, tensor<16x32xbf16>, i1, index, index, index, tensor<16x32xf32>) -> tensor<16x32xf32>
          %534 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x32xf32>
          %535 = "bufferization.to_tensor"(%534) <{restrict, writable}> : (memref<16x32xf32>) -> tensor<16x32xf32>
          %536 = "hivm.hir.fixpipe"(%533, %535) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
          %537 = "hivm.hir.load"(%536, %41) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
          %538 = "hivm.hir.vmul"(%537, %153, %41) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
          %539 = "hivm.hir.vcast"(%526, %41) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x32xbf16>, tensor<16x32xf32>) -> tensor<16x32xf32>
          %540 = "hivm.hir.vmul"(%537, %539, %41) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
          %541 = "hivm.hir.vreduce"(%540, %155) <{arith = #hivm.reduce_op<sum>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 1>, tie_break_left = true, unsigned_src = false}> : (tensor<16x32xf32>, tensor<16x1xf32>) -> tensor<16x1xf32>
          %542 = "tensor.collapse_shape"(%541) <{reassociation = [[0, 1]]}> : (tensor<16x1xf32>) -> tensor<16xf32>
          %543 = "hivm.hir.vcast"(%538, %43) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xbf16>) -> tensor<16x32xbf16>
          %544 = "affine.apply"(%518, %74) <{map = #map3}> : (index, index) -> index
          %545 = "affine.apply"(%544) <{map = #map9}> : (index) -> index
          %546 = "affine.apply"(%90, %545) <{map = #map3}> : (index, index) -> index
          %547 = "affine.max"(%546) <{map = #map4}> : (index) -> index
          %548 = "affine.min"(%547) <{map = #map6}> : (index) -> index
          %549 = "affine.apply"(%544) <{map = #map10}> : (index) -> index
          %550 = "affine.apply"(%549) <{map = #map11}> : (index) -> index
          %551 = "affine.max"(%550) <{map = #map4}> : (index) -> index
          %552 = "affine.min"(%551) <{map = #map12}> : (index) -> index
          %553 = "affine.min"(%254, %548) <{map = #map2}> : (index, index) -> index
          %554 = "affine.apply"(%548, %553) <{map = #map3}> : (index, index) -> index
          %555 = "arith.subi"(%22, %392) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
          %556 = "arith.maxsi"(%555, %22) : (i32, i32) -> i32
          %557 = "arith.index_cast"(%556) : (i32) -> index
          %558 = "affine.min"(%557, %552) <{map = #map2}> : (index, index) -> index
          %559 = "affine.apply"(%552, %558) <{map = #map3}> : (index, index) -> index
          %560 = "tensor.extract_slice"(%543, %553, %558, %554, %559) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<16x32xbf16>, index, index, index, index) -> tensor<?x?xbf16>
          %561 = "memref.subview"(%519, %554, %559) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x32xbf16, strided<[128, 1], offset: ?>>, index, index) -> memref<?x?xbf16, strided<[128, 1], offset: ?>>
          "hivm.hir.store"(%560, %561) : (tensor<?x?xbf16>, memref<?x?xbf16, strided<[128, 1], offset: ?>>) -> ()
          "scf.yield"(%532, %542) : (tensor<16x16xf32>, tensor<16xf32>) -> ()
        }, {
          "scf.yield"(%37, %35) : (tensor<16x16xf32>, tensor<16xf32>) -> ()
        }) : (i1) -> (tensor<16x16xf32>, tensor<16xf32>)
        %510 = "hivm.hir.vadd"(%arg31, %509#0, %36) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x16xf32>, tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
        %511 = "hivm.hir.vadd"(%arg32, %509#1, %34) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        "scf.yield"(%510, %511, %494, %501, %508, %487) : (tensor<16x16xf32>, tensor<16xf32>, tensor<16x32xf32>, tensor<16x32xf32>, tensor<16x32xf32>, tensor<32xf32>) -> ()
      }) {fixpipe_for_mmad_result_already_inserted = true} : (i32, i32, i32, tensor<16x16xf32>, tensor<16xf32>, tensor<16x32xf32>, tensor<16x32xf32>, tensor<16x32xf32>, tensor<32xf32>) -> (tensor<16x16xf32>, tensor<16xf32>, tensor<16x32xf32>, tensor<16x32xf32>, tensor<16x32xf32>, tensor<32xf32>)
      %290 = "hivm.hir.vmul"(%273, %3, %41) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, f32, tensor<16x32xf32>) -> tensor<16x32xf32>
      %291 = "hivm.hir.vexp"(%290, %41) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
      %292 = "hivm.hir.vmul"(%291, %153, %41) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
      %293 = "hivm.hir.vmul"(%287, %3, %39) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32xf32>, f32, tensor<32xf32>) -> tensor<32xf32>
      %294 = "hivm.hir.vexp"(%293, %39) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<32xf32>, tensor<32xf32>) -> tensor<32xf32>
      %295 = "hivm.hir.vmul"(%289#5, %294, %39) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32xf32>, tensor<32xf32>, tensor<32xf32>) -> tensor<32xf32>
      %296 = "hivm.hir.vmul"(%289#2, %291, %41) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
      %297 = "hivm.hir.vmul"(%296, %arg20, %41) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, f32, tensor<16x32xf32>) -> tensor<16x32xf32>
      %298 = "tensor.expand_shape"(%287) <{reassociation = [[0, 1]], static_output_shape = array<i64: 1, 32>}> : (tensor<32xf32>) -> tensor<1x32xf32>
      %299 = "hivm.hir.vbrc"(%298, %41) <{broadcast_dims = array<i64: 0>}> : (tensor<1x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
      %300 = "hivm.hir.vsub"(%299, %273, %41) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
      %301 = "hivm.hir.vmul"(%300, %3, %41) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, f32, tensor<16x32xf32>) -> tensor<16x32xf32>
      %302 = "hivm.hir.vexp"(%301, %41) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
      %303 = "tensor.extract_slice"(%302, %93) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 32>, static_strides = array<i64: 1, 1>}> : (tensor<16x32xf32>, index) -> tensor<?x32xf32>
      %304 = "tensor.insert_slice"(%303, %42, %93) <{operandSegmentSizes = array<i32: 1, 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 32>, static_strides = array<i64: 1, 1>}> : (tensor<?x32xf32>, tensor<16x32xf32>, index) -> tensor<16x32xf32>
      %305 = "hivm.hir.vmul"(%289#3, %304, %41) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
      %306 = "hivm.hir.vcast"(%268, %41) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x32xbf16>, tensor<16x32xf32>) -> tensor<16x32xf32>
      %307 = "hivm.hir.vmul"(%306, %291, %41) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
      %308 = "hivm.hir.vcast"(%289#4, %43) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xbf16>) -> tensor<16x32xbf16>
      %309 = "hivm.hir.vcast"(%308, %41) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x32xbf16>, tensor<16x32xf32>) -> tensor<16x32xf32>
      %310 = "hivm.hir.vmul"(%309, %2, %41) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, f32, tensor<16x32xf32>) -> tensor<16x32xf32>
      %311 = "hivm.hir.vadd"(%310, %11, %41) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, f32, tensor<16x32xf32>) -> tensor<16x32xf32>
      %312 = "hivm.hir.vcast"(%311, %43) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xbf16>) -> tensor<16x32xbf16>
      %313 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x32xbf16>
      %314 = "bufferization.to_tensor"(%313) <{restrict, writable}> : (memref<16x32xbf16>) -> tensor<16x32xbf16>
      %315 = "hivm.hir.store"(%312, %314) {"inserted-store"} : (tensor<16x32xbf16>, tensor<16x32xbf16>) -> tensor<16x32xbf16>
      %316 = "hivm.hir.load"(%315, %43) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x32xbf16>, tensor<16x32xbf16>) -> tensor<16x32xbf16>
      %317 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x32xbf16>
      %318 = "bufferization.to_tensor"(%317) <{restrict, writable}> : (memref<16x32xbf16>) -> tensor<16x32xbf16>
      %319 = "hivm.hir.store"(%312, %318) {"inserted-store"} : (tensor<16x32xbf16>, tensor<16x32xbf16>) -> tensor<16x32xbf16>
      %320 = "hivm.hir.load"(%319, %43) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x32xbf16>, tensor<16x32xbf16>) -> tensor<16x32xbf16>
      %321 = "hivm.hir.vcast"(%307, %43) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xbf16>) -> tensor<16x32xbf16>
      %322 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x32xbf16>
      %323 = "bufferization.to_tensor"(%322) <{restrict, writable}> : (memref<16x32xbf16>) -> tensor<16x32xbf16>
      %324 = "hivm.hir.store"(%321, %323) {"inserted-store"} : (tensor<16x32xbf16>, tensor<16x32xbf16>) -> tensor<16x32xbf16>
      %325 = "hivm.hir.load"(%324, %43) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x32xbf16>, tensor<16x32xbf16>) -> tensor<16x32xbf16>
      %326 = "hivm.hir.mmadL1"(%316, %325, %1, %13, %14, %13, %36) <{b_transpose, operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_for_result_already_inserted = true} : (tensor<16x32xbf16>, tensor<16x32xbf16>, i1, index, index, index, tensor<16x16xf32>) -> tensor<16x16xf32>
      %327 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xf32>
      %328 = "bufferization.to_tensor"(%327) <{restrict, writable}> : (memref<16x16xf32>) -> tensor<16x16xf32>
      %329 = "hivm.hir.fixpipe"(%326, %328) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
      %330 = "hivm.hir.load"(%329, %36) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
      %331 = "hivm.hir.vmul"(%330, %arg22, %36) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x16xf32>, f32, tensor<16x16xf32>) -> tensor<16x16xf32>
      %332 = "hivm.hir.vadd"(%289#0, %331, %36) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x16xf32>, tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
      %333 = "hivm.hir.mmadL1"(%127, %320, %1, %13, %13, %14, %41) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_for_result_already_inserted = true} : (tensor<16x16xbf16>, tensor<16x32xbf16>, i1, index, index, index, tensor<16x32xf32>) -> tensor<16x32xf32>
      %334 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x32xf32>
      %335 = "bufferization.to_tensor"(%334) <{restrict, writable}> : (memref<16x32xf32>) -> tensor<16x32xf32>
      %336 = "hivm.hir.fixpipe"(%333, %335) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
      %337 = "hivm.hir.load"(%336, %41) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
      %338 = "hivm.hir.vmul"(%337, %307, %41) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
      %339 = "hivm.hir.vreduce"(%338, %155) <{arith = #hivm.reduce_op<sum>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 1>, tie_break_left = true, unsigned_src = false}> : (tensor<16x32xf32>, tensor<16x1xf32>) -> tensor<16x1xf32>
      %340 = "tensor.collapse_shape"(%339) <{reassociation = [[0, 1]]}> : (tensor<16x1xf32>) -> tensor<16xf32>
      %341 = "hivm.hir.vadd"(%289#1, %340, %34) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
      %342 = "affine.apply"(%234) <{map = #map7}> : (index) -> index
      %343 = "affine.apply"(%342, %69) <{map = #map}> : (index, index) -> index
      %344 = "affine.apply"(%343, %236) <{map = #map}> : (index, index) -> index
      %345 = "memref.reinterpret_cast"(%arg3, %344) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 32>, static_strides = array<i64: 64, 1>}> : (memref<?xbf16>, index) -> memref<16x32xbf16, strided<[64, 1], offset: ?>>
      %346 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x32xbf16>
      %347 = "affine.apply"(%344, %69) <{map = #map3}> : (index, index) -> index
      %348 = "affine.apply"(%347) <{map = #map15}> : (index) -> index
      %349 = "affine.apply"(%90, %348) <{map = #map3}> : (index, index) -> index
      %350 = "affine.max"(%349) <{map = #map4}> : (index) -> index
      %351 = "affine.min"(%350) <{map = #map6}> : (index) -> index
      %352 = "affine.apply"(%347) <{map = #map16}> : (index) -> index
      %353 = "affine.apply"(%352) <{map = #map11}> : (index) -> index
      %354 = "affine.max"(%353) <{map = #map4}> : (index) -> index
      %355 = "affine.min"(%354) <{map = #map12}> : (index) -> index
      %356 = "affine.min"(%254, %351) <{map = #map2}> : (index, index) -> index
      %357 = "affine.apply"(%351, %356) <{map = #map3}> : (index, index) -> index
      %358 = "affine.min"(%259, %355) <{map = #map2}> : (index, index) -> index
      %359 = "affine.apply"(%355, %358) <{map = #map3}> : (index, index) -> index
      %360 = "arith.cmpi"(%357, %13) <{predicate = 2 : i64}> : (index, index) -> i1
      %361 = "arith.cmpi"(%359, %14) <{predicate = 2 : i64}> : (index, index) -> i1
      %362 = "arith.ori"(%360, %361) : (i1, i1) -> i1
      %363 = "memref.subview"(%345, %357, %359) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x32xbf16, strided<[64, 1], offset: ?>>, index, index) -> memref<?x?xbf16, strided<[64, 1], offset: ?>>
      %364 = "memref.subview"(%346, %356, %358, %357, %359) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x32xbf16>, index, index, index, index) -> memref<?x?xbf16, strided<[32, 1], offset: ?>>
      %365 = "arith.remui"(%358, %13) : (index, index) -> index
      "hivm.hir.load"(%363, %364, %7, %365, %362) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x?xbf16, strided<[64, 1], offset: ?>>, memref<?x?xbf16, strided<[32, 1], offset: ?>>, bf16, index, i1) -> ()
      %366 = "bufferization.to_tensor"(%346) <{restrict, writable}> : (memref<16x32xbf16>) -> tensor<16x32xbf16>
      %367 = "hivm.hir.vmul"(%306, %305, %41) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
      %368 = "tensor.empty"() : () -> tensor<1x32xf32>
      %369 = "hivm.hir.vreduce"(%367, %368) <{arith = #hivm.reduce_op<sum>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 0>, tie_break_left = true, unsigned_src = false}> : (tensor<16x32xf32>, tensor<1x32xf32>) -> tensor<1x32xf32>
      %370 = "tensor.collapse_shape"(%369) <{reassociation = [[0, 1]]}> : (tensor<1x32xf32>) -> tensor<32xf32>
      %371 = "hivm.hir.vadd"(%295, %370, %39) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32xf32>, tensor<32xf32>, tensor<32xf32>) -> tensor<32xf32>
      %372 = "hivm.hir.vcast"(%366, %41) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x32xbf16>, tensor<16x32xf32>) -> tensor<16x32xf32>
      %373 = "hivm.hir.vmul"(%372, %297, %41) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
      %374 = "hivm.hir.vsub"(%373, %367, %41) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
      %375 = "tensor.expand_shape"(%371) <{reassociation = [[0, 1]], static_output_shape = array<i64: 1, 32>}> : (tensor<32xf32>) -> tensor<1x32xf32>
      %376 = "hivm.hir.vbrc"(%375, %41) <{broadcast_dims = array<i64: 0>}> : (tensor<1x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
      %377 = "hivm.hir.vmul"(%157, %376, %41) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
      %378 = "hivm.hir.vadd"(%374, %377, %41) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
      %379 = "hivm.hir.vmul"(%338, %153, %41) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
      %380 = "hivm.hir.vadd"(%378, %379, %41) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
      %381 = "hivm.hir.vmul"(%337, %292, %41) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
      %382 = "hivm.hir.vadd"(%305, %381, %41) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
      %383 = "memref.reinterpret_cast"(%arg13, %239) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 32>, static_strides = array<i64: 128, 1>}> : (memref<?xf32>, index) -> memref<16x32xf32, strided<[128, 1], offset: ?>>
      %384 = "memref.reinterpret_cast"(%arg14, %239) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 32>, static_strides = array<i64: 128, 1>}> : (memref<?xf32>, index) -> memref<16x32xf32, strided<[128, 1], offset: ?>>
      %385 = "memref.reinterpret_cast"(%arg17, %239) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 32>, static_strides = array<i64: 128, 1>}> : (memref<?xf32>, index) -> memref<16x32xf32, strided<[128, 1], offset: ?>>
      %386 = "tensor.extract_slice"(%297, %255, %260, %256, %261) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<16x32xf32>, index, index, index, index) -> tensor<?x?xf32>
      %387 = "memref.subview"(%383, %256, %261) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x32xf32, strided<[128, 1], offset: ?>>, index, index) -> memref<?x?xf32, strided<[128, 1], offset: ?>>
      "hivm.hir.store"(%386, %387) : (tensor<?x?xf32>, memref<?x?xf32, strided<[128, 1], offset: ?>>) -> ()
      %388 = "tensor.extract_slice"(%382, %255, %260, %256, %261) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<16x32xf32>, index, index, index, index) -> tensor<?x?xf32>
      %389 = "memref.subview"(%384, %256, %261) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x32xf32, strided<[128, 1], offset: ?>>, index, index) -> memref<?x?xf32, strided<[128, 1], offset: ?>>
      "hivm.hir.store"(%388, %389) : (tensor<?x?xf32>, memref<?x?xf32, strided<[128, 1], offset: ?>>) -> ()
      %390 = "tensor.extract_slice"(%380, %255, %260, %256, %261) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<16x32xf32>, index, index, index, index) -> tensor<?x?xf32>
      %391 = "memref.subview"(%385, %256, %261) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x32xf32, strided<[128, 1], offset: ?>>, index, index) -> memref<?x?xf32, strided<[128, 1], offset: ?>>
      "hivm.hir.store"(%390, %391) : (tensor<?x?xf32>, memref<?x?xf32, strided<[128, 1], offset: ?>>) -> ()
      "scf.yield"(%332, %341) : (tensor<16x16xf32>, tensor<16xf32>) -> ()
    }) {fixpipe_for_mmad_result_already_inserted = true} : (i32, i32, i32, tensor<16x16xf32>, tensor<16xf32>) -> (tensor<16x16xf32>, tensor<16xf32>)
    %159 = "tensor.empty"() : () -> tensor<16x16xi32>
    %160 = "tensor.expand_shape"(%55) <{reassociation = [[0, 1]], static_output_shape = array<i64: 16, 1>}> : (tensor<16xi32>) -> tensor<16x1xi32>
    %161 = "hivm.hir.vbrc"(%160, %159) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xi32>, tensor<16x16xi32>) -> tensor<16x16xi32>
    %162 = "tensor.expand_shape"(%55) <{reassociation = [[0, 1]], static_output_shape = array<i64: 1, 16>}> : (tensor<16xi32>) -> tensor<1x16xi32>
    %163 = "hivm.hir.vbrc"(%162, %159) <{broadcast_dims = array<i64: 0>}> : (tensor<1x16xi32>, tensor<16x16xi32>) -> tensor<16x16xi32>
    %164 = "tensor.empty"() : () -> tensor<16x16xi1>
    %165 = "hivm.hir.vmax"(%161, %163, %159) <{broadcast = array<i64>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x16xi32>, tensor<16x16xi32>, tensor<16x16xi32>) -> tensor<16x16xi32>
    %166 = "hivm.hir.vcmp"(%165, %163, %164) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x16xi32>, tensor<16x16xi32>, tensor<16x16xi1>) -> tensor<16x16xi1>
    %167 = "hivm.hir.vnot"(%166, %164) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16x16xi1>, tensor<16x16xi1>) -> tensor<16x16xi1>
    %168 = "hivm.hir.vcast"(%59, %144) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<trunc>, transpose = array<i64>}> : (tensor<16xi1>, tensor<16xf16>) -> tensor<16xf16>
    %169 = "tensor.empty"() : () -> tensor<16x16xf16>
    %170 = "tensor.expand_shape"(%168) <{reassociation = [[0, 1]], static_output_shape = array<i64: 16, 1>}> : (tensor<16xf16>) -> tensor<16x1xf16>
    %171 = "hivm.hir.vbrc"(%170, %169) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xf16>, tensor<16x16xf16>) -> tensor<16x16xf16>
    %172 = "hivm.hir.vcmp"(%171, %4, %164) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x16xf16>, f16, tensor<16x16xi1>) -> tensor<16x16xi1>
    %173 = "hivm.hir.vnot"(%172, %164) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16x16xi1>, tensor<16x16xi1>) -> tensor<16x16xi1>
    %174 = "tensor.expand_shape"(%168) <{reassociation = [[0, 1]], static_output_shape = array<i64: 1, 16>}> : (tensor<16xf16>) -> tensor<1x16xf16>
    %175 = "hivm.hir.vbrc"(%174, %169) <{broadcast_dims = array<i64: 0>}> : (tensor<1x16xf16>, tensor<16x16xf16>) -> tensor<16x16xf16>
    %176 = "hivm.hir.vcmp"(%175, %4, %164) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x16xf16>, f16, tensor<16x16xi1>) -> tensor<16x16xi1>
    %177 = "hivm.hir.vnot"(%176, %164) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16x16xi1>, tensor<16x16xi1>) -> tensor<16x16xi1>
    %178 = "hivm.hir.vand"(%173, %177, %164) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x16xi1>, tensor<16x16xi1>, tensor<16x16xi1>) -> tensor<16x16xi1>
    %179 = "hivm.hir.vand"(%167, %178, %164) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x16xi1>, tensor<16x16xi1>, tensor<16x16xi1>) -> tensor<16x16xi1>
    %180 = "hivm.hir.vbrc"(%106, %36) <{broadcast_dims = array<i64: 0>}> : (tensor<1x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %181 = "hivm.hir.vmul"(%158#0, %180, %36) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x16xf32>, tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %182 = "hivm.hir.vsel"(%179, %181, %11, %36) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<16x16xi1>, tensor<16x16xf32>, f32, tensor<16x16xf32>) -> tensor<16x16xf32>
    %183 = "hivm.hir.vcast"(%182, %122) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x16xf32>, tensor<16x16xbf16>) -> tensor<16x16xbf16>
    %184 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xbf16>
    %185 = "bufferization.to_tensor"(%184) <{restrict, writable}> : (memref<16x16xbf16>) -> tensor<16x16xbf16>
    %186 = "hivm.hir.store"(%183, %185) {"inserted-store"} : (tensor<16x16xbf16>, tensor<16x16xbf16>) -> tensor<16x16xbf16>
    %187 = "hivm.hir.load"(%186, %122) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x16xbf16>, tensor<16x16xbf16>) -> tensor<16x16xbf16>
    %188 = "hivm.hir.mmadL1"(%187, %131, %1, %13, %13, %13, %36) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_for_result_already_inserted = true} : (tensor<16x16xbf16>, tensor<16x16xbf16>, i1, index, index, index, tensor<16x16xf32>) -> tensor<16x16xf32>
    %189 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xbf16>
    %190 = "bufferization.to_tensor"(%189) <{restrict, writable}> : (memref<16x16xbf16>) -> tensor<16x16xbf16>
    %191 = "hivm.hir.fixpipe"(%188, %190) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, pre_quant = #hivm.fixpipe_pre_quant_mode<F322BF16>}> : (tensor<16x16xf32>, tensor<16x16xbf16>) -> tensor<16x16xbf16>
    %192 = "hivm.hir.load"(%191, %122) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x16xbf16>, tensor<16x16xbf16>) -> tensor<16x16xbf16>
    %193 = "hivm.hir.mmadL1"(%135, %192, %1, %13, %13, %13, %36) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_for_result_already_inserted = true} : (tensor<16x16xbf16>, tensor<16x16xbf16>, i1, index, index, index, tensor<16x16xf32>) -> tensor<16x16xf32>
    %194 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xf32>
    %195 = "bufferization.to_tensor"(%194) <{restrict, writable}> : (memref<16x16xf32>) -> tensor<16x16xf32>
    %196 = "hivm.hir.fixpipe"(%193, %195) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %197 = "hivm.hir.load"(%196, %36) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %198 = "hivm.hir.vmul"(%197, %2, %36) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x16xf32>, f32, tensor<16x16xf32>) -> tensor<16x16xf32>
    %199 = "hivm.hir.vadd"(%198, %11, %36) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x16xf32>, f32, tensor<16x16xf32>) -> tensor<16x16xf32>
    %200 = "hivm.hir.vsel"(%179, %199, %11, %36) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<16x16xi1>, tensor<16x16xf32>, f32, tensor<16x16xf32>) -> tensor<16x16xf32>
    %201 = "arith.maxsi"(%53, %22) : (i32, i32) -> i32
    %202 = "arith.index_cast"(%201) : (i32) -> index
    %203 = "affine.apply"(%202) <{map = #map5}> : (index) -> index
    %204 = "affine.apply"(%203, %77) <{map = #map}> : (index, index) -> index
    %205 = "memref.reinterpret_cast"(%arg19, %204) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 16, 1>}> : (memref<?xf32>, index) -> memref<16x16xf32, strided<[16, 1], offset: ?>>
    %206 = "affine.apply"(%202, %75) <{map = #map}> : (index, index) -> index
    %207 = "memref.reinterpret_cast"(%arg18, %206) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<16xf32, strided<[1], offset: ?>>
    %208 = "affine.apply"(%203) <{map = #map17}> : (index) -> index
    %209 = "affine.apply"(%90, %208) <{map = #map3}> : (index, index) -> index
    %210 = "affine.max"(%209) <{map = #map4}> : (index) -> index
    %211 = "affine.min"(%210) <{map = #map6}> : (index) -> index
    %212 = "affine.apply"(%203) <{map = #map18}> : (index) -> index
    %213 = "affine.apply"(%212) <{map = #map19}> : (index) -> index
    %214 = "affine.max"(%213) <{map = #map4}> : (index) -> index
    %215 = "affine.min"(%214) <{map = #map6}> : (index) -> index
    %216 = "arith.subi"(%22, %53) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %217 = "arith.maxsi"(%216, %22) : (i32, i32) -> i32
    %218 = "arith.index_cast"(%217) : (i32) -> index
    %219 = "affine.min"(%218, %211) <{map = #map2}> : (index, index) -> index
    %220 = "affine.apply"(%211, %219) <{map = #map3}> : (index, index) -> index
    %221 = "affine.min"(%215) <{map = #map4}> : (index) -> index
    %222 = "affine.apply"(%215, %221) <{map = #map3}> : (index, index) -> index
    %223 = "tensor.extract_slice"(%200, %219, %221, %220, %222) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<16x16xf32>, index, index, index, index) -> tensor<?x?xf32>
    %224 = "memref.subview"(%205, %220, %222) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x16xf32, strided<[16, 1], offset: ?>>, index, index) -> memref<?x?xf32, strided<[16, 1], offset: ?>>
    "hivm.hir.store"(%223, %224) : (tensor<?x?xf32>, memref<?x?xf32, strided<[16, 1], offset: ?>>) -> ()
    %225 = "affine.apply"(%90, %202) <{map = #map3}> : (index, index) -> index
    %226 = "affine.max"(%225) <{map = #map4}> : (index) -> index
    %227 = "affine.min"(%226) <{map = #map6}> : (index) -> index
    %228 = "affine.min"(%218, %227) <{map = #map2}> : (index, index) -> index
    %229 = "affine.apply"(%227, %228) <{map = #map3}> : (index, index) -> index
    %230 = "tensor.extract_slice"(%158#1, %228, %229) <{operandSegmentSizes = array<i32: 1, 1, 1, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<16xf32>, index, index) -> tensor<?xf32>
    %231 = "memref.subview"(%207, %229) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<16xf32, strided<[1], offset: ?>>, index) -> memref<?xf32, strided<[1], offset: ?>>
    "hivm.hir.store"(%230, %231) : (tensor<?xf32>, memref<?xf32, strided<[1], offset: ?>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false]> : vector<27xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<MIX>, mix_mode = "mix", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hacc.target = #hacc.target<"Ascend910B1">, hivm.module_core_type = #hivm.module_core_type<MIX>} : () -> ()

