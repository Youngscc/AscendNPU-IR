#map = affine_map<()[s0] -> (s0 + 32)>
"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 2 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 2 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xbf16>, i32, memref<?xbf16>, i32, memref<?xf32>, i32, memref<?xf32>, i32, i32, i32, i32, i32) -> (), sym_name = "_triton_rope"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xbf16>, %arg4: i32, %arg5: memref<?xbf16>, %arg6: i32, %arg7: memref<?xf32>, %arg8: i32, %arg9: memref<?xf32>, %arg10: i32, %arg11: i32, %arg12: i32, %arg13: i32, %arg14: i32):
    "hivm.hir.set_mask_norm"() : () -> ()
    %0 = "arith.muli"(%arg12, %arg13) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %1 = "arith.muli"(%0, %arg14) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%1) <{effects = ["write"]}> {logical_block_num} : (i32) -> ()
    %2 = "hivm.hir.get_block_idx"() : () -> i64
    %3 = "arith.trunci"(%2) : (i64) -> i32
    %4 = "arith.muli"(%arg14, %arg13) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %5 = "arith.divsi"(%3, %4) : (i32, i32) -> i32
    %6 = "arith.remsi"(%5, %arg12) : (i32, i32) -> i32
    %7 = "arith.extsi"(%6) : (i32) -> i64
    %8 = "arith.extsi"(%arg11) : (i32) -> i64
    %9 = "arith.extsi"(%arg12) : (i32) -> i64
    %10 = "arith.extsi"(%arg4) : (i32) -> i64
    %11 = "arith.extsi"(%arg6) : (i32) -> i64
    %12 = "arith.extsi"(%arg8) : (i32) -> i64
    %13 = "arith.extsi"(%arg10) : (i32) -> i64
    "scf.for"(%7, %8, %9) ({
    ^bb0(%arg15: i64):
      %14 = "arith.muli"(%arg15, %10) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
      %15 = "arith.index_cast"(%14) : (i64) -> index
      %16 = "arith.muli"(%arg15, %11) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
      %17 = "arith.index_cast"(%16) : (i64) -> index
      %18 = "arith.muli"(%arg15, %12) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
      %19 = "arith.index_cast"(%18) : (i64) -> index
      %20 = "arith.muli"(%arg15, %13) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
      %21 = "arith.index_cast"(%20) : (i64) -> index
      %22 = "memref.reinterpret_cast"(%arg7, %19) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1, 32>, static_strides = array<i64: 32, 1>}> : (memref<?xf32>, index) -> memref<1x32xf32, strided<[32, 1], offset: ?>>
      %23 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<1x32xf32>
      "hivm.hir.load"(%22, %23) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<1x32xf32, strided<[32, 1], offset: ?>>, memref<1x32xf32>) -> ()
      %24 = "bufferization.to_tensor"(%23) <{restrict, writable}> : (memref<1x32xf32>) -> tensor<1x32xf32>
      %25 = "memref.reinterpret_cast"(%arg9, %21) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1, 32>, static_strides = array<i64: 32, 1>}> : (memref<?xf32>, index) -> memref<1x32xf32, strided<[32, 1], offset: ?>>
      %26 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<1x32xf32>
      "hivm.hir.load"(%25, %26) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<1x32xf32, strided<[32, 1], offset: ?>>, memref<1x32xf32>) -> ()
      %27 = "bufferization.to_tensor"(%26) <{restrict, writable}> : (memref<1x32xf32>) -> tensor<1x32xf32>
      %28 = "memref.reinterpret_cast"(%arg3, %15) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 8, 32>, static_strides = array<i64: 128, 1>}> : (memref<?xbf16>, index) -> memref<8x32xbf16, strided<[128, 1], offset: ?>>
      %29 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<8x32xbf16>
      "hivm.hir.load"(%28, %29) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<8x32xbf16, strided<[128, 1], offset: ?>>, memref<8x32xbf16>) -> ()
      %30 = "bufferization.to_tensor"(%29) <{restrict, writable}> : (memref<8x32xbf16>) -> tensor<8x32xbf16>
      %31 = "tensor.empty"() : () -> tensor<8x32xf32>
      %32 = "hivm.hir.vcast"(%30, %31) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<8x32xbf16>, tensor<8x32xf32>) -> tensor<8x32xf32>
      %33 = "memref.reinterpret_cast"(%arg5, %17) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1, 32>, static_strides = array<i64: 32, 1>}> : (memref<?xbf16>, index) -> memref<1x32xbf16, strided<[32, 1], offset: ?>>
      %34 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<1x32xbf16>
      "hivm.hir.load"(%33, %34) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<1x32xbf16, strided<[32, 1], offset: ?>>, memref<1x32xbf16>) -> ()
      %35 = "bufferization.to_tensor"(%34) <{restrict, writable}> : (memref<1x32xbf16>) -> tensor<1x32xbf16>
      %36 = "tensor.empty"() : () -> tensor<1x32xf32>
      %37 = "hivm.hir.vcast"(%35, %36) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<1x32xbf16>, tensor<1x32xf32>) -> tensor<1x32xf32>
      %38 = "affine.apply"(%15) <{map = #map}> : (index) -> index
      %39 = "memref.reinterpret_cast"(%arg3, %38) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 8, 32>, static_strides = array<i64: 128, 1>}> : (memref<?xbf16>, index) -> memref<8x32xbf16, strided<[128, 1], offset: ?>>
      %40 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<8x32xbf16>
      "hivm.hir.load"(%39, %40) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<8x32xbf16, strided<[128, 1], offset: ?>>, memref<8x32xbf16>) -> ()
      %41 = "bufferization.to_tensor"(%40) <{restrict, writable}> : (memref<8x32xbf16>) -> tensor<8x32xbf16>
      %42 = "hivm.hir.vcast"(%41, %31) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<8x32xbf16>, tensor<8x32xf32>) -> tensor<8x32xf32>
      %43 = "affine.apply"(%17) <{map = #map}> : (index) -> index
      %44 = "memref.reinterpret_cast"(%arg5, %43) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1, 32>, static_strides = array<i64: 32, 1>}> : (memref<?xbf16>, index) -> memref<1x32xbf16, strided<[32, 1], offset: ?>>
      %45 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<1x32xbf16>
      "hivm.hir.load"(%44, %45) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<1x32xbf16, strided<[32, 1], offset: ?>>, memref<1x32xbf16>) -> ()
      %46 = "bufferization.to_tensor"(%45) <{restrict, writable}> : (memref<1x32xbf16>) -> tensor<1x32xbf16>
      %47 = "hivm.hir.vcast"(%46, %36) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<1x32xbf16>, tensor<1x32xf32>) -> tensor<1x32xf32>
      %48 = "hivm.hir.vbrc"(%24, %31) <{broadcast_dims = array<i64: 0>}> : (tensor<1x32xf32>, tensor<8x32xf32>) -> tensor<8x32xf32>
      %49 = "hivm.hir.vmul"(%32, %48, %31) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<8x32xf32>, tensor<8x32xf32>, tensor<8x32xf32>) -> tensor<8x32xf32>
      %50 = "hivm.hir.vbrc"(%27, %31) <{broadcast_dims = array<i64: 0>}> : (tensor<1x32xf32>, tensor<8x32xf32>) -> tensor<8x32xf32>
      %51 = "hivm.hir.vmul"(%42, %50, %31) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<8x32xf32>, tensor<8x32xf32>, tensor<8x32xf32>) -> tensor<8x32xf32>
      %52 = "hivm.hir.vsub"(%49, %51, %31) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<8x32xf32>, tensor<8x32xf32>, tensor<8x32xf32>) -> tensor<8x32xf32>
      %53 = "tensor.empty"() : () -> tensor<8x32xbf16>
      %54 = "hivm.hir.vcast"(%52, %53) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<8x32xf32>, tensor<8x32xbf16>) -> tensor<8x32xbf16>
      "hivm.hir.store"(%54, %28) : (tensor<8x32xbf16>, memref<8x32xbf16, strided<[128, 1], offset: ?>>) -> ()
      %55 = "hivm.hir.vmul"(%42, %48, %31) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<8x32xf32>, tensor<8x32xf32>, tensor<8x32xf32>) -> tensor<8x32xf32>
      %56 = "hivm.hir.vmul"(%32, %50, %31) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<8x32xf32>, tensor<8x32xf32>, tensor<8x32xf32>) -> tensor<8x32xf32>
      %57 = "hivm.hir.vadd"(%55, %56, %31) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<8x32xf32>, tensor<8x32xf32>, tensor<8x32xf32>) -> tensor<8x32xf32>
      %58 = "hivm.hir.vcast"(%57, %53) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<8x32xf32>, tensor<8x32xbf16>) -> tensor<8x32xbf16>
      "hivm.hir.store"(%58, %39) : (tensor<8x32xbf16>, memref<8x32xbf16, strided<[128, 1], offset: ?>>) -> ()
      %59 = "hivm.hir.vmul"(%37, %24, %36) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1x32xf32>, tensor<1x32xf32>, tensor<1x32xf32>) -> tensor<1x32xf32>
      %60 = "hivm.hir.vmul"(%47, %27, %36) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1x32xf32>, tensor<1x32xf32>, tensor<1x32xf32>) -> tensor<1x32xf32>
      %61 = "hivm.hir.vsub"(%59, %60, %36) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1x32xf32>, tensor<1x32xf32>, tensor<1x32xf32>) -> tensor<1x32xf32>
      %62 = "tensor.empty"() : () -> tensor<1x32xbf16>
      %63 = "hivm.hir.vcast"(%61, %62) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<1x32xf32>, tensor<1x32xbf16>) -> tensor<1x32xbf16>
      "hivm.hir.store"(%63, %33) : (tensor<1x32xbf16>, memref<1x32xbf16, strided<[32, 1], offset: ?>>) -> ()
      %64 = "hivm.hir.vmul"(%47, %24, %36) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1x32xf32>, tensor<1x32xf32>, tensor<1x32xf32>) -> tensor<1x32xf32>
      %65 = "hivm.hir.vmul"(%37, %27, %36) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1x32xf32>, tensor<1x32xf32>, tensor<1x32xf32>) -> tensor<1x32xf32>
      %66 = "hivm.hir.vadd"(%64, %65, %36) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1x32xf32>, tensor<1x32xf32>, tensor<1x32xf32>) -> tensor<1x32xf32>
      %67 = "hivm.hir.vcast"(%66, %62) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<1x32xf32>, tensor<1x32xbf16>) -> tensor<1x32xbf16>
      "hivm.hir.store"(%67, %44) : (tensor<1x32xbf16>, memref<1x32xbf16, strided<[32, 1], offset: ?>>) -> ()
      "scf.yield"() : () -> ()
    }) : (i64, i64, i64) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, false, true, false, true, false, true, false, false, false, false, false]> : vector<15xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hacc.target = #hacc.target<"Ascend910B1">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

