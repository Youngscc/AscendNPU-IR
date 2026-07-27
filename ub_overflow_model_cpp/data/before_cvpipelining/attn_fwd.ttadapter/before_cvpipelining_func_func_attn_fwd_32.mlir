#map = affine_map<()[s0, s1] -> (s0 * s1)>
#map1 = affine_map<()[s0, s1] -> (s0 + s1)>
#map2 = affine_map<()[s0] -> (s0 + 128)>
#map3 = affine_map<()[s0, s1] -> (s0, s1)>
#map4 = affine_map<()[s0, s1] -> (s0 - s1)>
#map5 = affine_map<()[s0] -> (s0 + 32)>
"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {}, {}, {}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xf16>, memref<?xf16>, memref<?xf16>, f32, memref<?xf32>, memref<?xf16>, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32) -> (), sym_name = "_attn_fwd"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xf16>, %arg4: memref<?xf16>, %arg5: memref<?xf16>, %arg6: f32, %arg7: memref<?xf32>, %arg8: memref<?xf16>, %arg9: i32, %arg10: i32, %arg11: i32, %arg12: i32, %arg13: i32, %arg14: i32, %arg15: i32, %arg16: i32, %arg17: i32, %arg18: i32, %arg19: i32, %arg20: i32, %arg21: i32, %arg22: i32, %arg23: i32, %arg24: i32, %arg25: i32, %arg26: i32, %arg27: i32, %arg28: i32):
    %0 = "arith.constant"() <{value = true}> : () -> i1
    %1 = "arith.constant"() <{value = 2.000000e+00 : f32}> : () -> f32
    %2 = "arith.constant"() <{value = 0.693147182 : f32}> : () -> f32
    %3 = "arith.constant"() <{value = 0.000000e+00 : f16}> : () -> f16
    %4 = "arith.constant"() <{value = 128 : index}> : () -> index
    %5 = "arith.constant"() <{value = 1.000000e+00 : f32}> : () -> f32
    %6 = "arith.constant"() <{value = 32 : i32}> : () -> i32
    %7 = "arith.constant"() <{value = 0xFF800000 : f32}> : () -> f32
    %8 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %9 = "arith.constant"() <{value = 0.000000e+00 : f32}> : () -> f32
    %10 = "arith.constant"() <{value = 128 : i32}> : () -> i32
    %11 = "arith.constant"() <{value = 32 : index}> : () -> index
    %12 = "arith.constant"() <{value = 0 : index}> : () -> index
    %13 = "arith.constant"() <{value = 1.44269502 : f32}> : () -> f32
    "hivm.hir.set_mask_norm"() : () -> ()
    %14 = "arith.muli"(%arg26, %arg27) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %15 = "arith.muli"(%14, %arg28) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%15) <{effects = ["write"]}> {logical_block_num} : (i32) -> ()
    %16 = "hivm.hir.get_block_idx"() : () -> i64
    %17 = "arith.trunci"(%16) : (i64) -> i32
    %18 = "arith.divsi"(%17, %arg28) : (i32, i32) -> i32
    %19 = "arith.remsi"(%18, %arg27) : (i32, i32) -> i32
    %20 = "arith.muli"(%arg28, %arg27) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %21 = "arith.divsi"(%17, %20) : (i32, i32) -> i32
    %22 = "arith.remsi"(%21, %arg26) : (i32, i32) -> i32
    %23 = "tensor.empty"() : () -> tensor<1xf32>
    %24 = "tensor.empty"() : () -> tensor<128x128xf32>
    %25 = "hivm.hir.vbrc"(%9, %24) <{broadcast_dims = array<i64>}> : (f32, tensor<128x128xf32>) -> tensor<128x128xf32>
    %26 = "tensor.empty"() : () -> tensor<128x32xf32>
    %27 = "hivm.hir.vbrc"(%7, %26) <{broadcast_dims = array<i64>}> : (f32, tensor<128x32xf32>) -> tensor<128x32xf32>
    %28 = "tensor.empty"() : () -> tensor<128xf32>
    %29 = "hivm.hir.vbrc"(%7, %28) <{broadcast_dims = array<i64>}> : (f32, tensor<128xf32>) -> tensor<128xf32>
    %30 = "hivm.hir.vbrc"(%5, %28) <{broadcast_dims = array<i64>}> : (f32, tensor<128xf32>) -> tensor<128xf32>
    %31 = "arith.divsi"(%19, %arg22) : (i32, i32) -> i32
    %32 = "arith.remsi"(%19, %arg22) : (i32, i32) -> i32
    %33 = "arith.extsi"(%31) : (i32) -> i64
    %34 = "arith.extsi"(%arg9) : (i32) -> i64
    %35 = "arith.muli"(%33, %34) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
    %36 = "arith.extsi"(%32) : (i32) -> i64
    %37 = "arith.extsi"(%arg10) : (i32) -> i64
    %38 = "arith.muli"(%36, %37) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
    %39 = "arith.addi"(%35, %38) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
    %40 = "arith.extsi"(%arg18) : (i32) -> i64
    %41 = "arith.muli"(%33, %40) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
    %42 = "arith.extsi"(%arg19) : (i32) -> i64
    %43 = "arith.muli"(%36, %42) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
    %44 = "arith.addi"(%41, %43) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
    %45 = "arith.extsi"(%arg12) : (i32) -> i64
    %46 = "arith.muli"(%33, %45) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
    %47 = "arith.extsi"(%arg13) : (i32) -> i64
    %48 = "arith.muli"(%36, %47) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
    %49 = "arith.addi"(%46, %48) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
    %50 = "arith.muli"(%22, %10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %51 = "arith.index_cast"(%39) : (i64) -> index
    %52 = "arith.index_cast"(%50) : (i32) -> index
    %53 = "arith.index_cast"(%arg11) : (i32) -> index
    %54 = "affine.apply"(%52, %53) <{map = #map}> : (index, index) -> index
    %55 = "affine.apply"(%51, %54) <{map = #map1}> : (index, index) -> index
    %56 = "memref.reinterpret_cast"(%arg3, %55, %53) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 128, 128>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xf16>, index, index) -> memref<128x128xf16, strided<[?, 1], offset: ?>>
    %57 = "arith.index_cast"(%44) : (i64) -> index
    %58 = "arith.index_cast"(%arg20) : (i32) -> index
    %59 = "affine.apply"(%52, %58) <{map = #map}> : (index, index) -> index
    %60 = "affine.apply"(%57, %59) <{map = #map1}> : (index, index) -> index
    %61 = "memref.reinterpret_cast"(%arg8, %60, %58) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 128, 128>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xf16>, index, index) -> memref<128x128xf16, strided<[?, 1], offset: ?>>
    %62 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<128x128xf16>
    %63 = "affine.apply"(%52) <{map = #map2}> : (index) -> index
    %64 = "arith.index_cast"(%arg24) : (i32) -> index
    %65 = "affine.max"(%52, %64) <{map = #map3}> : (index, index) -> index
    %66 = "affine.min"(%63, %65) <{map = #map3}> : (index, index) -> index
    %67 = "affine.apply"(%66, %52) <{map = #map4}> : (index, index) -> index
    %68 = "arith.cmpi"(%67, %4) <{predicate = 2 : i64}> : (index, index) -> i1
    %69 = "memref.subview"(%56, %67) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 128>, static_strides = array<i64: 1, 1>}> : (memref<128x128xf16, strided<[?, 1], offset: ?>>, index) -> memref<?x128xf16, strided<[?, 1], offset: ?>>
    %70 = "memref.subview"(%62, %67) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 128>, static_strides = array<i64: 1, 1>}> : (memref<128x128xf16>, index) -> memref<?x128xf16, strided<[128, 1]>>
    "hivm.hir.load"(%69, %70, %3, %12, %68) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<?x128xf16, strided<[?, 1], offset: ?>>, memref<?x128xf16, strided<[128, 1]>>, f16, index, i1) -> ()
    %71 = "bufferization.to_tensor"(%62) <{restrict, writable}> : (memref<128x128xf16>) -> tensor<128x128xf16>
    %72 = "tensor.insert"(%arg6, %23, %12) : (f32, tensor<1xf32>, index) -> tensor<1xf32>
    %73 = "hivm.hir.vmul"(%72, %13, %23) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1xf32>, f32, tensor<1xf32>) -> tensor<1xf32>
    %74 = "tensor.extract"(%73, %12) {"DuplicateTensorExtractForCube::visitedLabel" = 1 : i32} : (tensor<1xf32>, index) -> f32
    %75 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<1xf32>
    %76 = "bufferization.to_tensor"(%75) <{restrict, writable}> : (memref<1xf32>) -> tensor<1xf32>
    %77 = "hivm.hir.store"(%73, %76) {"inserted-store"} : (tensor<1xf32>, tensor<1xf32>) -> tensor<1xf32>
    "annotation.mark"(%77) <{effects = ["write"]}> {hivm.tcore_type = #hivm.tcore_type<VECTOR>} : (tensor<1xf32>) -> ()
    %78 = "tensor.extract"(%77, %12) {"DuplicateTensorExtractForCube::newExtractLabel" = 1 : i32, "DuplicateTensorExtractForCube::visitedLabel" = 1 : i32} : (tensor<1xf32>, index) -> f32
    "annotation.mark"(%74, %78) <{effects = ["write"], keys = []}> {"DuplicateTensorExtractForCube::replacementLabel" = 1 : i32} : (f32, f32) -> ()
    %79 = "arith.muli"(%arg14, %6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %80 = "arith.muli"(%arg17, %6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %81 = "arith.index_cast"(%49) : (i64) -> index
    %82 = "arith.index_cast"(%arg14) : (i32) -> index
    %83 = "arith.index_cast"(%arg17) : (i32) -> index
    %84:5 = "scf.for"(%8, %arg25, %6, %30, %25, %29, %81, %81) ({
    ^bb0(%arg29: i32, %arg30: tensor<128xf32>, %arg31: tensor<128x128xf32>, %arg32: tensor<128xf32>, %arg33: index, %arg34: index):
      %103 = "memref.reinterpret_cast"(%arg5, %arg34, %83) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 32, 128>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xf16>, index, index) -> memref<32x128xf16, strided<[?, 1], offset: ?>>
      %104 = "memref.reinterpret_cast"(%arg4, %arg33, %82) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 128, 32>, static_strides = array<i64: 1, -9223372036854775808>}> : (memref<?xf16>, index, index) -> memref<128x32xf16, strided<[1, ?], offset: ?>>
      %105 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<128x32xf16>
      %106 = "arith.index_cast"(%arg29) : (i32) -> index
      %107 = "affine.apply"(%106) <{map = #map5}> : (index) -> index
      %108 = "arith.index_cast"(%arg25) : (i32) -> index
      %109 = "affine.max"(%106, %108) <{map = #map3}> : (index, index) -> index
      %110 = "affine.min"(%107, %109) <{map = #map3}> : (index, index) -> index
      %111 = "affine.apply"(%110, %106) <{map = #map4}> : (index, index) -> index
      %112 = "arith.cmpi"(%111, %11) <{predicate = 2 : i64}> : (index, index) -> i1
      %113 = "memref.subview"(%104, %111) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: 128, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<128x32xf16, strided<[1, ?], offset: ?>>, index) -> memref<128x?xf16, strided<[1, ?], offset: ?>>
      %114 = "memref.subview"(%105, %111) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: 128, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<128x32xf16>, index) -> memref<128x?xf16, strided<[32, 1]>>
      "hivm.hir.load"(%113, %114, %3, %12, %112) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = true, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<128x?xf16, strided<[1, ?], offset: ?>>, memref<128x?xf16, strided<[32, 1]>>, f16, index, i1) -> ()
      "annotation.mark"(%105) <{effects = ["write"]}> {MayImplicitTransposeWithLastAxis} : (memref<128x32xf16>) -> ()
      %115 = "bufferization.to_tensor"(%105) <{restrict, writable}> : (memref<128x32xf16>) -> tensor<128x32xf16>
      %116 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<128x32xf16>
      %117 = "bufferization.to_tensor"(%116) <{restrict, writable}> : (memref<128x32xf16>) -> tensor<128x32xf16>
      %118 = "hivm.hir.store"(%115, %117) {"inserted-store"} : (tensor<128x32xf16>, tensor<128x32xf16>) -> tensor<128x32xf16>
      %119 = "tensor.empty"() : () -> tensor<128x32xf16>
      %120 = "hivm.hir.load"(%118, %119) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<128x32xf16>, tensor<128x32xf16>) -> tensor<128x32xf16>
      "annotation.mark"(%115) <{effects = ["write"]}> {MayImplicitTransposeWithLastAxis} : (tensor<128x32xf16>) -> ()
      %121 = "hivm.hir.mmadL1"(%71, %120, %0, %67, %4, %11, %26) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_for_result_already_inserted = true} : (tensor<128x128xf16>, tensor<128x32xf16>, i1, index, index, index, tensor<128x32xf32>) -> tensor<128x32xf32>
      %122 = "tensor.extract_slice"(%121, %111) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: 128, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<128x32xf32>, index) -> tensor<128x?xf32>
      %123 = "memref_ext.alloc_workspace"(%arg2, %111) <{operandSegmentSizes = array<i32: 1, 1, 0>}> : (memref<?xi8>, index) -> memref<128x?xf32>
      "annotation.mark"(%123) <{effects = ["write"]}> {buffer_size_in_byte = 16384 : i64} : (memref<128x?xf32>) -> ()
      %124 = "bufferization.to_tensor"(%123) <{restrict, writable}> : (memref<128x?xf32>) -> tensor<128x?xf32>
      %125 = "hivm.hir.fixpipe"(%122, %124) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<128x?xf32>, tensor<128x?xf32>) -> tensor<128x?xf32>
      %126 = "tensor.empty"(%111) : (index) -> tensor<128x?xf32>
      %127 = "hivm.hir.load"(%125, %126) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<128x?xf32>, tensor<128x?xf32>) -> tensor<128x?xf32>
      %128 = "tensor.insert_slice"(%127, %27, %111) <{operandSegmentSizes = array<i32: 1, 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: 128, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<128x?xf32>, tensor<128x32xf32>, index) -> tensor<128x32xf32>
      %129 = "hivm.hir.vmul"(%128, %74, %26) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128x32xf32>, f32, tensor<128x32xf32>) -> tensor<128x32xf32>
      %130 = "tensor.empty"() : () -> tensor<128x1xf32>
      %131 = "hivm.hir.vreduce"(%129, %130) <{arith = #hivm.reduce_op<max>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 1>, tie_break_left = true, unsigned_src = false}> : (tensor<128x32xf32>, tensor<128x1xf32>) -> tensor<128x1xf32>
      %132 = "tensor.collapse_shape"(%131) <{reassociation = [[0, 1]]}> : (tensor<128x1xf32>) -> tensor<128xf32>
      %133 = "tensor.empty"() : () -> tensor<128xi1>
      %134 = "hivm.hir.vcmp"(%arg32, %arg32, %133) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, tensor<128xf32>, tensor<128xi1>) -> tensor<128xi1>
      %135 = "hivm.hir.vnot"(%134, %133) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<128xi1>, tensor<128xi1>) -> tensor<128xi1>
      %136 = "hivm.hir.vcmp"(%132, %132, %133) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, tensor<128xf32>, tensor<128xi1>) -> tensor<128xi1>
      %137 = "hivm.hir.vnot"(%136, %133) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<128xi1>, tensor<128xi1>) -> tensor<128xi1>
      %138 = "hivm.hir.vmax"(%arg32, %132, %28) <{broadcast = array<i64>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, tensor<128xf32>, tensor<128xf32>) -> tensor<128xf32>
      %139 = "hivm.hir.vsel"(%135, %132, %138, %28) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<128xi1>, tensor<128xf32>, tensor<128xf32>, tensor<128xf32>) -> tensor<128xf32>
      %140 = "hivm.hir.vsel"(%137, %arg32, %139, %28) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<128xi1>, tensor<128xf32>, tensor<128xf32>, tensor<128xf32>) -> tensor<128xf32>
      %141 = "tensor.expand_shape"(%140) <{reassociation = [[0, 1]], static_output_shape = array<i64: 128, 1>}> : (tensor<128xf32>) -> tensor<128x1xf32>
      %142 = "hivm.hir.vbrc"(%141, %26) <{broadcast_dims = array<i64: 1>}> : (tensor<128x1xf32>, tensor<128x32xf32>) -> tensor<128x32xf32>
      %143 = "hivm.hir.vsub"(%129, %142, %26) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128x32xf32>, tensor<128x32xf32>, tensor<128x32xf32>) -> tensor<128x32xf32>
      %144 = "hivm.hir.vmul"(%143, %2, %26) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128x32xf32>, f32, tensor<128x32xf32>) -> tensor<128x32xf32>
      %145 = "hivm.hir.vexp"(%144, %26) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<128x32xf32>, tensor<128x32xf32>) -> tensor<128x32xf32>
      %146 = "hivm.hir.vreduce"(%145, %130) <{arith = #hivm.reduce_op<sum>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 1>, tie_break_left = true, unsigned_src = false}> : (tensor<128x32xf32>, tensor<128x1xf32>) -> tensor<128x1xf32>
      %147 = "tensor.collapse_shape"(%146) <{reassociation = [[0, 1]]}> : (tensor<128x1xf32>) -> tensor<128xf32>
      %148 = "hivm.hir.vsub"(%arg32, %140, %28) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, tensor<128xf32>, tensor<128xf32>) -> tensor<128xf32>
      %149 = "hivm.hir.vmul"(%148, %2, %28) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, f32, tensor<128xf32>) -> tensor<128xf32>
      %150 = "hivm.hir.vexp"(%149, %28) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, tensor<128xf32>) -> tensor<128xf32>
      %151 = "hivm.hir.vmul"(%arg30, %150, %28) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, tensor<128xf32>, tensor<128xf32>) -> tensor<128xf32>
      %152 = "hivm.hir.vadd"(%151, %147, %28) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, tensor<128xf32>, tensor<128xf32>) -> tensor<128xf32>
      %153 = "tensor.expand_shape"(%150) <{reassociation = [[0, 1]], static_output_shape = array<i64: 128, 1>}> : (tensor<128xf32>) -> tensor<128x1xf32>
      %154 = "hivm.hir.vbrc"(%153, %24) <{broadcast_dims = array<i64: 1>}> : (tensor<128x1xf32>, tensor<128x128xf32>) -> tensor<128x128xf32>
      %155 = "hivm.hir.vmul"(%arg31, %154, %24) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128x128xf32>, tensor<128x128xf32>, tensor<128x128xf32>) -> tensor<128x128xf32>
      %156 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<32x128xf16>
      %157 = "memref.subview"(%103, %111) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 128>, static_strides = array<i64: 1, 1>}> : (memref<32x128xf16, strided<[?, 1], offset: ?>>, index) -> memref<?x128xf16, strided<[?, 1], offset: ?>>
      %158 = "memref.subview"(%156, %111) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 128>, static_strides = array<i64: 1, 1>}> : (memref<32x128xf16>, index) -> memref<?x128xf16, strided<[128, 1]>>
      "hivm.hir.load"(%157, %158, %3, %12, %112) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<?x128xf16, strided<[?, 1], offset: ?>>, memref<?x128xf16, strided<[128, 1]>>, f16, index, i1) -> ()
      %159 = "bufferization.to_tensor"(%156) <{restrict, writable}> : (memref<32x128xf16>) -> tensor<32x128xf16>
      %160 = "hivm.hir.vcast"(%145, %119) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<128x32xf32>, tensor<128x32xf16>) -> tensor<128x32xf16>
      %161 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<128x32xf16>
      %162 = "bufferization.to_tensor"(%161) <{restrict, writable}> : (memref<128x32xf16>) -> tensor<128x32xf16>
      %163 = "hivm.hir.store"(%160, %162) {"inserted-store"} : (tensor<128x32xf16>, tensor<128x32xf16>) -> tensor<128x32xf16>
      %164 = "hivm.hir.load"(%163, %119) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<128x32xf16>, tensor<128x32xf16>) -> tensor<128x32xf16>
      %165 = "hivm.hir.mmadL1"(%164, %159, %0, %4, %11, %4, %24) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_for_result_already_inserted = true} : (tensor<128x32xf16>, tensor<32x128xf16>, i1, index, index, index, tensor<128x128xf32>) -> tensor<128x128xf32>
      %166 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<128x128xf32>
      %167 = "bufferization.to_tensor"(%166) <{restrict, writable}> : (memref<128x128xf32>) -> tensor<128x128xf32>
      %168 = "hivm.hir.fixpipe"(%165, %167) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<128x128xf32>, tensor<128x128xf32>) -> tensor<128x128xf32>
      %169 = "hivm.hir.load"(%168, %24) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<128x128xf32>, tensor<128x128xf32>) -> tensor<128x128xf32>
      %170 = "hivm.hir.vadd"(%169, %155, %24) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128x128xf32>, tensor<128x128xf32>, tensor<128x128xf32>) -> tensor<128x128xf32>
      %171 = "arith.index_cast"(%79) : (i32) -> index
      %172 = "affine.apply"(%arg33, %171) <{map = #map1}> : (index, index) -> index
      %173 = "arith.index_cast"(%80) : (i32) -> index
      %174 = "affine.apply"(%arg34, %173) <{map = #map1}> : (index, index) -> index
      "scf.yield"(%152, %170, %140, %172, %174) : (tensor<128xf32>, tensor<128x128xf32>, tensor<128xf32>, index, index) -> ()
    }) {fixpipe_for_mmad_result_already_inserted = true} : (i32, i32, i32, tensor<128xf32>, tensor<128x128xf32>, tensor<128xf32>, index, index) -> (tensor<128xf32>, tensor<128x128xf32>, tensor<128xf32>, index, index)
    %85 = "hivm.hir.vln"(%84#0, %28) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, tensor<128xf32>) -> tensor<128xf32>
    %86 = "hivm.hir.vbrc"(%1, %28) <{broadcast_dims = array<i64>}> : (f32, tensor<128xf32>) -> tensor<128xf32>
    %87 = "hivm.hir.vln"(%86, %28) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, tensor<128xf32>) -> tensor<128xf32>
    %88 = "hivm.hir.vdiv"(%85, %87, %28) <{broadcast = array<i64>, isHP = false, isSigned = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, tensor<128xf32>, tensor<128xf32>) -> tensor<128xf32>
    %89 = "hivm.hir.vadd"(%84#2, %88, %28) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, tensor<128xf32>, tensor<128xf32>) -> tensor<128xf32>
    %90 = "tensor.expand_shape"(%84#0) <{reassociation = [[0, 1]], static_output_shape = array<i64: 128, 1>}> : (tensor<128xf32>) -> tensor<128x1xf32>
    %91 = "hivm.hir.vbrc"(%90, %24) <{broadcast_dims = array<i64: 1>}> : (tensor<128x1xf32>, tensor<128x128xf32>) -> tensor<128x128xf32>
    %92 = "hivm.hir.vdiv"(%84#1, %91, %24) <{broadcast = array<i64>, isHP = false, isSigned = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128x128xf32>, tensor<128x128xf32>, tensor<128x128xf32>) -> tensor<128x128xf32>
    %93 = "arith.muli"(%19, %arg24) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %94 = "arith.index_cast"(%93) : (i32) -> index
    %95 = "affine.apply"(%94, %52) <{map = #map1}> : (index, index) -> index
    %96 = "memref.reinterpret_cast"(%arg7, %95) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 128>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<128xf32, strided<[1], offset: ?>>
    %97 = "tensor.extract_slice"(%89, %67) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<128xf32>, index) -> tensor<?xf32>
    %98 = "memref.subview"(%96, %67) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<128xf32, strided<[1], offset: ?>>, index) -> memref<?xf32, strided<[1], offset: ?>>
    "hivm.hir.store"(%97, %98) : (tensor<?xf32>, memref<?xf32, strided<[1], offset: ?>>) -> ()
    %99 = "tensor.empty"() : () -> tensor<128x128xf16>
    %100 = "hivm.hir.vcast"(%92, %99) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<128x128xf32>, tensor<128x128xf16>) -> tensor<128x128xf16>
    %101 = "tensor.extract_slice"(%100, %67) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 128>, static_strides = array<i64: 1, 1>}> : (tensor<128x128xf16>, index) -> tensor<?x128xf16>
    %102 = "memref.subview"(%61, %67) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 128>, static_strides = array<i64: 1, 1>}> : (memref<128x128xf16, strided<[?, 1], offset: ?>>, index) -> memref<?x128xf16, strided<[?, 1], offset: ?>>
    "hivm.hir.store"(%101, %102) : (tensor<?x128xf16>, memref<?x128xf16, strided<[?, 1], offset: ?>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, false, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false]> : vector<29xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<MIX>, mix_mode = "mix", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hacc.target = #hacc.target<"Ascend910B1">, hivm.module_core_type = #hivm.module_core_type<MIX>} : () -> ()

