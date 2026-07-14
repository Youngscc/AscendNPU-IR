"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, i32, i32, i32, i32) -> (), sym_name = "merge_16x16_to_64x64_inverse_kernel"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xbf16>, %arg4: memref<?xbf16>, %arg5: memref<?xbf16>, %arg6: i32, %arg7: i32, %arg8: i32, %arg9: i32):
    %0 = "arith.constant"() <{value = 0 : index}> : () -> index
    %1 = "arith.constant"() <{value = true}> : () -> i1
    %2 = "arith.constant"() <{value = -1.000000e+00 : f32}> : () -> f32
    %3 = "arith.constant"() <{value = 32 : index}> : () -> index
    %4 = "arith.constant"() <{value = 256 : index}> : () -> index
    %5 = "arith.constant"() <{value = 16 : index}> : () -> index
    %6 = "arith.constant"() <{value = 64 : index}> : () -> index
    %7 = "arith.constant"() <{value = 0.000000e+00 : bf16}> : () -> bf16
    %8 = "arith.constant"() <{value = 32 : i32}> : () -> i32
    %9 = "arith.constant"() <{value = 48 : i32}> : () -> i32
    %10 = "arith.constant"() <{value = 4 : i32}> : () -> i32
    %11 = "arith.constant"() <{value = 64 : i32}> : () -> i32
    %12 = "arith.constant"() <{value = 16 : i32}> : () -> i32
    %13 = "arith.constant"() <{value = 0.000000e+00 : f32}> : () -> f32
    "hivm.hir.set_mask_norm"() : () -> ()
    %14 = "arith.muli"(%arg7, %arg8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %15 = "arith.muli"(%14, %arg9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%15) {logical_block_num} : (i32) -> ()
    %16 = "hivm.hir.get_block_idx"() : () -> i64
    %17 = "arith.trunci"(%16) : (i64) -> i32
    %18 = "arith.divsi"(%17, %arg9) : (i32, i32) -> i32
    %19 = "arith.remsi"(%18, %arg8) : (i32, i32) -> i32
    %20 = "arith.muli"(%arg9, %arg8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %21 = "arith.divsi"(%17, %20) : (i32, i32) -> i32
    %22 = "arith.remsi"(%21, %arg7) : (i32, i32) -> i32
    %23 = "tensor.empty"() : () -> tensor<16x16xf32>
    %24 = "tensor.empty"() : () -> tensor<16x16xf32>
    %25 = "tensor.empty"() : () -> tensor<16x16xf32>
    %26 = "tensor.empty"() : () -> tensor<16x16xf32>
    %27 = "tensor.empty"() : () -> tensor<16x16xf32>
    %28 = "tensor.empty"() : () -> tensor<16x16xf32>
    %29 = "tensor.empty"() : () -> tensor<16x16xf32>
    %30 = "tensor.empty"() : () -> tensor<16x16xf32>
    %31 = "tensor.empty"() : () -> tensor<16x16xf32>
    %32 = "tensor.empty"() : () -> tensor<16x16xf32>
    %33 = "tensor.empty"() : () -> tensor<32x32xf32>
    %34 = "tensor.empty"() : () -> tensor<32x32xf32>
    %35 = "tensor.empty"() : () -> tensor<32x32xf32>
    %36 = "tensor.empty"() : () -> tensor<32x32xf32>
    %37 = "hivm.hir.vbrc"(%13, %36) <{broadcast_dims = array<i64>}> : (f32, tensor<32x32xf32>) -> tensor<32x32xf32>
    %38 = "tensor.empty"() : () -> tensor<32x32xbf16>
    %39 = "tensor.empty"() : () -> tensor<32x32xbf16>
    %40 = "tensor.empty"() : () -> tensor<32x32xbf16>
    %41 = "tensor.empty"() : () -> tensor<32x32xbf16>
    %42 = "hivm.hir.vbrc"(%7, %41) <{broadcast_dims = array<i64>}> : (bf16, tensor<32x32xbf16>) -> tensor<32x32xbf16>
    %43 = "arith.divsi"(%19, %10) : (i32, i32) -> i32
    %44 = "arith.remsi"(%19, %10) : (i32, i32) -> i32
    %45 = "arith.muli"(%43, %arg6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %46 = "arith.muli"(%45, %10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %47 = "arith.addi"(%46, %44) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %48 = "arith.muli"(%47, %11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %49 = "arith.index_cast"(%48) : (i32) -> index
    %50 = "arith.muli"(%47, %12) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %51 = "arith.index_cast"(%50) : (i32) -> index
    %52 = "arith.muli"(%22, %11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %53 = "arith.addi"(%52, %12) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %54 = "arith.index_cast"(%53) : (i32) -> index
    %55 = "arith.muli"(%54, %6) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %56 = "arith.addi"(%51, %55) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %57 = "memref.reinterpret_cast"(%arg4, %56) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 64, 1>}> : (memref<?xbf16>, index) -> memref<16x16xbf16, strided<[64, 1], offset: ?>>
    %58 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x16xbf16>
    %59 = "arith.addi"(%54, %5) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %60 = "arith.index_cast"(%arg6) : (i32) -> index
    %61 = "arith.maxsi"(%54, %60) : (index, index) -> index
    %62 = "arith.minsi"(%59, %61) : (index, index) -> index
    %63 = "arith.subi"(%62, %54) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %64 = "arith.minsi"(%63, %5) : (index, index) -> index
    %65 = "arith.cmpi"(%64, %5) <{predicate = 2 : i64}> : (index, index) -> i1
    %66 = "memref.subview"(%57, %64) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16, strided<[64, 1], offset: ?>>, index) -> memref<?x16xbf16, strided<[64, 1], offset: ?>>
    %67 = "memref.subview"(%58, %64) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16>, index) -> memref<?x16xbf16, strided<[16, 1]>>
    "hivm.hir.load"(%66, %67, %7, %0, %65) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x16xbf16, strided<[64, 1], offset: ?>>, memref<?x16xbf16, strided<[16, 1]>>, bf16, index, i1) -> ()
    %68 = "bufferization.to_tensor"(%58) <{restrict, writable}> : (memref<16x16xbf16>) -> tensor<16x16xbf16>
    %69 = "hivm.hir.vcast"(%68, %32) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x16xbf16>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %70 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xf32>
    %71 = "bufferization.to_tensor"(%70) <{restrict, writable}> : (memref<16x16xf32>) -> tensor<16x16xf32>
    %72 = "hivm.hir.store"(%69, %71) {"inserted-store"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %73 = "tensor.empty"() : () -> tensor<16x16xf32>
    %74 = "hivm.hir.load"(%72, %73) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %75 = "arith.muli"(%54, %4) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %76 = "arith.addi"(%49, %75) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %77 = "memref.reinterpret_cast"(%arg3, %76) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 256, 1>}> : (memref<?xbf16>, index) -> memref<16x16xbf16, strided<[256, 1], offset: ?>>
    %78 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x16xbf16>
    %79 = "memref.subview"(%77, %64) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16, strided<[256, 1], offset: ?>>, index) -> memref<?x16xbf16, strided<[256, 1], offset: ?>>
    %80 = "memref.subview"(%78, %64) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16>, index) -> memref<?x16xbf16, strided<[16, 1]>>
    "hivm.hir.load"(%79, %80, %7, %0, %65) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x16xbf16, strided<[256, 1], offset: ?>>, memref<?x16xbf16, strided<[16, 1]>>, bf16, index, i1) -> ()
    %81 = "bufferization.to_tensor"(%78) <{restrict, writable}> : (memref<16x16xbf16>) -> tensor<16x16xbf16>
    %82 = "hivm.hir.vcast"(%81, %31) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x16xbf16>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %83 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xf32>
    %84 = "bufferization.to_tensor"(%83) <{restrict, writable}> : (memref<16x16xf32>) -> tensor<16x16xf32>
    %85 = "hivm.hir.store"(%82, %84) {"inserted-store"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %86 = "tensor.empty"() : () -> tensor<16x16xf32>
    %87 = "hivm.hir.load"(%85, %86) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %88 = "tensor.empty"() : () -> tensor<16x16xf32>
    %89 = "hivm.hir.mmadL1"(%74, %87, %1, %5, %5, %5, %88) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_already_inserted = true} : (tensor<16x16xf32>, tensor<16x16xf32>, i1, index, index, index, tensor<16x16xf32>) -> tensor<16x16xf32>
    %90 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xf32>
    %91 = "bufferization.to_tensor"(%90) <{restrict, writable}> : (memref<16x16xf32>) -> tensor<16x16xf32>
    %92 = "hivm.hir.fixpipe"(%89, %91) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %93 = "tensor.empty"() : () -> tensor<16x16xf32>
    %94 = "hivm.hir.load"(%92, %93) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %95 = "arith.index_cast"(%52) : (i32) -> index
    %96 = "arith.muli"(%95, %6) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %97 = "arith.addi"(%51, %96) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %98 = "memref.reinterpret_cast"(%arg4, %97) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 64, 1>}> : (memref<?xbf16>, index) -> memref<16x16xbf16, strided<[64, 1], offset: ?>>
    %99 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x16xbf16>
    %100 = "arith.addi"(%95, %5) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %101 = "arith.maxsi"(%95, %60) : (index, index) -> index
    %102 = "arith.minsi"(%100, %101) : (index, index) -> index
    %103 = "arith.subi"(%102, %95) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %104 = "arith.minsi"(%103, %5) : (index, index) -> index
    %105 = "arith.cmpi"(%104, %5) <{predicate = 2 : i64}> : (index, index) -> i1
    %106 = "memref.subview"(%98, %104) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16, strided<[64, 1], offset: ?>>, index) -> memref<?x16xbf16, strided<[64, 1], offset: ?>>
    %107 = "memref.subview"(%99, %104) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16>, index) -> memref<?x16xbf16, strided<[16, 1]>>
    "hivm.hir.load"(%106, %107, %7, %0, %105) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x16xbf16, strided<[64, 1], offset: ?>>, memref<?x16xbf16, strided<[16, 1]>>, bf16, index, i1) -> ()
    %108 = "bufferization.to_tensor"(%99) <{restrict, writable}> : (memref<16x16xbf16>) -> tensor<16x16xbf16>
    %109 = "hivm.hir.vcast"(%108, %30) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x16xbf16>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %110 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xf32>
    %111 = "bufferization.to_tensor"(%110) <{restrict, writable}> : (memref<16x16xf32>) -> tensor<16x16xf32>
    %112 = "hivm.hir.store"(%109, %111) {"inserted-store"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %113 = "tensor.empty"() : () -> tensor<16x16xf32>
    %114 = "hivm.hir.load"(%112, %113) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %115 = "tensor.empty"() : () -> tensor<16x16xf32>
    %116 = "hivm.hir.mmadL1"(%94, %114, %1, %5, %5, %5, %115) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_already_inserted = true} : (tensor<16x16xf32>, tensor<16x16xf32>, i1, index, index, index, tensor<16x16xf32>) -> tensor<16x16xf32>
    %117 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xf32>
    %118 = "bufferization.to_tensor"(%117) <{restrict, writable}> : (memref<16x16xf32>) -> tensor<16x16xf32>
    %119 = "hivm.hir.fixpipe"(%116, %118) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %120 = "tensor.empty"() : () -> tensor<16x16xf32>
    %121 = "hivm.hir.load"(%119, %120) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %122 = "hivm.hir.vmul"(%121, %2, %29) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x16xf32>, f32, tensor<16x16xf32>) -> tensor<16x16xf32>
    %123 = "hivm.hir.vadd"(%122, %13, %28) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x16xf32>, f32, tensor<16x16xf32>) -> tensor<16x16xf32>
    %124 = "arith.addi"(%52, %9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %125 = "arith.index_cast"(%124) : (i32) -> index
    %126 = "arith.muli"(%125, %6) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %127 = "arith.addi"(%51, %126) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %128 = "memref.reinterpret_cast"(%arg4, %127) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 64, 1>}> : (memref<?xbf16>, index) -> memref<16x16xbf16, strided<[64, 1], offset: ?>>
    %129 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x16xbf16>
    %130 = "arith.addi"(%125, %5) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %131 = "arith.maxsi"(%125, %60) : (index, index) -> index
    %132 = "arith.minsi"(%130, %131) : (index, index) -> index
    %133 = "arith.subi"(%132, %125) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %134 = "arith.minsi"(%133, %5) : (index, index) -> index
    %135 = "arith.cmpi"(%134, %5) <{predicate = 2 : i64}> : (index, index) -> i1
    %136 = "memref.subview"(%128, %134) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16, strided<[64, 1], offset: ?>>, index) -> memref<?x16xbf16, strided<[64, 1], offset: ?>>
    %137 = "memref.subview"(%129, %134) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16>, index) -> memref<?x16xbf16, strided<[16, 1]>>
    "hivm.hir.load"(%136, %137, %7, %0, %135) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x16xbf16, strided<[64, 1], offset: ?>>, memref<?x16xbf16, strided<[16, 1]>>, bf16, index, i1) -> ()
    %138 = "bufferization.to_tensor"(%129) <{restrict, writable}> : (memref<16x16xbf16>) -> tensor<16x16xbf16>
    %139 = "hivm.hir.vcast"(%138, %27) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x16xbf16>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %140 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xf32>
    %141 = "bufferization.to_tensor"(%140) <{restrict, writable}> : (memref<16x16xf32>) -> tensor<16x16xf32>
    %142 = "hivm.hir.store"(%139, %141) {"inserted-store"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %143 = "tensor.empty"() : () -> tensor<16x16xf32>
    %144 = "hivm.hir.load"(%142, %143) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %145 = "arith.muli"(%125, %4) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %146 = "arith.addi"(%49, %145) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %147 = "arith.addi"(%146, %3) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %148 = "memref.reinterpret_cast"(%arg3, %147) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 256, 1>}> : (memref<?xbf16>, index) -> memref<16x16xbf16, strided<[256, 1], offset: ?>>
    %149 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x16xbf16>
    %150 = "memref.subview"(%148, %134) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16, strided<[256, 1], offset: ?>>, index) -> memref<?x16xbf16, strided<[256, 1], offset: ?>>
    %151 = "memref.subview"(%149, %134) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16>, index) -> memref<?x16xbf16, strided<[16, 1]>>
    "hivm.hir.load"(%150, %151, %7, %0, %135) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x16xbf16, strided<[256, 1], offset: ?>>, memref<?x16xbf16, strided<[16, 1]>>, bf16, index, i1) -> ()
    %152 = "bufferization.to_tensor"(%149) <{restrict, writable}> : (memref<16x16xbf16>) -> tensor<16x16xbf16>
    %153 = "hivm.hir.vcast"(%152, %26) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x16xbf16>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %154 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xf32>
    %155 = "bufferization.to_tensor"(%154) <{restrict, writable}> : (memref<16x16xf32>) -> tensor<16x16xf32>
    %156 = "hivm.hir.store"(%153, %155) {"inserted-store"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %157 = "tensor.empty"() : () -> tensor<16x16xf32>
    %158 = "hivm.hir.load"(%156, %157) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %159 = "tensor.empty"() : () -> tensor<16x16xf32>
    %160 = "hivm.hir.mmadL1"(%144, %158, %1, %5, %5, %5, %159) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_already_inserted = true} : (tensor<16x16xf32>, tensor<16x16xf32>, i1, index, index, index, tensor<16x16xf32>) -> tensor<16x16xf32>
    %161 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xf32>
    %162 = "bufferization.to_tensor"(%161) <{restrict, writable}> : (memref<16x16xf32>) -> tensor<16x16xf32>
    %163 = "hivm.hir.fixpipe"(%160, %162) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %164 = "tensor.empty"() : () -> tensor<16x16xf32>
    %165 = "hivm.hir.load"(%163, %164) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %166 = "arith.addi"(%52, %8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %167 = "arith.index_cast"(%166) : (i32) -> index
    %168 = "arith.muli"(%167, %6) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %169 = "arith.addi"(%51, %168) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %170 = "memref.reinterpret_cast"(%arg4, %169) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 64, 1>}> : (memref<?xbf16>, index) -> memref<16x16xbf16, strided<[64, 1], offset: ?>>
    %171 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x16xbf16>
    %172 = "arith.addi"(%167, %5) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %173 = "arith.maxsi"(%167, %60) : (index, index) -> index
    %174 = "arith.minsi"(%172, %173) : (index, index) -> index
    %175 = "arith.subi"(%174, %167) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %176 = "arith.minsi"(%175, %5) : (index, index) -> index
    %177 = "arith.cmpi"(%176, %5) <{predicate = 2 : i64}> : (index, index) -> i1
    %178 = "memref.subview"(%170, %176) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16, strided<[64, 1], offset: ?>>, index) -> memref<?x16xbf16, strided<[64, 1], offset: ?>>
    %179 = "memref.subview"(%171, %176) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16>, index) -> memref<?x16xbf16, strided<[16, 1]>>
    "hivm.hir.load"(%178, %179, %7, %0, %177) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x16xbf16, strided<[64, 1], offset: ?>>, memref<?x16xbf16, strided<[16, 1]>>, bf16, index, i1) -> ()
    %180 = "bufferization.to_tensor"(%171) <{restrict, writable}> : (memref<16x16xbf16>) -> tensor<16x16xbf16>
    %181 = "hivm.hir.vcast"(%180, %25) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x16xbf16>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %182 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xf32>
    %183 = "bufferization.to_tensor"(%182) <{restrict, writable}> : (memref<16x16xf32>) -> tensor<16x16xf32>
    %184 = "hivm.hir.store"(%181, %183) {"inserted-store"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %185 = "tensor.empty"() : () -> tensor<16x16xf32>
    %186 = "hivm.hir.load"(%184, %185) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %187 = "tensor.empty"() : () -> tensor<16x16xf32>
    %188 = "hivm.hir.mmadL1"(%165, %186, %1, %5, %5, %5, %187) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_already_inserted = true} : (tensor<16x16xf32>, tensor<16x16xf32>, i1, index, index, index, tensor<16x16xf32>) -> tensor<16x16xf32>
    %189 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xf32>
    %190 = "bufferization.to_tensor"(%189) <{restrict, writable}> : (memref<16x16xf32>) -> tensor<16x16xf32>
    %191 = "hivm.hir.fixpipe"(%188, %190) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %192 = "tensor.empty"() : () -> tensor<16x16xf32>
    %193 = "hivm.hir.load"(%191, %192) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %194 = "hivm.hir.vmul"(%193, %2, %24) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x16xf32>, f32, tensor<16x16xf32>) -> tensor<16x16xf32>
    %195 = "hivm.hir.vadd"(%194, %13, %23) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x16xf32>, f32, tensor<16x16xf32>) -> tensor<16x16xf32>
    %196 = "tensor.insert_slice"(%181, %37) <{operandSegmentSizes = array<i32: 1, 1, 0, 0, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 1, 1>}> : (tensor<16x16xf32>, tensor<32x32xf32>) -> tensor<32x32xf32>
    %197 = "tensor.insert_slice"(%139, %196) <{operandSegmentSizes = array<i32: 1, 1, 0, 0, 0>, static_offsets = array<i64: 16, 16>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 1, 1>}> : (tensor<16x16xf32>, tensor<32x32xf32>) -> tensor<32x32xf32>
    %198 = "tensor.insert_slice"(%195, %197) <{operandSegmentSizes = array<i32: 1, 1, 0, 0, 0>, static_offsets = array<i64: 16, 0>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 1, 1>}> : (tensor<16x16xf32>, tensor<32x32xf32>) -> tensor<32x32xf32>
    %199 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<32x32xf32>
    %200 = "bufferization.to_tensor"(%199) <{restrict, writable}> : (memref<32x32xf32>) -> tensor<32x32xf32>
    %201 = "hivm.hir.store"(%198, %200) {"inserted-store"} : (tensor<32x32xf32>, tensor<32x32xf32>) -> tensor<32x32xf32>
    %202 = "tensor.empty"() : () -> tensor<32x32xf32>
    %203 = "hivm.hir.load"(%201, %202) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<32x32xf32>, tensor<32x32xf32>) -> tensor<32x32xf32>
    %204 = "arith.muli"(%167, %4) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %205 = "arith.addi"(%49, %204) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %206 = "memref.reinterpret_cast"(%arg3, %205) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 32, 32>, static_strides = array<i64: 256, 1>}> : (memref<?xbf16>, index) -> memref<32x32xbf16, strided<[256, 1], offset: ?>>
    %207 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<32x32xbf16>
    %208 = "arith.addi"(%167, %3) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %209 = "arith.minsi"(%208, %173) : (index, index) -> index
    %210 = "arith.subi"(%209, %167) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %211 = "arith.minsi"(%210, %3) : (index, index) -> index
    %212 = "arith.cmpi"(%211, %3) <{predicate = 2 : i64}> : (index, index) -> i1
    %213 = "memref.subview"(%206, %211) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 32>, static_strides = array<i64: 1, 1>}> : (memref<32x32xbf16, strided<[256, 1], offset: ?>>, index) -> memref<?x32xbf16, strided<[256, 1], offset: ?>>
    %214 = "memref.subview"(%207, %211) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 32>, static_strides = array<i64: 1, 1>}> : (memref<32x32xbf16>, index) -> memref<?x32xbf16, strided<[32, 1]>>
    "hivm.hir.load"(%213, %214, %7, %0, %212) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x32xbf16, strided<[256, 1], offset: ?>>, memref<?x32xbf16, strided<[32, 1]>>, bf16, index, i1) -> ()
    %215 = "bufferization.to_tensor"(%207) <{restrict, writable}> : (memref<32x32xbf16>) -> tensor<32x32xbf16>
    %216 = "hivm.hir.vcast"(%215, %35) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<32x32xbf16>, tensor<32x32xf32>) -> tensor<32x32xf32>
    %217 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<32x32xf32>
    %218 = "bufferization.to_tensor"(%217) <{restrict, writable}> : (memref<32x32xf32>) -> tensor<32x32xf32>
    %219 = "hivm.hir.store"(%216, %218) {"inserted-store"} : (tensor<32x32xf32>, tensor<32x32xf32>) -> tensor<32x32xf32>
    %220 = "tensor.empty"() : () -> tensor<32x32xf32>
    %221 = "hivm.hir.load"(%219, %220) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<32x32xf32>, tensor<32x32xf32>) -> tensor<32x32xf32>
    %222 = "tensor.empty"() : () -> tensor<32x32xf32>
    %223 = "hivm.hir.mmadL1"(%203, %221, %1, %3, %3, %3, %222) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_already_inserted = true} : (tensor<32x32xf32>, tensor<32x32xf32>, i1, index, index, index, tensor<32x32xf32>) -> tensor<32x32xf32>
    %224 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<32x32xf32>
    %225 = "bufferization.to_tensor"(%224) <{restrict, writable}> : (memref<32x32xf32>) -> tensor<32x32xf32>
    %226 = "hivm.hir.fixpipe"(%223, %225) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<32x32xf32>, tensor<32x32xf32>) -> tensor<32x32xf32>
    %227 = "tensor.empty"() : () -> tensor<32x32xf32>
    %228 = "hivm.hir.load"(%226, %227) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<32x32xf32>, tensor<32x32xf32>) -> tensor<32x32xf32>
    %229 = "tensor.insert_slice"(%109, %37) <{operandSegmentSizes = array<i32: 1, 1, 0, 0, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 1, 1>}> : (tensor<16x16xf32>, tensor<32x32xf32>) -> tensor<32x32xf32>
    %230 = "tensor.insert_slice"(%69, %229) <{operandSegmentSizes = array<i32: 1, 1, 0, 0, 0>, static_offsets = array<i64: 16, 16>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 1, 1>}> : (tensor<16x16xf32>, tensor<32x32xf32>) -> tensor<32x32xf32>
    %231 = "tensor.insert_slice"(%123, %230) <{operandSegmentSizes = array<i32: 1, 1, 0, 0, 0>, static_offsets = array<i64: 16, 0>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 1, 1>}> : (tensor<16x16xf32>, tensor<32x32xf32>) -> tensor<32x32xf32>
    %232 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<32x32xf32>
    %233 = "bufferization.to_tensor"(%232) <{restrict, writable}> : (memref<32x32xf32>) -> tensor<32x32xf32>
    %234 = "hivm.hir.store"(%231, %233) {"inserted-store"} : (tensor<32x32xf32>, tensor<32x32xf32>) -> tensor<32x32xf32>
    %235 = "tensor.empty"() : () -> tensor<32x32xf32>
    %236 = "hivm.hir.load"(%234, %235) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<32x32xf32>, tensor<32x32xf32>) -> tensor<32x32xf32>
    %237 = "tensor.empty"() : () -> tensor<32x32xf32>
    %238 = "hivm.hir.mmadL1"(%228, %236, %1, %3, %3, %3, %237) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_already_inserted = true} : (tensor<32x32xf32>, tensor<32x32xf32>, i1, index, index, index, tensor<32x32xf32>) -> tensor<32x32xf32>
    %239 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<32x32xf32>
    %240 = "bufferization.to_tensor"(%239) <{restrict, writable}> : (memref<32x32xf32>) -> tensor<32x32xf32>
    %241 = "hivm.hir.fixpipe"(%238, %240) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<32x32xf32>, tensor<32x32xf32>) -> tensor<32x32xf32>
    %242 = "tensor.empty"() : () -> tensor<32x32xf32>
    %243 = "hivm.hir.load"(%241, %242) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<32x32xf32>, tensor<32x32xf32>) -> tensor<32x32xf32>
    %244 = "hivm.hir.vmul"(%243, %2, %34) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32x32xf32>, f32, tensor<32x32xf32>) -> tensor<32x32xf32>
    %245 = "hivm.hir.vadd"(%244, %13, %33) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32x32xf32>, f32, tensor<32x32xf32>) -> tensor<32x32xf32>
    %246 = "arith.muli"(%95, %4) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %247 = "arith.addi"(%49, %246) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %248 = "memref.reinterpret_cast"(%arg5, %247) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 32, 32>, static_strides = array<i64: 256, 1>}> : (memref<?xbf16>, index) -> memref<32x32xbf16, strided<[256, 1], offset: ?>>
    %249 = "hivm.hir.vcast"(%231, %40) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<32x32xf32>, tensor<32x32xbf16>) -> tensor<32x32xbf16>
    %250 = "arith.addi"(%95, %3) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %251 = "arith.minsi"(%250, %101) : (index, index) -> index
    %252 = "arith.subi"(%251, %95) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %253 = "arith.minsi"(%252, %3) : (index, index) -> index
    %254 = "tensor.extract_slice"(%249, %253) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 32>, static_strides = array<i64: 1, 1>}> : (tensor<32x32xbf16>, index) -> tensor<?x32xbf16>
    %255 = "memref.subview"(%248, %253) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 32>, static_strides = array<i64: 1, 1>}> : (memref<32x32xbf16, strided<[256, 1], offset: ?>>, index) -> memref<?x32xbf16, strided<[256, 1], offset: ?>>
    "hivm.hir.store"(%254, %255) : (tensor<?x32xbf16>, memref<?x32xbf16, strided<[256, 1], offset: ?>>) -> ()
    %256 = "arith.addi"(%205, %3) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %257 = "memref.reinterpret_cast"(%arg5, %256) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 32, 32>, static_strides = array<i64: 256, 1>}> : (memref<?xbf16>, index) -> memref<32x32xbf16, strided<[256, 1], offset: ?>>
    %258 = "hivm.hir.vcast"(%198, %39) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<32x32xf32>, tensor<32x32xbf16>) -> tensor<32x32xbf16>
    %259 = "tensor.extract_slice"(%258, %211) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 32>, static_strides = array<i64: 1, 1>}> : (tensor<32x32xbf16>, index) -> tensor<?x32xbf16>
    %260 = "memref.subview"(%257, %211) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 32>, static_strides = array<i64: 1, 1>}> : (memref<32x32xbf16, strided<[256, 1], offset: ?>>, index) -> memref<?x32xbf16, strided<[256, 1], offset: ?>>
    "hivm.hir.store"(%259, %260) : (tensor<?x32xbf16>, memref<?x32xbf16, strided<[256, 1], offset: ?>>) -> ()
    %261 = "memref.reinterpret_cast"(%arg5, %205) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 32, 32>, static_strides = array<i64: 256, 1>}> : (memref<?xbf16>, index) -> memref<32x32xbf16, strided<[256, 1], offset: ?>>
    %262 = "hivm.hir.vcast"(%245, %38) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<32x32xf32>, tensor<32x32xbf16>) -> tensor<32x32xbf16>
    %263 = "tensor.extract_slice"(%262, %211) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 32>, static_strides = array<i64: 1, 1>}> : (tensor<32x32xbf16>, index) -> tensor<?x32xbf16>
    %264 = "memref.subview"(%261, %211) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 32>, static_strides = array<i64: 1, 1>}> : (memref<32x32xbf16, strided<[256, 1], offset: ?>>, index) -> memref<?x32xbf16, strided<[256, 1], offset: ?>>
    "hivm.hir.store"(%263, %264) : (tensor<?x32xbf16>, memref<?x32xbf16, strided<[256, 1], offset: ?>>) -> ()
    %265 = "arith.addi"(%247, %3) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %266 = "memref.reinterpret_cast"(%arg5, %265) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 32, 32>, static_strides = array<i64: 256, 1>}> : (memref<?xbf16>, index) -> memref<32x32xbf16, strided<[256, 1], offset: ?>>
    %267 = "tensor.extract_slice"(%42, %253) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 32>, static_strides = array<i64: 1, 1>}> : (tensor<32x32xbf16>, index) -> tensor<?x32xbf16>
    %268 = "memref.subview"(%266, %253) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 32>, static_strides = array<i64: 1, 1>}> : (memref<32x32xbf16, strided<[256, 1], offset: ?>>, index) -> memref<?x32xbf16, strided<[256, 1], offset: ?>>
    "hivm.hir.store"(%267, %268) : (tensor<?x32xbf16>, memref<?x32xbf16, strided<[256, 1], offset: ?>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, false, false, false, false]> : vector<10xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<MIX>, mix_mode = "mix", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<MIX>} : () -> ()

