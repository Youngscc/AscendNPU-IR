"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xbf16>, memref<?xi8>, memref<?xbf16>, memref<?xi8>, memref<?xbf16>, i32, i32, i32) -> (), sym_name = "dot_scale_kernel"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xbf16>, %arg4: memref<?xi8>, %arg5: memref<?xbf16>, %arg6: memref<?xi8>, %arg7: memref<?xbf16>, %arg8: i32, %arg9: i32, %arg10: i32):
    %0 = "arith.constant"() <{value = true}> : () -> i1
    %1 = "arith.constant"() <{value = 32 : i32}> : () -> i32
    %2 = "arith.constant"() <{value = 2 : i32}> : () -> i32
    %3 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %4 = "arith.constant"() <{value = 7 : i32}> : () -> i32
    %5 = "arith.constant"() <{value = 127 : i16}> : () -> i16
    %6 = "arith.constant"() <{value = 16 : index}> : () -> index
    %7 = "arith.constant"() <{value = 32 : index}> : () -> index
    "hivm.hir.set_mask_norm"() : () -> ()
    %8 = "arith.muli"(%arg8, %arg9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %9 = "arith.muli"(%8, %arg10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%9) {logical_block_num} : (i32) -> ()
    %10 = "memref.reinterpret_cast"(%arg3) <{operandSegmentSizes = array<i32: 1, 0, 0, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: 16, 32>, static_strides = array<i64: 32, 1>}> : (memref<?xbf16>) -> memref<16x32xbf16, strided<[32, 1]>>
    %11 = "memref.reinterpret_cast"(%arg5) <{operandSegmentSizes = array<i32: 1, 0, 0, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: 32, 16>, static_strides = array<i64: 32, 1>}> : (memref<?xbf16>) -> memref<32x16xbf16, strided<[32, 1]>>
    %12 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x32xbf16>
    "hivm.hir.load"(%10, %12) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<16x32xbf16, strided<[32, 1]>>, memref<16x32xbf16>) -> ()
    %13 = "bufferization.to_tensor"(%12) <{restrict, writable}> : (memref<16x32xbf16>) -> tensor<16x32xbf16>
    %14 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<32x16xbf16>
    "hivm.hir.load"(%11, %14) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<32x16xbf16, strided<[32, 1]>>, memref<32x16xbf16>) -> ()
    %15 = "bufferization.to_tensor"(%14) <{restrict, writable}> : (memref<32x16xbf16>) -> tensor<32x16xbf16>
    %16 = "memref.reinterpret_cast"(%arg4) <{operandSegmentSizes = array<i32: 1, 0, 0, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: 16, 1>, static_strides = array<i64: 1, 1>}> : (memref<?xi8>) -> memref<16x1xi8, strided<[1, 1]>>
    %17 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x1xi8>
    "hivm.hir.load"(%16, %17) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<16x1xi8, strided<[1, 1]>>, memref<16x1xi8>) -> ()
    %18 = "bufferization.to_tensor"(%17) <{restrict, writable}> : (memref<16x1xi8>) -> tensor<16x1xi8>
    %19 = "memref.reinterpret_cast"(%arg6) <{operandSegmentSizes = array<i32: 1, 0, 0, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: 16, 1>, static_strides = array<i64: 1, 1>}> : (memref<?xi8>) -> memref<16x1xi8, strided<[1, 1]>>
    %20 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x1xi8>
    "hivm.hir.load"(%19, %20) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<16x1xi8, strided<[1, 1]>>, memref<16x1xi8>) -> ()
    %21 = "bufferization.to_tensor"(%20) <{restrict, writable}> : (memref<16x1xi8>) -> tensor<16x1xi8>
    %22 = "tensor.empty"() : () -> tensor<16x1xi16>
    %23 = "tensor.empty"() : () -> tensor<16x1xi16>
    %24 = "tensor.empty"() : () -> tensor<16x1xi16>
    %25 = "tensor.empty"() : () -> tensor<16x1xf16>
    %26 = "hivm.hir.vcast"(%18, %25) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x1xi8>, tensor<16x1xf16>) -> tensor<16x1xf16>
    %27 = "hivm.hir.vcast"(%26, %24) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<trunc>, transpose = array<i64>}> : (tensor<16x1xf16>, tensor<16x1xi16>) -> tensor<16x1xi16>
    %28 = "hivm.hir.vadd"(%27, %5, %23) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x1xi16>, i16, tensor<16x1xi16>) -> tensor<16x1xi16>
    %29 = "tensor.empty"() : () -> tensor<16x1xf32>
    %30 = "hivm.hir.vcast"(%28, %29) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x1xi16>, tensor<16x1xf32>) -> tensor<16x1xf32>
    %31 = "tensor.empty"() : () -> tensor<16x1xi32>
    %32 = "tensor.empty"() : () -> tensor<16x1xi32>
    %33 = "tensor.empty"() : () -> tensor<16x1xi32>
    %34 = "tensor.empty"() : () -> tensor<16x1xi32>
    %35 = "tensor.empty"() : () -> tensor<16x1xi32>
    %36 = "tensor.empty"() : () -> tensor<16x1xi32>
    %37 = "tensor.empty"() : () -> tensor<16x1xi32>
    %38 = "tensor.empty"() : () -> tensor<16x1xi32>
    %39 = "hivm.hir.vcast"(%30, %38) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<trunc>, transpose = array<i64>}> : (tensor<16x1xf32>, tensor<16x1xi32>) -> tensor<16x1xi32>
    %40 = "hivm.hir.vbrc"(%4, %37) <{broadcast_dims = array<i64>}> : (i32, tensor<16x1xi32>) -> tensor<16x1xi32>
    %41 = "tensor.empty"() : () -> tensor<16x1xi32>
    %42 = "hivm.hir.vbrc"(%4, %41) <{broadcast_dims = array<i64>}> : (i32, tensor<16x1xi32>) -> tensor<16x1xi32>
    %43 = "hivm.hir.vmax"(%42, %3, %36) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x1xi32>, i32, tensor<16x1xi32>) -> tensor<16x1xi32>
    %44 = "tensor.empty"() : () -> tensor<16x1xi1>
    %45 = "tensor.empty"() : () -> tensor<16x1xi1>
    %46 = "tensor.empty"() : () -> tensor<16x1xi1>
    %47 = "tensor.empty"() : () -> tensor<16x1xi1>
    %48 = "hivm.hir.vcmp"(%43, %40, %47) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<16x1xi32>, tensor<16x1xi32>, tensor<16x1xi1>) -> tensor<16x1xi1>
    %49 = "tensor.empty"() : () -> tensor<16x1xi32>
    %50 = "hivm.hir.vbrc"(%4, %49) <{broadcast_dims = array<i64>}> : (i32, tensor<16x1xi32>) -> tensor<16x1xi32>
    %51 = "hivm.hir.vmax"(%50, %1, %35) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x1xi32>, i32, tensor<16x1xi32>) -> tensor<16x1xi32>
    %52 = "hivm.hir.vcmp"(%51, %40, %46) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<16x1xi32>, tensor<16x1xi32>, tensor<16x1xi1>) -> tensor<16x1xi1>
    %53 = "hivm.hir.vnot"(%52, %45) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16x1xi1>, tensor<16x1xi1>) -> tensor<16x1xi1>
    %54 = "hivm.hir.vand"(%48, %53, %44) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x1xi1>, tensor<16x1xi1>, tensor<16x1xi1>) -> tensor<16x1xi1>
    %55 = "hivm.hir.vsel"(%54, %4, %3, %34) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<16x1xi1>, i32, i32, tensor<16x1xi32>) -> tensor<16x1xi32>
    %56 = "tensor.empty"() : () -> tensor<16x1xi32>
    %57 = "hivm.hir.vbrc"(%2, %56) <{broadcast_dims = array<i64>}> : (i32, tensor<16x1xi32>) -> tensor<16x1xi32>
    %58 = "hivm.hir.vpow"(%57, %55, %33) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x1xi32>, tensor<16x1xi32>, tensor<16x1xi32>) -> tensor<16x1xi32>
    %59 = "hivm.hir.vmul"(%39, %58, %32) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x1xi32>, tensor<16x1xi32>, tensor<16x1xi32>) -> tensor<16x1xi32>
    %60 = "hivm.hir.vsel"(%54, %59, %3, %31) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<16x1xi1>, tensor<16x1xi32>, i32, tensor<16x1xi32>) -> tensor<16x1xi32>
    %61 = "hivm.hir.vcast"(%60, %22) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<truncwithoverflow>, transpose = array<i64>}> : (tensor<16x1xi32>, tensor<16x1xi16>) -> tensor<16x1xi16>
    %62 = "hivm.hir.bitcast"(%61) : (tensor<16x1xi16>) -> tensor<16x1xbf16>
    %63 = "tensor.empty"() : () -> tensor<1x16xi8>
    %64 = "hivm.hir.vtranspose"(%21, %63) <{disable_align = false, permutation = array<i64: 1, 0>}> : (tensor<16x1xi8>, tensor<1x16xi8>) -> tensor<1x16xi8>
    %65 = "tensor.empty"() : () -> tensor<1x16xi16>
    %66 = "tensor.empty"() : () -> tensor<1x16xi16>
    %67 = "tensor.empty"() : () -> tensor<1x16xi16>
    %68 = "tensor.empty"() : () -> tensor<1x16xf16>
    %69 = "hivm.hir.vcast"(%64, %68) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<1x16xi8>, tensor<1x16xf16>) -> tensor<1x16xf16>
    %70 = "hivm.hir.vcast"(%69, %67) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<trunc>, transpose = array<i64>}> : (tensor<1x16xf16>, tensor<1x16xi16>) -> tensor<1x16xi16>
    %71 = "hivm.hir.vadd"(%70, %5, %66) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1x16xi16>, i16, tensor<1x16xi16>) -> tensor<1x16xi16>
    %72 = "tensor.empty"() : () -> tensor<1x16xf32>
    %73 = "hivm.hir.vcast"(%71, %72) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<1x16xi16>, tensor<1x16xf32>) -> tensor<1x16xf32>
    %74 = "tensor.empty"() : () -> tensor<1x16xi32>
    %75 = "tensor.empty"() : () -> tensor<1x16xi32>
    %76 = "tensor.empty"() : () -> tensor<1x16xi32>
    %77 = "tensor.empty"() : () -> tensor<1x16xi32>
    %78 = "tensor.empty"() : () -> tensor<1x16xi32>
    %79 = "tensor.empty"() : () -> tensor<1x16xi32>
    %80 = "tensor.empty"() : () -> tensor<1x16xi32>
    %81 = "tensor.empty"() : () -> tensor<1x16xi32>
    %82 = "hivm.hir.vcast"(%73, %81) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<trunc>, transpose = array<i64>}> : (tensor<1x16xf32>, tensor<1x16xi32>) -> tensor<1x16xi32>
    %83 = "hivm.hir.vbrc"(%4, %80) <{broadcast_dims = array<i64>}> : (i32, tensor<1x16xi32>) -> tensor<1x16xi32>
    %84 = "tensor.empty"() : () -> tensor<1x16xi32>
    %85 = "hivm.hir.vbrc"(%4, %84) <{broadcast_dims = array<i64>}> : (i32, tensor<1x16xi32>) -> tensor<1x16xi32>
    %86 = "hivm.hir.vmax"(%85, %3, %79) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1x16xi32>, i32, tensor<1x16xi32>) -> tensor<1x16xi32>
    %87 = "tensor.empty"() : () -> tensor<1x16xi1>
    %88 = "tensor.empty"() : () -> tensor<1x16xi1>
    %89 = "tensor.empty"() : () -> tensor<1x16xi1>
    %90 = "tensor.empty"() : () -> tensor<1x16xi1>
    %91 = "hivm.hir.vcmp"(%86, %83, %90) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<1x16xi32>, tensor<1x16xi32>, tensor<1x16xi1>) -> tensor<1x16xi1>
    %92 = "tensor.empty"() : () -> tensor<1x16xi32>
    %93 = "hivm.hir.vbrc"(%4, %92) <{broadcast_dims = array<i64>}> : (i32, tensor<1x16xi32>) -> tensor<1x16xi32>
    %94 = "hivm.hir.vmax"(%93, %1, %78) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1x16xi32>, i32, tensor<1x16xi32>) -> tensor<1x16xi32>
    %95 = "hivm.hir.vcmp"(%94, %83, %89) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<1x16xi32>, tensor<1x16xi32>, tensor<1x16xi1>) -> tensor<1x16xi1>
    %96 = "hivm.hir.vnot"(%95, %88) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<1x16xi1>, tensor<1x16xi1>) -> tensor<1x16xi1>
    %97 = "hivm.hir.vand"(%91, %96, %87) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1x16xi1>, tensor<1x16xi1>, tensor<1x16xi1>) -> tensor<1x16xi1>
    %98 = "hivm.hir.vsel"(%97, %4, %3, %77) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<1x16xi1>, i32, i32, tensor<1x16xi32>) -> tensor<1x16xi32>
    %99 = "tensor.empty"() : () -> tensor<1x16xi32>
    %100 = "hivm.hir.vbrc"(%2, %99) <{broadcast_dims = array<i64>}> : (i32, tensor<1x16xi32>) -> tensor<1x16xi32>
    %101 = "hivm.hir.vpow"(%100, %98, %76) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1x16xi32>, tensor<1x16xi32>, tensor<1x16xi32>) -> tensor<1x16xi32>
    %102 = "hivm.hir.vmul"(%82, %101, %75) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1x16xi32>, tensor<1x16xi32>, tensor<1x16xi32>) -> tensor<1x16xi32>
    %103 = "hivm.hir.vsel"(%97, %102, %3, %74) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<1x16xi1>, tensor<1x16xi32>, i32, tensor<1x16xi32>) -> tensor<1x16xi32>
    %104 = "hivm.hir.vcast"(%103, %65) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<truncwithoverflow>, transpose = array<i64>}> : (tensor<1x16xi32>, tensor<1x16xi16>) -> tensor<1x16xi16>
    %105 = "hivm.hir.bitcast"(%104) : (tensor<1x16xi16>) -> tensor<1x16xbf16>
    %106 = "tensor.empty"() : () -> tensor<1x32x16xbf16>
    %107 = "tensor.expand_shape"(%105) <{reassociation = [[0, 1], [2]], static_output_shape = array<i64: 1, 1, 16>}> : (tensor<1x16xbf16>) -> tensor<1x1x16xbf16>
    %108 = "hivm.hir.vbrc"(%107, %106) <{broadcast_dims = array<i64: 1>}> : (tensor<1x1x16xbf16>, tensor<1x32x16xbf16>) -> tensor<1x32x16xbf16>
    %109 = "tensor.collapse_shape"(%108) <{reassociation = [[0, 1], [2]]}> : (tensor<1x32x16xbf16>) -> tensor<32x16xbf16>
    %110 = "tensor.empty"() : () -> tensor<32x16xbf16>
    %111 = "tensor.empty"() : () -> tensor<32x16xf32>
    %112 = "tensor.empty"() : () -> tensor<32x16xf32>
    %113 = "tensor.empty"() : () -> tensor<32x16xf32>
    %114 = "hivm.hir.vcast"(%15, %113) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<32x16xbf16>, tensor<32x16xf32>) -> tensor<32x16xf32>
    %115 = "hivm.hir.vcast"(%109, %112) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<32x16xbf16>, tensor<32x16xf32>) -> tensor<32x16xf32>
    %116 = "hivm.hir.vmul"(%114, %115, %111) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32x16xf32>, tensor<32x16xf32>, tensor<32x16xf32>) -> tensor<32x16xf32>
    %117 = "hivm.hir.vcast"(%116, %110) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<32x16xf32>, tensor<32x16xbf16>) -> tensor<32x16xbf16>
    %118 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<32x16xbf16>
    %119 = "bufferization.to_tensor"(%118) <{restrict, writable}> : (memref<32x16xbf16>) -> tensor<32x16xbf16>
    %120 = "hivm.hir.store"(%117, %119) {"inserted-store"} : (tensor<32x16xbf16>, tensor<32x16xbf16>) -> tensor<32x16xbf16>
    %121 = "tensor.empty"() : () -> tensor<32x16xbf16>
    %122 = "hivm.hir.load"(%120, %121) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<32x16xbf16>, tensor<32x16xbf16>) -> tensor<32x16xbf16>
    %123 = "tensor.empty"() : () -> tensor<16x1x32xbf16>
    %124 = "tensor.expand_shape"(%62) <{reassociation = [[0], [1, 2]], static_output_shape = array<i64: 16, 1, 1>}> : (tensor<16x1xbf16>) -> tensor<16x1x1xbf16>
    %125 = "hivm.hir.vbrc"(%124, %123) <{broadcast_dims = array<i64: 2>}> : (tensor<16x1x1xbf16>, tensor<16x1x32xbf16>) -> tensor<16x1x32xbf16>
    %126 = "tensor.collapse_shape"(%125) <{reassociation = [[0], [1, 2]]}> : (tensor<16x1x32xbf16>) -> tensor<16x32xbf16>
    %127 = "tensor.empty"() : () -> tensor<16x32xbf16>
    %128 = "tensor.empty"() : () -> tensor<16x32xf32>
    %129 = "tensor.empty"() : () -> tensor<16x32xf32>
    %130 = "tensor.empty"() : () -> tensor<16x32xf32>
    %131 = "hivm.hir.vcast"(%13, %130) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x32xbf16>, tensor<16x32xf32>) -> tensor<16x32xf32>
    %132 = "hivm.hir.vcast"(%126, %129) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x32xbf16>, tensor<16x32xf32>) -> tensor<16x32xf32>
    %133 = "hivm.hir.vmul"(%131, %132, %128) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
    %134 = "hivm.hir.vcast"(%133, %127) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xbf16>) -> tensor<16x32xbf16>
    %135 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x32xbf16>
    %136 = "bufferization.to_tensor"(%135) <{restrict, writable}> : (memref<16x32xbf16>) -> tensor<16x32xbf16>
    %137 = "hivm.hir.store"(%134, %136) {"inserted-store"} : (tensor<16x32xbf16>, tensor<16x32xbf16>) -> tensor<16x32xbf16>
    %138 = "tensor.empty"() : () -> tensor<16x32xbf16>
    %139 = "hivm.hir.load"(%137, %138) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x32xbf16>, tensor<16x32xbf16>) -> tensor<16x32xbf16>
    %140 = "tensor.empty"() : () -> tensor<16x16xf32>
    %141 = "hivm.hir.mmadL1"(%139, %122, %0, %6, %7, %6, %140) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_already_inserted = true} : (tensor<16x32xbf16>, tensor<32x16xbf16>, i1, index, index, index, tensor<16x16xf32>) -> tensor<16x16xf32>
    %142 = "memref.reinterpret_cast"(%arg7) <{operandSegmentSizes = array<i32: 1, 0, 0, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 16, 1>}> : (memref<?xbf16>) -> memref<16x16xbf16, strided<[16, 1]>>
    "hivm.hir.fixpipe"(%141, %142) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, pre_quant = #hivm.fixpipe_pre_quant_mode<F322BF16>}> : (tensor<16x16xf32>, memref<16x16xbf16, strided<[16, 1]>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, true, true, false, false, false]> : vector<11xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<MIX>, mix_mode = "mix", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<MIX>} : () -> ()

