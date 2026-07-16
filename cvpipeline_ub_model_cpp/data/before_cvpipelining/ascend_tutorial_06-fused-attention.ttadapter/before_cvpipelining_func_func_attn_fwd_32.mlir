"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32}, {}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, memref<?xf32>, memref<?xbf16>, memref<?xf32>, f32, i32, i32, i32) -> (), sym_name = "_attn_fwd"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xbf16>, %arg4: memref<?xbf16>, %arg5: memref<?xbf16>, %arg6: memref<?xf32>, %arg7: memref<?xbf16>, %arg8: memref<?xf32>, %arg9: f32, %arg10: i32, %arg11: i32, %arg12: i32):
    %0 = "arith.constant"() <{value = true}> : () -> i1
    %1 = "arith.constant"() <{value = 64 : index}> : () -> index
    %2 = "arith.constant"() <{value = 1.000000e+00 : f32}> : () -> f32
    %3 = "arith.constant"() <{value = 0xFF800000 : f32}> : () -> f32
    %4 = "arith.constant"() <{value = 128 : i32}> : () -> i32
    %5 = "arith.constant"() <{value = 16 : i32}> : () -> i32
    %6 = "arith.constant"() <{value = 32 : i32}> : () -> i32
    %7 = "arith.constant"() <{value = 2097152 : i64}> : () -> i64
    %8 = "arith.constant"() <{value = 65536 : i64}> : () -> i64
    %9 = "arith.constant"() <{value = 64 : i32}> : () -> i32
    %10 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %11 = "arith.constant"() <{value = 1024 : i32}> : () -> i32
    %12 = "arith.constant"() <{value = 2048 : i32}> : () -> i32
    %13 = "arith.constant"() <{value = 20 : i32}> : () -> i32
    %14 = "arith.constant"() <{value = 0.000000e+00 : f32}> : () -> f32
    %15 = "arith.constant"() <{value = 128 : index}> : () -> index
    "hivm.hir.set_mask_norm"() : () -> ()
    %16 = "arith.muli"(%arg10, %arg11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %17 = "arith.muli"(%16, %arg12) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%17) {logical_block_num} : (i32) -> ()
    %18 = "hivm.hir.get_block_idx"() : () -> i64
    %19 = "arith.trunci"(%18) : (i64) -> i32
    %20 = "arith.muli"(%arg12, %arg11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %21 = "arith.divsi"(%19, %20) : (i32, i32) -> i32
    %22 = "arith.remsi"(%21, %arg10) : (i32, i32) -> i32
    %23 = "tensor.empty"() : () -> tensor<64x64xf32>
    %24 = "tensor.empty"() : () -> tensor<64x64xf32>
    %25 = "tensor.empty"() : () -> tensor<64x64xf32>
    %26 = "tensor.empty"() : () -> tensor<64x64xf32>
    %27 = "tensor.empty"() : () -> tensor<64x64xf32>
    %28 = "hivm.hir.vbrc"(%14, %27) <{broadcast_dims = array<i64>}> : (f32, tensor<64x64xf32>) -> tensor<64x64xf32>
    %29 = "tensor.empty"() : () -> tensor<64x128xf32>
    %30 = "tensor.empty"() : () -> tensor<64x128xf32>
    %31 = "tensor.empty"() : () -> tensor<64x128xf32>
    %32 = "tensor.empty"() : () -> tensor<64x128xf32>
    %33 = "tensor.empty"() : () -> tensor<64xf32>
    %34 = "tensor.empty"() : () -> tensor<64xf32>
    %35 = "tensor.empty"() : () -> tensor<64xf32>
    %36 = "tensor.empty"() : () -> tensor<64xf32>
    %37 = "tensor.empty"() : () -> tensor<64xf32>
    %38 = "tensor.empty"() : () -> tensor<64xf32>
    %39 = "tensor.empty"() : () -> tensor<64xf32>
    %40 = "tensor.empty"() : () -> tensor<64xf32>
    %41 = "tensor.empty"() : () -> tensor<64xf32>
    %42 = "tensor.empty"() : () -> tensor<64xf32>
    %43 = "tensor.empty"() : () -> tensor<64xf32>
    %44 = "hivm.hir.vbrc"(%3, %43) <{broadcast_dims = array<i64>}> : (f32, tensor<64xf32>) -> tensor<64xf32>
    %45 = "hivm.hir.vbrc"(%2, %42) <{broadcast_dims = array<i64>}> : (f32, tensor<64xf32>) -> tensor<64xf32>
    "scf.for"(%22, %12, %13) ({
    ^bb0(%arg13: i32):
      %46 = "arith.divsi"(%arg13, %5) : (i32, i32) -> i32
      %47 = "arith.remsi"(%arg13, %5) : (i32, i32) -> i32
      %48 = "arith.divsi"(%46, %6) : (i32, i32) -> i32
      %49 = "arith.remsi"(%46, %6) : (i32, i32) -> i32
      %50 = "arith.extsi"(%48) : (i32) -> i64
      %51 = "arith.muli"(%50, %7) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
      %52 = "arith.extsi"(%49) : (i32) -> i64
      %53 = "arith.muli"(%52, %8) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
      %54 = "arith.addi"(%51, %53) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
      %55 = "arith.index_cast"(%54) : (i64) -> index
      %56 = "arith.muli"(%47, %9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %57 = "arith.maxsi"(%56, %10) : (i32, i32) -> i32
      %58 = "arith.index_cast"(%57) : (i32) -> index
      %59 = "arith.muli"(%58, %1) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %60 = "arith.addi"(%59, %55) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %61 = "memref.reinterpret_cast"(%arg3, %60) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64, 64>, static_strides = array<i64: 64, 1>}> : (memref<?xbf16>, index) -> memref<64x64xbf16, strided<[64, 1], offset: ?>>
      %62 = "memref.reinterpret_cast"(%arg7, %60) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64, 64>, static_strides = array<i64: 64, 1>}> : (memref<?xbf16>, index) -> memref<64x64xbf16, strided<[64, 1], offset: ?>>
      %63 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<64x64xbf16>
      "hivm.hir.load"(%61, %63) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<64x64xbf16, strided<[64, 1], offset: ?>>, memref<64x64xbf16>) -> ()
      %64 = "bufferization.to_tensor"(%63) <{restrict, writable}> : (memref<64x64xbf16>) -> tensor<64x64xbf16>
      %65:5 = "scf.for"(%10, %11, %4, %28, %45, %44, %10, %10) ({
      ^bb0(%arg14: i32, %arg15: tensor<64x64xf32>, %arg16: tensor<64xf32>, %arg17: tensor<64xf32>, %arg18: i32, %arg19: i32):
        %78 = "arith.index_cast"(%arg18) : (i32) -> index
        %79 = "arith.muli"(%78, %1) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
        %80 = "arith.addi"(%79, %55) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
        %81 = "memref.reinterpret_cast"(%arg4, %80) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 128, 64>, static_strides = array<i64: 64, 1>}> : (memref<?xbf16>, index) -> memref<128x64xbf16, strided<[64, 1], offset: ?>>
        %82 = "arith.index_cast"(%arg19) : (i32) -> index
        %83 = "arith.muli"(%82, %1) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
        %84 = "arith.addi"(%83, %55) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
        %85 = "memref.reinterpret_cast"(%arg5, %84) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 128, 64>, static_strides = array<i64: 64, 1>}> : (memref<?xbf16>, index) -> memref<128x64xbf16, strided<[64, 1], offset: ?>>
        %86 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<128x64xbf16>
        "hivm.hir.load"(%81, %86) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<128x64xbf16, strided<[64, 1], offset: ?>>, memref<128x64xbf16>) -> ()
        %87 = "bufferization.to_tensor"(%86) <{restrict, writable}> : (memref<128x64xbf16>) -> tensor<128x64xbf16>
        %88 = "tensor.empty"() : () -> tensor<64x128xbf16>
        %89 = "tensor.empty"() : () -> tensor<64x128xf32>
        %90 = "hivm.hir.mmadL1"(%64, %87, %0, %1, %1, %15, %89) <{b_transpose, operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_already_inserted = true} : (tensor<64x64xbf16>, tensor<128x64xbf16>, i1, index, index, index, tensor<64x128xf32>) -> tensor<64x128xf32>
        %91 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<64x128xf32>
        %92 = "bufferization.to_tensor"(%91) <{restrict, writable}> : (memref<64x128xf32>) -> tensor<64x128xf32>
        %93 = "hivm.hir.fixpipe"(%90, %92) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<64x128xf32>, tensor<64x128xf32>) -> tensor<64x128xf32>
        %94 = "tensor.empty"() : () -> tensor<64x128xf32>
        %95 = "hivm.hir.load"(%93, %94) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<64x128xf32>, tensor<64x128xf32>) -> tensor<64x128xf32>
        %96 = "hivm.hir.vmul"(%95, %arg9, %32) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x128xf32>, f32, tensor<64x128xf32>) -> tensor<64x128xf32>
        %97 = "tensor.empty"() : () -> tensor<64x1xf32>
        %98 = "hivm.hir.vreduce"(%96, %97) <{arith = #hivm.reduce_op<max>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 1>}> : (tensor<64x128xf32>, tensor<64x1xf32>) -> tensor<64x1xf32>
        %99 = "tensor.collapse_shape"(%98) <{reassociation = [[0, 1]]}> : (tensor<64x1xf32>) -> tensor<64xf32>
        %100 = "tensor.empty"() : () -> tensor<64xi1>
        %101 = "tensor.empty"() : () -> tensor<64xi1>
        %102 = "tensor.empty"() : () -> tensor<64xi1>
        %103 = "tensor.empty"() : () -> tensor<64xi1>
        %104 = "hivm.hir.vcmp"(%arg17, %arg17, %103) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>, tensor<64xi1>) -> tensor<64xi1>
        %105 = "hivm.hir.vnot"(%104, %102) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<64xi1>, tensor<64xi1>) -> tensor<64xi1>
        %106 = "hivm.hir.vcmp"(%99, %99, %101) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>, tensor<64xi1>) -> tensor<64xi1>
        %107 = "hivm.hir.vnot"(%106, %100) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<64xi1>, tensor<64xi1>) -> tensor<64xi1>
        %108 = "hivm.hir.vmax"(%arg17, %99, %41) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
        %109 = "hivm.hir.vsel"(%105, %99, %108, %40) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<64xi1>, tensor<64xf32>, tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
        %110 = "hivm.hir.vsel"(%107, %arg17, %109, %39) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<64xi1>, tensor<64xf32>, tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
        %111 = "tensor.expand_shape"(%110) <{reassociation = [[0, 1]], static_output_shape = array<i64: 64, 1>}> : (tensor<64xf32>) -> tensor<64x1xf32>
        %112 = "hivm.hir.vbrc"(%111, %31) <{broadcast_dims = array<i64: 1>}> : (tensor<64x1xf32>, tensor<64x128xf32>) -> tensor<64x128xf32>
        %113 = "hivm.hir.vsub"(%96, %112, %30) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x128xf32>, tensor<64x128xf32>, tensor<64x128xf32>) -> tensor<64x128xf32>
        %114 = "hivm.hir.vexp"(%113, %29) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<64x128xf32>, tensor<64x128xf32>) -> tensor<64x128xf32>
        %115 = "hivm.hir.vcast"(%114, %88) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<64x128xf32>, tensor<64x128xbf16>) -> tensor<64x128xbf16>
        %116 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<64x128xbf16>
        %117 = "bufferization.to_tensor"(%116) <{restrict, writable}> : (memref<64x128xbf16>) -> tensor<64x128xbf16>
        %118 = "hivm.hir.store"(%115, %117) {"inserted-store"} : (tensor<64x128xbf16>, tensor<64x128xbf16>) -> tensor<64x128xbf16>
        %119 = "tensor.empty"() : () -> tensor<64x128xbf16>
        %120 = "hivm.hir.load"(%118, %119) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<64x128xbf16>, tensor<64x128xbf16>) -> tensor<64x128xbf16>
        %121 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<128x64xbf16>
        "hivm.hir.load"(%85, %121) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<128x64xbf16, strided<[64, 1], offset: ?>>, memref<128x64xbf16>) -> ()
        %122 = "bufferization.to_tensor"(%121) <{restrict, writable}> : (memref<128x64xbf16>) -> tensor<128x64xbf16>
        %123 = "tensor.empty"() : () -> tensor<64x1xf32>
        %124 = "hivm.hir.vreduce"(%114, %123) <{arith = #hivm.reduce_op<sum>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 1>}> : (tensor<64x128xf32>, tensor<64x1xf32>) -> tensor<64x1xf32>
        %125 = "tensor.collapse_shape"(%124) <{reassociation = [[0, 1]]}> : (tensor<64x1xf32>) -> tensor<64xf32>
        %126 = "hivm.hir.vsub"(%arg17, %110, %38) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
        %127 = "hivm.hir.vexp"(%126, %37) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
        %128 = "hivm.hir.vmul"(%arg16, %127, %36) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
        %129 = "hivm.hir.vadd"(%128, %125, %35) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
        %130 = "tensor.expand_shape"(%127) <{reassociation = [[0, 1]], static_output_shape = array<i64: 64, 1>}> : (tensor<64xf32>) -> tensor<64x1xf32>
        %131 = "hivm.hir.vbrc"(%130, %26) <{broadcast_dims = array<i64: 1>}> : (tensor<64x1xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
        %132 = "hivm.hir.vmul"(%arg15, %131, %25) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x64xf32>, tensor<64x64xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
        %133 = "tensor.empty"() : () -> tensor<64x64xf32>
        %134 = "hivm.hir.mmadL1"(%120, %122, %0, %1, %15, %1, %133) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_already_inserted = true} : (tensor<64x128xbf16>, tensor<128x64xbf16>, i1, index, index, index, tensor<64x64xf32>) -> tensor<64x64xf32>
        %135 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<64x64xf32>
        %136 = "bufferization.to_tensor"(%135) <{restrict, writable}> : (memref<64x64xf32>) -> tensor<64x64xf32>
        %137 = "hivm.hir.fixpipe"(%134, %136) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<64x64xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
        %138 = "tensor.empty"() : () -> tensor<64x64xf32>
        %139 = "hivm.hir.load"(%137, %138) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<64x64xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
        %140 = "tensor.empty"() : () -> tensor<64x64xf32>
        %141 = "hivm.hir.vadd"(%139, %132, %140) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x64xf32>, tensor<64x64xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
        %142 = "arith.addi"(%arg18, %4) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
        %143 = "arith.addi"(%arg19, %4) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
        "scf.yield"(%141, %129, %110, %142, %143) : (tensor<64x64xf32>, tensor<64xf32>, tensor<64xf32>, i32, i32) -> ()
      }) : (i32, i32, i32, tensor<64x64xf32>, tensor<64xf32>, tensor<64xf32>, i32, i32) -> (tensor<64x64xf32>, tensor<64xf32>, tensor<64xf32>, i32, i32)
      %66 = "hivm.hir.vln"(%65#1, %34) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
      %67 = "hivm.hir.vadd"(%65#2, %66, %33) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
      %68 = "tensor.expand_shape"(%65#1) <{reassociation = [[0, 1]], static_output_shape = array<i64: 64, 1>}> : (tensor<64xf32>) -> tensor<64x1xf32>
      %69 = "hivm.hir.vbrc"(%68, %24) <{broadcast_dims = array<i64: 1>}> : (tensor<64x1xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
      %70 = "hivm.hir.vdiv"(%65#0, %69, %23) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x64xf32>, tensor<64x64xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
      %71 = "arith.muli"(%46, %11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %72 = "arith.index_cast"(%71) : (i32) -> index
      %73 = "arith.index_cast"(%56) : (i32) -> index
      %74 = "arith.addi"(%72, %73) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %75 = "memref.reinterpret_cast"(%arg6, %74) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<64xf32, strided<[1], offset: ?>>
      "hivm.hir.store"(%67, %75) : (tensor<64xf32>, memref<64xf32, strided<[1], offset: ?>>) -> ()
      %76 = "tensor.empty"() : () -> tensor<64x64xbf16>
      %77 = "hivm.hir.vcast"(%70, %76) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<64x64xf32>, tensor<64x64xbf16>) -> tensor<64x64xbf16>
      "hivm.hir.store"(%77, %62) : (tensor<64x64xbf16>, memref<64x64xbf16, strided<[64, 1], offset: ?>>) -> ()
      "scf.yield"() : () -> ()
    }) : (i32, i32, i32) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, true, true, true, false, false, false, false]> : vector<13xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<MIX>, mix_mode = "mix", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<MIX>} : () -> ()

