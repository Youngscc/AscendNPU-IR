#map = affine_map<()[s0, s1] -> (s0 + s1)>
#map1 = affine_map<()[s0] -> (s0 + 16)>
#map2 = affine_map<()[s0, s1] -> (s0, s1)>
#map3 = affine_map<()[s0, s1] -> (s0 - s1)>
#map4 = affine_map<()[s0] -> (s0, 16)>
"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xbf16>, memref<?xbf16>, i32, i32, i32, i32) -> (), sym_name = "solve_tril_16x16_kernel"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xbf16>, %arg4: memref<?xbf16>, %arg5: i32, %arg6: i32, %arg7: i32, %arg8: i32):
    %0 = "arith.constant"() <{value = 1 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = -1.000000e+00 : f32}> : () -> f32
    %2 = "arith.constant"() <{value = 0 : index}> : () -> index
    %3 = "arith.constant"() <{value = 1 : index}> : () -> index
    %4 = "arith.constant"() <{value = 0.000000e+00 : f32}> : () -> f32
    %5 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %6 = "arith.constant"() <{value = 32 : i32}> : () -> i32
    %7 = "arith.constant"() <{value = 16 : i32}> : () -> i32
    %8 = "arith.constant"() <{value = 4 : i32}> : () -> i32
    %9 = "arith.constant"() <{value = 1.000000e+00 : f32}> : () -> f32
    %10 = "arith.constant"() <{value = 2 : i32}> : () -> i32
    %11 = "arith.constant"() <{value = 64 : i32}> : () -> i32
    %12 = "arith.constant"() <{value = 16 : index}> : () -> index
    %13 = "arith.constant"() <{value = 0.000000e+00 : bf16}> : () -> bf16
    "hivm.hir.set_mask_norm"() : () -> ()
    %14 = "arith.muli"(%arg6, %arg7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %15 = "arith.muli"(%14, %arg8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%15) <{effects = ["write"]}> {logical_block_num} : (i32) -> ()
    %16 = "hivm.hir.get_block_idx"() : () -> i64
    %17 = "arith.trunci"(%16) : (i64) -> i32
    %18 = "arith.divsi"(%17, %arg8) : (i32, i32) -> i32
    %19 = "arith.remsi"(%18, %arg7) : (i32, i32) -> i32
    %20 = "arith.muli"(%arg8, %arg7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %21 = "arith.divsi"(%17, %20) : (i32, i32) -> i32
    %22 = "arith.remsi"(%21, %arg6) : (i32, i32) -> i32
    %23 = "tensor.empty"() : () -> tensor<1x16x16xf32>
    %24 = "tensor.empty"() : () -> tensor<1x16xf32>
    %25 = "arith.divsi"(%19, %8) : (i32, i32) -> i32
    %26 = "arith.remsi"(%19, %8) : (i32, i32) -> i32
    %27 = "arith.muli"(%25, %arg5) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %28 = "arith.muli"(%27, %8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %29 = "arith.addi"(%28, %26) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %30 = "arith.muli"(%29, %7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %31 = "arith.index_cast"(%30) : (i32) -> index
    %32 = "arith.muli"(%22, %6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %33 = "tensor.empty"() : () -> tensor<16xi32>
    %34 = "hivm.hir.varange"(%33, %2, %3) <{operandSegmentSizes = array<i32: 1, 1, 1>}> : (tensor<16xi32>, index, index) -> tensor<16xi32>
    %35 = "tensor.empty"() : () -> tensor<16xf32>
    %36 = "hivm.hir.vcast"(%34, %35) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16xi32>, tensor<16xf32>) -> tensor<16xf32>
    %37 = "tensor.empty"() : () -> tensor<16x16xf32>
    %38 = "tensor.expand_shape"(%36) <{reassociation = [[0, 1]], static_output_shape = array<i64: 16, 1>}> : (tensor<16xf32>) -> tensor<16x1xf32>
    %39 = "hivm.hir.vbrc"(%38, %37) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %40 = "tensor.expand_shape"(%36) <{reassociation = [[0, 1]], static_output_shape = array<i64: 1, 16>}> : (tensor<16xf32>) -> tensor<1x16xf32>
    %41 = "hivm.hir.vbrc"(%40, %37) <{broadcast_dims = array<i64: 0>}> : (tensor<1x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %42 = "tensor.empty"() : () -> tensor<16x16xi1>
    %43 = "hivm.hir.vcmp"(%39, %41, %42) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<gt>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x16xf32>, tensor<16x16xf32>, tensor<16x16xi1>) -> tensor<16x16xi1>
    %44 = "hivm.hir.vcast"(%43, %37) <{broadcast = array<i64>, cast = #hivm.cast<cast_unsigned>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x16xi1>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %45 = "tensor.expand_shape"(%44) <{reassociation = [[0, 1], [2]], static_output_shape = array<i64: 1, 16, 16>}> : (tensor<16x16xf32>) -> tensor<1x16x16xf32>
    %46 = "hivm.hir.vcmp"(%39, %41, %42) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x16xf32>, tensor<16x16xf32>, tensor<16x16xi1>) -> tensor<16x16xi1>
    %47 = "tensor.expand_shape"(%46) <{reassociation = [[0, 1], [2]], static_output_shape = array<i64: 1, 16, 16>}> : (tensor<16x16xi1>) -> tensor<1x16x16xi1>
    %48 = "scf.for"(%5, %10, %0, %32) ({
    ^bb0(%arg9: i32, %arg10: i32):
      %49 = "arith.muli"(%arg9, %7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %50 = "arith.addi"(%arg10, %49) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %51 = "arith.remsi"(%50, %7) : (i32, i32) -> i32
      %52 = "arith.muli"(%50, %11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %53 = "arith.index_cast"(%52) : (i32) -> index
      %54 = "affine.apply"(%31, %53) <{map = #map}> : (index, index) -> index
      %55 = "arith.index_cast"(%51) : (i32) -> index
      %56 = "affine.apply"(%54, %55) <{map = #map}> : (index, index) -> index
      %57 = "memref.reinterpret_cast"(%arg3, %56) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 64, 1>}> : (memref<?xbf16>, index) -> memref<16x16xbf16, strided<[64, 1], offset: ?>>
      %58 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x16xbf16>
      %59 = "arith.index_cast"(%50) : (i32) -> index
      %60 = "affine.apply"(%59) <{map = #map1}> : (index) -> index
      %61 = "arith.index_cast"(%arg5) : (i32) -> index
      %62 = "affine.max"(%59, %61) <{map = #map2}> : (index, index) -> index
      %63 = "affine.min"(%60, %62) <{map = #map2}> : (index, index) -> index
      %64 = "affine.apply"(%63, %59) <{map = #map3}> : (index, index) -> index
      %65 = "affine.apply"(%55) <{map = #map1}> : (index) -> index
      %66 = "affine.max"(%55) <{map = #map4}> : (index) -> index
      %67 = "affine.min"(%65, %66) <{map = #map2}> : (index, index) -> index
      %68 = "affine.apply"(%67, %55) <{map = #map3}> : (index, index) -> index
      %69 = "affine.min"(%64) <{map = #map4}> : (index) -> index
      %70 = "affine.min"(%68) <{map = #map4}> : (index) -> index
      %71 = "arith.cmpi"(%69, %12) <{predicate = 2 : i64}> : (index, index) -> i1
      %72 = "arith.cmpi"(%70, %12) <{predicate = 2 : i64}> : (index, index) -> i1
      %73 = "arith.ori"(%71, %72) : (i1, i1) -> i1
      %74 = "memref.subview"(%57, %69, %70) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16, strided<[64, 1], offset: ?>>, index, index) -> memref<?x?xbf16, strided<[64, 1], offset: ?>>
      %75 = "memref.subview"(%58, %69, %70) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16>, index, index) -> memref<?x?xbf16, strided<[16, 1]>>
      "hivm.hir.load"(%74, %75, %13, %2, %73) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x?xbf16, strided<[64, 1], offset: ?>>, memref<?x?xbf16, strided<[16, 1]>>, bf16, index, i1) -> ()
      %76 = "bufferization.to_tensor"(%58) <{restrict, writable}> : (memref<16x16xbf16>) -> tensor<16x16xbf16>
      %77 = "hivm.hir.vcast"(%76, %37) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x16xbf16>, tensor<16x16xf32>) -> tensor<16x16xf32>
      %78 = "tensor.expand_shape"(%77) <{reassociation = [[0, 1], [2]], static_output_shape = array<i64: 1, 16, 16>}> : (tensor<16x16xf32>) -> tensor<1x16x16xf32>
      %79 = "tensor.empty"() : () -> tensor<16x1x16xf32>
      %80 = "hivm.hir.vmul"(%78, %1, %23) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1x16x16xf32>, f32, tensor<1x16x16xf32>) -> tensor<1x16x16xf32>
      %81 = "hivm.hir.vadd"(%80, %4, %23) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1x16x16xf32>, f32, tensor<1x16x16xf32>) -> tensor<1x16x16xf32>
      %82 = "hivm.hir.vmul"(%81, %45, %23) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1x16x16xf32>, tensor<1x16x16xf32>, tensor<1x16x16xf32>) -> tensor<1x16x16xf32>
      %83 = "scf.for"(%0, %7, %0, %82) ({
      ^bb0(%arg11: i32, %arg12: tensor<1x16x16xf32>):
        %92 = "arith.index_cast"(%arg11) : (i32) -> index
        %93 = "tensor.extract_slice"(%77, %92) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808, 0>, static_sizes = array<i64: 1, 16>, static_strides = array<i64: 16, 1>}> : (tensor<16x16xf32>, index) -> tensor<1x16xf32>
        %94 = "hivm.hir.vmul"(%93, %1, %24) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1x16xf32>, f32, tensor<1x16xf32>) -> tensor<1x16xf32>
        %95 = "hivm.hir.vadd"(%94, %4, %24) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1x16xf32>, f32, tensor<1x16xf32>) -> tensor<1x16xf32>
        %96 = "tensor.expand_shape"(%95) <{reassociation = [[0], [1, 2]], static_output_shape = array<i64: 1, 16, 1>}> : (tensor<1x16xf32>) -> tensor<1x16x1xf32>
        %97 = "hivm.hir.vbrc"(%96, %23) <{broadcast_dims = array<i64: 2>}> : (tensor<1x16x1xf32>, tensor<1x16x16xf32>) -> tensor<1x16x16xf32>
        %98 = "hivm.hir.vmul"(%97, %arg12, %23) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1x16x16xf32>, tensor<1x16x16xf32>, tensor<1x16x16xf32>) -> tensor<1x16x16xf32>
        %99 = "hivm.hir.vtranspose"(%98, %79) <{disable_align = false, permutation = array<i64: 1, 0, 2>}> : (tensor<1x16x16xf32>, tensor<16x1x16xf32>) -> tensor<16x1x16xf32>
        %100 = "tensor.empty"() : () -> tensor<1x1x16xf32>
        %101 = "hivm.hir.vreduce"(%99, %100) <{arith = #hivm.reduce_op<sum>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 0>, tie_break_left = true, unsigned_src = false}> : (tensor<16x1x16xf32>, tensor<1x1x16xf32>) -> tensor<1x1x16xf32>
        %102 = "tensor.collapse_shape"(%101) <{reassociation = [[0, 1], [2]]}> : (tensor<1x1x16xf32>) -> tensor<1x16xf32>
        %103 = "hivm.hir.vadd"(%95, %102, %24) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1x16xf32>, tensor<1x16xf32>, tensor<1x16xf32>) -> tensor<1x16xf32>
        %104 = "tensor.expand_shape"(%103) <{reassociation = [[0], [1, 2]], static_output_shape = array<i64: 1, 1, 16>}> : (tensor<1x16xf32>) -> tensor<1x1x16xf32>
        %105 = "tensor.insert_slice"(%104, %arg12, %92) <{operandSegmentSizes = array<i32: 1, 1, 1, 0, 0>, static_offsets = array<i64: 0, -9223372036854775808, 0>, static_sizes = array<i64: 1, 1, 16>, static_strides = array<i64: 1, 1, 1>}> : (tensor<1x1x16xf32>, tensor<1x16x16xf32>, index) -> tensor<1x16x16xf32>
        "scf.yield"(%105) : (tensor<1x16x16xf32>) -> ()
      }) : (i32, i32, i32, tensor<1x16x16xf32>) -> tensor<1x16x16xf32>
      %84 = "hivm.hir.vadd"(%83, %9, %23) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1x16x16xf32>, f32, tensor<1x16x16xf32>) -> tensor<1x16x16xf32>
      %85 = "hivm.hir.vsel"(%47, %84, %83, %23) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<1x16x16xi1>, tensor<1x16x16xf32>, tensor<1x16x16xf32>, tensor<1x16x16xf32>) -> tensor<1x16x16xf32>
      %86 = "tensor.collapse_shape"(%85) <{reassociation = [[0, 1], [2]]}> : (tensor<1x16x16xf32>) -> tensor<16x16xf32>
      %87 = "memref.reinterpret_cast"(%arg4, %54) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 64, 1>}> : (memref<?xbf16>, index) -> memref<16x16xbf16, strided<[64, 1], offset: ?>>
      %88 = "tensor.empty"() : () -> tensor<16x16xbf16>
      %89 = "hivm.hir.vcast"(%86, %88) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x16xf32>, tensor<16x16xbf16>) -> tensor<16x16xbf16>
      %90 = "tensor.extract_slice"(%89, %64) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (tensor<16x16xbf16>, index) -> tensor<?x16xbf16>
      %91 = "memref.subview"(%87, %64) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16, strided<[64, 1], offset: ?>>, index) -> memref<?x16xbf16, strided<[64, 1], offset: ?>>
      "hivm.hir.store"(%90, %91) : (tensor<?x16xbf16>, memref<?x16xbf16, strided<[64, 1], offset: ?>>) -> ()
      "scf.yield"(%50) : (i32) -> ()
    }) : (i32, i32, i32, i32) -> i32
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, false, false, false, false]> : vector<9xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hacc.target = #hacc.target<"Ascend910B1">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

