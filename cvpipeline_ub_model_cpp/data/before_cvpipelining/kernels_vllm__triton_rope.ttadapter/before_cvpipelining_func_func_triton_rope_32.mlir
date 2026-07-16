"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 2 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 2 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xbf16>, i32, memref<?xbf16>, i32, memref<?xf32>, i32, memref<?xf32>, i32, i32, i32, i32, i32) -> (), sym_name = "_triton_rope"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xbf16>, %arg4: i32, %arg5: memref<?xbf16>, %arg6: i32, %arg7: memref<?xf32>, %arg8: i32, %arg9: memref<?xf32>, %arg10: i32, %arg11: i32, %arg12: i32, %arg13: i32, %arg14: i32):
    %0 = "arith.constant"() <{value = 32 : index}> : () -> index
    "hivm.hir.set_mask_norm"() : () -> ()
    %1 = "arith.muli"(%arg12, %arg13) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %2 = "arith.muli"(%1, %arg14) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%2) {logical_block_num} : (i32) -> ()
    %3 = "hivm.hir.get_block_idx"() : () -> i64
    %4 = "arith.trunci"(%3) : (i64) -> i32
    %5 = "arith.muli"(%arg14, %arg13) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %6 = "arith.divsi"(%4, %5) : (i32, i32) -> i32
    %7 = "arith.remsi"(%6, %arg12) : (i32, i32) -> i32
    %8 = "arith.extsi"(%7) : (i32) -> i64
    %9 = "arith.extsi"(%arg11) : (i32) -> i64
    %10 = "arith.extsi"(%arg12) : (i32) -> i64
    %11 = "arith.extsi"(%arg4) : (i32) -> i64
    %12 = "arith.extsi"(%arg6) : (i32) -> i64
    %13 = "arith.extsi"(%arg8) : (i32) -> i64
    %14 = "arith.extsi"(%arg10) : (i32) -> i64
    "scf.for"(%8, %9, %10) ({
    ^bb0(%arg15: i64):
      %15 = "arith.muli"(%arg15, %11) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
      %16 = "arith.index_cast"(%15) : (i64) -> index
      %17 = "arith.muli"(%arg15, %12) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
      %18 = "arith.index_cast"(%17) : (i64) -> index
      %19 = "arith.muli"(%arg15, %13) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
      %20 = "arith.index_cast"(%19) : (i64) -> index
      %21 = "arith.muli"(%arg15, %14) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
      %22 = "arith.index_cast"(%21) : (i64) -> index
      %23 = "memref.reinterpret_cast"(%arg7, %20) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1, 32>, static_strides = array<i64: 32, 1>}> : (memref<?xf32>, index) -> memref<1x32xf32, strided<[32, 1], offset: ?>>
      %24 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<1x32xf32>
      "hivm.hir.load"(%23, %24) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<1x32xf32, strided<[32, 1], offset: ?>>, memref<1x32xf32>) -> ()
      %25 = "bufferization.to_tensor"(%24) <{restrict, writable}> : (memref<1x32xf32>) -> tensor<1x32xf32>
      %26 = "memref.reinterpret_cast"(%arg9, %22) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1, 32>, static_strides = array<i64: 32, 1>}> : (memref<?xf32>, index) -> memref<1x32xf32, strided<[32, 1], offset: ?>>
      %27 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<1x32xf32>
      "hivm.hir.load"(%26, %27) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<1x32xf32, strided<[32, 1], offset: ?>>, memref<1x32xf32>) -> ()
      %28 = "bufferization.to_tensor"(%27) <{restrict, writable}> : (memref<1x32xf32>) -> tensor<1x32xf32>
      %29 = "memref.reinterpret_cast"(%arg3, %16) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 8, 32>, static_strides = array<i64: 128, 1>}> : (memref<?xbf16>, index) -> memref<8x32xbf16, strided<[128, 1], offset: ?>>
      %30 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<8x32xbf16>
      "hivm.hir.load"(%29, %30) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<8x32xbf16, strided<[128, 1], offset: ?>>, memref<8x32xbf16>) -> ()
      %31 = "bufferization.to_tensor"(%30) <{restrict, writable}> : (memref<8x32xbf16>) -> tensor<8x32xbf16>
      %32 = "tensor.empty"() : () -> tensor<8x32xf32>
      %33 = "tensor.empty"() : () -> tensor<8x32xf32>
      %34 = "tensor.empty"() : () -> tensor<8x32xf32>
      %35 = "tensor.empty"() : () -> tensor<8x32xf32>
      %36 = "tensor.empty"() : () -> tensor<8x32xf32>
      %37 = "tensor.empty"() : () -> tensor<8x32xf32>
      %38 = "tensor.empty"() : () -> tensor<8x32xf32>
      %39 = "tensor.empty"() : () -> tensor<8x32xf32>
      %40 = "tensor.empty"() : () -> tensor<8x32xf32>
      %41 = "tensor.empty"() : () -> tensor<8x32xf32>
      %42 = "hivm.hir.vcast"(%31, %41) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<8x32xbf16>, tensor<8x32xf32>) -> tensor<8x32xf32>
      %43 = "memref.reinterpret_cast"(%arg5, %18) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1, 32>, static_strides = array<i64: 32, 1>}> : (memref<?xbf16>, index) -> memref<1x32xbf16, strided<[32, 1], offset: ?>>
      %44 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<1x32xbf16>
      "hivm.hir.load"(%43, %44) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<1x32xbf16, strided<[32, 1], offset: ?>>, memref<1x32xbf16>) -> ()
      %45 = "bufferization.to_tensor"(%44) <{restrict, writable}> : (memref<1x32xbf16>) -> tensor<1x32xbf16>
      %46 = "tensor.empty"() : () -> tensor<1x32xf32>
      %47 = "tensor.empty"() : () -> tensor<1x32xf32>
      %48 = "tensor.empty"() : () -> tensor<1x32xf32>
      %49 = "tensor.empty"() : () -> tensor<1x32xf32>
      %50 = "tensor.empty"() : () -> tensor<1x32xf32>
      %51 = "tensor.empty"() : () -> tensor<1x32xf32>
      %52 = "tensor.empty"() : () -> tensor<1x32xf32>
      %53 = "tensor.empty"() : () -> tensor<1x32xf32>
      %54 = "hivm.hir.vcast"(%45, %53) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<1x32xbf16>, tensor<1x32xf32>) -> tensor<1x32xf32>
      %55 = "arith.addi"(%16, %0) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %56 = "memref.reinterpret_cast"(%arg3, %55) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 8, 32>, static_strides = array<i64: 128, 1>}> : (memref<?xbf16>, index) -> memref<8x32xbf16, strided<[128, 1], offset: ?>>
      %57 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<8x32xbf16>
      "hivm.hir.load"(%56, %57) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<8x32xbf16, strided<[128, 1], offset: ?>>, memref<8x32xbf16>) -> ()
      %58 = "bufferization.to_tensor"(%57) <{restrict, writable}> : (memref<8x32xbf16>) -> tensor<8x32xbf16>
      %59 = "hivm.hir.vcast"(%58, %40) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<8x32xbf16>, tensor<8x32xf32>) -> tensor<8x32xf32>
      %60 = "arith.addi"(%18, %0) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %61 = "memref.reinterpret_cast"(%arg5, %60) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1, 32>, static_strides = array<i64: 32, 1>}> : (memref<?xbf16>, index) -> memref<1x32xbf16, strided<[32, 1], offset: ?>>
      %62 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<1x32xbf16>
      "hivm.hir.load"(%61, %62) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<1x32xbf16, strided<[32, 1], offset: ?>>, memref<1x32xbf16>) -> ()
      %63 = "bufferization.to_tensor"(%62) <{restrict, writable}> : (memref<1x32xbf16>) -> tensor<1x32xbf16>
      %64 = "hivm.hir.vcast"(%63, %52) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<1x32xbf16>, tensor<1x32xf32>) -> tensor<1x32xf32>
      %65 = "hivm.hir.vbrc"(%25, %39) <{broadcast_dims = array<i64: 0>}> : (tensor<1x32xf32>, tensor<8x32xf32>) -> tensor<8x32xf32>
      %66 = "hivm.hir.vmul"(%42, %65, %38) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<8x32xf32>, tensor<8x32xf32>, tensor<8x32xf32>) -> tensor<8x32xf32>
      %67 = "hivm.hir.vbrc"(%28, %37) <{broadcast_dims = array<i64: 0>}> : (tensor<1x32xf32>, tensor<8x32xf32>) -> tensor<8x32xf32>
      %68 = "hivm.hir.vmul"(%59, %67, %36) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<8x32xf32>, tensor<8x32xf32>, tensor<8x32xf32>) -> tensor<8x32xf32>
      %69 = "hivm.hir.vsub"(%66, %68, %35) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<8x32xf32>, tensor<8x32xf32>, tensor<8x32xf32>) -> tensor<8x32xf32>
      %70 = "tensor.empty"() : () -> tensor<8x32xbf16>
      %71 = "tensor.empty"() : () -> tensor<8x32xbf16>
      %72 = "hivm.hir.vcast"(%69, %71) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<8x32xf32>, tensor<8x32xbf16>) -> tensor<8x32xbf16>
      "hivm.hir.store"(%72, %29) : (tensor<8x32xbf16>, memref<8x32xbf16, strided<[128, 1], offset: ?>>) -> ()
      %73 = "hivm.hir.vmul"(%59, %65, %34) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<8x32xf32>, tensor<8x32xf32>, tensor<8x32xf32>) -> tensor<8x32xf32>
      %74 = "hivm.hir.vmul"(%42, %67, %33) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<8x32xf32>, tensor<8x32xf32>, tensor<8x32xf32>) -> tensor<8x32xf32>
      %75 = "hivm.hir.vadd"(%73, %74, %32) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<8x32xf32>, tensor<8x32xf32>, tensor<8x32xf32>) -> tensor<8x32xf32>
      %76 = "hivm.hir.vcast"(%75, %70) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<8x32xf32>, tensor<8x32xbf16>) -> tensor<8x32xbf16>
      "hivm.hir.store"(%76, %56) : (tensor<8x32xbf16>, memref<8x32xbf16, strided<[128, 1], offset: ?>>) -> ()
      %77 = "hivm.hir.vmul"(%54, %25, %51) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1x32xf32>, tensor<1x32xf32>, tensor<1x32xf32>) -> tensor<1x32xf32>
      %78 = "hivm.hir.vmul"(%64, %28, %50) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1x32xf32>, tensor<1x32xf32>, tensor<1x32xf32>) -> tensor<1x32xf32>
      %79 = "hivm.hir.vsub"(%77, %78, %49) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1x32xf32>, tensor<1x32xf32>, tensor<1x32xf32>) -> tensor<1x32xf32>
      %80 = "tensor.empty"() : () -> tensor<1x32xbf16>
      %81 = "tensor.empty"() : () -> tensor<1x32xbf16>
      %82 = "hivm.hir.vcast"(%79, %81) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<1x32xf32>, tensor<1x32xbf16>) -> tensor<1x32xbf16>
      "hivm.hir.store"(%82, %43) : (tensor<1x32xbf16>, memref<1x32xbf16, strided<[32, 1], offset: ?>>) -> ()
      %83 = "hivm.hir.vmul"(%64, %25, %48) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1x32xf32>, tensor<1x32xf32>, tensor<1x32xf32>) -> tensor<1x32xf32>
      %84 = "hivm.hir.vmul"(%54, %28, %47) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1x32xf32>, tensor<1x32xf32>, tensor<1x32xf32>) -> tensor<1x32xf32>
      %85 = "hivm.hir.vadd"(%83, %84, %46) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1x32xf32>, tensor<1x32xf32>, tensor<1x32xf32>) -> tensor<1x32xf32>
      %86 = "hivm.hir.vcast"(%85, %80) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<1x32xf32>, tensor<1x32xbf16>) -> tensor<1x32xbf16>
      "hivm.hir.store"(%86, %61) : (tensor<1x32xbf16>, memref<1x32xbf16, strided<[32, 1], offset: ?>>) -> ()
      "scf.yield"() : () -> ()
    }) : (i64, i64, i64) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, false, true, false, true, false, true, false, false, false, false, false]> : vector<15xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

