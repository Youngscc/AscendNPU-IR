#map = affine_map<()[s0] -> (s0 * 32)>
#map1 = affine_map<()[s0, s1] -> (s0 + s1)>
"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xf32>, memref<?xf32>, i32, i32, i32) -> (), sym_name = "triton_softmax_2d"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xf32>, %arg4: memref<?xf32>, %arg5: i32, %arg6: i32, %arg7: i32):
    %0 = "arith.constant"() <{value = 16 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = 32 : i32}> : () -> i32
    "hivm.hir.set_mask_norm"() : () -> ()
    %2 = "arith.muli"(%arg5, %arg6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %3 = "arith.muli"(%2, %arg7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%3) <{effects = ["write"]}> {logical_block_num} : (i32) -> ()
    %4 = "hivm.hir.get_block_idx"() : () -> i64
    %5 = "arith.trunci"(%4) : (i64) -> i32
    %6 = "arith.divsi"(%5, %arg7) : (i32, i32) -> i32
    %7 = "arith.remsi"(%6, %arg6) : (i32, i32) -> i32
    %8 = "arith.muli"(%arg7, %arg6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %9 = "arith.divsi"(%5, %8) : (i32, i32) -> i32
    %10 = "arith.remsi"(%9, %arg5) : (i32, i32) -> i32
    %11 = "arith.muli"(%10, %0) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %12 = "arith.muli"(%7, %1) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %13 = "arith.index_cast"(%11) : (i32) -> index
    %14 = "affine.apply"(%13) <{map = #map}> : (index) -> index
    %15 = "arith.index_cast"(%12) : (i32) -> index
    %16 = "affine.apply"(%14, %15) <{map = #map1}> : (index, index) -> index
    %17 = "memref.reinterpret_cast"(%arg3, %16) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 32>, static_strides = array<i64: 32, 1>}> : (memref<?xf32>, index) -> memref<16x32xf32, strided<[32, 1], offset: ?>>
    %18 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x32xf32>
    "hivm.hir.load"(%17, %18) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<16x32xf32, strided<[32, 1], offset: ?>>, memref<16x32xf32>) -> ()
    %19 = "bufferization.to_tensor"(%18) <{restrict, writable}> : (memref<16x32xf32>) -> tensor<16x32xf32>
    %20 = "tensor.empty"() : () -> tensor<1x32xf32>
    %21 = "hivm.hir.vreduce"(%19, %20) <{arith = #hivm.reduce_op<max>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 0>, tie_break_left = true, unsigned_src = false}> : (tensor<16x32xf32>, tensor<1x32xf32>) -> tensor<1x32xf32>
    %22 = "tensor.empty"() : () -> tensor<16x32xf32>
    %23 = "tensor.empty"() : () -> tensor<16x32xf32>
    %24 = "tensor.empty"() : () -> tensor<16x32xf32>
    %25 = "tensor.empty"() : () -> tensor<16x32xf32>
    %26 = "tensor.empty"() : () -> tensor<16x32xf32>
    %27 = "hivm.hir.vbrc"(%21, %26) <{broadcast_dims = array<i64: 0>}> : (tensor<1x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
    %28 = "hivm.hir.vsub"(%19, %27, %25) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
    %29 = "hivm.hir.vexp"(%28, %24) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
    %30 = "tensor.empty"() : () -> tensor<1x32xf32>
    %31 = "hivm.hir.vreduce"(%29, %30) <{arith = #hivm.reduce_op<sum>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 0>, tie_break_left = true, unsigned_src = false}> : (tensor<16x32xf32>, tensor<1x32xf32>) -> tensor<1x32xf32>
    %32 = "hivm.hir.vbrc"(%31, %23) <{broadcast_dims = array<i64: 0>}> : (tensor<1x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
    %33 = "hivm.hir.vdiv"(%29, %32, %22) <{broadcast = array<i64>, isHP = false, isSigned = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
    %34 = "memref.reinterpret_cast"(%arg4, %16) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 32>, static_strides = array<i64: 32, 1>}> : (memref<?xf32>, index) -> memref<16x32xf32, strided<[32, 1], offset: ?>>
    "hivm.hir.store"(%33, %34) : (tensor<16x32xf32>, memref<16x32xf32, strided<[32, 1], offset: ?>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, false, false, false]> : vector<8xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hacc.target = #hacc.target<"Ascend910B1">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

