"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xi16>, memref<?xi16>, i32, i32, i32) -> (), sym_name = "sort_kernel_1d"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xi16>, %arg4: memref<?xi16>, %arg5: i32, %arg6: i32, %arg7: i32):
    %0 = "arith.constant"() <{value = 1 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = 0 : index}> : () -> index
    %2 = "arith.constant"() <{value = 1 : index}> : () -> index
    "hivm.hir.set_mask_norm"() : () -> ()
    %3 = "arith.muli"(%arg5, %arg6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %4 = "arith.muli"(%3, %arg7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%4) {logical_block_num} : (i32) -> ()
    %5 = "tensor.empty"() : () -> tensor<2xi32>
    %6 = "tensor.empty"() : () -> tensor<2xi32>
    %7 = "tensor.empty"() : () -> tensor<2xi32>
    %8 = "tensor.empty"() : () -> tensor<2xi32>
    %9 = "tensor.empty"() : () -> tensor<2xi32>
    %10 = "hivm.hir.varange"(%9, %1, %2) <{operandSegmentSizes = array<i32: 1, 1, 1>}> : (tensor<2xi32>, index, index) -> tensor<2xi32>
    %11 = "memref.reinterpret_cast"(%arg3) <{operandSegmentSizes = array<i32: 1, 0, 0, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: 2>, static_strides = array<i64: 1>}> : (memref<?xi16>) -> memref<2xi16, strided<[1]>>
    %12 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<2xi16>
    "hivm.hir.load"(%11, %12) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<2xi16, strided<[1]>>, memref<2xi16>) -> ()
    %13 = "bufferization.to_tensor"(%12) <{restrict, writable}> : (memref<2xi16>) -> tensor<2xi16>
    %14 = "bufferization.alloc_tensor"() <{operandSegmentSizes = array<i32: 0, 0, 0>}> : () -> tensor<i16>
    %15 = "tensor.empty"() : () -> tensor<1xi16>
    %16 = "hivm.hir.vreduce"(%13, %15) <{arith = #hivm.reduce_op<xori>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 0>}> : (tensor<2xi16>, tensor<1xi16>) -> tensor<1xi16>
    %17 = "tensor.extract"(%16, %1) : (tensor<1xi16>, index) -> i16
    %18 = "tensor.empty"() : () -> tensor<2xi16>
    %19 = "tensor.empty"() : () -> tensor<2xi16>
    %20 = "tensor.empty"() : () -> tensor<2xi16>
    %21 = "tensor.empty"() : () -> tensor<2xi16>
    %22 = "tensor.empty"() : () -> tensor<2xi16>
    %23 = "hivm.hir.vbrc"(%17, %22) <{broadcast_dims = array<i64>}> : (i16, tensor<2xi16>) -> tensor<2xi16>
    %24 = "hivm.hir.vor"(%13, %23, %21) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2xi16>, tensor<2xi16>, tensor<2xi16>) -> tensor<2xi16>
    %25 = "tensor.empty"() : () -> tensor<2xi16>
    %26 = "hivm.hir.vbrc"(%17, %25) <{broadcast_dims = array<i64>}> : (i16, tensor<2xi16>) -> tensor<2xi16>
    %27 = "hivm.hir.vand"(%13, %26, %20) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2xi16>, tensor<2xi16>, tensor<2xi16>) -> tensor<2xi16>
    %28 = "hivm.hir.vnot"(%27, %27) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<2xi16>, tensor<2xi16>) -> tensor<2xi16>
    %29 = "hivm.hir.vand"(%28, %24, %19) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2xi16>, tensor<2xi16>, tensor<2xi16>) -> tensor<2xi16>
    %30 = "tensor.empty"() : () -> tensor<2xi1>
    %31 = "tensor.empty"() : () -> tensor<2xi1>
    %32 = "tensor.empty"() : () -> tensor<2xi1>
    %33 = "hivm.hir.vcmp"(%13, %29, %32) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<gt>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<2xi16>, tensor<2xi16>, tensor<2xi1>) -> tensor<2xi1>
    %34 = "tensor.empty"() : () -> tensor<2xi32>
    %35 = "hivm.hir.vbrc"(%0, %34) <{broadcast_dims = array<i64>}> : (i32, tensor<2xi32>) -> tensor<2xi32>
    %36 = "hivm.hir.vor"(%10, %35, %8) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2xi32>, tensor<2xi32>, tensor<2xi32>) -> tensor<2xi32>
    %37 = "tensor.empty"() : () -> tensor<2xi32>
    %38 = "hivm.hir.vbrc"(%0, %37) <{broadcast_dims = array<i64>}> : (i32, tensor<2xi32>) -> tensor<2xi32>
    %39 = "hivm.hir.vand"(%10, %38, %7) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2xi32>, tensor<2xi32>, tensor<2xi32>) -> tensor<2xi32>
    %40 = "hivm.hir.vnot"(%39, %39) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<2xi32>, tensor<2xi32>) -> tensor<2xi32>
    %41 = "hivm.hir.vand"(%40, %36, %6) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2xi32>, tensor<2xi32>, tensor<2xi32>) -> tensor<2xi32>
    %42 = "hivm.hir.vcast"(%33, %5) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<2xi1>, tensor<2xi32>) -> tensor<2xi32>
    %43 = "hivm.hir.vcmp"(%42, %41, %31) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<2xi32>, tensor<2xi32>, tensor<2xi1>) -> tensor<2xi1>
    %44 = "hivm.hir.vnot"(%43, %30) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<2xi1>, tensor<2xi1>) -> tensor<2xi1>
    %45 = "hivm.hir.vsel"(%44, %29, %13, %18) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<2xi1>, tensor<2xi16>, tensor<2xi16>, tensor<2xi16>) -> tensor<2xi16>
    %46 = "memref.reinterpret_cast"(%arg4) <{operandSegmentSizes = array<i32: 1, 0, 0, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: 2>, static_strides = array<i64: 1>}> : (memref<?xi16>) -> memref<2xi16, strided<[1]>>
    "hivm.hir.store"(%45, %46) : (tensor<2xi16>, memref<2xi16, strided<[1]>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, false, false, false]> : vector<8xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

