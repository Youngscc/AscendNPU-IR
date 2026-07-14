"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, i32, i32, i32, i32) -> (), sym_name = "chunk_scaled_dot_kkt_fwd_kernel"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xbf16>, %arg4: memref<?xbf16>, %arg5: memref<?xbf16>, %arg6: i32, %arg7: i32, %arg8: i32, %arg9: i32):
    %0 = "arith.constant"() <{value = 1 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = true}> : () -> i1
    %2 = "arith.constant"() <{value = 1 : index}> : () -> index
    %3 = "arith.constant"() <{value = 64 : index}> : () -> index
    %4 = "arith.constant"() <{value = 256 : index}> : () -> index
    %5 = "arith.constant"() <{value = 0.000000e+00 : bf16}> : () -> bf16
    %6 = "arith.constant"() <{value = 16 : index}> : () -> index
    %7 = "arith.constant"() <{value = 0 : index}> : () -> index
    %8 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %9 = "arith.constant"() <{value = 4 : i32}> : () -> i32
    %10 = "arith.constant"() <{value = 16 : i32}> : () -> i32
    %11 = "arith.constant"() <{value = 64 : i32}> : () -> i32
    %12 = "arith.constant"() <{value = 0.000000e+00 : f32}> : () -> f32
    "hivm.hir.set_mask_norm"() : () -> ()
    %13 = "arith.muli"(%arg7, %arg8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %14 = "arith.muli"(%13, %arg9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%14) {logical_block_num} : (i32) -> ()
    %15 = "hivm.hir.get_block_idx"() : () -> i64
    %16 = "arith.trunci"(%15) : (i64) -> i32
    %17 = "arith.muli"(%arg9, %arg8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %18 = "arith.divsi"(%16, %17) : (i32, i32) -> i32
    %19 = "arith.remsi"(%18, %arg7) : (i32, i32) -> i32
    %20 = "tensor.empty"() : () -> tensor<16x16xf32>
    %21 = "tensor.empty"() : () -> tensor<16x16xf32>
    %22 = "tensor.empty"() : () -> tensor<16x16xf32>
    %23 = "tensor.empty"() : () -> tensor<16x16xf32>
    %24 = "tensor.empty"() : () -> tensor<16x16xf32>
    %25 = "tensor.empty"() : () -> tensor<16xi32>
    %26 = "hivm.hir.varange"(%25, %7, %2) <{operandSegmentSizes = array<i32: 1, 1, 1>}> : (tensor<16xi32>, index, index) -> tensor<16xi32>
    %27 = "tensor.empty"() : () -> tensor<16xf32>
    %28 = "hivm.hir.vcast"(%26, %27) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16xi32>, tensor<16xf32>) -> tensor<16xf32>
    %29 = "arith.muli"(%19, %10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %30 = "tensor.expand_shape"(%28) <{reassociation = [[0, 1]], static_output_shape = array<i64: 16, 1>}> : (tensor<16xf32>) -> tensor<16x1xf32>
    %31 = "hivm.hir.vbrc"(%30, %24) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %32 = "tensor.expand_shape"(%28) <{reassociation = [[0, 1]], static_output_shape = array<i64: 1, 16>}> : (tensor<16xf32>) -> tensor<1x16xf32>
    %33 = "hivm.hir.vbrc"(%32, %23) <{broadcast_dims = array<i64: 0>}> : (tensor<1x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %34 = "tensor.empty"() : () -> tensor<16x16xi1>
    %35 = "hivm.hir.vcmp"(%31, %33, %34) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<gt>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<16x16xf32>, tensor<16x16xf32>, tensor<16x16xi1>) -> tensor<16x16xi1>
    "scf.for"(%8, %9, %0) ({
    ^bb0(%arg10: i32):
      %36 = "arith.divsi"(%arg10, %9) : (i32, i32) -> i32
      %37 = "arith.remsi"(%arg10, %9) : (i32, i32) -> i32
      %38 = "arith.muli"(%36, %arg6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %39 = "arith.muli"(%37, %arg6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %40 = "arith.index_cast"(%39) : (i32) -> index
      %41 = "arith.index_cast"(%38) : (i32) -> index
      %42 = "arith.addi"(%40, %41) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %43 = "arith.maxsi"(%29, %8) : (i32, i32) -> i32
      %44 = "arith.index_cast"(%43) : (i32) -> index
      %45 = "arith.addi"(%44, %42) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %46 = "arith.index_cast"(%arg6) : (i32) -> index
      %47 = "memref.reinterpret_cast"(%arg4, %45) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 1>, static_strides = array<i64: 1, 1>}> : (memref<?xbf16>, index) -> memref<16x1xbf16, strided<[1, 1], offset: ?>>
      %48 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x1xbf16>
      %49 = "memref.collapse_shape"(%48) <{reassociation = [[0, 1]]}> : (memref<16x1xbf16>) -> memref<16xbf16>
      %50 = "arith.subi"(%46, %44) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %51 = "arith.maxsi"(%50, %7) : (index, index) -> index
      %52 = "arith.minsi"(%51, %6) : (index, index) -> index
      %53 = "arith.subi"(%8, %29) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %54 = "arith.maxsi"(%53, %8) : (i32, i32) -> i32
      %55 = "arith.index_cast"(%54) : (i32) -> index
      %56 = "arith.minsi"(%55, %52) : (index, index) -> index
      %57 = "arith.subi"(%52, %56) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %58 = "arith.cmpi"(%57, %6) <{predicate = 2 : i64}> : (index, index) -> i1
      "scf.if"(%58) ({
        "hivm.hir.vbrc"(%5, %49) <{broadcast_dims = array<i64>}> : (bf16, memref<16xbf16>) -> ()
        "scf.yield"() : () -> ()
      }, {
      }) {hivm.unlikely_condition} : (i1) -> ()
      %59 = "memref.subview"(%47, %57) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 1>, static_strides = array<i64: 1, 1>}> : (memref<16x1xbf16, strided<[1, 1], offset: ?>>, index) -> memref<?x1xbf16, strided<[1, 1], offset: ?>>
      %60 = "memref.subview"(%48, %56, %57) <{operandSegmentSizes = array<i32: 1, 1, 1, 0>, static_offsets = array<i64: -9223372036854775808, 0>, static_sizes = array<i64: -9223372036854775808, 1>, static_strides = array<i64: 1, 1>}> : (memref<16x1xbf16>, index, index) -> memref<?x1xbf16, strided<[1, 1], offset: ?>>
      "hivm.hir.load"(%59, %60, %5, %7) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 0>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x1xbf16, strided<[1, 1], offset: ?>>, memref<?x1xbf16, strided<[1, 1], offset: ?>>, bf16, index) -> ()
      %61 = "bufferization.to_tensor"(%48) <{restrict, writable}> : (memref<16x1xbf16>) -> tensor<16x1xbf16>
      %62 = "arith.muli"(%38, %9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %63 = "arith.addi"(%62, %37) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %64 = "arith.muli"(%63, %11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %65 = "arith.index_cast"(%64) : (i32) -> index
      %66 = "arith.muli"(%44, %4) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %67 = "arith.addi"(%66, %65) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %68 = "memref.reinterpret_cast"(%arg3, %67) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 64>, static_strides = array<i64: 256, 1>}> : (memref<?xbf16>, index) -> memref<16x64xbf16, strided<[256, 1], offset: ?>>
      %69 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x64xbf16>
      %70 = "arith.divsi"(%66, %4) : (index, index) -> index
      %71 = "arith.subi"(%46, %70) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %72 = "arith.maxsi"(%71, %7) : (index, index) -> index
      %73 = "arith.minsi"(%72, %6) : (index, index) -> index
      %74 = "arith.remsi"(%66, %4) : (index, index) -> index
      %75 = "arith.subi"(%3, %74) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %76 = "arith.maxsi"(%75, %7) : (index, index) -> index
      %77 = "arith.minsi"(%76, %3) : (index, index) -> index
      %78 = "arith.minsi"(%55, %73) : (index, index) -> index
      %79 = "arith.subi"(%73, %78) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %80 = "arith.minsi"(%77, %7) : (index, index) -> index
      %81 = "arith.subi"(%77, %80) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %82 = "arith.cmpi"(%79, %6) <{predicate = 2 : i64}> : (index, index) -> i1
      %83 = "arith.cmpi"(%81, %3) <{predicate = 2 : i64}> : (index, index) -> i1
      %84 = "arith.ori"(%82, %83) : (i1, i1) -> i1
      %85 = "memref.subview"(%68, %79, %81) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x64xbf16, strided<[256, 1], offset: ?>>, index, index) -> memref<?x?xbf16, strided<[256, 1], offset: ?>>
      %86 = "memref.subview"(%69, %78, %80, %79, %81) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x64xbf16>, index, index, index, index) -> memref<?x?xbf16, strided<[64, 1], offset: ?>>
      "hivm.hir.load"(%85, %86, %5, %80, %84) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<?x?xbf16, strided<[256, 1], offset: ?>>, memref<?x?xbf16, strided<[64, 1], offset: ?>>, bf16, index, i1) -> ()
      %87 = "bufferization.to_tensor"(%69) <{restrict, writable}> : (memref<16x64xbf16>) -> tensor<16x64xbf16>
      %88 = "tensor.empty"() : () -> tensor<16x16xf32>
      %89 = "hivm.hir.mmadL1"(%87, %87, %1, %79, %81, %79, %88) <{b_transpose, operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_already_inserted = true} : (tensor<16x64xbf16>, tensor<16x64xbf16>, i1, index, index, index, tensor<16x16xf32>) -> tensor<16x16xf32>
      %90 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xf32>
      %91 = "bufferization.to_tensor"(%90) <{restrict, writable}> : (memref<16x16xf32>) -> tensor<16x16xf32>
      %92 = "hivm.hir.fixpipe"(%89, %91) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
      %93 = "tensor.empty"() : () -> tensor<16x16xf32>
      %94 = "hivm.hir.load"(%92, %93) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
      %95 = "tensor.empty"() : () -> tensor<16x1xf32>
      %96 = "hivm.hir.vcast"(%61, %95) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x1xbf16>, tensor<16x1xf32>) -> tensor<16x1xf32>
      %97 = "hivm.hir.vbrc"(%96, %22) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
      %98 = "hivm.hir.vmul"(%94, %97, %21) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x16xf32>, tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
      %99 = "hivm.hir.vsel"(%35, %98, %12, %20) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<16x16xi1>, tensor<16x16xf32>, f32, tensor<16x16xf32>) -> tensor<16x16xf32>
      %100 = "arith.muli"(%63, %10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %101 = "arith.index_cast"(%100) : (i32) -> index
      %102 = "arith.muli"(%44, %3) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %103 = "arith.addi"(%102, %101) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %104 = "memref.reinterpret_cast"(%arg5, %103) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 64, 1>}> : (memref<?xbf16>, index) -> memref<16x16xbf16, strided<[64, 1], offset: ?>>
      %105 = "tensor.empty"() : () -> tensor<16x16xbf16>
      %106 = "hivm.hir.vcast"(%99, %105) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x16xf32>, tensor<16x16xbf16>) -> tensor<16x16xbf16>
      %107 = "arith.divsi"(%102, %3) : (index, index) -> index
      %108 = "arith.subi"(%46, %107) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %109 = "arith.maxsi"(%108, %7) : (index, index) -> index
      %110 = "arith.minsi"(%109, %6) : (index, index) -> index
      %111 = "arith.remsi"(%102, %3) : (index, index) -> index
      %112 = "arith.subi"(%6, %111) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %113 = "arith.maxsi"(%112, %7) : (index, index) -> index
      %114 = "arith.minsi"(%113, %6) : (index, index) -> index
      %115 = "arith.minsi"(%55, %110) : (index, index) -> index
      %116 = "arith.subi"(%110, %115) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %117 = "arith.minsi"(%114, %7) : (index, index) -> index
      %118 = "arith.subi"(%114, %117) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %119 = "tensor.extract_slice"(%106, %115, %117, %116, %118) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<16x16xbf16>, index, index, index, index) -> tensor<?x?xbf16>
      %120 = "memref.subview"(%104, %116, %118) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16, strided<[64, 1], offset: ?>>, index, index) -> memref<?x?xbf16, strided<[64, 1], offset: ?>>
      "hivm.hir.store"(%119, %120) : (tensor<?x?xbf16>, memref<?x?xbf16, strided<[64, 1], offset: ?>>) -> ()
      "scf.yield"() : () -> ()
    }) : (i32, i32, i32) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, false, false, false, false]> : vector<10xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<MIX>, mix_mode = "mix", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<MIX>} : () -> ()

