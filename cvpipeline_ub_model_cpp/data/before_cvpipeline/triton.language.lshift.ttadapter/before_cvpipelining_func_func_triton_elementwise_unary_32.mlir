"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xi32>, memref<?xi32>, i32, i32, i32) -> (), sym_name = "triton_elementwise_unary"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xi32>, %arg4: memref<?xi32>, %arg5: i32, %arg6: i32, %arg7: i32):
    %0 = "arith.constant"() <{value = 32 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %2 = "arith.constant"() <{value = 2 : i32}> : () -> i32
    "hivm.hir.set_mask_norm"() : () -> ()
    %3 = "arith.muli"(%arg5, %arg6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %4 = "arith.muli"(%3, %arg7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%4) {logical_block_num} : (i32) -> ()
    %5 = "tensor.empty"() : () -> tensor<32xi32>
    %6 = "tensor.empty"() : () -> tensor<32xi32>
    %7 = "tensor.empty"() : () -> tensor<32xi32>
    %8 = "tensor.empty"() : () -> tensor<32xi32>
    %9 = "tensor.empty"() : () -> tensor<32xi32>
    %10 = "tensor.empty"() : () -> tensor<32xi32>
    %11 = "tensor.empty"() : () -> tensor<32xi32>
    %12 = "hivm.hir.vbrc"(%2, %11) <{broadcast_dims = array<i64>}> : (i32, tensor<32xi32>) -> tensor<32xi32>
    %13 = "memref.reinterpret_cast"(%arg3) <{operandSegmentSizes = array<i32: 1, 0, 0, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: 32>, static_strides = array<i64: 1>}> : (memref<?xi32>) -> memref<32xi32, strided<[1]>>
    %14 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<32xi32>
    "hivm.hir.load"(%13, %14) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<32xi32, strided<[1]>>, memref<32xi32>) -> ()
    %15 = "bufferization.to_tensor"(%14) <{restrict, writable}> : (memref<32xi32>) -> tensor<32xi32>
    %16 = "tensor.empty"() : () -> tensor<32xi32>
    %17 = "hivm.hir.vbrc"(%2, %16) <{broadcast_dims = array<i64>}> : (i32, tensor<32xi32>) -> tensor<32xi32>
    %18 = "hivm.hir.vmax"(%17, %1, %10) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32xi32>, i32, tensor<32xi32>) -> tensor<32xi32>
    %19 = "tensor.empty"() : () -> tensor<32xi1>
    %20 = "tensor.empty"() : () -> tensor<32xi1>
    %21 = "tensor.empty"() : () -> tensor<32xi1>
    %22 = "tensor.empty"() : () -> tensor<32xi1>
    %23 = "hivm.hir.vcmp"(%18, %12, %22) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<32xi32>, tensor<32xi32>, tensor<32xi1>) -> tensor<32xi1>
    %24 = "tensor.empty"() : () -> tensor<32xi32>
    %25 = "hivm.hir.vbrc"(%2, %24) <{broadcast_dims = array<i64>}> : (i32, tensor<32xi32>) -> tensor<32xi32>
    %26 = "hivm.hir.vmax"(%25, %0, %9) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32xi32>, i32, tensor<32xi32>) -> tensor<32xi32>
    %27 = "hivm.hir.vcmp"(%26, %12, %21) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<32xi32>, tensor<32xi32>, tensor<32xi1>) -> tensor<32xi1>
    %28 = "hivm.hir.vnot"(%27, %20) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<32xi1>, tensor<32xi1>) -> tensor<32xi1>
    %29 = "hivm.hir.vand"(%23, %28, %19) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32xi1>, tensor<32xi1>, tensor<32xi1>) -> tensor<32xi1>
    %30 = "hivm.hir.vsel"(%29, %2, %1, %8) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<32xi1>, i32, i32, tensor<32xi32>) -> tensor<32xi32>
    %31 = "tensor.empty"() : () -> tensor<32xi32>
    %32 = "hivm.hir.vbrc"(%2, %31) <{broadcast_dims = array<i64>}> : (i32, tensor<32xi32>) -> tensor<32xi32>
    %33 = "hivm.hir.vpow"(%32, %30, %7) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32xi32>, tensor<32xi32>, tensor<32xi32>) -> tensor<32xi32>
    %34 = "hivm.hir.vmul"(%15, %33, %6) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32xi32>, tensor<32xi32>, tensor<32xi32>) -> tensor<32xi32>
    %35 = "hivm.hir.vsel"(%29, %34, %1, %5) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<32xi1>, tensor<32xi32>, i32, tensor<32xi32>) -> tensor<32xi32>
    %36 = "memref.reinterpret_cast"(%arg4) <{operandSegmentSizes = array<i32: 1, 0, 0, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: 32>, static_strides = array<i64: 1>}> : (memref<?xi32>) -> memref<32xi32, strided<[1]>>
    "hivm.hir.store"(%35, %36) : (tensor<32xi32>, memref<32xi32, strided<[1]>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, false, false, false]> : vector<8xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

