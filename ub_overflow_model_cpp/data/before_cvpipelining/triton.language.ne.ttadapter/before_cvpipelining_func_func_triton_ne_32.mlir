#map = affine_map<()[s0] -> (s0 + 32)>
#map1 = affine_map<()[s0] -> (s0, 128)>
#map2 = affine_map<()[s0, s1] -> (s0, s1)>
#map3 = affine_map<()[s0, s1] -> (s0 - s1)>
"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xi32>, memref<?xi32>, memref<?xi8>, i32, i32, i32) -> (), sym_name = "triton_ne"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xi32>, %arg4: memref<?xi32>, %arg5: memref<?xi8>, %arg6: i32, %arg7: i32, %arg8: i32):
    %0 = "arith.constant"() <{value = 1 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = 32 : index}> : () -> index
    %2 = "arith.constant"() <{value = 128 : i32}> : () -> i32
    %3 = "arith.constant"() <{value = 32 : i32}> : () -> i32
    %4 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %5 = "arith.constant"() <{value = 4 : i32}> : () -> i32
    %6 = "arith.constant"() <{value = 0 : index}> : () -> index
    "hivm.hir.set_mask_norm"() : () -> ()
    %7 = "arith.muli"(%arg6, %arg7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %8 = "arith.muli"(%7, %arg8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%8) <{effects = ["write"]}> {logical_block_num} : (i32) -> ()
    %9 = "hivm.hir.get_block_idx"() : () -> i64
    %10 = "arith.trunci"(%9) : (i64) -> i32
    %11 = "arith.muli"(%arg8, %arg7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %12 = "arith.divsi"(%10, %11) : (i32, i32) -> i32
    %13 = "arith.remsi"(%12, %arg6) : (i32, i32) -> i32
    %14 = "arith.muli"(%13, %2) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "scf.for"(%4, %5, %0) ({
    ^bb0(%arg9: i32):
      %15 = "arith.muli"(%arg9, %3) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %16 = "arith.addi"(%14, %15) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %17 = "arith.index_cast"(%16) : (i32) -> index
      %18 = "memref.reinterpret_cast"(%arg3, %17) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 32>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<32xi32, strided<[1], offset: ?>>
      %19 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<32xi32>
      %20 = "affine.apply"(%17) <{map = #map}> : (index) -> index
      %21 = "affine.max"(%17) <{map = #map1}> : (index) -> index
      %22 = "affine.min"(%20, %21) <{map = #map2}> : (index, index) -> index
      %23 = "affine.apply"(%22, %17) <{map = #map3}> : (index, index) -> index
      %24 = "arith.cmpi"(%23, %1) <{predicate = 2 : i64}> : (index, index) -> i1
      %25 = "memref.subview"(%18, %23) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<32xi32, strided<[1], offset: ?>>, index) -> memref<?xi32, strided<[1], offset: ?>>
      %26 = "memref.subview"(%19, %23) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<32xi32>, index) -> memref<?xi32, strided<[1]>>
      "hivm.hir.load"(%25, %26, %4, %6, %24) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xi32, strided<[1], offset: ?>>, memref<?xi32, strided<[1]>>, i32, index, i1) -> ()
      %27 = "bufferization.to_tensor"(%19) <{restrict, writable}> : (memref<32xi32>) -> tensor<32xi32>
      %28 = "memref.reinterpret_cast"(%arg4, %17) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 32>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<32xi32, strided<[1], offset: ?>>
      %29 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<32xi32>
      %30 = "memref.subview"(%28, %23) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<32xi32, strided<[1], offset: ?>>, index) -> memref<?xi32, strided<[1], offset: ?>>
      %31 = "memref.subview"(%29, %23) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<32xi32>, index) -> memref<?xi32, strided<[1]>>
      "hivm.hir.load"(%30, %31, %4, %6, %24) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xi32, strided<[1], offset: ?>>, memref<?xi32, strided<[1]>>, i32, index, i1) -> ()
      %32 = "bufferization.to_tensor"(%29) <{restrict, writable}> : (memref<32xi32>) -> tensor<32xi32>
      %33 = "tensor.empty"() : () -> tensor<32xi1>
      %34 = "hivm.hir.vcmp"(%27, %32, %33) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32xi32>, tensor<32xi32>, tensor<32xi1>) -> tensor<32xi1>
      %35 = "hivm.hir.vnot"(%34, %33) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<32xi1>, tensor<32xi1>) -> tensor<32xi1>
      %36 = "memref.reinterpret_cast"(%arg5, %17) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 32>, static_strides = array<i64: 1>}> : (memref<?xi8>, index) -> memref<32xi8, strided<[1], offset: ?>>
      %37 = "tensor.empty"() : () -> tensor<32xi8>
      %38 = "hivm.hir.vcast"(%35, %37) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<32xi1>, tensor<32xi8>) -> tensor<32xi8>
      %39 = "tensor.extract_slice"(%38, %23) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<32xi8>, index) -> tensor<?xi8>
      %40 = "memref.subview"(%36, %23) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<32xi8, strided<[1], offset: ?>>, index) -> memref<?xi8, strided<[1], offset: ?>>
      "hivm.hir.store"(%39, %40) : (tensor<?xi8>, memref<?xi8, strided<[1], offset: ?>>) -> ()
      "scf.yield"() : () -> ()
    }) : (i32, i32, i32) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, false, false, false]> : vector<9xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hacc.target = #hacc.target<"Ascend910B1">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

