#map = affine_map<()[s0, s1] -> (s0 + s1)>
#map1 = affine_map<()[s0, s1] -> (s0 - s1)>
#map2 = affine_map<()[s0] -> (s0, 0)>
#map3 = affine_map<()[s0] -> (s0, 16)>
#map4 = affine_map<()[s0, s1] -> (s0, s1)>
#map5 = affine_map<()[s0] -> (s0 * 256)>
#map6 = affine_map<()[s0] -> (s0 floordiv 256)>
#map7 = affine_map<()[s0] -> (s0 mod 256)>
#map8 = affine_map<()[s0] -> (-s0 + 64)>
#map9 = affine_map<()[s0] -> (s0, 64)>
#map10 = affine_map<()[s0] -> (s0 * 64)>
#map11 = affine_map<()[s0] -> (s0 floordiv 64)>
#map12 = affine_map<()[s0] -> (s0 mod 64)>
#map13 = affine_map<()[s0] -> (-s0 + 16)>
"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, i32, i32, i32, i32) -> (), sym_name = "chunk_scaled_dot_kkt_fwd_kernel"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xbf16>, %arg4: memref<?xbf16>, %arg5: memref<?xbf16>, %arg6: i32, %arg7: i32, %arg8: i32, %arg9: i32):
    %0 = "arith.constant"() <{value = 1 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = true}> : () -> i1
    %2 = "arith.constant"() <{value = 1 : index}> : () -> index
    %3 = "arith.constant"() <{value = 64 : index}> : () -> index
    %4 = "arith.constant"() <{value = 0.000000e+00 : bf16}> : () -> bf16
    %5 = "arith.constant"() <{value = 16 : index}> : () -> index
    %6 = "arith.constant"() <{value = 0 : index}> : () -> index
    %7 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %8 = "arith.constant"() <{value = 4 : i32}> : () -> i32
    %9 = "arith.constant"() <{value = 16 : i32}> : () -> i32
    %10 = "arith.constant"() <{value = 64 : i32}> : () -> i32
    %11 = "arith.constant"() <{value = 0.000000e+00 : f32}> : () -> f32
    "hivm.hir.set_mask_norm"() : () -> ()
    %12 = "arith.muli"(%arg7, %arg8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %13 = "arith.muli"(%12, %arg9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%13) <{effects = ["write"]}> {logical_block_num} : (i32) -> ()
    %14 = "hivm.hir.get_block_idx"() : () -> i64
    %15 = "arith.trunci"(%14) : (i64) -> i32
    %16 = "arith.muli"(%arg9, %arg8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %17 = "arith.divsi"(%15, %16) : (i32, i32) -> i32
    %18 = "arith.remsi"(%17, %arg7) : (i32, i32) -> i32
    %19 = "tensor.empty"() : () -> tensor<16x16xf32>
    %20 = "tensor.empty"() : () -> tensor<16xi32>
    %21 = "hivm.hir.varange"(%20, %6, %2) <{operandSegmentSizes = array<i32: 1, 1, 1>}> : (tensor<16xi32>, index, index) -> tensor<16xi32>
    %22 = "tensor.empty"() : () -> tensor<16xf32>
    %23 = "hivm.hir.vcast"(%21, %22) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16xi32>, tensor<16xf32>) -> tensor<16xf32>
    %24 = "arith.muli"(%18, %9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %25 = "tensor.expand_shape"(%23) <{reassociation = [[0, 1]], static_output_shape = array<i64: 16, 1>}> : (tensor<16xf32>) -> tensor<16x1xf32>
    %26 = "hivm.hir.vbrc"(%25, %19) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %27 = "tensor.expand_shape"(%23) <{reassociation = [[0, 1]], static_output_shape = array<i64: 1, 16>}> : (tensor<16xf32>) -> tensor<1x16xf32>
    %28 = "hivm.hir.vbrc"(%27, %19) <{broadcast_dims = array<i64: 0>}> : (tensor<1x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %29 = "tensor.empty"() : () -> tensor<16x16xi1>
    %30 = "hivm.hir.vcmp"(%26, %28, %29) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<gt>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x16xf32>, tensor<16x16xf32>, tensor<16x16xi1>) -> tensor<16x16xi1>
    "scf.for"(%7, %8, %0) ({
    ^bb0(%arg10: i32):
      %31 = "arith.divsi"(%arg10, %8) : (i32, i32) -> i32
      %32 = "arith.remsi"(%arg10, %8) : (i32, i32) -> i32
      %33 = "arith.muli"(%31, %arg6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %34 = "arith.muli"(%32, %arg6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %35 = "arith.index_cast"(%34) : (i32) -> index
      %36 = "arith.index_cast"(%33) : (i32) -> index
      %37 = "affine.apply"(%35, %36) <{map = #map}> : (index, index) -> index
      %38 = "arith.maxsi"(%24, %7) : (i32, i32) -> i32
      %39 = "arith.index_cast"(%38) : (i32) -> index
      %40 = "affine.apply"(%39, %37) <{map = #map}> : (index, index) -> index
      %41 = "arith.index_cast"(%arg6) : (i32) -> index
      %42 = "memref.reinterpret_cast"(%arg4, %40) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 1>, static_strides = array<i64: 1, 1>}> : (memref<?xbf16>, index) -> memref<16x1xbf16, strided<[1, 1], offset: ?>>
      %43 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x1xbf16>
      %44 = "memref.collapse_shape"(%43) <{reassociation = [[0, 1]]}> : (memref<16x1xbf16>) -> memref<16xbf16>
      %45 = "affine.apply"(%41, %39) <{map = #map1}> : (index, index) -> index
      %46 = "affine.max"(%45) <{map = #map2}> : (index) -> index
      %47 = "affine.min"(%46) <{map = #map3}> : (index) -> index
      %48 = "arith.subi"(%7, %24) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %49 = "arith.maxsi"(%48, %7) : (i32, i32) -> i32
      %50 = "arith.index_cast"(%49) : (i32) -> index
      %51 = "affine.min"(%50, %47) <{map = #map4}> : (index, index) -> index
      %52 = "affine.apply"(%47, %51) <{map = #map1}> : (index, index) -> index
      %53 = "arith.cmpi"(%52, %5) <{predicate = 2 : i64}> : (index, index) -> i1
      "scf.if"(%53) ({
        "hivm.hir.vbrc"(%4, %44) <{broadcast_dims = array<i64>}> : (bf16, memref<16xbf16>) -> ()
        "scf.yield"() : () -> ()
      }, {
      }) {hivm.unlikely_condition} : (i1) -> ()
      %54 = "memref.subview"(%42, %52) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 1>, static_strides = array<i64: 1, 1>}> : (memref<16x1xbf16, strided<[1, 1], offset: ?>>, index) -> memref<?x1xbf16, strided<[1, 1], offset: ?>>
      %55 = "memref.subview"(%43, %51, %52) <{operandSegmentSizes = array<i32: 1, 1, 1, 0>, static_offsets = array<i64: -9223372036854775808, 0>, static_sizes = array<i64: -9223372036854775808, 1>, static_strides = array<i64: 1, 1>}> : (memref<16x1xbf16>, index, index) -> memref<?x1xbf16, strided<[1, 1], offset: ?>>
      "hivm.hir.load"(%54, %55, %4, %6) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 0>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x1xbf16, strided<[1, 1], offset: ?>>, memref<?x1xbf16, strided<[1, 1], offset: ?>>, bf16, index) -> ()
      %56 = "bufferization.to_tensor"(%43) <{restrict, writable}> : (memref<16x1xbf16>) -> tensor<16x1xbf16>
      %57 = "arith.muli"(%33, %8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %58 = "arith.addi"(%57, %32) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %59 = "arith.muli"(%58, %10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %60 = "arith.index_cast"(%59) : (i32) -> index
      %61 = "affine.apply"(%39) <{map = #map5}> : (index) -> index
      %62 = "affine.apply"(%61, %60) <{map = #map}> : (index, index) -> index
      %63 = "memref.reinterpret_cast"(%arg3, %62) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 64>, static_strides = array<i64: 256, 1>}> : (memref<?xbf16>, index) -> memref<16x64xbf16, strided<[256, 1], offset: ?>>
      %64 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x64xbf16>
      %65 = "affine.apply"(%61) <{map = #map6}> : (index) -> index
      %66 = "affine.apply"(%41, %65) <{map = #map1}> : (index, index) -> index
      %67 = "affine.max"(%66) <{map = #map2}> : (index) -> index
      %68 = "affine.min"(%67) <{map = #map3}> : (index) -> index
      %69 = "affine.apply"(%61) <{map = #map7}> : (index) -> index
      %70 = "affine.apply"(%69) <{map = #map8}> : (index) -> index
      %71 = "affine.max"(%70) <{map = #map2}> : (index) -> index
      %72 = "affine.min"(%71) <{map = #map9}> : (index) -> index
      %73 = "affine.min"(%50, %68) <{map = #map4}> : (index, index) -> index
      %74 = "affine.apply"(%68, %73) <{map = #map1}> : (index, index) -> index
      %75 = "affine.min"(%72) <{map = #map2}> : (index) -> index
      %76 = "affine.apply"(%72, %75) <{map = #map1}> : (index, index) -> index
      %77 = "arith.cmpi"(%74, %5) <{predicate = 2 : i64}> : (index, index) -> i1
      %78 = "arith.cmpi"(%76, %3) <{predicate = 2 : i64}> : (index, index) -> i1
      %79 = "arith.ori"(%77, %78) : (i1, i1) -> i1
      %80 = "memref.subview"(%63, %74, %76) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x64xbf16, strided<[256, 1], offset: ?>>, index, index) -> memref<?x?xbf16, strided<[256, 1], offset: ?>>
      %81 = "memref.subview"(%64, %73, %75, %74, %76) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x64xbf16>, index, index, index, index) -> memref<?x?xbf16, strided<[64, 1], offset: ?>>
      %82 = "arith.remui"(%75, %5) : (index, index) -> index
      "hivm.hir.load"(%80, %81, %4, %82, %79) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<?x?xbf16, strided<[256, 1], offset: ?>>, memref<?x?xbf16, strided<[64, 1], offset: ?>>, bf16, index, i1) -> ()
      %83 = "bufferization.to_tensor"(%64) <{restrict, writable}> : (memref<16x64xbf16>) -> tensor<16x64xbf16>
      %84 = "hivm.hir.mmadL1"(%83, %83, %1, %74, %76, %74, %19) <{b_transpose, operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_for_result_already_inserted = true} : (tensor<16x64xbf16>, tensor<16x64xbf16>, i1, index, index, index, tensor<16x16xf32>) -> tensor<16x16xf32>
      %85 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xf32>
      %86 = "bufferization.to_tensor"(%85) <{restrict, writable}> : (memref<16x16xf32>) -> tensor<16x16xf32>
      %87 = "hivm.hir.fixpipe"(%84, %86) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
      %88 = "hivm.hir.load"(%87, %19) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
      %89 = "tensor.empty"() : () -> tensor<16x1xf32>
      %90 = "hivm.hir.vcast"(%56, %89) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x1xbf16>, tensor<16x1xf32>) -> tensor<16x1xf32>
      %91 = "hivm.hir.vbrc"(%90, %19) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
      %92 = "hivm.hir.vmul"(%88, %91, %19) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x16xf32>, tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
      %93 = "hivm.hir.vsel"(%30, %92, %11, %19) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<16x16xi1>, tensor<16x16xf32>, f32, tensor<16x16xf32>) -> tensor<16x16xf32>
      %94 = "arith.muli"(%58, %9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %95 = "arith.index_cast"(%94) : (i32) -> index
      %96 = "affine.apply"(%39) <{map = #map10}> : (index) -> index
      %97 = "affine.apply"(%96, %95) <{map = #map}> : (index, index) -> index
      %98 = "memref.reinterpret_cast"(%arg5, %97) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 64, 1>}> : (memref<?xbf16>, index) -> memref<16x16xbf16, strided<[64, 1], offset: ?>>
      %99 = "tensor.empty"() : () -> tensor<16x16xbf16>
      %100 = "hivm.hir.vcast"(%93, %99) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x16xf32>, tensor<16x16xbf16>) -> tensor<16x16xbf16>
      %101 = "affine.apply"(%96) <{map = #map11}> : (index) -> index
      %102 = "affine.apply"(%41, %101) <{map = #map1}> : (index, index) -> index
      %103 = "affine.max"(%102) <{map = #map2}> : (index) -> index
      %104 = "affine.min"(%103) <{map = #map3}> : (index) -> index
      %105 = "affine.apply"(%96) <{map = #map12}> : (index) -> index
      %106 = "affine.apply"(%105) <{map = #map13}> : (index) -> index
      %107 = "affine.max"(%106) <{map = #map2}> : (index) -> index
      %108 = "affine.min"(%107) <{map = #map3}> : (index) -> index
      %109 = "affine.min"(%50, %104) <{map = #map4}> : (index, index) -> index
      %110 = "affine.apply"(%104, %109) <{map = #map1}> : (index, index) -> index
      %111 = "affine.min"(%108) <{map = #map2}> : (index) -> index
      %112 = "affine.apply"(%108, %111) <{map = #map1}> : (index, index) -> index
      %113 = "tensor.extract_slice"(%100, %109, %111, %110, %112) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<16x16xbf16>, index, index, index, index) -> tensor<?x?xbf16>
      %114 = "memref.subview"(%98, %110, %112) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16, strided<[64, 1], offset: ?>>, index, index) -> memref<?x?xbf16, strided<[64, 1], offset: ?>>
      "hivm.hir.store"(%113, %114) : (tensor<?x?xbf16>, memref<?x?xbf16, strided<[64, 1], offset: ?>>) -> ()
      "scf.yield"() : () -> ()
    }) {fixpipe_for_mmad_result_already_inserted = true} : (i32, i32, i32) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, false, false, false, false]> : vector<10xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<MIX>, mix_mode = "mix", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hacc.target = #hacc.target<"Ascend910B1">, hivm.module_core_type = #hivm.module_core_type<MIX>} : () -> ()

