#map = affine_map<()[s0] -> (s0 - 1)>
#map1 = affine_map<()[s0, s1] -> (s0 + s1)>
#map2 = affine_map<()[s0] -> (s0 + 16)>
#map3 = affine_map<()[s0, s1] -> (s0, s1)>
#map4 = affine_map<()[s0, s1] -> (s0 - s1)>
"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 2 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xi32>, memref<?xi32>, memref<?xi32>, memref<?xf32>, memref<?xf32>, i32, i32, i32, i32) -> (), sym_name = "sample_recovered_tokens_kernel"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xi32>, %arg4: memref<?xi32>, %arg5: memref<?xi32>, %arg6: memref<?xf32>, %arg7: memref<?xf32>, %arg8: i32, %arg9: i32, %arg10: i32, %arg11: i32):
    %0 = "arith.constant"() <{value = 1 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = -2139095040 : i32}> : () -> i32
    %2 = "arith.constant"() <{value = 0x7F800000 : f32}> : () -> f32
    %3 = "arith.constant"() <{value = 2147483647 : i32}> : () -> i32
    %4 = "arith.constant"() <{value = 0xFF800000 : f32}> : () -> f32
    %5 = "arith.constant"() <{value = 16 : index}> : () -> index
    %6 = "arith.constant"() <{value = 0 : index}> : () -> index
    %7 = "arith.constant"() <{value = 15 : i32}> : () -> i32
    %8 = "arith.constant"() <{value = -1.000000e+00 : f32}> : () -> f32
    %9 = "arith.constant"() <{value = -1 : i32}> : () -> i32
    %10 = "arith.constant"() <{value = 16 : i32}> : () -> i32
    %11 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %12 = "arith.constant"() <{value = 0.000000e+00 : f32}> : () -> f32
    "hivm.hir.set_mask_norm"() : () -> ()
    %13 = "arith.muli"(%arg9, %arg10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %14 = "arith.muli"(%13, %arg11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%14) <{effects = ["write"]}> {logical_block_num} : (i32) -> ()
    %15 = "hivm.hir.get_block_idx"() : () -> i64
    %16 = "arith.trunci"(%15) : (i64) -> i32
    %17 = "arith.divsi"(%16, %arg11) : (i32, i32) -> i32
    %18 = "arith.remsi"(%17, %arg10) : (i32, i32) -> i32
    %19 = "arith.muli"(%arg11, %arg10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %20 = "arith.divsi"(%16, %19) : (i32, i32) -> i32
    %21 = "arith.remsi"(%20, %arg9) : (i32, i32) -> i32
    %22 = "tensor.empty"() : () -> tensor<1xf32>
    %23 = "tensor.empty"() : () -> tensor<1xf32>
    %24 = "hivm.hir.vbrc"(%12, %22) <{broadcast_dims = array<i64>}> : (f32, tensor<1xf32>) -> tensor<1xf32>
    %25 = "arith.cmpi"(%21, %11) <{predicate = 0 : i64}> : (i32, i32) -> i1
    %26 = "scf.if"(%25) ({
      "scf.yield"(%11) : (i32) -> ()
    }, {
      %107 = "arith.index_cast"(%21) : (i32) -> index
      %108 = "affine.apply"(%107) <{map = #map}> : (index) -> index
      %109 = "memref.reinterpret_cast"(%arg4, %108) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<1xi32, strided<[1], offset: ?>>
      %110 = "memref.load"(%109, %6) : (memref<1xi32, strided<[1], offset: ?>>, index) -> i32
      "scf.yield"(%110) : (i32) -> ()
    }) : (i1) -> i32
    %27 = "arith.index_cast"(%21) : (i32) -> index
    %28 = "memref.reinterpret_cast"(%arg4, %27) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<1xi32, strided<[1], offset: ?>>
    %29 = "memref.load"(%28, %6) : (memref<1xi32, strided<[1], offset: ?>>, index) -> i32
    %30 = "arith.subi"(%29, %26) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %31 = "arith.cmpi"(%18, %30) <{predicate = 5 : i64}> : (i32, i32) -> i1
    "scf.if"(%31) ({
      "scf.yield"() : () -> ()
    }, {
      %32 = "arith.addi"(%arg8, %7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %33 = "arith.divsi"(%32, %10) : (i32, i32) -> i32
      %34 = "arith.index_cast"(%26) : (i32) -> index
      %35 = "arith.index_cast"(%18) : (i32) -> index
      %36 = "affine.apply"(%34, %35) <{map = #map1}> : (index, index) -> index
      %37 = "memref.reinterpret_cast"(%arg5, %36) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<1xi32, strided<[1], offset: ?>>
      %38 = "memref.load"(%37, %6) : (memref<1xi32, strided<[1], offset: ?>>, index) -> i32
      %39 = "arith.addi"(%26, %18) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %40 = "arith.muli"(%39, %arg8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %41 = "arith.index_cast"(%40) : (i32) -> index
      %42 = "arith.index_cast"(%38) : (i32) -> index
      %43 = "affine.apply"(%41, %42) <{map = #map1}> : (index, index) -> index
      %44 = "memref.reinterpret_cast"(%arg6, %43) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<1xf32, strided<[1], offset: ?>>
      %45 = "memref.load"(%44, %6) : (memref<1xf32, strided<[1], offset: ?>>, index) -> f32
      "hivm.hir.store"(%24, %44) : (tensor<1xf32>, memref<1xf32, strided<[1], offset: ?>>) -> ()
      %46 = "arith.muli"(%21, %arg8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %47 = "arith.index_cast"(%46) : (i32) -> index
      %48:2 = "scf.for"(%11, %33, %0, %9, %8) ({
      ^bb0(%arg12: i32, %arg13: i32, %arg14: f32):
        %53 = "arith.muli"(%arg12, %10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
        %54 = "arith.index_cast"(%53) : (i32) -> index
        %55 = "affine.apply"(%41, %54) <{map = #map1}> : (index, index) -> index
        %56 = "memref.reinterpret_cast"(%arg6, %55) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<16xf32, strided<[1], offset: ?>>
        %57 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16xf32>
        %58 = "affine.apply"(%54) <{map = #map2}> : (index) -> index
        %59 = "arith.index_cast"(%arg8) : (i32) -> index
        %60 = "affine.max"(%54, %59) <{map = #map3}> : (index, index) -> index
        %61 = "affine.min"(%58, %60) <{map = #map3}> : (index, index) -> index
        %62 = "affine.apply"(%61, %54) <{map = #map4}> : (index, index) -> index
        %63 = "arith.cmpi"(%62, %5) <{predicate = 2 : i64}> : (index, index) -> i1
        %64 = "memref.subview"(%56, %62) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<16xf32, strided<[1], offset: ?>>, index) -> memref<?xf32, strided<[1], offset: ?>>
        %65 = "memref.subview"(%57, %62) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<16xf32>, index) -> memref<?xf32, strided<[1]>>
        "hivm.hir.load"(%64, %65, %12, %6, %63) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xf32, strided<[1], offset: ?>>, memref<?xf32, strided<[1]>>, f32, index, i1) -> ()
        %66 = "bufferization.to_tensor"(%57) <{restrict, writable}> : (memref<16xf32>) -> tensor<16xf32>
        %67 = "affine.apply"(%47, %54) <{map = #map1}> : (index, index) -> index
        %68 = "memref.reinterpret_cast"(%arg7, %67) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<16xf32, strided<[1], offset: ?>>
        %69 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16xf32>
        %70 = "memref.subview"(%68, %62) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<16xf32, strided<[1], offset: ?>>, index) -> memref<?xf32, strided<[1], offset: ?>>
        %71 = "memref.subview"(%69, %62) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<16xf32>, index) -> memref<?xf32, strided<[1]>>
        "hivm.hir.load"(%70, %71, %4, %6, %63) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xf32, strided<[1], offset: ?>>, memref<?xf32, strided<[1]>>, f32, index, i1) -> ()
        %72 = "bufferization.to_tensor"(%69) <{restrict, writable}> : (memref<16xf32>) -> tensor<16xf32>
        %73 = "tensor.empty"() : () -> tensor<16xf32>
        %74 = "hivm.hir.vdiv"(%66, %72, %73) <{broadcast = array<i64>, isHP = false, isSigned = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %75 = "tensor.empty"() : () -> tensor<1xf32>
        %76 = "tensor.empty"() : () -> tensor<1xi32>
        %77:2 = "hivm.hir.vreduce"(%74, %75, %76) <{arith = #hivm.reduce_op<max_with_index_left>, operandSegmentSizes = array<i32: 1, 2, 0, 0>, reduce_dims = array<i64: 0>, tie_break_left = true, unsigned_src = false}> : (tensor<16xf32>, tensor<1xf32>, tensor<1xi32>) -> (tensor<1xf32>, tensor<1xi32>)
        %78 = "tensor.empty"() : () -> tensor<16xi1>
        %79 = "hivm.hir.vcmp"(%74, %74, %78) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xi1>) -> tensor<16xi1>
        %80 = "hivm.hir.vnot"(%79, %78) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xi1>, tensor<16xi1>) -> tensor<16xi1>
        %81 = "hivm.hir.vsel"(%80, %2, %12, %73) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<16xi1>, f32, f32, tensor<16xf32>) -> tensor<16xf32>
        %82:2 = "hivm.hir.vreduce"(%81, %75, %76) <{arith = #hivm.reduce_op<max_with_index_left>, operandSegmentSizes = array<i32: 1, 2, 0, 0>, reduce_dims = array<i64: 0>, tie_break_left = true, unsigned_src = false}> : (tensor<16xf32>, tensor<1xf32>, tensor<1xi32>) -> (tensor<1xf32>, tensor<1xi32>)
        %83 = "hivm.hir.bitcast"(%82#0) : (tensor<1xf32>) -> tensor<1xi32>
        %84 = "hivm.hir.vbrc"(%3, %76) <{broadcast_dims = array<i64>}> : (i32, tensor<1xi32>) -> tensor<1xi32>
        %85 = "hivm.hir.vand"(%83, %84, %76) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1xi32>, tensor<1xi32>, tensor<1xi32>) -> tensor<1xi32>
        %86 = "hivm.hir.vadd"(%85, %1, %76) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1xi32>, i32, tensor<1xi32>) -> tensor<1xi32>
        %87 = "hivm.hir.bitcast"(%86) : (tensor<1xi32>) -> tensor<1xf32>
        %88 = "hivm.hir.vabs"(%87, %75) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<1xf32>, tensor<1xf32>) -> tensor<1xf32>
        %89 = "hivm.hir.bitcast"(%88) : (tensor<1xf32>) -> tensor<1xi32>
        %90 = "hivm.hir.vmin"(%89, %0, %76) <{broadcast = array<i64>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1xi32>, i32, tensor<1xi32>) -> tensor<1xi32>
        %91 = "hivm.hir.vmul"(%90, %9, %90) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1xi32>, i32, tensor<1xi32>) -> tensor<1xi32>
        %92 = "hivm.hir.vadd"(%91, %0, %91) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1xi32>, i32, tensor<1xi32>) -> tensor<1xi32>
        %93 = "hivm.hir.vcast"(%92, %75) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<1xi32>, tensor<1xf32>) -> tensor<1xf32>
        %94 = "tensor.empty"() : () -> tensor<1xi1>
        %95 = "hivm.hir.vcmp"(%93, %12, %94) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1xf32>, f32, tensor<1xi1>) -> tensor<1xi1>
        %96 = "hivm.hir.vnot"(%95, %94) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<1xi1>, tensor<1xi1>) -> tensor<1xi1>
        %97 = "hivm.hir.vsel"(%96, %82#1, %77#1, %76) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<1xi1>, tensor<1xi32>, tensor<1xi32>, tensor<1xi32>) -> tensor<1xi32>
        %98 = "tensor.extract"(%97, %6) : (tensor<1xi32>, index) -> i32
        %99 = "arith.index_cast"(%98) : (i32) -> index
        %100 = "tensor.extract"(%66, %99) {DiscreteMemAccess} : (tensor<16xf32>, index) -> f32
        %101 = "tensor.extract"(%72, %99) {DiscreteMemAccess} : (tensor<16xf32>, index) -> f32
        %102 = "arith.divf"(%100, %101) <{fastmath = #arith.fastmath<none>}> : (f32, f32) -> f32
        %103 = "arith.cmpf"(%102, %arg14) <{fastmath = #arith.fastmath<none>, predicate = 2 : i64}> : (f32, f32) -> i1
        %104 = "arith.select"(%103, %102, %arg14) : (i1, f32, f32) -> f32
        %105 = "scf.if"(%103) ({
          %106 = "arith.addi"(%53, %98) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
          "scf.yield"(%106) : (i32) -> ()
        }, {
          "scf.yield"(%arg13) : (i32) -> ()
        }) : (i1) -> i32
        "scf.yield"(%105, %104) : (i32, f32) -> ()
      }) : (i32, i32, i32, i32, f32) -> (i32, f32)
      %49 = "tensor.empty"() : () -> tensor<1xi32>
      %50 = "tensor.insert"(%48#0, %49, %6) : (i32, tensor<1xi32>, index) -> tensor<1xi32>
      %51 = "memref.reinterpret_cast"(%arg3, %36) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<1xi32, strided<[1], offset: ?>>
      "hivm.hir.store"(%50, %51) : (tensor<1xi32>, memref<1xi32, strided<[1], offset: ?>>) -> ()
      %52 = "tensor.insert"(%45, %23, %6) : (f32, tensor<1xf32>, index) -> tensor<1xf32>
      "hivm.hir.store"(%52, %44) : (tensor<1xf32>, memref<1xf32, strided<[1], offset: ?>>) -> ()
      "scf.yield"() : () -> ()
    }) : (i1) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, true, true, false, false, false, false]> : vector<12xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hacc.target = #hacc.target<"Ascend910B1">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

