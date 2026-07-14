"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xf32>, memref<?xf32>, i32, i32, i32) -> (), sym_name = "erf_kernel"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xf32>, %arg4: memref<?xf32>, %arg5: i32, %arg6: i32, %arg7: i32):
    %0 = "arith.constant"() <{value = 26267.2246 : f32}> : () -> f32
    %1 = "arith.constant"() <{value = 13243.3662 : f32}> : () -> f32
    %2 = "arith.constant"() <{value = 3023.12476 : f32}> : () -> f32
    %3 = "arith.constant"() <{value = 398.569641 : f32}> : () -> f32
    %4 = "arith.constant"() <{value = 31.2128582 : f32}> : () -> f32
    %5 = "arith.constant"() <{value = 29639.3848 : f32}> : () -> f32
    %6 = "arith.constant"() <{value = 5063.7915 : f32}> : () -> f32
    %7 = "arith.constant"() <{value = 1393.80615 : f32}> : () -> f32
    %8 = "arith.constant"() <{value = 101.62809 : f32}> : () -> f32
    %9 = "arith.constant"() <{value = 7.55170154 : f32}> : () -> f32
    %10 = "arith.constant"() <{value = 0.0534437485 : f32}> : () -> f32
    %11 = "arith.constant"() <{value = -3.920000e+00 : f32}> : () -> f32
    %12 = "arith.constant"() <{value = 3.920000e+00 : f32}> : () -> f32
    %13 = "arith.constant"() <{value = 32 : index}> : () -> index
    %14 = "arith.constant"() <{value = 8 : index}> : () -> index
    %15 = "arith.constant"() <{value = 2 : i32}> : () -> i32
    %16 = "arith.constant"() <{value = 4 : i32}> : () -> i32
    %17 = "arith.constant"() <{value = 8 : i32}> : () -> i32
    "hivm.hir.set_mask_norm"() : () -> ()
    %18 = "arith.muli"(%arg5, %arg6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %19 = "arith.muli"(%18, %arg7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%19) {logical_block_num} : (i32) -> ()
    %20 = "hivm.hir.get_block_idx"() : () -> i64
    %21 = "arith.trunci"(%20) : (i64) -> i32
    %22 = "arith.remsi"(%21, %arg7) : (i32, i32) -> i32
    %23 = "arith.divsi"(%21, %arg7) : (i32, i32) -> i32
    %24 = "arith.remsi"(%23, %arg6) : (i32, i32) -> i32
    %25 = "arith.muli"(%arg7, %arg6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %26 = "arith.divsi"(%21, %25) : (i32, i32) -> i32
    %27 = "arith.remsi"(%26, %arg5) : (i32, i32) -> i32
    %28 = "arith.muli"(%27, %15) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %29 = "arith.muli"(%24, %16) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %30 = "arith.muli"(%22, %17) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %31 = "arith.index_cast"(%28) : (i32) -> index
    %32 = "arith.muli"(%31, %13) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %33 = "arith.index_cast"(%29) : (i32) -> index
    %34 = "arith.muli"(%33, %14) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %35 = "arith.index_cast"(%30) : (i32) -> index
    %36 = "arith.addi"(%32, %34) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %37 = "arith.addi"(%36, %35) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %38 = "memref.reinterpret_cast"(%arg4, %37) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 2, 4, 8>, static_strides = array<i64: 32, 8, 1>}> : (memref<?xf32>, index) -> memref<2x4x8xf32, strided<[32, 8, 1], offset: ?>>
    %39 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<2x4x8xf32>
    "hivm.hir.load"(%38, %39) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<2x4x8xf32, strided<[32, 8, 1], offset: ?>>, memref<2x4x8xf32>) -> ()
    %40 = "bufferization.to_tensor"(%39) <{restrict, writable}> : (memref<2x4x8xf32>) -> tensor<2x4x8xf32>
    %41 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %42 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %43 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %44 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %45 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %46 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %47 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %48 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %49 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %50 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %51 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %52 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %53 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %54 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %55 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %56 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %57 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %58 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %59 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %60 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %61 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %62 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %63 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %64 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %65 = "hivm.hir.vmin"(%40, %12, %64) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, f32, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %66 = "hivm.hir.vmax"(%65, %11, %63) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, f32, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %67 = "hivm.hir.vmul"(%66, %66, %62) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, tensor<2x4x8xf32>, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %68 = "hivm.hir.vmul"(%67, %10, %61) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, f32, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %69 = "hivm.hir.vadd"(%68, %9, %60) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, f32, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %70 = "hivm.hir.vmul"(%69, %67, %59) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, tensor<2x4x8xf32>, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %71 = "hivm.hir.vadd"(%70, %8, %58) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, f32, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %72 = "hivm.hir.vmul"(%71, %67, %57) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, tensor<2x4x8xf32>, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %73 = "hivm.hir.vadd"(%72, %7, %56) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, f32, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %74 = "hivm.hir.vmul"(%73, %67, %55) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, tensor<2x4x8xf32>, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %75 = "hivm.hir.vadd"(%74, %6, %54) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, f32, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %76 = "hivm.hir.vmul"(%75, %67, %53) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, tensor<2x4x8xf32>, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %77 = "hivm.hir.vadd"(%76, %5, %52) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, f32, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %78 = "hivm.hir.vmul"(%66, %77, %51) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, tensor<2x4x8xf32>, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %79 = "hivm.hir.vadd"(%67, %4, %50) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, f32, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %80 = "hivm.hir.vmul"(%79, %67, %49) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, tensor<2x4x8xf32>, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %81 = "hivm.hir.vadd"(%80, %3, %48) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, f32, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %82 = "hivm.hir.vmul"(%81, %67, %47) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, tensor<2x4x8xf32>, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %83 = "hivm.hir.vadd"(%82, %2, %46) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, f32, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %84 = "hivm.hir.vmul"(%83, %67, %45) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, tensor<2x4x8xf32>, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %85 = "hivm.hir.vadd"(%84, %1, %44) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, f32, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %86 = "hivm.hir.vmul"(%85, %67, %43) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, tensor<2x4x8xf32>, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %87 = "hivm.hir.vadd"(%86, %0, %42) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, f32, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %88 = "hivm.hir.vdiv"(%78, %87, %41) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, tensor<2x4x8xf32>, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %89 = "memref.reinterpret_cast"(%arg3, %37) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 2, 4, 8>, static_strides = array<i64: 32, 8, 1>}> : (memref<?xf32>, index) -> memref<2x4x8xf32, strided<[32, 8, 1], offset: ?>>
    "hivm.hir.store"(%88, %89) : (tensor<2x4x8xf32>, memref<2x4x8xf32, strided<[32, 8, 1], offset: ?>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, false, false, false]> : vector<8xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

