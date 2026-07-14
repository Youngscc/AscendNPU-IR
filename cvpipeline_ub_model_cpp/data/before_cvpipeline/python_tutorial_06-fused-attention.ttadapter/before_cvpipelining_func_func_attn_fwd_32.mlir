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
    "hivm.hir.set_mask_norm"() : () -> ()
    %17 = "arith.muli"(%arg11, %arg12) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %18 = "arith.muli"(%17, %arg13) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%18) {logical_block_num} : (i32) -> ()
    %19 = "hivm.hir.get_block_idx"() : () -> i64
    %20 = "arith.trunci"(%19) : (i64) -> i32
    %21 = "arith.divsi"(%20, %arg13) : (i32, i32) -> i32
    %22 = "arith.remsi"(%21, %arg12) : (i32, i32) -> i32
    %23 = "arith.muli"(%arg13, %arg12) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %24 = "arith.divsi"(%20, %23) : (i32, i32) -> i32
    %25 = "arith.remsi"(%24, %arg11) : (i32, i32) -> i32
    %26 = "tensor.empty"() : () -> tensor<1xf32>
    %27 = "tensor.empty"() : () -> tensor<1xf32>
    %28 = "tensor.empty"() : () -> tensor<64x64xf32>
    %29 = "tensor.empty"() : () -> tensor<64x64xf32>
    %30 = "tensor.empty"() : () -> tensor<64x64xf32>
    %31 = "tensor.empty"() : () -> tensor<64x64xf32>
    %32 = "tensor.empty"() : () -> tensor<64x64xf32>
    %33 = "tensor.empty"() : () -> tensor<64x64xf32>
    %34 = "tensor.empty"() : () -> tensor<64x64xf32>
    %35 = "hivm.hir.vbrc"(%12, %34) <{broadcast_dims = array<i64>}> : (f32, tensor<64x64xf32>) -> tensor<64x64xf32>
    %36 = "tensor.empty"() : () -> tensor<64x32xf32>
    %37 = "tensor.empty"() : () -> tensor<64x32xf32>
    %38 = "tensor.empty"() : () -> tensor<64x32xf32>
    %39 = "tensor.empty"() : () -> tensor<64x32xf32>
    %40 = "tensor.empty"() : () -> tensor<64x32xf32>
    %41 = "tensor.empty"() : () -> tensor<64x32xf32>
    %42 = "tensor.empty"() : () -> tensor<64x32xf32>
    %43 = "tensor.empty"() : () -> tensor<64x32xf32>
    %44 = "tensor.empty"() : () -> tensor<64x32xf32>
    %45 = "tensor.empty"() : () -> tensor<64x32xf32>
    %46 = "tensor.empty"() : () -> tensor<64x32xf32>
    %47 = "tensor.empty"() : () -> tensor<64x32xf32>
    %48 = "tensor.empty"() : () -> tensor<64xf32>
    %49 = "tensor.empty"() : () -> tensor<64xf32>
    %50 = "tensor.empty"() : () -> tensor<64xf32>
    %51 = "tensor.empty"() : () -> tensor<64xf32>
    %52 = "tensor.empty"() : () -> tensor<64xf32>
    %53 = "tensor.empty"() : () -> tensor<64xf32>
    %54 = "tensor.empty"() : () -> tensor<64xf32>
    %55 = "tensor.empty"() : () -> tensor<64xf32>
    %56 = "tensor.empty"() : () -> tensor<64xf32>
    %57 = "tensor.empty"() : () -> tensor<64xf32>
    %58 = "tensor.empty"() : () -> tensor<64xf32>
    %59 = "tensor.empty"() : () -> tensor<64xf32>
    %60 = "tensor.empty"() : () -> tensor<64xf32>
    %61 = "tensor.empty"() : () -> tensor<64xf32>
    %62 = "tensor.empty"() : () -> tensor<64xf32>
    %63 = "tensor.empty"() : () -> tensor<64xf32>
    %64 = "tensor.empty"() : () -> tensor<64xf32>
    %65 = "tensor.empty"() : () -> tensor<64xf32>
    %66 = "tensor.empty"() : () -> tensor<64xf32>
    %67 = "tensor.empty"() : () -> tensor<64xf32>
    %68 = "tensor.empty"() : () -> tensor<64xf32>
    %69 = "tensor.empty"() : () -> tensor<64xf32>
    %70 = "tensor.empty"() : () -> tensor<64xf32>
    %71 = "hivm.hir.vbrc"(%9, %70) <{broadcast_dims = array<i64>}> : (f32, tensor<64xf32>) -> tensor<64xf32>
    %72 = "hivm.hir.vbrc"(%8, %69) <{broadcast_dims = array<i64>}> : (f32, tensor<64xf32>) -> tensor<64xf32>
    %73 = "arith.divsi"(%22, %arg5) : (i32, i32) -> i32
    %74 = "arith.remsi"(%22, %arg5) : (i32, i32) -> i32
    %75 = "arith.muli"(%arg5, %arg10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %76 = "arith.muli"(%73, %75) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %77 = "arith.muli"(%74, %arg10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %78 = "arith.addi"(%76, %77) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %79 = "arith.muli"(%25, %14) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %80 = "arith.addi"(%78, %79) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %81 = "tensor.empty"() : () -> tensor<64xi32>
    %82 = "tensor.empty"() : () -> tensor<64xi32>
    %83 = "hivm.hir.varange"(%82, %15, %4) <{operandSegmentSizes = array<i32: 1, 1, 1>}> : (tensor<64xi32>, index, index) -> tensor<64xi32>
    %84 = "hivm.hir.vadd"(%83, %79, %81) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xi32>, i32, tensor<64xi32>) -> tensor<64xi32>
    %85 = "tensor.empty"() : () -> tensor<32xi32>
    %86 = "hivm.hir.varange"(%85, %15, %4) <{operandSegmentSizes = array<i32: 1, 1, 1>}> : (tensor<32xi32>, index, index) -> tensor<32xi32>
    %87 = "tensor.insert"(%arg3, %27, %15) : (f32, tensor<1xf32>, index) -> tensor<1xf32>
    %88 = "hivm.hir.vmul"(%87, %16, %26) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1xf32>, f32, tensor<1xf32>) -> tensor<1xf32>
    %89 = "tensor.extract"(%88, %15) {"DuplicateTensorExtractForCube::visitedLabel" = 1 : i32} : (tensor<1xf32>, index) -> f32
    %90 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<1xf32>
    %91 = "bufferization.to_tensor"(%90) <{restrict, writable}> : (memref<1xf32>) -> tensor<1xf32>
    %92 = "hivm.hir.store"(%88, %91) {"inserted-store"} : (tensor<1xf32>, tensor<1xf32>) -> tensor<1xf32>
    "annotation.mark"(%92) {hivm.tcore_type = #hivm.tcore_type<VECTOR>} : (tensor<1xf32>) -> ()
    %93 = "tensor.extract"(%92, %15) {"DuplicateTensorExtractForCube::newExtractLabel" = 1 : i32, "DuplicateTensorExtractForCube::visitedLabel" = 1 : i32} : (tensor<1xf32>, index) -> f32
    "annotation.mark"(%89, %93) <{keys = []}> {"DuplicateTensorExtractForCube::replacementLabel" = 1 : i32} : (f32, f32) -> ()
    %94 = "arith.maxsi"(%80, %13) : (i32, i32) -> i32
    %95 = "arith.index_cast"(%94) : (i32) -> index
    %96 = "arith.muli"(%95, %7) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %97 = "arith.index_cast"(%75) : (i32) -> index
    %98 = "memref.reinterpret_cast"(%arg6, %96) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64, 64>, static_strides = array<i64: 64, 1>}> : (memref<?xf16>, index) -> memref<64x64xf16, strided<[64, 1], offset: ?>>
    %99 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<64x64xf16>
    %100 = "arith.divsi"(%96, %7) : (index, index) -> index
    %101 = "arith.subi"(%97, %100) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %102 = "arith.maxsi"(%101, %15) : (index, index) -> index
    %103 = "arith.minsi"(%102, %7) : (index, index) -> index
    %104 = "arith.remsi"(%96, %7) : (index, index) -> index
    %105 = "arith.subi"(%7, %104) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %106 = "arith.maxsi"(%105, %15) : (index, index) -> index
    %107 = "arith.minsi"(%106, %7) : (index, index) -> index
    %108 = "arith.subi"(%13, %80) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %109 = "arith.maxsi"(%108, %13) : (i32, i32) -> i32
    %110 = "arith.index_cast"(%109) : (i32) -> index
    %111 = "arith.minsi"(%110, %103) : (index, index) -> index
    %112 = "arith.subi"(%103, %111) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %113 = "arith.minsi"(%107, %15) : (index, index) -> index
    %114 = "arith.subi"(%107, %113) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %115 = "arith.cmpi"(%112, %7) <{predicate = 2 : i64}> : (index, index) -> i1
    %116 = "arith.cmpi"(%114, %7) <{predicate = 2 : i64}> : (index, index) -> i1
    %117 = "arith.ori"(%115, %116) : (i1, i1) -> i1
    %118 = "memref.subview"(%98, %112, %114) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<64x64xf16, strided<[64, 1], offset: ?>>, index, index) -> memref<?x?xf16, strided<[64, 1], offset: ?>>
    %119 = "memref.subview"(%99, %111, %113, %112, %114) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<64x64xf16>, index, index, index, index) -> memref<?x?xf16, strided<[64, 1], offset: ?>>
    "hivm.hir.load"(%118, %119, %6, %113, %117) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<?x?xf16, strided<[64, 1], offset: ?>>, memref<?x?xf16, strided<[64, 1], offset: ?>>, f16, index, i1) -> ()
    %120 = "bufferization.to_tensor"(%99) <{restrict, writable}> : (memref<64x64xf16>) -> tensor<64x64xf16>
    %121:5 = "scf.for"(%13, %79, %11, %35, %72, %71, %78, %78) ({
    ^bb0(%arg20: i32, %arg21: tensor<64x64xf32>, %arg22: tensor<64xf32>, %arg23: tensor<64xf32>, %arg24: i32, %arg25: i32):
      %268 = "arith.maxsi"(%arg24, %13) : (i32, i32) -> i32
      %269 = "arith.index_cast"(%268) : (i32) -> index
      %270 = "arith.muli"(%269, %7) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %271 = "memref.reinterpret_cast"(%arg7, %270) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 32, 64>, static_strides = array<i64: 64, 1>}> : (memref<?xf16>, index) -> memref<32x64xf16, strided<[64, 1], offset: ?>>
      %272 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<32x64xf16>
      %273 = "arith.divsi"(%270, %7) : (index, index) -> index
      %274 = "arith.subi"(%97, %273) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %275 = "arith.maxsi"(%274, %15) : (index, index) -> index
      %276 = "arith.minsi"(%275, %5) : (index, index) -> index
      %277 = "arith.remsi"(%270, %7) : (index, index) -> index
      %278 = "arith.subi"(%7, %277) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %279 = "arith.maxsi"(%278, %15) : (index, index) -> index
      %280 = "arith.minsi"(%279, %7) : (index, index) -> index
      %281 = "arith.subi"(%13, %arg24) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %282 = "arith.maxsi"(%281, %13) : (i32, i32) -> i32
      %283 = "arith.index_cast"(%282) : (i32) -> index
      %284 = "arith.minsi"(%283, %276) : (index, index) -> index
      %285 = "arith.subi"(%276, %284) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %286 = "arith.minsi"(%280, %15) : (index, index) -> index
      %287 = "arith.subi"(%280, %286) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %288 = "arith.cmpi"(%285, %5) <{predicate = 2 : i64}> : (index, index) -> i1
      %289 = "arith.cmpi"(%287, %7) <{predicate = 2 : i64}> : (index, index) -> i1
      %290 = "arith.ori"(%288, %289) : (i1, i1) -> i1
      %291 = "memref.subview"(%271, %285, %287) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<32x64xf16, strided<[64, 1], offset: ?>>, index, index) -> memref<?x?xf16, strided<[64, 1], offset: ?>>
      %292 = "memref.subview"(%272, %284, %286, %285, %287) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<32x64xf16>, index, index, index, index) -> memref<?x?xf16, strided<[64, 1], offset: ?>>
      "hivm.hir.load"(%291, %292, %6, %286, %290) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<?x?xf16, strided<[64, 1], offset: ?>>, memref<?x?xf16, strided<[64, 1], offset: ?>>, f16, index, i1) -> ()
      %293 = "bufferization.to_tensor"(%272) <{restrict, writable}> : (memref<32x64xf16>) -> tensor<32x64xf16>
      %294 = "tensor.empty"() : () -> tensor<64x32xf16>
      %295 = "tensor.empty"() : () -> tensor<64x32xf32>
      %296 = "hivm.hir.mmadL1"(%120, %293, %1, %112, %114, %285, %295) <{b_transpose, operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_already_inserted = true} : (tensor<64x64xf16>, tensor<32x64xf16>, i1, index, index, index, tensor<64x32xf32>) -> tensor<64x32xf32>
      %297 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<64x32xf32>
      %298 = "bufferization.to_tensor"(%297) <{restrict, writable}> : (memref<64x32xf32>) -> tensor<64x32xf32>
      %299 = "hivm.hir.fixpipe"(%296, %298) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<64x32xf32>, tensor<64x32xf32>) -> tensor<64x32xf32>
      %300 = "tensor.empty"() : () -> tensor<64x32xf32>
      %301 = "hivm.hir.load"(%299, %300) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<64x32xf32>, tensor<64x32xf32>) -> tensor<64x32xf32>
      %302 = "tensor.empty"() : () -> tensor<64x32xf32>
      %303 = "hivm.hir.load"(%299, %302) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<64x32xf32>, tensor<64x32xf32>) -> tensor<64x32xf32>
      %304 = "tensor.empty"() : () -> tensor<64x1xf32>
      %305 = "hivm.hir.vreduce"(%301, %304) <{arith = #hivm.reduce_op<max>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 1>}> : (tensor<64x32xf32>, tensor<64x1xf32>) -> tensor<64x1xf32>
      %306 = "tensor.collapse_shape"(%305) <{reassociation = [[0, 1]]}> : (tensor<64x1xf32>) -> tensor<64xf32>
      %307 = "hivm.hir.vmul"(%306, %89, %68) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, f32, tensor<64xf32>) -> tensor<64xf32>
      %308 = "tensor.empty"() : () -> tensor<64xi1>
      %309 = "tensor.empty"() : () -> tensor<64xi1>
      %310 = "tensor.empty"() : () -> tensor<64xi1>
      %311 = "tensor.empty"() : () -> tensor<64xi1>
      %312 = "hivm.hir.vcmp"(%arg23, %arg23, %311) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>, tensor<64xi1>) -> tensor<64xi1>
      %313 = "hivm.hir.vnot"(%312, %310) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<64xi1>, tensor<64xi1>) -> tensor<64xi1>
      %314 = "hivm.hir.vcmp"(%307, %307, %309) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>, tensor<64xi1>) -> tensor<64xi1>
      %315 = "hivm.hir.vnot"(%314, %308) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<64xi1>, tensor<64xi1>) -> tensor<64xi1>
      %316 = "hivm.hir.vmax"(%arg23, %307, %67) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
      %317 = "hivm.hir.vsel"(%313, %307, %316, %66) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<64xi1>, tensor<64xf32>, tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
      %318 = "hivm.hir.vsel"(%315, %arg23, %317, %65) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<64xi1>, tensor<64xf32>, tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
      %319 = "hivm.hir.vmul"(%303, %89, %47) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x32xf32>, f32, tensor<64x32xf32>) -> tensor<64x32xf32>
      %320 = "tensor.expand_shape"(%318) <{reassociation = [[0, 1]], static_output_shape = array<i64: 64, 1>}> : (tensor<64xf32>) -> tensor<64x1xf32>
      %321 = "hivm.hir.vbrc"(%320, %46) <{broadcast_dims = array<i64: 1>}> : (tensor<64x1xf32>, tensor<64x32xf32>) -> tensor<64x32xf32>
      %322 = "hivm.hir.vsub"(%319, %321, %45) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x32xf32>, tensor<64x32xf32>, tensor<64x32xf32>) -> tensor<64x32xf32>
      %323 = "hivm.hir.vmul"(%322, %3, %44) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x32xf32>, f32, tensor<64x32xf32>) -> tensor<64x32xf32>
      %324 = "hivm.hir.vexp"(%323, %43) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<64x32xf32>, tensor<64x32xf32>) -> tensor<64x32xf32>
      %325 = "hivm.hir.vsub"(%arg23, %318, %64) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
      %326 = "hivm.hir.vmul"(%325, %3, %63) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, f32, tensor<64xf32>) -> tensor<64xf32>
      %327 = "hivm.hir.vexp"(%326, %62) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
      %328 = "tensor.empty"() : () -> tensor<64x1xf32>
      %329 = "hivm.hir.vreduce"(%324, %328) <{arith = #hivm.reduce_op<sum>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 1>}> : (tensor<64x32xf32>, tensor<64x1xf32>) -> tensor<64x1xf32>
      %330 = "tensor.collapse_shape"(%329) <{reassociation = [[0, 1]]}> : (tensor<64x1xf32>) -> tensor<64xf32>
      %331 = "tensor.expand_shape"(%327) <{reassociation = [[0, 1]], static_output_shape = array<i64: 64, 1>}> : (tensor<64xf32>) -> tensor<64x1xf32>
      %332 = "hivm.hir.vbrc"(%331, %33) <{broadcast_dims = array<i64: 1>}> : (tensor<64x1xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
      %333 = "hivm.hir.vmul"(%arg21, %332, %32) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x64xf32>, tensor<64x64xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
      %334 = "arith.maxsi"(%arg25, %13) : (i32, i32) -> i32
      %335 = "arith.index_cast"(%334) : (i32) -> index
      %336 = "arith.muli"(%335, %7) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %337 = "memref.reinterpret_cast"(%arg8, %336) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 32, 64>, static_strides = array<i64: 64, 1>}> : (memref<?xf16>, index) -> memref<32x64xf16, strided<[64, 1], offset: ?>>
      %338 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<32x64xf16>
      %339 = "arith.divsi"(%336, %7) : (index, index) -> index
      %340 = "arith.subi"(%97, %339) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %341 = "arith.maxsi"(%340, %15) : (index, index) -> index
      %342 = "arith.minsi"(%341, %5) : (index, index) -> index
      %343 = "arith.remsi"(%336, %7) : (index, index) -> index
      %344 = "arith.subi"(%7, %343) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %345 = "arith.maxsi"(%344, %15) : (index, index) -> index
      %346 = "arith.minsi"(%345, %7) : (index, index) -> index
      %347 = "arith.subi"(%13, %arg25) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %348 = "arith.maxsi"(%347, %13) : (i32, i32) -> i32
      %349 = "arith.index_cast"(%348) : (i32) -> index
      %350 = "arith.minsi"(%349, %342) : (index, index) -> index
      %351 = "arith.subi"(%342, %350) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %352 = "arith.minsi"(%346, %15) : (index, index) -> index
      %353 = "arith.subi"(%346, %352) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %354 = "arith.cmpi"(%351, %5) <{predicate = 2 : i64}> : (index, index) -> i1
      %355 = "arith.cmpi"(%353, %7) <{predicate = 2 : i64}> : (index, index) -> i1
      %356 = "arith.ori"(%354, %355) : (i1, i1) -> i1
      %357 = "memref.subview"(%337, %351, %353) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<32x64xf16, strided<[64, 1], offset: ?>>, index, index) -> memref<?x?xf16, strided<[64, 1], offset: ?>>
      %358 = "memref.subview"(%338, %350, %352, %351, %353) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<32x64xf16>, index, index, index, index) -> memref<?x?xf16, strided<[64, 1], offset: ?>>
      "hivm.hir.load"(%357, %358, %6, %352, %356) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<?x?xf16, strided<[64, 1], offset: ?>>, memref<?x?xf16, strided<[64, 1], offset: ?>>, f16, index, i1) -> ()
      %359 = "bufferization.to_tensor"(%338) <{restrict, writable}> : (memref<32x64xf16>) -> tensor<32x64xf16>
      %360 = "hivm.hir.vcast"(%324, %294) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<64x32xf32>, tensor<64x32xf16>) -> tensor<64x32xf16>
      %361 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<64x32xf16>
      %362 = "bufferization.to_tensor"(%361) <{restrict, writable}> : (memref<64x32xf16>) -> tensor<64x32xf16>
      %363 = "hivm.hir.store"(%360, %362) {"inserted-store"} : (tensor<64x32xf16>, tensor<64x32xf16>) -> tensor<64x32xf16>
      %364 = "tensor.empty"() : () -> tensor<64x32xf16>
      %365 = "hivm.hir.load"(%363, %364) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<64x32xf16>, tensor<64x32xf16>) -> tensor<64x32xf16>
      %366 = "tensor.empty"() : () -> tensor<64x64xf32>
      %367 = "hivm.hir.mmadL1"(%365, %359, %1, %7, %5, %353, %366) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_already_inserted = true} : (tensor<64x32xf16>, tensor<32x64xf16>, i1, index, index, index, tensor<64x64xf32>) -> tensor<64x64xf32>
      %368 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<64x64xf32>
      %369 = "bufferization.to_tensor"(%368) <{restrict, writable}> : (memref<64x64xf32>) -> tensor<64x64xf32>
      %370 = "hivm.hir.fixpipe"(%367, %369) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<64x64xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
      %371 = "tensor.empty"() : () -> tensor<64x64xf32>
      %372 = "hivm.hir.load"(%370, %371) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<64x64xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
      %373 = "tensor.empty"() : () -> tensor<64x64xf32>
      %374 = "hivm.hir.vadd"(%372, %333, %373) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x64xf32>, tensor<64x64xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
      %375 = "hivm.hir.vmul"(%arg22, %327, %61) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
      %376 = "hivm.hir.vadd"(%375, %330, %60) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
      %377 = "arith.addi"(%arg24, %11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %378 = "arith.addi"(%arg25, %11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      "scf.yield"(%374, %376, %318, %377, %378) : (tensor<64x64xf32>, tensor<64xf32>, tensor<64xf32>, i32, i32) -> ()
    }) : (i32, i32, i32, tensor<64x64xf32>, tensor<64xf32>, tensor<64xf32>, i32, i32) -> (tensor<64x64xf32>, tensor<64xf32>, tensor<64xf32>, i32, i32)
    %122 = "arith.muli"(%25, %14) <{overflowFlags = #arith.overflow<none>}> {tt.divisibility = dense<64> : tensor<1xi32>} : (i32, i32) -> i32
    %123 = "arith.addi"(%25, %0) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %124 = "arith.muli"(%123, %14) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %125 = "arith.addi"(%78, %122) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %126 = "tensor.expand_shape"(%86) <{reassociation = [[0, 1]], static_output_shape = array<i64: 1, 32>}> : (tensor<32xi32>) -> tensor<1x32xi32>
    %127 = "tensor.empty"() : () -> tensor<64x32xi32>
    %128 = "tensor.empty"() : () -> tensor<64x32xi32>
    %129 = "tensor.empty"() : () -> tensor<64x32xi32>
    %130 = "tensor.expand_shape"(%84) <{reassociation = [[0, 1]], static_output_shape = array<i64: 64, 1>}> : (tensor<64xi32>) -> tensor<64x1xi32>
    %131 = "hivm.hir.vbrc"(%130, %129) <{broadcast_dims = array<i64: 1>}> : (tensor<64x1xi32>, tensor<64x32xi32>) -> tensor<64x32xi32>
    %132:5 = "scf.for"(%122, %124, %11, %121#0, %121#1, %121#2, %125, %125) ({
    ^bb0(%arg14: i32, %arg15: tensor<64x64xf32>, %arg16: tensor<64xf32>, %arg17: tensor<64xf32>, %arg18: i32, %arg19: i32):
      %152 = "arith.maxsi"(%arg18, %13) : (i32, i32) -> i32
      %153 = "arith.index_cast"(%152) : (i32) -> index
      %154 = "arith.muli"(%153, %7) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %155 = "memref.reinterpret_cast"(%arg7, %154) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 32, 64>, static_strides = array<i64: 64, 1>}> : (memref<?xf16>, index) -> memref<32x64xf16, strided<[64, 1], offset: ?>>
      %156 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<32x64xf16>
      %157 = "arith.divsi"(%154, %7) : (index, index) -> index
      %158 = "arith.subi"(%97, %157) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %159 = "arith.maxsi"(%158, %15) : (index, index) -> index
      %160 = "arith.minsi"(%159, %5) : (index, index) -> index
      %161 = "arith.remsi"(%154, %7) : (index, index) -> index
      %162 = "arith.subi"(%7, %161) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %163 = "arith.maxsi"(%162, %15) : (index, index) -> index
      %164 = "arith.minsi"(%163, %7) : (index, index) -> index
      %165 = "arith.subi"(%13, %arg18) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %166 = "arith.maxsi"(%165, %13) : (i32, i32) -> i32
      %167 = "arith.index_cast"(%166) : (i32) -> index
      %168 = "arith.minsi"(%167, %160) : (index, index) -> index
      %169 = "arith.subi"(%160, %168) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %170 = "arith.minsi"(%164, %15) : (index, index) -> index
      %171 = "arith.subi"(%164, %170) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %172 = "arith.cmpi"(%169, %5) <{predicate = 2 : i64}> : (index, index) -> i1
      %173 = "arith.cmpi"(%171, %7) <{predicate = 2 : i64}> : (index, index) -> i1
      %174 = "arith.ori"(%172, %173) : (i1, i1) -> i1
      %175 = "memref.subview"(%155, %169, %171) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<32x64xf16, strided<[64, 1], offset: ?>>, index, index) -> memref<?x?xf16, strided<[64, 1], offset: ?>>
      %176 = "memref.subview"(%156, %168, %170, %169, %171) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<32x64xf16>, index, index, index, index) -> memref<?x?xf16, strided<[64, 1], offset: ?>>
      "hivm.hir.load"(%175, %176, %6, %170, %174) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<?x?xf16, strided<[64, 1], offset: ?>>, memref<?x?xf16, strided<[64, 1], offset: ?>>, f16, index, i1) -> ()
      %177 = "bufferization.to_tensor"(%156) <{restrict, writable}> : (memref<32x64xf16>) -> tensor<32x64xf16>
      %178 = "tensor.empty"() : () -> tensor<64x32xf16>
      %179 = "tensor.empty"() : () -> tensor<64x32xf32>
      %180 = "hivm.hir.mmadL1"(%120, %177, %1, %112, %114, %169, %179) <{b_transpose, operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_already_inserted = true} : (tensor<64x64xf16>, tensor<32x64xf16>, i1, index, index, index, tensor<64x32xf32>) -> tensor<64x32xf32>
      %181 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<64x32xf32>
      %182 = "bufferization.to_tensor"(%181) <{restrict, writable}> : (memref<64x32xf32>) -> tensor<64x32xf32>
      %183 = "hivm.hir.fixpipe"(%180, %182) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<64x32xf32>, tensor<64x32xf32>) -> tensor<64x32xf32>
      %184 = "tensor.empty"() : () -> tensor<64x32xf32>
      %185 = "hivm.hir.load"(%183, %184) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<64x32xf32>, tensor<64x32xf32>) -> tensor<64x32xf32>
      %186 = "tensor.empty"() : () -> tensor<1x32xi32>
      %187 = "hivm.hir.vadd"(%126, %arg14, %186) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1x32xi32>, i32, tensor<1x32xi32>) -> tensor<1x32xi32>
      %188 = "hivm.hir.vbrc"(%187, %128) <{broadcast_dims = array<i64: 0>}> : (tensor<1x32xi32>, tensor<64x32xi32>) -> tensor<64x32xi32>
      %189 = "hivm.hir.vmax"(%131, %188, %127) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x32xi32>, tensor<64x32xi32>, tensor<64x32xi32>) -> tensor<64x32xi32>
      %190 = "tensor.empty"() : () -> tensor<64x32xi1>
      %191 = "hivm.hir.vcmp"(%189, %131, %190) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<64x32xi32>, tensor<64x32xi32>, tensor<64x32xi1>) -> tensor<64x32xi1>
      %192 = "hivm.hir.vmul"(%185, %89, %42) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x32xf32>, f32, tensor<64x32xf32>) -> tensor<64x32xf32>
      %193 = "hivm.hir.vsel"(%191, %12, %10, %41) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<64x32xi1>, f32, f32, tensor<64x32xf32>) -> tensor<64x32xf32>
      %194 = "hivm.hir.vadd"(%192, %193, %40) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x32xf32>, tensor<64x32xf32>, tensor<64x32xf32>) -> tensor<64x32xf32>
      %195 = "tensor.empty"() : () -> tensor<64x1xf32>
      %196 = "hivm.hir.vreduce"(%194, %195) <{arith = #hivm.reduce_op<max>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 1>}> : (tensor<64x32xf32>, tensor<64x1xf32>) -> tensor<64x1xf32>
      %197 = "tensor.collapse_shape"(%196) <{reassociation = [[0, 1]]}> : (tensor<64x1xf32>) -> tensor<64xf32>
      %198 = "tensor.empty"() : () -> tensor<64xi1>
      %199 = "tensor.empty"() : () -> tensor<64xi1>
      %200 = "tensor.empty"() : () -> tensor<64xi1>
      %201 = "tensor.empty"() : () -> tensor<64xi1>
      %202 = "hivm.hir.vcmp"(%arg17, %arg17, %201) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>, tensor<64xi1>) -> tensor<64xi1>
      %203 = "hivm.hir.vnot"(%202, %200) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<64xi1>, tensor<64xi1>) -> tensor<64xi1>
      %204 = "hivm.hir.vcmp"(%197, %197, %199) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>, tensor<64xi1>) -> tensor<64xi1>
      %205 = "hivm.hir.vnot"(%204, %198) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<64xi1>, tensor<64xi1>) -> tensor<64xi1>
      %206 = "hivm.hir.vmax"(%arg17, %197, %59) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
      %207 = "hivm.hir.vsel"(%203, %197, %206, %58) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<64xi1>, tensor<64xf32>, tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
      %208 = "hivm.hir.vsel"(%205, %arg17, %207, %57) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<64xi1>, tensor<64xf32>, tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
      %209 = "tensor.expand_shape"(%208) <{reassociation = [[0, 1]], static_output_shape = array<i64: 64, 1>}> : (tensor<64xf32>) -> tensor<64x1xf32>
      %210 = "hivm.hir.vbrc"(%209, %39) <{broadcast_dims = array<i64: 1>}> : (tensor<64x1xf32>, tensor<64x32xf32>) -> tensor<64x32xf32>
      %211 = "hivm.hir.vsub"(%194, %210, %38) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x32xf32>, tensor<64x32xf32>, tensor<64x32xf32>) -> tensor<64x32xf32>
      %212 = "hivm.hir.vmul"(%211, %3, %37) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x32xf32>, f32, tensor<64x32xf32>) -> tensor<64x32xf32>
      %213 = "hivm.hir.vexp"(%212, %36) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<64x32xf32>, tensor<64x32xf32>) -> tensor<64x32xf32>
      %214 = "hivm.hir.vsub"(%arg17, %208, %56) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
      %215 = "hivm.hir.vmul"(%214, %3, %55) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, f32, tensor<64xf32>) -> tensor<64xf32>
      %216 = "hivm.hir.vexp"(%215, %54) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
      %217 = "tensor.empty"() : () -> tensor<64x1xf32>
      %218 = "hivm.hir.vreduce"(%213, %217) <{arith = #hivm.reduce_op<sum>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 1>}> : (tensor<64x32xf32>, tensor<64x1xf32>) -> tensor<64x1xf32>
      %219 = "tensor.collapse_shape"(%218) <{reassociation = [[0, 1]]}> : (tensor<64x1xf32>) -> tensor<64xf32>
      %220 = "tensor.expand_shape"(%216) <{reassociation = [[0, 1]], static_output_shape = array<i64: 64, 1>}> : (tensor<64xf32>) -> tensor<64x1xf32>
      %221 = "hivm.hir.vbrc"(%220, %31) <{broadcast_dims = array<i64: 1>}> : (tensor<64x1xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
      %222 = "hivm.hir.vmul"(%arg15, %221, %30) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x64xf32>, tensor<64x64xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
      %223 = "arith.maxsi"(%arg19, %13) : (i32, i32) -> i32
      %224 = "arith.index_cast"(%223) : (i32) -> index
      %225 = "arith.muli"(%224, %7) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %226 = "memref.reinterpret_cast"(%arg8, %225) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 32, 64>, static_strides = array<i64: 64, 1>}> : (memref<?xf16>, index) -> memref<32x64xf16, strided<[64, 1], offset: ?>>
      %227 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<32x64xf16>
      %228 = "arith.divsi"(%225, %7) : (index, index) -> index
      %229 = "arith.subi"(%97, %228) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %230 = "arith.maxsi"(%229, %15) : (index, index) -> index
      %231 = "arith.minsi"(%230, %5) : (index, index) -> index
      %232 = "arith.remsi"(%225, %7) : (index, index) -> index
      %233 = "arith.subi"(%7, %232) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %234 = "arith.maxsi"(%233, %15) : (index, index) -> index
      %235 = "arith.minsi"(%234, %7) : (index, index) -> index
      %236 = "arith.subi"(%13, %arg19) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %237 = "arith.maxsi"(%236, %13) : (i32, i32) -> i32
      %238 = "arith.index_cast"(%237) : (i32) -> index
      %239 = "arith.minsi"(%238, %231) : (index, index) -> index
      %240 = "arith.subi"(%231, %239) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %241 = "arith.minsi"(%235, %15) : (index, index) -> index
      %242 = "arith.subi"(%235, %241) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %243 = "arith.cmpi"(%240, %5) <{predicate = 2 : i64}> : (index, index) -> i1
      %244 = "arith.cmpi"(%242, %7) <{predicate = 2 : i64}> : (index, index) -> i1
      %245 = "arith.ori"(%243, %244) : (i1, i1) -> i1
      %246 = "memref.subview"(%226, %240, %242) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<32x64xf16, strided<[64, 1], offset: ?>>, index, index) -> memref<?x?xf16, strided<[64, 1], offset: ?>>
      %247 = "memref.subview"(%227, %239, %241, %240, %242) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<32x64xf16>, index, index, index, index) -> memref<?x?xf16, strided<[64, 1], offset: ?>>
      "hivm.hir.load"(%246, %247, %6, %241, %245) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<?x?xf16, strided<[64, 1], offset: ?>>, memref<?x?xf16, strided<[64, 1], offset: ?>>, f16, index, i1) -> ()
      %248 = "bufferization.to_tensor"(%227) <{restrict, writable}> : (memref<32x64xf16>) -> tensor<32x64xf16>
      %249 = "hivm.hir.vcast"(%213, %178) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<64x32xf32>, tensor<64x32xf16>) -> tensor<64x32xf16>
      %250 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<64x32xf16>
      %251 = "bufferization.to_tensor"(%250) <{restrict, writable}> : (memref<64x32xf16>) -> tensor<64x32xf16>
      %252 = "hivm.hir.store"(%249, %251) {"inserted-store"} : (tensor<64x32xf16>, tensor<64x32xf16>) -> tensor<64x32xf16>
      %253 = "tensor.empty"() : () -> tensor<64x32xf16>
      %254 = "hivm.hir.load"(%252, %253) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<64x32xf16>, tensor<64x32xf16>) -> tensor<64x32xf16>
      %255 = "tensor.empty"() : () -> tensor<64x64xf32>
      %256 = "hivm.hir.mmadL1"(%254, %248, %1, %7, %5, %242, %255) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_already_inserted = true} : (tensor<64x32xf16>, tensor<32x64xf16>, i1, index, index, index, tensor<64x64xf32>) -> tensor<64x64xf32>
      %257 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<64x64xf32>
      %258 = "bufferization.to_tensor"(%257) <{restrict, writable}> : (memref<64x64xf32>) -> tensor<64x64xf32>
      %259 = "hivm.hir.fixpipe"(%256, %258) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<64x64xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
      %260 = "tensor.empty"() : () -> tensor<64x64xf32>
      %261 = "hivm.hir.load"(%259, %260) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<64x64xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
      %262 = "tensor.empty"() : () -> tensor<64x64xf32>
      %263 = "hivm.hir.vadd"(%261, %222, %262) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x64xf32>, tensor<64x64xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
      %264 = "hivm.hir.vmul"(%arg16, %216, %53) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
      %265 = "hivm.hir.vadd"(%264, %219, %52) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
      %266 = "arith.addi"(%arg18, %11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %267 = "arith.addi"(%arg19, %11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      "scf.yield"(%263, %265, %208, %266, %267) : (tensor<64x64xf32>, tensor<64xf32>, tensor<64xf32>, i32, i32) -> ()
    }) : (i32, i32, i32, tensor<64x64xf32>, tensor<64xf32>, tensor<64xf32>, i32, i32) -> (tensor<64x64xf32>, tensor<64xf32>, tensor<64xf32>, i32, i32)
    %133 = "hivm.hir.vln"(%132#1, %51) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
    %134 = "tensor.empty"() : () -> tensor<64xf32>
    %135 = "hivm.hir.vbrc"(%2, %134) <{broadcast_dims = array<i64>}> : (f32, tensor<64xf32>) -> tensor<64xf32>
    %136 = "hivm.hir.vln"(%135, %50) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
    %137 = "hivm.hir.vdiv"(%133, %136, %49) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
    %138 = "hivm.hir.vadd"(%132#2, %137, %48) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
    %139 = "tensor.expand_shape"(%132#1) <{reassociation = [[0, 1]], static_output_shape = array<i64: 64, 1>}> : (tensor<64xf32>) -> tensor<64x1xf32>
    %140 = "hivm.hir.vbrc"(%139, %29) <{broadcast_dims = array<i64: 1>}> : (tensor<64x1xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
    %141 = "hivm.hir.vdiv"(%132#0, %140, %28) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x64xf32>, tensor<64x64xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
    %142 = "arith.muli"(%22, %arg10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %143 = "arith.index_cast"(%142) : (i32) -> index
    %144 = "arith.index_cast"(%79) : (i32) -> index
    %145 = "arith.addi"(%143, %144) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %146 = "memref.reinterpret_cast"(%arg4, %145) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<64xf32, strided<[1], offset: ?>>
    "hivm.hir.store"(%138, %146) : (tensor<64xf32>, memref<64xf32, strided<[1], offset: ?>>) -> ()
    %147 = "tensor.empty"() : () -> tensor<64x64xf16>
    %148 = "hivm.hir.vcast"(%141, %147) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<64x64xf32>, tensor<64x64xf16>) -> tensor<64x64xf16>
    %149 = "memref.reinterpret_cast"(%arg9, %96) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64, 64>, static_strides = array<i64: 64, 1>}> : (memref<?xf16>, index) -> memref<64x64xf16, strided<[64, 1], offset: ?>>
    %150 = "tensor.extract_slice"(%148, %111, %113, %112, %114) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<64x64xf16>, index, index, index, index) -> tensor<?x?xf16>
    %151 = "memref.subview"(%149, %112, %114) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<64x64xf16, strided<[64, 1], offset: ?>>, index, index) -> memref<?x?xf16, strided<[64, 1], offset: ?>>
    "hivm.hir.store"(%150, %151) : (tensor<?x?xf16>, memref<?x?xf16, strided<[64, 1], offset: ?>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, false, true, false, true, true, true, true, false, false, false, false]> : vector<14xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<MIX>, mix_mode = "mix", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<MIX>} : () -> ()

