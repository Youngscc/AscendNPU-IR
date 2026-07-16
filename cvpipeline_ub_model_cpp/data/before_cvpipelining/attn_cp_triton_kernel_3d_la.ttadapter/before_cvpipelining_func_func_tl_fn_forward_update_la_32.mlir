"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 2 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 2 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xbf16>, memref<?xf32>, memref<?xbf16>, memref<?xf32>, i32, i32, i32) -> (), sym_name = "tl_fn_forward_update_la"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xbf16>, %arg4: memref<?xf32>, %arg5: memref<?xbf16>, %arg6: memref<?xf32>, %arg7: i32, %arg8: i32, %arg9: i32):
    %0 = "arith.constant"() <{value = 1 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = 2048 : i32}> : () -> i32
    %2 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %3 = "arith.constant"() <{value = 65536 : i32}> : () -> i32
    %4 = "arith.constant"() <{value = 1024 : i32}> : () -> i32
    %5 = "arith.constant"() <{value = 2 : i32}> : () -> i32
    %6 = "arith.constant"() <{value = 32 : i32}> : () -> i32
    "hivm.hir.set_mask_norm"() : () -> ()
    %7 = "arith.muli"(%arg7, %arg8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %8 = "arith.muli"(%7, %arg9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%8) {logical_block_num} : (i32) -> ()
    %9 = "hivm.hir.get_block_idx"() : () -> i64
    %10 = "arith.trunci"(%9) : (i64) -> i32
    %11 = "arith.muli"(%arg9, %arg8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %12 = "arith.divsi"(%10, %11) : (i32, i32) -> i32
    %13 = "arith.remsi"(%12, %arg7) : (i32, i32) -> i32
    %14 = "arith.muli"(%13, %6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %15 = "arith.muli"(%13, %1) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "scf.for"(%2, %5, %0) ({
    ^bb0(%arg10: i32):
      %16 = "arith.divsi"(%arg10, %5) : (i32, i32) -> i32
      %17 = "arith.remsi"(%arg10, %5) : (i32, i32) -> i32
      %18 = "arith.muli"(%16, %5) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %19 = "arith.addi"(%18, %17) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %20 = "arith.muli"(%19, %4) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %21 = "arith.index_cast"(%20) : (i32) -> index
      %22 = "arith.index_cast"(%14) : (i32) -> index
      %23 = "arith.addi"(%21, %22) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %24 = "memref.reinterpret_cast"(%arg4, %23) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 32>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<32xf32, strided<[1], offset: ?>>
      %25 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<32xf32>
      "hivm.hir.load"(%24, %25) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<32xf32, strided<[1], offset: ?>>, memref<32xf32>) -> ()
      %26 = "bufferization.to_tensor"(%25) <{restrict, writable}> : (memref<32xf32>) -> tensor<32xf32>
      %27 = "memref.reinterpret_cast"(%arg6, %23) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 32>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<32xf32, strided<[1], offset: ?>>
      %28 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<32xf32>
      "hivm.hir.load"(%27, %28) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<32xf32, strided<[1], offset: ?>>, memref<32xf32>) -> ()
      %29 = "bufferization.to_tensor"(%28) <{restrict, writable}> : (memref<32xf32>) -> tensor<32xf32>
      %30 = "arith.muli"(%19, %3) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %31 = "arith.index_cast"(%30) : (i32) -> index
      %32 = "arith.index_cast"(%15) : (i32) -> index
      %33 = "arith.addi"(%31, %32) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %34 = "memref.reinterpret_cast"(%arg3, %33) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 32, 64>, static_strides = array<i64: 64, 1>}> : (memref<?xbf16>, index) -> memref<32x64xbf16, strided<[64, 1], offset: ?>>
      %35 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<32x64xbf16>
      "hivm.hir.load"(%34, %35) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<32x64xbf16, strided<[64, 1], offset: ?>>, memref<32x64xbf16>) -> ()
      %36 = "bufferization.to_tensor"(%35) <{restrict, writable}> : (memref<32x64xbf16>) -> tensor<32x64xbf16>
      %37 = "memref.reinterpret_cast"(%arg5, %33) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 32, 64>, static_strides = array<i64: 64, 1>}> : (memref<?xbf16>, index) -> memref<32x64xbf16, strided<[64, 1], offset: ?>>
      %38 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<32x64xbf16>
      "hivm.hir.load"(%37, %38) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<32x64xbf16, strided<[64, 1], offset: ?>>, memref<32x64xbf16>) -> ()
      %39 = "bufferization.to_tensor"(%38) <{restrict, writable}> : (memref<32x64xbf16>) -> tensor<32x64xbf16>
      %40 = "tensor.empty"() : () -> tensor<32xf32>
      %41 = "tensor.empty"() : () -> tensor<32xf32>
      %42 = "tensor.empty"() : () -> tensor<32xf32>
      %43 = "tensor.empty"() : () -> tensor<32xf32>
      %44 = "tensor.empty"() : () -> tensor<32xf32>
      %45 = "tensor.empty"() : () -> tensor<32xf32>
      %46 = "hivm.hir.vexp"(%29, %45) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<32xf32>, tensor<32xf32>) -> tensor<32xf32>
      %47 = "hivm.hir.vexp"(%26, %44) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<32xf32>, tensor<32xf32>) -> tensor<32xf32>
      %48 = "hivm.hir.vadd"(%46, %47, %43) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32xf32>, tensor<32xf32>, tensor<32xf32>) -> tensor<32xf32>
      %49 = "hivm.hir.vln"(%48, %42) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<32xf32>, tensor<32xf32>) -> tensor<32xf32>
      %50 = "hivm.hir.vsub"(%26, %49, %41) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32xf32>, tensor<32xf32>, tensor<32xf32>) -> tensor<32xf32>
      %51 = "tensor.expand_shape"(%50) <{reassociation = [[0, 1]], static_output_shape = array<i64: 32, 1>}> : (tensor<32xf32>) -> tensor<32x1xf32>
      %52 = "hivm.hir.vsub"(%29, %49, %40) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32xf32>, tensor<32xf32>, tensor<32xf32>) -> tensor<32xf32>
      %53 = "tensor.expand_shape"(%52) <{reassociation = [[0, 1]], static_output_shape = array<i64: 32, 1>}> : (tensor<32xf32>) -> tensor<32x1xf32>
      %54 = "tensor.empty"() : () -> tensor<32x1xf32>
      %55 = "tensor.empty"() : () -> tensor<32x1xf32>
      %56 = "hivm.hir.vexp"(%51, %55) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<32x1xf32>, tensor<32x1xf32>) -> tensor<32x1xf32>
      %57 = "tensor.empty"() : () -> tensor<32x64xf32>
      %58 = "tensor.empty"() : () -> tensor<32x64xf32>
      %59 = "tensor.empty"() : () -> tensor<32x64xf32>
      %60 = "tensor.empty"() : () -> tensor<32x64xf32>
      %61 = "tensor.empty"() : () -> tensor<32x64xf32>
      %62 = "tensor.empty"() : () -> tensor<32x64xf32>
      %63 = "tensor.empty"() : () -> tensor<32x64xf32>
      %64 = "hivm.hir.vbrc"(%56, %63) <{broadcast_dims = array<i64: 1>}> : (tensor<32x1xf32>, tensor<32x64xf32>) -> tensor<32x64xf32>
      %65 = "hivm.hir.vcast"(%36, %62) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<32x64xbf16>, tensor<32x64xf32>) -> tensor<32x64xf32>
      %66 = "hivm.hir.vmul"(%64, %65, %61) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32x64xf32>, tensor<32x64xf32>, tensor<32x64xf32>) -> tensor<32x64xf32>
      %67 = "hivm.hir.vexp"(%53, %54) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<32x1xf32>, tensor<32x1xf32>) -> tensor<32x1xf32>
      %68 = "hivm.hir.vbrc"(%67, %60) <{broadcast_dims = array<i64: 1>}> : (tensor<32x1xf32>, tensor<32x64xf32>) -> tensor<32x64xf32>
      %69 = "hivm.hir.vcast"(%39, %59) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<32x64xbf16>, tensor<32x64xf32>) -> tensor<32x64xf32>
      %70 = "hivm.hir.vmul"(%68, %69, %58) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32x64xf32>, tensor<32x64xf32>, tensor<32x64xf32>) -> tensor<32x64xf32>
      %71 = "hivm.hir.vadd"(%66, %70, %57) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32x64xf32>, tensor<32x64xf32>, tensor<32x64xf32>) -> tensor<32x64xf32>
      "hivm.hir.store"(%49, %24) : (tensor<32xf32>, memref<32xf32, strided<[1], offset: ?>>) -> ()
      %72 = "tensor.empty"() : () -> tensor<32x64xbf16>
      %73 = "hivm.hir.vcast"(%71, %72) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<32x64xf32>, tensor<32x64xbf16>) -> tensor<32x64xbf16>
      "hivm.hir.store"(%73, %34) : (tensor<32x64xbf16>, memref<32x64xbf16, strided<[64, 1], offset: ?>>) -> ()
      "scf.yield"() : () -> ()
    }) : (i32, i32, i32) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, true, false, false, false]> : vector<10xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

