#map = affine_map<()[s0] -> (s0 + 512)>
#map1 = affine_map<()[s0, s1] -> (s0 * s1)>
#map2 = affine_map<()[s0, s1] -> (s0 + s1)>
#map3 = affine_map<()[s0] -> (s0 + 128)>
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
    %4 = "arith.constant"() <{value = 2 : i32}> : () -> i32
    %5 = "arith.constant"() <{value = 8 : i32}> : () -> i32
    %6 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %7 = "arith.constant"() <{value = 256 : i32}> : () -> i32
    %8 = "arith.constant"() <{value = 2.000000e+00 : f32}> : () -> f32
    %9 = "arith.constant"() <{value = 128 : index}> : () -> index
    %10 = "arith.constant"() <{value = 128 : i32}> : () -> i32
    %11 = "arith.constant"() <{value = 0xFF800000 : f32}> : () -> f32
    %12 = "arith.constant"() <{value = 16 : index}> : () -> index
    %13 = "arith.constant"() <{value = 64 : index}> : () -> index
    %14 = "arith.constant"() <{value = 256 : index}> : () -> index
    "hivm.hir.set_mask_norm"() : () -> ()
    %15 = "arith.muli"(%arg42, %arg43) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %16 = "arith.muli"(%15, %arg44) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%16) <{effects = ["write"]}> {logical_block_num} : (i32) -> ()
    %17 = "hivm.hir.get_block_idx"() : () -> i64
    %18 = "arith.trunci"(%17) : (i64) -> i32
    %19 = "arith.muli"(%arg44, %arg43) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %20 = "arith.divsi"(%18, %19) : (i32, i32) -> i32
    %21 = "arith.remsi"(%20, %arg42) : (i32, i32) -> i32
    %22 = "tensor.empty"() : () -> tensor<16x128xf32>
    %23 = "hivm.hir.vbrc"(%11, %22) <{broadcast_dims = array<i64>}> : (f32, tensor<16x128xf32>) -> tensor<16x128xf32>
    %24 = "tensor.empty"() : () -> tensor<16xf32>
    %25 = "tensor.empty"() : () -> tensor<16x256xf32>
    %26 = "hivm.hir.vbrc"(%11, %24) <{broadcast_dims = array<i64>}> : (f32, tensor<16xf32>) -> tensor<16xf32>
    %27 = "arith.muli"(%21, %arg41) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %28 = "arith.addi"(%21, %0) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %29 = "arith.muli"(%28, %arg41) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %30 = "arith.minsi"(%arg37, %29) : (i32, i32) -> i32
    %31 = "arith.muli"(%21, %arg32) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %32 = "arith.muli"(%21, %arg34) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %33 = "arith.index_cast"(%31) : (i32) -> index
    %34 = "arith.index_cast"(%arg33) : (i32) -> index
    %35 = "memref.reinterpret_cast"(%arg11, %33, %34) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 128>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<16x128xbf16, strided<[?, 1], offset: ?>>
    %36 = "arith.index_cast"(%32) : (i32) -> index
    %37 = "arith.index_cast"(%arg35) : (i32) -> index
    "scf.for"(%27, %30, %0) ({
    ^bb0(%arg45: i32):
      %38 = "arith.muli"(%arg45, %arg13) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %39 = "arith.muli"(%arg45, %arg24) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %40 = "arith.muli"(%arg45, %arg27) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %41 = "arith.muli"(%arg45, %arg22) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %42 = "arith.index_cast"(%38) : (i32) -> index
      %43 = "arith.index_cast"(%arg15) : (i32) -> index
      %44 = "memref.reinterpret_cast"(%arg3, %42, %43) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 512>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<16x512xbf16, strided<[?, 1], offset: ?>>
      %45 = "affine.apply"(%42) <{map = #map}> : (index) -> index
      %46 = "memref.reinterpret_cast"(%arg3, %45, %43) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 64>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<16x64xbf16, strided<[?, 1], offset: ?>>
      %47 = "arith.index_cast"(%39) : (i32) -> index
      %48 = "arith.index_cast"(%41) : (i32) -> index
      %49 = "arith.index_cast"(%arg23) : (i32) -> index
      %50 = "scf.for"(%6, %5, %0, %26) ({
      ^bb0(%arg46: i32, %arg47: tensor<16xf32>):
        %51 = "arith.muli"(%arg46, %10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
        %52 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x512xbf16>
        "hivm.hir.load"(%44, %52) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<16x512xbf16, strided<[?, 1], offset: ?>>, memref<16x512xbf16>) -> ()
        %53 = "bufferization.to_tensor"(%52) <{restrict, writable}> : (memref<16x512xbf16>) -> tensor<16x512xbf16>
        %54 = "arith.index_cast"(%51) : (i32) -> index
        %55 = "arith.index_cast"(%arg26) : (i32) -> index
        %56 = "affine.apply"(%54, %55) <{map = #map1}> : (index, index) -> index
        %57 = "affine.apply"(%47, %56) <{map = #map2}> : (index, index) -> index
        %58 = "memref.reinterpret_cast"(%arg8, %57, %55) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 128, 512>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<128x512xbf16, strided<[?, 1], offset: ?>>
        %59 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<128x512xbf16>
        "hivm.hir.load"(%58, %59) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<128x512xbf16, strided<[?, 1], offset: ?>>, memref<128x512xbf16>) -> ()
        %60 = "bufferization.to_tensor"(%59) <{restrict, writable}> : (memref<128x512xbf16>) -> tensor<128x512xbf16>
        %61 = "hivm.hir.mmadL1"(%53, %60, %2, %12, %3, %9, %22) <{b_transpose, operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> : (tensor<16x512xbf16>, tensor<128x512xbf16>, i1, index, index, index, tensor<16x128xf32>) -> tensor<16x128xf32>
        %62 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x64xbf16>
        "hivm.hir.load"(%46, %62) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<16x64xbf16, strided<[?, 1], offset: ?>>, memref<16x64xbf16>) -> ()
        %63 = "bufferization.to_tensor"(%62) <{restrict, writable}> : (memref<16x64xbf16>) -> tensor<16x64xbf16>
        %64 = "arith.index_cast"(%40) : (i32) -> index
        %65 = "arith.index_cast"(%arg29) : (i32) -> index
        %66 = "affine.apply"(%54, %65) <{map = #map1}> : (index, index) -> index
        %67 = "affine.apply"(%64, %66) <{map = #map2}> : (index, index) -> index
        %68 = "memref.reinterpret_cast"(%arg9, %67, %65) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 128, 64>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<128x64xbf16, strided<[?, 1], offset: ?>>
        %69 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<128x64xbf16>
        "hivm.hir.load"(%68, %69) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<128x64xbf16, strided<[?, 1], offset: ?>>, memref<128x64xbf16>) -> ()
        %70 = "bufferization.to_tensor"(%69) <{restrict, writable}> : (memref<128x64xbf16>) -> tensor<128x64xbf16>
        %71 = "hivm.hir.mmadL1"(%63, %70, %1, %12, %13, %9, %61) <{b_transpose, operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_for_result_already_inserted = true} : (tensor<16x64xbf16>, tensor<128x64xbf16>, i1, index, index, index, tensor<16x128xf32>) -> tensor<16x128xf32>
        %72 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x128xf32>
        %73 = "bufferization.to_tensor"(%72) <{restrict, writable}> : (memref<16x128xf32>) -> tensor<16x128xf32>
        %74 = "hivm.hir.fixpipe"(%71, %73) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<16x128xf32>, tensor<16x128xf32>) -> tensor<16x128xf32>
        %75 = "hivm.hir.load"(%74, %22) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<16x128xf32>, tensor<16x128xf32>) -> tensor<16x128xf32>
        %76 = "hivm.hir.vmul"(%75, %arg36, %22) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x128xf32>, f32, tensor<16x128xf32>) -> tensor<16x128xf32>
        %77 = "affine.apply"(%54) <{map = #map3}> : (index) -> index
        %78 = "affine.max"(%54) <{map = #map4}> : (index) -> index
        %79 = "affine.min"(%77, %78) <{map = #map5}> : (index, index) -> index
        %80 = "affine.apply"(%79, %54) <{map = #map6}> : (index, index) -> index
        %81 = "tensor.extract_slice"(%76, %80) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: 16, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<16x128xf32>, index) -> tensor<16x?xf32>
        %82 = "tensor.insert_slice"(%81, %23, %80) <{operandSegmentSizes = array<i32: 1, 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: 16, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<16x?xf32>, tensor<16x128xf32>, index) -> tensor<16x128xf32>
        %83 = "tensor.empty"() : () -> tensor<16x1xf32>
        %84 = "hivm.hir.vreduce"(%82, %83) <{arith = #hivm.reduce_op<max>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 1>, tie_break_left = true, unsigned_src = false}> : (tensor<16x128xf32>, tensor<16x1xf32>) -> tensor<16x1xf32>
        %85 = "tensor.collapse_shape"(%84) <{reassociation = [[0, 1]]}> : (tensor<16x1xf32>) -> tensor<16xf32>
        %86 = "hivm.hir.vbrc"(%84, %22) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xf32>, tensor<16x128xf32>) -> tensor<16x128xf32>
        %87 = "hivm.hir.vsub"(%82, %86, %22) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x128xf32>, tensor<16x128xf32>, tensor<16x128xf32>) -> tensor<16x128xf32>
        %88 = "hivm.hir.vexp"(%87, %22) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16x128xf32>, tensor<16x128xf32>) -> tensor<16x128xf32>
        %89 = "hivm.hir.vreduce"(%88, %83) <{arith = #hivm.reduce_op<sum>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 1>, tie_break_left = true, unsigned_src = false}> : (tensor<16x128xf32>, tensor<16x1xf32>) -> tensor<16x1xf32>
        %90 = "tensor.collapse_shape"(%89) <{reassociation = [[0, 1]]}> : (tensor<16x1xf32>) -> tensor<16xf32>
        %91 = "hivm.hir.vln"(%90, %24) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %92 = "hivm.hir.vadd"(%85, %91, %24) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %93 = "hivm.hir.vadd"(%arg47, %92, %24) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %94 = "hivm.hir.vdiv"(%93, %8, %24) <{broadcast = array<i64>, isHP = false, isSigned = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, f32, tensor<16xf32>) -> tensor<16xf32>
        %95 = "hivm.hir.vsub"(%arg47, %94, %24) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %96 = "hivm.hir.vexp"(%95, %24) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %97 = "hivm.hir.vsub"(%92, %94, %24) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %98 = "hivm.hir.vexp"(%97, %24) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %99 = "hivm.hir.vadd"(%96, %98, %24) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %100 = "hivm.hir.vln"(%99, %24) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %101 = "hivm.hir.vadd"(%94, %100, %24) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %102 = "tensor.empty"() : () -> tensor<16xi1>
        %103 = "hivm.hir.vcmp"(%101, %101, %102) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xi1>) -> tensor<16xi1>
        %104 = "hivm.hir.vnot"(%103, %102) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xi1>, tensor<16xi1>) -> tensor<16xi1>
        %105 = "hivm.hir.vsel"(%104, %92, %101, %24) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<16xi1>, tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %106 = "hivm.hir.vsub"(%92, %105, %24) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %107 = "hivm.hir.vexp"(%106, %24) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %108 = "hivm.hir.vbrc"(%89, %22) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xf32>, tensor<16x128xf32>) -> tensor<16x128xf32>
        %109 = "hivm.hir.vdiv"(%88, %108, %22) <{broadcast = array<i64>, isHP = false, isSigned = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x128xf32>, tensor<16x128xf32>, tensor<16x128xf32>) -> tensor<16x128xf32>
        %110 = "tensor.empty"() : () -> tensor<16x128xbf16>
        %111 = "hivm.hir.vcast"(%109, %110) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x128xf32>, tensor<16x128xbf16>) -> tensor<16x128xbf16>
        "hivm.hir.store"(%111, %35) : (tensor<16x128xbf16>, memref<16x128xbf16, strided<[?, 1], offset: ?>>) -> ()
        %112 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x128xbf16>
        "hivm.hir.load"(%35, %112) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<16x128xbf16, strided<[?, 1], offset: ?>>, memref<16x128xbf16>) -> ()
        %113 = "bufferization.to_tensor"(%112) <{restrict, writable}> : (memref<16x128xbf16>) -> tensor<16x128xbf16>
        %114 = "hivm.hir.vsub"(%arg47, %105, %24) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %115 = "hivm.hir.vexp"(%114, %24) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %116 = "tensor.expand_shape"(%115) <{reassociation = [[0, 1]], static_output_shape = array<i64: 16, 1>}> : (tensor<16xf32>) -> tensor<16x1xf32>
        %117 = "hivm.hir.vbrc"(%116, %25) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xf32>, tensor<16x256xf32>) -> tensor<16x256xf32>
        %118 = "tensor.expand_shape"(%107) <{reassociation = [[0, 1]], static_output_shape = array<i64: 16, 1>}> : (tensor<16xf32>) -> tensor<16x1xf32>
        %119 = "hivm.hir.vbrc"(%118, %25) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xf32>, tensor<16x256xf32>) -> tensor<16x256xf32>
        "scf.for"(%6, %4, %0) ({
        ^bb0(%arg48: i32):
          %120 = "arith.muli"(%arg48, %7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
          %121 = "arith.index_cast"(%120) : (i32) -> index
          %122 = "affine.apply"(%57, %121) <{map = #map2}> : (index, index) -> index
          %123 = "memref.reinterpret_cast"(%arg8, %122, %55) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 128, 256>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xbf16>, index, index) -> memref<128x256xbf16, strided<[?, 1], offset: ?>>
          %124 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<128x256xbf16>
          "hivm.hir.load"(%123, %124) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<128x256xbf16, strided<[?, 1], offset: ?>>, memref<128x256xbf16>) -> ()
          %125 = "bufferization.to_tensor"(%124) <{restrict, writable}> : (memref<128x256xbf16>) -> tensor<128x256xbf16>
          %126 = "hivm.hir.mmadL1"(%113, %125, %2, %12, %9, %14, %25) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_for_result_already_inserted = true} : (tensor<16x128xbf16>, tensor<128x256xbf16>, i1, index, index, index, tensor<16x256xf32>) -> tensor<16x256xf32>
          %127 = "affine.apply"(%36, %121) <{map = #map2}> : (index, index) -> index
          %128 = "memref.reinterpret_cast"(%arg12, %127, %37) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 256>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xf32>, index, index) -> memref<16x256xf32, strided<[?, 1], offset: ?>>
          "hivm.hir.fixpipe"(%126, %128) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<16x256xf32>, memref<16x256xf32, strided<[?, 1], offset: ?>>) -> ()
          %129 = "affine.apply"(%48, %121) <{map = #map2}> : (index, index) -> index
          %130 = "memref.reinterpret_cast"(%arg7, %129, %49) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 256>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xf32>, index, index) -> memref<16x256xf32, strided<[?, 1], offset: ?>>
          %131 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x256xf32>
          "hivm.hir.load"(%130, %131) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<16x256xf32, strided<[?, 1], offset: ?>>, memref<16x256xf32>) -> ()
          %132 = "bufferization.to_tensor"(%131) <{restrict, writable}> : (memref<16x256xf32>) -> tensor<16x256xf32>
          %133 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x256xf32>
          "hivm.hir.load"(%128, %133) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<16x256xf32, strided<[?, 1], offset: ?>>, memref<16x256xf32>) -> ()
          %134 = "bufferization.to_tensor"(%133) <{restrict, writable}> : (memref<16x256xf32>) -> tensor<16x256xf32>
          %135 = "hivm.hir.vmul"(%132, %117, %25) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x256xf32>, tensor<16x256xf32>, tensor<16x256xf32>) -> tensor<16x256xf32>
          %136 = "hivm.hir.vmul"(%134, %119, %25) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x256xf32>, tensor<16x256xf32>, tensor<16x256xf32>) -> tensor<16x256xf32>
          %137 = "hivm.hir.vadd"(%135, %136, %25) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x256xf32>, tensor<16x256xf32>, tensor<16x256xf32>) -> tensor<16x256xf32>
          "hivm.hir.store"(%137, %130) : (tensor<16x256xf32>, memref<16x256xf32, strided<[?, 1], offset: ?>>) -> ()
          "scf.yield"() : () -> ()
        }) {fixpipe_for_mmad_result_already_inserted = true} : (i32, i32, i32) -> ()
        "scf.yield"(%105) : (tensor<16xf32>) -> ()
      }) {fixpipe_for_mmad_result_already_inserted = true} : (i32, i32, i32, tensor<16xf32>) -> tensor<16xf32>
      "scf.yield"() : () -> ()
    }) : (i32, i32, i32) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false]> : vector<45xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<MIX>, mix_mode = "mix", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hacc.target = #hacc.target<"Ascend910B1">, hivm.module_core_type = #hivm.module_core_type<MIX>} : () -> ()

