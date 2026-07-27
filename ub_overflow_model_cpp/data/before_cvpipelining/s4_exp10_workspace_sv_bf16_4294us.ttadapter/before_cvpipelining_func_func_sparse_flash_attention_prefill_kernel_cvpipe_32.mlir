#map = affine_map<()[s0] -> (s0 + 512)>
#map1 = affine_map<()[s0, s1] -> (s0 * s1)>
#map2 = affine_map<()[s0, s1] -> (s0 + s1)>
#map3 = affine_map<()[s0] -> (s0 + 256)>
#map4 = affine_map<()[s0] -> (s0, 1024)>
#map5 = affine_map<()[s0, s1] -> (s0, s1)>
#map6 = affine_map<()[s0, s1] -> (s0 - s1)>
"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 2 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 2 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 2 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {}, {}, {}, {}, {}, {}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, memref<?xi32>, memref<?xf32>, memref<?xbf16>, memref<?xbf16>, memref<?xf32>, memref<?xbf16>, memref<?xbf16>, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, f32, i32, i32, i32, i32, i32, i32, i32, i32) -> (), sym_name = "sparse_flash_attention_prefill_kernel_cvpipe"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xbf16>, %arg4: memref<?xbf16>, %arg5: memref<?xbf16>, %arg6: memref<?xi32>, %arg7: memref<?xf32>, %arg8: memref<?xbf16>, %arg9: memref<?xbf16>, %arg10: memref<?xf32>, %arg11: memref<?xbf16>, %arg12: memref<?xbf16>, %arg13: i32, %arg14: i32, %arg15: i32, %arg16: i32, %arg17: i32, %arg18: i32, %arg19: i32, %arg20: i32, %arg21: i32, %arg22: i32, %arg23: i32, %arg24: i32, %arg25: i32, %arg26: i32, %arg27: i32, %arg28: i32, %arg29: i32, %arg30: i32, %arg31: i32, %arg32: i32, %arg33: i32, %arg34: i32, %arg35: i32, %arg36: f32, %arg37: i32, %arg38: i32, %arg39: i32, %arg40: i32, %arg41: i32, %arg42: i32, %arg43: i32, %arg44: i32):
    %0 = "arith.constant"() <{value = 1 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = false}> : () -> i1
    %2 = "arith.constant"() <{value = true}> : () -> i1
    %3 = "arith.constant"() <{value = 512 : index}> : () -> index
    %4 = "arith.constant"() <{value = 4 : i32}> : () -> i32
    %5 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %6 = "arith.constant"() <{value = 256 : index}> : () -> index
    %7 = "arith.constant"() <{value = 256 : i32}> : () -> i32
    %8 = "arith.constant"() <{value = 0xFF800000 : f32}> : () -> f32
    %9 = "arith.constant"() <{value = 16 : index}> : () -> index
    %10 = "arith.constant"() <{value = 64 : index}> : () -> index
    "hivm.hir.set_mask_norm"() : () -> ()
    %11 = "arith.muli"(%arg42, %arg43) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %12 = "arith.muli"(%11, %arg44) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%12) <{effects = ["write"]}> {logical_block_num} : (i32) -> ()
    %13 = "hivm.hir.get_block_idx"() : () -> i64
    %14 = "arith.trunci"(%13) : (i64) -> i32
    %15 = "arith.muli"(%arg44, %arg43) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %16 = "arith.divsi"(%14, %15) : (i32, i32) -> i32
    %17 = "arith.remsi"(%16, %arg42) : (i32, i32) -> i32
    %18 = "tensor.empty"() : () -> tensor<16x256xf32>
    %19 = "hivm.hir.vbrc"(%8, %18) <{broadcast_dims = array<i64>}> : (f32, tensor<16x256xf32>) -> tensor<16x256xf32>
    %20 = "tensor.empty"() : () -> tensor<16x512xf32>
    %21 = "tensor.empty"() : () -> tensor<16xf32>
    %22 = "hivm.hir.vbrc"(%8, %21) <{broadcast_dims = array<i64>}> : (f32, tensor<16xf32>) -> tensor<16xf32>
    %23 = "arith.muli"(%17, %arg41) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %24 = "arith.addi"(%17, %0) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %25 = "arith.muli"(%24, %arg41) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %26 = "arith.minsi"(%arg37, %25) : (i32, i32) -> i32
    %27 = "arith.muli"(%17, %arg32) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %28 = "arith.muli"(%17, %arg34) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %29 = "arith.index_cast"(%27) : (i32) -> index
    %30 = "arith.index_cast"(%arg33) : (i32) -> index
    %31 = "memref.reinterpret_cast"(%arg11, %29, %30) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 256>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<16x256xbf16, strided<[?, 1], offset: ?>>
    %32 = "arith.index_cast"(%28) : (i32) -> index
    %33 = "arith.index_cast"(%arg35) : (i32) -> index
    %34 = "memref.reinterpret_cast"(%arg12, %32, %33) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 512>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<16x512xbf16, strided<[?, 1], offset: ?>>
    "scf.for"(%23, %26, %0) ({
    ^bb0(%arg45: i32):
      %35 = "arith.muli"(%arg45, %arg13) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %36 = "arith.muli"(%arg45, %arg24) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %37 = "arith.muli"(%arg45, %arg27) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %38 = "arith.muli"(%arg45, %arg22) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %39 = "arith.index_cast"(%35) : (i32) -> index
      %40 = "arith.index_cast"(%arg15) : (i32) -> index
      %41 = "memref.reinterpret_cast"(%arg3, %39, %40) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 512>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<16x512xbf16, strided<[?, 1], offset: ?>>
      %42 = "affine.apply"(%39) <{map = #map}> : (index) -> index
      %43 = "memref.reinterpret_cast"(%arg3, %42, %40) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 64>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<16x64xbf16, strided<[?, 1], offset: ?>>
      %44 = "arith.index_cast"(%36) : (i32) -> index
      %45 = "arith.index_cast"(%38) : (i32) -> index
      %46 = "arith.index_cast"(%arg23) : (i32) -> index
      %47 = "memref.reinterpret_cast"(%arg7, %45, %46) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 512>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xf32>, index, index) -> memref<16x512xf32, strided<[?, 1], offset: ?>>
      %48 = "scf.for"(%5, %4, %0, %22) ({
      ^bb0(%arg46: i32, %arg47: tensor<16xf32>):
        %49 = "arith.muli"(%arg46, %7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
        %50 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x512xbf16>
        "hivm.hir.load"(%41, %50) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<16x512xbf16, strided<[?, 1], offset: ?>>, memref<16x512xbf16>) -> ()
        %51 = "bufferization.to_tensor"(%50) <{restrict, writable}> : (memref<16x512xbf16>) -> tensor<16x512xbf16>
        %52 = "arith.index_cast"(%49) : (i32) -> index
        %53 = "arith.index_cast"(%arg26) : (i32) -> index
        %54 = "affine.apply"(%52, %53) <{map = #map1}> : (index, index) -> index
        %55 = "affine.apply"(%44, %54) <{map = #map2}> : (index, index) -> index
        %56 = "memref.reinterpret_cast"(%arg8, %55, %53) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 256, 512>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<256x512xbf16, strided<[?, 1], offset: ?>>
        %57 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<256x512xbf16>
        "hivm.hir.load"(%56, %57) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<256x512xbf16, strided<[?, 1], offset: ?>>, memref<256x512xbf16>) -> ()
        %58 = "bufferization.to_tensor"(%57) <{restrict, writable}> : (memref<256x512xbf16>) -> tensor<256x512xbf16>
        %59 = "hivm.hir.mmadL1"(%51, %58, %2, %9, %3, %6, %18) <{b_transpose, operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> : (tensor<16x512xbf16>, tensor<256x512xbf16>, i1, index, index, index, tensor<16x256xf32>) -> tensor<16x256xf32>
        %60 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x64xbf16>
        "hivm.hir.load"(%43, %60) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<16x64xbf16, strided<[?, 1], offset: ?>>, memref<16x64xbf16>) -> ()
        %61 = "bufferization.to_tensor"(%60) <{restrict, writable}> : (memref<16x64xbf16>) -> tensor<16x64xbf16>
        %62 = "arith.index_cast"(%37) : (i32) -> index
        %63 = "arith.index_cast"(%arg29) : (i32) -> index
        %64 = "affine.apply"(%52, %63) <{map = #map1}> : (index, index) -> index
        %65 = "affine.apply"(%62, %64) <{map = #map2}> : (index, index) -> index
        %66 = "memref.reinterpret_cast"(%arg9, %65, %63) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 256, 64>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<256x64xbf16, strided<[?, 1], offset: ?>>
        %67 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<256x64xbf16>
        "hivm.hir.load"(%66, %67) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<256x64xbf16, strided<[?, 1], offset: ?>>, memref<256x64xbf16>) -> ()
        %68 = "bufferization.to_tensor"(%67) <{restrict, writable}> : (memref<256x64xbf16>) -> tensor<256x64xbf16>
        %69 = "hivm.hir.mmadL1"(%61, %68, %1, %9, %10, %6, %59) <{b_transpose, operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_for_result_already_inserted = true} : (tensor<16x64xbf16>, tensor<256x64xbf16>, i1, index, index, index, tensor<16x256xf32>) -> tensor<16x256xf32>
        %70 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x256xf32>
        %71 = "bufferization.to_tensor"(%70) <{restrict, writable}> : (memref<16x256xf32>) -> tensor<16x256xf32>
        %72 = "hivm.hir.fixpipe"(%69, %71) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<16x256xf32>, tensor<16x256xf32>) -> tensor<16x256xf32>
        %73 = "hivm.hir.load"(%72, %18) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<16x256xf32>, tensor<16x256xf32>) -> tensor<16x256xf32>
        %74 = "hivm.hir.vmul"(%73, %arg36, %18) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x256xf32>, f32, tensor<16x256xf32>) -> tensor<16x256xf32>
        %75 = "affine.apply"(%52) <{map = #map3}> : (index) -> index
        %76 = "affine.max"(%52) <{map = #map4}> : (index) -> index
        %77 = "affine.min"(%75, %76) <{map = #map5}> : (index, index) -> index
        %78 = "affine.apply"(%77, %52) <{map = #map6}> : (index, index) -> index
        %79 = "tensor.extract_slice"(%74, %78) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: 16, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<16x256xf32>, index) -> tensor<16x?xf32>
        %80 = "tensor.insert_slice"(%79, %19, %78) <{operandSegmentSizes = array<i32: 1, 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: 16, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<16x?xf32>, tensor<16x256xf32>, index) -> tensor<16x256xf32>
        %81 = "tensor.empty"() : () -> tensor<16x1xf32>
        %82 = "hivm.hir.vreduce"(%80, %81) <{arith = #hivm.reduce_op<max>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 1>, tie_break_left = true, unsigned_src = false}> : (tensor<16x256xf32>, tensor<16x1xf32>) -> tensor<16x1xf32>
        %83 = "tensor.collapse_shape"(%82) <{reassociation = [[0, 1]]}> : (tensor<16x1xf32>) -> tensor<16xf32>
        %84 = "hivm.hir.vbrc"(%82, %18) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xf32>, tensor<16x256xf32>) -> tensor<16x256xf32>
        %85 = "hivm.hir.vsub"(%80, %84, %18) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x256xf32>, tensor<16x256xf32>, tensor<16x256xf32>) -> tensor<16x256xf32>
        %86 = "hivm.hir.vexp"(%85, %18) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16x256xf32>, tensor<16x256xf32>) -> tensor<16x256xf32>
        %87 = "hivm.hir.vreduce"(%86, %81) <{arith = #hivm.reduce_op<sum>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 1>, tie_break_left = true, unsigned_src = false}> : (tensor<16x256xf32>, tensor<16x1xf32>) -> tensor<16x1xf32>
        %88 = "tensor.collapse_shape"(%87) <{reassociation = [[0, 1]]}> : (tensor<16x1xf32>) -> tensor<16xf32>
        %89 = "hivm.hir.vln"(%88, %21) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %90 = "hivm.hir.vadd"(%83, %89, %21) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %91 = "tensor.empty"() : () -> tensor<16xi1>
        %92 = "hivm.hir.vcmp"(%arg47, %arg47, %91) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xi1>) -> tensor<16xi1>
        %93 = "hivm.hir.vnot"(%92, %91) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xi1>, tensor<16xi1>) -> tensor<16xi1>
        %94 = "hivm.hir.vcmp"(%90, %90, %91) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xi1>) -> tensor<16xi1>
        %95 = "hivm.hir.vnot"(%94, %91) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xi1>, tensor<16xi1>) -> tensor<16xi1>
        %96 = "hivm.hir.vmax"(%arg47, %90, %21) <{broadcast = array<i64>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %97 = "hivm.hir.vsel"(%93, %90, %96, %21) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<16xi1>, tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %98 = "hivm.hir.vsel"(%95, %arg47, %97, %21) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<16xi1>, tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %99 = "hivm.hir.vsub"(%arg47, %98, %21) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %100 = "hivm.hir.vexp"(%99, %21) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %101 = "hivm.hir.vsub"(%90, %98, %21) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %102 = "hivm.hir.vexp"(%101, %21) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %103 = "hivm.hir.vadd"(%100, %102, %21) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %104 = "hivm.hir.vln"(%103, %21) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %105 = "hivm.hir.vadd"(%98, %104, %21) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %106 = "hivm.hir.vsub"(%90, %105, %21) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %107 = "hivm.hir.vexp"(%106, %21) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %108 = "hivm.hir.vbrc"(%87, %18) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xf32>, tensor<16x256xf32>) -> tensor<16x256xf32>
        %109 = "hivm.hir.vdiv"(%86, %108, %18) <{broadcast = array<i64>, isHP = false, isSigned = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x256xf32>, tensor<16x256xf32>, tensor<16x256xf32>) -> tensor<16x256xf32>
        %110 = "tensor.empty"() : () -> tensor<16x256xbf16>
        %111 = "hivm.hir.vcast"(%109, %110) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x256xf32>, tensor<16x256xbf16>) -> tensor<16x256xbf16>
        "hivm.hir.store"(%111, %31) : (tensor<16x256xbf16>, memref<16x256xbf16, strided<[?, 1], offset: ?>>) -> ()
        %112 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x256xbf16>
        "hivm.hir.load"(%31, %112) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<16x256xbf16, strided<[?, 1], offset: ?>>, memref<16x256xbf16>) -> ()
        %113 = "bufferization.to_tensor"(%112) <{restrict, writable}> : (memref<16x256xbf16>) -> tensor<16x256xbf16>
        %114 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<256x512xbf16>
        "hivm.hir.load"(%56, %114) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<256x512xbf16, strided<[?, 1], offset: ?>>, memref<256x512xbf16>) -> ()
        %115 = "bufferization.to_tensor"(%114) <{restrict, writable}> : (memref<256x512xbf16>) -> tensor<256x512xbf16>
        %116 = "hivm.hir.mmadL1"(%113, %115, %2, %9, %6, %3, %20) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_for_result_already_inserted = true} : (tensor<16x256xbf16>, tensor<256x512xbf16>, i1, index, index, index, tensor<16x512xf32>) -> tensor<16x512xf32>
        "hivm.hir.fixpipe"(%116, %34) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, pre_quant = #hivm.fixpipe_pre_quant_mode<F322BF16>}> : (tensor<16x512xf32>, memref<16x512xbf16, strided<[?, 1], offset: ?>>) -> ()
        %117 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x512xf32>
        "hivm.hir.load"(%47, %117) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<16x512xf32, strided<[?, 1], offset: ?>>, memref<16x512xf32>) -> ()
        %118 = "bufferization.to_tensor"(%117) <{restrict, writable}> : (memref<16x512xf32>) -> tensor<16x512xf32>
        %119 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x512xbf16>
        "hivm.hir.load"(%34, %119) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<16x512xbf16, strided<[?, 1], offset: ?>>, memref<16x512xbf16>) -> ()
        %120 = "bufferization.to_tensor"(%119) <{restrict, writable}> : (memref<16x512xbf16>) -> tensor<16x512xbf16>
        %121 = "hivm.hir.vsub"(%arg47, %105, %21) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %122 = "hivm.hir.vexp"(%121, %21) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %123 = "tensor.expand_shape"(%122) <{reassociation = [[0, 1]], static_output_shape = array<i64: 16, 1>}> : (tensor<16xf32>) -> tensor<16x1xf32>
        %124 = "hivm.hir.vbrc"(%123, %20) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xf32>, tensor<16x512xf32>) -> tensor<16x512xf32>
        %125 = "hivm.hir.vmul"(%118, %124, %20) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x512xf32>, tensor<16x512xf32>, tensor<16x512xf32>) -> tensor<16x512xf32>
        %126 = "hivm.hir.vcast"(%120, %20) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x512xbf16>, tensor<16x512xf32>) -> tensor<16x512xf32>
        %127 = "tensor.expand_shape"(%107) <{reassociation = [[0, 1]], static_output_shape = array<i64: 16, 1>}> : (tensor<16xf32>) -> tensor<16x1xf32>
        %128 = "hivm.hir.vbrc"(%127, %20) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xf32>, tensor<16x512xf32>) -> tensor<16x512xf32>
        %129 = "hivm.hir.vmul"(%126, %128, %20) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x512xf32>, tensor<16x512xf32>, tensor<16x512xf32>) -> tensor<16x512xf32>
        %130 = "hivm.hir.vadd"(%125, %129, %20) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x512xf32>, tensor<16x512xf32>, tensor<16x512xf32>) -> tensor<16x512xf32>
        "hivm.hir.store"(%130, %47) : (tensor<16x512xf32>, memref<16x512xf32, strided<[?, 1], offset: ?>>) -> ()
        "scf.yield"(%105) : (tensor<16xf32>) -> ()
      }) {fixpipe_for_mmad_result_already_inserted = true} : (i32, i32, i32, tensor<16xf32>) -> tensor<16xf32>
      "scf.yield"() : () -> ()
    }) : (i32, i32, i32) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false]> : vector<45xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<MIX>, mix_mode = "mix", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hacc.target = #hacc.target<"Ascend910B1">, hivm.module_core_type = #hivm.module_core_type<MIX>} : () -> ()

