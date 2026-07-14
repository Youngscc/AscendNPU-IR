"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xi32>, memref<?xi32>, memref<?xi32>, i32, i32, i32) -> (), sym_name = "assume_kernel"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xi32>, %arg4: memref<?xi32>, %arg5: memref<?xi32>, %arg6: i32, %arg7: i32, %arg8: i32):
    %0 = "arith.constant"() <{value = -1 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %2 = "arith.constant"() <{value = 1.280000e+02 : f32}> : () -> f32
    %3 = "arith.constant"() <{value = true}> : () -> i1
    %4 = "arith.constant"() <{value = 128 : i32}> : () -> i32
    "hivm.hir.set_mask_norm"() : () -> ()
    %5 = "arith.muli"(%arg6, %arg7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %6 = "arith.muli"(%5, %arg8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%6) {logical_block_num} : (i32) -> ()
    %7 = "tensor.empty"() : () -> tensor<128xi32>
    %8 = "tensor.empty"() : () -> tensor<128xi32>
    %9 = "tensor.empty"() : () -> tensor<128xi32>
    %10 = "tensor.empty"() : () -> tensor<128xi32>
    %11 = "tensor.empty"() : () -> tensor<128xi32>
    %12 = "hivm.hir.vbrc"(%4, %11) <{broadcast_dims = array<i64>}> : (i32, tensor<128xi32>) -> tensor<128xi32>
    "llvm.intr.assume"(%3) : (i1) -> ()
    %13 = "memref.reinterpret_cast"(%arg3) <{operandSegmentSizes = array<i32: 1, 0, 0, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: 128>, static_strides = array<i64: 1>}> : (memref<?xi32>) -> memref<128xi32, strided<[1]>>
    %14 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<128xi32>
    "hivm.hir.load"(%13, %14) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<128xi32, strided<[1]>>, memref<128xi32>) -> ()
    %15 = "bufferization.to_tensor"(%14) <{restrict, writable}> : (memref<128xi32>) -> tensor<128xi32>
    %16 = "memref.reinterpret_cast"(%arg4) <{operandSegmentSizes = array<i32: 1, 0, 0, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: 128>, static_strides = array<i64: 1>}> : (memref<?xi32>) -> memref<128xi32, strided<[1]>>
    %17 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<128xi32>
    "hivm.hir.load"(%16, %17) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<128xi32, strided<[1]>>, memref<128xi32>) -> ()
    %18 = "bufferization.to_tensor"(%17) <{restrict, writable}> : (memref<128xi32>) -> tensor<128xi32>
    %19 = "tensor.empty"() : () -> tensor<128xf32>
    %20 = "tensor.empty"() : () -> tensor<128xf32>
    %21 = "hivm.hir.vcast"(%15, %20) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<128xi32>, tensor<128xf32>) -> tensor<128xf32>
    %22 = "hivm.hir.vdiv"(%21, %2, %19) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xf32>, f32, tensor<128xf32>) -> tensor<128xf32>
    %23 = "hivm.hir.vcast"(%22, %10) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<trunc>, transpose = array<i64>}> : (tensor<128xf32>, tensor<128xi32>) -> tensor<128xi32>
    %24 = "hivm.hir.vmod"(%18, %4, %9) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<128xi32>, i32, tensor<128xi32>) -> tensor<128xi32>
    %25 = "tensor.empty"() : () -> tensor<128xi1>
    %26 = "hivm.hir.vcmp"(%12, %1, %25) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<128xi32>, i32, tensor<128xi1>) -> tensor<128xi1>
    %27 = "hivm.hir.vsel"(%26, %0, %24, %8) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<128xi1>, i32, tensor<128xi32>, tensor<128xi32>) -> tensor<128xi32>
    %28 = "hivm.hir.vadd"(%23, %27, %7) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xi32>, tensor<128xi32>, tensor<128xi32>) -> tensor<128xi32>
    %29 = "memref.reinterpret_cast"(%arg5) <{operandSegmentSizes = array<i32: 1, 0, 0, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: 128>, static_strides = array<i64: 1>}> : (memref<?xi32>) -> memref<128xi32, strided<[1]>>
    "hivm.hir.store"(%28, %29) : (tensor<128xi32>, memref<128xi32, strided<[1]>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, false, false, false]> : vector<9xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

