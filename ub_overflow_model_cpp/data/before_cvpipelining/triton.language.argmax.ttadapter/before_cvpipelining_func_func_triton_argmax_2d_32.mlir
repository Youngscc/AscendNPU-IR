"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xf32>, memref<?xi32>, i32, i32, i32) -> (), sym_name = "triton_argmax_2d"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xf32>, %arg4: memref<?xi32>, %arg5: i32, %arg6: i32, %arg7: i32):
    %0 = "arith.constant"() <{value = 1 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = -1 : i32}> : () -> i32
    %2 = "arith.constant"() <{value = -2139095040 : i32}> : () -> i32
    %3 = "arith.constant"() <{value = 0.000000e+00 : f32}> : () -> f32
    %4 = "arith.constant"() <{value = 0x7F800000 : f32}> : () -> f32
    %5 = "arith.constant"() <{value = 2147483647 : i32}> : () -> i32
    "hivm.hir.set_mask_norm"() : () -> ()
    %6 = "arith.muli"(%arg5, %arg6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %7 = "arith.muli"(%6, %arg7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%7) <{effects = ["write"]}> {logical_block_num} : (i32) -> ()
    %8 = "memref.reinterpret_cast"(%arg3) <{operandSegmentSizes = array<i32: 1, 0, 0, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: 4, 8>, static_strides = array<i64: 8, 1>}> : (memref<?xf32>) -> memref<4x8xf32, strided<[8, 1]>>
    %9 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<4x8xf32>
    "hivm.hir.load"(%8, %9) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<4x8xf32, strided<[8, 1]>>, memref<4x8xf32>) -> ()
    %10 = "bufferization.to_tensor"(%9) <{restrict, writable}> : (memref<4x8xf32>) -> tensor<4x8xf32>
    %11 = "tensor.empty"() : () -> tensor<4xf32>
    %12 = "tensor.empty"() : () -> tensor<4xf32>
    %13 = "tensor.empty"() : () -> tensor<4xi32>
    %14 = "tensor.empty"() : () -> tensor<4xi32>
    %15 = "tensor.empty"() : () -> tensor<4xi32>
    %16 = "tensor.empty"() : () -> tensor<4xi32>
    %17 = "tensor.empty"() : () -> tensor<4x1xf32>
    %18 = "tensor.empty"() : () -> tensor<4x1xi32>
    %19:2 = "hivm.hir.vreduce"(%10, %17, %18) <{arith = #hivm.reduce_op<max_with_index_left>, operandSegmentSizes = array<i32: 1, 2, 0, 0>, reduce_dims = array<i64: 1>, tie_break_left = true, unsigned_src = false}> : (tensor<4x8xf32>, tensor<4x1xf32>, tensor<4x1xi32>) -> (tensor<4x1xf32>, tensor<4x1xi32>)
    %20 = "tensor.collapse_shape"(%19#1) <{reassociation = [[0, 1]]}> : (tensor<4x1xi32>) -> tensor<4xi32>
    %21 = "tensor.empty"() : () -> tensor<4x8xi1>
    %22 = "tensor.empty"() : () -> tensor<4x8xi1>
    %23 = "hivm.hir.vcmp"(%10, %10, %22) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<4x8xf32>, tensor<4x8xf32>, tensor<4x8xi1>) -> tensor<4x8xi1>
    %24 = "hivm.hir.vnot"(%23, %21) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<4x8xi1>, tensor<4x8xi1>) -> tensor<4x8xi1>
    %25 = "tensor.empty"() : () -> tensor<4x8xf32>
    %26 = "hivm.hir.vsel"(%24, %4, %3, %25) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<4x8xi1>, f32, f32, tensor<4x8xf32>) -> tensor<4x8xf32>
    %27 = "tensor.empty"() : () -> tensor<4x1xf32>
    %28 = "tensor.empty"() : () -> tensor<4x1xi32>
    %29:2 = "hivm.hir.vreduce"(%26, %27, %28) <{arith = #hivm.reduce_op<max_with_index_left>, operandSegmentSizes = array<i32: 1, 2, 0, 0>, reduce_dims = array<i64: 1>, tie_break_left = true, unsigned_src = false}> : (tensor<4x8xf32>, tensor<4x1xf32>, tensor<4x1xi32>) -> (tensor<4x1xf32>, tensor<4x1xi32>)
    %30 = "tensor.collapse_shape"(%29#0) <{reassociation = [[0, 1]]}> : (tensor<4x1xf32>) -> tensor<4xf32>
    %31 = "tensor.collapse_shape"(%29#1) <{reassociation = [[0, 1]]}> : (tensor<4x1xi32>) -> tensor<4xi32>
    %32 = "hivm.hir.bitcast"(%30) : (tensor<4xf32>) -> tensor<4xi32>
    %33 = "tensor.empty"() : () -> tensor<4xi32>
    %34 = "hivm.hir.vbrc"(%5, %33) <{broadcast_dims = array<i64>}> : (i32, tensor<4xi32>) -> tensor<4xi32>
    %35 = "hivm.hir.vand"(%32, %34, %16) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<4xi32>, tensor<4xi32>, tensor<4xi32>) -> tensor<4xi32>
    %36 = "hivm.hir.vadd"(%35, %2, %15) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<4xi32>, i32, tensor<4xi32>) -> tensor<4xi32>
    %37 = "hivm.hir.bitcast"(%36) : (tensor<4xi32>) -> tensor<4xf32>
    %38 = "hivm.hir.vabs"(%37, %12) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<4xf32>, tensor<4xf32>) -> tensor<4xf32>
    %39 = "hivm.hir.bitcast"(%38) : (tensor<4xf32>) -> tensor<4xi32>
    %40 = "hivm.hir.vmin"(%39, %0, %14) <{broadcast = array<i64>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<4xi32>, i32, tensor<4xi32>) -> tensor<4xi32>
    %41 = "hivm.hir.vmul"(%40, %1, %40) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<4xi32>, i32, tensor<4xi32>) -> tensor<4xi32>
    %42 = "hivm.hir.vadd"(%41, %0, %41) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<4xi32>, i32, tensor<4xi32>) -> tensor<4xi32>
    %43 = "hivm.hir.vcast"(%42, %11) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<4xi32>, tensor<4xf32>) -> tensor<4xf32>
    %44 = "tensor.empty"() : () -> tensor<4xi1>
    %45 = "tensor.empty"() : () -> tensor<4xi1>
    %46 = "hivm.hir.vcmp"(%43, %3, %45) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<4xf32>, f32, tensor<4xi1>) -> tensor<4xi1>
    %47 = "hivm.hir.vnot"(%46, %44) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<4xi1>, tensor<4xi1>) -> tensor<4xi1>
    %48 = "hivm.hir.vsel"(%47, %31, %20, %13) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<4xi1>, tensor<4xi32>, tensor<4xi32>, tensor<4xi32>) -> tensor<4xi32>
    %49 = "memref.reinterpret_cast"(%arg4) <{operandSegmentSizes = array<i32: 1, 0, 0, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: 4>, static_strides = array<i64: 1>}> : (memref<?xi32>) -> memref<4xi32, strided<[1]>>
    "hivm.hir.store"(%48, %49) : (tensor<4xi32>, memref<4xi32, strided<[1]>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, false, false, false]> : vector<8xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hacc.target = #hacc.target<"Ascend910B1">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

