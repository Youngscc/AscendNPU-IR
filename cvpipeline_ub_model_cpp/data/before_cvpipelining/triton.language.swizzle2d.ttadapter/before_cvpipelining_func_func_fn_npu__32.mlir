"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xi32>, memref<?xi32>, i32, i32, i32) -> (), sym_name = "fn_npu_"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xi32>, %arg4: memref<?xi32>, %arg5: i32, %arg6: i32, %arg7: i32):
    %0 = "arith.constant"() <{value = -1 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %2 = "arith.constant"() <{value = 8.000000e+00 : f32}> : () -> f32
    %3 = "arith.constant"() <{value = 0 : index}> : () -> index
    %4 = "arith.constant"() <{value = 1 : index}> : () -> index
    %5 = "arith.constant"() <{value = 4 : i32}> : () -> i32
    %6 = "arith.constant"() <{value = 8 : i32}> : () -> i32
    %7 = "arith.constant"() <{value = 2 : i32}> : () -> i32
    "hivm.hir.set_mask_norm"() : () -> ()
    %8 = "arith.muli"(%arg5, %arg6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %9 = "arith.muli"(%8, %arg7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%9) {logical_block_num} : (i32) -> ()
    %10 = "tensor.empty"() : () -> tensor<4xi32>
    %11 = "tensor.empty"() : () -> tensor<4xi32>
    %12 = "tensor.empty"() : () -> tensor<4xi32>
    %13 = "tensor.empty"() : () -> tensor<4xi32>
    %14 = "tensor.empty"() : () -> tensor<4xi32>
    %15 = "tensor.empty"() : () -> tensor<4xi32>
    %16 = "tensor.empty"() : () -> tensor<4xi32>
    %17 = "tensor.empty"() : () -> tensor<4xi32>
    %18 = "tensor.empty"() : () -> tensor<4xi32>
    %19 = "tensor.empty"() : () -> tensor<4xi32>
    %20 = "tensor.empty"() : () -> tensor<4xi32>
    %21 = "tensor.empty"() : () -> tensor<4xi32>
    %22 = "tensor.empty"() : () -> tensor<4xi32>
    %23 = "tensor.empty"() : () -> tensor<4xi32>
    %24 = "tensor.empty"() : () -> tensor<4xi32>
    %25 = "hivm.hir.vbrc"(%6, %24) <{broadcast_dims = array<i64>}> : (i32, tensor<4xi32>) -> tensor<4xi32>
    %26 = "hivm.hir.varange"(%23, %3, %4) <{operandSegmentSizes = array<i32: 1, 1, 1>}> : (tensor<4xi32>, index, index) -> tensor<4xi32>
    %27 = "hivm.hir.vmul"(%26, %5, %22) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<4xi32>, i32, tensor<4xi32>) -> tensor<4xi32>
    %28 = "hivm.hir.vadd"(%27, %26, %21) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<4xi32>, tensor<4xi32>, tensor<4xi32>) -> tensor<4xi32>
    %29 = "tensor.empty"() : () -> tensor<4xf32>
    %30 = "tensor.empty"() : () -> tensor<4xf32>
    %31 = "tensor.empty"() : () -> tensor<4xf32>
    %32 = "tensor.empty"() : () -> tensor<4xf32>
    %33 = "tensor.empty"() : () -> tensor<4xf32>
    %34 = "hivm.hir.vcast"(%28, %33) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<4xi32>, tensor<4xf32>) -> tensor<4xf32>
    %35 = "hivm.hir.vdiv"(%34, %2, %32) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<4xf32>, f32, tensor<4xf32>) -> tensor<4xf32>
    %36 = "hivm.hir.vcast"(%35, %20) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<trunc>, transpose = array<i64>}> : (tensor<4xf32>, tensor<4xi32>) -> tensor<4xi32>
    %37 = "hivm.hir.vmul"(%36, %7, %19) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<4xi32>, i32, tensor<4xi32>) -> tensor<4xi32>
    %38 = "hivm.hir.vmul"(%37, %0, %18) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<4xi32>, i32, tensor<4xi32>) -> tensor<4xi32>
    %39 = "hivm.hir.vadd"(%38, %5, %17) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<4xi32>, i32, tensor<4xi32>) -> tensor<4xi32>
    %40 = "hivm.hir.vmin"(%39, %7, %16) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<4xi32>, i32, tensor<4xi32>) -> tensor<4xi32>
    %41 = "hivm.hir.vmod"(%28, %6, %15) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<4xi32>, i32, tensor<4xi32>) -> tensor<4xi32>
    %42 = "tensor.empty"() : () -> tensor<4xi1>
    %43 = "tensor.empty"() : () -> tensor<4xi1>
    %44 = "hivm.hir.vcmp"(%25, %1, %43) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<4xi32>, i32, tensor<4xi1>) -> tensor<4xi1>
    %45 = "hivm.hir.vsel"(%44, %0, %41, %14) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<4xi1>, i32, tensor<4xi32>, tensor<4xi32>) -> tensor<4xi32>
    %46 = "hivm.hir.vmod"(%45, %40, %13) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<4xi32>, tensor<4xi32>, tensor<4xi32>) -> tensor<4xi32>
    %47 = "hivm.hir.vcmp"(%40, %1, %42) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<4xi32>, i32, tensor<4xi1>) -> tensor<4xi1>
    %48 = "hivm.hir.vsel"(%47, %0, %46, %12) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<4xi1>, i32, tensor<4xi32>, tensor<4xi32>) -> tensor<4xi32>
    %49 = "hivm.hir.vadd"(%37, %48, %11) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<4xi32>, tensor<4xi32>, tensor<4xi32>) -> tensor<4xi32>
    %50 = "hivm.hir.vcast"(%45, %31) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<4xi32>, tensor<4xf32>) -> tensor<4xf32>
    %51 = "hivm.hir.vcast"(%40, %30) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<4xi32>, tensor<4xf32>) -> tensor<4xf32>
    %52 = "hivm.hir.vdiv"(%50, %51, %29) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<4xf32>, tensor<4xf32>, tensor<4xf32>) -> tensor<4xf32>
    %53 = "hivm.hir.vcast"(%52, %10) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<trunc>, transpose = array<i64>}> : (tensor<4xf32>, tensor<4xi32>) -> tensor<4xi32>
    %54 = "memref.reinterpret_cast"(%arg3) <{operandSegmentSizes = array<i32: 1, 0, 0, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: 4>, static_strides = array<i64: 1>}> : (memref<?xi32>) -> memref<4xi32, strided<[1]>>
    "hivm.hir.store"(%49, %54) : (tensor<4xi32>, memref<4xi32, strided<[1]>>) -> ()
    %55 = "memref.reinterpret_cast"(%arg4) <{operandSegmentSizes = array<i32: 1, 0, 0, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: 4>, static_strides = array<i64: 1>}> : (memref<?xi32>) -> memref<4xi32, strided<[1]>>
    "hivm.hir.store"(%53, %55) : (tensor<4xi32>, memref<4xi32, strided<[1]>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, false, false, false]> : vector<8xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

