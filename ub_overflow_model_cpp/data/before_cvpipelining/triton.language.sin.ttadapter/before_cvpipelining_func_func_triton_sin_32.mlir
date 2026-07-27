#map = affine_map<()[s0] -> (s0 + 1024)>
#map1 = affine_map<()[s0, s1] -> (s0, s1)>
#map2 = affine_map<()[s0, s1] -> (s0 - s1)>
"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xf32>, memref<?xf32>, i32, i32, i32, i32) -> (), sym_name = "triton_sin"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xf32>, %arg4: memref<?xf32>, %arg5: i32, %arg6: i32, %arg7: i32, %arg8: i32):
    %0 = "arith.constant"() <{value = -0.166666582 : f32}> : () -> f32
    %1 = "arith.constant"() <{value = 8.333050e-03 : f32}> : () -> f32
    %2 = "arith.constant"() <{value = -1.98089445E-4 : f32}> : () -> f32
    %3 = "arith.constant"() <{value = 2.60492652E-6 : f32}> : () -> f32
    %4 = "arith.constant"() <{value = 1.000000e+00 : f32}> : () -> f32
    %5 = "arith.constant"() <{value = -2.000000e+00 : f32}> : () -> f32
    %6 = "arith.constant"() <{value = 4.000000e+00 : f32}> : () -> f32
    %7 = "arith.constant"() <{value = 5.000000e-01 : f32}> : () -> f32
    %8 = "arith.constant"() <{value = 1.24467439E-13 : f32}> : () -> f32
    %9 = "arith.constant"() <{value = -1.74122761E-9 : f32}> : () -> f32
    %10 = "arith.constant"() <{value = -8.9071691E-6 : f32}> : () -> f32
    %11 = "arith.constant"() <{value = 3.14160156 : f32}> : () -> f32
    %12 = "arith.constant"() <{value = 2.048000e+03 : f32}> : () -> f32
    %13 = "arith.constant"() <{value = 4.8828125E-4 : f32}> : () -> f32
    %14 = "arith.constant"() <{value = 0.318309873 : f32}> : () -> f32
    %15 = "arith.constant"() <{value = 0.000000e+00 : f32}> : () -> f32
    %16 = "arith.constant"() <{value = 1024 : index}> : () -> index
    %17 = "arith.constant"() <{value = 32768 : i32}> : () -> i32
    %18 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %19 = "arith.constant"() <{value = 1024 : i32}> : () -> i32
    %20 = "arith.constant"() <{value = 0 : index}> : () -> index
    "hivm.hir.set_mask_norm"() : () -> ()
    %21 = "arith.muli"(%arg6, %arg7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %22 = "arith.muli"(%21, %arg8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%22) <{effects = ["write"]}> {logical_block_num} : (i32) -> ()
    %23 = "hivm.hir.get_block_idx"() : () -> i64
    %24 = "arith.trunci"(%23) : (i64) -> i32
    %25 = "arith.muli"(%arg8, %arg7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %26 = "arith.divsi"(%24, %25) : (i32, i32) -> i32
    %27 = "arith.remsi"(%26, %arg6) : (i32, i32) -> i32
    %28 = "arith.muli"(%27, %17) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "scf.for"(%18, %17, %19) ({
    ^bb0(%arg9: i32):
      %29 = "arith.addi"(%28, %arg9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %30 = "arith.index_cast"(%29) : (i32) -> index
      %31 = "memref.reinterpret_cast"(%arg3, %30) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1024>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<1024xf32, strided<[1], offset: ?>>
      %32 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<1024xf32>
      %33 = "affine.apply"(%30) <{map = #map}> : (index) -> index
      %34 = "arith.index_cast"(%arg5) : (i32) -> index
      %35 = "affine.max"(%30, %34) <{map = #map1}> : (index, index) -> index
      %36 = "affine.min"(%33, %35) <{map = #map1}> : (index, index) -> index
      %37 = "affine.apply"(%36, %30) <{map = #map2}> : (index, index) -> index
      %38 = "arith.cmpi"(%37, %16) <{predicate = 2 : i64}> : (index, index) -> i1
      %39 = "memref.subview"(%31, %37) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<1024xf32, strided<[1], offset: ?>>, index) -> memref<?xf32, strided<[1], offset: ?>>
      %40 = "memref.subview"(%32, %37) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<1024xf32>, index) -> memref<?xf32, strided<[1]>>
      "hivm.hir.load"(%39, %40, %15, %20, %38) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xf32, strided<[1], offset: ?>>, memref<?xf32, strided<[1]>>, f32, index, i1) -> ()
      %41 = "bufferization.to_tensor"(%32) <{restrict, writable}> : (memref<1024xf32>) -> tensor<1024xf32>
      %42 = "tensor.empty"() : () -> tensor<1024xf32>
      %43 = "hivm.hir.vmul"(%41, %14, %42) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, f32, tensor<1024xf32>) -> tensor<1024xf32>
      %44 = "hivm.hir.vmul"(%43, %13, %42) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, f32, tensor<1024xf32>) -> tensor<1024xf32>
      %45 = "hivm.hir.vcast"(%43, %42) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<round>, transpose = array<i64>}> : (tensor<1024xf32>, tensor<1024xf32>) -> tensor<1024xf32>
      %46 = "hivm.hir.vcast"(%44, %42) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<round>, transpose = array<i64>}> : (tensor<1024xf32>, tensor<1024xf32>) -> tensor<1024xf32>
      %47 = "hivm.hir.vmul"(%46, %12, %42) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, f32, tensor<1024xf32>) -> tensor<1024xf32>
      %48 = "hivm.hir.vsub"(%45, %47, %42) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, tensor<1024xf32>, tensor<1024xf32>) -> tensor<1024xf32>
      %49 = "hivm.hir.vmul"(%47, %11, %42) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, f32, tensor<1024xf32>) -> tensor<1024xf32>
      %50 = "hivm.hir.vsub"(%41, %49, %42) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, tensor<1024xf32>, tensor<1024xf32>) -> tensor<1024xf32>
      %51 = "hivm.hir.vmul"(%48, %11, %42) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, f32, tensor<1024xf32>) -> tensor<1024xf32>
      %52 = "hivm.hir.vsub"(%50, %51, %42) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, tensor<1024xf32>, tensor<1024xf32>) -> tensor<1024xf32>
      %53 = "hivm.hir.vmul"(%47, %10, %42) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, f32, tensor<1024xf32>) -> tensor<1024xf32>
      %54 = "hivm.hir.vsub"(%52, %53, %42) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, tensor<1024xf32>, tensor<1024xf32>) -> tensor<1024xf32>
      %55 = "hivm.hir.vmul"(%48, %10, %42) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, f32, tensor<1024xf32>) -> tensor<1024xf32>
      %56 = "hivm.hir.vsub"(%54, %55, %42) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, tensor<1024xf32>, tensor<1024xf32>) -> tensor<1024xf32>
      %57 = "hivm.hir.vmul"(%47, %9, %42) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, f32, tensor<1024xf32>) -> tensor<1024xf32>
      %58 = "hivm.hir.vsub"(%56, %57, %42) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, tensor<1024xf32>, tensor<1024xf32>) -> tensor<1024xf32>
      %59 = "hivm.hir.vmul"(%48, %9, %42) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, f32, tensor<1024xf32>) -> tensor<1024xf32>
      %60 = "hivm.hir.vsub"(%58, %59, %42) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, tensor<1024xf32>, tensor<1024xf32>) -> tensor<1024xf32>
      %61 = "hivm.hir.vmul"(%47, %8, %42) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, f32, tensor<1024xf32>) -> tensor<1024xf32>
      %62 = "hivm.hir.vsub"(%60, %61, %42) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, tensor<1024xf32>, tensor<1024xf32>) -> tensor<1024xf32>
      %63 = "hivm.hir.vmul"(%48, %8, %42) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, f32, tensor<1024xf32>) -> tensor<1024xf32>
      %64 = "hivm.hir.vsub"(%62, %63, %42) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, tensor<1024xf32>, tensor<1024xf32>) -> tensor<1024xf32>
      %65 = "hivm.hir.vmul"(%45, %7, %42) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, f32, tensor<1024xf32>) -> tensor<1024xf32>
      %66 = "hivm.hir.vcast"(%65, %42) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<floor>, transpose = array<i64>}> : (tensor<1024xf32>, tensor<1024xf32>) -> tensor<1024xf32>
      %67 = "hivm.hir.vmul"(%66, %6, %42) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, f32, tensor<1024xf32>) -> tensor<1024xf32>
      %68 = "hivm.hir.vmul"(%45, %5, %42) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, f32, tensor<1024xf32>) -> tensor<1024xf32>
      %69 = "hivm.hir.vadd"(%67, %68, %42) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, tensor<1024xf32>, tensor<1024xf32>) -> tensor<1024xf32>
      %70 = "hivm.hir.vadd"(%69, %4, %42) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, f32, tensor<1024xf32>) -> tensor<1024xf32>
      %71 = "hivm.hir.vmul"(%64, %64, %42) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, tensor<1024xf32>, tensor<1024xf32>) -> tensor<1024xf32>
      %72 = "hivm.hir.vmul"(%71, %3, %42) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, f32, tensor<1024xf32>) -> tensor<1024xf32>
      %73 = "hivm.hir.vadd"(%72, %2, %42) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, f32, tensor<1024xf32>) -> tensor<1024xf32>
      %74 = "hivm.hir.vmul"(%73, %71, %42) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, tensor<1024xf32>, tensor<1024xf32>) -> tensor<1024xf32>
      %75 = "hivm.hir.vadd"(%74, %1, %42) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, f32, tensor<1024xf32>) -> tensor<1024xf32>
      %76 = "hivm.hir.vmul"(%75, %71, %42) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, tensor<1024xf32>, tensor<1024xf32>) -> tensor<1024xf32>
      %77 = "hivm.hir.vadd"(%76, %0, %42) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, f32, tensor<1024xf32>) -> tensor<1024xf32>
      %78 = "hivm.hir.vmul"(%77, %71, %42) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, tensor<1024xf32>, tensor<1024xf32>) -> tensor<1024xf32>
      %79 = "hivm.hir.vadd"(%78, %4, %42) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, f32, tensor<1024xf32>) -> tensor<1024xf32>
      %80 = "hivm.hir.vmul"(%79, %64, %42) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, tensor<1024xf32>, tensor<1024xf32>) -> tensor<1024xf32>
      %81 = "hivm.hir.vmul"(%80, %70, %42) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, tensor<1024xf32>, tensor<1024xf32>) -> tensor<1024xf32>
      %82 = "memref.reinterpret_cast"(%arg4, %30) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1024>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<1024xf32, strided<[1], offset: ?>>
      %83 = "tensor.extract_slice"(%81, %37) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<1024xf32>, index) -> tensor<?xf32>
      %84 = "memref.subview"(%82, %37) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<1024xf32, strided<[1], offset: ?>>, index) -> memref<?xf32, strided<[1], offset: ?>>
      "hivm.hir.store"(%83, %84) : (tensor<?xf32>, memref<?xf32, strided<[1], offset: ?>>) -> ()
      "scf.yield"() : () -> ()
    }) : (i32, i32, i32) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, false, false, false, false]> : vector<9xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hacc.target = #hacc.target<"Ascend910B1">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

