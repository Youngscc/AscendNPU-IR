"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 2 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 2 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 2 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {}, {}, {}, {}, {}, {}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, memref<?xi32>, memref<?xf32>, memref<?xbf16>, memref<?xbf16>, memref<?xf32>, memref<?xbf16>, memref<?xf32>, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, f32, i32, i32, i32, i32, i32, i32, i32, i32) -> (), sym_name = "sparse_flash_attention_prefill_kernel_cvpipe"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xbf16>, %arg4: memref<?xbf16>, %arg5: memref<?xbf16>, %arg6: memref<?xi32>, %arg7: memref<?xf32>, %arg8: memref<?xbf16>, %arg9: memref<?xbf16>, %arg10: memref<?xf32>, %arg11: memref<?xbf16>, %arg12: memref<?xf32>, %arg13: i32, %arg14: i32, %arg15: i32, %arg16: i32, %arg17: i32, %arg18: i32, %arg19: i32, %arg20: i32, %arg21: i32, %arg22: i32, %arg23: i32, %arg24: i32, %arg25: i32, %arg26: i32, %arg27: i32, %arg28: i32, %arg29: i32, %arg30: i32, %arg31: i32, %arg32: i32, %arg33: i32, %arg34: i32, %arg35: i32, %arg36: f32, %arg37: i32, %arg38: i32, %arg39: i32, %arg40: i32, %arg41: i32, %arg42: i32, %arg43: i32, %arg44: i32):
    %0 = "arith.constant"() <{value = 1 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = false}> : () -> i1
    %2 = "arith.constant"() <{value = true}> : () -> i1
    %3 = "arith.constant"() <{value = 512 : index}> : () -> index
    %4 = "arith.constant"() <{value = 2 : i32}> : () -> i32
    %5 = "arith.constant"() <{value = 8 : i32}> : () -> i32
    %6 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %7 = "arith.constant"() <{value = 256 : i32}> : () -> i32
    %8 = "arith.constant"() <{value = 2.000000e+00 : f32}> : () -> f32
    %9 = "arith.constant"() <{value = 1024 : index}> : () -> index
    %10 = "arith.constant"() <{value = 128 : index}> : () -> index
    %11 = "arith.constant"() <{value = 128 : i32}> : () -> i32
    %12 = "arith.constant"() <{value = 0xFF800000 : f32}> : () -> f32
    %13 = "arith.constant"() <{value = 16 : index}> : () -> index
    %14 = "arith.constant"() <{value = 64 : index}> : () -> index
    %15 = "arith.constant"() <{value = 256 : index}> : () -> index
    "hivm.hir.set_mask_norm"() : () -> ()
    %16 = "arith.muli"(%arg42, %arg43) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %17 = "arith.muli"(%16, %arg44) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%17) {logical_block_num} : (i32) -> ()
    %18 = "hivm.hir.get_block_idx"() : () -> i64
    %19 = "arith.trunci"(%18) : (i64) -> i32
    %20 = "arith.muli"(%arg44, %arg43) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %21 = "arith.divsi"(%19, %20) : (i32, i32) -> i32
    %22 = "arith.remsi"(%21, %arg42) : (i32, i32) -> i32
    %23 = "tensor.empty"() : () -> tensor<16x128xf32>
    %24 = "tensor.empty"() : () -> tensor<16x128xf32>
    %25 = "tensor.empty"() : () -> tensor<16x128xf32>
    %26 = "tensor.empty"() : () -> tensor<16x128xf32>
    %27 = "tensor.empty"() : () -> tensor<16x128xf32>
    %28 = "tensor.empty"() : () -> tensor<16x128xf32>
    %29 = "tensor.empty"() : () -> tensor<16x128xf32>
    %30 = "hivm.hir.vbrc"(%12, %29) <{broadcast_dims = array<i64>}> : (f32, tensor<16x128xf32>) -> tensor<16x128xf32>
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
    %45 = "tensor.empty"() : () -> tensor<16xf32>
    %46 = "tensor.empty"() : () -> tensor<16xf32>
    %47 = "tensor.empty"() : () -> tensor<16xf32>
    %48 = "tensor.empty"() : () -> tensor<16x256xf32>
    %49 = "tensor.empty"() : () -> tensor<16x256xf32>
    %50 = "tensor.empty"() : () -> tensor<16x256xf32>
    %51 = "tensor.empty"() : () -> tensor<16x256xf32>
    %52 = "tensor.empty"() : () -> tensor<16x256xf32>
    %53 = "hivm.hir.vbrc"(%12, %47) <{broadcast_dims = array<i64>}> : (f32, tensor<16xf32>) -> tensor<16xf32>
    %54 = "arith.muli"(%22, %arg41) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %55 = "arith.addi"(%22, %0) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %56 = "arith.muli"(%55, %arg41) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %57 = "arith.minsi"(%arg37, %56) : (i32, i32) -> i32
    %58 = "arith.muli"(%22, %arg32) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %59 = "arith.muli"(%22, %arg34) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %60 = "arith.index_cast"(%58) : (i32) -> index
    %61 = "arith.index_cast"(%arg33) : (i32) -> index
    %62 = "memref.reinterpret_cast"(%arg11, %60, %61) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 128>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<16x128xbf16, strided<[?, 1], offset: ?>>
    %63 = "arith.index_cast"(%59) : (i32) -> index
    %64 = "arith.index_cast"(%arg35) : (i32) -> index
    "scf.for"(%54, %57, %0) ({
    ^bb0(%arg45: i32):
      %65 = "arith.muli"(%arg45, %arg13) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %66 = "arith.muli"(%arg45, %arg24) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %67 = "arith.muli"(%arg45, %arg27) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %68 = "arith.muli"(%arg45, %arg22) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %69 = "arith.index_cast"(%65) : (i32) -> index
      %70 = "arith.index_cast"(%arg15) : (i32) -> index
      %71 = "memref.reinterpret_cast"(%arg3, %69, %70) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 512>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<16x512xbf16, strided<[?, 1], offset: ?>>
      %72 = "arith.addi"(%69, %3) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %73 = "memref.reinterpret_cast"(%arg3, %72, %70) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 64>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<16x64xbf16, strided<[?, 1], offset: ?>>
      %74 = "arith.index_cast"(%66) : (i32) -> index
      %75 = "arith.index_cast"(%68) : (i32) -> index
      %76 = "arith.index_cast"(%arg23) : (i32) -> index
      %77 = "scf.for"(%6, %5, %0, %53) ({
      ^bb0(%arg46: i32, %arg47: tensor<16xf32>):
        %78 = "arith.muli"(%arg46, %11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
        %79 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x512xbf16>
        "hivm.hir.load"(%71, %79) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<16x512xbf16, strided<[?, 1], offset: ?>>, memref<16x512xbf16>) -> ()
        %80 = "bufferization.to_tensor"(%79) <{restrict, writable}> : (memref<16x512xbf16>) -> tensor<16x512xbf16>
        %81 = "arith.index_cast"(%78) : (i32) -> index
        %82 = "arith.index_cast"(%arg26) : (i32) -> index
        %83 = "arith.muli"(%81, %82) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
        %84 = "arith.addi"(%74, %83) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
        %85 = "memref.reinterpret_cast"(%arg8, %84, %82) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 128, 512>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<128x512xbf16, strided<[?, 1], offset: ?>>
        %86 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<128x512xbf16>
        "hivm.hir.load"(%85, %86) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<128x512xbf16, strided<[?, 1], offset: ?>>, memref<128x512xbf16>) -> ()
        %87 = "bufferization.to_tensor"(%86) <{restrict, writable}> : (memref<128x512xbf16>) -> tensor<128x512xbf16>
        %88 = "tensor.empty"() : () -> tensor<16x128xf32>
        %89 = "hivm.hir.mmadL1"(%80, %87, %2, %13, %3, %10, %88) <{b_transpose, operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> : (tensor<16x512xbf16>, tensor<128x512xbf16>, i1, index, index, index, tensor<16x128xf32>) -> tensor<16x128xf32>
        %90 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x64xbf16>
        "hivm.hir.load"(%73, %90) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<16x64xbf16, strided<[?, 1], offset: ?>>, memref<16x64xbf16>) -> ()
        %91 = "bufferization.to_tensor"(%90) <{restrict, writable}> : (memref<16x64xbf16>) -> tensor<16x64xbf16>
        %92 = "arith.index_cast"(%67) : (i32) -> index
        %93 = "arith.index_cast"(%arg29) : (i32) -> index
        %94 = "arith.muli"(%81, %93) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
        %95 = "arith.addi"(%92, %94) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
        %96 = "memref.reinterpret_cast"(%arg9, %95, %93) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 128, 64>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<128x64xbf16, strided<[?, 1], offset: ?>>
        %97 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<128x64xbf16>
        "hivm.hir.load"(%96, %97) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<128x64xbf16, strided<[?, 1], offset: ?>>, memref<128x64xbf16>) -> ()
        %98 = "bufferization.to_tensor"(%97) <{restrict, writable}> : (memref<128x64xbf16>) -> tensor<128x64xbf16>
        %99 = "hivm.hir.mmadL1"(%91, %98, %1, %13, %14, %10, %89) <{b_transpose, operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_already_inserted = true} : (tensor<16x64xbf16>, tensor<128x64xbf16>, i1, index, index, index, tensor<16x128xf32>) -> tensor<16x128xf32>
        %100 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x128xf32>
        %101 = "bufferization.to_tensor"(%100) <{restrict, writable}> : (memref<16x128xf32>) -> tensor<16x128xf32>
        %102 = "hivm.hir.fixpipe"(%99, %101) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<16x128xf32>, tensor<16x128xf32>) -> tensor<16x128xf32>
        %103 = "tensor.empty"() : () -> tensor<16x128xf32>
        %104 = "hivm.hir.load"(%102, %103) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<16x128xf32>, tensor<16x128xf32>) -> tensor<16x128xf32>
        %105 = "hivm.hir.vmul"(%104, %arg36, %28) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x128xf32>, f32, tensor<16x128xf32>) -> tensor<16x128xf32>
        %106 = "arith.addi"(%81, %10) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
        %107 = "arith.maxsi"(%81, %9) : (index, index) -> index
        %108 = "arith.minsi"(%106, %107) : (index, index) -> index
        %109 = "arith.subi"(%108, %81) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
        %110 = "tensor.extract_slice"(%105, %109) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: 16, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<16x128xf32>, index) -> tensor<16x?xf32>
        %111 = "tensor.insert_slice"(%110, %30, %109) <{operandSegmentSizes = array<i32: 1, 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: 16, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<16x?xf32>, tensor<16x128xf32>, index) -> tensor<16x128xf32>
        %112 = "tensor.empty"() : () -> tensor<16x1xf32>
        %113 = "hivm.hir.vreduce"(%111, %112) <{arith = #hivm.reduce_op<max>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 1>}> : (tensor<16x128xf32>, tensor<16x1xf32>) -> tensor<16x1xf32>
        %114 = "tensor.collapse_shape"(%113) <{reassociation = [[0, 1]]}> : (tensor<16x1xf32>) -> tensor<16xf32>
        %115 = "hivm.hir.vbrc"(%113, %27) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xf32>, tensor<16x128xf32>) -> tensor<16x128xf32>
        %116 = "hivm.hir.vsub"(%111, %115, %26) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x128xf32>, tensor<16x128xf32>, tensor<16x128xf32>) -> tensor<16x128xf32>
        %117 = "hivm.hir.vexp"(%116, %25) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16x128xf32>, tensor<16x128xf32>) -> tensor<16x128xf32>
        %118 = "tensor.empty"() : () -> tensor<16x1xf32>
        %119 = "hivm.hir.vreduce"(%117, %118) <{arith = #hivm.reduce_op<sum>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 1>}> : (tensor<16x128xf32>, tensor<16x1xf32>) -> tensor<16x1xf32>
        %120 = "tensor.collapse_shape"(%119) <{reassociation = [[0, 1]]}> : (tensor<16x1xf32>) -> tensor<16xf32>
        %121 = "hivm.hir.vln"(%120, %46) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %122 = "hivm.hir.vadd"(%114, %121, %45) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %123 = "hivm.hir.vadd"(%arg47, %122, %44) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %124 = "hivm.hir.vdiv"(%123, %8, %43) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, f32, tensor<16xf32>) -> tensor<16xf32>
        %125 = "hivm.hir.vsub"(%arg47, %124, %42) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %126 = "hivm.hir.vexp"(%125, %41) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %127 = "hivm.hir.vsub"(%122, %124, %40) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %128 = "hivm.hir.vexp"(%127, %39) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %129 = "hivm.hir.vadd"(%126, %128, %38) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %130 = "hivm.hir.vln"(%129, %37) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %131 = "hivm.hir.vadd"(%124, %130, %36) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %132 = "tensor.empty"() : () -> tensor<16xi1>
        %133 = "tensor.empty"() : () -> tensor<16xi1>
        %134 = "hivm.hir.vcmp"(%131, %131, %133) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xi1>) -> tensor<16xi1>
        %135 = "hivm.hir.vnot"(%134, %132) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xi1>, tensor<16xi1>) -> tensor<16xi1>
        %136 = "hivm.hir.vsel"(%135, %122, %131, %35) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<16xi1>, tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %137 = "hivm.hir.vsub"(%122, %136, %34) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %138 = "hivm.hir.vexp"(%137, %33) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %139 = "hivm.hir.vbrc"(%119, %24) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xf32>, tensor<16x128xf32>) -> tensor<16x128xf32>
        %140 = "hivm.hir.vdiv"(%117, %139, %23) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x128xf32>, tensor<16x128xf32>, tensor<16x128xf32>) -> tensor<16x128xf32>
        %141 = "tensor.empty"() : () -> tensor<16x128xbf16>
        %142 = "hivm.hir.vcast"(%140, %141) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x128xf32>, tensor<16x128xbf16>) -> tensor<16x128xbf16>
        "hivm.hir.store"(%142, %62) : (tensor<16x128xbf16>, memref<16x128xbf16, strided<[?, 1], offset: ?>>) -> ()
        %143 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x128xbf16>
        "hivm.hir.load"(%62, %143) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<16x128xbf16, strided<[?, 1], offset: ?>>, memref<16x128xbf16>) -> ()
        %144 = "bufferization.to_tensor"(%143) <{restrict, writable}> : (memref<16x128xbf16>) -> tensor<16x128xbf16>
        %145 = "hivm.hir.vsub"(%arg47, %136, %32) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %146 = "hivm.hir.vexp"(%145, %31) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %147 = "tensor.expand_shape"(%146) <{reassociation = [[0, 1]], static_output_shape = array<i64: 16, 1>}> : (tensor<16xf32>) -> tensor<16x1xf32>
        %148 = "hivm.hir.vbrc"(%147, %52) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xf32>, tensor<16x256xf32>) -> tensor<16x256xf32>
        %149 = "tensor.expand_shape"(%138) <{reassociation = [[0, 1]], static_output_shape = array<i64: 16, 1>}> : (tensor<16xf32>) -> tensor<16x1xf32>
        %150 = "hivm.hir.vbrc"(%149, %51) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xf32>, tensor<16x256xf32>) -> tensor<16x256xf32>
        "scf.for"(%6, %4, %0) ({
        ^bb0(%arg48: i32):
          %151 = "arith.muli"(%arg48, %7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
          %152 = "arith.index_cast"(%151) : (i32) -> index
          %153 = "arith.addi"(%84, %152) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
          %154 = "memref.reinterpret_cast"(%arg8, %153, %82) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 128, 256>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<128x256xbf16, strided<[?, 1], offset: ?>>
          %155 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<128x256xbf16>
          "hivm.hir.load"(%154, %155) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<128x256xbf16, strided<[?, 1], offset: ?>>, memref<128x256xbf16>) -> ()
          %156 = "bufferization.to_tensor"(%155) <{restrict, writable}> : (memref<128x256xbf16>) -> tensor<128x256xbf16>
          %157 = "tensor.empty"() : () -> tensor<16x256xf32>
          %158 = "hivm.hir.mmadL1"(%144, %156, %2, %13, %10, %15, %157) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_already_inserted = true} : (tensor<16x128xbf16>, tensor<128x256xbf16>, i1, index, index, index, tensor<16x256xf32>) -> tensor<16x256xf32>
          %159 = "arith.addi"(%63, %152) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
          %160 = "memref.reinterpret_cast"(%arg12, %159, %64) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 256>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xf32>, index, index) -> memref<16x256xf32, strided<[?, 1], offset: ?>>
          "hivm.hir.fixpipe"(%158, %160) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<16x256xf32>, memref<16x256xf32, strided<[?, 1], offset: ?>>) -> ()
          %161 = "arith.addi"(%75, %152) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
          %162 = "memref.reinterpret_cast"(%arg7, %161, %76) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 256>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xf32>, index, index) -> memref<16x256xf32, strided<[?, 1], offset: ?>>
          %163 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x256xf32>
          "hivm.hir.load"(%162, %163) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<16x256xf32, strided<[?, 1], offset: ?>>, memref<16x256xf32>) -> ()
          %164 = "bufferization.to_tensor"(%163) <{restrict, writable}> : (memref<16x256xf32>) -> tensor<16x256xf32>
          %165 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x256xf32>
          "hivm.hir.load"(%160, %165) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<16x256xf32, strided<[?, 1], offset: ?>>, memref<16x256xf32>) -> ()
          %166 = "bufferization.to_tensor"(%165) <{restrict, writable}> : (memref<16x256xf32>) -> tensor<16x256xf32>
          %167 = "hivm.hir.vmul"(%164, %148, %50) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x256xf32>, tensor<16x256xf32>, tensor<16x256xf32>) -> tensor<16x256xf32>
          %168 = "hivm.hir.vmul"(%166, %150, %49) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x256xf32>, tensor<16x256xf32>, tensor<16x256xf32>) -> tensor<16x256xf32>
          %169 = "hivm.hir.vadd"(%167, %168, %48) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x256xf32>, tensor<16x256xf32>, tensor<16x256xf32>) -> tensor<16x256xf32>
          "hivm.hir.store"(%169, %162) : (tensor<16x256xf32>, memref<16x256xf32, strided<[?, 1], offset: ?>>) -> ()
          "scf.yield"() : () -> ()
        }) : (i32, i32, i32) -> ()
        "scf.yield"(%136) : (tensor<16xf32>) -> ()
      }) : (i32, i32, i32, tensor<16xf32>) -> tensor<16xf32>
      "scf.yield"() : () -> ()
    }) : (i32, i32, i32) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false]> : vector<45xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<MIX>, mix_mode = "mix", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<MIX>} : () -> ()

