"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, memref<?xf32>, memref<?xi64>, memref<?xi64>, i32, i32, i32, i32) -> (), sym_name = "recompute_w_u_fwd_kernel"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xbf16>, %arg4: memref<?xbf16>, %arg5: memref<?xbf16>, %arg6: memref<?xbf16>, %arg7: memref<?xbf16>, %arg8: memref<?xbf16>, %arg9: memref<?xf32>, %arg10: memref<?xi64>, %arg11: memref<?xi64>, %arg12: i32, %arg13: i32, %arg14: i32, %arg15: i32):
    %0 = "arith.constant"() <{value = 1 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = true}> : () -> i1
    %2 = "arith.constant"() <{value = 256 : index}> : () -> index
    %3 = "arith.constant"() <{value = 0.000000e+00 : bf16}> : () -> bf16
    %4 = "arith.constant"() <{value = 16 : index}> : () -> index
    %5 = "arith.constant"() <{value = 64 : index}> : () -> index
    %6 = "arith.constant"() <{value = 0 : index}> : () -> index
    %7 = "arith.constant"() <{value = 1 : index}> : () -> index
    %8 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %9 = "arith.constant"() <{value = 4 : i32}> : () -> i32
    %10 = "arith.constant"() <{value = 2 : i32}> : () -> i32
    %11 = "arith.constant"() <{value = 16 : i32}> : () -> i32
    %12 = "arith.constant"() <{value = 64 : i32}> : () -> i32
    %13 = "arith.constant"() <{value = 0.000000e+00 : f32}> : () -> f32
    "hivm.hir.set_mask_norm"() : () -> ()
    %14 = "arith.muli"(%arg13, %arg14) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %15 = "arith.muli"(%14, %arg15) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%15) {logical_block_num} : (i32) -> ()
    %16 = "hivm.hir.get_block_idx"() : () -> i64
    %17 = "arith.trunci"(%16) : (i64) -> i32
    %18 = "arith.muli"(%arg15, %arg14) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %19 = "arith.divsi"(%17, %18) : (i32, i32) -> i32
    %20 = "arith.remsi"(%19, %arg13) : (i32, i32) -> i32
    %21 = "tensor.empty"() : () -> tensor<16x64xf32>
    %22 = "tensor.empty"() : () -> tensor<16x64xf32>
    %23 = "tensor.empty"() : () -> tensor<16x64xf32>
    %24 = "tensor.empty"() : () -> tensor<16x64xf32>
    %25 = "tensor.empty"() : () -> tensor<16x64xf32>
    %26 = "tensor.empty"() : () -> tensor<16x64xf32>
    %27 = "tensor.empty"() : () -> tensor<16x64xf32>
    %28 = "arith.muli"(%20, %10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %29 = "arith.index_cast"(%28) : (i32) -> index
    %30 = "memref.reinterpret_cast"(%arg11, %29) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi64>, index) -> memref<1xi64, strided<[1], offset: ?>>
    %31 = "arith.addi"(%29, %7) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %32 = "memref.reinterpret_cast"(%arg11, %31) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi64>, index) -> memref<1xi64, strided<[1], offset: ?>>
    "scf.for"(%8, %9, %0) ({
    ^bb0(%arg16: i32):
      %33 = "arith.remsi"(%arg16, %9) : (i32, i32) -> i32
      %34 = "memref.load"(%30, %6) : (memref<1xi64, strided<[1], offset: ?>>, index) -> i64
      %35 = "arith.trunci"(%34) : (i64) -> i32
      %36 = "memref.load"(%32, %6) : (memref<1xi64, strided<[1], offset: ?>>, index) -> i64
      %37 = "arith.trunci"(%36) : (i64) -> i32
      %38 = "arith.index_cast"(%35) : (i32) -> index
      %39 = "memref.reinterpret_cast"(%arg10, %38) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi64>, index) -> memref<1xi64, strided<[1], offset: ?>>
      %40 = "memref.load"(%39, %6) : (memref<1xi64, strided<[1], offset: ?>>, index) -> i64
      %41 = "arith.trunci"(%40) : (i64) -> i32
      %42 = "arith.addi"(%38, %7) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %43 = "memref.reinterpret_cast"(%arg10, %42) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi64>, index) -> memref<1xi64, strided<[1], offset: ?>>
      %44 = "memref.load"(%43, %6) : (memref<1xi64, strided<[1], offset: ?>>, index) -> i64
      %45 = "arith.trunci"(%44) : (i64) -> i32
      %46 = "arith.subi"(%45, %41) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %47 = "arith.muli"(%37, %11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %48 = "arith.muli"(%41, %9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %49 = "arith.addi"(%48, %33) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %50 = "arith.muli"(%49, %11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %51 = "arith.index_cast"(%50) : (i32) -> index
      %52 = "arith.index_cast"(%47) : (i32) -> index
      %53 = "arith.muli"(%52, %5) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %54 = "arith.addi"(%51, %53) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %55 = "memref.reinterpret_cast"(%arg8, %54) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 64, 1>}> : (memref<?xbf16>, index) -> memref<16x16xbf16, strided<[64, 1], offset: ?>>
      %56 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x16xbf16>
      %57 = "arith.addi"(%52, %4) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %58 = "arith.index_cast"(%46) : (i32) -> index
      %59 = "arith.maxsi"(%52, %58) : (index, index) -> index
      %60 = "arith.minsi"(%57, %59) : (index, index) -> index
      %61 = "arith.subi"(%60, %52) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %62 = "arith.cmpi"(%61, %4) <{predicate = 2 : i64}> : (index, index) -> i1
      %63 = "memref.subview"(%55, %61) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16, strided<[64, 1], offset: ?>>, index) -> memref<?x16xbf16, strided<[64, 1], offset: ?>>
      %64 = "memref.subview"(%56, %61) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16>, index) -> memref<?x16xbf16, strided<[16, 1]>>
      "hivm.hir.load"(%63, %64, %3, %6, %62) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x16xbf16, strided<[64, 1], offset: ?>>, memref<?x16xbf16, strided<[16, 1]>>, bf16, index, i1) -> ()
      %65 = "bufferization.to_tensor"(%56) <{restrict, writable}> : (memref<16x16xbf16>) -> tensor<16x16xbf16>
      %66 = "tensor.empty"() : () -> tensor<16x16xf32>
      %67 = "hivm.hir.vcast"(%65, %66) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x16xbf16>, tensor<16x16xf32>) -> tensor<16x16xf32>
      %68 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xf32>
      %69 = "bufferization.to_tensor"(%68) <{restrict, writable}> : (memref<16x16xf32>) -> tensor<16x16xf32>
      %70 = "hivm.hir.store"(%67, %69) {"inserted-store"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
      %71 = "tensor.empty"() : () -> tensor<16x16xf32>
      %72 = "hivm.hir.load"(%70, %71) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
      %73 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xf32>
      %74 = "bufferization.to_tensor"(%73) <{restrict, writable}> : (memref<16x16xf32>) -> tensor<16x16xf32>
      %75 = "hivm.hir.store"(%67, %74) {"inserted-store"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
      %76 = "tensor.empty"() : () -> tensor<16x16xf32>
      %77 = "hivm.hir.load"(%75, %76) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
      %78 = "arith.index_cast"(%41) : (i32) -> index
      %79 = "arith.muli"(%33, %arg12) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %80 = "arith.index_cast"(%79) : (i32) -> index
      %81 = "arith.addi"(%78, %80) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %82 = "arith.addi"(%81, %52) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %83 = "memref.reinterpret_cast"(%arg9, %82) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<16xf32, strided<[1], offset: ?>>
      %84 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16xf32>
      %85 = "memref.subview"(%83, %61) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<16xf32, strided<[1], offset: ?>>, index) -> memref<?xf32, strided<[1], offset: ?>>
      %86 = "memref.subview"(%84, %61) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<16xf32>, index) -> memref<?xf32, strided<[1]>>
      "hivm.hir.load"(%85, %86, %13, %6, %62) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xf32, strided<[1], offset: ?>>, memref<?xf32, strided<[1]>>, f32, index, i1) -> ()
      %87 = "bufferization.to_tensor"(%84) <{restrict, writable}> : (memref<16xf32>) -> tensor<16xf32>
      %88 = "tensor.empty"() : () -> tensor<16xf32>
      %89 = "tensor.empty"() : () -> tensor<16xf32>
      %90 = "hivm.hir.vexp"(%87, %89) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
      %91 = "memref.reinterpret_cast"(%arg5, %82) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16>, static_strides = array<i64: 1>}> : (memref<?xbf16>, index) -> memref<16xbf16, strided<[1], offset: ?>>
      %92 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16xbf16>
      %93 = "memref.subview"(%91, %61) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<16xbf16, strided<[1], offset: ?>>, index) -> memref<?xbf16, strided<[1], offset: ?>>
      %94 = "memref.subview"(%92, %61) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<16xbf16>, index) -> memref<?xbf16, strided<[1]>>
      "hivm.hir.load"(%93, %94, %3, %6, %62) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xbf16, strided<[1], offset: ?>>, memref<?xbf16, strided<[1]>>, bf16, index, i1) -> ()
      %95 = "bufferization.to_tensor"(%92) <{restrict, writable}> : (memref<16xbf16>) -> tensor<16xbf16>
      %96 = "hivm.hir.vcast"(%95, %88) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16xbf16>, tensor<16xf32>) -> tensor<16xf32>
      %97 = "arith.muli"(%49, %12) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %98 = "arith.index_cast"(%97) : (i32) -> index
      %99 = "arith.muli"(%52, %2) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %100 = "arith.addi"(%98, %99) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %101 = "memref.reinterpret_cast"(%arg4, %100) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 64>, static_strides = array<i64: 256, 1>}> : (memref<?xbf16>, index) -> memref<16x64xbf16, strided<[256, 1], offset: ?>>
      %102 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x64xbf16>
      %103 = "arith.minsi"(%61, %4) : (index, index) -> index
      %104 = "arith.cmpi"(%103, %4) <{predicate = 2 : i64}> : (index, index) -> i1
      %105 = "memref.subview"(%101, %103) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 64>, static_strides = array<i64: 1, 1>}> : (memref<16x64xbf16, strided<[256, 1], offset: ?>>, index) -> memref<?x64xbf16, strided<[256, 1], offset: ?>>
      %106 = "memref.subview"(%102, %103) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 64>, static_strides = array<i64: 1, 1>}> : (memref<16x64xbf16>, index) -> memref<?x64xbf16, strided<[64, 1]>>
      "hivm.hir.load"(%105, %106, %3, %6, %104) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x64xbf16, strided<[256, 1], offset: ?>>, memref<?x64xbf16, strided<[64, 1]>>, bf16, index, i1) -> ()
      %107 = "bufferization.to_tensor"(%102) <{restrict, writable}> : (memref<16x64xbf16>) -> tensor<16x64xbf16>
      %108 = "hivm.hir.vcast"(%107, %27) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x64xbf16>, tensor<16x64xf32>) -> tensor<16x64xf32>
      %109 = "tensor.expand_shape"(%96) <{reassociation = [[0, 1]], static_output_shape = array<i64: 16, 1>}> : (tensor<16xf32>) -> tensor<16x1xf32>
      %110 = "hivm.hir.vbrc"(%109, %26) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xf32>, tensor<16x64xf32>) -> tensor<16x64xf32>
      %111 = "hivm.hir.vmul"(%108, %110, %25) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x64xf32>, tensor<16x64xf32>, tensor<16x64xf32>) -> tensor<16x64xf32>
      %112 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x64xf32>
      %113 = "bufferization.to_tensor"(%112) <{restrict, writable}> : (memref<16x64xf32>) -> tensor<16x64xf32>
      %114 = "hivm.hir.store"(%111, %113) {"inserted-store"} : (tensor<16x64xf32>, tensor<16x64xf32>) -> tensor<16x64xf32>
      %115 = "tensor.empty"() : () -> tensor<16x64xf32>
      %116 = "hivm.hir.load"(%114, %115) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x64xf32>, tensor<16x64xf32>) -> tensor<16x64xf32>
      %117 = "tensor.empty"() : () -> tensor<16x64xf32>
      %118 = "hivm.hir.mmadL1"(%72, %116, %1, %4, %4, %5, %117) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_already_inserted = true} : (tensor<16x16xf32>, tensor<16x64xf32>, i1, index, index, index, tensor<16x64xf32>) -> tensor<16x64xf32>
      %119 = "memref.reinterpret_cast"(%arg7, %100) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 64>, static_strides = array<i64: 256, 1>}> : (memref<?xbf16>, index) -> memref<16x64xbf16, strided<[256, 1], offset: ?>>
      %120 = "tensor.extract_slice"(%118, %103) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 64>, static_strides = array<i64: 1, 1>}> : (tensor<16x64xf32>, index) -> tensor<?x64xf32>
      %121 = "memref.subview"(%119, %103) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 64>, static_strides = array<i64: 1, 1>}> : (memref<16x64xbf16, strided<[256, 1], offset: ?>>, index) -> memref<?x64xbf16, strided<[256, 1], offset: ?>>
      "hivm.hir.fixpipe"(%120, %121) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, pre_quant = #hivm.fixpipe_pre_quant_mode<F322BF16>}> : (tensor<?x64xf32>, memref<?x64xbf16, strided<[256, 1], offset: ?>>) -> ()
      %122 = "memref.reinterpret_cast"(%arg3, %100) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 64>, static_strides = array<i64: 256, 1>}> : (memref<?xbf16>, index) -> memref<16x64xbf16, strided<[256, 1], offset: ?>>
      %123 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x64xbf16>
      %124 = "memref.subview"(%122, %103) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 64>, static_strides = array<i64: 1, 1>}> : (memref<16x64xbf16, strided<[256, 1], offset: ?>>, index) -> memref<?x64xbf16, strided<[256, 1], offset: ?>>
      %125 = "memref.subview"(%123, %103) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 64>, static_strides = array<i64: 1, 1>}> : (memref<16x64xbf16>, index) -> memref<?x64xbf16, strided<[64, 1]>>
      "hivm.hir.load"(%124, %125, %3, %6, %104) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x64xbf16, strided<[256, 1], offset: ?>>, memref<?x64xbf16, strided<[64, 1]>>, bf16, index, i1) -> ()
      %126 = "bufferization.to_tensor"(%123) <{restrict, writable}> : (memref<16x64xbf16>) -> tensor<16x64xbf16>
      %127 = "hivm.hir.vcast"(%126, %24) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x64xbf16>, tensor<16x64xf32>) -> tensor<16x64xf32>
      %128 = "hivm.hir.vmul"(%127, %110, %23) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x64xf32>, tensor<16x64xf32>, tensor<16x64xf32>) -> tensor<16x64xf32>
      %129 = "tensor.expand_shape"(%90) <{reassociation = [[0, 1]], static_output_shape = array<i64: 16, 1>}> : (tensor<16xf32>) -> tensor<16x1xf32>
      %130 = "hivm.hir.vbrc"(%129, %22) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xf32>, tensor<16x64xf32>) -> tensor<16x64xf32>
      %131 = "hivm.hir.vmul"(%128, %130, %21) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x64xf32>, tensor<16x64xf32>, tensor<16x64xf32>) -> tensor<16x64xf32>
      %132 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x64xf32>
      %133 = "bufferization.to_tensor"(%132) <{restrict, writable}> : (memref<16x64xf32>) -> tensor<16x64xf32>
      %134 = "hivm.hir.store"(%131, %133) {"inserted-store"} : (tensor<16x64xf32>, tensor<16x64xf32>) -> tensor<16x64xf32>
      %135 = "tensor.empty"() : () -> tensor<16x64xf32>
      %136 = "hivm.hir.load"(%134, %135) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x64xf32>, tensor<16x64xf32>) -> tensor<16x64xf32>
      %137 = "tensor.empty"() : () -> tensor<16x64xf32>
      %138 = "hivm.hir.mmadL1"(%77, %136, %1, %4, %4, %5, %137) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_already_inserted = true} : (tensor<16x16xf32>, tensor<16x64xf32>, i1, index, index, index, tensor<16x64xf32>) -> tensor<16x64xf32>
      %139 = "memref.reinterpret_cast"(%arg6, %100) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 64>, static_strides = array<i64: 256, 1>}> : (memref<?xbf16>, index) -> memref<16x64xbf16, strided<[256, 1], offset: ?>>
      %140 = "tensor.extract_slice"(%138, %103) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 64>, static_strides = array<i64: 1, 1>}> : (tensor<16x64xf32>, index) -> tensor<?x64xf32>
      %141 = "memref.subview"(%139, %103) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 64>, static_strides = array<i64: 1, 1>}> : (memref<16x64xbf16, strided<[256, 1], offset: ?>>, index) -> memref<?x64xbf16, strided<[256, 1], offset: ?>>
      "hivm.hir.fixpipe"(%140, %141) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, pre_quant = #hivm.fixpipe_pre_quant_mode<F322BF16>}> : (tensor<?x64xf32>, memref<?x64xbf16, strided<[256, 1], offset: ?>>) -> ()
      "scf.yield"() : () -> ()
    }) : (i32, i32, i32) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false]> : vector<16xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<MIX>, mix_mode = "mix", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<MIX>} : () -> ()

