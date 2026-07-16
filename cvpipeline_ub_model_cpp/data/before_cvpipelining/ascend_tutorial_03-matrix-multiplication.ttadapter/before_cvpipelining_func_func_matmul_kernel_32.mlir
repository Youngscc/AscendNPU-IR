"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xf16>, memref<?xf16>, memref<?xf16>, i32, i32, i32, i32, i32, i32, i32, i32, i32) -> (), sym_name = "matmul_kernel"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xf16>, %arg4: memref<?xf16>, %arg5: memref<?xf16>, %arg6: i32, %arg7: i32, %arg8: i32, %arg9: i32, %arg10: i32, %arg11: i32, %arg12: i32, %arg13: i32, %arg14: i32):
    %0 = "arith.constant"() <{value = 1 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = 32 : index}> : () -> index
    %2 = "arith.constant"() <{value = 0.000000e+00 : f16}> : () -> f16
    %3 = "arith.constant"() <{value = 0 : index}> : () -> index
    %4 = "arith.constant"() <{value = 64 : index}> : () -> index
    %5 = "arith.constant"() <{value = 0.00999999977 : f32}> : () -> f32
    %6 = "arith.constant"() <{value = 1.000000e+00 : f32}> : () -> f32
    %7 = "arith.constant"() <{value = 64 : i32}> : () -> i32
    %8 = "arith.constant"() <{value = 32 : i32}> : () -> i32
    %9 = "arith.constant"() <{value = 63 : i32}> : () -> i32
    %10 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %11 = "arith.constant"() <{value = 2 : i32}> : () -> i32
    %12 = "arith.constant"() <{value = 0.000000e+00 : f32}> : () -> f32
    "hivm.hir.set_mask_norm"() : () -> ()
    %13 = "arith.muli"(%arg12, %arg13) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %14 = "arith.muli"(%13, %arg14) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%14) {logical_block_num} : (i32) -> ()
    %15 = "hivm.hir.get_block_idx"() : () -> i64
    %16 = "arith.trunci"(%15) : (i64) -> i32
    %17 = "arith.muli"(%arg14, %arg13) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %18 = "arith.divsi"(%16, %17) : (i32, i32) -> i32
    %19 = "arith.remsi"(%18, %arg12) : (i32, i32) -> i32
    %20 = "tensor.empty"() : () -> tensor<32x64xf32>
    %21 = "tensor.empty"() : () -> tensor<32x64xf32>
    %22 = "tensor.empty"() : () -> tensor<32x64xf32>
    %23 = "tensor.empty"() : () -> tensor<32x64xf32>
    %24 = "hivm.hir.vbrc"(%12, %23) <{broadcast_dims = array<i64>}> : (f32, tensor<32x64xf32>) -> tensor<32x64xf32>
    %25 = "arith.addi"(%arg6, %9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %26 = "arith.divsi"(%25, %7) : (i32, i32) -> i32
    %27 = "arith.addi"(%arg7, %9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %28 = "arith.divsi"(%27, %7) : (i32, i32) -> i32
    %29 = "arith.divsi"(%19, %28) : (i32, i32) -> i32
    %30 = "arith.subi"(%26, %29) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %31 = "arith.minsi"(%30, %0) : (i32, i32) -> i32
    %32 = "arith.remsi"(%19, %28) : (i32, i32) -> i32
    %33 = "arith.remsi"(%32, %31) : (i32, i32) -> i32
    %34 = "arith.addi"(%29, %33) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %35 = "arith.divsi"(%32, %31) : (i32, i32) -> i32
    %36 = "arith.muli"(%34, %7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %37 = "arith.muli"(%35, %7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %38 = "arith.index_cast"(%36) : (i32) -> index
    %39 = "arith.index_cast"(%arg9) : (i32) -> index
    %40 = "arith.muli"(%38, %39) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %41 = "arith.index_cast"(%arg10) : (i32) -> index
    %42 = "arith.index_cast"(%37) : (i32) -> index
    %43 = "arith.addi"(%arg8, %9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %44 = "arith.divsi"(%43, %7) : (i32, i32) -> i32
    %45 = "tensor.empty"() : () -> tensor<64x64xf32>
    %46 = "scf.for"(%10, %44, %0, %45) ({
    ^bb0(%arg16: i32, %arg17: tensor<64x64xf32>):
      %82 = "arith.muli"(%arg16, %7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %83 = "arith.index_cast"(%82) : (i32) -> index
      %84 = "arith.addi"(%40, %83) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %85 = "memref.reinterpret_cast"(%arg3, %84, %39) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64, 64>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xf16>, index, index) -> memref<64x64xf16, strided<[?, 1], offset: ?>>
      %86 = "arith.muli"(%82, %arg10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %87 = "arith.index_cast"(%86) : (i32) -> index
      %88 = "arith.addi"(%87, %42) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %89 = "memref.reinterpret_cast"(%arg4, %88, %41) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64, 64>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xf16>, index, index) -> memref<64x64xf16, strided<[?, 1], offset: ?>>
      %90 = "arith.subi"(%arg8, %82) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %91 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<64x64xf16>
      %92 = "arith.addi"(%38, %4) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %93 = "arith.index_cast"(%arg6) : (i32) -> index
      %94 = "arith.maxsi"(%38, %93) : (index, index) -> index
      %95 = "arith.minsi"(%92, %94) : (index, index) -> index
      %96 = "arith.subi"(%95, %38) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %97 = "arith.index_cast"(%90) : (i32) -> index
      %98 = "arith.maxsi"(%97, %3) : (index, index) -> index
      %99 = "arith.minsi"(%98, %4) : (index, index) -> index
      %100 = "arith.minsi"(%96, %4) : (index, index) -> index
      %101 = "arith.minsi"(%99, %4) : (index, index) -> index
      %102 = "arith.cmpi"(%100, %4) <{predicate = 2 : i64}> : (index, index) -> i1
      %103 = "arith.cmpi"(%101, %4) <{predicate = 2 : i64}> : (index, index) -> i1
      %104 = "arith.ori"(%102, %103) : (i1, i1) -> i1
      %105 = "memref.subview"(%85, %100, %101) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<64x64xf16, strided<[?, 1], offset: ?>>, index, index) -> memref<?x?xf16, strided<[?, 1], offset: ?>>
      %106 = "memref.subview"(%91, %100, %101) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<64x64xf16>, index, index) -> memref<?x?xf16, strided<[64, 1]>>
      "hivm.hir.load"(%105, %106, %2, %3, %104) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<?x?xf16, strided<[?, 1], offset: ?>>, memref<?x?xf16, strided<[64, 1]>>, f16, index, i1) -> ()
      %107 = "bufferization.to_tensor"(%91) <{restrict, writable}> : (memref<64x64xf16>) -> tensor<64x64xf16>
      %108 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<64x64xf16>
      %109 = "arith.addi"(%42, %4) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %110 = "arith.index_cast"(%arg7) : (i32) -> index
      %111 = "arith.maxsi"(%42, %110) : (index, index) -> index
      %112 = "arith.minsi"(%109, %111) : (index, index) -> index
      %113 = "arith.subi"(%112, %42) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %114 = "arith.minsi"(%113, %4) : (index, index) -> index
      %115 = "arith.cmpi"(%114, %4) <{predicate = 2 : i64}> : (index, index) -> i1
      %116 = "arith.ori"(%103, %115) : (i1, i1) -> i1
      %117 = "memref.subview"(%89, %101, %114) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<64x64xf16, strided<[?, 1], offset: ?>>, index, index) -> memref<?x?xf16, strided<[?, 1], offset: ?>>
      %118 = "memref.subview"(%108, %101, %114) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<64x64xf16>, index, index) -> memref<?x?xf16, strided<[64, 1]>>
      "hivm.hir.load"(%117, %118, %2, %3, %116) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<?x?xf16, strided<[?, 1], offset: ?>>, memref<?x?xf16, strided<[64, 1]>>, f16, index, i1) -> ()
      %119 = "bufferization.to_tensor"(%108) <{restrict, writable}> : (memref<64x64xf16>) -> tensor<64x64xf16>
      %120 = "arith.cmpi"(%arg16, %10) <{predicate = 0 : i64}> : (i32, i32) -> i1
      %121 = "hivm.hir.mmadL1"(%107, %119, %120, %100, %101, %114, %arg17) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_already_inserted = true} : (tensor<64x64xf16>, tensor<64x64xf16>, i1, index, index, index, tensor<64x64xf32>) -> tensor<64x64xf32>
      "scf.yield"(%121) : (tensor<64x64xf32>) -> ()
    }) : (i32, i32, i32, tensor<64x64xf32>) -> tensor<64x64xf32>
    %47 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<64x64xf32>
    %48 = "bufferization.to_tensor"(%47) <{restrict, writable}> : (memref<64x64xf32>) -> tensor<64x64xf32>
    %49 = "hivm.hir.fixpipe"(%46, %48) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<64x64xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
    %50 = "tensor.empty"() : () -> tensor<64x64xf32>
    %51 = "hivm.hir.load"(%49, %50) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<64x64xf32>, tensor<64x64xf32>) -> tensor<64x64xf32>
    "scf.for"(%10, %11, %0) ({
    ^bb0(%arg15: i32):
      %52 = "arith.muli"(%arg15, %8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %53 = "arith.index_cast"(%52) : (i32) -> index
      %54 = "tensor.extract_slice"(%51, %53) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808, 0>, static_sizes = array<i64: 32, 64>, static_strides = array<i64: 1, 1>}> : (tensor<64x64xf32>, index) -> tensor<32x64xf32>
      %55 = "tensor.empty"() : () -> tensor<32x64xi1>
      %56 = "hivm.hir.vcmp"(%54, %24, %55) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<ge>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<32x64xf32>, tensor<32x64xf32>, tensor<32x64xi1>) -> tensor<32x64xi1>
      %57 = "hivm.hir.vmul"(%54, %5, %22) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32x64xf32>, f32, tensor<32x64xf32>) -> tensor<32x64xf32>
      %58 = "hivm.hir.vsel"(%56, %54, %57, %21) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<32x64xi1>, tensor<32x64xf32>, tensor<32x64xf32>, tensor<32x64xf32>) -> tensor<32x64xf32>
      %59 = "hivm.hir.vadd"(%58, %6, %20) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32x64xf32>, f32, tensor<32x64xf32>) -> tensor<32x64xf32>
      %60 = "tensor.empty"() : () -> tensor<32x64xf16>
      %61 = "hivm.hir.vcast"(%59, %60) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<32x64xf32>, tensor<32x64xf16>) -> tensor<32x64xf16>
      %62 = "arith.addi"(%36, %52) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %63 = "arith.index_cast"(%arg11) : (i32) -> index
      %64 = "arith.index_cast"(%62) : (i32) -> index
      %65 = "arith.muli"(%64, %63) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %66 = "arith.addi"(%65, %42) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %67 = "memref.reinterpret_cast"(%arg5, %66, %63) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 32, 64>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xf16>, index, index) -> memref<32x64xf16, strided<[?, 1], offset: ?>>
      %68 = "arith.addi"(%64, %1) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %69 = "arith.index_cast"(%arg6) : (i32) -> index
      %70 = "arith.maxsi"(%64, %69) : (index, index) -> index
      %71 = "arith.minsi"(%68, %70) : (index, index) -> index
      %72 = "arith.subi"(%71, %64) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %73 = "arith.addi"(%42, %4) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %74 = "arith.index_cast"(%arg7) : (i32) -> index
      %75 = "arith.maxsi"(%42, %74) : (index, index) -> index
      %76 = "arith.minsi"(%73, %75) : (index, index) -> index
      %77 = "arith.subi"(%76, %42) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %78 = "arith.minsi"(%72, %1) : (index, index) -> index
      %79 = "arith.minsi"(%77, %4) : (index, index) -> index
      %80 = "tensor.extract_slice"(%61, %78, %79) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<32x64xf16>, index, index) -> tensor<?x?xf16>
      %81 = "memref.subview"(%67, %78, %79) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<32x64xf16, strided<[?, 1], offset: ?>>, index, index) -> memref<?x?xf16, strided<[?, 1], offset: ?>>
      "hivm.hir.store"(%80, %81) : (tensor<?x?xf16>, memref<?x?xf16, strided<[?, 1], offset: ?>>) -> ()
      "scf.yield"() : () -> ()
    }) {hivm.parallel_loop} : (i32, i32, i32) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, false, false, false, false, false, false, false, false, false]> : vector<15xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<MIX>, mix_mode = "mix", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<MIX>} : () -> ()

