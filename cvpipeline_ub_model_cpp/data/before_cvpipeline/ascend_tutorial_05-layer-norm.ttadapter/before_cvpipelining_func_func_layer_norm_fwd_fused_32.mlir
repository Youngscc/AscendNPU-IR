"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xf16>, memref<?xf16>, memref<?xf16>, memref<?xf16>, memref<?xf32>, memref<?xf32>, i32, i32, f32, i32, i32, i32) -> (), sym_name = "_layer_norm_fwd_fused"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xf16>, %arg4: memref<?xf16>, %arg5: memref<?xf16>, %arg6: memref<?xf16>, %arg7: memref<?xf32>, %arg8: memref<?xf32>, %arg9: i32, %arg10: i32, %arg11: f32, %arg12: i32, %arg13: i32, %arg14: i32):
    %0 = "arith.constant"() <{value = 0.000000e+00 : f16}> : () -> f16
    %1 = "arith.constant"() <{value = 128 : i32}> : () -> i32
    %2 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %3 = "arith.constant"() <{value = 1.000000e+00 : f32}> : () -> f32
    %4 = "arith.constant"() <{value = 0 : index}> : () -> index
    %5 = "arith.constant"() <{value = 128 : index}> : () -> index
    %6 = "arith.constant"() <{value = 0.000000e+00 : f32}> : () -> f32
    "hivm.hir.set_mask_norm"() : () -> ()
    %7 = "arith.muli"(%arg12, %arg13) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %8 = "arith.muli"(%7, %arg14) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%8) {logical_block_num} : (i32) -> ()
    %9 = "hivm.hir.get_block_idx"() : () -> i64
    %10 = "arith.trunci"(%9) : (i64) -> i32
    %11 = "arith.muli"(%arg14, %arg13) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %12 = "arith.divsi"(%10, %11) : (i32, i32) -> i32
    %13 = "arith.remsi"(%12, %arg12) : (i32, i32) -> i32
    %14 = "tensor.empty"() : () -> tensor<128xf32>
    %15 = "tensor.empty"() : () -> tensor<128xf32>
    %16 = "tensor.empty"() : () -> tensor<128xf32>
    %17 = "tensor.empty"() : () -> tensor<128xf32>
    %18 = "tensor.empty"() : () -> tensor<128xf32>
    %19 = "tensor.empty"() : () -> tensor<128xf32>
    %20 = "tensor.empty"() : () -> tensor<128xf32>
    %21 = "tensor.empty"() : () -> tensor<128xf32>
    %22 = "tensor.empty"() : () -> tensor<128xf32>
    %23 = "tensor.empty"() : () -> tensor<128xf32>
    %24 = "tensor.empty"() : () -> tensor<128xf32>
    %25 = "tensor.empty"() : () -> tensor<128xf32>
    %26 = "tensor.empty"() : () -> tensor<128xf32>
    %27 = "tensor.empty"() : () -> tensor<128xf32>
    %28 = "hivm.hir.vbrc"(%6, %27) <{broadcast_dims = array<i64>}> : (f32, tensor<128xf32>) -> tensor<128xf32>
    %29 = "arith.muli"(%13, %arg9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %30 = "arith.index_cast"(%29) : (i32) -> index
    %31 = "scf.for"(%2, %arg10, %1, %28) ({
    ^bb0(%arg18: i32, %arg19: tensor<128xf32>):
      %118 = "arith.index_cast"(%arg18) : (i32) -> index
      %119 = "arith.addi"(%30, %118) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %120 = "memref.reinterpret_cast"(%arg3, %119) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 128>, static_strides = array<i64: 1>}> : (memref<?xf16>, index) -> memref<128xf16, strided<[1], offset: ?>>
      %121 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<128xf16>
      %122 = "arith.addi"(%118, %5) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %123 = "arith.index_cast"(%arg10) : (i32) -> index
      %124 = "arith.maxsi"(%118, %123) : (index, index) -> index
      %125 = "arith.minsi"(%122, %124) : (index, index) -> index
      %126 = "arith.subi"(%125, %118) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %127 = "arith.cmpi"(%126, %5) <{predicate = 2 : i64}> : (index, index) -> i1
      %128 = "memref.subview"(%120, %126) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<128xf16, strided<[1], offset: ?>>, index) -> memref<?xf16, strided<[1], offset: ?>>
      %129 = "memref.subview"(%121, %126) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<128xf16>, index) -> memref<?xf16, strided<[1]>>
      "hivm.hir.load"(%128, %129, %0, %4, %127) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xf16, strided<[1], offset: ?>>, memref<?xf16, strided<[1]>>, f16, index, i1) -> ()
      %130 = "bufferization.to_tensor"(%121) <{restrict, writable}> : (memref<128xf16>) -> tensor<128xf16>
      %131 = "hivm.hir.vcast"(%130, %26) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<128xf16>, tensor<128xf32>) -> tensor<128xf32>
      %132 = "hivm.hir.vadd"(%arg19, %131, %25) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, tensor<128xf32>, tensor<128xf32>) -> tensor<128xf32>
      "scf.yield"(%132) : (tensor<128xf32>) -> ()
    }) : (i32, i32, i32, tensor<128xf32>) -> tensor<128xf32>
    %32 = "bufferization.alloc_tensor"() <{operandSegmentSizes = array<i32: 0, 0, 0>}> : () -> tensor<f32>
    %33 = "tensor.empty"() : () -> tensor<1xf32>
    %34 = "hivm.hir.vreduce"(%31, %33) <{arith = #hivm.reduce_op<sum>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 0>}> : (tensor<128xf32>, tensor<1xf32>) -> tensor<1xf32>
    %35 = "tensor.extract"(%34, %4) : (tensor<1xf32>, index) -> f32
    %36 = "arith.sitofp"(%arg10) : (i32) -> f32
    %37 = "arith.divf"(%35, %36) <{fastmath = #arith.fastmath<none>}> : (f32, f32) -> f32
    %38 = "scf.for"(%2, %arg10, %1, %28) ({
    ^bb0(%arg16: i32, %arg17: tensor<128xf32>):
      %99 = "arith.index_cast"(%arg16) : (i32) -> index
      %100 = "arith.addi"(%30, %99) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %101 = "memref.reinterpret_cast"(%arg3, %100) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 128>, static_strides = array<i64: 1>}> : (memref<?xf16>, index) -> memref<128xf16, strided<[1], offset: ?>>
      %102 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<128xf16>
      %103 = "arith.addi"(%99, %5) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %104 = "arith.index_cast"(%arg10) : (i32) -> index
      %105 = "arith.maxsi"(%99, %104) : (index, index) -> index
      %106 = "arith.minsi"(%103, %105) : (index, index) -> index
      %107 = "arith.subi"(%106, %99) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %108 = "arith.cmpi"(%107, %5) <{predicate = 2 : i64}> : (index, index) -> i1
      %109 = "memref.subview"(%101, %107) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<128xf16, strided<[1], offset: ?>>, index) -> memref<?xf16, strided<[1], offset: ?>>
      %110 = "memref.subview"(%102, %107) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<128xf16>, index) -> memref<?xf16, strided<[1]>>
      "hivm.hir.load"(%109, %110, %0, %4, %108) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xf16, strided<[1], offset: ?>>, memref<?xf16, strided<[1]>>, f16, index, i1) -> ()
      %111 = "bufferization.to_tensor"(%102) <{restrict, writable}> : (memref<128xf16>) -> tensor<128xf16>
      %112 = "hivm.hir.vcast"(%111, %24) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<128xf16>, tensor<128xf32>) -> tensor<128xf32>
      %113 = "hivm.hir.vsub"(%112, %37, %23) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, f32, tensor<128xf32>) -> tensor<128xf32>
      %114 = "tensor.extract_slice"(%113, %107) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<128xf32>, index) -> tensor<?xf32>
      %115 = "tensor.insert_slice"(%114, %28, %107) <{operandSegmentSizes = array<i32: 1, 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<?xf32>, tensor<128xf32>, index) -> tensor<128xf32>
      %116 = "hivm.hir.vmul"(%115, %115, %22) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, tensor<128xf32>, tensor<128xf32>) -> tensor<128xf32>
      %117 = "hivm.hir.vadd"(%arg17, %116, %21) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, tensor<128xf32>, tensor<128xf32>) -> tensor<128xf32>
      "scf.yield"(%117) : (tensor<128xf32>) -> ()
    }) : (i32, i32, i32, tensor<128xf32>) -> tensor<128xf32>
    %39 = "bufferization.alloc_tensor"() <{operandSegmentSizes = array<i32: 0, 0, 0>}> : () -> tensor<f32>
    %40 = "tensor.empty"() : () -> tensor<1xf32>
    %41 = "hivm.hir.vreduce"(%38, %40) <{arith = #hivm.reduce_op<sum>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 0>}> : (tensor<128xf32>, tensor<1xf32>) -> tensor<1xf32>
    %42 = "tensor.extract"(%41, %4) : (tensor<1xf32>, index) -> f32
    %43 = "arith.divf"(%42, %36) <{fastmath = #arith.fastmath<none>}> : (f32, f32) -> f32
    %44 = "tensor.empty"() : () -> tensor<1xf32>
    %45 = "tensor.empty"() : () -> tensor<1xf32>
    %46 = "tensor.empty"() : () -> tensor<1xf32>
    %47 = "tensor.empty"() : () -> tensor<1xf32>
    %48 = "tensor.empty"() : () -> tensor<1xf32>
    %49 = "tensor.empty"() : () -> tensor<1xf32>
    %50 = "tensor.empty"() : () -> tensor<1xf32>
    %51 = "tensor.insert"(%43, %50, %4) : (f32, tensor<1xf32>, index) -> tensor<1xf32>
    %52 = "tensor.insert"(%arg11, %49, %4) : (f32, tensor<1xf32>, index) -> tensor<1xf32>
    %53 = "hivm.hir.vadd"(%51, %52, %45) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1xf32>, tensor<1xf32>, tensor<1xf32>) -> tensor<1xf32>
    %54 = "tensor.extract"(%53, %4) : (tensor<1xf32>, index) -> f32
    %55 = "tensor.insert"(%54, %48, %4) : (f32, tensor<1xf32>, index) -> tensor<1xf32>
    %56 = "hivm.hir.vsqrt"(%55, %44) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<1xf32>, tensor<1xf32>) -> tensor<1xf32>
    %57 = "tensor.extract"(%56, %4) : (tensor<1xf32>, index) -> f32
    %58 = "arith.divf"(%3, %57) <{fastmath = #arith.fastmath<none>}> : (f32, f32) -> f32
    %59 = "arith.index_cast"(%13) : (i32) -> index
    %60 = "tensor.insert"(%37, %47, %4) : (f32, tensor<1xf32>, index) -> tensor<1xf32>
    %61 = "memref.reinterpret_cast"(%arg7, %59) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<1xf32, strided<[1], offset: ?>>
    "hivm.hir.store"(%60, %61) : (tensor<1xf32>, memref<1xf32, strided<[1], offset: ?>>) -> ()
    %62 = "tensor.insert"(%58, %46, %4) : (f32, tensor<1xf32>, index) -> tensor<1xf32>
    %63 = "memref.reinterpret_cast"(%arg8, %59) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<1xf32, strided<[1], offset: ?>>
    "hivm.hir.store"(%62, %63) : (tensor<1xf32>, memref<1xf32, strided<[1], offset: ?>>) -> ()
    "scf.for"(%2, %arg10, %1) ({
    ^bb0(%arg15: i32):
      %64 = "arith.index_cast"(%arg15) : (i32) -> index
      %65 = "memref.reinterpret_cast"(%arg5, %64) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 128>, static_strides = array<i64: 1>}> : (memref<?xf16>, index) -> memref<128xf16, strided<[1], offset: ?>>
      %66 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<128xf16>
      %67 = "arith.addi"(%64, %5) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %68 = "arith.index_cast"(%arg10) : (i32) -> index
      %69 = "arith.maxsi"(%64, %68) : (index, index) -> index
      %70 = "arith.minsi"(%67, %69) : (index, index) -> index
      %71 = "arith.subi"(%70, %64) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %72 = "arith.cmpi"(%71, %5) <{predicate = 2 : i64}> : (index, index) -> i1
      %73 = "memref.subview"(%65, %71) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<128xf16, strided<[1], offset: ?>>, index) -> memref<?xf16, strided<[1], offset: ?>>
      %74 = "memref.subview"(%66, %71) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<128xf16>, index) -> memref<?xf16, strided<[1]>>
      "hivm.hir.load"(%73, %74, %0, %4, %72) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xf16, strided<[1], offset: ?>>, memref<?xf16, strided<[1]>>, f16, index, i1) -> ()
      %75 = "bufferization.to_tensor"(%66) <{restrict, writable}> : (memref<128xf16>) -> tensor<128xf16>
      %76 = "memref.reinterpret_cast"(%arg6, %64) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 128>, static_strides = array<i64: 1>}> : (memref<?xf16>, index) -> memref<128xf16, strided<[1], offset: ?>>
      %77 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<128xf16>
      %78 = "memref.subview"(%76, %71) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<128xf16, strided<[1], offset: ?>>, index) -> memref<?xf16, strided<[1], offset: ?>>
      %79 = "memref.subview"(%77, %71) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<128xf16>, index) -> memref<?xf16, strided<[1]>>
      "hivm.hir.load"(%78, %79, %0, %4, %72) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xf16, strided<[1], offset: ?>>, memref<?xf16, strided<[1]>>, f16, index, i1) -> ()
      %80 = "bufferization.to_tensor"(%77) <{restrict, writable}> : (memref<128xf16>) -> tensor<128xf16>
      %81 = "arith.addi"(%30, %64) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %82 = "memref.reinterpret_cast"(%arg3, %81) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 128>, static_strides = array<i64: 1>}> : (memref<?xf16>, index) -> memref<128xf16, strided<[1], offset: ?>>
      %83 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<128xf16>
      %84 = "memref.subview"(%82, %71) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<128xf16, strided<[1], offset: ?>>, index) -> memref<?xf16, strided<[1], offset: ?>>
      %85 = "memref.subview"(%83, %71) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<128xf16>, index) -> memref<?xf16, strided<[1]>>
      "hivm.hir.load"(%84, %85, %0, %4, %72) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xf16, strided<[1], offset: ?>>, memref<?xf16, strided<[1]>>, f16, index, i1) -> ()
      %86 = "bufferization.to_tensor"(%83) <{restrict, writable}> : (memref<128xf16>) -> tensor<128xf16>
      %87 = "hivm.hir.vcast"(%86, %20) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<128xf16>, tensor<128xf32>) -> tensor<128xf32>
      %88 = "hivm.hir.vsub"(%87, %37, %19) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, f32, tensor<128xf32>) -> tensor<128xf32>
      %89 = "hivm.hir.vmul"(%88, %58, %18) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, f32, tensor<128xf32>) -> tensor<128xf32>
      %90 = "hivm.hir.vcast"(%75, %17) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<128xf16>, tensor<128xf32>) -> tensor<128xf32>
      %91 = "hivm.hir.vmul"(%89, %90, %16) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, tensor<128xf32>, tensor<128xf32>) -> tensor<128xf32>
      %92 = "hivm.hir.vcast"(%80, %15) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<128xf16>, tensor<128xf32>) -> tensor<128xf32>
      %93 = "hivm.hir.vadd"(%91, %92, %14) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, tensor<128xf32>, tensor<128xf32>) -> tensor<128xf32>
      %94 = "memref.reinterpret_cast"(%arg4, %81) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 128>, static_strides = array<i64: 1>}> : (memref<?xf16>, index) -> memref<128xf16, strided<[1], offset: ?>>
      %95 = "tensor.empty"() : () -> tensor<128xf16>
      %96 = "hivm.hir.vcast"(%93, %95) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<128xf32>, tensor<128xf16>) -> tensor<128xf16>
      %97 = "tensor.extract_slice"(%96, %71) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<128xf16>, index) -> tensor<?xf16>
      %98 = "memref.subview"(%94, %71) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<128xf16, strided<[1], offset: ?>>, index) -> memref<?xf16, strided<[1], offset: ?>>
      "hivm.hir.store"(%97, %98) : (tensor<?xf16>, memref<?xf16, strided<[1], offset: ?>>) -> ()
      "scf.yield"() : () -> ()
    }) : (i32, i32, i32) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, true, true, true, false, false, false, false, false, false]> : vector<15xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

