#map = affine_map<()[s0] -> (s0 * 64)>
#map1 = affine_map<()[s0] -> (s0 floordiv 64)>
#map2 = affine_map<()[s0, s1] -> (s0 - s1)>
#map3 = affine_map<()[s0] -> (s0, 0)>
#map4 = affine_map<()[s0] -> (s0, 64)>
#map5 = affine_map<()[s0] -> (s0 mod 64)>
#map6 = affine_map<()[s0] -> (-s0 + 64)>
#map7 = affine_map<()[s0, s1] -> (s0, s1)>
#map8 = affine_map<()[s0] -> (s0, 32)>
#map9 = affine_map<()[s0, s1] -> (s0 + s1)>
"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, f32, memref<?xf32>, i32, memref<?xf16>, memref<?xf16>, memref<?xf16>, memref<?xf16>, i32, i32, i32, i32) -> (), sym_name = "_attn_fwd"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: f32, %arg4: memref<?xf32>, %arg5: i32, %arg6: memref<?xf16>, %arg7: memref<?xf16>, %arg8: memref<?xf16>, %arg9: memref<?xf16>, %arg10: i32, %arg11: i32, %arg12: i32, %arg13: i32):
    %0 = "arith.constant"() <{value = 1 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = true}> : () -> i1
    %2 = "arith.constant"() <{value = 2.000000e+00 : f32}> : () -> f32
    %3 = "arith.constant"() <{value = 0.693147182 : f32}> : () -> f32
    %4 = "arith.constant"() <{value = 1 : index}> : () -> index
    %5 = "arith.constant"() <{value = 32 : index}> : () -> index
    %6 = "arith.constant"() <{value = 0.000000e+00 : f16}> : () -> f16
    %7 = "arith.constant"() <{value = 64 : index}> : () -> index
    %8 = "arith.constant"() <{value = 1.000000e+00 : f32}> : () -> f32
    %9 = "arith.constant"() <{value = 0xFF800000 : f32}> : () -> f32
    %10 = "arith.constant"() <{value = -1.000000e+06 : f32}> : () -> f32
    %11 = "arith.constant"() <{value = 32 : i32}> : () -> i32
    %12 = "arith.constant"() <{value = 0.000000e+00 : f32}> : () -> f32
    %13 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %14 = "arith.constant"() <{value = 64 : i32}> : () -> i32
    %15 = "arith.constant"() <{value = 0 : index}> : () -> index
    %16 = "arith.constant"() <{value = 1.44269502 : f32}> : () -> f32
    %17 = "arith.constant"() <{value = 16 : index}> : () -> index
    "hivm.hir.set_mask_norm"() : () -> ()
    %18 = "arith.muli"(%arg11, %arg12) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %19 = "arith.muli"(%18, %arg13) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%19) <{effects = ["write"]}> {logical_block_num} : (i32) -> ()
    %20 = "hivm.hir.get_block_idx"() : () -> i64
    %21 = "arith.trunci"(%20) : (i64) -> i32
    %22 = "arith.divsi"(%21, %arg13) : (i32, i32) -> i32
    %23 = "arith.remsi"(%22, %arg12) : (i32, i32) -> i32
    %24 = "arith.muli"(%arg13, %arg12) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %25 = "arith.divsi"(%21, %24) : (i32, i32) -> i32
    %26 = "arith.remsi"(%25, %arg11) : (i32, i32) -> i32
    %27 = "tensor.empty"() : () -> tensor<1xf32>
    %28 = "tensor.empty"() : () -> tensor<64x64xf32>
    %29 = "hivm.hir.vbrc"(%12, %28) <{broadcast_dims = array<i64>}> : (f32, tensor<64x64xf32>) -> tensor<64x64xf32>
    %30 = "tensor.empty"() : () -> tensor<64x32xf32>
    %31 = "tensor.empty"() : () -> tensor<64xf32>
    %32 = "hivm.hir.vbrc"(%9, %31) <{broadcast_dims = array<i64>}> : (f32, tensor<64xf32>) -> tensor<64xf32>
    %33 = "hivm.hir.vbrc"(%8, %31) <{broadcast_dims = array<i64>}> : (f32, tensor<64xf32>) -> tensor<64xf32>
    %34 = "arith.divsi"(%23, %arg5) : (i32, i32) -> i32
    %35 = "arith.remsi"(%23, %arg5) : (i32, i32) -> i32
    %36 = "arith.muli"(%arg5, %arg10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %37 = "arith.muli"(%34, %36) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %38 = "arith.muli"(%35, %arg10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %39 = "arith.addi"(%37, %38) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %40 = "arith.muli"(%26, %14) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %41 = "arith.addi"(%39, %40) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %42 = "tensor.empty"() : () -> tensor<64xi32>
    %43 = "hivm.hir.varange"(%42, %15, %4) <{operandSegmentSizes = array<i32: 1, 1, 1>}> : (tensor<64xi32>, index, index) -> tensor<64xi32>
    %44 = "hivm.hir.vadd"(%43, %40, %42) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xi32>, i32, tensor<64xi32>) -> tensor<64xi32>
    %45 = "tensor.empty"() : () -> tensor<32xi32>
    %46 = "hivm.hir.varange"(%45, %15, %4) <{operandSegmentSizes = array<i32: 1, 1, 1>}> : (tensor<32xi32>, index, index) -> tensor<32xi32>
    %47 = "tensor.insert"(%arg3, %27, %15) : (f32, tensor<1xf32>, index) -> tensor<1xf32>
    %48 = "hivm.hir.vmul"(%47, %16, %27) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1xf32>, f32, tensor<1xf32>) -> tensor<1xf32>
    %49 = "tensor.extract"(%48, %15) {"DuplicateTensorExtractForCube::visitedLabel" = 1 : i32} : (tensor<1xf32>, index) -> f32
    %50 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<1xf32>
    %51 = "bufferization.to_tensor"(%50) <{restrict, writable}> : (memref<1xf32>) -> tensor<1xf32>
    %52 = "hivm.hir.store"(%48, %51) {"inserted-store"} : (tensor<1xf32>, tensor<1xf32>) -> tensor<1xf32>
    "annotation.mark"(%52) <{effects = ["write"]}> {hivm.tcore_type = #hivm.tcore_type<VECTOR>} : (tensor<1xf32>) -> ()
    %53 = "tensor.extract"(%52, %15) {"DuplicateTensorExtractForCube::newExtractLabel" = 1 : i32, "DuplicateTensorExtractForCube::visitedLabel" = 1 : i32} : (tensor<1xf32>, index) -> f32
    "annotation.mark"(%49, %53) <{effects = ["write"], keys = []}> {"DuplicateTensorExtractForCube::replacementLabel" = 1 : i32} : (f32, f32) -> ()
    %54 = "arith.maxsi"(%41, %13) : (i32, i32) -> i32
    %55 = "arith.index_cast"(%54) : (i32) -> index
    %56 = "affine.apply"(%55) <{map = #map}> : (index) -> index
    %57 = "arith.index_cast"(%36) : (i32) -> index
    %58 = "memref.reinterpret_cast"(%arg6, %56) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64, 64>, static_strides = array<i64: 64, 1>}> : (memref<?xf16>, index) -> memref<64x64xf16, strided<[64, 1], offset: ?>>
    %59 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<64x64xf16>
    %60 = "affine.apply"(%56) <{map = #map1}> : (index) -> index
    %61 = "affine.apply"(%57, %60) <{map = #map2}> : (index, index) -> index
    %62 = "affine.max"(%61) <{map = #map3}> : (index) -> index
    %63 = "affine.min"(%62) <{map = #map4}> : (index) -> index
    %64 = "affine.apply"(%56) <{map = #map5}> : (index) -> index
    %65 = "affine.apply"(%64) <{map = #map6}> : (index) -> index
    %66 = "affine.max"(%65) <{map = #map3}> : (index) -> index
    %67 = "affine.min"(%66) <{map = #map4}> : (index) -> index
    %68 = "arith.subi"(%13, %41) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %69 = "arith.maxsi"(%68, %13) : (i32, i32) -> i32
    %70 = "arith.index_cast"(%69) : (i32) -> index
    %71 = "affine.min"(%70, %63) <{map = #map7}> : (index, index) -> index
    %72 = "affine.apply"(%63, %71) <{map = #map2}> : (index, index) -> index
    %73 = "affine.min"(%67) <{map = #map3}> : (index) -> index
    %74 = "affine.apply"(%67, %73) <{map = #map2}> : (index, index) -> index
    %75 = "arith.cmpi"(%72, %7) <{predicate = 2 : i64}> : (index, index) -> i1
    %76 = "arith.cmpi"(%74, %7) <{predicate = 2 : i64}> : (index, index) -> i1
    %77 = "arith.ori"(%75, %76) : (i1, i1) -> i1
    %78 = "memref.subview"(%58, %72, %74) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<64x64xf16, strided<[64, 1], offset: ?>>, index, index) -> memref<?x?xf16, strided<[64, 1], offset: ?>>
    %79 = "memref.subview"(%59, %71, %73, %72, %74) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<64x64xf16>, index, index, index, index) -> memref<?x?xf16, strided<[64, 1], offset: ?>>
    %80 = "arith.remui"(%73, %17) : (index, index) -> index
    "hivm.hir.load"(%78, %79, %6, %80, %77) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<?x?xf16, strided<[64, 1], offset: ?>>, memref<?x?xf16, strided<[64, 1], offset: ?>>, f16, index, i1) -> ()
    %81 = "bufferization.to_tensor"(%59) <{restrict, writable}> : (memref<64x64xf16>) -> tensor<64x64xf16>
    %82:5 = "scf.for"(%13, %40, %11, %29, %33, %32, %39, %39) ({
    ^bb0(%arg20: i32, %arg21: tensor<64x64xf32>, %arg22: tensor<64xf32>, %arg23: tensor<64xf32>, %arg24: i32, %arg25: i32):
      %218 = "arith.maxsi"(%arg24, %13) : (i32, i32) -> i32
      %219 = "arith.index_cast"(%218) : (i32) -> index
      %220 = "affine.apply"(%219) <{map = #map}> : (index) -> index
      %221 = "memref.reinterpret_cast"(%arg7, %220) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 32, 64>, static_strides = array<i64: 64, 1>}> : (memref<?xf16>, index) -> memref<32x64xf16, strided<[64, 1], offset: ?>>
      %222 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<32x64xf16>
      %223 = "affine.apply"(%220) <{map = #map1}> : (index) -> index
      %224 = "affine.apply"(%57, %223) <{map = #map2}> : (index, index) -> index
      %225 = "affine.max"(%224) <{map = #map3}> : (index) -> index
      %226 = "affine.min"(%225) <{map = #map8}> : (index) -> index
      %227 = "affine.apply"(%220) <{map = #map5}> : (index) -> index
      %228 = "affine.apply"(%227) <{map = #map6}> : (index) -> index
      %229 = "affine.max"(%228) <{map = #map3}> : (index) -> index
      %230 = "affine.min"(%229) <{map = #map4}> : (index) -> index
      %231 = "arith.subi"(%13, %arg24) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %232 = "arith.maxsi"(%231, %13) : (i32, i32) -> i32
      %233 = "arith.index_cast"(%232) : (i32) -> index
      %234 = "affine.min"(%233, %226) <{map = #map7}> : (index, index) -> index
      %235 = "affine.apply"(%226, %234) <{map = #map2}> : (index, index) -> index
      %236 = "affine.min"(%230) <{map = #map3}> : (index) -> index
      %237 = "affine.apply"(%230, %236) <{map = #map2}> : (index, index) -> index
      %238 = "arith.cmpi"(%235, %5) <{predicate = 2 : i64}> : (index, index) -> i1
      %239 = "arith.cmpi"(%237, %7) <{predicate = 2 : i64}> : (index, index) -> i1
      %240 = "arith.ori"(%238, %239) : (i1, i1) -> i1
      %241 = "memref.subview"(%221, %235, %237) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<32x64xf16, strided<[64, 1], offset: ?>>, index, index) -> memref<?x?xf16, strided<[64, 1], offset: ?>>
      %242 = "memref.subview"(%222, %234, %236, %235, %237) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<32x64xf16>, index, index, index, index) -> memref<?x?xf16, strided<[64, 1], offset: ?>>
      %243 = "arith.remui"(%236, %17) : (index, index) -> index
      "hivm.hir.load"(%241, %242, %6, %243, %240) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<?x?xf16, strided<[64, 1], offset: ?>>, memref<?x?xf16, strided<[64, 1], offset: ?>>, f16, index, i1) -> ()
      %244 = "bufferization.to_tensor"(%222) <{restrict, writable}> : (memref<32x64xf16>) -> tensor<32x64xf16>
      %245 = "tensor.empty"() : () -> tensor<64x32xf16>
      %246 = "hivm.hir.mmadL1"(%81, %244, %1, %72, %74, %235, %30) <{b_transpose, operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_for_result_already_inserted = true} : (tensor<64x64xf16>, tensor<32x64xf16>, i1, index, index, index, tensor<64x32xf32>) -> tensor<64x32xf32>
      %247 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<64x32xf32>
      %248 = "bufferization.to_tensor"(%247) <{restrict, writable}> : (memref<64x32xf32>) -> tensor<64x32xf32>
      %249 = "hivm.hir.fixpipe"(%246, %248) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<64x32xf32>, tensor<64x32xf32>) -> tensor<64x32xf32>
      %250 = "hivm.hir.load"(%249, %30) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<64x32xf32>, tensor<64x32xf32>) -> tensor<64x32xf32>
      %251 = "tensor.empty"() : () -> tensor<64x1xf32>
      %252 = "hivm.hir.vreduce"(%250, %251) <{arith = #hivm.reduce_op<max>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 1>, tie_break_left = true, unsigned_src = false}> : (tensor<64x32xf32>, tensor<64x1xf32>) -> tensor<64x1xf32>
      %253 = "tensor.collapse_shape"(%252) <{reassociation = [[0, 1]]}> : (tensor<64x1xf32>) -> tensor<64xf32>
      %254 = "hivm.hir.vmul"(%253, %49, %31) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, f32, tensor<64xf32>) -> tensor<64xf32>
      %255 = "tensor.empty"() : () -> tensor<64xi1>
      %256 = "hivm.hir.vcmp"(%arg23, %arg23, %255) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>, tensor<64xi1>) -> tensor<64xi1>
      %257 = "hivm.hir.vnot"(%256, %255) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<64xi1>, tensor<64xi1>) -> tensor<64xi1>
      %258 = "hivm.hir.vcmp"(%254, %254, %255) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>, tensor<64xi1>) -> tensor<64xi1>
      %259 = "hivm.hir.vnot"(%258, %255) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<64xi1>, tensor<64xi1>) -> tensor<64xi1>
      %260 = "hivm.hir.vmax"(%arg23, %254, %31) <{broadcast = array<i64>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
      %261 = "hivm.hir.vsel"(%257, %254, %260, %31) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<64xi1>, tensor<64xf32>, tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
      %262 = "hivm.hir.vsel"(%259, %arg23, %261, %31) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<64xi1>, tensor<64xf32>, tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
      %263 = "hivm.hir.vmul"(%250, %49, %30) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x32xf32>, f32, tensor<64x32xf32>) -> tensor<64x32xf32>
      %264 = "tensor.expand_shape"(%262) <{reassociation = [[0, 1]], static_output_shape = array<i64: 64, 1>}> : (tensor<64xf32>) -> tensor<64x1xf32>
      %265 = "hivm.hir.vbrc"(%264, %30) <{broadcast_dims = array<i64: 1>}> : (tensor<64x1xf32>, tensor<64x32xf32>) -> tensor<64x32xf32>
      %266 = "hivm.hir.vsub"(%263, %265, %30) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x32xf32>, tensor<64x32xf32>, tensor<64x32xf32>) -> tensor<64x32xf32>
      %267 = "hivm.hir.vmul"(%266, %3, %30) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x32xf32>, f32, tensor<64x32xf32>) -> tensor<64x32xf32>
      %268 = "hivm.hir.vexp"(%267, %30) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<64x32xf32>, tensor<64x32xf32>) -> tensor<64x32xf32>
      %269 = "hivm.hir.vsub"(%arg23, %262, %31) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
      %270 = "hivm.hir.vmul"(%269, %3, %31) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, f32, tensor<64xf32>) -> tensor<64xf32>
      %271 = "hivm.hir.vexp"(%270, %31) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
      %272 = "hivm.hir.vreduce"(%268, %251) <{arith = #hivm.reduce_op<sum>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 1>, tie_break_left = true, unsigned_src = false}> : (tensor<64x32xf32>, tensor<64x1xf32>) -> tensor<64x1xf32>
      %273 = "tensor.collapse_shape"(%272) <{reassociation = [[0, 1]]}> : (tensor<64x1xf32>) -> tensor<64xf32>
      %274 = "tensor.expand_shape"(%271) <{reassociation = [[0, 1]], static_output_shape = array<i64: 64, 1>}> : (tensor<64xf32>) -> tensor<64x1xf32>
      %275 = "hivm.hir.vbrc"(%274, %28) <{broadcast_dims = array<i64: 1>}> : (tensor<64x1xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
      %276 = "hivm.hir.vmul"(%arg21, %275, %28) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x64xf32>, tensor<64x64xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
      %277 = "arith.maxsi"(%arg25, %13) : (i32, i32) -> i32
      %278 = "arith.index_cast"(%277) : (i32) -> index
      %279 = "affine.apply"(%278) <{map = #map}> : (index) -> index
      %280 = "memref.reinterpret_cast"(%arg8, %279) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 32, 64>, static_strides = array<i64: 64, 1>}> : (memref<?xf16>, index) -> memref<32x64xf16, strided<[64, 1], offset: ?>>
      %281 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<32x64xf16>
      %282 = "affine.apply"(%279) <{map = #map1}> : (index) -> index
      %283 = "affine.apply"(%57, %282) <{map = #map2}> : (index, index) -> index
      %284 = "affine.max"(%283) <{map = #map3}> : (index) -> index
      %285 = "affine.min"(%284) <{map = #map8}> : (index) -> index
      %286 = "affine.apply"(%279) <{map = #map5}> : (index) -> index
      %287 = "affine.apply"(%286) <{map = #map6}> : (index) -> index
      %288 = "affine.max"(%287) <{map = #map3}> : (index) -> index
      %289 = "affine.min"(%288) <{map = #map4}> : (index) -> index
      %290 = "arith.subi"(%13, %arg25) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %291 = "arith.maxsi"(%290, %13) : (i32, i32) -> i32
      %292 = "arith.index_cast"(%291) : (i32) -> index
      %293 = "affine.min"(%292, %285) <{map = #map7}> : (index, index) -> index
      %294 = "affine.apply"(%285, %293) <{map = #map2}> : (index, index) -> index
      %295 = "affine.min"(%289) <{map = #map3}> : (index) -> index
      %296 = "affine.apply"(%289, %295) <{map = #map2}> : (index, index) -> index
      %297 = "arith.cmpi"(%294, %5) <{predicate = 2 : i64}> : (index, index) -> i1
      %298 = "arith.cmpi"(%296, %7) <{predicate = 2 : i64}> : (index, index) -> i1
      %299 = "arith.ori"(%297, %298) : (i1, i1) -> i1
      %300 = "memref.subview"(%280, %294, %296) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<32x64xf16, strided<[64, 1], offset: ?>>, index, index) -> memref<?x?xf16, strided<[64, 1], offset: ?>>
      %301 = "memref.subview"(%281, %293, %295, %294, %296) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<32x64xf16>, index, index, index, index) -> memref<?x?xf16, strided<[64, 1], offset: ?>>
      %302 = "arith.remui"(%295, %17) : (index, index) -> index
      "hivm.hir.load"(%300, %301, %6, %302, %299) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<?x?xf16, strided<[64, 1], offset: ?>>, memref<?x?xf16, strided<[64, 1], offset: ?>>, f16, index, i1) -> ()
      %303 = "bufferization.to_tensor"(%281) <{restrict, writable}> : (memref<32x64xf16>) -> tensor<32x64xf16>
      %304 = "hivm.hir.vcast"(%268, %245) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<64x32xf32>, tensor<64x32xf16>) -> tensor<64x32xf16>
      %305 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<64x32xf16>
      %306 = "bufferization.to_tensor"(%305) <{restrict, writable}> : (memref<64x32xf16>) -> tensor<64x32xf16>
      %307 = "hivm.hir.store"(%304, %306) {"inserted-store"} : (tensor<64x32xf16>, tensor<64x32xf16>) -> tensor<64x32xf16>
      %308 = "hivm.hir.load"(%307, %245) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<64x32xf16>, tensor<64x32xf16>) -> tensor<64x32xf16>
      %309 = "hivm.hir.mmadL1"(%308, %303, %1, %7, %5, %296, %28) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_for_result_already_inserted = true} : (tensor<64x32xf16>, tensor<32x64xf16>, i1, index, index, index, tensor<64x64xf32>) -> tensor<64x64xf32>
      %310 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<64x64xf32>
      %311 = "bufferization.to_tensor"(%310) <{restrict, writable}> : (memref<64x64xf32>) -> tensor<64x64xf32>
      %312 = "hivm.hir.fixpipe"(%309, %311) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<64x64xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
      %313 = "hivm.hir.load"(%312, %28) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<64x64xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
      %314 = "hivm.hir.vadd"(%313, %276, %28) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x64xf32>, tensor<64x64xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
      %315 = "hivm.hir.vmul"(%arg22, %271, %31) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
      %316 = "hivm.hir.vadd"(%315, %273, %31) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
      %317 = "arith.addi"(%arg24, %11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %318 = "arith.addi"(%arg25, %11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      "scf.yield"(%314, %316, %262, %317, %318) : (tensor<64x64xf32>, tensor<64xf32>, tensor<64xf32>, i32, i32) -> ()
    }) {fixpipe_for_mmad_result_already_inserted = true} : (i32, i32, i32, tensor<64x64xf32>, tensor<64xf32>, tensor<64xf32>, i32, i32) -> (tensor<64x64xf32>, tensor<64xf32>, tensor<64xf32>, i32, i32)
    %83 = "arith.muli"(%26, %14) <{overflowFlags = #arith.overflow<none>}> {tt.divisibility = dense<64> : tensor<1xi32>} : (i32, i32) -> i32
    %84 = "arith.addi"(%26, %0) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %85 = "arith.muli"(%84, %14) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %86 = "arith.addi"(%39, %83) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %87 = "tensor.expand_shape"(%46) <{reassociation = [[0, 1]], static_output_shape = array<i64: 1, 32>}> : (tensor<32xi32>) -> tensor<1x32xi32>
    %88 = "tensor.empty"() : () -> tensor<64x32xi32>
    %89 = "tensor.expand_shape"(%44) <{reassociation = [[0, 1]], static_output_shape = array<i64: 64, 1>}> : (tensor<64xi32>) -> tensor<64x1xi32>
    %90 = "hivm.hir.vbrc"(%89, %88) <{broadcast_dims = array<i64: 1>}> : (tensor<64x1xi32>, tensor<64x32xi32>) -> tensor<64x32xi32>
    %91:5 = "scf.for"(%83, %85, %11, %82#0, %82#1, %82#2, %86, %86) ({
    ^bb0(%arg14: i32, %arg15: tensor<64x64xf32>, %arg16: tensor<64xf32>, %arg17: tensor<64xf32>, %arg18: i32, %arg19: i32):
      %110 = "arith.maxsi"(%arg18, %13) : (i32, i32) -> i32
      %111 = "arith.index_cast"(%110) : (i32) -> index
      %112 = "affine.apply"(%111) <{map = #map}> : (index) -> index
      %113 = "memref.reinterpret_cast"(%arg7, %112) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 32, 64>, static_strides = array<i64: 64, 1>}> : (memref<?xf16>, index) -> memref<32x64xf16, strided<[64, 1], offset: ?>>
      %114 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<32x64xf16>
      %115 = "affine.apply"(%112) <{map = #map1}> : (index) -> index
      %116 = "affine.apply"(%57, %115) <{map = #map2}> : (index, index) -> index
      %117 = "affine.max"(%116) <{map = #map3}> : (index) -> index
      %118 = "affine.min"(%117) <{map = #map8}> : (index) -> index
      %119 = "affine.apply"(%112) <{map = #map5}> : (index) -> index
      %120 = "affine.apply"(%119) <{map = #map6}> : (index) -> index
      %121 = "affine.max"(%120) <{map = #map3}> : (index) -> index
      %122 = "affine.min"(%121) <{map = #map4}> : (index) -> index
      %123 = "arith.subi"(%13, %arg18) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %124 = "arith.maxsi"(%123, %13) : (i32, i32) -> i32
      %125 = "arith.index_cast"(%124) : (i32) -> index
      %126 = "affine.min"(%125, %118) <{map = #map7}> : (index, index) -> index
      %127 = "affine.apply"(%118, %126) <{map = #map2}> : (index, index) -> index
      %128 = "affine.min"(%122) <{map = #map3}> : (index) -> index
      %129 = "affine.apply"(%122, %128) <{map = #map2}> : (index, index) -> index
      %130 = "arith.cmpi"(%127, %5) <{predicate = 2 : i64}> : (index, index) -> i1
      %131 = "arith.cmpi"(%129, %7) <{predicate = 2 : i64}> : (index, index) -> i1
      %132 = "arith.ori"(%130, %131) : (i1, i1) -> i1
      %133 = "memref.subview"(%113, %127, %129) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<32x64xf16, strided<[64, 1], offset: ?>>, index, index) -> memref<?x?xf16, strided<[64, 1], offset: ?>>
      %134 = "memref.subview"(%114, %126, %128, %127, %129) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<32x64xf16>, index, index, index, index) -> memref<?x?xf16, strided<[64, 1], offset: ?>>
      %135 = "arith.remui"(%128, %17) : (index, index) -> index
      "hivm.hir.load"(%133, %134, %6, %135, %132) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<?x?xf16, strided<[64, 1], offset: ?>>, memref<?x?xf16, strided<[64, 1], offset: ?>>, f16, index, i1) -> ()
      %136 = "bufferization.to_tensor"(%114) <{restrict, writable}> : (memref<32x64xf16>) -> tensor<32x64xf16>
      %137 = "tensor.empty"() : () -> tensor<64x32xf16>
      %138 = "hivm.hir.mmadL1"(%81, %136, %1, %72, %74, %127, %30) <{b_transpose, operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_for_result_already_inserted = true} : (tensor<64x64xf16>, tensor<32x64xf16>, i1, index, index, index, tensor<64x32xf32>) -> tensor<64x32xf32>
      %139 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<64x32xf32>
      %140 = "bufferization.to_tensor"(%139) <{restrict, writable}> : (memref<64x32xf32>) -> tensor<64x32xf32>
      %141 = "hivm.hir.fixpipe"(%138, %140) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<64x32xf32>, tensor<64x32xf32>) -> tensor<64x32xf32>
      %142 = "hivm.hir.load"(%141, %30) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<64x32xf32>, tensor<64x32xf32>) -> tensor<64x32xf32>
      %143 = "tensor.empty"() : () -> tensor<1x32xi32>
      %144 = "hivm.hir.vadd"(%87, %arg14, %143) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1x32xi32>, i32, tensor<1x32xi32>) -> tensor<1x32xi32>
      %145 = "hivm.hir.vbrc"(%144, %88) <{broadcast_dims = array<i64: 0>}> : (tensor<1x32xi32>, tensor<64x32xi32>) -> tensor<64x32xi32>
      %146 = "hivm.hir.vmax"(%90, %145, %88) <{broadcast = array<i64>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x32xi32>, tensor<64x32xi32>, tensor<64x32xi32>) -> tensor<64x32xi32>
      %147 = "tensor.empty"() : () -> tensor<64x32xi1>
      %148 = "hivm.hir.vcmp"(%146, %90, %147) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x32xi32>, tensor<64x32xi32>, tensor<64x32xi1>) -> tensor<64x32xi1>
      %149 = "hivm.hir.vmul"(%142, %49, %30) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x32xf32>, f32, tensor<64x32xf32>) -> tensor<64x32xf32>
      %150 = "hivm.hir.vsel"(%148, %12, %10, %30) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<64x32xi1>, f32, f32, tensor<64x32xf32>) -> tensor<64x32xf32>
      %151 = "hivm.hir.vadd"(%149, %150, %30) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x32xf32>, tensor<64x32xf32>, tensor<64x32xf32>) -> tensor<64x32xf32>
      %152 = "tensor.empty"() : () -> tensor<64x1xf32>
      %153 = "hivm.hir.vreduce"(%151, %152) <{arith = #hivm.reduce_op<max>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 1>, tie_break_left = true, unsigned_src = false}> : (tensor<64x32xf32>, tensor<64x1xf32>) -> tensor<64x1xf32>
      %154 = "tensor.collapse_shape"(%153) <{reassociation = [[0, 1]]}> : (tensor<64x1xf32>) -> tensor<64xf32>
      %155 = "tensor.empty"() : () -> tensor<64xi1>
      %156 = "hivm.hir.vcmp"(%arg17, %arg17, %155) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>, tensor<64xi1>) -> tensor<64xi1>
      %157 = "hivm.hir.vnot"(%156, %155) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<64xi1>, tensor<64xi1>) -> tensor<64xi1>
      %158 = "hivm.hir.vcmp"(%154, %154, %155) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>, tensor<64xi1>) -> tensor<64xi1>
      %159 = "hivm.hir.vnot"(%158, %155) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<64xi1>, tensor<64xi1>) -> tensor<64xi1>
      %160 = "hivm.hir.vmax"(%arg17, %154, %31) <{broadcast = array<i64>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
      %161 = "hivm.hir.vsel"(%157, %154, %160, %31) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<64xi1>, tensor<64xf32>, tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
      %162 = "hivm.hir.vsel"(%159, %arg17, %161, %31) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<64xi1>, tensor<64xf32>, tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
      %163 = "tensor.expand_shape"(%162) <{reassociation = [[0, 1]], static_output_shape = array<i64: 64, 1>}> : (tensor<64xf32>) -> tensor<64x1xf32>
      %164 = "hivm.hir.vbrc"(%163, %30) <{broadcast_dims = array<i64: 1>}> : (tensor<64x1xf32>, tensor<64x32xf32>) -> tensor<64x32xf32>
      %165 = "hivm.hir.vsub"(%151, %164, %30) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x32xf32>, tensor<64x32xf32>, tensor<64x32xf32>) -> tensor<64x32xf32>
      %166 = "hivm.hir.vmul"(%165, %3, %30) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x32xf32>, f32, tensor<64x32xf32>) -> tensor<64x32xf32>
      %167 = "hivm.hir.vexp"(%166, %30) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<64x32xf32>, tensor<64x32xf32>) -> tensor<64x32xf32>
      %168 = "hivm.hir.vsub"(%arg17, %162, %31) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
      %169 = "hivm.hir.vmul"(%168, %3, %31) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, f32, tensor<64xf32>) -> tensor<64xf32>
      %170 = "hivm.hir.vexp"(%169, %31) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
      %171 = "hivm.hir.vreduce"(%167, %152) <{arith = #hivm.reduce_op<sum>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 1>, tie_break_left = true, unsigned_src = false}> : (tensor<64x32xf32>, tensor<64x1xf32>) -> tensor<64x1xf32>
      %172 = "tensor.collapse_shape"(%171) <{reassociation = [[0, 1]]}> : (tensor<64x1xf32>) -> tensor<64xf32>
      %173 = "tensor.expand_shape"(%170) <{reassociation = [[0, 1]], static_output_shape = array<i64: 64, 1>}> : (tensor<64xf32>) -> tensor<64x1xf32>
      %174 = "hivm.hir.vbrc"(%173, %28) <{broadcast_dims = array<i64: 1>}> : (tensor<64x1xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
      %175 = "hivm.hir.vmul"(%arg15, %174, %28) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x64xf32>, tensor<64x64xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
      %176 = "arith.maxsi"(%arg19, %13) : (i32, i32) -> i32
      %177 = "arith.index_cast"(%176) : (i32) -> index
      %178 = "affine.apply"(%177) <{map = #map}> : (index) -> index
      %179 = "memref.reinterpret_cast"(%arg8, %178) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 32, 64>, static_strides = array<i64: 64, 1>}> : (memref<?xf16>, index) -> memref<32x64xf16, strided<[64, 1], offset: ?>>
      %180 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<32x64xf16>
      %181 = "affine.apply"(%178) <{map = #map1}> : (index) -> index
      %182 = "affine.apply"(%57, %181) <{map = #map2}> : (index, index) -> index
      %183 = "affine.max"(%182) <{map = #map3}> : (index) -> index
      %184 = "affine.min"(%183) <{map = #map8}> : (index) -> index
      %185 = "affine.apply"(%178) <{map = #map5}> : (index) -> index
      %186 = "affine.apply"(%185) <{map = #map6}> : (index) -> index
      %187 = "affine.max"(%186) <{map = #map3}> : (index) -> index
      %188 = "affine.min"(%187) <{map = #map4}> : (index) -> index
      %189 = "arith.subi"(%13, %arg19) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %190 = "arith.maxsi"(%189, %13) : (i32, i32) -> i32
      %191 = "arith.index_cast"(%190) : (i32) -> index
      %192 = "affine.min"(%191, %184) <{map = #map7}> : (index, index) -> index
      %193 = "affine.apply"(%184, %192) <{map = #map2}> : (index, index) -> index
      %194 = "affine.min"(%188) <{map = #map3}> : (index) -> index
      %195 = "affine.apply"(%188, %194) <{map = #map2}> : (index, index) -> index
      %196 = "arith.cmpi"(%193, %5) <{predicate = 2 : i64}> : (index, index) -> i1
      %197 = "arith.cmpi"(%195, %7) <{predicate = 2 : i64}> : (index, index) -> i1
      %198 = "arith.ori"(%196, %197) : (i1, i1) -> i1
      %199 = "memref.subview"(%179, %193, %195) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<32x64xf16, strided<[64, 1], offset: ?>>, index, index) -> memref<?x?xf16, strided<[64, 1], offset: ?>>
      %200 = "memref.subview"(%180, %192, %194, %193, %195) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<32x64xf16>, index, index, index, index) -> memref<?x?xf16, strided<[64, 1], offset: ?>>
      %201 = "arith.remui"(%194, %17) : (index, index) -> index
      "hivm.hir.load"(%199, %200, %6, %201, %198) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<?x?xf16, strided<[64, 1], offset: ?>>, memref<?x?xf16, strided<[64, 1], offset: ?>>, f16, index, i1) -> ()
      %202 = "bufferization.to_tensor"(%180) <{restrict, writable}> : (memref<32x64xf16>) -> tensor<32x64xf16>
      %203 = "hivm.hir.vcast"(%167, %137) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<64x32xf32>, tensor<64x32xf16>) -> tensor<64x32xf16>
      %204 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<64x32xf16>
      %205 = "bufferization.to_tensor"(%204) <{restrict, writable}> : (memref<64x32xf16>) -> tensor<64x32xf16>
      %206 = "hivm.hir.store"(%203, %205) {"inserted-store"} : (tensor<64x32xf16>, tensor<64x32xf16>) -> tensor<64x32xf16>
      %207 = "hivm.hir.load"(%206, %137) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<64x32xf16>, tensor<64x32xf16>) -> tensor<64x32xf16>
      %208 = "hivm.hir.mmadL1"(%207, %202, %1, %7, %5, %195, %28) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_for_result_already_inserted = true} : (tensor<64x32xf16>, tensor<32x64xf16>, i1, index, index, index, tensor<64x64xf32>) -> tensor<64x64xf32>
      %209 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<64x64xf32>
      %210 = "bufferization.to_tensor"(%209) <{restrict, writable}> : (memref<64x64xf32>) -> tensor<64x64xf32>
      %211 = "hivm.hir.fixpipe"(%208, %210) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<64x64xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
      %212 = "hivm.hir.load"(%211, %28) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<64x64xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
      %213 = "hivm.hir.vadd"(%212, %175, %28) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x64xf32>, tensor<64x64xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
      %214 = "hivm.hir.vmul"(%arg16, %170, %31) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
      %215 = "hivm.hir.vadd"(%214, %172, %31) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
      %216 = "arith.addi"(%arg18, %11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %217 = "arith.addi"(%arg19, %11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      "scf.yield"(%213, %215, %162, %216, %217) : (tensor<64x64xf32>, tensor<64xf32>, tensor<64xf32>, i32, i32) -> ()
    }) {fixpipe_for_mmad_result_already_inserted = true} : (i32, i32, i32, tensor<64x64xf32>, tensor<64xf32>, tensor<64xf32>, i32, i32) -> (tensor<64x64xf32>, tensor<64xf32>, tensor<64xf32>, i32, i32)
    %92 = "hivm.hir.vln"(%91#1, %31) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
    %93 = "hivm.hir.vbrc"(%2, %31) <{broadcast_dims = array<i64>}> : (f32, tensor<64xf32>) -> tensor<64xf32>
    %94 = "hivm.hir.vln"(%93, %31) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
    %95 = "hivm.hir.vdiv"(%92, %94, %31) <{broadcast = array<i64>, isHP = false, isSigned = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
    %96 = "hivm.hir.vadd"(%91#2, %95, %31) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
    %97 = "tensor.expand_shape"(%91#1) <{reassociation = [[0, 1]], static_output_shape = array<i64: 64, 1>}> : (tensor<64xf32>) -> tensor<64x1xf32>
    %98 = "hivm.hir.vbrc"(%97, %28) <{broadcast_dims = array<i64: 1>}> : (tensor<64x1xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
    %99 = "hivm.hir.vdiv"(%91#0, %98, %28) <{broadcast = array<i64>, isHP = false, isSigned = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x64xf32>, tensor<64x64xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
    %100 = "arith.muli"(%23, %arg10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %101 = "arith.index_cast"(%100) : (i32) -> index
    %102 = "arith.index_cast"(%40) : (i32) -> index
    %103 = "affine.apply"(%101, %102) <{map = #map9}> : (index, index) -> index
    %104 = "memref.reinterpret_cast"(%arg4, %103) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<64xf32, strided<[1], offset: ?>>
    "hivm.hir.store"(%96, %104) : (tensor<64xf32>, memref<64xf32, strided<[1], offset: ?>>) -> ()
    %105 = "tensor.empty"() : () -> tensor<64x64xf16>
    %106 = "hivm.hir.vcast"(%99, %105) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<64x64xf32>, tensor<64x64xf16>) -> tensor<64x64xf16>
    %107 = "memref.reinterpret_cast"(%arg9, %56) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64, 64>, static_strides = array<i64: 64, 1>}> : (memref<?xf16>, index) -> memref<64x64xf16, strided<[64, 1], offset: ?>>
    %108 = "tensor.extract_slice"(%106, %71, %73, %72, %74) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<64x64xf16>, index, index, index, index) -> tensor<?x?xf16>
    %109 = "memref.subview"(%107, %72, %74) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<64x64xf16, strided<[64, 1], offset: ?>>, index, index) -> memref<?x?xf16, strided<[64, 1], offset: ?>>
    "hivm.hir.store"(%108, %109) : (tensor<?x?xf16>, memref<?x?xf16, strided<[64, 1], offset: ?>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, false, true, false, true, true, true, true, false, false, false, false]> : vector<14xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<MIX>, mix_mode = "mix", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hacc.target = #hacc.target<"Ascend910B1">, hivm.module_core_type = #hivm.module_core_type<MIX>} : () -> ()

