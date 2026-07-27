#map = affine_map<()[s0, s1] -> (s0 * s1)>
#map1 = affine_map<()[s0, s1] -> (s0 + s1)>
#map2 = affine_map<()[s0] -> (s0 + 64)>
#map3 = affine_map<()[s0, s1] -> (s0, s1)>
#map4 = affine_map<()[s0, s1] -> (s0 - s1)>
#map5 = affine_map<()[s0] -> (s0, 0)>
#map6 = affine_map<()[s0] -> (s0, 64)>
#map7 = affine_map<()[s0] -> (s0 + 32)>
#map8 = affine_map<()[s0] -> (s0, 32)>
"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xf16>, memref<?xf16>, memref<?xf16>, i32, i32, i32, i32, i32, i32, i32, i32, i32) -> (), sym_name = "matmul_kernel"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xf16>, %arg4: memref<?xf16>, %arg5: memref<?xf16>, %arg6: i32, %arg7: i32, %arg8: i32, %arg9: i32, %arg10: i32, %arg11: i32, %arg12: i32, %arg13: i32, %arg14: i32):
    %0 = "arith.constant"() <{value = 1 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = 0.000000e+00 : f16}> : () -> f16
    %2 = "arith.constant"() <{value = 0 : index}> : () -> index
    %3 = "arith.constant"() <{value = 64 : index}> : () -> index
    %4 = "arith.constant"() <{value = 0.00999999977 : f32}> : () -> f32
    %5 = "arith.constant"() <{value = 1.000000e+00 : f32}> : () -> f32
    %6 = "arith.constant"() <{value = 64 : i32}> : () -> i32
    %7 = "arith.constant"() <{value = 32 : i32}> : () -> i32
    %8 = "arith.constant"() <{value = 63 : i32}> : () -> i32
    %9 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %10 = "arith.constant"() <{value = 2 : i32}> : () -> i32
    %11 = "arith.constant"() <{value = 0.000000e+00 : f32}> : () -> f32
    "hivm.hir.set_mask_norm"() : () -> ()
    %12 = "arith.muli"(%arg12, %arg13) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %13 = "arith.muli"(%12, %arg14) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%13) <{effects = ["write"]}> {logical_block_num} : (i32) -> ()
    %14 = "hivm.hir.get_block_idx"() : () -> i64
    %15 = "arith.trunci"(%14) : (i64) -> i32
    %16 = "arith.muli"(%arg14, %arg13) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %17 = "arith.divsi"(%15, %16) : (i32, i32) -> i32
    %18 = "arith.remsi"(%17, %arg12) : (i32, i32) -> i32
    %19 = "tensor.empty"() : () -> tensor<32x64xf32>
    %20 = "hivm.hir.vbrc"(%11, %19) <{broadcast_dims = array<i64>}> : (f32, tensor<32x64xf32>) -> tensor<32x64xf32>
    %21 = "arith.addi"(%arg6, %8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %22 = "arith.divsi"(%21, %6) : (i32, i32) -> i32
    %23 = "arith.addi"(%arg7, %8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %24 = "arith.divsi"(%23, %6) : (i32, i32) -> i32
    %25 = "arith.divsi"(%18, %24) : (i32, i32) -> i32
    %26 = "arith.subi"(%22, %25) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %27 = "arith.minsi"(%26, %0) : (i32, i32) -> i32
    %28 = "arith.remsi"(%18, %24) : (i32, i32) -> i32
    %29 = "arith.remsi"(%28, %27) : (i32, i32) -> i32
    %30 = "arith.addi"(%25, %29) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %31 = "arith.divsi"(%28, %27) : (i32, i32) -> i32
    %32 = "arith.muli"(%30, %6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %33 = "arith.muli"(%31, %6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %34 = "arith.index_cast"(%32) : (i32) -> index
    %35 = "arith.index_cast"(%arg9) : (i32) -> index
    %36 = "affine.apply"(%34, %35) <{map = #map}> : (index, index) -> index
    %37 = "arith.index_cast"(%arg10) : (i32) -> index
    %38 = "arith.index_cast"(%33) : (i32) -> index
    %39 = "arith.addi"(%arg8, %8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %40 = "arith.divsi"(%39, %6) : (i32, i32) -> i32
    %41 = "tensor.empty"() : () -> tensor<64x64xf32>
    %42 = "scf.for"(%9, %40, %0, %41) ({
    ^bb0(%arg16: i32, %arg17: tensor<64x64xf32>):
      %77 = "arith.muli"(%arg16, %6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %78 = "arith.index_cast"(%77) : (i32) -> index
      %79 = "affine.apply"(%36, %78) <{map = #map1}> : (index, index) -> index
      %80 = "memref.reinterpret_cast"(%arg3, %79, %35) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64, 64>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xf16>, index, index) -> memref<64x64xf16, strided<[?, 1], offset: ?>>
      %81 = "arith.muli"(%77, %arg10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %82 = "arith.index_cast"(%81) : (i32) -> index
      %83 = "affine.apply"(%82, %38) <{map = #map1}> : (index, index) -> index
      %84 = "memref.reinterpret_cast"(%arg4, %83, %37) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64, 64>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xf16>, index, index) -> memref<64x64xf16, strided<[?, 1], offset: ?>>
      %85 = "arith.subi"(%arg8, %77) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %86 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<64x64xf16>
      %87 = "affine.apply"(%34) <{map = #map2}> : (index) -> index
      %88 = "arith.index_cast"(%arg6) : (i32) -> index
      %89 = "affine.max"(%34, %88) <{map = #map3}> : (index, index) -> index
      %90 = "affine.min"(%87, %89) <{map = #map3}> : (index, index) -> index
      %91 = "affine.apply"(%90, %34) <{map = #map4}> : (index, index) -> index
      %92 = "arith.index_cast"(%85) : (i32) -> index
      %93 = "affine.max"(%92) <{map = #map5}> : (index) -> index
      %94 = "affine.min"(%93) <{map = #map6}> : (index) -> index
      %95 = "affine.min"(%91) <{map = #map6}> : (index) -> index
      %96 = "affine.min"(%94) <{map = #map6}> : (index) -> index
      %97 = "arith.cmpi"(%95, %3) <{predicate = 2 : i64}> : (index, index) -> i1
      %98 = "arith.cmpi"(%96, %3) <{predicate = 2 : i64}> : (index, index) -> i1
      %99 = "arith.ori"(%97, %98) : (i1, i1) -> i1
      %100 = "memref.subview"(%80, %95, %96) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<64x64xf16, strided<[?, 1], offset: ?>>, index, index) -> memref<?x?xf16, strided<[?, 1], offset: ?>>
      %101 = "memref.subview"(%86, %95, %96) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<64x64xf16>, index, index) -> memref<?x?xf16, strided<[64, 1]>>
      "hivm.hir.load"(%100, %101, %1, %2, %99) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<?x?xf16, strided<[?, 1], offset: ?>>, memref<?x?xf16, strided<[64, 1]>>, f16, index, i1) -> ()
      %102 = "bufferization.to_tensor"(%86) <{restrict, writable}> : (memref<64x64xf16>) -> tensor<64x64xf16>
      %103 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<64x64xf16>
      %104 = "affine.apply"(%38) <{map = #map2}> : (index) -> index
      %105 = "arith.index_cast"(%arg7) : (i32) -> index
      %106 = "affine.max"(%38, %105) <{map = #map3}> : (index, index) -> index
      %107 = "affine.min"(%104, %106) <{map = #map3}> : (index, index) -> index
      %108 = "affine.apply"(%107, %38) <{map = #map4}> : (index, index) -> index
      %109 = "affine.min"(%108) <{map = #map6}> : (index) -> index
      %110 = "arith.cmpi"(%109, %3) <{predicate = 2 : i64}> : (index, index) -> i1
      %111 = "arith.ori"(%98, %110) : (i1, i1) -> i1
      %112 = "memref.subview"(%84, %96, %109) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<64x64xf16, strided<[?, 1], offset: ?>>, index, index) -> memref<?x?xf16, strided<[?, 1], offset: ?>>
      %113 = "memref.subview"(%103, %96, %109) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<64x64xf16>, index, index) -> memref<?x?xf16, strided<[64, 1]>>
      "hivm.hir.load"(%112, %113, %1, %2, %111) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<?x?xf16, strided<[?, 1], offset: ?>>, memref<?x?xf16, strided<[64, 1]>>, f16, index, i1) -> ()
      %114 = "bufferization.to_tensor"(%103) <{restrict, writable}> : (memref<64x64xf16>) -> tensor<64x64xf16>
      %115 = "arith.cmpi"(%arg16, %9) <{predicate = 0 : i64}> : (i32, i32) -> i1
      %116 = "hivm.hir.mmadL1"(%102, %114, %115, %95, %96, %109, %arg17) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_for_result_already_inserted = true} : (tensor<64x64xf16>, tensor<64x64xf16>, i1, index, index, index, tensor<64x64xf32>) -> tensor<64x64xf32>
      "scf.yield"(%116) : (tensor<64x64xf32>) -> ()
    }) {fixpipe_for_mmad_result_already_inserted = true} : (i32, i32, i32, tensor<64x64xf32>) -> tensor<64x64xf32>
    %43 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<64x64xf32>
    %44 = "bufferization.to_tensor"(%43) <{restrict, writable}> : (memref<64x64xf32>) -> tensor<64x64xf32>
    %45 = "hivm.hir.fixpipe"(%42, %44) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<64x64xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
    "scf.for"(%9, %10, %0) ({
    ^bb0(%arg15: i32):
      %46 = "arith.muli"(%arg15, %7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %47 = "arith.index_cast"(%46) : (i32) -> index
      %48 = "tensor.extract_slice"(%45, %47) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808, 0>, static_sizes = array<i64: 32, 64>, static_strides = array<i64: 1, 1>}> : (tensor<64x64xf32>, index) -> tensor<32x64xf32>
      %49 = "hivm.hir.load"(%48, %19) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<32x64xf32>, tensor<32x64xf32>) -> tensor<32x64xf32>
      %50 = "tensor.empty"() : () -> tensor<32x64xi1>
      %51 = "hivm.hir.vcmp"(%49, %20, %50) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<ge>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32x64xf32>, tensor<32x64xf32>, tensor<32x64xi1>) -> tensor<32x64xi1>
      %52 = "hivm.hir.vmul"(%49, %4, %19) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32x64xf32>, f32, tensor<32x64xf32>) -> tensor<32x64xf32>
      %53 = "hivm.hir.vsel"(%51, %49, %52, %19) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<32x64xi1>, tensor<32x64xf32>, tensor<32x64xf32>, tensor<32x64xf32>) -> tensor<32x64xf32>
      %54 = "hivm.hir.vadd"(%53, %5, %19) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32x64xf32>, f32, tensor<32x64xf32>) -> tensor<32x64xf32>
      %55 = "tensor.empty"() : () -> tensor<32x64xf16>
      %56 = "hivm.hir.vcast"(%54, %55) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<32x64xf32>, tensor<32x64xf16>) -> tensor<32x64xf16>
      %57 = "arith.addi"(%32, %46) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %58 = "arith.index_cast"(%arg11) : (i32) -> index
      %59 = "arith.index_cast"(%57) : (i32) -> index
      %60 = "affine.apply"(%59, %58) <{map = #map}> : (index, index) -> index
      %61 = "affine.apply"(%60, %38) <{map = #map1}> : (index, index) -> index
      %62 = "memref.reinterpret_cast"(%arg5, %61, %58) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 32, 64>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xf16>, index, index) -> memref<32x64xf16, strided<[?, 1], offset: ?>>
      %63 = "affine.apply"(%59) <{map = #map7}> : (index) -> index
      %64 = "arith.index_cast"(%arg6) : (i32) -> index
      %65 = "affine.max"(%59, %64) <{map = #map3}> : (index, index) -> index
      %66 = "affine.min"(%63, %65) <{map = #map3}> : (index, index) -> index
      %67 = "affine.apply"(%66, %59) <{map = #map4}> : (index, index) -> index
      %68 = "affine.apply"(%38) <{map = #map2}> : (index) -> index
      %69 = "arith.index_cast"(%arg7) : (i32) -> index
      %70 = "affine.max"(%38, %69) <{map = #map3}> : (index, index) -> index
      %71 = "affine.min"(%68, %70) <{map = #map3}> : (index, index) -> index
      %72 = "affine.apply"(%71, %38) <{map = #map4}> : (index, index) -> index
      %73 = "affine.min"(%67) <{map = #map8}> : (index) -> index
      %74 = "affine.min"(%72) <{map = #map6}> : (index) -> index
      %75 = "tensor.extract_slice"(%56, %73, %74) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<32x64xf16>, index, index) -> tensor<?x?xf16>
      %76 = "memref.subview"(%62, %73, %74) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<32x64xf16, strided<[?, 1], offset: ?>>, index, index) -> memref<?x?xf16, strided<[?, 1], offset: ?>>
      "hivm.hir.store"(%75, %76) : (tensor<?x?xf16>, memref<?x?xf16, strided<[?, 1], offset: ?>>) -> ()
      "scf.yield"() : () -> ()
    }) {hivm.parallel_loop} : (i32, i32, i32) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, false, false, false, false, false, false, false, false, false]> : vector<15xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<MIX>, mix_mode = "mix", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hacc.target = #hacc.target<"Ascend910B1">, hivm.module_core_type = #hivm.module_core_type<MIX>} : () -> ()

