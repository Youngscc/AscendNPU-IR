"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xi32>, memref<?xi32>, i32, i32, i32) -> (), sym_name = "flip_kernel"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xi32>, %arg4: memref<?xi32>, %arg5: i32, %arg6: i32, %arg7: i32):
    "hivm.hir.set_mask_norm"() : () -> ()
    %0 = "arith.muli"(%arg5, %arg6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %1 = "arith.muli"(%0, %arg7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%1) {logical_block_num} : (i32) -> ()
    %2 = "memref.reinterpret_cast"(%arg3) <{operandSegmentSizes = array<i32: 1, 0, 0, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: 8, 2, 2, 2, 2, 2, 2>, static_strides = array<i64: 64, 32, 16, 8, 4, 2, 1>}> : (memref<?xi32>) -> memref<8x2x2x2x2x2x2xi32, strided<[64, 32, 16, 8, 4, 2, 1]>>
    %3 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<8x2x2x2x2x2x2xi32>
    "hivm.hir.load"(%2, %3) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<8x2x2x2x2x2x2xi32, strided<[64, 32, 16, 8, 4, 2, 1]>>, memref<8x2x2x2x2x2x2xi32>) -> ()
    %4 = "bufferization.to_tensor"(%3) <{restrict, writable}> : (memref<8x2x2x2x2x2x2xi32>) -> tensor<8x2x2x2x2x2x2xi32>
    %5 = "tensor.empty"() : () -> tensor<8x1x2x2x2x2x2xi32>
    %6 = "hivm.hir.vreduce"(%4, %5) <{arith = #hivm.reduce_op<xori>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 1>}> : (tensor<8x2x2x2x2x2x2xi32>, tensor<8x1x2x2x2x2x2xi32>) -> tensor<8x1x2x2x2x2x2xi32>
    %7 = "tensor.empty"() : () -> tensor<8x2x2x2x2x2x2xi32>
    %8 = "tensor.empty"() : () -> tensor<8x2x2x2x2x2x2xi32>
    %9 = "tensor.empty"() : () -> tensor<8x2x2x2x2x2x2xi32>
    %10 = "tensor.empty"() : () -> tensor<8x2x2x2x2x2x2xi32>
    %11 = "tensor.empty"() : () -> tensor<8x2x2x2x2x2x2xi32>
    %12 = "tensor.empty"() : () -> tensor<8x2x2x2x2x2x2xi32>
    %13 = "tensor.empty"() : () -> tensor<8x2x2x2x2x2x2xi32>
    %14 = "tensor.empty"() : () -> tensor<8x2x2x2x2x2x2xi32>
    %15 = "tensor.empty"() : () -> tensor<8x2x2x2x2x2x2xi32>
    %16 = "tensor.empty"() : () -> tensor<8x2x2x2x2x2x2xi32>
    %17 = "tensor.empty"() : () -> tensor<8x2x2x2x2x2x2xi32>
    %18 = "tensor.empty"() : () -> tensor<8x2x2x2x2x2x2xi32>
    %19 = "tensor.empty"() : () -> tensor<8x2x2x2x2x2x2xi32>
    %20 = "tensor.empty"() : () -> tensor<8x2x2x2x2x2x2xi32>
    %21 = "tensor.empty"() : () -> tensor<8x2x2x2x2x2x2xi32>
    %22 = "tensor.empty"() : () -> tensor<8x2x2x2x2x2x2xi32>
    %23 = "tensor.empty"() : () -> tensor<8x2x2x2x2x2x2xi32>
    %24 = "tensor.empty"() : () -> tensor<8x2x2x2x2x2x2xi32>
    %25 = "tensor.empty"() : () -> tensor<8x2x2x2x2x2x2xi32>
    %26 = "tensor.empty"() : () -> tensor<8x2x2x2x2x2x2xi32>
    %27 = "tensor.empty"() : () -> tensor<8x2x2x2x2x2x2xi32>
    %28 = "tensor.empty"() : () -> tensor<8x2x2x2x2x2x2xi32>
    %29 = "tensor.empty"() : () -> tensor<8x2x2x2x2x2x2xi32>
    %30 = "tensor.empty"() : () -> tensor<8x2x2x2x2x2x2xi32>
    %31 = "hivm.hir.vbrc"(%6, %30) <{broadcast_dims = array<i64: 1>}> : (tensor<8x1x2x2x2x2x2xi32>, tensor<8x2x2x2x2x2x2xi32>) -> tensor<8x2x2x2x2x2x2xi32>
    %32 = "hivm.hir.vor"(%4, %31, %29) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<8x2x2x2x2x2x2xi32>, tensor<8x2x2x2x2x2x2xi32>, tensor<8x2x2x2x2x2x2xi32>) -> tensor<8x2x2x2x2x2x2xi32>
    %33 = "hivm.hir.vand"(%4, %31, %28) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<8x2x2x2x2x2x2xi32>, tensor<8x2x2x2x2x2x2xi32>, tensor<8x2x2x2x2x2x2xi32>) -> tensor<8x2x2x2x2x2x2xi32>
    %34 = "hivm.hir.vnot"(%33, %33) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<8x2x2x2x2x2x2xi32>, tensor<8x2x2x2x2x2x2xi32>) -> tensor<8x2x2x2x2x2x2xi32>
    %35 = "hivm.hir.vand"(%34, %32, %27) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<8x2x2x2x2x2x2xi32>, tensor<8x2x2x2x2x2x2xi32>, tensor<8x2x2x2x2x2x2xi32>) -> tensor<8x2x2x2x2x2x2xi32>
    %36 = "tensor.empty"() : () -> tensor<8x2x1x2x2x2x2xi32>
    %37 = "hivm.hir.vreduce"(%35, %36) <{arith = #hivm.reduce_op<xori>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 2>}> : (tensor<8x2x2x2x2x2x2xi32>, tensor<8x2x1x2x2x2x2xi32>) -> tensor<8x2x1x2x2x2x2xi32>
    %38 = "hivm.hir.vbrc"(%37, %26) <{broadcast_dims = array<i64: 2>}> : (tensor<8x2x1x2x2x2x2xi32>, tensor<8x2x2x2x2x2x2xi32>) -> tensor<8x2x2x2x2x2x2xi32>
    %39 = "hivm.hir.vor"(%35, %38, %25) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<8x2x2x2x2x2x2xi32>, tensor<8x2x2x2x2x2x2xi32>, tensor<8x2x2x2x2x2x2xi32>) -> tensor<8x2x2x2x2x2x2xi32>
    %40 = "hivm.hir.vand"(%35, %38, %24) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<8x2x2x2x2x2x2xi32>, tensor<8x2x2x2x2x2x2xi32>, tensor<8x2x2x2x2x2x2xi32>) -> tensor<8x2x2x2x2x2x2xi32>
    %41 = "hivm.hir.vnot"(%40, %40) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<8x2x2x2x2x2x2xi32>, tensor<8x2x2x2x2x2x2xi32>) -> tensor<8x2x2x2x2x2x2xi32>
    %42 = "hivm.hir.vand"(%41, %39, %23) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<8x2x2x2x2x2x2xi32>, tensor<8x2x2x2x2x2x2xi32>, tensor<8x2x2x2x2x2x2xi32>) -> tensor<8x2x2x2x2x2x2xi32>
    %43 = "tensor.empty"() : () -> tensor<8x2x2x1x2x2x2xi32>
    %44 = "hivm.hir.vreduce"(%42, %43) <{arith = #hivm.reduce_op<xori>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 3>}> : (tensor<8x2x2x2x2x2x2xi32>, tensor<8x2x2x1x2x2x2xi32>) -> tensor<8x2x2x1x2x2x2xi32>
    %45 = "hivm.hir.vbrc"(%44, %22) <{broadcast_dims = array<i64: 3>}> : (tensor<8x2x2x1x2x2x2xi32>, tensor<8x2x2x2x2x2x2xi32>) -> tensor<8x2x2x2x2x2x2xi32>
    %46 = "hivm.hir.vor"(%42, %45, %21) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<8x2x2x2x2x2x2xi32>, tensor<8x2x2x2x2x2x2xi32>, tensor<8x2x2x2x2x2x2xi32>) -> tensor<8x2x2x2x2x2x2xi32>
    %47 = "hivm.hir.vand"(%42, %45, %20) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<8x2x2x2x2x2x2xi32>, tensor<8x2x2x2x2x2x2xi32>, tensor<8x2x2x2x2x2x2xi32>) -> tensor<8x2x2x2x2x2x2xi32>
    %48 = "hivm.hir.vnot"(%47, %47) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<8x2x2x2x2x2x2xi32>, tensor<8x2x2x2x2x2x2xi32>) -> tensor<8x2x2x2x2x2x2xi32>
    %49 = "hivm.hir.vand"(%48, %46, %19) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<8x2x2x2x2x2x2xi32>, tensor<8x2x2x2x2x2x2xi32>, tensor<8x2x2x2x2x2x2xi32>) -> tensor<8x2x2x2x2x2x2xi32>
    %50 = "tensor.empty"() : () -> tensor<8x2x2x2x1x2x2xi32>
    %51 = "hivm.hir.vreduce"(%49, %50) <{arith = #hivm.reduce_op<xori>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 4>}> : (tensor<8x2x2x2x2x2x2xi32>, tensor<8x2x2x2x1x2x2xi32>) -> tensor<8x2x2x2x1x2x2xi32>
    %52 = "hivm.hir.vbrc"(%51, %18) <{broadcast_dims = array<i64: 4>}> : (tensor<8x2x2x2x1x2x2xi32>, tensor<8x2x2x2x2x2x2xi32>) -> tensor<8x2x2x2x2x2x2xi32>
    %53 = "hivm.hir.vor"(%49, %52, %17) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<8x2x2x2x2x2x2xi32>, tensor<8x2x2x2x2x2x2xi32>, tensor<8x2x2x2x2x2x2xi32>) -> tensor<8x2x2x2x2x2x2xi32>
    %54 = "hivm.hir.vand"(%49, %52, %16) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<8x2x2x2x2x2x2xi32>, tensor<8x2x2x2x2x2x2xi32>, tensor<8x2x2x2x2x2x2xi32>) -> tensor<8x2x2x2x2x2x2xi32>
    %55 = "hivm.hir.vnot"(%54, %54) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<8x2x2x2x2x2x2xi32>, tensor<8x2x2x2x2x2x2xi32>) -> tensor<8x2x2x2x2x2x2xi32>
    %56 = "hivm.hir.vand"(%55, %53, %15) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<8x2x2x2x2x2x2xi32>, tensor<8x2x2x2x2x2x2xi32>, tensor<8x2x2x2x2x2x2xi32>) -> tensor<8x2x2x2x2x2x2xi32>
    %57 = "tensor.empty"() : () -> tensor<8x2x2x2x2x1x2xi32>
    %58 = "hivm.hir.vreduce"(%56, %57) <{arith = #hivm.reduce_op<xori>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 5>}> : (tensor<8x2x2x2x2x2x2xi32>, tensor<8x2x2x2x2x1x2xi32>) -> tensor<8x2x2x2x2x1x2xi32>
    %59 = "hivm.hir.vbrc"(%58, %14) <{broadcast_dims = array<i64: 5>}> : (tensor<8x2x2x2x2x1x2xi32>, tensor<8x2x2x2x2x2x2xi32>) -> tensor<8x2x2x2x2x2x2xi32>
    %60 = "hivm.hir.vor"(%56, %59, %13) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<8x2x2x2x2x2x2xi32>, tensor<8x2x2x2x2x2x2xi32>, tensor<8x2x2x2x2x2x2xi32>) -> tensor<8x2x2x2x2x2x2xi32>
    %61 = "hivm.hir.vand"(%56, %59, %12) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<8x2x2x2x2x2x2xi32>, tensor<8x2x2x2x2x2x2xi32>, tensor<8x2x2x2x2x2x2xi32>) -> tensor<8x2x2x2x2x2x2xi32>
    %62 = "hivm.hir.vnot"(%61, %61) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<8x2x2x2x2x2x2xi32>, tensor<8x2x2x2x2x2x2xi32>) -> tensor<8x2x2x2x2x2x2xi32>
    %63 = "hivm.hir.vand"(%62, %60, %11) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<8x2x2x2x2x2x2xi32>, tensor<8x2x2x2x2x2x2xi32>, tensor<8x2x2x2x2x2x2xi32>) -> tensor<8x2x2x2x2x2x2xi32>
    %64 = "tensor.empty"() : () -> tensor<8x2x2x2x2x2x1xi32>
    %65 = "hivm.hir.vreduce"(%63, %64) <{arith = #hivm.reduce_op<xori>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 6>}> : (tensor<8x2x2x2x2x2x2xi32>, tensor<8x2x2x2x2x2x1xi32>) -> tensor<8x2x2x2x2x2x1xi32>
    %66 = "hivm.hir.vbrc"(%65, %10) <{broadcast_dims = array<i64: 6>}> : (tensor<8x2x2x2x2x2x1xi32>, tensor<8x2x2x2x2x2x2xi32>) -> tensor<8x2x2x2x2x2x2xi32>
    %67 = "hivm.hir.vor"(%63, %66, %9) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<8x2x2x2x2x2x2xi32>, tensor<8x2x2x2x2x2x2xi32>, tensor<8x2x2x2x2x2x2xi32>) -> tensor<8x2x2x2x2x2x2xi32>
    %68 = "hivm.hir.vand"(%63, %66, %8) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<8x2x2x2x2x2x2xi32>, tensor<8x2x2x2x2x2x2xi32>, tensor<8x2x2x2x2x2x2xi32>) -> tensor<8x2x2x2x2x2x2xi32>
    %69 = "hivm.hir.vnot"(%68, %68) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<8x2x2x2x2x2x2xi32>, tensor<8x2x2x2x2x2x2xi32>) -> tensor<8x2x2x2x2x2x2xi32>
    %70 = "hivm.hir.vand"(%69, %67, %7) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<8x2x2x2x2x2x2xi32>, tensor<8x2x2x2x2x2x2xi32>, tensor<8x2x2x2x2x2x2xi32>) -> tensor<8x2x2x2x2x2x2xi32>
    %71 = "memref.reinterpret_cast"(%arg4) <{operandSegmentSizes = array<i32: 1, 0, 0, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: 8, 2, 2, 2, 2, 2, 2>, static_strides = array<i64: 64, 32, 16, 8, 4, 2, 1>}> : (memref<?xi32>) -> memref<8x2x2x2x2x2x2xi32, strided<[64, 32, 16, 8, 4, 2, 1]>>
    "hivm.hir.store"(%70, %71) : (tensor<8x2x2x2x2x2x2xi32>, memref<8x2x2x2x2x2x2xi32, strided<[64, 32, 16, 8, 4, 2, 1]>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, false, false, false]> : vector<8xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

