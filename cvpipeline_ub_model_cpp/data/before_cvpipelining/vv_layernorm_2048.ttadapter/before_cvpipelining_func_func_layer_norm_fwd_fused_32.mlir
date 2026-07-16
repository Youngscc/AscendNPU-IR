"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xf32>, memref<?xf32>, memref<?xf32>, memref<?xf32>, memref<?xf32>, memref<?xf32>, i32, i32, f32, i32, i32, i32) -> (), sym_name = "_layer_norm_fwd_fused"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xf32>, %arg4: memref<?xf32>, %arg5: memref<?xf32>, %arg6: memref<?xf32>, %arg7: memref<?xf32>, %arg8: memref<?xf32>, %arg9: i32, %arg10: i32, %arg11: f32, %arg12: i32, %arg13: i32, %arg14: i32):
    %0 = "arith.constant"() <{value = 2048 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %2 = "arith.constant"() <{value = 1.000000e+00 : f32}> : () -> f32
    %3 = "arith.constant"() <{value = 0 : index}> : () -> index
    %4 = "arith.constant"() <{value = 2048 : index}> : () -> index
    %5 = "arith.constant"() <{value = 0.000000e+00 : f32}> : () -> f32
    "hivm.hir.set_mask_norm"() : () -> ()
    %6 = "arith.muli"(%arg12, %arg13) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %7 = "arith.muli"(%6, %arg14) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%7) {logical_block_num} : (i32) -> ()
    %8 = "hivm.hir.get_block_idx"() : () -> i64
    %9 = "arith.trunci"(%8) : (i64) -> i32
    %10 = "arith.muli"(%arg14, %arg13) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %11 = "arith.divsi"(%9, %10) : (i32, i32) -> i32
    %12 = "arith.remsi"(%11, %arg12) : (i32, i32) -> i32
    %13 = "tensor.empty"() : () -> tensor<2048xf32>
    %14 = "tensor.empty"() : () -> tensor<2048xf32>
    %15 = "tensor.empty"() : () -> tensor<2048xf32>
    %16 = "tensor.empty"() : () -> tensor<2048xf32>
    %17 = "tensor.empty"() : () -> tensor<2048xf32>
    %18 = "tensor.empty"() : () -> tensor<2048xf32>
    %19 = "tensor.empty"() : () -> tensor<2048xf32>
    %20 = "tensor.empty"() : () -> tensor<2048xf32>
    %21 = "tensor.empty"() : () -> tensor<2048xf32>
    %22 = "hivm.hir.vbrc"(%5, %21) <{broadcast_dims = array<i64>}> : (f32, tensor<2048xf32>) -> tensor<2048xf32>
    %23 = "arith.muli"(%12, %arg9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %24 = "scf.for"(%1, %arg10, %0, %22) ({
    ^bb0(%arg18: i32, %arg19: tensor<2048xf32>):
      %107 = "arith.index_cast"(%23) : (i32) -> index
      %108 = "arith.index_cast"(%arg18) : (i32) -> index
      %109 = "arith.addi"(%107, %108) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %110 = "memref.reinterpret_cast"(%arg3, %109) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 2048>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<2048xf32, strided<[1], offset: ?>>
      %111 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<2048xf32>
      %112 = "arith.addi"(%108, %4) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %113 = "arith.index_cast"(%arg10) : (i32) -> index
      %114 = "arith.maxsi"(%108, %113) : (index, index) -> index
      %115 = "arith.minsi"(%112, %114) : (index, index) -> index
      %116 = "arith.subi"(%115, %108) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %117 = "arith.cmpi"(%116, %4) <{predicate = 2 : i64}> : (index, index) -> i1
      %118 = "memref.subview"(%110, %116) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<2048xf32, strided<[1], offset: ?>>, index) -> memref<?xf32, strided<[1], offset: ?>>
      %119 = "memref.subview"(%111, %116) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<2048xf32>, index) -> memref<?xf32, strided<[1]>>
      "hivm.hir.load"(%118, %119, %5, %3, %117) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xf32, strided<[1], offset: ?>>, memref<?xf32, strided<[1]>>, f32, index, i1) -> ()
      %120 = "bufferization.to_tensor"(%111) <{restrict, writable}> : (memref<2048xf32>) -> tensor<2048xf32>
      %121 = "hivm.hir.vadd"(%arg19, %120, %20) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2048xf32>, tensor<2048xf32>, tensor<2048xf32>) -> tensor<2048xf32>
      "scf.yield"(%121) : (tensor<2048xf32>) -> ()
    }) : (i32, i32, i32, tensor<2048xf32>) -> tensor<2048xf32>
    %25 = "bufferization.alloc_tensor"() <{operandSegmentSizes = array<i32: 0, 0, 0>}> : () -> tensor<f32>
    %26 = "tensor.empty"() : () -> tensor<1xf32>
    %27 = "hivm.hir.vreduce"(%24, %26) <{arith = #hivm.reduce_op<sum>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 0>}> : (tensor<2048xf32>, tensor<1xf32>) -> tensor<1xf32>
    %28 = "tensor.extract"(%27, %3) : (tensor<1xf32>, index) -> f32
    %29 = "arith.sitofp"(%arg10) : (i32) -> f32
    %30 = "arith.divf"(%28, %29) <{fastmath = #arith.fastmath<none>}> : (f32, f32) -> f32
    %31 = "scf.for"(%1, %arg10, %0, %22) ({
    ^bb0(%arg16: i32, %arg17: tensor<2048xf32>):
      %88 = "arith.index_cast"(%23) : (i32) -> index
      %89 = "arith.index_cast"(%arg16) : (i32) -> index
      %90 = "arith.addi"(%88, %89) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %91 = "memref.reinterpret_cast"(%arg3, %90) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 2048>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<2048xf32, strided<[1], offset: ?>>
      %92 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<2048xf32>
      %93 = "arith.addi"(%89, %4) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %94 = "arith.index_cast"(%arg10) : (i32) -> index
      %95 = "arith.maxsi"(%89, %94) : (index, index) -> index
      %96 = "arith.minsi"(%93, %95) : (index, index) -> index
      %97 = "arith.subi"(%96, %89) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %98 = "arith.cmpi"(%97, %4) <{predicate = 2 : i64}> : (index, index) -> i1
      %99 = "memref.subview"(%91, %97) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<2048xf32, strided<[1], offset: ?>>, index) -> memref<?xf32, strided<[1], offset: ?>>
      %100 = "memref.subview"(%92, %97) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<2048xf32>, index) -> memref<?xf32, strided<[1]>>
      "hivm.hir.load"(%99, %100, %5, %3, %98) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xf32, strided<[1], offset: ?>>, memref<?xf32, strided<[1]>>, f32, index, i1) -> ()
      %101 = "bufferization.to_tensor"(%92) <{restrict, writable}> : (memref<2048xf32>) -> tensor<2048xf32>
      %102 = "hivm.hir.vsub"(%101, %30, %19) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2048xf32>, f32, tensor<2048xf32>) -> tensor<2048xf32>
      %103 = "tensor.extract_slice"(%102, %97) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<2048xf32>, index) -> tensor<?xf32>
      %104 = "tensor.insert_slice"(%103, %22, %97) <{operandSegmentSizes = array<i32: 1, 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<?xf32>, tensor<2048xf32>, index) -> tensor<2048xf32>
      %105 = "hivm.hir.vmul"(%104, %104, %18) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2048xf32>, tensor<2048xf32>, tensor<2048xf32>) -> tensor<2048xf32>
      %106 = "hivm.hir.vadd"(%arg17, %105, %17) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2048xf32>, tensor<2048xf32>, tensor<2048xf32>) -> tensor<2048xf32>
      "scf.yield"(%106) : (tensor<2048xf32>) -> ()
    }) : (i32, i32, i32, tensor<2048xf32>) -> tensor<2048xf32>
    %32 = "bufferization.alloc_tensor"() <{operandSegmentSizes = array<i32: 0, 0, 0>}> : () -> tensor<f32>
    %33 = "tensor.empty"() : () -> tensor<1xf32>
    %34 = "hivm.hir.vreduce"(%31, %33) <{arith = #hivm.reduce_op<sum>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 0>}> : (tensor<2048xf32>, tensor<1xf32>) -> tensor<1xf32>
    %35 = "tensor.extract"(%34, %3) : (tensor<1xf32>, index) -> f32
    %36 = "arith.divf"(%35, %29) <{fastmath = #arith.fastmath<none>}> : (f32, f32) -> f32
    %37 = "tensor.empty"() : () -> tensor<1xf32>
    %38 = "tensor.empty"() : () -> tensor<1xf32>
    %39 = "tensor.empty"() : () -> tensor<1xf32>
    %40 = "tensor.empty"() : () -> tensor<1xf32>
    %41 = "tensor.empty"() : () -> tensor<1xf32>
    %42 = "tensor.empty"() : () -> tensor<1xf32>
    %43 = "tensor.empty"() : () -> tensor<1xf32>
    %44 = "tensor.insert"(%36, %43, %3) : (f32, tensor<1xf32>, index) -> tensor<1xf32>
    %45 = "tensor.insert"(%arg11, %42, %3) : (f32, tensor<1xf32>, index) -> tensor<1xf32>
    %46 = "hivm.hir.vadd"(%44, %45, %38) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1xf32>, tensor<1xf32>, tensor<1xf32>) -> tensor<1xf32>
    %47 = "tensor.extract"(%46, %3) : (tensor<1xf32>, index) -> f32
    %48 = "tensor.insert"(%47, %41, %3) : (f32, tensor<1xf32>, index) -> tensor<1xf32>
    %49 = "hivm.hir.vsqrt"(%48, %37) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<1xf32>, tensor<1xf32>) -> tensor<1xf32>
    %50 = "tensor.extract"(%49, %3) : (tensor<1xf32>, index) -> f32
    %51 = "arith.divf"(%2, %50) <{fastmath = #arith.fastmath<none>}> : (f32, f32) -> f32
    %52 = "tensor.insert"(%30, %40, %3) : (f32, tensor<1xf32>, index) -> tensor<1xf32>
    %53 = "arith.index_cast"(%12) : (i32) -> index
    %54 = "memref.reinterpret_cast"(%arg7, %53) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<1xf32, strided<[1], offset: ?>>
    "hivm.hir.store"(%52, %54) : (tensor<1xf32>, memref<1xf32, strided<[1], offset: ?>>) -> ()
    %55 = "tensor.insert"(%51, %39, %3) : (f32, tensor<1xf32>, index) -> tensor<1xf32>
    %56 = "memref.reinterpret_cast"(%arg8, %53) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<1xf32, strided<[1], offset: ?>>
    "hivm.hir.store"(%55, %56) : (tensor<1xf32>, memref<1xf32, strided<[1], offset: ?>>) -> ()
    "scf.for"(%1, %arg10, %0) ({
    ^bb0(%arg15: i32):
      %57 = "arith.index_cast"(%arg15) : (i32) -> index
      %58 = "memref.reinterpret_cast"(%arg5, %57) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 2048>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<2048xf32, strided<[1], offset: ?>>
      %59 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<2048xf32>
      %60 = "arith.addi"(%57, %4) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %61 = "arith.index_cast"(%arg10) : (i32) -> index
      %62 = "arith.maxsi"(%57, %61) : (index, index) -> index
      %63 = "arith.minsi"(%60, %62) : (index, index) -> index
      %64 = "arith.subi"(%63, %57) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %65 = "arith.cmpi"(%64, %4) <{predicate = 2 : i64}> : (index, index) -> i1
      %66 = "memref.subview"(%58, %64) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<2048xf32, strided<[1], offset: ?>>, index) -> memref<?xf32, strided<[1], offset: ?>>
      %67 = "memref.subview"(%59, %64) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<2048xf32>, index) -> memref<?xf32, strided<[1]>>
      "hivm.hir.load"(%66, %67, %5, %3, %65) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xf32, strided<[1], offset: ?>>, memref<?xf32, strided<[1]>>, f32, index, i1) -> ()
      %68 = "bufferization.to_tensor"(%59) <{restrict, writable}> : (memref<2048xf32>) -> tensor<2048xf32>
      %69 = "memref.reinterpret_cast"(%arg6, %57) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 2048>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<2048xf32, strided<[1], offset: ?>>
      %70 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<2048xf32>
      %71 = "memref.subview"(%69, %64) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<2048xf32, strided<[1], offset: ?>>, index) -> memref<?xf32, strided<[1], offset: ?>>
      %72 = "memref.subview"(%70, %64) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<2048xf32>, index) -> memref<?xf32, strided<[1]>>
      "hivm.hir.load"(%71, %72, %5, %3, %65) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xf32, strided<[1], offset: ?>>, memref<?xf32, strided<[1]>>, f32, index, i1) -> ()
      %73 = "bufferization.to_tensor"(%70) <{restrict, writable}> : (memref<2048xf32>) -> tensor<2048xf32>
      %74 = "arith.index_cast"(%23) : (i32) -> index
      %75 = "arith.addi"(%74, %57) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %76 = "memref.reinterpret_cast"(%arg3, %75) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 2048>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<2048xf32, strided<[1], offset: ?>>
      %77 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<2048xf32>
      %78 = "memref.subview"(%76, %64) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<2048xf32, strided<[1], offset: ?>>, index) -> memref<?xf32, strided<[1], offset: ?>>
      %79 = "memref.subview"(%77, %64) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<2048xf32>, index) -> memref<?xf32, strided<[1]>>
      "hivm.hir.load"(%78, %79, %5, %3, %65) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xf32, strided<[1], offset: ?>>, memref<?xf32, strided<[1]>>, f32, index, i1) -> ()
      %80 = "bufferization.to_tensor"(%77) <{restrict, writable}> : (memref<2048xf32>) -> tensor<2048xf32>
      %81 = "hivm.hir.vsub"(%80, %30, %16) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2048xf32>, f32, tensor<2048xf32>) -> tensor<2048xf32>
      %82 = "hivm.hir.vmul"(%81, %51, %15) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2048xf32>, f32, tensor<2048xf32>) -> tensor<2048xf32>
      %83 = "hivm.hir.vmul"(%82, %68, %14) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2048xf32>, tensor<2048xf32>, tensor<2048xf32>) -> tensor<2048xf32>
      %84 = "hivm.hir.vadd"(%83, %73, %13) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2048xf32>, tensor<2048xf32>, tensor<2048xf32>) -> tensor<2048xf32>
      %85 = "memref.reinterpret_cast"(%arg4, %75) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 2048>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<2048xf32, strided<[1], offset: ?>>
      %86 = "tensor.extract_slice"(%84, %64) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<2048xf32>, index) -> tensor<?xf32>
      %87 = "memref.subview"(%85, %64) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<2048xf32, strided<[1], offset: ?>>, index) -> memref<?xf32, strided<[1], offset: ?>>
      "hivm.hir.store"(%86, %87) : (tensor<?xf32>, memref<?xf32, strided<[1], offset: ?>>) -> ()
      "scf.yield"() : () -> ()
    }) : (i32, i32, i32) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, true, true, true, false, false, false, false, false, false]> : vector<15xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

