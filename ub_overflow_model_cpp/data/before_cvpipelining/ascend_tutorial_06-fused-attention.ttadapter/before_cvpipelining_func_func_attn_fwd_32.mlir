#map = affine_map<()[s0] -> (s0 * 64)>
#map1 = affine_map<()[s0, s1] -> (s0 + s1)>
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
    "annotation.mark"(%17) <{effects = ["write"]}> {logical_block_num} : (i32) -> ()
    %18 = "hivm.hir.get_block_idx"() : () -> i64
    %19 = "arith.trunci"(%18) : (i64) -> i32
    %20 = "arith.muli"(%arg12, %arg11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %21 = "arith.divsi"(%19, %20) : (i32, i32) -> i32
    %22 = "arith.remsi"(%21, %arg10) : (i32, i32) -> i32
    %23 = "tensor.empty"() : () -> tensor<64x64xf32>
    %24 = "hivm.hir.vbrc"(%14, %23) <{broadcast_dims = array<i64>}> : (f32, tensor<64x64xf32>) -> tensor<64x64xf32>
    %25 = "tensor.empty"() : () -> tensor<64x128xf32>
    %26 = "tensor.empty"() : () -> tensor<64xf32>
    %27 = "hivm.hir.vbrc"(%3, %26) <{broadcast_dims = array<i64>}> : (f32, tensor<64xf32>) -> tensor<64xf32>
    %28 = "hivm.hir.vbrc"(%2, %26) <{broadcast_dims = array<i64>}> : (f32, tensor<64xf32>) -> tensor<64xf32>
    "scf.for"(%22, %12, %13) ({
    ^bb0(%arg13: i32):
      %29 = "arith.divsi"(%arg13, %5) : (i32, i32) -> i32
      %30 = "arith.remsi"(%arg13, %5) : (i32, i32) -> i32
      %31 = "arith.divsi"(%29, %6) : (i32, i32) -> i32
      %32 = "arith.remsi"(%29, %6) : (i32, i32) -> i32
      %33 = "arith.extsi"(%31) : (i32) -> i64
      %34 = "arith.muli"(%33, %7) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
      %35 = "arith.extsi"(%32) : (i32) -> i64
      %36 = "arith.muli"(%35, %8) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
      %37 = "arith.addi"(%34, %36) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
      %38 = "arith.index_cast"(%37) : (i64) -> index
      %39 = "arith.muli"(%30, %9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %40 = "arith.maxsi"(%39, %10) : (i32, i32) -> i32
      %41 = "arith.index_cast"(%40) : (i32) -> index
      %42 = "affine.apply"(%41) <{map = #map}> : (index) -> index
      %43 = "affine.apply"(%42, %38) <{map = #map1}> : (index, index) -> index
      %44 = "memref.reinterpret_cast"(%arg3, %43) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64, 64>, static_strides = array<i64: 64, 1>}> : (memref<?xbf16>, index) -> memref<64x64xbf16, strided<[64, 1], offset: ?>>
      %45 = "memref.reinterpret_cast"(%arg7, %43) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64, 64>, static_strides = array<i64: 64, 1>}> : (memref<?xbf16>, index) -> memref<64x64xbf16, strided<[64, 1], offset: ?>>
      %46 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<64x64xbf16>
      "hivm.hir.load"(%44, %46) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<64x64xbf16, strided<[64, 1], offset: ?>>, memref<64x64xbf16>) -> ()
      %47 = "bufferization.to_tensor"(%46) <{restrict, writable}> : (memref<64x64xbf16>) -> tensor<64x64xbf16>
      %48:5 = "scf.for"(%10, %11, %4, %24, %28, %27, %10, %10) ({
      ^bb0(%arg14: i32, %arg15: tensor<64x64xf32>, %arg16: tensor<64xf32>, %arg17: tensor<64xf32>, %arg18: i32, %arg19: i32):
        %61 = "arith.index_cast"(%arg18) : (i32) -> index
        %62 = "affine.apply"(%61) <{map = #map}> : (index) -> index
        %63 = "affine.apply"(%62, %38) <{map = #map1}> : (index, index) -> index
        %64 = "memref.reinterpret_cast"(%arg4, %63) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 128, 64>, static_strides = array<i64: 64, 1>}> : (memref<?xbf16>, index) -> memref<128x64xbf16, strided<[64, 1], offset: ?>>
        %65 = "arith.index_cast"(%arg19) : (i32) -> index
        %66 = "affine.apply"(%65) <{map = #map}> : (index) -> index
        %67 = "affine.apply"(%66, %38) <{map = #map1}> : (index, index) -> index
        %68 = "memref.reinterpret_cast"(%arg5, %67) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 128, 64>, static_strides = array<i64: 64, 1>}> : (memref<?xbf16>, index) -> memref<128x64xbf16, strided<[64, 1], offset: ?>>
        %69 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<128x64xbf16>
        "hivm.hir.load"(%64, %69) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<128x64xbf16, strided<[64, 1], offset: ?>>, memref<128x64xbf16>) -> ()
        %70 = "bufferization.to_tensor"(%69) <{restrict, writable}> : (memref<128x64xbf16>) -> tensor<128x64xbf16>
        %71 = "tensor.empty"() : () -> tensor<64x128xbf16>
        %72 = "hivm.hir.mmadL1"(%47, %70, %0, %1, %1, %15, %25) <{b_transpose, operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_for_result_already_inserted = true} : (tensor<64x64xbf16>, tensor<128x64xbf16>, i1, index, index, index, tensor<64x128xf32>) -> tensor<64x128xf32>
        %73 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<64x128xf32>
        %74 = "bufferization.to_tensor"(%73) <{restrict, writable}> : (memref<64x128xf32>) -> tensor<64x128xf32>
        %75 = "hivm.hir.fixpipe"(%72, %74) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<64x128xf32>, tensor<64x128xf32>) -> tensor<64x128xf32>
        %76 = "hivm.hir.load"(%75, %25) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<64x128xf32>, tensor<64x128xf32>) -> tensor<64x128xf32>
        %77 = "hivm.hir.vmul"(%76, %arg9, %25) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x128xf32>, f32, tensor<64x128xf32>) -> tensor<64x128xf32>
        %78 = "tensor.empty"() : () -> tensor<64x1xf32>
        %79 = "hivm.hir.vreduce"(%77, %78) <{arith = #hivm.reduce_op<max>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 1>, tie_break_left = true, unsigned_src = false}> : (tensor<64x128xf32>, tensor<64x1xf32>) -> tensor<64x1xf32>
        %80 = "tensor.collapse_shape"(%79) <{reassociation = [[0, 1]]}> : (tensor<64x1xf32>) -> tensor<64xf32>
        %81 = "tensor.empty"() : () -> tensor<64xi1>
        %82 = "hivm.hir.vcmp"(%arg17, %arg17, %81) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>, tensor<64xi1>) -> tensor<64xi1>
        %83 = "hivm.hir.vnot"(%82, %81) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<64xi1>, tensor<64xi1>) -> tensor<64xi1>
        %84 = "hivm.hir.vcmp"(%80, %80, %81) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>, tensor<64xi1>) -> tensor<64xi1>
        %85 = "hivm.hir.vnot"(%84, %81) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<64xi1>, tensor<64xi1>) -> tensor<64xi1>
        %86 = "hivm.hir.vmax"(%arg17, %80, %26) <{broadcast = array<i64>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
        %87 = "hivm.hir.vsel"(%83, %80, %86, %26) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<64xi1>, tensor<64xf32>, tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
        %88 = "hivm.hir.vsel"(%85, %arg17, %87, %26) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<64xi1>, tensor<64xf32>, tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
        %89 = "tensor.expand_shape"(%88) <{reassociation = [[0, 1]], static_output_shape = array<i64: 64, 1>}> : (tensor<64xf32>) -> tensor<64x1xf32>
        %90 = "hivm.hir.vbrc"(%89, %25) <{broadcast_dims = array<i64: 1>}> : (tensor<64x1xf32>, tensor<64x128xf32>) -> tensor<64x128xf32>
        %91 = "hivm.hir.vsub"(%77, %90, %25) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x128xf32>, tensor<64x128xf32>, tensor<64x128xf32>) -> tensor<64x128xf32>
        %92 = "hivm.hir.vexp"(%91, %25) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<64x128xf32>, tensor<64x128xf32>) -> tensor<64x128xf32>
        %93 = "hivm.hir.vcast"(%92, %71) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<64x128xf32>, tensor<64x128xbf16>) -> tensor<64x128xbf16>
        %94 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<64x128xbf16>
        %95 = "bufferization.to_tensor"(%94) <{restrict, writable}> : (memref<64x128xbf16>) -> tensor<64x128xbf16>
        %96 = "hivm.hir.store"(%93, %95) {"inserted-store"} : (tensor<64x128xbf16>, tensor<64x128xbf16>) -> tensor<64x128xbf16>
        %97 = "hivm.hir.load"(%96, %71) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<64x128xbf16>, tensor<64x128xbf16>) -> tensor<64x128xbf16>
        %98 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<128x64xbf16>
        "hivm.hir.load"(%68, %98) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<128x64xbf16, strided<[64, 1], offset: ?>>, memref<128x64xbf16>) -> ()
        %99 = "bufferization.to_tensor"(%98) <{restrict, writable}> : (memref<128x64xbf16>) -> tensor<128x64xbf16>
        %100 = "hivm.hir.vreduce"(%92, %78) <{arith = #hivm.reduce_op<sum>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 1>, tie_break_left = true, unsigned_src = false}> : (tensor<64x128xf32>, tensor<64x1xf32>) -> tensor<64x1xf32>
        %101 = "tensor.collapse_shape"(%100) <{reassociation = [[0, 1]]}> : (tensor<64x1xf32>) -> tensor<64xf32>
        %102 = "hivm.hir.vsub"(%arg17, %88, %26) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
        %103 = "hivm.hir.vexp"(%102, %26) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
        %104 = "hivm.hir.vmul"(%arg16, %103, %26) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
        %105 = "hivm.hir.vadd"(%104, %101, %26) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
        %106 = "tensor.expand_shape"(%103) <{reassociation = [[0, 1]], static_output_shape = array<i64: 64, 1>}> : (tensor<64xf32>) -> tensor<64x1xf32>
        %107 = "hivm.hir.vbrc"(%106, %23) <{broadcast_dims = array<i64: 1>}> : (tensor<64x1xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
        %108 = "hivm.hir.vmul"(%arg15, %107, %23) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x64xf32>, tensor<64x64xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
        %109 = "hivm.hir.mmadL1"(%97, %99, %0, %1, %15, %1, %23) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_for_result_already_inserted = true} : (tensor<64x128xbf16>, tensor<128x64xbf16>, i1, index, index, index, tensor<64x64xf32>) -> tensor<64x64xf32>
        %110 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<64x64xf32>
        %111 = "bufferization.to_tensor"(%110) <{restrict, writable}> : (memref<64x64xf32>) -> tensor<64x64xf32>
        %112 = "hivm.hir.fixpipe"(%109, %111) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<64x64xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
        %113 = "hivm.hir.load"(%112, %23) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<64x64xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
        %114 = "hivm.hir.vadd"(%113, %108, %23) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x64xf32>, tensor<64x64xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
        %115 = "arith.addi"(%arg18, %4) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
        %116 = "arith.addi"(%arg19, %4) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
        "scf.yield"(%114, %105, %88, %115, %116) : (tensor<64x64xf32>, tensor<64xf32>, tensor<64xf32>, i32, i32) -> ()
      }) {fixpipe_for_mmad_result_already_inserted = true} : (i32, i32, i32, tensor<64x64xf32>, tensor<64xf32>, tensor<64xf32>, i32, i32) -> (tensor<64x64xf32>, tensor<64xf32>, tensor<64xf32>, i32, i32)
      %49 = "hivm.hir.vln"(%48#1, %26) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
      %50 = "hivm.hir.vadd"(%48#2, %49, %26) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xf32>, tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
      %51 = "tensor.expand_shape"(%48#1) <{reassociation = [[0, 1]], static_output_shape = array<i64: 64, 1>}> : (tensor<64xf32>) -> tensor<64x1xf32>
      %52 = "hivm.hir.vbrc"(%51, %23) <{broadcast_dims = array<i64: 1>}> : (tensor<64x1xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
      %53 = "hivm.hir.vdiv"(%48#0, %52, %23) <{broadcast = array<i64>, isHP = false, isSigned = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x64xf32>, tensor<64x64xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
      %54 = "arith.muli"(%29, %11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %55 = "arith.index_cast"(%54) : (i32) -> index
      %56 = "arith.index_cast"(%39) : (i32) -> index
      %57 = "affine.apply"(%55, %56) <{map = #map1}> : (index, index) -> index
      %58 = "memref.reinterpret_cast"(%arg6, %57) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<64xf32, strided<[1], offset: ?>>
      "hivm.hir.store"(%50, %58) : (tensor<64xf32>, memref<64xf32, strided<[1], offset: ?>>) -> ()
      %59 = "tensor.empty"() : () -> tensor<64x64xbf16>
      %60 = "hivm.hir.vcast"(%53, %59) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<64x64xf32>, tensor<64x64xbf16>) -> tensor<64x64xbf16>
      "hivm.hir.store"(%60, %45) : (tensor<64x64xbf16>, memref<64x64xbf16, strided<[64, 1], offset: ?>>) -> ()
      "scf.yield"() : () -> ()
    }) : (i32, i32, i32) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, true, true, true, false, false, false, false]> : vector<13xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<MIX>, mix_mode = "mix", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hacc.target = #hacc.target<"Ascend910B1">, hivm.module_core_type = #hivm.module_core_type<MIX>} : () -> ()

