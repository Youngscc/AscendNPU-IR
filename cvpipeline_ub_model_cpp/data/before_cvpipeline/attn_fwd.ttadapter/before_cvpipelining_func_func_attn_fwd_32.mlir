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
    "annotation.mark"(%15) {logical_block_num} : (i32) -> ()
    %16 = "hivm.hir.get_block_idx"() : () -> i64
    %17 = "arith.trunci"(%16) : (i64) -> i32
    %18 = "arith.divsi"(%17, %arg28) : (i32, i32) -> i32
    %19 = "arith.remsi"(%18, %arg27) : (i32, i32) -> i32
    %20 = "arith.muli"(%arg28, %arg27) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %21 = "arith.divsi"(%17, %20) : (i32, i32) -> i32
    %22 = "arith.remsi"(%21, %arg26) : (i32, i32) -> i32
    %23 = "tensor.empty"() : () -> tensor<1xf32>
    %24 = "tensor.empty"() : () -> tensor<1xf32>
    %25 = "tensor.empty"() : () -> tensor<128x128xf32>
    %26 = "tensor.empty"() : () -> tensor<128x128xf32>
    %27 = "tensor.empty"() : () -> tensor<128x128xf32>
    %28 = "tensor.empty"() : () -> tensor<128x128xf32>
    %29 = "tensor.empty"() : () -> tensor<128x128xf32>
    %30 = "hivm.hir.vbrc"(%9, %29) <{broadcast_dims = array<i64>}> : (f32, tensor<128x128xf32>) -> tensor<128x128xf32>
    %31 = "tensor.empty"() : () -> tensor<128x32xf32>
    %32 = "tensor.empty"() : () -> tensor<128x32xf32>
    %33 = "tensor.empty"() : () -> tensor<128x32xf32>
    %34 = "tensor.empty"() : () -> tensor<128x32xf32>
    %35 = "tensor.empty"() : () -> tensor<128x32xf32>
    %36 = "tensor.empty"() : () -> tensor<128x32xf32>
    %37 = "hivm.hir.vbrc"(%7, %36) <{broadcast_dims = array<i64>}> : (f32, tensor<128x32xf32>) -> tensor<128x32xf32>
    %38 = "tensor.empty"() : () -> tensor<128xf32>
    %39 = "tensor.empty"() : () -> tensor<128xf32>
    %40 = "tensor.empty"() : () -> tensor<128xf32>
    %41 = "tensor.empty"() : () -> tensor<128xf32>
    %42 = "tensor.empty"() : () -> tensor<128xf32>
    %43 = "tensor.empty"() : () -> tensor<128xf32>
    %44 = "tensor.empty"() : () -> tensor<128xf32>
    %45 = "tensor.empty"() : () -> tensor<128xf32>
    %46 = "tensor.empty"() : () -> tensor<128xf32>
    %47 = "tensor.empty"() : () -> tensor<128xf32>
    %48 = "tensor.empty"() : () -> tensor<128xf32>
    %49 = "tensor.empty"() : () -> tensor<128xf32>
    %50 = "tensor.empty"() : () -> tensor<128xf32>
    %51 = "tensor.empty"() : () -> tensor<128xf32>
    %52 = "hivm.hir.vbrc"(%7, %51) <{broadcast_dims = array<i64>}> : (f32, tensor<128xf32>) -> tensor<128xf32>
    %53 = "hivm.hir.vbrc"(%5, %50) <{broadcast_dims = array<i64>}> : (f32, tensor<128xf32>) -> tensor<128xf32>
    %54 = "arith.divsi"(%19, %arg22) : (i32, i32) -> i32
    %55 = "arith.remsi"(%19, %arg22) : (i32, i32) -> i32
    %56 = "arith.extsi"(%54) : (i32) -> i64
    %57 = "arith.extsi"(%arg9) : (i32) -> i64
    %58 = "arith.muli"(%56, %57) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
    %59 = "arith.extsi"(%55) : (i32) -> i64
    %60 = "arith.extsi"(%arg10) : (i32) -> i64
    %61 = "arith.muli"(%59, %60) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
    %62 = "arith.addi"(%58, %61) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
    %63 = "arith.extsi"(%arg18) : (i32) -> i64
    %64 = "arith.muli"(%56, %63) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
    %65 = "arith.extsi"(%arg19) : (i32) -> i64
    %66 = "arith.muli"(%59, %65) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
    %67 = "arith.addi"(%64, %66) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
    %68 = "arith.extsi"(%arg12) : (i32) -> i64
    %69 = "arith.muli"(%56, %68) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
    %70 = "arith.extsi"(%arg13) : (i32) -> i64
    %71 = "arith.muli"(%59, %70) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
    %72 = "arith.addi"(%69, %71) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
    %73 = "arith.muli"(%22, %10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %74 = "arith.index_cast"(%62) : (i64) -> index
    %75 = "arith.index_cast"(%73) : (i32) -> index
    %76 = "arith.index_cast"(%arg11) : (i32) -> index
    %77 = "arith.muli"(%75, %76) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %78 = "arith.addi"(%74, %77) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %79 = "memref.reinterpret_cast"(%arg3, %78, %76) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 128, 128>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xf16>, index, index) -> memref<128x128xf16, strided<[?, 1], offset: ?>>
    %80 = "arith.index_cast"(%67) : (i64) -> index
    %81 = "arith.index_cast"(%arg20) : (i32) -> index
    %82 = "arith.muli"(%75, %81) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %83 = "arith.addi"(%80, %82) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %84 = "memref.reinterpret_cast"(%arg8, %83, %81) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 128, 128>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xf16>, index, index) -> memref<128x128xf16, strided<[?, 1], offset: ?>>
    %85 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<128x128xf16>
    %86 = "arith.addi"(%75, %4) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %87 = "arith.index_cast"(%arg24) : (i32) -> index
    %88 = "arith.maxsi"(%75, %87) : (index, index) -> index
    %89 = "arith.minsi"(%86, %88) : (index, index) -> index
    %90 = "arith.subi"(%89, %75) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %91 = "arith.cmpi"(%90, %4) <{predicate = 2 : i64}> : (index, index) -> i1
    %92 = "memref.subview"(%79, %90) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 128>, static_strides = array<i64: 1, 1>}> : (memref<128x128xf16, strided<[?, 1], offset: ?>>, index) -> memref<?x128xf16, strided<[?, 1], offset: ?>>
    %93 = "memref.subview"(%85, %90) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 128>, static_strides = array<i64: 1, 1>}> : (memref<128x128xf16>, index) -> memref<?x128xf16, strided<[128, 1]>>
    "hivm.hir.load"(%92, %93, %3, %12, %91) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<?x128xf16, strided<[?, 1], offset: ?>>, memref<?x128xf16, strided<[128, 1]>>, f16, index, i1) -> ()
    %94 = "bufferization.to_tensor"(%85) <{restrict, writable}> : (memref<128x128xf16>) -> tensor<128x128xf16>
    %95 = "tensor.insert"(%arg6, %24, %12) : (f32, tensor<1xf32>, index) -> tensor<1xf32>
    %96 = "hivm.hir.vmul"(%95, %13, %23) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1xf32>, f32, tensor<1xf32>) -> tensor<1xf32>
    %97 = "tensor.extract"(%96, %12) {"DuplicateTensorExtractForCube::visitedLabel" = 1 : i32} : (tensor<1xf32>, index) -> f32
    %98 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<1xf32>
    %99 = "bufferization.to_tensor"(%98) <{restrict, writable}> : (memref<1xf32>) -> tensor<1xf32>
    %100 = "hivm.hir.store"(%96, %99) {"inserted-store"} : (tensor<1xf32>, tensor<1xf32>) -> tensor<1xf32>
    "annotation.mark"(%100) {hivm.tcore_type = #hivm.tcore_type<VECTOR>} : (tensor<1xf32>) -> ()
    %101 = "tensor.extract"(%100, %12) {"DuplicateTensorExtractForCube::newExtractLabel" = 1 : i32, "DuplicateTensorExtractForCube::visitedLabel" = 1 : i32} : (tensor<1xf32>, index) -> f32
    "annotation.mark"(%97, %101) <{keys = []}> {"DuplicateTensorExtractForCube::replacementLabel" = 1 : i32} : (f32, f32) -> ()
    %102 = "arith.muli"(%arg14, %6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %103 = "arith.muli"(%arg17, %6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %104 = "arith.index_cast"(%72) : (i64) -> index
    %105 = "arith.index_cast"(%arg14) : (i32) -> index
    %106 = "arith.index_cast"(%arg17) : (i32) -> index
    %107:5 = "scf.for"(%8, %arg25, %6, %53, %30, %52, %104, %104) ({
    ^bb0(%arg29: i32, %arg30: tensor<128xf32>, %arg31: tensor<128x128xf32>, %arg32: tensor<128xf32>, %arg33: index, %arg34: index):
      %127 = "memref.reinterpret_cast"(%arg5, %arg34, %106) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 32, 128>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xf16>, index, index) -> memref<32x128xf16, strided<[?, 1], offset: ?>>
      %128 = "memref.reinterpret_cast"(%arg4, %arg33, %105) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 128, 32>, static_strides = array<i64: 1, -9223372036854775808>}> : (memref<?xf16>, index, index) -> memref<128x32xf16, strided<[1, ?], offset: ?>>
      %129 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<128x32xf16>
      %130 = "arith.index_cast"(%arg29) : (i32) -> index
      %131 = "arith.addi"(%130, %11) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %132 = "arith.index_cast"(%arg25) : (i32) -> index
      %133 = "arith.maxsi"(%130, %132) : (index, index) -> index
      %134 = "arith.minsi"(%131, %133) : (index, index) -> index
      %135 = "arith.subi"(%134, %130) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %136 = "arith.cmpi"(%135, %11) <{predicate = 2 : i64}> : (index, index) -> i1
      %137 = "memref.subview"(%128, %135) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: 128, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<128x32xf16, strided<[1, ?], offset: ?>>, index) -> memref<128x?xf16, strided<[1, ?], offset: ?>>
      %138 = "memref.subview"(%129, %135) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: 128, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<128x32xf16>, index) -> memref<128x?xf16, strided<[32, 1]>>
      "hivm.hir.load"(%137, %138, %3, %12, %136) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = true, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<128x?xf16, strided<[1, ?], offset: ?>>, memref<128x?xf16, strided<[32, 1]>>, f16, index, i1) -> ()
      "annotation.mark"(%129) {MayImplicitTransposeWithLastAxis} : (memref<128x32xf16>) -> ()
      %139 = "bufferization.to_tensor"(%129) <{restrict, writable}> : (memref<128x32xf16>) -> tensor<128x32xf16>
      %140 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<128x32xf16>
      %141 = "bufferization.to_tensor"(%140) <{restrict, writable}> : (memref<128x32xf16>) -> tensor<128x32xf16>
      %142 = "hivm.hir.store"(%139, %141) {"inserted-store"} : (tensor<128x32xf16>, tensor<128x32xf16>) -> tensor<128x32xf16>
      %143 = "tensor.empty"() : () -> tensor<128x32xf16>
      %144 = "hivm.hir.load"(%142, %143) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<128x32xf16>, tensor<128x32xf16>) -> tensor<128x32xf16>
      "annotation.mark"(%139) {MayImplicitTransposeWithLastAxis} : (tensor<128x32xf16>) -> ()
      %145 = "tensor.empty"() : () -> tensor<128x32xf32>
      %146 = "hivm.hir.mmadL1"(%94, %144, %0, %90, %4, %11, %145) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_already_inserted = true} : (tensor<128x128xf16>, tensor<128x32xf16>, i1, index, index, index, tensor<128x32xf32>) -> tensor<128x32xf32>
      %147 = "tensor.extract_slice"(%146, %135) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: 128, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<128x32xf32>, index) -> tensor<128x?xf32>
      %148 = "memref_ext.alloc_workspace"(%arg2, %135) <{operandSegmentSizes = array<i32: 1, 1, 0>}> : (memref<?xi8>, index) -> memref<128x?xf32>
      "annotation.mark"(%148) {buffer_size_in_byte = 16384 : i64} : (memref<128x?xf32>) -> ()
      %149 = "bufferization.to_tensor"(%148) <{restrict, writable}> : (memref<128x?xf32>) -> tensor<128x?xf32>
      %150 = "hivm.hir.fixpipe"(%147, %149) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<128x?xf32>, tensor<128x?xf32>) -> tensor<128x?xf32>
      %151 = "tensor.empty"(%135) : (index) -> tensor<128x?xf32>
      %152 = "hivm.hir.load"(%150, %151) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<128x?xf32>, tensor<128x?xf32>) -> tensor<128x?xf32>
      %153 = "tensor.insert_slice"(%152, %37, %135) <{operandSegmentSizes = array<i32: 1, 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: 128, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<128x?xf32>, tensor<128x32xf32>, index) -> tensor<128x32xf32>
      %154 = "hivm.hir.vmul"(%153, %97, %35) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128x32xf32>, f32, tensor<128x32xf32>) -> tensor<128x32xf32>
      %155 = "tensor.empty"() : () -> tensor<128x1xf32>
      %156 = "hivm.hir.vreduce"(%154, %155) <{arith = #hivm.reduce_op<max>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 1>}> : (tensor<128x32xf32>, tensor<128x1xf32>) -> tensor<128x1xf32>
      %157 = "tensor.collapse_shape"(%156) <{reassociation = [[0, 1]]}> : (tensor<128x1xf32>) -> tensor<128xf32>
      %158 = "tensor.empty"() : () -> tensor<128xi1>
      %159 = "tensor.empty"() : () -> tensor<128xi1>
      %160 = "tensor.empty"() : () -> tensor<128xi1>
      %161 = "tensor.empty"() : () -> tensor<128xi1>
      %162 = "hivm.hir.vcmp"(%arg32, %arg32, %161) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<128xf32>, tensor<128xf32>, tensor<128xi1>) -> tensor<128xi1>
      %163 = "hivm.hir.vnot"(%162, %160) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<128xi1>, tensor<128xi1>) -> tensor<128xi1>
      %164 = "hivm.hir.vcmp"(%157, %157, %159) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<128xf32>, tensor<128xf32>, tensor<128xi1>) -> tensor<128xi1>
      %165 = "hivm.hir.vnot"(%164, %158) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<128xi1>, tensor<128xi1>) -> tensor<128xi1>
      %166 = "hivm.hir.vmax"(%arg32, %157, %49) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, tensor<128xf32>, tensor<128xf32>) -> tensor<128xf32>
      %167 = "hivm.hir.vsel"(%163, %157, %166, %48) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<128xi1>, tensor<128xf32>, tensor<128xf32>, tensor<128xf32>) -> tensor<128xf32>
      %168 = "hivm.hir.vsel"(%165, %arg32, %167, %47) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<128xi1>, tensor<128xf32>, tensor<128xf32>, tensor<128xf32>) -> tensor<128xf32>
      %169 = "tensor.expand_shape"(%168) <{reassociation = [[0, 1]], static_output_shape = array<i64: 128, 1>}> : (tensor<128xf32>) -> tensor<128x1xf32>
      %170 = "hivm.hir.vbrc"(%169, %34) <{broadcast_dims = array<i64: 1>}> : (tensor<128x1xf32>, tensor<128x32xf32>) -> tensor<128x32xf32>
      %171 = "hivm.hir.vsub"(%154, %170, %33) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128x32xf32>, tensor<128x32xf32>, tensor<128x32xf32>) -> tensor<128x32xf32>
      %172 = "hivm.hir.vmul"(%171, %2, %32) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128x32xf32>, f32, tensor<128x32xf32>) -> tensor<128x32xf32>
      %173 = "hivm.hir.vexp"(%172, %31) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<128x32xf32>, tensor<128x32xf32>) -> tensor<128x32xf32>
      %174 = "tensor.empty"() : () -> tensor<128x1xf32>
      %175 = "hivm.hir.vreduce"(%173, %174) <{arith = #hivm.reduce_op<sum>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 1>}> : (tensor<128x32xf32>, tensor<128x1xf32>) -> tensor<128x1xf32>
      %176 = "tensor.collapse_shape"(%175) <{reassociation = [[0, 1]]}> : (tensor<128x1xf32>) -> tensor<128xf32>
      %177 = "hivm.hir.vsub"(%arg32, %168, %46) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, tensor<128xf32>, tensor<128xf32>) -> tensor<128xf32>
      %178 = "hivm.hir.vmul"(%177, %2, %45) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, f32, tensor<128xf32>) -> tensor<128xf32>
      %179 = "hivm.hir.vexp"(%178, %44) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, tensor<128xf32>) -> tensor<128xf32>
      %180 = "hivm.hir.vmul"(%arg30, %179, %43) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, tensor<128xf32>, tensor<128xf32>) -> tensor<128xf32>
      %181 = "hivm.hir.vadd"(%180, %176, %42) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, tensor<128xf32>, tensor<128xf32>) -> tensor<128xf32>
      %182 = "tensor.expand_shape"(%179) <{reassociation = [[0, 1]], static_output_shape = array<i64: 128, 1>}> : (tensor<128xf32>) -> tensor<128x1xf32>
      %183 = "hivm.hir.vbrc"(%182, %28) <{broadcast_dims = array<i64: 1>}> : (tensor<128x1xf32>, tensor<128x128xf32>) -> tensor<128x128xf32>
      %184 = "hivm.hir.vmul"(%arg31, %183, %27) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128x128xf32>, tensor<128x128xf32>, tensor<128x128xf32>) -> tensor<128x128xf32>
      %185 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<32x128xf16>
      %186 = "memref.subview"(%127, %135) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 128>, static_strides = array<i64: 1, 1>}> : (memref<32x128xf16, strided<[?, 1], offset: ?>>, index) -> memref<?x128xf16, strided<[?, 1], offset: ?>>
      %187 = "memref.subview"(%185, %135) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 128>, static_strides = array<i64: 1, 1>}> : (memref<32x128xf16>, index) -> memref<?x128xf16, strided<[128, 1]>>
      "hivm.hir.load"(%186, %187, %3, %12, %136) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<?x128xf16, strided<[?, 1], offset: ?>>, memref<?x128xf16, strided<[128, 1]>>, f16, index, i1) -> ()
      %188 = "bufferization.to_tensor"(%185) <{restrict, writable}> : (memref<32x128xf16>) -> tensor<32x128xf16>
      %189 = "tensor.empty"() : () -> tensor<128x32xf16>
      %190 = "hivm.hir.vcast"(%173, %189) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<128x32xf32>, tensor<128x32xf16>) -> tensor<128x32xf16>
      %191 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<128x32xf16>
      %192 = "bufferization.to_tensor"(%191) <{restrict, writable}> : (memref<128x32xf16>) -> tensor<128x32xf16>
      %193 = "hivm.hir.store"(%190, %192) {"inserted-store"} : (tensor<128x32xf16>, tensor<128x32xf16>) -> tensor<128x32xf16>
      %194 = "tensor.empty"() : () -> tensor<128x32xf16>
      %195 = "hivm.hir.load"(%193, %194) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<128x32xf16>, tensor<128x32xf16>) -> tensor<128x32xf16>
      %196 = "tensor.empty"() : () -> tensor<128x128xf32>
      %197 = "hivm.hir.mmadL1"(%195, %188, %0, %4, %11, %4, %196) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_already_inserted = true} : (tensor<128x32xf16>, tensor<32x128xf16>, i1, index, index, index, tensor<128x128xf32>) -> tensor<128x128xf32>
      %198 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<128x128xf32>
      %199 = "bufferization.to_tensor"(%198) <{restrict, writable}> : (memref<128x128xf32>) -> tensor<128x128xf32>
      %200 = "hivm.hir.fixpipe"(%197, %199) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<128x128xf32>, tensor<128x128xf32>) -> tensor<128x128xf32>
      %201 = "tensor.empty"() : () -> tensor<128x128xf32>
      %202 = "hivm.hir.load"(%200, %201) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<128x128xf32>, tensor<128x128xf32>) -> tensor<128x128xf32>
      %203 = "tensor.empty"() : () -> tensor<128x128xf32>
      %204 = "hivm.hir.vadd"(%202, %184, %203) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128x128xf32>, tensor<128x128xf32>, tensor<128x128xf32>) -> tensor<128x128xf32>
      %205 = "arith.index_cast"(%102) : (i32) -> index
      %206 = "arith.addi"(%arg33, %205) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %207 = "arith.index_cast"(%103) : (i32) -> index
      %208 = "arith.addi"(%arg34, %207) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      "scf.yield"(%181, %204, %168, %206, %208) : (tensor<128xf32>, tensor<128x128xf32>, tensor<128xf32>, index, index) -> ()
    }) : (i32, i32, i32, tensor<128xf32>, tensor<128x128xf32>, tensor<128xf32>, index, index) -> (tensor<128xf32>, tensor<128x128xf32>, tensor<128xf32>, index, index)
    %108 = "hivm.hir.vln"(%107#0, %41) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, tensor<128xf32>) -> tensor<128xf32>
    %109 = "tensor.empty"() : () -> tensor<128xf32>
    %110 = "hivm.hir.vbrc"(%1, %109) <{broadcast_dims = array<i64>}> : (f32, tensor<128xf32>) -> tensor<128xf32>
    %111 = "hivm.hir.vln"(%110, %40) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, tensor<128xf32>) -> tensor<128xf32>
    %112 = "hivm.hir.vdiv"(%108, %111, %39) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, tensor<128xf32>, tensor<128xf32>) -> tensor<128xf32>
    %113 = "hivm.hir.vadd"(%107#2, %112, %38) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, tensor<128xf32>, tensor<128xf32>) -> tensor<128xf32>
    %114 = "tensor.expand_shape"(%107#0) <{reassociation = [[0, 1]], static_output_shape = array<i64: 128, 1>}> : (tensor<128xf32>) -> tensor<128x1xf32>
    %115 = "hivm.hir.vbrc"(%114, %26) <{broadcast_dims = array<i64: 1>}> : (tensor<128x1xf32>, tensor<128x128xf32>) -> tensor<128x128xf32>
    %116 = "hivm.hir.vdiv"(%107#1, %115, %25) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128x128xf32>, tensor<128x128xf32>, tensor<128x128xf32>) -> tensor<128x128xf32>
    %117 = "arith.muli"(%19, %arg24) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %118 = "arith.index_cast"(%117) : (i32) -> index
    %119 = "arith.addi"(%118, %75) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %120 = "memref.reinterpret_cast"(%arg7, %119) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 128>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<128xf32, strided<[1], offset: ?>>
    %121 = "tensor.extract_slice"(%113, %90) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<128xf32>, index) -> tensor<?xf32>
    %122 = "memref.subview"(%120, %90) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<128xf32, strided<[1], offset: ?>>, index) -> memref<?xf32, strided<[1], offset: ?>>
    "hivm.hir.store"(%121, %122) : (tensor<?xf32>, memref<?xf32, strided<[1], offset: ?>>) -> ()
    %123 = "tensor.empty"() : () -> tensor<128x128xf16>
    %124 = "hivm.hir.vcast"(%116, %123) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<128x128xf32>, tensor<128x128xf16>) -> tensor<128x128xf16>
    %125 = "tensor.extract_slice"(%124, %90) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 128>, static_strides = array<i64: 1, 1>}> : (tensor<128x128xf16>, index) -> tensor<?x128xf16>
    %126 = "memref.subview"(%84, %90) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 128>, static_strides = array<i64: 1, 1>}> : (memref<128x128xf16, strided<[?, 1], offset: ?>>, index) -> memref<?x128xf16, strided<[?, 1], offset: ?>>
    "hivm.hir.store"(%125, %126) : (tensor<?x128xf16>, memref<?x128xf16, strided<[?, 1], offset: ?>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, false, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false]> : vector<29xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<MIX>, mix_mode = "mix", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<MIX>} : () -> ()

