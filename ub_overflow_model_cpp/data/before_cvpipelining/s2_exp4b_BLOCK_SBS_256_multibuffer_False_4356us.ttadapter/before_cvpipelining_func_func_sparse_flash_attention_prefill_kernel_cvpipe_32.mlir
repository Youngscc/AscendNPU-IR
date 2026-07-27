#map = affine_map<()[s0] -> (s0 + 512)>
#map1 = affine_map<()[s0, s1] -> (s0 * s1)>
#map2 = affine_map<()[s0, s1] -> (s0 + s1)>
#map3 = affine_map<()[s0] -> (s0 + 256)>
#map4 = affine_map<()[s0] -> (s0, 1024)>
#map5 = affine_map<()[s0, s1] -> (s0, s1)>
#map6 = affine_map<()[s0, s1] -> (s0 - s1)>
"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 2 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 2 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 2 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {}, {}, {}, {}, {}, {}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, memref<?xi32>, memref<?xf32>, memref<?xbf16>, memref<?xbf16>, memref<?xf32>, memref<?xbf16>, memref<?xf32>, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, f32, i32, i32, i32, i32, i32, i32, i32, i32) -> (), sym_name = "sparse_flash_attention_prefill_kernel_cvpipe"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xbf16>, %arg4: memref<?xbf16>, %arg5: memref<?xbf16>, %arg6: memref<?xi32>, %arg7: memref<?xf32>, %arg8: memref<?xbf16>, %arg9: memref<?xbf16>, %arg10: memref<?xf32>, %arg11: memref<?xbf16>, %arg12: memref<?xf32>, %arg13: i32, %arg14: i32, %arg15: i32, %arg16: i32, %arg17: i32, %arg18: i32, %arg19: i32, %arg20: i32, %arg21: i32, %arg22: i32, %arg23: i32, %arg24: i32, %arg25: i32, %arg26: i32, %arg27: i32, %arg28: i32, %arg29: i32, %arg30: i32, %arg31: i32, %arg32: i32, %arg33: i32, %arg34: i32, %arg35: i32, %arg36: f32, %arg37: i32, %arg38: i32, %arg39: i32, %arg40: i32, %arg41: i32, %arg42: i32, %arg43: i32, %arg44: i32):
    %0 = "arith.constant"() <{value = 1 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = false}> : () -> i1
    %2 = "arith.constant"() <{value = true}> : () -> i1
    %3 = "arith.constant"() <{value = 512 : index}> : () -> index
    %4 = "arith.constant"() <{value = 4 : i32}> : () -> i32
    %5 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %6 = "arith.constant"() <{value = 2.000000e+00 : f32}> : () -> f32
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
    %21 = "tensor.empty"() : () -> tensor<16xf32>
    %22 = "tensor.empty"() : () -> tensor<16x512xf32>
    %23 = "hivm.hir.vbrc"(%9, %21) <{broadcast_dims = array<i64>}> : (f32, tensor<16xf32>) -> tensor<16xf32>
    %24 = "arith.muli"(%18, %arg41) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %25 = "arith.addi"(%18, %0) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %26 = "arith.muli"(%25, %arg41) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %27 = "arith.minsi"(%arg37, %26) : (i32, i32) -> i32
    %28 = "arith.muli"(%18, %arg32) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %29 = "arith.muli"(%18, %arg34) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %30 = "arith.index_cast"(%28) : (i32) -> index
    %31 = "arith.index_cast"(%arg33) : (i32) -> index
    %32 = "memref.reinterpret_cast"(%arg11, %30, %31) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 256>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<16x256xbf16, strided<[?, 1], offset: ?>>
    %33 = "arith.index_cast"(%29) : (i32) -> index
    %34 = "arith.index_cast"(%arg35) : (i32) -> index
    %35 = "memref.reinterpret_cast"(%arg12, %33, %34) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 512>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xf32>, index, index) -> memref<16x512xf32, strided<[?, 1], offset: ?>>
    "scf.for"(%24, %27, %0) ({
    ^bb0(%arg45: i32):
      %36 = "arith.muli"(%arg45, %arg13) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %37 = "arith.muli"(%arg45, %arg24) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %38 = "arith.muli"(%arg45, %arg27) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %39 = "arith.muli"(%arg45, %arg22) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %40 = "arith.index_cast"(%36) : (i32) -> index
      %41 = "arith.index_cast"(%arg15) : (i32) -> index
      %42 = "memref.reinterpret_cast"(%arg3, %40, %41) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 512>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<16x512xbf16, strided<[?, 1], offset: ?>>
      %43 = "affine.apply"(%40) <{map = #map}> : (index) -> index
      %44 = "memref.reinterpret_cast"(%arg3, %43, %41) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 64>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<16x64xbf16, strided<[?, 1], offset: ?>>
      %45 = "arith.index_cast"(%37) : (i32) -> index
      %46 = "arith.index_cast"(%39) : (i32) -> index
      %47 = "arith.index_cast"(%arg23) : (i32) -> index
      %48 = "memref.reinterpret_cast"(%arg7, %46, %47) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 512>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xf32>, index, index) -> memref<16x512xf32, strided<[?, 1], offset: ?>>
      %49 = "scf.for"(%5, %4, %0, %23) ({
      ^bb0(%arg46: i32, %arg47: tensor<16xf32>):
        %50 = "arith.muli"(%arg46, %8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
        %51 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x512xbf16>
        "hivm.hir.load"(%42, %51) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<16x512xbf16, strided<[?, 1], offset: ?>>, memref<16x512xbf16>) -> ()
        %52 = "bufferization.to_tensor"(%51) <{restrict, writable}> : (memref<16x512xbf16>) -> tensor<16x512xbf16>
        %53 = "arith.index_cast"(%50) : (i32) -> index
        %54 = "arith.index_cast"(%arg26) : (i32) -> index
        %55 = "affine.apply"(%53, %54) <{map = #map1}> : (index, index) -> index
        %56 = "affine.apply"(%45, %55) <{map = #map2}> : (index, index) -> index
        %57 = "memref.reinterpret_cast"(%arg8, %56, %54) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 256, 512>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<256x512xbf16, strided<[?, 1], offset: ?>>
        %58 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<256x512xbf16>
        "hivm.hir.load"(%57, %58) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<256x512xbf16, strided<[?, 1], offset: ?>>, memref<256x512xbf16>) -> ()
        %59 = "bufferization.to_tensor"(%58) <{restrict, writable}> : (memref<256x512xbf16>) -> tensor<256x512xbf16>
        %60 = "hivm.hir.mmadL1"(%52, %59, %2, %10, %3, %7, %19) <{b_transpose, operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> : (tensor<16x512xbf16>, tensor<256x512xbf16>, i1, index, index, index, tensor<16x256xf32>) -> tensor<16x256xf32>
        %61 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x64xbf16>
        "hivm.hir.load"(%44, %61) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<16x64xbf16, strided<[?, 1], offset: ?>>, memref<16x64xbf16>) -> ()
        %62 = "bufferization.to_tensor"(%61) <{restrict, writable}> : (memref<16x64xbf16>) -> tensor<16x64xbf16>
        %63 = "arith.index_cast"(%38) : (i32) -> index
        %64 = "arith.index_cast"(%arg29) : (i32) -> index
        %65 = "affine.apply"(%53, %64) <{map = #map1}> : (index, index) -> index
        %66 = "affine.apply"(%63, %65) <{map = #map2}> : (index, index) -> index
        %67 = "memref.reinterpret_cast"(%arg9, %66, %64) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 256, 64>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<256x64xbf16, strided<[?, 1], offset: ?>>
        %68 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<256x64xbf16>
        "hivm.hir.load"(%67, %68) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<256x64xbf16, strided<[?, 1], offset: ?>>, memref<256x64xbf16>) -> ()
        %69 = "bufferization.to_tensor"(%68) <{restrict, writable}> : (memref<256x64xbf16>) -> tensor<256x64xbf16>
        %70 = "hivm.hir.mmadL1"(%62, %69, %1, %10, %11, %7, %60) <{b_transpose, operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_for_result_already_inserted = true} : (tensor<16x64xbf16>, tensor<256x64xbf16>, i1, index, index, index, tensor<16x256xf32>) -> tensor<16x256xf32>
        %71 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x256xf32>
        %72 = "bufferization.to_tensor"(%71) <{restrict, writable}> : (memref<16x256xf32>) -> tensor<16x256xf32>
        %73 = "hivm.hir.fixpipe"(%70, %72) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<16x256xf32>, tensor<16x256xf32>) -> tensor<16x256xf32>
        %74 = "hivm.hir.load"(%73, %19) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<16x256xf32>, tensor<16x256xf32>) -> tensor<16x256xf32>
        %75 = "hivm.hir.vmul"(%74, %arg36, %19) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x256xf32>, f32, tensor<16x256xf32>) -> tensor<16x256xf32>
        %76 = "affine.apply"(%53) <{map = #map3}> : (index) -> index
        %77 = "affine.max"(%53) <{map = #map4}> : (index) -> index
        %78 = "affine.min"(%76, %77) <{map = #map5}> : (index, index) -> index
        %79 = "affine.apply"(%78, %53) <{map = #map6}> : (index, index) -> index
        %80 = "tensor.extract_slice"(%75, %79) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: 16, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<16x256xf32>, index) -> tensor<16x?xf32>
        %81 = "tensor.insert_slice"(%80, %20, %79) <{operandSegmentSizes = array<i32: 1, 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: 16, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<16x?xf32>, tensor<16x256xf32>, index) -> tensor<16x256xf32>
        %82 = "tensor.empty"() : () -> tensor<16x1xf32>
        %83 = "hivm.hir.vreduce"(%81, %82) <{arith = #hivm.reduce_op<max>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 1>, tie_break_left = true, unsigned_src = false}> : (tensor<16x256xf32>, tensor<16x1xf32>) -> tensor<16x1xf32>
        %84 = "tensor.collapse_shape"(%83) <{reassociation = [[0, 1]]}> : (tensor<16x1xf32>) -> tensor<16xf32>
        %85 = "hivm.hir.vbrc"(%83, %19) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xf32>, tensor<16x256xf32>) -> tensor<16x256xf32>
        %86 = "hivm.hir.vsub"(%81, %85, %19) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x256xf32>, tensor<16x256xf32>, tensor<16x256xf32>) -> tensor<16x256xf32>
        %87 = "hivm.hir.vexp"(%86, %19) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16x256xf32>, tensor<16x256xf32>) -> tensor<16x256xf32>
        %88 = "hivm.hir.vreduce"(%87, %82) <{arith = #hivm.reduce_op<sum>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 1>, tie_break_left = true, unsigned_src = false}> : (tensor<16x256xf32>, tensor<16x1xf32>) -> tensor<16x1xf32>
        %89 = "tensor.collapse_shape"(%88) <{reassociation = [[0, 1]]}> : (tensor<16x1xf32>) -> tensor<16xf32>
        %90 = "hivm.hir.vln"(%89, %21) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %91 = "hivm.hir.vadd"(%84, %90, %21) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %92 = "hivm.hir.vadd"(%arg47, %91, %21) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %93 = "hivm.hir.vdiv"(%92, %6, %21) <{broadcast = array<i64>, isHP = false, isSigned = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, f32, tensor<16xf32>) -> tensor<16xf32>
        %94 = "hivm.hir.vsub"(%arg47, %93, %21) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %95 = "hivm.hir.vexp"(%94, %21) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %96 = "hivm.hir.vsub"(%91, %93, %21) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %97 = "hivm.hir.vexp"(%96, %21) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %98 = "hivm.hir.vadd"(%95, %97, %21) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %99 = "hivm.hir.vln"(%98, %21) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %100 = "hivm.hir.vadd"(%93, %99, %21) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %101 = "tensor.empty"() : () -> tensor<16xi1>
        %102 = "hivm.hir.vcmp"(%100, %100, %101) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xi1>) -> tensor<16xi1>
        %103 = "hivm.hir.vnot"(%102, %101) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xi1>, tensor<16xi1>) -> tensor<16xi1>
        %104 = "hivm.hir.vsel"(%103, %91, %100, %21) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<16xi1>, tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %105 = "hivm.hir.vsub"(%91, %104, %21) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %106 = "hivm.hir.vexp"(%105, %21) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %107 = "hivm.hir.vbrc"(%88, %19) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xf32>, tensor<16x256xf32>) -> tensor<16x256xf32>
        %108 = "hivm.hir.vdiv"(%87, %107, %19) <{broadcast = array<i64>, isHP = false, isSigned = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x256xf32>, tensor<16x256xf32>, tensor<16x256xf32>) -> tensor<16x256xf32>
        %109 = "tensor.empty"() : () -> tensor<16x256xbf16>
        %110 = "hivm.hir.vcast"(%108, %109) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x256xf32>, tensor<16x256xbf16>) -> tensor<16x256xbf16>
        "hivm.hir.store"(%110, %32) : (tensor<16x256xbf16>, memref<16x256xbf16, strided<[?, 1], offset: ?>>) -> ()
        %111 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x256xbf16>
        "hivm.hir.load"(%32, %111) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<16x256xbf16, strided<[?, 1], offset: ?>>, memref<16x256xbf16>) -> ()
        %112 = "bufferization.to_tensor"(%111) <{restrict, writable}> : (memref<16x256xbf16>) -> tensor<16x256xbf16>
        %113 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<256x512xbf16>
        "hivm.hir.load"(%57, %113) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<256x512xbf16, strided<[?, 1], offset: ?>>, memref<256x512xbf16>) -> ()
        %114 = "bufferization.to_tensor"(%113) <{restrict, writable}> : (memref<256x512xbf16>) -> tensor<256x512xbf16>
        %115 = "hivm.hir.mmadL1"(%112, %114, %2, %10, %7, %3, %22) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_for_result_already_inserted = true} : (tensor<16x256xbf16>, tensor<256x512xbf16>, i1, index, index, index, tensor<16x512xf32>) -> tensor<16x512xf32>
        "hivm.hir.fixpipe"(%115, %35) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<16x512xf32>, memref<16x512xf32, strided<[?, 1], offset: ?>>) -> ()
        %116 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x512xf32>
        "hivm.hir.load"(%48, %116) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<16x512xf32, strided<[?, 1], offset: ?>>, memref<16x512xf32>) -> ()
        %117 = "bufferization.to_tensor"(%116) <{restrict, writable}> : (memref<16x512xf32>) -> tensor<16x512xf32>
        %118 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x512xf32>
        "hivm.hir.load"(%35, %118) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<16x512xf32, strided<[?, 1], offset: ?>>, memref<16x512xf32>) -> ()
        %119 = "bufferization.to_tensor"(%118) <{restrict, writable}> : (memref<16x512xf32>) -> tensor<16x512xf32>
        %120 = "hivm.hir.vsub"(%arg47, %104, %21) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %121 = "hivm.hir.vexp"(%120, %21) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %122 = "tensor.expand_shape"(%121) <{reassociation = [[0, 1]], static_output_shape = array<i64: 16, 1>}> : (tensor<16xf32>) -> tensor<16x1xf32>
        %123 = "hivm.hir.vbrc"(%122, %22) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xf32>, tensor<16x512xf32>) -> tensor<16x512xf32>
        %124 = "hivm.hir.vmul"(%117, %123, %22) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x512xf32>, tensor<16x512xf32>, tensor<16x512xf32>) -> tensor<16x512xf32>
        %125 = "tensor.expand_shape"(%106) <{reassociation = [[0, 1]], static_output_shape = array<i64: 16, 1>}> : (tensor<16xf32>) -> tensor<16x1xf32>
        %126 = "hivm.hir.vbrc"(%125, %22) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xf32>, tensor<16x512xf32>) -> tensor<16x512xf32>
        %127 = "hivm.hir.vmul"(%119, %126, %22) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x512xf32>, tensor<16x512xf32>, tensor<16x512xf32>) -> tensor<16x512xf32>
        %128 = "hivm.hir.vadd"(%124, %127, %22) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x512xf32>, tensor<16x512xf32>, tensor<16x512xf32>) -> tensor<16x512xf32>
        "hivm.hir.store"(%128, %48) : (tensor<16x512xf32>, memref<16x512xf32, strided<[?, 1], offset: ?>>) -> ()
        "scf.yield"(%104) : (tensor<16xf32>) -> ()
      }) {fixpipe_for_mmad_result_already_inserted = true} : (i32, i32, i32, tensor<16xf32>) -> tensor<16xf32>
      "scf.yield"() : () -> ()
    }) : (i32, i32, i32) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false]> : vector<45xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<MIX>, mix_mode = "mix", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hacc.target = #hacc.target<"Ascend910B1">, hivm.module_core_type = #hivm.module_core_type<MIX>} : () -> ()

