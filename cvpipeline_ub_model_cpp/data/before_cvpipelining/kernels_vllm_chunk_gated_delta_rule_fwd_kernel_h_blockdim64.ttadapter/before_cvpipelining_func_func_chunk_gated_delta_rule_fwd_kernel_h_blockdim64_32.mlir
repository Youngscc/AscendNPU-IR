"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, memref<?xf32>, memref<?xbf16>, i32, i32, i32, i32) -> (), sym_name = "chunk_gated_delta_rule_fwd_kernel_h_blockdim64"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xbf16>, %arg4: memref<?xbf16>, %arg5: memref<?xbf16>, %arg6: memref<?xbf16>, %arg7: memref<?xf32>, %arg8: memref<?xbf16>, %arg9: i32, %arg10: i32, %arg11: i32, %arg12: i32):
    %0 = "arith.constant"() <{value = 1 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = true}> : () -> i1
    %2 = "arith.constant"() <{value = -1.000000e+00 : f32}> : () -> f32
    %3 = "arith.constant"() <{value = 0.000000e+00 : bf16}> : () -> bf16
    %4 = "arith.constant"() <{value = 512 : index}> : () -> index
    %5 = "arith.constant"() <{value = 64 : index}> : () -> index
    %6 = "arith.constant"() <{value = 128 : index}> : () -> index
    %7 = "arith.constant"() <{value = 0xFF800000 : f32}> : () -> f32
    %8 = "arith.constant"() <{value = 512 : i32}> : () -> i32
    %9 = "arith.constant"() <{value = 16384 : i32}> : () -> i32
    %10 = "arith.constant"() <{value = 65536 : i32}> : () -> i32
    %11 = "arith.constant"() <{value = 63 : i32}> : () -> i32
    %12 = "arith.constant"() <{value = 0 : index}> : () -> index
    %13 = "arith.constant"() <{value = 4 : i32}> : () -> i32
    %14 = "arith.constant"() <{value = 64 : i32}> : () -> i32
    %15 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %16 = "arith.constant"() <{value = 128 : i32}> : () -> i32
    %17 = "arith.constant"() <{value = 0.000000e+00 : f32}> : () -> f32
    "hivm.hir.set_mask_norm"() : () -> ()
    %18 = "arith.muli"(%arg10, %arg11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %19 = "arith.muli"(%18, %arg12) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%19) {logical_block_num} : (i32) -> ()
    %20 = "hivm.hir.get_block_idx"() : () -> i64
    %21 = "arith.trunci"(%20) : (i64) -> i32
    %22 = "arith.divsi"(%21, %arg12) : (i32, i32) -> i32
    %23 = "arith.remsi"(%22, %arg11) : (i32, i32) -> i32
    %24 = "tensor.empty"() : () -> tensor<64xf32>
    %25 = "tensor.empty"() : () -> tensor<64xf32>
    %26 = "tensor.empty"() : () -> tensor<64xf32>
    %27 = "tensor.empty"() : () -> tensor<64xf32>
    %28 = "tensor.empty"() : () -> tensor<64xf32>
    %29 = "hivm.hir.vbrc"(%17, %28) <{broadcast_dims = array<i64>}> : (f32, tensor<64xf32>) -> tensor<64xf32>
    %30 = "tensor.empty"() : () -> tensor<64x64xf32>
    %31 = "tensor.empty"() : () -> tensor<64x64xf32>
    %32 = "tensor.empty"() : () -> tensor<64x64xf32>
    %33 = "tensor.empty"() : () -> tensor<64x64xf32>
    %34 = "tensor.empty"() : () -> tensor<64x64xf32>
    %35 = "tensor.empty"() : () -> tensor<64x64xf32>
    %36 = "tensor.empty"() : () -> tensor<64x64xf32>
    %37 = "tensor.empty"() : () -> tensor<128x64xf32>
    %38 = "tensor.empty"() : () -> tensor<128x64xf32>
    %39 = "tensor.empty"() : () -> tensor<128x64xf32>
    %40 = "hivm.hir.vbrc"(%17, %39) <{broadcast_dims = array<i64>}> : (f32, tensor<128x64xf32>) -> tensor<128x64xf32>
    %41 = "arith.divsi"(%23, %13) : (i32, i32) -> i32
    %42 = "arith.remsi"(%23, %13) : (i32, i32) -> i32
    %43 = "arith.muli"(%41, %arg9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %44 = "arith.addi"(%arg9, %11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %45 = "arith.divsi"(%44, %14) : (i32, i32) -> i32
    %46 = "arith.muli"(%41, %45) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %47 = "arith.muli"(%42, %9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %48 = "arith.muli"(%43, %8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %49 = "arith.index_cast"(%48) : (i32) -> index
    %50 = "arith.muli"(%42, %16) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %51 = "arith.index_cast"(%50) : (i32) -> index
    %52 = "arith.addi"(%49, %51) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %53 = "arith.index_cast"(%43) : (i32) -> index
    %54 = "arith.muli"(%42, %arg9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %55 = "arith.index_cast"(%54) : (i32) -> index
    %56 = "arith.addi"(%53, %55) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %57:2 = "scf.for"(%15, %45, %0, %40, %40) ({
    ^bb0(%arg13: i32, %arg14: tensor<128x64xf32>, %arg15: tensor<128x64xf32>):
      %58 = "arith.addi"(%46, %arg13) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %59 = "arith.muli"(%58, %10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %60 = "arith.index_cast"(%59) : (i32) -> index
      %61 = "arith.index_cast"(%47) : (i32) -> index
      %62 = "arith.addi"(%60, %61) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %63 = "memref.reinterpret_cast"(%arg8, %62) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 128, 64>, static_strides = array<i64: 128, 1>}> : (memref<?xbf16>, index) -> memref<128x64xbf16, strided<[128, 1], offset: ?>>
      %64 = "tensor.empty"() : () -> tensor<128x64xbf16>
      %65 = "tensor.empty"() : () -> tensor<128x64xbf16>
      %66 = "hivm.hir.vcast"(%arg14, %65) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<128x64xf32>, tensor<128x64xbf16>) -> tensor<128x64xbf16>
      %67 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<128x64xbf16>
      %68 = "bufferization.to_tensor"(%67) <{restrict, writable}> : (memref<128x64xbf16>) -> tensor<128x64xbf16>
      %69 = "hivm.hir.store"(%66, %68) {"inserted-store"} : (tensor<128x64xbf16>, tensor<128x64xbf16>) -> tensor<128x64xbf16>
      %70 = "tensor.empty"() : () -> tensor<128x64xbf16>
      %71 = "hivm.hir.load"(%69, %70) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<128x64xbf16>, tensor<128x64xbf16>) -> tensor<128x64xbf16>
      "hivm.hir.store"(%66, %63) : (tensor<128x64xbf16>, memref<128x64xbf16, strided<[128, 1], offset: ?>>) -> ()
      %72 = "arith.addi"(%62, %5) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %73 = "memref.reinterpret_cast"(%arg8, %72) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 128, 64>, static_strides = array<i64: 128, 1>}> : (memref<?xbf16>, index) -> memref<128x64xbf16, strided<[128, 1], offset: ?>>
      %74 = "hivm.hir.vcast"(%arg15, %64) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<128x64xf32>, tensor<128x64xbf16>) -> tensor<128x64xbf16>
      %75 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<128x64xbf16>
      %76 = "bufferization.to_tensor"(%75) <{restrict, writable}> : (memref<128x64xbf16>) -> tensor<128x64xbf16>
      %77 = "hivm.hir.store"(%74, %76) {"inserted-store"} : (tensor<128x64xbf16>, tensor<128x64xbf16>) -> tensor<128x64xbf16>
      %78 = "tensor.empty"() : () -> tensor<128x64xbf16>
      %79 = "hivm.hir.load"(%77, %78) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<128x64xbf16>, tensor<128x64xbf16>) -> tensor<128x64xbf16>
      "hivm.hir.store"(%74, %73) : (tensor<128x64xbf16>, memref<128x64xbf16, strided<[128, 1], offset: ?>>) -> ()
      %80 = "arith.muli"(%arg13, %14) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %81 = "arith.index_cast"(%80) : (i32) -> index
      %82 = "arith.muli"(%81, %4) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %83 = "arith.addi"(%52, %82) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %84 = "memref.reinterpret_cast"(%arg5, %83) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64, 128>, static_strides = array<i64: 512, 1>}> : (memref<?xbf16>, index) -> memref<64x128xbf16, strided<[512, 1], offset: ?>>
      %85 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<64x128xbf16>
      %86 = "arith.addi"(%81, %5) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %87 = "arith.index_cast"(%arg9) : (i32) -> index
      %88 = "arith.maxsi"(%81, %87) : (index, index) -> index
      %89 = "arith.minsi"(%86, %88) : (index, index) -> index
      %90 = "arith.subi"(%89, %81) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %91 = "arith.minsi"(%90, %5) : (index, index) -> index
      %92 = "arith.cmpi"(%91, %5) <{predicate = 2 : i64}> : (index, index) -> i1
      %93 = "memref.subview"(%84, %91) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 128>, static_strides = array<i64: 1, 1>}> : (memref<64x128xbf16, strided<[512, 1], offset: ?>>, index) -> memref<?x128xbf16, strided<[512, 1], offset: ?>>
      %94 = "memref.subview"(%85, %91) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 128>, static_strides = array<i64: 1, 1>}> : (memref<64x128xbf16>, index) -> memref<?x128xbf16, strided<[128, 1]>>
      "hivm.hir.load"(%93, %94, %3, %12, %92) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<?x128xbf16, strided<[512, 1], offset: ?>>, memref<?x128xbf16, strided<[128, 1]>>, bf16, index, i1) -> ()
      %95 = "bufferization.to_tensor"(%85) <{restrict, writable}> : (memref<64x128xbf16>) -> tensor<64x128xbf16>
      %96 = "arith.maxsi"(%80, %15) : (i32, i32) -> i32
      %97 = "arith.index_cast"(%96) : (i32) -> index
      %98 = "arith.muli"(%97, %4) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %99 = "arith.addi"(%98, %52) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %100 = "memref.reinterpret_cast"(%arg3, %99) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64, 128>, static_strides = array<i64: 512, 1>}> : (memref<?xbf16>, index) -> memref<64x128xbf16, strided<[512, 1], offset: ?>>
      %101 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<64x128xbf16>
      %102 = "arith.divsi"(%98, %4) : (index, index) -> index
      %103 = "arith.subi"(%87, %102) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %104 = "arith.maxsi"(%103, %12) : (index, index) -> index
      %105 = "arith.minsi"(%104, %5) : (index, index) -> index
      %106 = "arith.remsi"(%98, %4) : (index, index) -> index
      %107 = "arith.subi"(%6, %106) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %108 = "arith.maxsi"(%107, %12) : (index, index) -> index
      %109 = "arith.minsi"(%108, %6) : (index, index) -> index
      %110 = "arith.subi"(%15, %80) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %111 = "arith.maxsi"(%110, %15) : (i32, i32) -> i32
      %112 = "arith.index_cast"(%111) : (i32) -> index
      %113 = "arith.minsi"(%112, %105) : (index, index) -> index
      %114 = "arith.subi"(%105, %113) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %115 = "arith.minsi"(%109, %12) : (index, index) -> index
      %116 = "arith.subi"(%109, %115) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %117 = "arith.cmpi"(%114, %5) <{predicate = 2 : i64}> : (index, index) -> i1
      %118 = "arith.cmpi"(%116, %6) <{predicate = 2 : i64}> : (index, index) -> i1
      %119 = "arith.ori"(%117, %118) : (i1, i1) -> i1
      %120 = "memref.subview"(%100, %114, %116) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<64x128xbf16, strided<[512, 1], offset: ?>>, index, index) -> memref<?x?xbf16, strided<[512, 1], offset: ?>>
      %121 = "memref.subview"(%101, %113, %115, %114, %116) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<64x128xbf16>, index, index, index, index) -> memref<?x?xbf16, strided<[128, 1], offset: ?>>
      "hivm.hir.load"(%120, %121, %3, %115, %119) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<?x?xbf16, strided<[512, 1], offset: ?>>, memref<?x?xbf16, strided<[128, 1], offset: ?>>, bf16, index, i1) -> ()
      %122 = "bufferization.to_tensor"(%101) <{restrict, writable}> : (memref<64x128xbf16>) -> tensor<64x128xbf16>
      %123 = "arith.addi"(%arg13, %0) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %124 = "arith.muli"(%123, %14) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %125 = "arith.minsi"(%124, %arg9) : (i32, i32) -> i32
      %126 = "arith.subi"(%125, %0) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %127 = "arith.index_cast"(%126) : (i32) -> index
      %128 = "arith.addi"(%56, %127) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %129 = "memref.reinterpret_cast"(%arg7, %128) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<1xf32, strided<[1], offset: ?>>
      %130 = "memref.load"(%129, %12) : (memref<1xf32, strided<[1], offset: ?>>, index) -> f32
      %131 = "arith.addi"(%56, %81) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %132 = "memref.reinterpret_cast"(%arg7, %131) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<64xf32, strided<[1], offset: ?>>
      %133 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<64xf32>
      %134 = "arith.cmpi"(%90, %5) <{predicate = 2 : i64}> : (index, index) -> i1
      %135 = "memref.subview"(%132, %90) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<64xf32, strided<[1], offset: ?>>, index) -> memref<?xf32, strided<[1], offset: ?>>
      %136 = "memref.subview"(%133, %90) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<64xf32>, index) -> memref<?xf32, strided<[1]>>
      "hivm.hir.load"(%135, %136, %17, %12, %134) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xf32, strided<[1], offset: ?>>, memref<?xf32, strided<[1]>>, f32, index, i1) -> ()
      %137 = "bufferization.to_tensor"(%133) <{restrict, writable}> : (memref<64xf32>) -> tensor<64xf32>
      %138 = "hivm.hir.vmul"(%137, %2, %27) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, f32, tensor<64xf32>) -> tensor<64xf32>
      %139 = "hivm.hir.vadd"(%138, %130, %26) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, f32, tensor<64xf32>) -> tensor<64xf32>
      %140 = "tensor.empty"() : () -> tensor<64xi1>
      %141 = "hivm.hir.vcmp"(%139, %29, %140) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<le>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>, tensor<64xi1>) -> tensor<64xi1>
      %142 = "hivm.hir.vsel"(%141, %139, %7, %25) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<64xi1>, tensor<64xf32>, f32, tensor<64xf32>) -> tensor<64xf32>
      %143 = "hivm.hir.vexp"(%142, %24) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
      %144 = "tensor.empty"() : () -> tensor<1xf32>
      %145 = "tensor.empty"() : () -> tensor<1xf32>
      %146 = "tensor.insert"(%130, %145, %12) : (f32, tensor<1xf32>, index) -> tensor<1xf32>
      %147 = "hivm.hir.vexp"(%146, %144) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<1xf32>, tensor<1xf32>) -> tensor<1xf32>
      %148 = "tensor.extract"(%147, %12) {"DuplicateTensorExtractForCube::visitedLabel" = 1 : i32} : (tensor<1xf32>, index) -> f32
      %149 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<1xf32>
      %150 = "bufferization.to_tensor"(%149) <{restrict, writable}> : (memref<1xf32>) -> tensor<1xf32>
      %151 = "hivm.hir.store"(%147, %150) {"inserted-store"} : (tensor<1xf32>, tensor<1xf32>) -> tensor<1xf32>
      "annotation.mark"(%151) {hivm.tcore_type = #hivm.tcore_type<VECTOR>} : (tensor<1xf32>) -> ()
      %152 = "tensor.extract"(%151, %12) {"DuplicateTensorExtractForCube::newExtractLabel" = 1 : i32, "DuplicateTensorExtractForCube::visitedLabel" = 1 : i32} : (tensor<1xf32>, index) -> f32
      "annotation.mark"(%148, %152) <{keys = []}> {"DuplicateTensorExtractForCube::replacementLabel" = 1 : i32} : (f32, f32) -> ()
      %153 = "memref.reinterpret_cast"(%arg4, %83) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64, 64>, static_strides = array<i64: 512, 1>}> : (memref<?xbf16>, index) -> memref<64x64xbf16, strided<[512, 1], offset: ?>>
      %154 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<64x64xbf16>
      %155 = "memref.subview"(%153, %91) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 64>, static_strides = array<i64: 1, 1>}> : (memref<64x64xbf16, strided<[512, 1], offset: ?>>, index) -> memref<?x64xbf16, strided<[512, 1], offset: ?>>
      %156 = "memref.subview"(%154, %91) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 64>, static_strides = array<i64: 1, 1>}> : (memref<64x64xbf16>, index) -> memref<?x64xbf16, strided<[64, 1]>>
      "hivm.hir.load"(%155, %156, %3, %12, %92) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x64xbf16, strided<[512, 1], offset: ?>>, memref<?x64xbf16, strided<[64, 1]>>, bf16, index, i1) -> ()
      %157 = "bufferization.to_tensor"(%154) <{restrict, writable}> : (memref<64x64xbf16>) -> tensor<64x64xbf16>
      %158 = "hivm.hir.vcast"(%157, %36) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<64x64xbf16>, tensor<64x64xf32>) -> tensor<64x64xf32>
      %159 = "tensor.empty"() : () -> tensor<64x64xf32>
      %160 = "hivm.hir.mmadL1"(%95, %71, %1, %91, %6, %5, %159) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_already_inserted = true} : (tensor<64x128xbf16>, tensor<128x64xbf16>, i1, index, index, index, tensor<64x64xf32>) -> tensor<64x64xf32>
      %161 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<64x64xf32>
      %162 = "bufferization.to_tensor"(%161) <{restrict, writable}> : (memref<64x64xf32>) -> tensor<64x64xf32>
      %163 = "hivm.hir.fixpipe"(%160, %162) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<64x64xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
      %164 = "tensor.empty"() : () -> tensor<64x64xf32>
      %165 = "hivm.hir.load"(%163, %164) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<64x64xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
      %166 = "hivm.hir.vsub"(%158, %165, %35) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x64xf32>, tensor<64x64xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
      %167 = "memref.reinterpret_cast"(%arg6, %99) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64, 64>, static_strides = array<i64: 512, 1>}> : (memref<?xbf16>, index) -> memref<64x64xbf16, strided<[512, 1], offset: ?>>
      %168 = "tensor.empty"() : () -> tensor<64x64xbf16>
      %169 = "tensor.empty"() : () -> tensor<64x64xbf16>
      %170 = "tensor.empty"() : () -> tensor<64x64xbf16>
      %171 = "tensor.empty"() : () -> tensor<64x64xbf16>
      %172 = "hivm.hir.vcast"(%166, %171) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<64x64xf32>, tensor<64x64xbf16>) -> tensor<64x64xbf16>
      %173 = "arith.minsi"(%108, %5) : (index, index) -> index
      %174 = "arith.minsi"(%173, %12) : (index, index) -> index
      %175 = "arith.subi"(%173, %174) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %176 = "tensor.extract_slice"(%172, %113, %174, %114, %175) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<64x64xbf16>, index, index, index, index) -> tensor<?x?xbf16>
      %177 = "memref.subview"(%167, %114, %175) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<64x64xbf16, strided<[512, 1], offset: ?>>, index, index) -> memref<?x?xbf16, strided<[512, 1], offset: ?>>
      "hivm.hir.store"(%176, %177) : (tensor<?x?xbf16>, memref<?x?xbf16, strided<[512, 1], offset: ?>>) -> ()
      %178 = "tensor.expand_shape"(%143) <{reassociation = [[0, 1]], static_output_shape = array<i64: 64, 1>}> : (tensor<64xf32>) -> tensor<64x1xf32>
      %179 = "hivm.hir.vbrc"(%178, %34) <{broadcast_dims = array<i64: 1>}> : (tensor<64x1xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
      %180 = "hivm.hir.vmul"(%166, %179, %33) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x64xf32>, tensor<64x64xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
      %181 = "hivm.hir.vmul"(%arg14, %148, %38) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128x64xf32>, f32, tensor<128x64xf32>) -> tensor<128x64xf32>
      %182 = "hivm.hir.vcast"(%180, %170) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<64x64xf32>, tensor<64x64xbf16>) -> tensor<64x64xbf16>
      %183 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<64x64xbf16>
      %184 = "bufferization.to_tensor"(%183) <{restrict, writable}> : (memref<64x64xbf16>) -> tensor<64x64xbf16>
      %185 = "hivm.hir.store"(%182, %184) {"inserted-store"} : (tensor<64x64xbf16>, tensor<64x64xbf16>) -> tensor<64x64xbf16>
      %186 = "tensor.empty"() : () -> tensor<64x64xbf16>
      %187 = "hivm.hir.load"(%185, %186) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<64x64xbf16>, tensor<64x64xbf16>) -> tensor<64x64xbf16>
      %188 = "tensor.empty"() : () -> tensor<128x64xf32>
      %189 = "hivm.hir.mmadL1"(%122, %187, %1, %116, %114, %5, %188) <{a_transpose, operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_already_inserted = true} : (tensor<64x128xbf16>, tensor<64x64xbf16>, i1, index, index, index, tensor<128x64xf32>) -> tensor<128x64xf32>
      %190 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<128x64xf32>
      %191 = "bufferization.to_tensor"(%190) <{restrict, writable}> : (memref<128x64xf32>) -> tensor<128x64xf32>
      %192 = "hivm.hir.fixpipe"(%189, %191) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<128x64xf32>, tensor<128x64xf32>) -> tensor<128x64xf32>
      %193 = "tensor.empty"() : () -> tensor<128x64xf32>
      %194 = "hivm.hir.load"(%192, %193) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<128x64xf32>, tensor<128x64xf32>) -> tensor<128x64xf32>
      %195 = "tensor.empty"() : () -> tensor<128x64xf32>
      %196 = "hivm.hir.vadd"(%194, %181, %195) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128x64xf32>, tensor<128x64xf32>, tensor<128x64xf32>) -> tensor<128x64xf32>
      %197 = "arith.addi"(%83, %5) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %198 = "memref.reinterpret_cast"(%arg4, %197) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64, 64>, static_strides = array<i64: 512, 1>}> : (memref<?xbf16>, index) -> memref<64x64xbf16, strided<[512, 1], offset: ?>>
      %199 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<64x64xbf16>
      %200 = "memref.subview"(%198, %91) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 64>, static_strides = array<i64: 1, 1>}> : (memref<64x64xbf16, strided<[512, 1], offset: ?>>, index) -> memref<?x64xbf16, strided<[512, 1], offset: ?>>
      %201 = "memref.subview"(%199, %91) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 64>, static_strides = array<i64: 1, 1>}> : (memref<64x64xbf16>, index) -> memref<?x64xbf16, strided<[64, 1]>>
      "hivm.hir.load"(%200, %201, %3, %12, %92) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x64xbf16, strided<[512, 1], offset: ?>>, memref<?x64xbf16, strided<[64, 1]>>, bf16, index, i1) -> ()
      %202 = "bufferization.to_tensor"(%199) <{restrict, writable}> : (memref<64x64xbf16>) -> tensor<64x64xbf16>
      %203 = "hivm.hir.vcast"(%202, %32) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<64x64xbf16>, tensor<64x64xf32>) -> tensor<64x64xf32>
      %204 = "tensor.empty"() : () -> tensor<64x64xf32>
      %205 = "hivm.hir.mmadL1"(%95, %79, %1, %91, %6, %5, %204) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_already_inserted = true} : (tensor<64x128xbf16>, tensor<128x64xbf16>, i1, index, index, index, tensor<64x64xf32>) -> tensor<64x64xf32>
      %206 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<64x64xf32>
      %207 = "bufferization.to_tensor"(%206) <{restrict, writable}> : (memref<64x64xf32>) -> tensor<64x64xf32>
      %208 = "hivm.hir.fixpipe"(%205, %207) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<64x64xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
      %209 = "tensor.empty"() : () -> tensor<64x64xf32>
      %210 = "hivm.hir.load"(%208, %209) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<64x64xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
      %211 = "hivm.hir.vsub"(%203, %210, %31) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x64xf32>, tensor<64x64xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
      %212 = "arith.addi"(%99, %5) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %213 = "memref.reinterpret_cast"(%arg6, %212) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64, 64>, static_strides = array<i64: 512, 1>}> : (memref<?xbf16>, index) -> memref<64x64xbf16, strided<[512, 1], offset: ?>>
      %214 = "hivm.hir.vcast"(%211, %169) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<64x64xf32>, tensor<64x64xbf16>) -> tensor<64x64xbf16>
      %215 = "arith.subi"(%212, %52) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %216 = "arith.divsi"(%215, %4) : (index, index) -> index
      %217 = "arith.subi"(%87, %216) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %218 = "arith.maxsi"(%217, %12) : (index, index) -> index
      %219 = "arith.minsi"(%218, %5) : (index, index) -> index
      %220 = "arith.remsi"(%215, %4) : (index, index) -> index
      %221 = "arith.subi"(%6, %220) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %222 = "arith.maxsi"(%221, %12) : (index, index) -> index
      %223 = "arith.minsi"(%222, %5) : (index, index) -> index
      %224 = "arith.minsi"(%112, %219) : (index, index) -> index
      %225 = "arith.subi"(%219, %224) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %226 = "arith.minsi"(%223, %12) : (index, index) -> index
      %227 = "arith.subi"(%223, %226) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %228 = "tensor.extract_slice"(%214, %224, %226, %225, %227) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<64x64xbf16>, index, index, index, index) -> tensor<?x?xbf16>
      %229 = "memref.subview"(%213, %225, %227) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<64x64xbf16, strided<[512, 1], offset: ?>>, index, index) -> memref<?x?xbf16, strided<[512, 1], offset: ?>>
      "hivm.hir.store"(%228, %229) : (tensor<?x?xbf16>, memref<?x?xbf16, strided<[512, 1], offset: ?>>) -> ()
      %230 = "hivm.hir.vmul"(%211, %179, %30) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x64xf32>, tensor<64x64xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
      %231 = "hivm.hir.vmul"(%arg15, %148, %37) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128x64xf32>, f32, tensor<128x64xf32>) -> tensor<128x64xf32>
      %232 = "hivm.hir.vcast"(%230, %168) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<64x64xf32>, tensor<64x64xbf16>) -> tensor<64x64xbf16>
      %233 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<64x64xbf16>
      %234 = "bufferization.to_tensor"(%233) <{restrict, writable}> : (memref<64x64xbf16>) -> tensor<64x64xbf16>
      %235 = "hivm.hir.store"(%232, %234) {"inserted-store"} : (tensor<64x64xbf16>, tensor<64x64xbf16>) -> tensor<64x64xbf16>
      %236 = "tensor.empty"() : () -> tensor<64x64xbf16>
      %237 = "hivm.hir.load"(%235, %236) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<64x64xbf16>, tensor<64x64xbf16>) -> tensor<64x64xbf16>
      %238 = "tensor.empty"() : () -> tensor<128x64xf32>
      %239 = "hivm.hir.mmadL1"(%122, %237, %1, %116, %114, %5, %238) <{a_transpose, operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_already_inserted = true} : (tensor<64x128xbf16>, tensor<64x64xbf16>, i1, index, index, index, tensor<128x64xf32>) -> tensor<128x64xf32>
      %240 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<128x64xf32>
      %241 = "bufferization.to_tensor"(%240) <{restrict, writable}> : (memref<128x64xf32>) -> tensor<128x64xf32>
      %242 = "hivm.hir.fixpipe"(%239, %241) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<128x64xf32>, tensor<128x64xf32>) -> tensor<128x64xf32>
      %243 = "tensor.empty"() : () -> tensor<128x64xf32>
      %244 = "hivm.hir.load"(%242, %243) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<128x64xf32>, tensor<128x64xf32>) -> tensor<128x64xf32>
      %245 = "tensor.empty"() : () -> tensor<128x64xf32>
      %246 = "hivm.hir.vadd"(%244, %231, %245) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128x64xf32>, tensor<128x64xf32>, tensor<128x64xf32>) -> tensor<128x64xf32>
      "scf.yield"(%196, %246) : (tensor<128x64xf32>, tensor<128x64xf32>) -> ()
    }) : (i32, i32, i32, tensor<128x64xf32>, tensor<128x64xf32>) -> (tensor<128x64xf32>, tensor<128x64xf32>)
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, true, true, true, false, false, false, false]> : vector<13xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<MIX>, mix_mode = "mix", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<MIX>} : () -> ()

