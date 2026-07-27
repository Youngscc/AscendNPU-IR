#map = affine_map<()[s0] -> (s0 + 1)>
#map1 = affine_map<()[s0] -> (s0 + 2)>
#map2 = affine_map<()[s0, s1] -> (s0 * s1)>
#map3 = affine_map<()[s0, s1] -> (s0 + s1)>
#map4 = affine_map<()[s0] -> (s0 * 32)>
#map5 = affine_map<()[s0] -> (s0 + 128)>
#map6 = affine_map<()[s0] -> (s0 * 128)>
#map7 = affine_map<()[s0] -> (s0 + 32)>
"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 2 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xi64>, memref<?xi64>, memref<?xi64>, memref<?xi32>, memref<?xi32>, i32, i32, i32, i32) -> (), sym_name = "grouped_matmul_kernel"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xi64>, %arg4: memref<?xi64>, %arg5: memref<?xi64>, %arg6: memref<?xi32>, %arg7: memref<?xi32>, %arg8: i32, %arg9: i32, %arg10: i32, %arg11: i32):
    %0 = "arith.constant"() <{value = 1 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = 128 : index}> : () -> index
    %2 = "arith.constant"() <{value = 2 : i64}> : () -> i64
    %3 = "arith.constant"() <{value = 32 : index}> : () -> index
    %4 = "arith.constant"() <{value = 0 : index}> : () -> index
    %5 = "arith.constant"() <{value = 31 : i32}> : () -> i32
    %6 = "arith.constant"() <{value = 127 : i32}> : () -> i32
    %7 = "arith.constant"() <{value = 20 : i32}> : () -> i32
    %8 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %9 = "arith.constant"() <{value = 3 : i32}> : () -> i32
    %10 = "arith.constant"() <{value = 128 : i32}> : () -> i32
    %11 = "arith.constant"() <{value = 32 : i32}> : () -> i32
    "hivm.hir.set_mask_norm"() : () -> ()
    %12 = "arith.muli"(%arg9, %arg10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %13 = "arith.muli"(%12, %arg11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%13) <{effects = ["write"]}> {logical_block_num} : (i32) -> ()
    %14 = "hivm.hir.get_block_idx"() : () -> i64
    %15 = "arith.trunci"(%14) : (i64) -> i32
    %16 = "arith.muli"(%arg11, %arg10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %17 = "arith.divsi"(%15, %16) : (i32, i32) -> i32
    %18 = "arith.remsi"(%17, %arg9) : (i32, i32) -> i32
    %19:2 = "scf.for"(%8, %arg8, %0, %18, %8) ({
    ^bb0(%arg12: i32, %arg13: i32, %arg14: i32):
      %20 = "arith.muli"(%arg12, %9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %21 = "arith.index_cast"(%20) : (i32) -> index
      %22 = "memref.reinterpret_cast"(%arg6, %21) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<1xi32, strided<[1], offset: ?>>
      %23 = "memref.load"(%22, %4) : (memref<1xi32, strided<[1], offset: ?>>, index) -> i32
      %24 = "affine.apply"(%21) <{map = #map}> : (index) -> index
      %25 = "memref.reinterpret_cast"(%arg6, %24) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<1xi32, strided<[1], offset: ?>>
      %26 = "memref.load"(%25, %4) : (memref<1xi32, strided<[1], offset: ?>>, index) -> i32
      %27 = "affine.apply"(%21) <{map = #map1}> : (index) -> index
      %28 = "memref.reinterpret_cast"(%arg6, %27) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<1xi32, strided<[1], offset: ?>>
      %29 = "memref.load"(%28, %4) : (memref<1xi32, strided<[1], offset: ?>>, index) -> i32
      %30 = "arith.addi"(%23, %6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %31 = "arith.divsi"(%30, %10) : (i32, i32) -> i32
      %32 = "arith.addi"(%26, %6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %33 = "arith.divsi"(%32, %10) : (i32, i32) -> i32
      %34 = "arith.muli"(%31, %33) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %35 = "arith.addi"(%arg14, %34) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %36 = "memref.reinterpret_cast"(%arg7, %21) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<1xi32, strided<[1], offset: ?>>
      %37 = "memref.reinterpret_cast"(%arg7, %24) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<1xi32, strided<[1], offset: ?>>
      %38 = "memref.reinterpret_cast"(%arg7, %27) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<1xi32, strided<[1], offset: ?>>
      %39 = "arith.index_cast"(%arg12) : (i32) -> index
      %40 = "memref.reinterpret_cast"(%arg3, %39) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi64>, index) -> memref<1xi64, strided<[1], offset: ?>>
      %41 = "memref.reinterpret_cast"(%arg4, %39) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi64>, index) -> memref<1xi64, strided<[1], offset: ?>>
      %42 = "memref.reinterpret_cast"(%arg5, %39) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi64>, index) -> memref<1xi64, strided<[1], offset: ?>>
      %43 = "arith.addi"(%29, %5) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %44 = "arith.divsi"(%43, %11) : (i32, i32) -> i32
      %45 = "scf.while"(%arg13) ({
      ^bb0(%arg20: i32):
        %100 = "arith.cmpi"(%arg20, %arg14) <{predicate = 5 : i64}> : (i32, i32) -> i1
        %101 = "arith.cmpi"(%arg20, %35) <{predicate = 2 : i64}> : (i32, i32) -> i1
        %102 = "arith.andi"(%100, %101) : (i1, i1) -> i1
        "scf.condition"(%102, %arg20) : (i1, i32) -> ()
      }, {
      ^bb0(%arg15: i32):
        %46 = "memref.load"(%36, %4) : (memref<1xi32, strided<[1], offset: ?>>, index) -> i32
        %47 = "memref.load"(%37, %4) : (memref<1xi32, strided<[1], offset: ?>>, index) -> i32
        %48 = "memref.load"(%38, %4) : (memref<1xi32, strided<[1], offset: ?>>, index) -> i32
        %49 = "memref.load"(%40, %4) : (memref<1xi64, strided<[1], offset: ?>>, index) -> i64
        %50 = "memref.load"(%41, %4) : (memref<1xi64, strided<[1], offset: ?>>, index) -> i64
        %51 = "memref.load"(%42, %4) : (memref<1xi64, strided<[1], offset: ?>>, index) -> i64
        %52 = "arith.subi"(%arg15, %arg14) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
        %53 = "arith.divsi"(%52, %33) : (i32, i32) -> i32
        %54 = "arith.remsi"(%52, %33) : (i32, i32) -> i32
        %55 = "arith.muli"(%53, %10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
        %56 = "arith.muli"(%54, %10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
        %57 = "arith.muli"(%47, %11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
        %58 = "arith.index_cast"(%55) : (i32) -> index
        %59 = "arith.index_cast"(%46) : (i32) -> index
        %60 = "affine.apply"(%58, %59) <{map = #map2}> : (index, index) -> index
        %61 = "arith.index_cast"(%47) : (i32) -> index
        %62 = "arith.index_cast"(%56) : (i32) -> index
        %63 = "tensor.empty"() : () -> tensor<128x128xf32>
        %64:3 = "scf.for"(%8, %44, %0, %63, %60, %4) ({
        ^bb0(%arg16: i32, %arg17: tensor<128x128xf32>, %arg18: index, %arg19: index):
          %76 = "affine.apply"(%arg19, %62) <{map = #map3}> : (index, index) -> index
          %77 = "affine.apply"(%61) <{map = #map4}> : (index) -> index
          %78 = "affine.apply"(%77) <{map = #map5}> : (index) -> index
          %79 = "arith.index_cast"(%76) : (index) -> i64
          %80 = "arith.muli"(%79, %2) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
          %81 = "arith.addi"(%50, %80) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
          %82 = "hivm.hir.pointer_cast"(%81, %78) <{operandSegmentSizes = array<i32: 1, 1>}> : (i64, index) -> memref<?xf16>
          "annotation.mark"(%82) <{effects = ["write"]}> {address_space = #hivm.address_space<gm>} : (memref<?xf16>) -> ()
          %83 = "memref.reinterpret_cast"(%82, %61) <{operandSegmentSizes = array<i32: 1, 0, 0, 1>, static_offsets = array<i64: 0>, static_sizes = array<i64: 32, 128>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xf16>, index) -> memref<32x128xf16, strided<[?, 1]>>
          %84 = "affine.apply"(%59) <{map = #map6}> : (index) -> index
          %85 = "affine.apply"(%84) <{map = #map7}> : (index) -> index
          %86 = "arith.index_cast"(%arg18) : (index) -> i64
          %87 = "arith.muli"(%86, %2) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
          %88 = "arith.addi"(%49, %87) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
          %89 = "hivm.hir.pointer_cast"(%88, %85) <{operandSegmentSizes = array<i32: 1, 1>}> : (i64, index) -> memref<?xf16>
          "annotation.mark"(%89) <{effects = ["write"]}> {address_space = #hivm.address_space<gm>} : (memref<?xf16>) -> ()
          %90 = "memref.reinterpret_cast"(%89, %59) <{operandSegmentSizes = array<i32: 1, 0, 0, 1>, static_offsets = array<i64: 0>, static_sizes = array<i64: 128, 32>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xf16>, index) -> memref<128x32xf16, strided<[?, 1]>>
          %91 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<128x32xf16>
          "hivm.hir.load"(%90, %91) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<128x32xf16, strided<[?, 1]>>, memref<128x32xf16>) -> ()
          %92 = "bufferization.to_tensor"(%91) <{restrict, writable}> : (memref<128x32xf16>) -> tensor<128x32xf16>
          %93 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<32x128xf16>
          "hivm.hir.load"(%83, %93) <{eviction_policy = #hivm.eviction_policy<EvictFirst>, init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<32x128xf16, strided<[?, 1]>>, memref<32x128xf16>) -> ()
          %94 = "bufferization.to_tensor"(%93) <{restrict, writable}> : (memref<32x128xf16>) -> tensor<32x128xf16>
          %95 = "arith.cmpi"(%arg16, %8) <{predicate = 0 : i64}> : (i32, i32) -> i1
          %96 = "hivm.hir.mmadL1"(%92, %94, %95, %1, %3, %1, %arg17) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_for_result_already_inserted = true} : (tensor<128x32xf16>, tensor<32x128xf16>, i1, index, index, index, tensor<128x128xf32>) -> tensor<128x128xf32>
          %97 = "affine.apply"(%arg18) <{map = #map7}> : (index) -> index
          %98 = "arith.index_cast"(%57) : (i32) -> index
          %99 = "affine.apply"(%arg19, %98) <{map = #map3}> : (index, index) -> index
          "scf.yield"(%96, %97, %99) : (tensor<128x128xf32>, index, index) -> ()
        }) {fixpipe_for_mmad_result_already_inserted = true, tt.divisibility_arg1 = dense<16> : tensor<2xi32>, tt.divisibility_arg2 = dense<16> : tensor<2xi32>} : (i32, i32, i32, tensor<128x128xf32>, index, index) -> (tensor<128x128xf32>, index, index)
        %65 = "arith.index_cast"(%48) : (i32) -> index
        %66 = "affine.apply"(%58, %65) <{map = #map2}> : (index, index) -> index
        %67 = "affine.apply"(%66, %62) <{map = #map3}> : (index, index) -> index
        %68 = "affine.apply"(%65) <{map = #map6}> : (index) -> index
        %69 = "affine.apply"(%68) <{map = #map5}> : (index) -> index
        %70 = "arith.index_cast"(%67) : (index) -> i64
        %71 = "arith.muli"(%70, %2) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
        %72 = "arith.addi"(%51, %71) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
        %73 = "hivm.hir.pointer_cast"(%72, %69) <{operandSegmentSizes = array<i32: 1, 1>}> : (i64, index) -> memref<?xf16>
        "annotation.mark"(%73) <{effects = ["write"]}> {address_space = #hivm.address_space<gm>} : (memref<?xf16>) -> ()
        %74 = "memref.reinterpret_cast"(%73, %65) <{operandSegmentSizes = array<i32: 1, 0, 0, 1>, static_offsets = array<i64: 0>, static_sizes = array<i64: 128, 128>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xf16>, index) -> memref<128x128xf16, strided<[?, 1], offset: ?>>
        "hivm.hir.fixpipe"(%64#0, %74) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, pre_quant = #hivm.fixpipe_pre_quant_mode<F322F16>}> : (tensor<128x128xf32>, memref<128x128xf16, strided<[?, 1], offset: ?>>) -> ()
        %75 = "arith.addi"(%arg15, %7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
        "scf.yield"(%75) : (i32) -> ()
      }) : (i32) -> i32
      "scf.yield"(%45, %35) : (i32, i32) -> ()
    }) : (i32, i32, i32, i32, i32) -> (i32, i32)
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, true, true, false, false, false, false]> : vector<12xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIC>, mix_mode = "mix", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hacc.target = #hacc.target<"Ascend910B1">, hivm.module_core_type = #hivm.module_core_type<AIC>} : () -> ()

