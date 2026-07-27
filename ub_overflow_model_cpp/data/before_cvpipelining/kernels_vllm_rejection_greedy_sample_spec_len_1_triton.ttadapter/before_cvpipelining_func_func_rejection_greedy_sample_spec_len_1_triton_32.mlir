#map = affine_map<()[s0] -> (s0 + 16)>
#map1 = affine_map<()[s0, s1] -> (s0, s1)>
#map2 = affine_map<()[s0, s1] -> (s0 - s1)>
#map3 = affine_map<()[s0] -> (s0 * 2)>
#map4 = affine_map<()[s0] -> (s0 + 1)>
"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xi32>, memref<?xi32>, memref<?xi32>, memref<?xi32>, i32, i32, i32, i32) -> (), sym_name = "rejection_greedy_sample_spec_len_1_triton"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xi32>, %arg4: memref<?xi32>, %arg5: memref<?xi32>, %arg6: memref<?xi32>, %arg7: i32, %arg8: i32, %arg9: i32, %arg10: i32):
    %0 = "arith.constant"() <{value = 1 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = 0 : index}> : () -> index
    %2 = "arith.constant"() <{value = 16 : index}> : () -> index
    %3 = "arith.constant"() <{value = 16 : i32}> : () -> i32
    %4 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %5 = "arith.constant"() <{value = 2 : i32}> : () -> i32
    "hivm.hir.set_mask_norm"() : () -> ()
    %6 = "arith.muli"(%arg8, %arg9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %7 = "arith.muli"(%6, %arg10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%7) <{effects = ["write"]}> {logical_block_num} : (i32) -> ()
    %8 = "hivm.hir.get_block_idx"() : () -> i64
    %9 = "arith.trunci"(%8) : (i64) -> i32
    %10 = "arith.muli"(%arg10, %arg9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %11 = "arith.divsi"(%9, %10) : (i32, i32) -> i32
    %12 = "arith.remsi"(%11, %arg8) : (i32, i32) -> i32
    %13 = "arith.muli"(%12, %3) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %14 = "arith.index_cast"(%13) : (i32) -> index
    %15 = "memref.reinterpret_cast"(%arg4, %14) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<16xi32, strided<[1], offset: ?>>
    %16 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16xi32>
    %17 = "affine.apply"(%14) <{map = #map}> : (index) -> index
    %18 = "arith.index_cast"(%arg7) : (i32) -> index
    %19 = "affine.max"(%14, %18) <{map = #map1}> : (index, index) -> index
    %20 = "affine.min"(%17, %19) <{map = #map1}> : (index, index) -> index
    %21 = "affine.apply"(%20, %14) <{map = #map2}> : (index, index) -> index
    %22 = "arith.cmpi"(%21, %2) <{predicate = 2 : i64}> : (index, index) -> i1
    %23 = "memref.subview"(%15, %21) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<16xi32, strided<[1], offset: ?>>, index) -> memref<?xi32, strided<[1], offset: ?>>
    %24 = "memref.subview"(%16, %21) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<16xi32>, index) -> memref<?xi32, strided<[1]>>
    "hivm.hir.load"(%23, %24, %4, %1, %22) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xi32, strided<[1], offset: ?>>, memref<?xi32, strided<[1]>>, i32, index, i1) -> ()
    %25 = "bufferization.to_tensor"(%16) <{restrict, writable}> : (memref<16xi32>) -> tensor<16xi32>
    %26 = "memref.reinterpret_cast"(%arg5, %14) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<16xi32, strided<[1], offset: ?>>
    %27 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16xi32>
    %28 = "memref.subview"(%26, %21) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<16xi32, strided<[1], offset: ?>>, index) -> memref<?xi32, strided<[1], offset: ?>>
    %29 = "memref.subview"(%27, %21) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<16xi32>, index) -> memref<?xi32, strided<[1]>>
    "hivm.hir.load"(%28, %29, %4, %1, %22) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xi32, strided<[1], offset: ?>>, memref<?xi32, strided<[1]>>, i32, index, i1) -> ()
    %30 = "bufferization.to_tensor"(%27) <{restrict, writable}> : (memref<16xi32>) -> tensor<16xi32>
    %31 = "affine.apply"(%14) <{map = #map3}> : (index) -> index
    %32 = "memref.reinterpret_cast"(%arg3, %31) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16>, static_strides = array<i64: 2>}> : (memref<?xi32>, index) -> memref<16xi32, strided<[2], offset: ?>>
    %33 = "tensor.extract_slice"(%30, %21) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<16xi32>, index) -> tensor<?xi32>
    %34 = "memref.subview"(%32, %21) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<16xi32, strided<[2], offset: ?>>, index) -> memref<?xi32, strided<[2], offset: ?>>
    "hivm.hir.store"(%33, %34) : (tensor<?xi32>, memref<?xi32, strided<[2], offset: ?>>) -> ()
    "scf.for"(%4, %3, %0) ({
    ^bb0(%arg11: i32):
      %35 = "arith.index_cast"(%arg11) : (i32) -> index
      %36 = "tensor.extract"(%25, %35) : (tensor<16xi32>, index) -> i32
      %37 = "tensor.extract"(%30, %35) : (tensor<16xi32>, index) -> i32
      %38 = "arith.addi"(%13, %arg11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %39 = "arith.cmpi"(%36, %37) <{predicate = 0 : i64}> : (i32, i32) -> i1
      "scf.if"(%39) ({
        %40 = "arith.index_cast"(%38) : (i32) -> index
        %41 = "memref.reinterpret_cast"(%arg6, %40) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<1xi32, strided<[1], offset: ?>>
        %42 = "memref.load"(%41, %1) : (memref<1xi32, strided<[1], offset: ?>>, index) -> i32
        %43 = "arith.muli"(%38, %5) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
        %44 = "arith.index_cast"(%43) : (i32) -> index
        %45 = "affine.apply"(%44) <{map = #map4}> : (index) -> index
        %46 = "tensor.empty"() : () -> tensor<1xi32>
        %47 = "tensor.insert"(%42, %46, %1) : (i32, tensor<1xi32>, index) -> tensor<1xi32>
        %48 = "memref.reinterpret_cast"(%arg3, %45) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<1xi32, strided<[1], offset: ?>>
        "hivm.hir.store"(%47, %48) : (tensor<1xi32>, memref<1xi32, strided<[1], offset: ?>>) -> ()
        "scf.yield"() : () -> ()
      }, {
      }) : (i1) -> ()
      "scf.yield"() : () -> ()
    }) : (i32, i32, i32) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, true, false, false, false, false]> : vector<11xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hacc.target = #hacc.target<"Ascend910B1">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

