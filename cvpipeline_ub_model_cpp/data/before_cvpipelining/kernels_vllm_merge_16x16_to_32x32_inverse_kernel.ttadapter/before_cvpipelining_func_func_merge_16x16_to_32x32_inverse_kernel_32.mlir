"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, i32, i32, i32, i32) -> (), sym_name = "merge_16x16_to_32x32_inverse_kernel"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xbf16>, %arg4: memref<?xbf16>, %arg5: memref<?xbf16>, %arg6: i32, %arg7: i32, %arg8: i32, %arg9: i32):
    %0 = "arith.constant"() <{value = true}> : () -> i1
    %1 = "arith.constant"() <{value = -1.000000e+00 : f32}> : () -> f32
    %2 = "arith.constant"() <{value = -16 : i32}> : () -> i32
    %3 = "arith.constant"() <{value = 0.000000e+00 : bf16}> : () -> bf16
    %4 = "arith.constant"() <{value = 32 : index}> : () -> index
    %5 = "arith.constant"() <{value = 16 : index}> : () -> index
    %6 = "arith.constant"() <{value = 0 : index}> : () -> index
    %7 = "arith.constant"() <{value = 64 : index}> : () -> index
    %8 = "arith.constant"() <{value = 128 : index}> : () -> index
    %9 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %10 = "arith.constant"() <{value = 4 : i32}> : () -> i32
    %11 = "arith.constant"() <{value = 32 : i32}> : () -> i32
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
    %28 = "arith.divsi"(%19, %10) : (i32, i32) -> i32
    %29 = "arith.remsi"(%19, %10) : (i32, i32) -> i32
    %30 = "arith.muli"(%28, %arg6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %31 = "arith.muli"(%30, %10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %32 = "arith.addi"(%31, %29) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %33 = "arith.muli"(%32, %11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %34 = "arith.index_cast"(%33) : (i32) -> index
    %35 = "arith.muli"(%32, %12) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %36 = "arith.index_cast"(%35) : (i32) -> index
    %37 = "arith.muli"(%22, %11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %38 = "arith.addi"(%37, %12) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %39 = "arith.maxsi"(%38, %9) : (i32, i32) -> i32
    %40 = "arith.index_cast"(%39) : (i32) -> index
    %41 = "arith.muli"(%40, %8) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %42 = "arith.addi"(%41, %34) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %43 = "arith.index_cast"(%arg6) : (i32) -> index
    %44 = "memref.reinterpret_cast"(%arg3, %42) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 128, 1>}> : (memref<?xbf16>, index) -> memref<16x16xbf16, strided<[128, 1], offset: ?>>
    %45 = "arith.maxsi"(%37, %9) : (i32, i32) -> i32
    %46 = "arith.index_cast"(%45) : (i32) -> index
    %47 = "arith.muli"(%46, %7) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %48 = "arith.addi"(%47, %36) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %49 = "memref.reinterpret_cast"(%arg4, %48) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 64, 1>}> : (memref<?xbf16>, index) -> memref<16x16xbf16, strided<[64, 1], offset: ?>>
    %50 = "arith.muli"(%40, %7) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %51 = "arith.addi"(%50, %36) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %52 = "memref.reinterpret_cast"(%arg4, %51) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 64, 1>}> : (memref<?xbf16>, index) -> memref<16x16xbf16, strided<[64, 1], offset: ?>>
    %53 = "arith.muli"(%46, %8) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %54 = "arith.addi"(%53, %34) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %55 = "memref.reinterpret_cast"(%arg5, %54) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 128, 1>}> : (memref<?xbf16>, index) -> memref<16x16xbf16, strided<[128, 1], offset: ?>>
    %56 = "arith.addi"(%42, %5) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %57 = "memref.reinterpret_cast"(%arg5, %56) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 128, 1>}> : (memref<?xbf16>, index) -> memref<16x16xbf16, strided<[128, 1], offset: ?>>
    %58 = "memref.reinterpret_cast"(%arg5, %42) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 128, 1>}> : (memref<?xbf16>, index) -> memref<16x16xbf16, strided<[128, 1], offset: ?>>
    %59 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x16xbf16>
    %60 = "arith.divsi"(%41, %8) : (index, index) -> index
    %61 = "arith.subi"(%43, %60) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %62 = "arith.maxsi"(%61, %6) : (index, index) -> index
    %63 = "arith.minsi"(%62, %5) : (index, index) -> index
    %64 = "arith.remsi"(%41, %8) : (index, index) -> index
    %65 = "arith.subi"(%4, %64) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %66 = "arith.maxsi"(%65, %6) : (index, index) -> index
    %67 = "arith.minsi"(%66, %5) : (index, index) -> index
    %68 = "arith.subi"(%2, %37) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %69 = "arith.maxsi"(%68, %9) : (i32, i32) -> i32
    %70 = "arith.index_cast"(%69) : (i32) -> index
    %71 = "arith.minsi"(%70, %63) : (index, index) -> index
    %72 = "arith.subi"(%63, %71) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %73 = "arith.minsi"(%67, %6) : (index, index) -> index
    %74 = "arith.subi"(%67, %73) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %75 = "arith.cmpi"(%72, %5) <{predicate = 2 : i64}> : (index, index) -> i1
    %76 = "arith.cmpi"(%74, %5) <{predicate = 2 : i64}> : (index, index) -> i1
    %77 = "arith.ori"(%75, %76) : (i1, i1) -> i1
    %78 = "memref.subview"(%44, %72, %74) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16, strided<[128, 1], offset: ?>>, index, index) -> memref<?x?xbf16, strided<[128, 1], offset: ?>>
    %79 = "memref.subview"(%59, %71, %73, %72, %74) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16>, index, index, index, index) -> memref<?x?xbf16, strided<[16, 1], offset: ?>>
    "hivm.hir.load"(%78, %79, %3, %73, %77) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x?xbf16, strided<[128, 1], offset: ?>>, memref<?x?xbf16, strided<[16, 1], offset: ?>>, bf16, index, i1) -> ()
    %80 = "bufferization.to_tensor"(%59) <{restrict, writable}> : (memref<16x16xbf16>) -> tensor<16x16xbf16>
    %81 = "hivm.hir.vcast"(%80, %27) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x16xbf16>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %82 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xf32>
    %83 = "bufferization.to_tensor"(%82) <{restrict, writable}> : (memref<16x16xf32>) -> tensor<16x16xf32>
    %84 = "hivm.hir.store"(%81, %83) {"inserted-store"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %85 = "tensor.empty"() : () -> tensor<16x16xf32>
    %86 = "hivm.hir.load"(%84, %85) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %87 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x16xbf16>
    %88 = "arith.divsi"(%47, %7) : (index, index) -> index
    %89 = "arith.subi"(%43, %88) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %90 = "arith.maxsi"(%89, %6) : (index, index) -> index
    %91 = "arith.minsi"(%90, %5) : (index, index) -> index
    %92 = "arith.remsi"(%47, %7) : (index, index) -> index
    %93 = "arith.subi"(%5, %92) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %94 = "arith.maxsi"(%93, %6) : (index, index) -> index
    %95 = "arith.minsi"(%94, %5) : (index, index) -> index
    %96 = "arith.subi"(%9, %37) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %97 = "arith.maxsi"(%96, %9) : (i32, i32) -> i32
    %98 = "arith.index_cast"(%97) : (i32) -> index
    %99 = "arith.minsi"(%98, %91) : (index, index) -> index
    %100 = "arith.subi"(%91, %99) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %101 = "arith.minsi"(%95, %6) : (index, index) -> index
    %102 = "arith.subi"(%95, %101) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %103 = "arith.cmpi"(%100, %5) <{predicate = 2 : i64}> : (index, index) -> i1
    %104 = "arith.cmpi"(%102, %5) <{predicate = 2 : i64}> : (index, index) -> i1
    %105 = "arith.ori"(%103, %104) : (i1, i1) -> i1
    %106 = "memref.subview"(%49, %100, %102) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16, strided<[64, 1], offset: ?>>, index, index) -> memref<?x?xbf16, strided<[64, 1], offset: ?>>
    %107 = "memref.subview"(%87, %99, %101, %100, %102) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16>, index, index, index, index) -> memref<?x?xbf16, strided<[16, 1], offset: ?>>
    "hivm.hir.load"(%106, %107, %3, %101, %105) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x?xbf16, strided<[64, 1], offset: ?>>, memref<?x?xbf16, strided<[16, 1], offset: ?>>, bf16, index, i1) -> ()
    %108 = "bufferization.to_tensor"(%87) <{restrict, writable}> : (memref<16x16xbf16>) -> tensor<16x16xbf16>
    %109 = "hivm.hir.vcast"(%108, %26) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x16xbf16>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %110 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xf32>
    %111 = "bufferization.to_tensor"(%110) <{restrict, writable}> : (memref<16x16xf32>) -> tensor<16x16xf32>
    %112 = "hivm.hir.store"(%109, %111) {"inserted-store"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %113 = "tensor.empty"() : () -> tensor<16x16xf32>
    %114 = "hivm.hir.load"(%112, %113) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %115 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x16xbf16>
    %116 = "arith.divsi"(%50, %7) : (index, index) -> index
    %117 = "arith.subi"(%43, %116) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %118 = "arith.maxsi"(%117, %6) : (index, index) -> index
    %119 = "arith.minsi"(%118, %5) : (index, index) -> index
    %120 = "arith.remsi"(%50, %7) : (index, index) -> index
    %121 = "arith.subi"(%5, %120) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %122 = "arith.maxsi"(%121, %6) : (index, index) -> index
    %123 = "arith.minsi"(%122, %5) : (index, index) -> index
    %124 = "arith.minsi"(%70, %119) : (index, index) -> index
    %125 = "arith.subi"(%119, %124) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %126 = "arith.minsi"(%123, %6) : (index, index) -> index
    %127 = "arith.subi"(%123, %126) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %128 = "arith.cmpi"(%125, %5) <{predicate = 2 : i64}> : (index, index) -> i1
    %129 = "arith.cmpi"(%127, %5) <{predicate = 2 : i64}> : (index, index) -> i1
    %130 = "arith.ori"(%128, %129) : (i1, i1) -> i1
    %131 = "memref.subview"(%52, %125, %127) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16, strided<[64, 1], offset: ?>>, index, index) -> memref<?x?xbf16, strided<[64, 1], offset: ?>>
    %132 = "memref.subview"(%115, %124, %126, %125, %127) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16>, index, index, index, index) -> memref<?x?xbf16, strided<[16, 1], offset: ?>>
    "hivm.hir.load"(%131, %132, %3, %126, %130) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x?xbf16, strided<[64, 1], offset: ?>>, memref<?x?xbf16, strided<[16, 1], offset: ?>>, bf16, index, i1) -> ()
    %133 = "bufferization.to_tensor"(%115) <{restrict, writable}> : (memref<16x16xbf16>) -> tensor<16x16xbf16>
    %134 = "hivm.hir.vcast"(%133, %25) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x16xbf16>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %135 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xf32>
    %136 = "bufferization.to_tensor"(%135) <{restrict, writable}> : (memref<16x16xf32>) -> tensor<16x16xf32>
    %137 = "hivm.hir.store"(%134, %136) {"inserted-store"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %138 = "tensor.empty"() : () -> tensor<16x16xf32>
    %139 = "hivm.hir.load"(%137, %138) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %140 = "tensor.empty"() : () -> tensor<16x16xf32>
    %141 = "hivm.hir.mmadL1"(%139, %86, %0, %5, %5, %5, %140) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_already_inserted = true} : (tensor<16x16xf32>, tensor<16x16xf32>, i1, index, index, index, tensor<16x16xf32>) -> tensor<16x16xf32>
    %142 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xf32>
    %143 = "bufferization.to_tensor"(%142) <{restrict, writable}> : (memref<16x16xf32>) -> tensor<16x16xf32>
    %144 = "hivm.hir.fixpipe"(%141, %143) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %145 = "tensor.empty"() : () -> tensor<16x16xf32>
    %146 = "hivm.hir.load"(%144, %145) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %147 = "tensor.empty"() : () -> tensor<16x16xf32>
    %148 = "hivm.hir.mmadL1"(%146, %114, %0, %5, %5, %5, %147) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_already_inserted = true} : (tensor<16x16xf32>, tensor<16x16xf32>, i1, index, index, index, tensor<16x16xf32>) -> tensor<16x16xf32>
    %149 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xf32>
    %150 = "bufferization.to_tensor"(%149) <{restrict, writable}> : (memref<16x16xf32>) -> tensor<16x16xf32>
    %151 = "hivm.hir.fixpipe"(%148, %150) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %152 = "tensor.empty"() : () -> tensor<16x16xf32>
    %153 = "hivm.hir.load"(%151, %152) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %154 = "hivm.hir.vmul"(%153, %1, %24) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x16xf32>, f32, tensor<16x16xf32>) -> tensor<16x16xf32>
    %155 = "hivm.hir.vadd"(%154, %13, %23) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x16xf32>, f32, tensor<16x16xf32>) -> tensor<16x16xf32>
    %156 = "arith.divsi"(%53, %8) : (index, index) -> index
    %157 = "arith.subi"(%43, %156) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %158 = "arith.maxsi"(%157, %6) : (index, index) -> index
    %159 = "arith.minsi"(%158, %5) : (index, index) -> index
    %160 = "arith.remsi"(%53, %8) : (index, index) -> index
    %161 = "arith.subi"(%4, %160) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %162 = "arith.maxsi"(%161, %6) : (index, index) -> index
    %163 = "arith.minsi"(%162, %5) : (index, index) -> index
    %164 = "arith.minsi"(%98, %159) : (index, index) -> index
    %165 = "arith.subi"(%159, %164) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %166 = "arith.minsi"(%163, %6) : (index, index) -> index
    %167 = "arith.subi"(%163, %166) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %168 = "tensor.extract_slice"(%108, %164, %166, %165, %167) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<16x16xbf16>, index, index, index, index) -> tensor<?x?xbf16>
    %169 = "memref.subview"(%55, %165, %167) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16, strided<[128, 1], offset: ?>>, index, index) -> memref<?x?xbf16, strided<[128, 1], offset: ?>>
    "hivm.hir.store"(%168, %169) : (tensor<?x?xbf16>, memref<?x?xbf16, strided<[128, 1], offset: ?>>) -> ()
    %170 = "arith.subi"(%56, %34) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %171 = "arith.divsi"(%170, %8) : (index, index) -> index
    %172 = "arith.subi"(%43, %171) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %173 = "arith.maxsi"(%172, %6) : (index, index) -> index
    %174 = "arith.minsi"(%173, %5) : (index, index) -> index
    %175 = "arith.remsi"(%170, %8) : (index, index) -> index
    %176 = "arith.subi"(%4, %175) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %177 = "arith.maxsi"(%176, %6) : (index, index) -> index
    %178 = "arith.minsi"(%177, %5) : (index, index) -> index
    %179 = "arith.minsi"(%70, %174) : (index, index) -> index
    %180 = "arith.subi"(%174, %179) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %181 = "arith.minsi"(%178, %6) : (index, index) -> index
    %182 = "arith.subi"(%178, %181) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %183 = "tensor.extract_slice"(%133, %179, %181, %180, %182) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<16x16xbf16>, index, index, index, index) -> tensor<?x?xbf16>
    %184 = "memref.subview"(%57, %180, %182) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16, strided<[128, 1], offset: ?>>, index, index) -> memref<?x?xbf16, strided<[128, 1], offset: ?>>
    "hivm.hir.store"(%183, %184) : (tensor<?x?xbf16>, memref<?x?xbf16, strided<[128, 1], offset: ?>>) -> ()
    %185 = "tensor.empty"() : () -> tensor<16x16xbf16>
    %186 = "hivm.hir.vcast"(%155, %185) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x16xf32>, tensor<16x16xbf16>) -> tensor<16x16xbf16>
    %187 = "tensor.extract_slice"(%186, %71, %73, %72, %74) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<16x16xbf16>, index, index, index, index) -> tensor<?x?xbf16>
    %188 = "memref.subview"(%58, %72, %74) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16, strided<[128, 1], offset: ?>>, index, index) -> memref<?x?xbf16, strided<[128, 1], offset: ?>>
    "hivm.hir.store"(%187, %188) : (tensor<?x?xbf16>, memref<?x?xbf16, strided<[128, 1], offset: ?>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, false, false, false, false]> : vector<10xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<MIX>, mix_mode = "mix", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<MIX>} : () -> ()

