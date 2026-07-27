#map = affine_map<()[s0, s1] -> (s0 + s1)>
#map1 = affine_map<()[s0] -> (s0 + 64)>
#map2 = affine_map<()[s0] -> (s0 + 128)>
#map3 = affine_map<()[s0] -> (s0 + 192)>
#map4 = affine_map<()[s0] -> (s0 + 256)>
#map5 = affine_map<()[s0] -> (s0 + 512)>
#map6 = affine_map<()[s0] -> (s0 + 1)>
"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, i32, i32, i32) -> (), sym_name = "fused_qkvzba_split_reshape_cat_kernel"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xbf16>, %arg4: memref<?xbf16>, %arg5: memref<?xbf16>, %arg6: memref<?xbf16>, %arg7: memref<?xbf16>, %arg8: memref<?xbf16>, %arg9: i32, %arg10: i32, %arg11: i32):
    %0 = "arith.constant"() <{value = 0 : index}> : () -> index
    %1 = "arith.constant"() <{value = 4 : i32}> : () -> i32
    %2 = "arith.constant"() <{value = 256 : i32}> : () -> i32
    %3 = "arith.constant"() <{value = 64 : i32}> : () -> i32
    %4 = "arith.constant"() <{value = 2 : i32}> : () -> i32
    %5 = "arith.constant"() <{value = 1024 : i32}> : () -> i32
    %6 = "arith.constant"() <{value = 768 : i32}> : () -> i32
    %7 = "arith.constant"() <{value = 8 : i32}> : () -> i32
    "hivm.hir.set_mask_norm"() : () -> ()
    %8 = "arith.muli"(%arg9, %arg10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %9 = "arith.muli"(%8, %arg11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%9) <{effects = ["write"]}> {logical_block_num} : (i32) -> ()
    %10 = "hivm.hir.get_block_idx"() : () -> i64
    %11 = "arith.trunci"(%10) : (i64) -> i32
    %12 = "arith.divsi"(%11, %arg11) : (i32, i32) -> i32
    %13 = "arith.remsi"(%12, %arg10) : (i32, i32) -> i32
    %14 = "arith.muli"(%arg11, %arg10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %15 = "arith.divsi"(%11, %14) : (i32, i32) -> i32
    %16 = "arith.remsi"(%15, %arg9) : (i32, i32) -> i32
    %17 = "arith.muli"(%16, %5) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %18 = "arith.index_cast"(%17) : (i32) -> index
    %19 = "arith.muli"(%13, %2) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %20 = "arith.index_cast"(%19) : (i32) -> index
    %21 = "affine.apply"(%18, %20) <{map = #map}> : (index, index) -> index
    %22 = "memref.reinterpret_cast"(%arg7, %21) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64>, static_strides = array<i64: 1>}> : (memref<?xbf16>, index) -> memref<64xbf16, strided<[1], offset: ?>>
    %23 = "affine.apply"(%21) <{map = #map1}> : (index) -> index
    %24 = "memref.reinterpret_cast"(%arg7, %23) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64>, static_strides = array<i64: 1>}> : (memref<?xbf16>, index) -> memref<64xbf16, strided<[1], offset: ?>>
    %25 = "affine.apply"(%21) <{map = #map2}> : (index) -> index
    %26 = "memref.reinterpret_cast"(%arg7, %25) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64>, static_strides = array<i64: 1>}> : (memref<?xbf16>, index) -> memref<64xbf16, strided<[1], offset: ?>>
    %27 = "affine.apply"(%21) <{map = #map3}> : (index) -> index
    %28 = "memref.reinterpret_cast"(%arg7, %27) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64>, static_strides = array<i64: 1>}> : (memref<?xbf16>, index) -> memref<64xbf16, strided<[1], offset: ?>>
    %29 = "arith.muli"(%16, %6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %30 = "arith.index_cast"(%29) : (i32) -> index
    %31 = "arith.muli"(%13, %3) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %32 = "arith.index_cast"(%31) : (i32) -> index
    %33 = "affine.apply"(%30, %32) <{map = #map}> : (index, index) -> index
    %34 = "memref.reinterpret_cast"(%arg3, %33) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64>, static_strides = array<i64: 1>}> : (memref<?xbf16>, index) -> memref<64xbf16, strided<[1], offset: ?>>
    %35 = "affine.apply"(%30) <{map = #map4}> : (index) -> index
    %36 = "affine.apply"(%35, %32) <{map = #map}> : (index, index) -> index
    %37 = "memref.reinterpret_cast"(%arg3, %36) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64>, static_strides = array<i64: 1>}> : (memref<?xbf16>, index) -> memref<64xbf16, strided<[1], offset: ?>>
    %38 = "affine.apply"(%30) <{map = #map5}> : (index) -> index
    %39 = "arith.divsi"(%19, %1) : (i32, i32) -> i32
    %40 = "arith.index_cast"(%39) : (i32) -> index
    %41 = "affine.apply"(%38, %40) <{map = #map}> : (index, index) -> index
    %42 = "memref.reinterpret_cast"(%arg3, %41) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64>, static_strides = array<i64: 1>}> : (memref<?xbf16>, index) -> memref<64xbf16, strided<[1], offset: ?>>
    %43 = "arith.muli"(%16, %2) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %44 = "arith.index_cast"(%43) : (i32) -> index
    %45 = "affine.apply"(%44, %40) <{map = #map}> : (index, index) -> index
    %46 = "memref.reinterpret_cast"(%arg4, %45) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64>, static_strides = array<i64: 1>}> : (memref<?xbf16>, index) -> memref<64xbf16, strided<[1], offset: ?>>
    %47 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<64xbf16>
    "hivm.hir.load"(%22, %47) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<64xbf16, strided<[1], offset: ?>>, memref<64xbf16>) -> ()
    %48 = "bufferization.to_tensor"(%47) <{restrict, writable}> : (memref<64xbf16>) -> tensor<64xbf16>
    "hivm.hir.store"(%48, %34) : (tensor<64xbf16>, memref<64xbf16, strided<[1], offset: ?>>) -> ()
    %49 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<64xbf16>
    "hivm.hir.load"(%24, %49) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<64xbf16, strided<[1], offset: ?>>, memref<64xbf16>) -> ()
    %50 = "bufferization.to_tensor"(%49) <{restrict, writable}> : (memref<64xbf16>) -> tensor<64xbf16>
    "hivm.hir.store"(%50, %37) : (tensor<64xbf16>, memref<64xbf16, strided<[1], offset: ?>>) -> ()
    %51 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<64xbf16>
    "hivm.hir.load"(%26, %51) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<64xbf16, strided<[1], offset: ?>>, memref<64xbf16>) -> ()
    %52 = "bufferization.to_tensor"(%51) <{restrict, writable}> : (memref<64xbf16>) -> tensor<64xbf16>
    "hivm.hir.store"(%52, %42) : (tensor<64xbf16>, memref<64xbf16, strided<[1], offset: ?>>) -> ()
    %53 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<64xbf16>
    "hivm.hir.load"(%28, %53) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<64xbf16, strided<[1], offset: ?>>, memref<64xbf16>) -> ()
    %54 = "bufferization.to_tensor"(%53) <{restrict, writable}> : (memref<64xbf16>) -> tensor<64xbf16>
    "hivm.hir.store"(%54, %46) : (tensor<64xbf16>, memref<64xbf16, strided<[1], offset: ?>>) -> ()
    %55 = "arith.muli"(%16, %7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %56 = "arith.index_cast"(%55) : (i32) -> index
    %57 = "arith.muli"(%13, %4) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %58 = "arith.index_cast"(%57) : (i32) -> index
    %59 = "affine.apply"(%56, %58) <{map = #map}> : (index, index) -> index
    %60 = "memref.reinterpret_cast"(%arg8, %59) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xbf16>, index) -> memref<1xbf16, strided<[1], offset: ?>>
    %61 = "arith.muli"(%16, %1) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %62 = "arith.index_cast"(%61) : (i32) -> index
    %63 = "arith.muli"(%13, %1) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %64 = "arith.divsi"(%63, %1) : (i32, i32) -> i32
    %65 = "arith.index_cast"(%64) : (i32) -> index
    %66 = "affine.apply"(%62, %65) <{map = #map}> : (index, index) -> index
    %67 = "memref.load"(%60, %0) : (memref<1xbf16, strided<[1], offset: ?>>, index) -> bf16
    %68 = "tensor.empty"() : () -> tensor<1xbf16>
    %69 = "tensor.empty"() : () -> tensor<1xbf16>
    %70 = "tensor.insert"(%67, %69, %0) : (bf16, tensor<1xbf16>, index) -> tensor<1xbf16>
    %71 = "memref.reinterpret_cast"(%arg5, %66) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xbf16>, index) -> memref<1xbf16, strided<[1], offset: ?>>
    "hivm.hir.store"(%70, %71) : (tensor<1xbf16>, memref<1xbf16, strided<[1], offset: ?>>) -> ()
    %72 = "affine.apply"(%59) <{map = #map6}> : (index) -> index
    %73 = "memref.reinterpret_cast"(%arg8, %72) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xbf16>, index) -> memref<1xbf16, strided<[1], offset: ?>>
    %74 = "memref.load"(%73, %0) : (memref<1xbf16, strided<[1], offset: ?>>, index) -> bf16
    %75 = "tensor.insert"(%74, %68, %0) : (bf16, tensor<1xbf16>, index) -> tensor<1xbf16>
    %76 = "memref.reinterpret_cast"(%arg6, %66) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xbf16>, index) -> memref<1xbf16, strided<[1], offset: ?>>
    "hivm.hir.store"(%75, %76) : (tensor<1xbf16>, memref<1xbf16, strided<[1], offset: ?>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, true, true, true, false, false, false]> : vector<12xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hacc.target = #hacc.target<"Ascend910B1">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

