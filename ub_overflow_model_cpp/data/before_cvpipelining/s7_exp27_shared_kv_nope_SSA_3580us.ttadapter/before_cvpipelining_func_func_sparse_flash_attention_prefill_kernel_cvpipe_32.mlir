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
    %27 = "tensor.empty"() : () -> tensor<16x256xf32>
    %28 = "tensor.empty"() : () -> tensor<16x256xf32>
    %29 = "hivm.hir.vbrc"(%10, %28) <{broadcast_dims = array<i64>}> : (f32, tensor<16x256xf32>) -> tensor<16x256xf32>
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
    %47 = "hivm.hir.vbrc"(%6, %46) <{broadcast_dims = array<i64>}> : (f32, tensor<16xf32>) -> tensor<16xf32>
    %48 = "hivm.hir.vbrc"(%10, %45) <{broadcast_dims = array<i64>}> : (f32, tensor<16xf32>) -> tensor<16xf32>
    %49 = "arith.muli"(%19, %arg41) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %50 = "arith.addi"(%19, %0) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %51 = "arith.muli"(%50, %arg41) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %52 = "arith.minsi"(%arg37, %51) : (i32, i32) -> i32
    %53 = "arith.muli"(%19, %arg32) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %54 = "arith.muli"(%19, %arg34) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %55 = "arith.index_cast"(%53) : (i32) -> index
    %56 = "arith.index_cast"(%arg33) : (i32) -> index
    %57 = "memref.reinterpret_cast"(%arg11, %55, %56) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 256>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<16x256xbf16, strided<[?, 1], offset: ?>>
    %58 = "arith.index_cast"(%54) : (i32) -> index
    %59 = "arith.index_cast"(%arg35) : (i32) -> index
    %60 = "memref.reinterpret_cast"(%arg12, %58, %59) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 512>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<16x512xbf16, strided<[?, 1], offset: ?>>
    "scf.for"(%49, %52, %0) ({
    ^bb0(%arg45: i32):
      %61 = "arith.muli"(%arg45, %arg13) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %62 = "arith.muli"(%arg45, %arg24) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %63 = "arith.muli"(%arg45, %arg27) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %64 = "arith.muli"(%arg45, %arg22) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %65 = "arith.index_cast"(%62) : (i32) -> index
      %66 = "arith.index_cast"(%61) : (i32) -> index
      %67 = "arith.index_cast"(%arg15) : (i32) -> index
      %68 = "memref.reinterpret_cast"(%arg3, %66, %67) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 512>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<16x512xbf16, strided<[?, 1], offset: ?>>
      %69 = "arith.addi"(%66, %3) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %70 = "memref.reinterpret_cast"(%arg3, %69, %67) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 64>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<16x64xbf16, strided<[?, 1], offset: ?>>
      %71:3 = "scf.for"(%5, %4, %0, %48, %47, %35) ({
      ^bb0(%arg46: i32, %arg47: tensor<16xf32>, %arg48: tensor<16xf32>, %arg49: tensor<16x512xf32>):
        %75 = "arith.muli"(%arg46, %9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
        %76 = "arith.index_cast"(%75) : (i32) -> index
        %77 = "arith.index_cast"(%arg26) : (i32) -> index
        %78 = "arith.muli"(%76, %77) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
        %79 = "arith.addi"(%65, %78) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
        %80 = "memref.reinterpret_cast"(%arg8, %79, %77) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 256, 512>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<256x512xbf16, strided<[?, 1], offset: ?>>
        %81 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<256x512xbf16>
        "hivm.hir.load"(%80, %81) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<256x512xbf16, strided<[?, 1], offset: ?>>, memref<256x512xbf16>) -> ()
        %82 = "bufferization.to_tensor"(%81) <{restrict, writable}> : (memref<256x512xbf16>) -> tensor<256x512xbf16>
        %83 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x512xbf16>
        "hivm.hir.load"(%68, %83) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<16x512xbf16, strided<[?, 1], offset: ?>>, memref<16x512xbf16>) -> ()
        %84 = "bufferization.to_tensor"(%83) <{restrict, writable}> : (memref<16x512xbf16>) -> tensor<16x512xbf16>
        %85 = "tensor.empty"() : () -> tensor<16x256xf32>
        %86 = "hivm.hir.mmadL1"(%84, %82, %2, %11, %3, %8, %85) <{b_transpose, operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> : (tensor<16x512xbf16>, tensor<256x512xbf16>, i1, index, index, index, tensor<16x256xf32>) -> tensor<16x256xf32>
        %87 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x64xbf16>
        "hivm.hir.load"(%70, %87) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<16x64xbf16, strided<[?, 1], offset: ?>>, memref<16x64xbf16>) -> ()
        %88 = "bufferization.to_tensor"(%87) <{restrict, writable}> : (memref<16x64xbf16>) -> tensor<16x64xbf16>
        %89 = "arith.index_cast"(%63) : (i32) -> index
        %90 = "arith.index_cast"(%arg29) : (i32) -> index
        %91 = "arith.muli"(%76, %90) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
        %92 = "arith.addi"(%89, %91) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
        %93 = "memref.reinterpret_cast"(%arg9, %92, %90) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 256, 64>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<256x64xbf16, strided<[?, 1], offset: ?>>
        %94 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<256x64xbf16>
        "hivm.hir.load"(%93, %94) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<256x64xbf16, strided<[?, 1], offset: ?>>, memref<256x64xbf16>) -> ()
        %95 = "bufferization.to_tensor"(%94) <{restrict, writable}> : (memref<256x64xbf16>) -> tensor<256x64xbf16>
        %96 = "hivm.hir.mmadL1"(%88, %95, %1, %11, %12, %8, %86) <{b_transpose, operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_already_inserted = true} : (tensor<16x64xbf16>, tensor<256x64xbf16>, i1, index, index, index, tensor<16x256xf32>) -> tensor<16x256xf32>
        %97 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x256xf32>
        %98 = "bufferization.to_tensor"(%97) <{restrict, writable}> : (memref<16x256xf32>) -> tensor<16x256xf32>
        %99 = "hivm.hir.fixpipe"(%96, %98) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<16x256xf32>, tensor<16x256xf32>) -> tensor<16x256xf32>
        %100 = "tensor.empty"() : () -> tensor<16x256xf32>
        %101 = "hivm.hir.load"(%99, %100) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<16x256xf32>, tensor<16x256xf32>) -> tensor<16x256xf32>
        %102 = "hivm.hir.vmul"(%101, %arg36, %27) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x256xf32>, f32, tensor<16x256xf32>) -> tensor<16x256xf32>
        %103 = "arith.addi"(%76, %8) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
        %104 = "arith.maxsi"(%76, %7) : (index, index) -> index
        %105 = "arith.minsi"(%103, %104) : (index, index) -> index
        %106 = "arith.subi"(%105, %76) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
        %107 = "tensor.extract_slice"(%102, %106) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: 16, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<16x256xf32>, index) -> tensor<16x?xf32>
        %108 = "tensor.insert_slice"(%107, %29, %106) <{operandSegmentSizes = array<i32: 1, 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: 16, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<16x?xf32>, tensor<16x256xf32>, index) -> tensor<16x256xf32>
        %109 = "tensor.empty"() : () -> tensor<16x1xf32>
        %110 = "hivm.hir.vreduce"(%108, %109) <{arith = #hivm.reduce_op<max>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 1>}> : (tensor<16x256xf32>, tensor<16x1xf32>) -> tensor<16x1xf32>
        %111 = "tensor.collapse_shape"(%110) <{reassociation = [[0, 1]]}> : (tensor<16x1xf32>) -> tensor<16xf32>
        %112 = "hivm.hir.vbrc"(%110, %26) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xf32>, tensor<16x256xf32>) -> tensor<16x256xf32>
        %113 = "hivm.hir.vsub"(%108, %112, %25) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x256xf32>, tensor<16x256xf32>, tensor<16x256xf32>) -> tensor<16x256xf32>
        %114 = "hivm.hir.vexp"(%113, %24) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16x256xf32>, tensor<16x256xf32>) -> tensor<16x256xf32>
        %115 = "tensor.empty"() : () -> tensor<16x1xf32>
        %116 = "hivm.hir.vreduce"(%114, %115) <{arith = #hivm.reduce_op<sum>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 1>}> : (tensor<16x256xf32>, tensor<16x1xf32>) -> tensor<16x1xf32>
        %117 = "tensor.collapse_shape"(%116) <{reassociation = [[0, 1]]}> : (tensor<16x1xf32>) -> tensor<16xf32>
        %118 = "tensor.empty"() : () -> tensor<16xi1>
        %119 = "hivm.hir.vcmp"(%arg47, %111, %118) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<gt>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xi1>) -> tensor<16xi1>
        %120 = "hivm.hir.vsel"(%119, %arg47, %111, %44) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<16xi1>, tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %121 = "hivm.hir.vsub"(%arg47, %120, %43) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %122 = "hivm.hir.vexp"(%121, %42) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %123 = "hivm.hir.vsub"(%111, %120, %41) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %124 = "hivm.hir.vexp"(%123, %40) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %125 = "hivm.hir.vmul"(%122, %arg48, %39) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %126 = "hivm.hir.vmul"(%124, %117, %38) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %127 = "hivm.hir.vadd"(%125, %126, %37) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %128 = "hivm.hir.vdiv"(%125, %127, %36) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %129 = "tensor.expand_shape"(%124) <{reassociation = [[0, 1]], static_output_shape = array<i64: 16, 1>}> : (tensor<16xf32>) -> tensor<16x1xf32>
        %130 = "hivm.hir.vbrc"(%129, %23) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xf32>, tensor<16x256xf32>) -> tensor<16x256xf32>
        %131 = "hivm.hir.vmul"(%130, %114, %22) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x256xf32>, tensor<16x256xf32>, tensor<16x256xf32>) -> tensor<16x256xf32>
        %132 = "tensor.expand_shape"(%127) <{reassociation = [[0, 1]], static_output_shape = array<i64: 16, 1>}> : (tensor<16xf32>) -> tensor<16x1xf32>
        %133 = "hivm.hir.vbrc"(%132, %21) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xf32>, tensor<16x256xf32>) -> tensor<16x256xf32>
        %134 = "hivm.hir.vdiv"(%131, %133, %20) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x256xf32>, tensor<16x256xf32>, tensor<16x256xf32>) -> tensor<16x256xf32>
        %135 = "tensor.empty"() : () -> tensor<16x256xbf16>
        %136 = "hivm.hir.vcast"(%134, %135) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x256xf32>, tensor<16x256xbf16>) -> tensor<16x256xbf16>
        "hivm.hir.store"(%136, %57) : (tensor<16x256xbf16>, memref<16x256xbf16, strided<[?, 1], offset: ?>>) -> ()
        %137 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x256xbf16>
        "hivm.hir.load"(%57, %137) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<16x256xbf16, strided<[?, 1], offset: ?>>, memref<16x256xbf16>) -> ()
        %138 = "bufferization.to_tensor"(%137) <{restrict, writable}> : (memref<16x256xbf16>) -> tensor<16x256xbf16>
        %139 = "tensor.empty"() : () -> tensor<16x512xf32>
        %140 = "hivm.hir.mmadL1"(%138, %82, %2, %11, %8, %3, %139) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_already_inserted = true} : (tensor<16x256xbf16>, tensor<256x512xbf16>, i1, index, index, index, tensor<16x512xf32>) -> tensor<16x512xf32>
        "hivm.hir.fixpipe"(%140, %60) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, pre_quant = #hivm.fixpipe_pre_quant_mode<F322BF16>}> : (tensor<16x512xf32>, memref<16x512xbf16, strided<[?, 1], offset: ?>>) -> ()
        %141 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x512xbf16>
        "hivm.hir.load"(%60, %141) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<16x512xbf16, strided<[?, 1], offset: ?>>, memref<16x512xbf16>) -> ()
        %142 = "bufferization.to_tensor"(%141) <{restrict, writable}> : (memref<16x512xbf16>) -> tensor<16x512xbf16>
        %143 = "tensor.expand_shape"(%128) <{reassociation = [[0, 1]], static_output_shape = array<i64: 16, 1>}> : (tensor<16xf32>) -> tensor<16x1xf32>
        %144 = "hivm.hir.vbrc"(%143, %33) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xf32>, tensor<16x512xf32>) -> tensor<16x512xf32>
        %145 = "hivm.hir.vmul"(%arg49, %144, %32) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x512xf32>, tensor<16x512xf32>, tensor<16x512xf32>) -> tensor<16x512xf32>
        %146 = "hivm.hir.vcast"(%142, %31) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x512xbf16>, tensor<16x512xf32>) -> tensor<16x512xf32>
        %147 = "hivm.hir.vadd"(%145, %146, %30) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x512xf32>, tensor<16x512xf32>, tensor<16x512xf32>) -> tensor<16x512xf32>
        "scf.yield"(%120, %127, %147) : (tensor<16xf32>, tensor<16xf32>, tensor<16x512xf32>) -> ()
      }) : (i32, i32, i32, tensor<16xf32>, tensor<16xf32>, tensor<16x512xf32>) -> (tensor<16xf32>, tensor<16xf32>, tensor<16x512xf32>)
      %72 = "arith.index_cast"(%64) : (i32) -> index
      %73 = "arith.index_cast"(%arg23) : (i32) -> index
      %74 = "memref.reinterpret_cast"(%arg7, %72, %73) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 512>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xf32>, index, index) -> memref<16x512xf32, strided<[?, 1], offset: ?>>
      "hivm.hir.store"(%71#2, %74) : (tensor<16x512xf32>, memref<16x512xf32, strided<[?, 1], offset: ?>>) -> ()
      "scf.yield"() : () -> ()
    }) : (i32, i32, i32) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false]> : vector<45xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<MIX>, mix_mode = "mix", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<MIX>} : () -> ()

