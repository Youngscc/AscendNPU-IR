#map = affine_map<()[s0] -> (s0 + 512)>
#map1 = affine_map<()[s0, s1] -> (s0 * s1)>
#map2 = affine_map<()[s0, s1] -> (s0 + s1)>
#map3 = affine_map<()[s0] -> (s0 + 256)>
#map4 = affine_map<()[s0] -> (s0, 1024)>
#map5 = affine_map<()[s0, s1] -> (s0, s1)>
#map6 = affine_map<()[s0, s1] -> (s0 - s1)>
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
    %7 = "arith.constant"() <{value = 256 : index}> : () -> index
    %8 = "arith.constant"() <{value = 256 : i32}> : () -> i32
    %9 = "arith.constant"() <{value = 0xFF800000 : f32}> : () -> f32
    %10 = "arith.constant"() <{value = 16 : index}> : () -> index
    %11 = "arith.constant"() <{value = 64 : index}> : () -> index
    "hivm.hir.set_mask_norm"() : () -> ()
    %12 = "arith.muli"(%arg42, %arg43) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %13 = "arith.muli"(%12, %arg44) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%13) <{effects = ["write"]}> {logical_block_num} : (i32) -> ()
    %14 = "hivm.hir.get_block_idx"() : () -> i64
    %15 = "arith.trunci"(%14) : (i64) -> i32
    %16 = "arith.muli"(%arg44, %arg43) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %17 = "arith.divsi"(%15, %16) : (i32, i32) -> i32
    %18 = "arith.remsi"(%17, %arg42) : (i32, i32) -> i32
    %19 = "tensor.empty"() : () -> tensor<16x256xf32>
    %20 = "hivm.hir.vbrc"(%9, %19) <{broadcast_dims = array<i64>}> : (f32, tensor<16x256xf32>) -> tensor<16x256xf32>
    %21 = "tensor.empty"() : () -> tensor<16x512xf32>
    %22 = "hivm.hir.vbrc"(%6, %21) <{broadcast_dims = array<i64>}> : (f32, tensor<16x512xf32>) -> tensor<16x512xf32>
    %23 = "tensor.empty"() : () -> tensor<16xf32>
    %24 = "hivm.hir.vbrc"(%6, %23) <{broadcast_dims = array<i64>}> : (f32, tensor<16xf32>) -> tensor<16xf32>
    %25 = "hivm.hir.vbrc"(%9, %23) <{broadcast_dims = array<i64>}> : (f32, tensor<16xf32>) -> tensor<16xf32>
    %26 = "arith.muli"(%18, %arg41) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %27 = "arith.addi"(%18, %0) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %28 = "arith.muli"(%27, %arg41) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %29 = "arith.minsi"(%arg37, %28) : (i32, i32) -> i32
    %30 = "arith.muli"(%18, %arg32) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %31 = "arith.muli"(%18, %arg34) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %32 = "arith.index_cast"(%30) : (i32) -> index
    %33 = "arith.index_cast"(%arg33) : (i32) -> index
    %34 = "memref.reinterpret_cast"(%arg11, %32, %33) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 256>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<16x256xbf16, strided<[?, 1], offset: ?>>
    %35 = "arith.index_cast"(%31) : (i32) -> index
    %36 = "arith.index_cast"(%arg35) : (i32) -> index
    %37 = "memref.reinterpret_cast"(%arg12, %35, %36) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 512>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<16x512xbf16, strided<[?, 1], offset: ?>>
    "scf.for"(%26, %29, %0) ({
    ^bb0(%arg45: i32):
      %38 = "arith.muli"(%arg45, %arg13) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %39 = "arith.muli"(%arg45, %arg24) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %40 = "arith.muli"(%arg45, %arg27) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %41 = "arith.muli"(%arg45, %arg22) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %42 = "arith.index_cast"(%39) : (i32) -> index
      %43 = "arith.index_cast"(%38) : (i32) -> index
      %44 = "arith.index_cast"(%arg15) : (i32) -> index
      %45 = "memref.reinterpret_cast"(%arg3, %43, %44) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 512>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<16x512xbf16, strided<[?, 1], offset: ?>>
      %46 = "affine.apply"(%43) <{map = #map}> : (index) -> index
      %47 = "memref.reinterpret_cast"(%arg3, %46, %44) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 64>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<16x64xbf16, strided<[?, 1], offset: ?>>
      %48:3 = "scf.for"(%5, %4, %0, %25, %24, %22) ({
      ^bb0(%arg46: i32, %arg47: tensor<16xf32>, %arg48: tensor<16xf32>, %arg49: tensor<16x512xf32>):
        %52 = "arith.muli"(%arg46, %8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
        %53 = "arith.index_cast"(%52) : (i32) -> index
        %54 = "arith.index_cast"(%arg26) : (i32) -> index
        %55 = "affine.apply"(%53, %54) <{map = #map1}> : (index, index) -> index
        %56 = "affine.apply"(%42, %55) <{map = #map2}> : (index, index) -> index
        %57 = "memref.reinterpret_cast"(%arg8, %56, %54) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 256, 512>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<256x512xbf16, strided<[?, 1], offset: ?>>
        %58 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<256x512xbf16>
        "hivm.hir.load"(%57, %58) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<256x512xbf16, strided<[?, 1], offset: ?>>, memref<256x512xbf16>) -> ()
        %59 = "bufferization.to_tensor"(%58) <{restrict, writable}> : (memref<256x512xbf16>) -> tensor<256x512xbf16>
        %60 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x512xbf16>
        "hivm.hir.load"(%45, %60) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<16x512xbf16, strided<[?, 1], offset: ?>>, memref<16x512xbf16>) -> ()
        %61 = "bufferization.to_tensor"(%60) <{restrict, writable}> : (memref<16x512xbf16>) -> tensor<16x512xbf16>
        %62 = "hivm.hir.mmadL1"(%61, %59, %2, %10, %3, %7, %19) <{b_transpose, operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> : (tensor<16x512xbf16>, tensor<256x512xbf16>, i1, index, index, index, tensor<16x256xf32>) -> tensor<16x256xf32>
        %63 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x64xbf16>
        "hivm.hir.load"(%47, %63) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<16x64xbf16, strided<[?, 1], offset: ?>>, memref<16x64xbf16>) -> ()
        %64 = "bufferization.to_tensor"(%63) <{restrict, writable}> : (memref<16x64xbf16>) -> tensor<16x64xbf16>
        %65 = "arith.index_cast"(%40) : (i32) -> index
        %66 = "arith.index_cast"(%arg29) : (i32) -> index
        %67 = "affine.apply"(%53, %66) <{map = #map1}> : (index, index) -> index
        %68 = "affine.apply"(%65, %67) <{map = #map2}> : (index, index) -> index
        %69 = "memref.reinterpret_cast"(%arg9, %68, %66) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 256, 64>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<256x64xbf16, strided<[?, 1], offset: ?>>
        %70 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<256x64xbf16>
        "hivm.hir.load"(%69, %70) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<256x64xbf16, strided<[?, 1], offset: ?>>, memref<256x64xbf16>) -> ()
        %71 = "bufferization.to_tensor"(%70) <{restrict, writable}> : (memref<256x64xbf16>) -> tensor<256x64xbf16>
        %72 = "hivm.hir.mmadL1"(%64, %71, %1, %10, %11, %7, %62) <{b_transpose, operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_for_result_already_inserted = true} : (tensor<16x64xbf16>, tensor<256x64xbf16>, i1, index, index, index, tensor<16x256xf32>) -> tensor<16x256xf32>
        %73 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x256xf32>
        %74 = "bufferization.to_tensor"(%73) <{restrict, writable}> : (memref<16x256xf32>) -> tensor<16x256xf32>
        %75 = "hivm.hir.fixpipe"(%72, %74) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<16x256xf32>, tensor<16x256xf32>) -> tensor<16x256xf32>
        %76 = "hivm.hir.load"(%75, %19) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<16x256xf32>, tensor<16x256xf32>) -> tensor<16x256xf32>
        %77 = "hivm.hir.vmul"(%76, %arg36, %19) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x256xf32>, f32, tensor<16x256xf32>) -> tensor<16x256xf32>
        %78 = "affine.apply"(%53) <{map = #map3}> : (index) -> index
        %79 = "affine.max"(%53) <{map = #map4}> : (index) -> index
        %80 = "affine.min"(%78, %79) <{map = #map5}> : (index, index) -> index
        %81 = "affine.apply"(%80, %53) <{map = #map6}> : (index, index) -> index
        %82 = "tensor.extract_slice"(%77, %81) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: 16, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<16x256xf32>, index) -> tensor<16x?xf32>
        %83 = "tensor.insert_slice"(%82, %20, %81) <{operandSegmentSizes = array<i32: 1, 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: 16, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<16x?xf32>, tensor<16x256xf32>, index) -> tensor<16x256xf32>
        %84 = "tensor.empty"() : () -> tensor<16x1xf32>
        %85 = "hivm.hir.vreduce"(%83, %84) <{arith = #hivm.reduce_op<max>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 1>, tie_break_left = true, unsigned_src = false}> : (tensor<16x256xf32>, tensor<16x1xf32>) -> tensor<16x1xf32>
        %86 = "tensor.collapse_shape"(%85) <{reassociation = [[0, 1]]}> : (tensor<16x1xf32>) -> tensor<16xf32>
        %87 = "hivm.hir.vbrc"(%85, %19) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xf32>, tensor<16x256xf32>) -> tensor<16x256xf32>
        %88 = "hivm.hir.vsub"(%83, %87, %19) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x256xf32>, tensor<16x256xf32>, tensor<16x256xf32>) -> tensor<16x256xf32>
        %89 = "hivm.hir.vexp"(%88, %19) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16x256xf32>, tensor<16x256xf32>) -> tensor<16x256xf32>
        %90 = "hivm.hir.vreduce"(%89, %84) <{arith = #hivm.reduce_op<sum>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 1>, tie_break_left = true, unsigned_src = false}> : (tensor<16x256xf32>, tensor<16x1xf32>) -> tensor<16x1xf32>
        %91 = "tensor.collapse_shape"(%90) <{reassociation = [[0, 1]]}> : (tensor<16x1xf32>) -> tensor<16xf32>
        %92 = "tensor.empty"() : () -> tensor<16xi1>
        %93 = "hivm.hir.vcmp"(%arg47, %86, %92) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<gt>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xi1>) -> tensor<16xi1>
        %94 = "hivm.hir.vsel"(%93, %arg47, %86, %23) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<16xi1>, tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %95 = "hivm.hir.vsub"(%arg47, %94, %23) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %96 = "hivm.hir.vexp"(%95, %23) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %97 = "hivm.hir.vsub"(%86, %94, %23) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %98 = "hivm.hir.vexp"(%97, %23) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %99 = "hivm.hir.vmul"(%96, %arg48, %23) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %100 = "hivm.hir.vmul"(%98, %91, %23) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %101 = "hivm.hir.vadd"(%99, %100, %23) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %102 = "hivm.hir.vdiv"(%99, %101, %23) <{broadcast = array<i64>, isHP = false, isSigned = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %103 = "tensor.expand_shape"(%98) <{reassociation = [[0, 1]], static_output_shape = array<i64: 16, 1>}> : (tensor<16xf32>) -> tensor<16x1xf32>
        %104 = "hivm.hir.vbrc"(%103, %19) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xf32>, tensor<16x256xf32>) -> tensor<16x256xf32>
        %105 = "hivm.hir.vmul"(%104, %89, %19) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x256xf32>, tensor<16x256xf32>, tensor<16x256xf32>) -> tensor<16x256xf32>
        %106 = "tensor.expand_shape"(%101) <{reassociation = [[0, 1]], static_output_shape = array<i64: 16, 1>}> : (tensor<16xf32>) -> tensor<16x1xf32>
        %107 = "hivm.hir.vbrc"(%106, %19) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xf32>, tensor<16x256xf32>) -> tensor<16x256xf32>
        %108 = "hivm.hir.vdiv"(%105, %107, %19) <{broadcast = array<i64>, isHP = false, isSigned = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x256xf32>, tensor<16x256xf32>, tensor<16x256xf32>) -> tensor<16x256xf32>
        %109 = "tensor.empty"() : () -> tensor<16x256xbf16>
        %110 = "hivm.hir.vcast"(%108, %109) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x256xf32>, tensor<16x256xbf16>) -> tensor<16x256xbf16>
        "hivm.hir.store"(%110, %34) : (tensor<16x256xbf16>, memref<16x256xbf16, strided<[?, 1], offset: ?>>) -> ()
        %111 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x256xbf16>
        "hivm.hir.load"(%34, %111) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<16x256xbf16, strided<[?, 1], offset: ?>>, memref<16x256xbf16>) -> ()
        %112 = "bufferization.to_tensor"(%111) <{restrict, writable}> : (memref<16x256xbf16>) -> tensor<16x256xbf16>
        %113 = "hivm.hir.mmadL1"(%112, %59, %2, %10, %7, %3, %21) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_for_result_already_inserted = true} : (tensor<16x256xbf16>, tensor<256x512xbf16>, i1, index, index, index, tensor<16x512xf32>) -> tensor<16x512xf32>
        "hivm.hir.fixpipe"(%113, %37) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, pre_quant = #hivm.fixpipe_pre_quant_mode<F322BF16>}> : (tensor<16x512xf32>, memref<16x512xbf16, strided<[?, 1], offset: ?>>) -> ()
        %114 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x512xbf16>
        "hivm.hir.load"(%37, %114) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<16x512xbf16, strided<[?, 1], offset: ?>>, memref<16x512xbf16>) -> ()
        %115 = "bufferization.to_tensor"(%114) <{restrict, writable}> : (memref<16x512xbf16>) -> tensor<16x512xbf16>
        %116 = "tensor.expand_shape"(%102) <{reassociation = [[0, 1]], static_output_shape = array<i64: 16, 1>}> : (tensor<16xf32>) -> tensor<16x1xf32>
        %117 = "hivm.hir.vbrc"(%116, %21) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xf32>, tensor<16x512xf32>) -> tensor<16x512xf32>
        %118 = "hivm.hir.vmul"(%arg49, %117, %21) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x512xf32>, tensor<16x512xf32>, tensor<16x512xf32>) -> tensor<16x512xf32>
        %119 = "hivm.hir.vcast"(%115, %21) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x512xbf16>, tensor<16x512xf32>) -> tensor<16x512xf32>
        %120 = "hivm.hir.vadd"(%118, %119, %21) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x512xf32>, tensor<16x512xf32>, tensor<16x512xf32>) -> tensor<16x512xf32>
        "scf.yield"(%94, %101, %120) : (tensor<16xf32>, tensor<16xf32>, tensor<16x512xf32>) -> ()
      }) {fixpipe_for_mmad_result_already_inserted = true} : (i32, i32, i32, tensor<16xf32>, tensor<16xf32>, tensor<16x512xf32>) -> (tensor<16xf32>, tensor<16xf32>, tensor<16x512xf32>)
      %49 = "arith.index_cast"(%41) : (i32) -> index
      %50 = "arith.index_cast"(%arg23) : (i32) -> index
      %51 = "memref.reinterpret_cast"(%arg7, %49, %50) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 512>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xf32>, index, index) -> memref<16x512xf32, strided<[?, 1], offset: ?>>
      "hivm.hir.store"(%48#2, %51) : (tensor<16x512xf32>, memref<16x512xf32, strided<[?, 1], offset: ?>>) -> ()
      "scf.yield"() : () -> ()
    }) : (i32, i32, i32) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false]> : vector<45xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<MIX>, mix_mode = "mix", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hacc.target = #hacc.target<"Ascend910B1">, hivm.module_core_type = #hivm.module_core_type<MIX>} : () -> ()

