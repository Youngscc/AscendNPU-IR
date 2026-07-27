#map = affine_map<()[s0] -> (s0 - 1)>
#map1 = affine_map<()[s0] -> (s0 + 4)>
#map2 = affine_map<()[s0, s1] -> (s0, s1)>
#map3 = affine_map<()[s0, s1] -> (s0 - s1)>
#map4 = affine_map<()[s0] -> (-s0)>
#map5 = affine_map<()[s0] -> (-s0 + 1)>
#map6 = affine_map<()[s0] -> (s0, 0)>
#map7 = affine_map<()[s0] -> (s0, 16)>
"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {}, {}, {}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xi32>, memref<?xi32>, memref<?xi32>, i32, i32, i32, i32, i32, i32) -> (), sym_name = "expand_kernel"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xi32>, %arg4: memref<?xi32>, %arg5: memref<?xi32>, %arg6: i32, %arg7: i32, %arg8: i32, %arg9: i32, %arg10: i32, %arg11: i32):
    %0 = "arith.constant"() <{value = 1 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = 4 : index}> : () -> index
    %2 = "arith.constant"() <{value = 0 : index}> : () -> index
    %3 = "arith.constant"() <{value = 4 : i32}> : () -> i32
    %4 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    "hivm.hir.set_mask_norm"() : () -> ()
    %5 = "arith.muli"(%arg9, %arg10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %6 = "arith.muli"(%5, %arg11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%6) <{effects = ["write"]}> {logical_block_num} : (i32) -> ()
    %7 = "hivm.hir.get_block_idx"() : () -> i64
    %8 = "arith.trunci"(%7) : (i64) -> i32
    %9 = "arith.muli"(%arg11, %arg10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %10 = "arith.divsi"(%8, %9) : (i32, i32) -> i32
    %11 = "arith.remsi"(%10, %arg9) : (i32, i32) -> i32
    %12 = "tensor.empty"() : () -> tensor<4xi32>
    %13 = "hivm.hir.vbrc"(%4, %12) <{broadcast_dims = array<i64>}> : (i32, tensor<4xi32>) -> tensor<4xi32>
    %14 = "arith.muli"(%11, %3) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %15 = "arith.index_cast"(%14) : (i32) -> index
    %16 = "memref.reinterpret_cast"(%arg5, %15) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 4>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<4xi32, strided<[1], offset: ?>>
    %17 = "affine.apply"(%15) <{map = #map}> : (index) -> index
    %18 = "memref.reinterpret_cast"(%arg5, %17) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 4>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<4xi32, strided<[1], offset: ?>>
    %19 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<4xi32>
    %20 = "affine.apply"(%15) <{map = #map1}> : (index) -> index
    %21 = "arith.index_cast"(%arg8) : (i32) -> index
    %22 = "affine.max"(%15, %21) <{map = #map2}> : (index, index) -> index
    %23 = "affine.min"(%20, %22) <{map = #map2}> : (index, index) -> index
    %24 = "affine.apply"(%23, %15) <{map = #map3}> : (index, index) -> index
    %25 = "arith.cmpi"(%24, %1) <{predicate = 2 : i64}> : (index, index) -> i1
    %26 = "memref.subview"(%18, %24) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<4xi32, strided<[1], offset: ?>>, index) -> memref<?xi32, strided<[1], offset: ?>>
    %27 = "memref.subview"(%19, %24) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<4xi32>, index) -> memref<?xi32, strided<[1]>>
    "hivm.hir.load"(%26, %27, %4, %2, %25) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xi32, strided<[1], offset: ?>>, memref<?xi32, strided<[1]>>, i32, index, i1) -> ()
    %28 = "bufferization.to_tensor"(%19) <{restrict, writable}> : (memref<4xi32>) -> tensor<4xi32>
    %29 = "affine.apply"(%15) <{map = #map4}> : (index) -> index
    %30 = "affine.apply"(%15) <{map = #map5}> : (index) -> index
    %31 = "arith.cmpi"(%29, %2) <{predicate = 2 : i64}> : (index, index) -> i1
    %32 = "arith.cmpi"(%29, %1) <{predicate = 5 : i64}> : (index, index) -> i1
    %33 = "arith.ori"(%31, %32) : (i1, i1) -> i1
    %34 = "scf.if"(%33) ({
      "scf.yield"(%28) : (tensor<4xi32>) -> ()
    }, {
      %63 = "tensor.extract_slice"(%28, %29) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<4xi32>, index) -> tensor<?xi32>
      %64 = "tensor.insert_slice"(%63, %13, %29) <{operandSegmentSizes = array<i32: 1, 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<?xi32>, tensor<4xi32>, index) -> tensor<4xi32>
      %65 = "tensor.extract_slice"(%64, %30) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<4xi32>, index) -> tensor<?xi32>
      %66 = "tensor.insert_slice"(%65, %28, %30) <{operandSegmentSizes = array<i32: 1, 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<?xi32>, tensor<4xi32>, index) -> tensor<4xi32>
      "scf.yield"(%66) : (tensor<4xi32>) -> ()
    }) : (i1) -> tensor<4xi32>
    %35 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<4xi32>
    %36 = "memref.subview"(%16, %24) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<4xi32, strided<[1], offset: ?>>, index) -> memref<?xi32, strided<[1], offset: ?>>
    %37 = "memref.subview"(%35, %24) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<4xi32>, index) -> memref<?xi32, strided<[1]>>
    "hivm.hir.load"(%36, %37, %4, %2, %25) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xi32, strided<[1], offset: ?>>, memref<?xi32, strided<[1]>>, i32, index, i1) -> ()
    %38 = "bufferization.to_tensor"(%35) <{restrict, writable}> : (memref<4xi32>) -> tensor<4xi32>
    %39 = "memref.reinterpret_cast"(%arg4, %15) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 4>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<4xi32, strided<[1], offset: ?>>
    %40 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<4xi32>
    %41 = "memref.subview"(%39, %24) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<4xi32, strided<[1], offset: ?>>, index) -> memref<?xi32, strided<[1], offset: ?>>
    %42 = "memref.subview"(%40, %24) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<4xi32>, index) -> memref<?xi32, strided<[1]>>
    "hivm.hir.load"(%41, %42, %4, %2, %25) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xi32, strided<[1], offset: ?>>, memref<?xi32, strided<[1]>>, i32, index, i1) -> ()
    %43 = "bufferization.to_tensor"(%40) <{restrict, writable}> : (memref<4xi32>) -> tensor<4xi32>
    %44 = "hivm.hir.vbrc"(%arg6, %12) <{broadcast_dims = array<i64>}> : (i32, tensor<4xi32>) -> tensor<4xi32>
    %45 = "tensor.empty"() : () -> tensor<4xi1>
    %46 = "hivm.hir.vcmp"(%43, %44, %45) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<4xi32>, tensor<4xi32>, tensor<4xi1>) -> tensor<4xi1>
    %47 = "hivm.hir.vsel"(%46, %arg7, %43, %12) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<4xi1>, i32, tensor<4xi32>, tensor<4xi32>) -> tensor<4xi32>
    %48 = "tensor.empty"() : () -> tensor<16xi32>
    "scf.for"(%4, %3, %0) ({
    ^bb0(%arg12: i32):
      %49 = "arith.index_cast"(%arg12) : (i32) -> index
      %50 = "tensor.extract"(%38, %49) {DiscreteMemAccess} : (tensor<4xi32>, index) -> i32
      %51 = "tensor.extract"(%34, %49) {DiscreteMemAccess} : (tensor<4xi32>, index) -> i32
      %52 = "arith.subi"(%50, %51) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %53 = "tensor.extract"(%34, %49) : (tensor<4xi32>, index) -> i32
      %54 = "tensor.extract"(%47, %49) : (tensor<4xi32>, index) -> i32
      %55 = "arith.index_cast"(%53) : (i32) -> index
      %56 = "memref.reinterpret_cast"(%arg3, %55) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<16xi32, strided<[1], offset: ?>>
      %57 = "hivm.hir.vbrc"(%54, %48) <{broadcast_dims = array<i64>}> : (i32, tensor<16xi32>) -> tensor<16xi32>
      %58 = "arith.index_cast"(%52) : (i32) -> index
      %59 = "affine.max"(%58) <{map = #map6}> : (index) -> index
      %60 = "affine.min"(%59) <{map = #map7}> : (index) -> index
      %61 = "tensor.extract_slice"(%57, %60) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<16xi32>, index) -> tensor<?xi32>
      %62 = "memref.subview"(%56, %60) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<16xi32, strided<[1], offset: ?>>, index) -> memref<?xi32, strided<[1], offset: ?>>
      "hivm.hir.store"(%61, %62) : (tensor<?xi32>, memref<?xi32, strided<[1], offset: ?>>) -> ()
      "scf.yield"() : () -> ()
    }) : (i32, i32, i32) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, false, false, false, false, false, false]> : vector<12xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hacc.target = #hacc.target<"Ascend910B1">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

