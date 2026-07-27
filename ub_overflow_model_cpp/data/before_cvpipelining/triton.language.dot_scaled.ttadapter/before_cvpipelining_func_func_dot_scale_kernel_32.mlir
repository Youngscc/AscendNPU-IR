"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xbf16>, memref<?xi8>, memref<?xbf16>, memref<?xi8>, memref<?xbf16>, i32, i32, i32) -> (), sym_name = "dot_scale_kernel"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xbf16>, %arg4: memref<?xi8>, %arg5: memref<?xbf16>, %arg6: memref<?xi8>, %arg7: memref<?xbf16>, %arg8: i32, %arg9: i32, %arg10: i32):
    %0 = "arith.constant"() <{value = true}> : () -> i1
    %1 = "arith.constant"() <{value = 7 : i16}> : () -> i16
    %2 = "arith.constant"() <{value = 127 : i16}> : () -> i16
    %3 = "arith.constant"() <{value = 16 : index}> : () -> index
    %4 = "arith.constant"() <{value = 32 : index}> : () -> index
    "hivm.hir.set_mask_norm"() : () -> ()
    %5 = "arith.muli"(%arg8, %arg9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %6 = "arith.muli"(%5, %arg10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%6) <{effects = ["write"]}> {logical_block_num} : (i32) -> ()
    %7 = "memref.reinterpret_cast"(%arg3) <{operandSegmentSizes = array<i32: 1, 0, 0, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: 16, 32>, static_strides = array<i64: 32, 1>}> : (memref<?xbf16>) -> memref<16x32xbf16, strided<[32, 1]>>
    %8 = "memref.reinterpret_cast"(%arg5) <{operandSegmentSizes = array<i32: 1, 0, 0, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: 32, 16>, static_strides = array<i64: 32, 1>}> : (memref<?xbf16>) -> memref<32x16xbf16, strided<[32, 1]>>
    %9 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x32xbf16>
    "hivm.hir.load"(%7, %9) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<16x32xbf16, strided<[32, 1]>>, memref<16x32xbf16>) -> ()
    %10 = "bufferization.to_tensor"(%9) <{restrict, writable}> : (memref<16x32xbf16>) -> tensor<16x32xbf16>
    %11 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<32x16xbf16>
    "hivm.hir.load"(%8, %11) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<32x16xbf16, strided<[32, 1]>>, memref<32x16xbf16>) -> ()
    %12 = "bufferization.to_tensor"(%11) <{restrict, writable}> : (memref<32x16xbf16>) -> tensor<32x16xbf16>
    %13 = "memref.reinterpret_cast"(%arg4) <{operandSegmentSizes = array<i32: 1, 0, 0, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: 16, 1>, static_strides = array<i64: 1, 1>}> : (memref<?xi8>) -> memref<16x1xi8, strided<[1, 1]>>
    %14 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x1xi8>
    "hivm.hir.load"(%13, %14) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<16x1xi8, strided<[1, 1]>>, memref<16x1xi8>) -> ()
    %15 = "bufferization.to_tensor"(%14) <{restrict, writable}> : (memref<16x1xi8>) -> tensor<16x1xi8>
    %16 = "memref.reinterpret_cast"(%arg6) <{operandSegmentSizes = array<i32: 1, 0, 0, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: 16, 1>, static_strides = array<i64: 1, 1>}> : (memref<?xi8>) -> memref<16x1xi8, strided<[1, 1]>>
    %17 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x1xi8>
    "hivm.hir.load"(%16, %17) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<16x1xi8, strided<[1, 1]>>, memref<16x1xi8>) -> ()
    %18 = "bufferization.to_tensor"(%17) <{restrict, writable}> : (memref<16x1xi8>) -> tensor<16x1xi8>
    %19 = "tensor.empty"() : () -> tensor<16x1xi16>
    %20 = "tensor.empty"() : () -> tensor<16x1xi16>
    %21 = "tensor.empty"() : () -> tensor<16x1xi16>
    %22 = "tensor.empty"() : () -> tensor<16x1xf16>
    %23 = "hivm.hir.vcast"(%15, %22) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x1xi8>, tensor<16x1xf16>) -> tensor<16x1xf16>
    %24 = "hivm.hir.vcast"(%23, %21) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<trunc>, transpose = array<i64>}> : (tensor<16x1xf16>, tensor<16x1xi16>) -> tensor<16x1xi16>
    %25 = "hivm.hir.vadd"(%24, %2, %20) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x1xi16>, i16, tensor<16x1xi16>) -> tensor<16x1xi16>
    %26 = "hivm.hir.vshl"(%25, %1, %19) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x1xi16>, i16, tensor<16x1xi16>) -> tensor<16x1xi16>
    %27 = "hivm.hir.bitcast"(%26) : (tensor<16x1xi16>) -> tensor<16x1xbf16>
    %28 = "tensor.empty"() : () -> tensor<1x16xi8>
    %29 = "hivm.hir.vtranspose"(%18, %28) <{disable_align = false, permutation = array<i64: 1, 0>}> : (tensor<16x1xi8>, tensor<1x16xi8>) -> tensor<1x16xi8>
    %30 = "tensor.empty"() : () -> tensor<1x16xi16>
    %31 = "tensor.empty"() : () -> tensor<1x16xi16>
    %32 = "tensor.empty"() : () -> tensor<1x16xi16>
    %33 = "tensor.empty"() : () -> tensor<1x16xf16>
    %34 = "hivm.hir.vcast"(%29, %33) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<1x16xi8>, tensor<1x16xf16>) -> tensor<1x16xf16>
    %35 = "hivm.hir.vcast"(%34, %32) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<trunc>, transpose = array<i64>}> : (tensor<1x16xf16>, tensor<1x16xi16>) -> tensor<1x16xi16>
    %36 = "hivm.hir.vadd"(%35, %2, %31) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1x16xi16>, i16, tensor<1x16xi16>) -> tensor<1x16xi16>
    %37 = "hivm.hir.vshl"(%36, %1, %30) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1x16xi16>, i16, tensor<1x16xi16>) -> tensor<1x16xi16>
    %38 = "hivm.hir.bitcast"(%37) : (tensor<1x16xi16>) -> tensor<1x16xbf16>
    %39 = "tensor.empty"() : () -> tensor<1x32x16xbf16>
    %40 = "tensor.expand_shape"(%38) <{reassociation = [[0, 1], [2]], static_output_shape = array<i64: 1, 1, 16>}> : (tensor<1x16xbf16>) -> tensor<1x1x16xbf16>
    %41 = "hivm.hir.vbrc"(%40, %39) <{broadcast_dims = array<i64: 1>}> : (tensor<1x1x16xbf16>, tensor<1x32x16xbf16>) -> tensor<1x32x16xbf16>
    %42 = "tensor.collapse_shape"(%41) <{reassociation = [[0, 1], [2]]}> : (tensor<1x32x16xbf16>) -> tensor<32x16xbf16>
    %43 = "tensor.empty"() : () -> tensor<32x16xbf16>
    %44 = "tensor.empty"() : () -> tensor<32x16xf32>
    %45 = "tensor.empty"() : () -> tensor<32x16xf32>
    %46 = "tensor.empty"() : () -> tensor<32x16xf32>
    %47 = "hivm.hir.vcast"(%12, %46) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<32x16xbf16>, tensor<32x16xf32>) -> tensor<32x16xf32>
    %48 = "hivm.hir.vcast"(%42, %45) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<32x16xbf16>, tensor<32x16xf32>) -> tensor<32x16xf32>
    %49 = "hivm.hir.vmul"(%47, %48, %44) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32x16xf32>, tensor<32x16xf32>, tensor<32x16xf32>) -> tensor<32x16xf32>
    %50 = "hivm.hir.vcast"(%49, %43) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<32x16xf32>, tensor<32x16xbf16>) -> tensor<32x16xbf16>
    %51 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<32x16xbf16>
    %52 = "bufferization.to_tensor"(%51) <{restrict, writable}> : (memref<32x16xbf16>) -> tensor<32x16xbf16>
    %53 = "hivm.hir.store"(%50, %52) {"inserted-store"} : (tensor<32x16xbf16>, tensor<32x16xbf16>) -> tensor<32x16xbf16>
    %54 = "tensor.empty"() : () -> tensor<32x16xbf16>
    %55 = "hivm.hir.load"(%53, %54) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<32x16xbf16>, tensor<32x16xbf16>) -> tensor<32x16xbf16>
    %56 = "tensor.empty"() : () -> tensor<16x1x32xbf16>
    %57 = "tensor.expand_shape"(%27) <{reassociation = [[0], [1, 2]], static_output_shape = array<i64: 16, 1, 1>}> : (tensor<16x1xbf16>) -> tensor<16x1x1xbf16>
    %58 = "hivm.hir.vbrc"(%57, %56) <{broadcast_dims = array<i64: 2>}> : (tensor<16x1x1xbf16>, tensor<16x1x32xbf16>) -> tensor<16x1x32xbf16>
    %59 = "tensor.collapse_shape"(%58) <{reassociation = [[0], [1, 2]]}> : (tensor<16x1x32xbf16>) -> tensor<16x32xbf16>
    %60 = "tensor.empty"() : () -> tensor<16x32xbf16>
    %61 = "tensor.empty"() : () -> tensor<16x32xf32>
    %62 = "tensor.empty"() : () -> tensor<16x32xf32>
    %63 = "tensor.empty"() : () -> tensor<16x32xf32>
    %64 = "hivm.hir.vcast"(%10, %63) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x32xbf16>, tensor<16x32xf32>) -> tensor<16x32xf32>
    %65 = "hivm.hir.vcast"(%59, %62) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x32xbf16>, tensor<16x32xf32>) -> tensor<16x32xf32>
    %66 = "hivm.hir.vmul"(%64, %65, %61) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
    %67 = "hivm.hir.vcast"(%66, %60) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xbf16>) -> tensor<16x32xbf16>
    %68 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x32xbf16>
    %69 = "bufferization.to_tensor"(%68) <{restrict, writable}> : (memref<16x32xbf16>) -> tensor<16x32xbf16>
    %70 = "hivm.hir.store"(%67, %69) {"inserted-store"} : (tensor<16x32xbf16>, tensor<16x32xbf16>) -> tensor<16x32xbf16>
    %71 = "tensor.empty"() : () -> tensor<16x32xbf16>
    %72 = "hivm.hir.load"(%70, %71) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x32xbf16>, tensor<16x32xbf16>) -> tensor<16x32xbf16>
    %73 = "tensor.empty"() : () -> tensor<16x16xf32>
    %74 = "hivm.hir.mmadL1"(%72, %55, %0, %3, %4, %3, %73) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_for_result_already_inserted = true} : (tensor<16x32xbf16>, tensor<32x16xbf16>, i1, index, index, index, tensor<16x16xf32>) -> tensor<16x16xf32>
    %75 = "memref.reinterpret_cast"(%arg7) <{operandSegmentSizes = array<i32: 1, 0, 0, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 16, 1>}> : (memref<?xbf16>) -> memref<16x16xbf16, strided<[16, 1]>>
    "hivm.hir.fixpipe"(%74, %75) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, pre_quant = #hivm.fixpipe_pre_quant_mode<F322BF16>}> : (tensor<16x16xf32>, memref<16x16xbf16, strided<[16, 1]>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, true, true, false, false, false]> : vector<11xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<MIX>, mix_mode = "mix", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hacc.target = #hacc.target<"Ascend910B1">, hivm.module_core_type = #hivm.module_core_type<MIX>} : () -> ()

