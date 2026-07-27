#map = affine_map<()[s0] -> (s0, 0)>
#map1 = affine_map<()[s0] -> (s0, 1024)>
"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {}, {}, {}, {}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xf16>, memref<?xf16>, i32, i32, i32, i32, i32, i32, i32) -> (), sym_name = "softmax_kernel"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xf16>, %arg4: memref<?xf16>, %arg5: i32, %arg6: i32, %arg7: i32, %arg8: i32, %arg9: i32, %arg10: i32, %arg11: i32):
    %0 = "arith.constant"() <{value = 0xFC00 : f16}> : () -> f16
    %1 = "arith.constant"() <{value = 1024 : index}> : () -> index
    %2 = "arith.constant"() <{value = 0 : index}> : () -> index
    "hivm.hir.set_mask_norm"() : () -> ()
    %3 = "arith.muli"(%arg9, %arg10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %4 = "arith.muli"(%3, %arg11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%4) <{effects = ["write"]}> {logical_block_num} : (i32) -> ()
    %5 = "hivm.hir.get_block_idx"() : () -> i64
    %6 = "arith.trunci"(%5) : (i64) -> i32
    %7 = "arith.muli"(%arg11, %arg10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %8 = "arith.divsi"(%6, %7) : (i32, i32) -> i32
    %9 = "arith.remsi"(%8, %arg9) : (i32, i32) -> i32
    "scf.for"(%9, %arg7, %arg9) ({
    ^bb0(%arg12: i32):
      %10 = "arith.muli"(%arg12, %arg5) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %11 = "arith.index_cast"(%10) : (i32) -> index
      %12 = "memref.reinterpret_cast"(%arg4, %11) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1024>, static_strides = array<i64: 1>}> : (memref<?xf16>, index) -> memref<1024xf16, strided<[1], offset: ?>>
      %13 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<1024xf16>
      %14 = "arith.index_cast"(%arg8) : (i32) -> index
      %15 = "affine.max"(%14) <{map = #map}> : (index) -> index
      %16 = "affine.min"(%15) <{map = #map1}> : (index) -> index
      %17 = "arith.cmpi"(%16, %1) <{predicate = 2 : i64}> : (index, index) -> i1
      %18 = "memref.subview"(%12, %16) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<1024xf16, strided<[1], offset: ?>>, index) -> memref<?xf16, strided<[1], offset: ?>>
      %19 = "memref.subview"(%13, %16) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<1024xf16>, index) -> memref<?xf16, strided<[1]>>
      "hivm.hir.load"(%18, %19, %0, %2, %17) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xf16, strided<[1], offset: ?>>, memref<?xf16, strided<[1]>>, f16, index, i1) -> ()
      %20 = "bufferization.to_tensor"(%13) <{restrict, writable}> : (memref<1024xf16>) -> tensor<1024xf16>
      %21 = "tensor.empty"() : () -> tensor<1024xf32>
      %22 = "hivm.hir.vcast"(%20, %21) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<1024xf16>, tensor<1024xf32>) -> tensor<1024xf32>
      %23 = "bufferization.alloc_tensor"() <{operandSegmentSizes = array<i32: 0, 0, 0>}> : () -> tensor<f32>
      %24 = "tensor.empty"() : () -> tensor<1xf32>
      %25 = "hivm.hir.vreduce"(%22, %24) <{arith = #hivm.reduce_op<max>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 0>, tie_break_left = true, unsigned_src = false}> : (tensor<1024xf32>, tensor<1xf32>) -> tensor<1xf32>
      %26 = "tensor.extract"(%25, %2) : (tensor<1xf32>, index) -> f32
      %27 = "hivm.hir.vsub"(%22, %26, %21) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, f32, tensor<1024xf32>) -> tensor<1024xf32>
      %28 = "hivm.hir.vexp"(%27, %21) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, tensor<1024xf32>) -> tensor<1024xf32>
      %29 = "bufferization.alloc_tensor"() <{operandSegmentSizes = array<i32: 0, 0, 0>}> : () -> tensor<f32>
      %30 = "hivm.hir.vreduce"(%28, %24) <{arith = #hivm.reduce_op<sum>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 0>, tie_break_left = true, unsigned_src = false}> : (tensor<1024xf32>, tensor<1xf32>) -> tensor<1xf32>
      %31 = "tensor.extract"(%30, %2) : (tensor<1xf32>, index) -> f32
      %32 = "hivm.hir.vdiv"(%28, %31, %21) <{broadcast = array<i64>, isHP = false, isSigned = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, f32, tensor<1024xf32>) -> tensor<1024xf32>
      %33 = "arith.muli"(%arg12, %arg6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %34 = "arith.index_cast"(%33) : (i32) -> index
      %35 = "memref.reinterpret_cast"(%arg3, %34) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1024>, static_strides = array<i64: 1>}> : (memref<?xf16>, index) -> memref<1024xf16, strided<[1], offset: ?>>
      %36 = "tensor.empty"() : () -> tensor<1024xf16>
      %37 = "hivm.hir.vcast"(%32, %36) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<1024xf32>, tensor<1024xf16>) -> tensor<1024xf16>
      %38 = "tensor.extract_slice"(%37, %16) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<1024xf16>, index) -> tensor<?xf16>
      %39 = "memref.subview"(%35, %16) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<1024xf16, strided<[1], offset: ?>>, index) -> memref<?xf16, strided<[1], offset: ?>>
      "hivm.hir.store"(%38, %39) : (tensor<?xf16>, memref<?xf16, strided<[1], offset: ?>>) -> ()
      "scf.yield"() : () -> ()
    }) : (i32, i32, i32) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, false, false, false, false, false, false, false]> : vector<12xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hacc.target = #hacc.target<"Ascend910B1">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

