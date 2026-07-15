"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 2 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xi64>, memref<?xi64>, memref<?xi64>, memref<?xi32>, memref<?xi32>, i32, i32, i32, i32) -> (), sym_name = "grouped_matmul_kernel"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xi64>, %arg4: memref<?xi64>, %arg5: memref<?xi64>, %arg6: memref<?xi32>, %arg7: memref<?xi32>, %arg8: i32, %arg9: i32, %arg10: i32, %arg11: i32):
    %0 = "arith.constant"() <{value = 1 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = 128 : index}> : () -> index
    %2 = "arith.constant"() <{value = 2 : i64}> : () -> i64
    %3 = "arith.constant"() <{value = 32 : index}> : () -> index
    %4 = "arith.constant"() <{value = 2 : index}> : () -> index
    %5 = "arith.constant"() <{value = 1 : index}> : () -> index
    %6 = "arith.constant"() <{value = 0 : index}> : () -> index
    %7 = "arith.constant"() <{value = 31 : i32}> : () -> i32
    %8 = "arith.constant"() <{value = 127 : i32}> : () -> i32
    %9 = "arith.constant"() <{value = 20 : i32}> : () -> i32
    %10 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %11 = "arith.constant"() <{value = 3 : i32}> : () -> i32
    %12 = "arith.constant"() <{value = 128 : i32}> : () -> i32
    %13 = "arith.constant"() <{value = 32 : i32}> : () -> i32
    "hivm.hir.set_mask_norm"() : () -> ()
    %14 = "arith.muli"(%arg9, %arg10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %15 = "arith.muli"(%14, %arg11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%15) {logical_block_num} : (i32) -> ()
    %16 = "hivm.hir.get_block_idx"() : () -> i64
    %17 = "arith.trunci"(%16) : (i64) -> i32
    %18 = "arith.muli"(%arg11, %arg10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %19 = "arith.divsi"(%17, %18) : (i32, i32) -> i32
    %20 = "arith.remsi"(%19, %arg9) : (i32, i32) -> i32
    %21:2 = "scf.for"(%10, %arg8, %0, %20, %10) ({
    ^bb0(%arg12: i32, %arg13: i32, %arg14: i32):
      %22 = "arith.muli"(%arg12, %11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %23 = "arith.index_cast"(%22) : (i32) -> index
      %24 = "memref.reinterpret_cast"(%arg6, %23) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<1xi32, strided<[1], offset: ?>>
      %25 = "memref.load"(%24, %6) : (memref<1xi32, strided<[1], offset: ?>>, index) -> i32
      %26 = "arith.addi"(%23, %5) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %27 = "memref.reinterpret_cast"(%arg6, %26) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<1xi32, strided<[1], offset: ?>>
      %28 = "memref.load"(%27, %6) : (memref<1xi32, strided<[1], offset: ?>>, index) -> i32
      %29 = "arith.addi"(%23, %4) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %30 = "memref.reinterpret_cast"(%arg6, %29) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<1xi32, strided<[1], offset: ?>>
      %31 = "memref.load"(%30, %6) : (memref<1xi32, strided<[1], offset: ?>>, index) -> i32
      %32 = "arith.addi"(%25, %8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %33 = "arith.divsi"(%32, %12) : (i32, i32) -> i32
      %34 = "arith.addi"(%28, %8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %35 = "arith.divsi"(%34, %12) : (i32, i32) -> i32
      %36 = "arith.muli"(%33, %35) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %37 = "arith.addi"(%arg14, %36) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %38 = "memref.reinterpret_cast"(%arg7, %23) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<1xi32, strided<[1], offset: ?>>
      %39 = "memref.reinterpret_cast"(%arg7, %26) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<1xi32, strided<[1], offset: ?>>
      %40 = "memref.reinterpret_cast"(%arg7, %29) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<1xi32, strided<[1], offset: ?>>
      %41 = "arith.index_cast"(%arg12) : (i32) -> index
      %42 = "memref.reinterpret_cast"(%arg3, %41) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi64>, index) -> memref<1xi64, strided<[1], offset: ?>>
      %43 = "memref.reinterpret_cast"(%arg4, %41) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi64>, index) -> memref<1xi64, strided<[1], offset: ?>>
      %44 = "memref.reinterpret_cast"(%arg5, %41) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi64>, index) -> memref<1xi64, strided<[1], offset: ?>>
      %45 = "arith.addi"(%31, %7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %46 = "arith.divsi"(%45, %13) : (i32, i32) -> i32
      %47 = "scf.while"(%arg13) ({
      ^bb0(%arg20: i32):
        %102 = "arith.cmpi"(%arg20, %arg14) <{predicate = 5 : i64}> : (i32, i32) -> i1
        %103 = "arith.cmpi"(%arg20, %37) <{predicate = 2 : i64}> : (i32, i32) -> i1
        %104 = "arith.andi"(%102, %103) : (i1, i1) -> i1
        "scf.condition"(%104, %arg20) : (i1, i32) -> ()
      }, {
      ^bb0(%arg15: i32):
        %48 = "memref.load"(%38, %6) : (memref<1xi32, strided<[1], offset: ?>>, index) -> i32
        %49 = "memref.load"(%39, %6) : (memref<1xi32, strided<[1], offset: ?>>, index) -> i32
        %50 = "memref.load"(%40, %6) : (memref<1xi32, strided<[1], offset: ?>>, index) -> i32
        %51 = "memref.load"(%42, %6) : (memref<1xi64, strided<[1], offset: ?>>, index) -> i64
        %52 = "memref.load"(%43, %6) : (memref<1xi64, strided<[1], offset: ?>>, index) -> i64
        %53 = "memref.load"(%44, %6) : (memref<1xi64, strided<[1], offset: ?>>, index) -> i64
        %54 = "arith.subi"(%arg15, %arg14) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
        %55 = "arith.divsi"(%54, %35) : (i32, i32) -> i32
        %56 = "arith.remsi"(%54, %35) : (i32, i32) -> i32
        %57 = "arith.muli"(%55, %12) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
        %58 = "arith.muli"(%56, %12) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
        %59 = "arith.muli"(%49, %13) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
        %60 = "arith.index_cast"(%57) : (i32) -> index
        %61 = "arith.index_cast"(%48) : (i32) -> index
        %62 = "arith.muli"(%60, %61) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
        %63 = "arith.index_cast"(%49) : (i32) -> index
        %64 = "arith.index_cast"(%58) : (i32) -> index
        %65 = "tensor.empty"() : () -> tensor<128x128xf32>
        %66:3 = "scf.for"(%10, %46, %0, %65, %62, %6) ({
        ^bb0(%arg16: i32, %arg17: tensor<128x128xf32>, %arg18: index, %arg19: index):
          %78 = "arith.addi"(%arg19, %64) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
          %79 = "arith.muli"(%63, %3) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
          %80 = "arith.addi"(%79, %1) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
          %81 = "arith.index_cast"(%78) : (index) -> i64
          %82 = "arith.muli"(%81, %2) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
          %83 = "arith.addi"(%52, %82) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
          %84 = "hivm.hir.pointer_cast"(%83, %80) <{operandSegmentSizes = array<i32: 1, 1>}> : (i64, index) -> memref<?xf16>
          "annotation.mark"(%84) {address_space = #hivm.address_space<gm>} : (memref<?xf16>) -> ()
          %85 = "memref.reinterpret_cast"(%84, %63) <{operandSegmentSizes = array<i32: 1, 0, 0, 1>, static_offsets = array<i64: 0>, static_sizes = array<i64: 32, 128>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xf16>, index) -> memref<32x128xf16, strided<[?, 1]>>
          %86 = "arith.muli"(%61, %1) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
          %87 = "arith.addi"(%86, %3) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
          %88 = "arith.index_cast"(%arg18) : (index) -> i64
          %89 = "arith.muli"(%88, %2) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
          %90 = "arith.addi"(%51, %89) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
          %91 = "hivm.hir.pointer_cast"(%90, %87) <{operandSegmentSizes = array<i32: 1, 1>}> : (i64, index) -> memref<?xf16>
          "annotation.mark"(%91) {address_space = #hivm.address_space<gm>} : (memref<?xf16>) -> ()
          %92 = "memref.reinterpret_cast"(%91, %61) <{operandSegmentSizes = array<i32: 1, 0, 0, 1>, static_offsets = array<i64: 0>, static_sizes = array<i64: 128, 32>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xf16>, index) -> memref<128x32xf16, strided<[?, 1]>>
          %93 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<128x32xf16>
          "hivm.hir.load"(%92, %93) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<128x32xf16, strided<[?, 1]>>, memref<128x32xf16>) -> ()
          %94 = "bufferization.to_tensor"(%93) <{restrict, writable}> : (memref<128x32xf16>) -> tensor<128x32xf16>
          %95 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<32x128xf16>
          "hivm.hir.load"(%85, %95) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<32x128xf16, strided<[?, 1]>>, memref<32x128xf16>) -> ()
          %96 = "bufferization.to_tensor"(%95) <{restrict, writable}> : (memref<32x128xf16>) -> tensor<32x128xf16>
          %97 = "arith.cmpi"(%arg16, %10) <{predicate = 0 : i64}> : (i32, i32) -> i1
          %98 = "hivm.hir.mmadL1"(%94, %96, %97, %1, %3, %1, %arg17) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_already_inserted = true} : (tensor<128x32xf16>, tensor<32x128xf16>, i1, index, index, index, tensor<128x128xf32>) -> tensor<128x128xf32>
          %99 = "arith.addi"(%arg18, %3) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
          %100 = "arith.index_cast"(%59) : (i32) -> index
          %101 = "arith.addi"(%arg19, %100) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
          "scf.yield"(%98, %99, %101) : (tensor<128x128xf32>, index, index) -> ()
        }) {tt.divisibility_arg1 = dense<16> : tensor<2xi32>, tt.divisibility_arg2 = dense<16> : tensor<2xi32>} : (i32, i32, i32, tensor<128x128xf32>, index, index) -> (tensor<128x128xf32>, index, index)
        %67 = "arith.index_cast"(%50) : (i32) -> index
        %68 = "arith.muli"(%60, %67) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
        %69 = "arith.addi"(%68, %64) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
        %70 = "arith.muli"(%67, %1) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
        %71 = "arith.addi"(%70, %1) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
        %72 = "arith.index_cast"(%69) : (index) -> i64
        %73 = "arith.muli"(%72, %2) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
        %74 = "arith.addi"(%53, %73) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
        %75 = "hivm.hir.pointer_cast"(%74, %71) <{operandSegmentSizes = array<i32: 1, 1>}> : (i64, index) -> memref<?xf16>
        "annotation.mark"(%75) {address_space = #hivm.address_space<gm>} : (memref<?xf16>) -> ()
        %76 = "memref.reinterpret_cast"(%75, %67) <{operandSegmentSizes = array<i32: 1, 0, 0, 1>, static_offsets = array<i64: 0>, static_sizes = array<i64: 128, 128>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xf16>, index) -> memref<128x128xf16, strided<[?, 1], offset: ?>>
        "hivm.hir.fixpipe"(%66#0, %76) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, pre_quant = #hivm.fixpipe_pre_quant_mode<F322F16>}> : (tensor<128x128xf32>, memref<128x128xf16, strided<[?, 1], offset: ?>>) -> ()
        %77 = "arith.addi"(%arg15, %9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
        "scf.yield"(%77) : (i32) -> ()
      }) : (i32) -> i32
      "scf.yield"(%47, %37) : (i32, i32) -> ()
    }) : (i32, i32, i32, i32, i32) -> (i32, i32)
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, true, true, false, false, false, false]> : vector<12xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIC>, mix_mode = "mix", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<AIC>} : () -> ()

