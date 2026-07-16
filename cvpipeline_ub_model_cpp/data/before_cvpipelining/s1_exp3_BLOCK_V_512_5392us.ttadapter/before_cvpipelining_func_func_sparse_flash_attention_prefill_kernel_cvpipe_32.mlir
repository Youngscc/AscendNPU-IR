"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 2 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 2 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 2 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {}, {}, {}, {}, {}, {}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, memref<?xi32>, memref<?xf32>, memref<?xbf16>, memref<?xbf16>, memref<?xf32>, memref<?xbf16>, memref<?xf32>, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, f32, i32, i32, i32, i32, i32, i32, i32, i32) -> (), sym_name = "sparse_flash_attention_prefill_kernel_cvpipe"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xbf16>, %arg4: memref<?xbf16>, %arg5: memref<?xbf16>, %arg6: memref<?xi32>, %arg7: memref<?xf32>, %arg8: memref<?xbf16>, %arg9: memref<?xbf16>, %arg10: memref<?xf32>, %arg11: memref<?xbf16>, %arg12: memref<?xf32>, %arg13: i32, %arg14: i32, %arg15: i32, %arg16: i32, %arg17: i32, %arg18: i32, %arg19: i32, %arg20: i32, %arg21: i32, %arg22: i32, %arg23: i32, %arg24: i32, %arg25: i32, %arg26: i32, %arg27: i32, %arg28: i32, %arg29: i32, %arg30: i32, %arg31: i32, %arg32: i32, %arg33: i32, %arg34: i32, %arg35: i32, %arg36: f32, %arg37: i32, %arg38: i32, %arg39: i32, %arg40: i32, %arg41: i32, %arg42: i32, %arg43: i32, %arg44: i32):
    %0 = "arith.constant"() <{value = 1 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = false}> : () -> i1
    %2 = "arith.constant"() <{value = true}> : () -> i1
    %3 = "arith.constant"() <{value = 512 : index}> : () -> index
    %4 = "arith.constant"() <{value = 8 : i32}> : () -> i32
    %5 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %6 = "arith.constant"() <{value = 2.000000e+00 : f32}> : () -> f32
    %7 = "arith.constant"() <{value = 1024 : index}> : () -> index
    %8 = "arith.constant"() <{value = 128 : index}> : () -> index
    %9 = "arith.constant"() <{value = 128 : i32}> : () -> i32
    %10 = "arith.constant"() <{value = 0xFF800000 : f32}> : () -> f32
    %11 = "arith.constant"() <{value = 16 : index}> : () -> index
    %12 = "arith.constant"() <{value = 64 : index}> : () -> index
    "hivm.hir.set_mask_norm"() : () -> ()
    %13 = "arith.muli"(%arg42, %arg43) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %14 = "arith.muli"(%13, %arg44) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%14) {logical_block_num} : (i32) -> ()
    %15 = "hivm.hir.get_block_idx"() : () -> i64
    %16 = "arith.trunci"(%15) : (i64) -> i32
    %17 = "arith.muli"(%arg44, %arg43) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %18 = "arith.divsi"(%16, %17) : (i32, i32) -> i32
    %19 = "arith.remsi"(%18, %arg42) : (i32, i32) -> i32
    %20 = "tensor.empty"() : () -> tensor<16x128xf32>
    %21 = "tensor.empty"() : () -> tensor<16x128xf32>
    %22 = "tensor.empty"() : () -> tensor<16x128xf32>
    %23 = "tensor.empty"() : () -> tensor<16x128xf32>
    %24 = "tensor.empty"() : () -> tensor<16x128xf32>
    %25 = "tensor.empty"() : () -> tensor<16x128xf32>
    %26 = "tensor.empty"() : () -> tensor<16x128xf32>
    %27 = "hivm.hir.vbrc"(%10, %26) <{broadcast_dims = array<i64>}> : (f32, tensor<16x128xf32>) -> tensor<16x128xf32>
    %28 = "tensor.empty"() : () -> tensor<16xf32>
    %29 = "tensor.empty"() : () -> tensor<16xf32>
    %30 = "tensor.empty"() : () -> tensor<16xf32>
    %31 = "tensor.empty"() : () -> tensor<16xf32>
    %32 = "tensor.empty"() : () -> tensor<16xf32>
    %33 = "tensor.empty"() : () -> tensor<16xf32>
    %34 = "tensor.empty"() : () -> tensor<16xf32>
    %35 = "tensor.empty"() : () -> tensor<16xf32>
    %36 = "tensor.empty"() : () -> tensor<16xf32>
    %37 = "tensor.empty"() : () -> tensor<16xf32>
    %38 = "tensor.empty"() : () -> tensor<16xf32>
    %39 = "tensor.empty"() : () -> tensor<16xf32>
    %40 = "tensor.empty"() : () -> tensor<16xf32>
    %41 = "tensor.empty"() : () -> tensor<16xf32>
    %42 = "tensor.empty"() : () -> tensor<16xf32>
    %43 = "tensor.empty"() : () -> tensor<16xf32>
    %44 = "tensor.empty"() : () -> tensor<16xf32>
    %45 = "tensor.empty"() : () -> tensor<16x512xf32>
    %46 = "tensor.empty"() : () -> tensor<16x512xf32>
    %47 = "tensor.empty"() : () -> tensor<16x512xf32>
    %48 = "tensor.empty"() : () -> tensor<16x512xf32>
    %49 = "tensor.empty"() : () -> tensor<16x512xf32>
    %50 = "hivm.hir.vbrc"(%10, %44) <{broadcast_dims = array<i64>}> : (f32, tensor<16xf32>) -> tensor<16xf32>
    %51 = "arith.muli"(%19, %arg41) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %52 = "arith.addi"(%19, %0) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %53 = "arith.muli"(%52, %arg41) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %54 = "arith.minsi"(%arg37, %53) : (i32, i32) -> i32
    %55 = "arith.muli"(%19, %arg32) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %56 = "arith.muli"(%19, %arg34) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %57 = "arith.index_cast"(%55) : (i32) -> index
    %58 = "arith.index_cast"(%arg33) : (i32) -> index
    %59 = "memref.reinterpret_cast"(%arg11, %57, %58) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 128>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<16x128xbf16, strided<[?, 1], offset: ?>>
    %60 = "arith.index_cast"(%56) : (i32) -> index
    %61 = "arith.index_cast"(%arg35) : (i32) -> index
    %62 = "memref.reinterpret_cast"(%arg12, %60, %61) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 512>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xf32>, index, index) -> memref<16x512xf32, strided<[?, 1], offset: ?>>
    "scf.for"(%51, %54, %0) ({
    ^bb0(%arg45: i32):
      %63 = "arith.muli"(%arg45, %arg13) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %64 = "arith.muli"(%arg45, %arg24) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %65 = "arith.muli"(%arg45, %arg27) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %66 = "arith.muli"(%arg45, %arg22) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %67 = "arith.index_cast"(%63) : (i32) -> index
      %68 = "arith.index_cast"(%arg15) : (i32) -> index
      %69 = "memref.reinterpret_cast"(%arg3, %67, %68) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 512>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<16x512xbf16, strided<[?, 1], offset: ?>>
      %70 = "arith.addi"(%67, %3) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %71 = "memref.reinterpret_cast"(%arg3, %70, %68) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 64>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<16x64xbf16, strided<[?, 1], offset: ?>>
      %72 = "arith.index_cast"(%64) : (i32) -> index
      %73 = "arith.index_cast"(%66) : (i32) -> index
      %74 = "arith.index_cast"(%arg23) : (i32) -> index
      %75 = "memref.reinterpret_cast"(%arg7, %73, %74) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 512>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xf32>, index, index) -> memref<16x512xf32, strided<[?, 1], offset: ?>>
      %76 = "scf.for"(%5, %4, %0, %50) ({
      ^bb0(%arg46: i32, %arg47: tensor<16xf32>):
        %77 = "arith.muli"(%arg46, %9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
        %78 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x512xbf16>
        "hivm.hir.load"(%69, %78) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<16x512xbf16, strided<[?, 1], offset: ?>>, memref<16x512xbf16>) -> ()
        %79 = "bufferization.to_tensor"(%78) <{restrict, writable}> : (memref<16x512xbf16>) -> tensor<16x512xbf16>
        %80 = "arith.index_cast"(%77) : (i32) -> index
        %81 = "arith.index_cast"(%arg26) : (i32) -> index
        %82 = "arith.muli"(%80, %81) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
        %83 = "arith.addi"(%72, %82) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
        %84 = "memref.reinterpret_cast"(%arg8, %83, %81) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 128, 512>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<128x512xbf16, strided<[?, 1], offset: ?>>
        %85 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<128x512xbf16>
        "hivm.hir.load"(%84, %85) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<128x512xbf16, strided<[?, 1], offset: ?>>, memref<128x512xbf16>) -> ()
        %86 = "bufferization.to_tensor"(%85) <{restrict, writable}> : (memref<128x512xbf16>) -> tensor<128x512xbf16>
        %87 = "tensor.empty"() : () -> tensor<16x128xf32>
        %88 = "hivm.hir.mmadL1"(%79, %86, %2, %11, %3, %8, %87) <{b_transpose, operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> : (tensor<16x512xbf16>, tensor<128x512xbf16>, i1, index, index, index, tensor<16x128xf32>) -> tensor<16x128xf32>
        %89 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x64xbf16>
        "hivm.hir.load"(%71, %89) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<16x64xbf16, strided<[?, 1], offset: ?>>, memref<16x64xbf16>) -> ()
        %90 = "bufferization.to_tensor"(%89) <{restrict, writable}> : (memref<16x64xbf16>) -> tensor<16x64xbf16>
        %91 = "arith.index_cast"(%65) : (i32) -> index
        %92 = "arith.index_cast"(%arg29) : (i32) -> index
        %93 = "arith.muli"(%80, %92) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
        %94 = "arith.addi"(%91, %93) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
        %95 = "memref.reinterpret_cast"(%arg9, %94, %92) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 128, 64>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<128x64xbf16, strided<[?, 1], offset: ?>>
        %96 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<128x64xbf16>
        "hivm.hir.load"(%95, %96) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<128x64xbf16, strided<[?, 1], offset: ?>>, memref<128x64xbf16>) -> ()
        %97 = "bufferization.to_tensor"(%96) <{restrict, writable}> : (memref<128x64xbf16>) -> tensor<128x64xbf16>
        %98 = "hivm.hir.mmadL1"(%90, %97, %1, %11, %12, %8, %88) <{b_transpose, operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_already_inserted = true} : (tensor<16x64xbf16>, tensor<128x64xbf16>, i1, index, index, index, tensor<16x128xf32>) -> tensor<16x128xf32>
        %99 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x128xf32>
        %100 = "bufferization.to_tensor"(%99) <{restrict, writable}> : (memref<16x128xf32>) -> tensor<16x128xf32>
        %101 = "hivm.hir.fixpipe"(%98, %100) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<16x128xf32>, tensor<16x128xf32>) -> tensor<16x128xf32>
        %102 = "tensor.empty"() : () -> tensor<16x128xf32>
        %103 = "hivm.hir.load"(%101, %102) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<16x128xf32>, tensor<16x128xf32>) -> tensor<16x128xf32>
        %104 = "hivm.hir.vmul"(%103, %arg36, %25) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x128xf32>, f32, tensor<16x128xf32>) -> tensor<16x128xf32>
        %105 = "arith.addi"(%80, %8) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
        %106 = "arith.maxsi"(%80, %7) : (index, index) -> index
        %107 = "arith.minsi"(%105, %106) : (index, index) -> index
        %108 = "arith.subi"(%107, %80) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
        %109 = "tensor.extract_slice"(%104, %108) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: 16, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<16x128xf32>, index) -> tensor<16x?xf32>
        %110 = "tensor.insert_slice"(%109, %27, %108) <{operandSegmentSizes = array<i32: 1, 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: 16, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<16x?xf32>, tensor<16x128xf32>, index) -> tensor<16x128xf32>
        %111 = "tensor.empty"() : () -> tensor<16x1xf32>
        %112 = "hivm.hir.vreduce"(%110, %111) <{arith = #hivm.reduce_op<max>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 1>}> : (tensor<16x128xf32>, tensor<16x1xf32>) -> tensor<16x1xf32>
        %113 = "tensor.collapse_shape"(%112) <{reassociation = [[0, 1]]}> : (tensor<16x1xf32>) -> tensor<16xf32>
        %114 = "hivm.hir.vbrc"(%112, %24) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xf32>, tensor<16x128xf32>) -> tensor<16x128xf32>
        %115 = "hivm.hir.vsub"(%110, %114, %23) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x128xf32>, tensor<16x128xf32>, tensor<16x128xf32>) -> tensor<16x128xf32>
        %116 = "hivm.hir.vexp"(%115, %22) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16x128xf32>, tensor<16x128xf32>) -> tensor<16x128xf32>
        %117 = "tensor.empty"() : () -> tensor<16x1xf32>
        %118 = "hivm.hir.vreduce"(%116, %117) <{arith = #hivm.reduce_op<sum>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 1>}> : (tensor<16x128xf32>, tensor<16x1xf32>) -> tensor<16x1xf32>
        %119 = "tensor.collapse_shape"(%118) <{reassociation = [[0, 1]]}> : (tensor<16x1xf32>) -> tensor<16xf32>
        %120 = "hivm.hir.vln"(%119, %43) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %121 = "hivm.hir.vadd"(%113, %120, %42) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %122 = "hivm.hir.vadd"(%arg47, %121, %41) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %123 = "hivm.hir.vdiv"(%122, %6, %40) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, f32, tensor<16xf32>) -> tensor<16xf32>
        %124 = "hivm.hir.vsub"(%arg47, %123, %39) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %125 = "hivm.hir.vexp"(%124, %38) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %126 = "hivm.hir.vsub"(%121, %123, %37) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %127 = "hivm.hir.vexp"(%126, %36) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %128 = "hivm.hir.vadd"(%125, %127, %35) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %129 = "hivm.hir.vln"(%128, %34) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %130 = "hivm.hir.vadd"(%123, %129, %33) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %131 = "tensor.empty"() : () -> tensor<16xi1>
        %132 = "tensor.empty"() : () -> tensor<16xi1>
        %133 = "hivm.hir.vcmp"(%130, %130, %132) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xi1>) -> tensor<16xi1>
        %134 = "hivm.hir.vnot"(%133, %131) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xi1>, tensor<16xi1>) -> tensor<16xi1>
        %135 = "hivm.hir.vsel"(%134, %121, %130, %32) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<16xi1>, tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %136 = "hivm.hir.vsub"(%121, %135, %31) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %137 = "hivm.hir.vexp"(%136, %30) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %138 = "hivm.hir.vbrc"(%118, %21) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xf32>, tensor<16x128xf32>) -> tensor<16x128xf32>
        %139 = "hivm.hir.vdiv"(%116, %138, %20) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x128xf32>, tensor<16x128xf32>, tensor<16x128xf32>) -> tensor<16x128xf32>
        %140 = "tensor.empty"() : () -> tensor<16x128xbf16>
        %141 = "hivm.hir.vcast"(%139, %140) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x128xf32>, tensor<16x128xbf16>) -> tensor<16x128xbf16>
        "hivm.hir.store"(%141, %59) : (tensor<16x128xbf16>, memref<16x128xbf16, strided<[?, 1], offset: ?>>) -> ()
        %142 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x128xbf16>
        "hivm.hir.load"(%59, %142) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<16x128xbf16, strided<[?, 1], offset: ?>>, memref<16x128xbf16>) -> ()
        %143 = "bufferization.to_tensor"(%142) <{restrict, writable}> : (memref<16x128xbf16>) -> tensor<16x128xbf16>
        %144 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<128x512xbf16>
        "hivm.hir.load"(%84, %144) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<128x512xbf16, strided<[?, 1], offset: ?>>, memref<128x512xbf16>) -> ()
        %145 = "bufferization.to_tensor"(%144) <{restrict, writable}> : (memref<128x512xbf16>) -> tensor<128x512xbf16>
        %146 = "tensor.empty"() : () -> tensor<16x512xf32>
        %147 = "hivm.hir.mmadL1"(%143, %145, %2, %11, %8, %3, %146) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_already_inserted = true} : (tensor<16x128xbf16>, tensor<128x512xbf16>, i1, index, index, index, tensor<16x512xf32>) -> tensor<16x512xf32>
        "hivm.hir.fixpipe"(%147, %62) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<16x512xf32>, memref<16x512xf32, strided<[?, 1], offset: ?>>) -> ()
        %148 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x512xf32>
        "hivm.hir.load"(%75, %148) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<16x512xf32, strided<[?, 1], offset: ?>>, memref<16x512xf32>) -> ()
        %149 = "bufferization.to_tensor"(%148) <{restrict, writable}> : (memref<16x512xf32>) -> tensor<16x512xf32>
        %150 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x512xf32>
        "hivm.hir.load"(%62, %150) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<16x512xf32, strided<[?, 1], offset: ?>>, memref<16x512xf32>) -> ()
        %151 = "bufferization.to_tensor"(%150) <{restrict, writable}> : (memref<16x512xf32>) -> tensor<16x512xf32>
        %152 = "hivm.hir.vsub"(%arg47, %135, %29) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %153 = "hivm.hir.vexp"(%152, %28) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %154 = "tensor.expand_shape"(%153) <{reassociation = [[0, 1]], static_output_shape = array<i64: 16, 1>}> : (tensor<16xf32>) -> tensor<16x1xf32>
        %155 = "hivm.hir.vbrc"(%154, %49) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xf32>, tensor<16x512xf32>) -> tensor<16x512xf32>
        %156 = "hivm.hir.vmul"(%149, %155, %48) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x512xf32>, tensor<16x512xf32>, tensor<16x512xf32>) -> tensor<16x512xf32>
        %157 = "tensor.expand_shape"(%137) <{reassociation = [[0, 1]], static_output_shape = array<i64: 16, 1>}> : (tensor<16xf32>) -> tensor<16x1xf32>
        %158 = "hivm.hir.vbrc"(%157, %47) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xf32>, tensor<16x512xf32>) -> tensor<16x512xf32>
        %159 = "hivm.hir.vmul"(%151, %158, %46) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x512xf32>, tensor<16x512xf32>, tensor<16x512xf32>) -> tensor<16x512xf32>
        %160 = "hivm.hir.vadd"(%156, %159, %45) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x512xf32>, tensor<16x512xf32>, tensor<16x512xf32>) -> tensor<16x512xf32>
        "hivm.hir.store"(%160, %75) : (tensor<16x512xf32>, memref<16x512xf32, strided<[?, 1], offset: ?>>) -> ()
        "scf.yield"(%135) : (tensor<16xf32>) -> ()
      }) : (i32, i32, i32, tensor<16xf32>) -> tensor<16xf32>
      "scf.yield"() : () -> ()
    }) : (i32, i32, i32) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false]> : vector<45xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<MIX>, mix_mode = "mix", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<MIX>} : () -> ()

