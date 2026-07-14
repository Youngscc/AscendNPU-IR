"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 2 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 2 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {}, {}, {}, {}, {}, {}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, memref<?xi32>, memref<?xf32>, memref<?xbf16>, memref<?xbf16>, memref<?xf32>, memref<?xbf16>, memref<?xbf16>, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, f32, i32, i32, i32, i32, i32, i32, i32, i32) -> (), sym_name = "sparse_flash_attention_prefill_kernel_cvpipe"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xbf16>, %arg4: memref<?xbf16>, %arg5: memref<?xbf16>, %arg6: memref<?xi32>, %arg7: memref<?xf32>, %arg8: memref<?xbf16>, %arg9: memref<?xbf16>, %arg10: memref<?xf32>, %arg11: memref<?xbf16>, %arg12: memref<?xbf16>, %arg13: i32, %arg14: i32, %arg15: i32, %arg16: i32, %arg17: i32, %arg18: i32, %arg19: i32, %arg20: i32, %arg21: i32, %arg22: i32, %arg23: i32, %arg24: i32, %arg25: i32, %arg26: i32, %arg27: i32, %arg28: i32, %arg29: i32, %arg30: i32, %arg31: i32, %arg32: i32, %arg33: i32, %arg34: i32, %arg35: i32, %arg36: f32, %arg37: i32, %arg38: i32, %arg39: i32, %arg40: i32, %arg41: i32, %arg42: i32, %arg43: i32, %arg44: i32):
    %0 = "arith.constant"() <{value = 1 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = false}> : () -> i1
    %2 = "arith.constant"() <{value = true}> : () -> i1
    %3 = "arith.constant"() <{value = 512 : index}> : () -> index
    %4 = "arith.constant"() <{value = 4 : i32}> : () -> i32
    %5 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %6 = "arith.constant"() <{value = 0.000000e+00 : f32}> : () -> f32
    %7 = "arith.constant"() <{value = 1024 : index}> : () -> index
    %8 = "arith.constant"() <{value = 256 : index}> : () -> index
    %9 = "arith.constant"() <{value = 256 : i32}> : () -> i32
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
    %20 = "tensor.empty"() : () -> tensor<16x256xf32>
    %21 = "tensor.empty"() : () -> tensor<16x256xf32>
    %22 = "tensor.empty"() : () -> tensor<16x256xf32>
    %23 = "tensor.empty"() : () -> tensor<16x256xf32>
    %24 = "tensor.empty"() : () -> tensor<16x256xf32>
    %25 = "tensor.empty"() : () -> tensor<16x256xf32>
    %26 = "tensor.empty"() : () -> tensor<16x256xf32>
    %27 = "hivm.hir.vbrc"(%10, %26) <{broadcast_dims = array<i64>}> : (f32, tensor<16x256xf32>) -> tensor<16x256xf32>
    %28 = "tensor.empty"() : () -> tensor<16x512xf32>
    %29 = "tensor.empty"() : () -> tensor<16x512xf32>
    %30 = "tensor.empty"() : () -> tensor<16x512xf32>
    %31 = "tensor.empty"() : () -> tensor<16x512xf32>
    %32 = "tensor.empty"() : () -> tensor<16x512xf32>
    %33 = "tensor.empty"() : () -> tensor<16x512xf32>
    %34 = "tensor.empty"() : () -> tensor<16x512xf32>
    %35 = "hivm.hir.vbrc"(%6, %34) <{broadcast_dims = array<i64>}> : (f32, tensor<16x512xf32>) -> tensor<16x512xf32>
    %36 = "tensor.empty"() : () -> tensor<16xf32>
    %37 = "tensor.empty"() : () -> tensor<16xf32>
    %38 = "tensor.empty"() : () -> tensor<16xf32>
    %39 = "tensor.empty"() : () -> tensor<16xf32>
    %40 = "tensor.empty"() : () -> tensor<16xf32>
    %41 = "tensor.empty"() : () -> tensor<16xf32>
    %42 = "tensor.empty"() : () -> tensor<16xf32>
    %43 = "tensor.empty"() : () -> tensor<16xf32>
    %44 = "tensor.empty"() : () -> tensor<16xf32>
    %45 = "tensor.empty"() : () -> tensor<16xf32>
    %46 = "tensor.empty"() : () -> tensor<16xf32>
    %47 = "tensor.empty"() : () -> tensor<16xf32>
    %48 = "tensor.empty"() : () -> tensor<16xf32>
    %49 = "tensor.empty"() : () -> tensor<16xf32>
    %50 = "tensor.empty"() : () -> tensor<16xf32>
    %51 = "tensor.empty"() : () -> tensor<16xf32>
    %52 = "tensor.empty"() : () -> tensor<16xf32>
    %53 = "hivm.hir.vbrc"(%10, %52) <{broadcast_dims = array<i64>}> : (f32, tensor<16xf32>) -> tensor<16xf32>
    %54 = "arith.muli"(%19, %arg41) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %55 = "arith.addi"(%19, %0) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %56 = "arith.muli"(%55, %arg41) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %57 = "arith.minsi"(%arg37, %56) : (i32, i32) -> i32
    %58 = "arith.muli"(%19, %arg32) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %59 = "arith.muli"(%19, %arg34) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %60 = "arith.index_cast"(%58) : (i32) -> index
    %61 = "arith.index_cast"(%arg33) : (i32) -> index
    %62 = "memref.reinterpret_cast"(%arg11, %60, %61) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 256>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<16x256xbf16, strided<[?, 1], offset: ?>>
    %63 = "arith.index_cast"(%59) : (i32) -> index
    %64 = "arith.index_cast"(%arg35) : (i32) -> index
    %65 = "memref.reinterpret_cast"(%arg12, %63, %64) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 512>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<16x512xbf16, strided<[?, 1], offset: ?>>
    "scf.for"(%54, %57, %0) ({
    ^bb0(%arg45: i32):
      %66 = "arith.muli"(%arg45, %arg13) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %67 = "arith.muli"(%arg45, %arg24) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %68 = "arith.muli"(%arg45, %arg27) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %69 = "arith.muli"(%arg45, %arg22) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %70 = "arith.index_cast"(%66) : (i32) -> index
      %71 = "arith.index_cast"(%arg15) : (i32) -> index
      %72 = "memref.reinterpret_cast"(%arg3, %70, %71) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 512>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<16x512xbf16, strided<[?, 1], offset: ?>>
      %73 = "arith.addi"(%70, %3) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %74 = "memref.reinterpret_cast"(%arg3, %73, %71) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 64>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<16x64xbf16, strided<[?, 1], offset: ?>>
      %75 = "arith.index_cast"(%67) : (i32) -> index
      %76:2 = "scf.for"(%5, %4, %0, %53, %35) ({
      ^bb0(%arg46: i32, %arg47: tensor<16xf32>, %arg48: tensor<16x512xf32>):
        %80 = "arith.muli"(%arg46, %9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
        %81 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x512xbf16>
        "hivm.hir.load"(%72, %81) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<16x512xbf16, strided<[?, 1], offset: ?>>, memref<16x512xbf16>) -> ()
        %82 = "bufferization.to_tensor"(%81) <{restrict, writable}> : (memref<16x512xbf16>) -> tensor<16x512xbf16>
        %83 = "arith.index_cast"(%80) : (i32) -> index
        %84 = "arith.index_cast"(%arg26) : (i32) -> index
        %85 = "arith.muli"(%83, %84) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
        %86 = "arith.addi"(%75, %85) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
        %87 = "memref.reinterpret_cast"(%arg8, %86, %84) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 256, 512>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<256x512xbf16, strided<[?, 1], offset: ?>>
        %88 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<256x512xbf16>
        "hivm.hir.load"(%87, %88) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<256x512xbf16, strided<[?, 1], offset: ?>>, memref<256x512xbf16>) -> ()
        %89 = "bufferization.to_tensor"(%88) <{restrict, writable}> : (memref<256x512xbf16>) -> tensor<256x512xbf16>
        %90 = "tensor.empty"() : () -> tensor<16x256xf32>
        %91 = "hivm.hir.mmadL1"(%82, %89, %2, %11, %3, %8, %90) <{b_transpose, operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> : (tensor<16x512xbf16>, tensor<256x512xbf16>, i1, index, index, index, tensor<16x256xf32>) -> tensor<16x256xf32>
        %92 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x64xbf16>
        "hivm.hir.load"(%74, %92) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<16x64xbf16, strided<[?, 1], offset: ?>>, memref<16x64xbf16>) -> ()
        %93 = "bufferization.to_tensor"(%92) <{restrict, writable}> : (memref<16x64xbf16>) -> tensor<16x64xbf16>
        %94 = "arith.index_cast"(%68) : (i32) -> index
        %95 = "arith.index_cast"(%arg29) : (i32) -> index
        %96 = "arith.muli"(%83, %95) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
        %97 = "arith.addi"(%94, %96) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
        %98 = "memref.reinterpret_cast"(%arg9, %97, %95) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 256, 64>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<256x64xbf16, strided<[?, 1], offset: ?>>
        %99 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<256x64xbf16>
        "hivm.hir.load"(%98, %99) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<256x64xbf16, strided<[?, 1], offset: ?>>, memref<256x64xbf16>) -> ()
        %100 = "bufferization.to_tensor"(%99) <{restrict, writable}> : (memref<256x64xbf16>) -> tensor<256x64xbf16>
        %101 = "hivm.hir.mmadL1"(%93, %100, %1, %11, %12, %8, %91) <{b_transpose, operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_already_inserted = true} : (tensor<16x64xbf16>, tensor<256x64xbf16>, i1, index, index, index, tensor<16x256xf32>) -> tensor<16x256xf32>
        %102 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x256xf32>
        %103 = "bufferization.to_tensor"(%102) <{restrict, writable}> : (memref<16x256xf32>) -> tensor<16x256xf32>
        %104 = "hivm.hir.fixpipe"(%101, %103) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<16x256xf32>, tensor<16x256xf32>) -> tensor<16x256xf32>
        %105 = "tensor.empty"() : () -> tensor<16x256xf32>
        %106 = "hivm.hir.load"(%104, %105) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<16x256xf32>, tensor<16x256xf32>) -> tensor<16x256xf32>
        %107 = "hivm.hir.vmul"(%106, %arg36, %25) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x256xf32>, f32, tensor<16x256xf32>) -> tensor<16x256xf32>
        %108 = "arith.addi"(%83, %8) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
        %109 = "arith.maxsi"(%83, %7) : (index, index) -> index
        %110 = "arith.minsi"(%108, %109) : (index, index) -> index
        %111 = "arith.subi"(%110, %83) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
        %112 = "tensor.extract_slice"(%107, %111) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: 16, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<16x256xf32>, index) -> tensor<16x?xf32>
        %113 = "tensor.insert_slice"(%112, %27, %111) <{operandSegmentSizes = array<i32: 1, 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: 16, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<16x?xf32>, tensor<16x256xf32>, index) -> tensor<16x256xf32>
        %114 = "tensor.empty"() : () -> tensor<16x1xf32>
        %115 = "hivm.hir.vreduce"(%113, %114) <{arith = #hivm.reduce_op<max>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 1>}> : (tensor<16x256xf32>, tensor<16x1xf32>) -> tensor<16x1xf32>
        %116 = "tensor.collapse_shape"(%115) <{reassociation = [[0, 1]]}> : (tensor<16x1xf32>) -> tensor<16xf32>
        %117 = "hivm.hir.vbrc"(%115, %24) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xf32>, tensor<16x256xf32>) -> tensor<16x256xf32>
        %118 = "hivm.hir.vsub"(%113, %117, %23) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x256xf32>, tensor<16x256xf32>, tensor<16x256xf32>) -> tensor<16x256xf32>
        %119 = "hivm.hir.vexp"(%118, %22) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16x256xf32>, tensor<16x256xf32>) -> tensor<16x256xf32>
        %120 = "tensor.empty"() : () -> tensor<16x1xf32>
        %121 = "hivm.hir.vreduce"(%119, %120) <{arith = #hivm.reduce_op<sum>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 1>}> : (tensor<16x256xf32>, tensor<16x1xf32>) -> tensor<16x1xf32>
        %122 = "tensor.collapse_shape"(%121) <{reassociation = [[0, 1]]}> : (tensor<16x1xf32>) -> tensor<16xf32>
        %123 = "hivm.hir.vln"(%122, %51) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %124 = "hivm.hir.vadd"(%116, %123, %50) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %125 = "tensor.empty"() : () -> tensor<16xi1>
        %126 = "tensor.empty"() : () -> tensor<16xi1>
        %127 = "tensor.empty"() : () -> tensor<16xi1>
        %128 = "tensor.empty"() : () -> tensor<16xi1>
        %129 = "hivm.hir.vcmp"(%arg47, %arg47, %128) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xi1>) -> tensor<16xi1>
        %130 = "hivm.hir.vnot"(%129, %127) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xi1>, tensor<16xi1>) -> tensor<16xi1>
        %131 = "hivm.hir.vcmp"(%124, %124, %126) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xi1>) -> tensor<16xi1>
        %132 = "hivm.hir.vnot"(%131, %125) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xi1>, tensor<16xi1>) -> tensor<16xi1>
        %133 = "hivm.hir.vmax"(%arg47, %124, %49) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %134 = "hivm.hir.vsel"(%130, %124, %133, %48) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<16xi1>, tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %135 = "hivm.hir.vsel"(%132, %arg47, %134, %47) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<16xi1>, tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %136 = "hivm.hir.vsub"(%arg47, %135, %46) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %137 = "hivm.hir.vexp"(%136, %45) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %138 = "hivm.hir.vsub"(%124, %135, %44) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %139 = "hivm.hir.vexp"(%138, %43) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %140 = "hivm.hir.vadd"(%137, %139, %42) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %141 = "hivm.hir.vln"(%140, %41) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %142 = "hivm.hir.vadd"(%135, %141, %40) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %143 = "hivm.hir.vsub"(%124, %142, %39) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %144 = "hivm.hir.vexp"(%143, %38) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %145 = "hivm.hir.vbrc"(%121, %21) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xf32>, tensor<16x256xf32>) -> tensor<16x256xf32>
        %146 = "hivm.hir.vdiv"(%119, %145, %20) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x256xf32>, tensor<16x256xf32>, tensor<16x256xf32>) -> tensor<16x256xf32>
        %147 = "tensor.empty"() : () -> tensor<16x256xbf16>
        %148 = "hivm.hir.vcast"(%146, %147) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x256xf32>, tensor<16x256xbf16>) -> tensor<16x256xbf16>
        "hivm.hir.store"(%148, %62) : (tensor<16x256xbf16>, memref<16x256xbf16, strided<[?, 1], offset: ?>>) -> ()
        %149 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x256xbf16>
        "hivm.hir.load"(%62, %149) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<16x256xbf16, strided<[?, 1], offset: ?>>, memref<16x256xbf16>) -> ()
        %150 = "bufferization.to_tensor"(%149) <{restrict, writable}> : (memref<16x256xbf16>) -> tensor<16x256xbf16>
        %151 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<256x512xbf16>
        "hivm.hir.load"(%87, %151) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<256x512xbf16, strided<[?, 1], offset: ?>>, memref<256x512xbf16>) -> ()
        %152 = "bufferization.to_tensor"(%151) <{restrict, writable}> : (memref<256x512xbf16>) -> tensor<256x512xbf16>
        %153 = "tensor.empty"() : () -> tensor<16x512xf32>
        %154 = "hivm.hir.mmadL1"(%150, %152, %2, %11, %8, %3, %153) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_already_inserted = true} : (tensor<16x256xbf16>, tensor<256x512xbf16>, i1, index, index, index, tensor<16x512xf32>) -> tensor<16x512xf32>
        "hivm.hir.fixpipe"(%154, %65) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, pre_quant = #hivm.fixpipe_pre_quant_mode<F322BF16>}> : (tensor<16x512xf32>, memref<16x512xbf16, strided<[?, 1], offset: ?>>) -> ()
        %155 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x512xbf16>
        "hivm.hir.load"(%65, %155) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<16x512xbf16, strided<[?, 1], offset: ?>>, memref<16x512xbf16>) -> ()
        %156 = "bufferization.to_tensor"(%155) <{restrict, writable}> : (memref<16x512xbf16>) -> tensor<16x512xbf16>
        %157 = "hivm.hir.vsub"(%arg47, %142, %37) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %158 = "hivm.hir.vexp"(%157, %36) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %159 = "tensor.expand_shape"(%158) <{reassociation = [[0, 1]], static_output_shape = array<i64: 16, 1>}> : (tensor<16xf32>) -> tensor<16x1xf32>
        %160 = "hivm.hir.vbrc"(%159, %33) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xf32>, tensor<16x512xf32>) -> tensor<16x512xf32>
        %161 = "hivm.hir.vmul"(%arg48, %160, %32) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x512xf32>, tensor<16x512xf32>, tensor<16x512xf32>) -> tensor<16x512xf32>
        %162 = "hivm.hir.vcast"(%156, %31) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x512xbf16>, tensor<16x512xf32>) -> tensor<16x512xf32>
        %163 = "tensor.expand_shape"(%144) <{reassociation = [[0, 1]], static_output_shape = array<i64: 16, 1>}> : (tensor<16xf32>) -> tensor<16x1xf32>
        %164 = "hivm.hir.vbrc"(%163, %30) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xf32>, tensor<16x512xf32>) -> tensor<16x512xf32>
        %165 = "hivm.hir.vmul"(%162, %164, %29) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x512xf32>, tensor<16x512xf32>, tensor<16x512xf32>) -> tensor<16x512xf32>
        %166 = "hivm.hir.vadd"(%161, %165, %28) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x512xf32>, tensor<16x512xf32>, tensor<16x512xf32>) -> tensor<16x512xf32>
        "scf.yield"(%142, %166) : (tensor<16xf32>, tensor<16x512xf32>) -> ()
      }) : (i32, i32, i32, tensor<16xf32>, tensor<16x512xf32>) -> (tensor<16xf32>, tensor<16x512xf32>)
      %77 = "arith.index_cast"(%69) : (i32) -> index
      %78 = "arith.index_cast"(%arg23) : (i32) -> index
      %79 = "memref.reinterpret_cast"(%arg7, %77, %78) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 512>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xf32>, index, index) -> memref<16x512xf32, strided<[?, 1], offset: ?>>
      "hivm.hir.store"(%76#1, %79) : (tensor<16x512xf32>, memref<16x512xf32, strided<[?, 1], offset: ?>>) -> ()
      "scf.yield"() : () -> ()
    }) : (i32, i32, i32) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false]> : vector<45xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<MIX>, mix_mode = "mix", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<MIX>} : () -> ()

