#map = affine_map<()[s0] -> (s0 + 326)>
#map1 = affine_map<()[s0] -> (s0, 1024)>
#map2 = affine_map<()[s0, s1] -> (s0, s1)>
#map3 = affine_map<()[s0, s1] -> (s0 - s1)>
#map4 = affine_map<()[s0] -> (s0 * 16)>
#map5 = affine_map<()[s0] -> (s0, 326)>
"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xf32>, memref<?xi32>, memref<?xf32>, i32, i32, i32, i32) -> (), sym_name = "get_element_test_kernel"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xf32>, %arg4: memref<?xi32>, %arg5: memref<?xf32>, %arg6: i32, %arg7: i32, %arg8: i32, %arg9: i32):
    %0 = "arith.constant"() <{value = 1 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = 326 : index}> : () -> index
    %2 = "arith.constant"() <{value = 326 : i32}> : () -> i32
    %3 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %4 = "arith.constant"() <{value = 26 : i32}> : () -> i32
    %5 = "arith.constant"() <{value = 16 : i32}> : () -> i32
    %6 = "arith.constant"() <{value = 0 : index}> : () -> index
    "hivm.hir.set_mask_norm"() : () -> ()
    %7 = "arith.muli"(%arg7, %arg8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %8 = "arith.muli"(%7, %arg9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%8) <{effects = ["write"]}> {logical_block_num} : (i32) -> ()
    %9 = "hivm.hir.get_block_idx"() : () -> i64
    %10 = "arith.trunci"(%9) : (i64) -> i32
    %11 = "arith.muli"(%arg9, %arg8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %12 = "arith.divsi"(%10, %11) : (i32, i32) -> i32
    %13 = "arith.remsi"(%12, %arg7) : (i32, i32) -> i32
    %14 = "arith.muli"(%13, %4) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %15 = "arith.index_cast"(%14) : (i32) -> index
    %16 = "memref.reinterpret_cast"(%arg4, %15) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 326>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<326xi32, strided<[1], offset: ?>>
    %17 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<326xi32>
    %18 = "affine.apply"(%15) <{map = #map}> : (index) -> index
    %19 = "affine.max"(%15) <{map = #map1}> : (index) -> index
    %20 = "affine.min"(%18, %19) <{map = #map2}> : (index, index) -> index
    %21 = "affine.apply"(%20, %15) <{map = #map3}> : (index, index) -> index
    %22 = "arith.cmpi"(%21, %1) <{predicate = 2 : i64}> : (index, index) -> i1
    %23 = "memref.subview"(%16, %21) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<326xi32, strided<[1], offset: ?>>, index) -> memref<?xi32, strided<[1], offset: ?>>
    %24 = "memref.subview"(%17, %21) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<326xi32>, index) -> memref<?xi32, strided<[1]>>
    "hivm.hir.load"(%23, %24, %3, %6, %22) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xi32, strided<[1], offset: ?>>, memref<?xi32, strided<[1]>>, i32, index, i1) -> ()
    %25 = "bufferization.to_tensor"(%17) <{restrict, writable}> : (memref<326xi32>) -> tensor<326xi32>
    %26 = "tensor.empty"() : () -> tensor<326x16xf32>
    %27 = "scf.for"(%3, %2, %0, %26) ({
    ^bb0(%arg10: i32, %arg11: tensor<326x16xf32>):
      %33 = "arith.index_cast"(%arg10) : (i32) -> index
      %34 = "tensor.extract"(%25, %33) : (tensor<326xi32>, index) -> i32
      %35 = "arith.muli"(%34, %5) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %36 = "arith.index_cast"(%35) : (i32) -> index
      %37 = "memref.reinterpret_cast"(%arg3, %36) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1, 16>, static_strides = array<i64: 16, 1>}> : (memref<?xf32>, index) -> memref<1x16xf32, strided<[16, 1], offset: ?>>
      %38 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<1x16xf32>
      "hivm.hir.load"(%37, %38) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<1x16xf32, strided<[16, 1], offset: ?>>, memref<1x16xf32>) -> ()
      %39 = "bufferization.to_tensor"(%38) <{restrict, writable}> : (memref<1x16xf32>) -> tensor<1x16xf32>
      %40 = "tensor.insert_slice"(%39, %arg11, %33) <{operandSegmentSizes = array<i32: 1, 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808, 0>, static_sizes = array<i64: 1, 16>, static_strides = array<i64: 1, 1>}> : (tensor<1x16xf32>, tensor<326x16xf32>, index) -> tensor<326x16xf32>
      "scf.yield"(%40) : (tensor<326x16xf32>) -> ()
    }) : (i32, i32, i32, tensor<326x16xf32>) -> tensor<326x16xf32>
    %28 = "affine.apply"(%15) <{map = #map4}> : (index) -> index
    %29 = "memref.reinterpret_cast"(%arg5, %28) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 326, 16>, static_strides = array<i64: 16, 1>}> : (memref<?xf32>, index) -> memref<326x16xf32, strided<[16, 1], offset: ?>>
    %30 = "affine.min"(%21) <{map = #map5}> : (index) -> index
    %31 = "tensor.extract_slice"(%27, %30) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (tensor<326x16xf32>, index) -> tensor<?x16xf32>
    %32 = "memref.subview"(%29, %30) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (memref<326x16xf32, strided<[16, 1], offset: ?>>, index) -> memref<?x16xf32, strided<[16, 1], offset: ?>>
    "hivm.hir.store"(%31, %32) : (tensor<?x16xf32>, memref<?x16xf32, strided<[16, 1], offset: ?>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, false, false, false, false]> : vector<10xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hacc.target = #hacc.target<"Ascend910B1">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

