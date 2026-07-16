"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, memref<?xf32>, memref<?xf32>, i32, i32, i32, i32, i32, f32, i32, i32, i32) -> (), sym_name = "layer_norm_fwd_kernel"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xbf16>, %arg4: memref<?xbf16>, %arg5: memref<?xbf16>, %arg6: memref<?xbf16>, %arg7: memref<?xbf16>, %arg8: memref<?xf32>, %arg9: memref<?xf32>, %arg10: i32, %arg11: i32, %arg12: i32, %arg13: i32, %arg14: i32, %arg15: f32, %arg16: i32, %arg17: i32, %arg18: i32):
    %0 = "arith.constant"() <{value = 1 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = -1.000000e+00 : f32}> : () -> f32
    %2 = "arith.constant"() <{value = 0.000000e+00 : bf16}> : () -> bf16
    %3 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %4 = "arith.constant"() <{value = 1.000000e+00 : f32}> : () -> f32
    %5 = "arith.constant"() <{value = 128 : index}> : () -> index
    %6 = "arith.constant"() <{value = 0 : index}> : () -> index
    %7 = "arith.constant"() <{value = 0.000000e+00 : f32}> : () -> f32
    "hivm.hir.set_mask_norm"() : () -> ()
    %8 = "arith.muli"(%arg16, %arg17) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %9 = "arith.muli"(%8, %arg18) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%9) {logical_block_num} : (i32) -> ()
    %10 = "hivm.hir.get_block_idx"() : () -> i64
    %11 = "arith.trunci"(%10) : (i64) -> i32
    %12 = "arith.divsi"(%11, %arg18) : (i32, i32) -> i32
    %13 = "arith.remsi"(%12, %arg17) : (i32, i32) -> i32
    %14 = "arith.muli"(%arg18, %arg17) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %15 = "arith.divsi"(%11, %14) : (i32, i32) -> i32
    %16 = "arith.remsi"(%15, %arg16) : (i32, i32) -> i32
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
    %28 = "tensor.empty"() : () -> tensor<128xf32>
    %29 = "tensor.empty"() : () -> tensor<128xf32>
    %30 = "tensor.empty"() : () -> tensor<128xf32>
    %31 = "tensor.empty"() : () -> tensor<128xf32>
    %32 = "tensor.empty"() : () -> tensor<128xf32>
    %33 = "hivm.hir.vbrc"(%7, %32) <{broadcast_dims = array<i64>}> : (f32, tensor<128xf32>) -> tensor<128xf32>
    %34 = "arith.cmpi"(%arg13, %0) <{predicate = 2 : i64}> : (i32, i32) -> i1
    %35 = "arith.select"(%34, %arg13, %0) : (i1, i32, i32) -> i32
    %36 = "arith.divsi"(%arg13, %35) : (i32, i32) -> i32
    %37 = "arith.remsi"(%arg13, %35) : (i32, i32) -> i32
    %38 = "arith.cmpi"(%16, %37) <{predicate = 2 : i64}> : (i32, i32) -> i1
    %39 = "scf.if"(%38) ({
      %143 = "arith.addi"(%36, %0) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      "scf.yield"(%143) : (i32) -> ()
    }, {
      "scf.yield"(%36) : (i32) -> ()
    }) : (i1) -> i32
    %40 = "arith.muli"(%16, %arg10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %41 = "arith.muli"(%13, %arg14) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %42 = "arith.muli"(%16, %arg11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %43 = "arith.muli"(%16, %arg12) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %44 = "arith.muli"(%13, %arg13) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %45 = "arith.index_cast"(%41) : (i32) -> index
    %46 = "arith.sitofp"(%arg14) : (i32) -> f32
    %47 = "memref.reinterpret_cast"(%arg5, %45) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 128>, static_strides = array<i64: 1>}> : (memref<?xbf16>, index) -> memref<128xbf16, strided<[1], offset: ?>>
    %48 = "memref.reinterpret_cast"(%arg6, %45) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 128>, static_strides = array<i64: 1>}> : (memref<?xbf16>, index) -> memref<128xbf16, strided<[1], offset: ?>>
    "scf.for"(%3, %39, %0) ({
    ^bb0(%arg19: i32):
      %49 = "arith.muli"(%arg19, %35) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %50 = "arith.muli"(%49, %arg10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %51 = "arith.index_cast"(%50) : (i32) -> index
      %52 = "arith.index_cast"(%40) : (i32) -> index
      %53 = "arith.addi"(%51, %52) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %54 = "arith.addi"(%53, %45) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %55 = "arith.muli"(%49, %arg11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %56 = "arith.index_cast"(%55) : (i32) -> index
      %57 = "arith.index_cast"(%42) : (i32) -> index
      %58 = "arith.addi"(%56, %57) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %59 = "arith.addi"(%58, %45) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %60 = "arith.muli"(%49, %arg12) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %61 = "arith.index_cast"(%60) : (i32) -> index
      %62 = "arith.index_cast"(%43) : (i32) -> index
      %63 = "arith.addi"(%61, %62) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %64 = "arith.addi"(%63, %45) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %65 = "arith.index_cast"(%49) : (i32) -> index
      %66 = "arith.index_cast"(%44) : (i32) -> index
      %67 = "arith.addi"(%65, %66) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %68 = "memref.reinterpret_cast"(%arg3, %54) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 128>, static_strides = array<i64: 1>}> : (memref<?xbf16>, index) -> memref<128xbf16, strided<[1], offset: ?>>
      %69 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<128xbf16>
      %70 = "arith.index_cast"(%arg14) : (i32) -> index
      %71 = "arith.maxsi"(%70, %6) : (index, index) -> index
      %72 = "arith.minsi"(%71, %5) : (index, index) -> index
      %73 = "arith.cmpi"(%72, %5) <{predicate = 2 : i64}> : (index, index) -> i1
      %74 = "memref.subview"(%68, %72) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<128xbf16, strided<[1], offset: ?>>, index) -> memref<?xbf16, strided<[1], offset: ?>>
      %75 = "memref.subview"(%69, %72) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<128xbf16>, index) -> memref<?xbf16, strided<[1]>>
      "hivm.hir.load"(%74, %75, %2, %6, %73) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xbf16, strided<[1], offset: ?>>, memref<?xbf16, strided<[1]>>, bf16, index, i1) -> ()
      %76 = "bufferization.to_tensor"(%69) <{restrict, writable}> : (memref<128xbf16>) -> tensor<128xbf16>
      %77 = "hivm.hir.vcast"(%76, %31) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<128xbf16>, tensor<128xf32>) -> tensor<128xf32>
      %78 = "bufferization.alloc_tensor"() <{operandSegmentSizes = array<i32: 0, 0, 0>}> : () -> tensor<f32>
      %79 = "tensor.empty"() : () -> tensor<1xf32>
      %80 = "hivm.hir.vreduce"(%77, %79) <{arith = #hivm.reduce_op<sum>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 0>}> : (tensor<128xf32>, tensor<1xf32>) -> tensor<1xf32>
      %81 = "tensor.extract"(%80, %6) : (tensor<1xf32>, index) -> f32
      %82 = "arith.divf"(%81, %46) <{fastmath = #arith.fastmath<none>}> : (f32, f32) -> f32
      %83 = "arith.index_cast"(%16) : (i32) -> index
      %84 = "arith.addi"(%67, %83) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %85 = "tensor.empty"() : () -> tensor<1xf32>
      %86 = "tensor.empty"() : () -> tensor<1xf32>
      %87 = "tensor.empty"() : () -> tensor<1xf32>
      %88 = "tensor.empty"() : () -> tensor<1xf32>
      %89 = "tensor.empty"() : () -> tensor<1xf32>
      %90 = "tensor.empty"() : () -> tensor<1xf32>
      %91 = "tensor.empty"() : () -> tensor<1xf32>
      %92 = "tensor.insert"(%82, %91, %6) : (f32, tensor<1xf32>, index) -> tensor<1xf32>
      %93 = "memref.reinterpret_cast"(%arg8, %84) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<1xf32, strided<[1], offset: ?>>
      "hivm.hir.store"(%92, %93) : (tensor<1xf32>, memref<1xf32, strided<[1], offset: ?>>) -> ()
      %94 = "hivm.hir.vsub"(%77, %82, %30) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, f32, tensor<128xf32>) -> tensor<128xf32>
      %95 = "tensor.extract_slice"(%94, %72) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<128xf32>, index) -> tensor<?xf32>
      %96 = "tensor.insert_slice"(%95, %33, %72) <{operandSegmentSizes = array<i32: 1, 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<?xf32>, tensor<128xf32>, index) -> tensor<128xf32>
      %97 = "hivm.hir.vmul"(%96, %96, %29) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, tensor<128xf32>, tensor<128xf32>) -> tensor<128xf32>
      %98 = "bufferization.alloc_tensor"() <{operandSegmentSizes = array<i32: 0, 0, 0>}> : () -> tensor<f32>
      %99 = "tensor.empty"() : () -> tensor<1xf32>
      %100 = "hivm.hir.vreduce"(%97, %99) <{arith = #hivm.reduce_op<sum>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 0>}> : (tensor<128xf32>, tensor<1xf32>) -> tensor<1xf32>
      %101 = "tensor.extract"(%100, %6) : (tensor<1xf32>, index) -> f32
      %102 = "arith.divf"(%101, %46) <{fastmath = #arith.fastmath<none>}> : (f32, f32) -> f32
      %103 = "tensor.insert"(%102, %90, %6) : (f32, tensor<1xf32>, index) -> tensor<1xf32>
      %104 = "tensor.insert"(%arg15, %89, %6) : (f32, tensor<1xf32>, index) -> tensor<1xf32>
      %105 = "hivm.hir.vadd"(%103, %104, %86) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1xf32>, tensor<1xf32>, tensor<1xf32>) -> tensor<1xf32>
      %106 = "tensor.extract"(%105, %6) : (tensor<1xf32>, index) -> f32
      %107 = "tensor.insert"(%106, %88, %6) : (f32, tensor<1xf32>, index) -> tensor<1xf32>
      %108 = "hivm.hir.vsqrt"(%107, %85) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<1xf32>, tensor<1xf32>) -> tensor<1xf32>
      %109 = "tensor.extract"(%108, %6) : (tensor<1xf32>, index) -> f32
      %110 = "arith.divf"(%4, %109) <{fastmath = #arith.fastmath<none>}> : (f32, f32) -> f32
      %111 = "tensor.insert"(%110, %87, %6) : (f32, tensor<1xf32>, index) -> tensor<1xf32>
      %112 = "memref.reinterpret_cast"(%arg9, %84) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<1xf32, strided<[1], offset: ?>>
      "hivm.hir.store"(%111, %112) : (tensor<1xf32>, memref<1xf32, strided<[1], offset: ?>>) -> ()
      %113 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<128xbf16>
      %114 = "memref.subview"(%47, %72) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<128xbf16, strided<[1], offset: ?>>, index) -> memref<?xbf16, strided<[1], offset: ?>>
      %115 = "memref.subview"(%113, %72) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<128xbf16>, index) -> memref<?xbf16, strided<[1]>>
      "hivm.hir.load"(%114, %115, %2, %6, %73) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xbf16, strided<[1], offset: ?>>, memref<?xbf16, strided<[1]>>, bf16, index, i1) -> ()
      %116 = "bufferization.to_tensor"(%113) <{restrict, writable}> : (memref<128xbf16>) -> tensor<128xbf16>
      %117 = "hivm.hir.vcast"(%116, %28) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<128xbf16>, tensor<128xf32>) -> tensor<128xf32>
      %118 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<128xbf16>
      %119 = "memref.subview"(%48, %72) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<128xbf16, strided<[1], offset: ?>>, index) -> memref<?xbf16, strided<[1], offset: ?>>
      %120 = "memref.subview"(%118, %72) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<128xbf16>, index) -> memref<?xbf16, strided<[1]>>
      "hivm.hir.load"(%119, %120, %2, %6, %73) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xbf16, strided<[1], offset: ?>>, memref<?xbf16, strided<[1]>>, bf16, index, i1) -> ()
      %121 = "bufferization.to_tensor"(%118) <{restrict, writable}> : (memref<128xbf16>) -> tensor<128xbf16>
      %122 = "hivm.hir.vcast"(%121, %27) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<128xbf16>, tensor<128xf32>) -> tensor<128xf32>
      %123 = "hivm.hir.vmul"(%94, %110, %26) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, f32, tensor<128xf32>) -> tensor<128xf32>
      %124 = "hivm.hir.vmul"(%123, %117, %25) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, tensor<128xf32>, tensor<128xf32>) -> tensor<128xf32>
      %125 = "hivm.hir.vadd"(%124, %122, %24) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, tensor<128xf32>, tensor<128xf32>) -> tensor<128xf32>
      %126 = "memref.reinterpret_cast"(%arg7, %64) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 128>, static_strides = array<i64: 1>}> : (memref<?xbf16>, index) -> memref<128xbf16, strided<[1], offset: ?>>
      %127 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<128xbf16>
      %128 = "memref.subview"(%126, %72) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<128xbf16, strided<[1], offset: ?>>, index) -> memref<?xbf16, strided<[1], offset: ?>>
      %129 = "memref.subview"(%127, %72) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<128xbf16>, index) -> memref<?xbf16, strided<[1]>>
      "hivm.hir.load"(%128, %129, %2, %6, %73) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xbf16, strided<[1], offset: ?>>, memref<?xbf16, strided<[1]>>, bf16, index, i1) -> ()
      %130 = "bufferization.to_tensor"(%127) <{restrict, writable}> : (memref<128xbf16>) -> tensor<128xbf16>
      %131 = "hivm.hir.vcast"(%130, %23) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<128xbf16>, tensor<128xf32>) -> tensor<128xf32>
      %132 = "hivm.hir.vmul"(%131, %1, %22) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, f32, tensor<128xf32>) -> tensor<128xf32>
      %133 = "hivm.hir.vadd"(%132, %7, %21) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, f32, tensor<128xf32>) -> tensor<128xf32>
      %134 = "hivm.hir.vexp"(%133, %20) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, tensor<128xf32>) -> tensor<128xf32>
      %135 = "hivm.hir.vadd"(%134, %4, %19) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, f32, tensor<128xf32>) -> tensor<128xf32>
      %136 = "hivm.hir.vdiv"(%131, %135, %18) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, tensor<128xf32>, tensor<128xf32>) -> tensor<128xf32>
      %137 = "hivm.hir.vmul"(%125, %136, %17) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, tensor<128xf32>, tensor<128xf32>) -> tensor<128xf32>
      %138 = "memref.reinterpret_cast"(%arg4, %59) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 128>, static_strides = array<i64: 1>}> : (memref<?xbf16>, index) -> memref<128xbf16, strided<[1], offset: ?>>
      %139 = "tensor.empty"() : () -> tensor<128xbf16>
      %140 = "hivm.hir.vcast"(%137, %139) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<128xf32>, tensor<128xbf16>) -> tensor<128xbf16>
      %141 = "tensor.extract_slice"(%140, %72) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<128xbf16>, index) -> tensor<?xbf16>
      %142 = "memref.subview"(%138, %72) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<128xbf16, strided<[1], offset: ?>>, index) -> memref<?xbf16, strided<[1], offset: ?>>
      "hivm.hir.store"(%141, %142) : (tensor<?xbf16>, memref<?xbf16, strided<[1], offset: ?>>) -> ()
      "scf.yield"() : () -> ()
    }) : (i32, i32, i32) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false]> : vector<19xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

