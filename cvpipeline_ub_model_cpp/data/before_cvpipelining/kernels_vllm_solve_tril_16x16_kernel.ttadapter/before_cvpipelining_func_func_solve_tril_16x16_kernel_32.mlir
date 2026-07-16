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
    "annotation.mark"(%15) {logical_block_num} : (i32) -> ()
    %16 = "hivm.hir.get_block_idx"() : () -> i64
    %17 = "arith.trunci"(%16) : (i64) -> i32
    %18 = "arith.divsi"(%17, %arg8) : (i32, i32) -> i32
    %19 = "arith.remsi"(%18, %arg7) : (i32, i32) -> i32
    %20 = "arith.muli"(%arg8, %arg7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %21 = "arith.divsi"(%17, %20) : (i32, i32) -> i32
    %22 = "arith.remsi"(%21, %arg6) : (i32, i32) -> i32
    %23 = "tensor.empty"() : () -> tensor<1x16x16xf32>
    %24 = "tensor.empty"() : () -> tensor<1x16x16xf32>
    %25 = "tensor.empty"() : () -> tensor<1x16x16xf32>
    %26 = "tensor.empty"() : () -> tensor<1x16x16xf32>
    %27 = "tensor.empty"() : () -> tensor<1x16x16xf32>
    %28 = "tensor.empty"() : () -> tensor<1x16x16xf32>
    %29 = "tensor.empty"() : () -> tensor<1x16x16xf32>
    %30 = "tensor.empty"() : () -> tensor<1x16xf32>
    %31 = "tensor.empty"() : () -> tensor<1x16xf32>
    %32 = "tensor.empty"() : () -> tensor<1x16xf32>
    %33 = "arith.divsi"(%19, %8) : (i32, i32) -> i32
    %34 = "arith.remsi"(%19, %8) : (i32, i32) -> i32
    %35 = "arith.muli"(%33, %arg5) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %36 = "arith.muli"(%35, %8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %37 = "arith.addi"(%36, %34) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %38 = "arith.muli"(%37, %7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %39 = "arith.index_cast"(%38) : (i32) -> index
    %40 = "arith.muli"(%22, %6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %41 = "tensor.empty"() : () -> tensor<16xi32>
    %42 = "hivm.hir.varange"(%41, %2, %3) <{operandSegmentSizes = array<i32: 1, 1, 1>}> : (tensor<16xi32>, index, index) -> tensor<16xi32>
    %43 = "tensor.empty"() : () -> tensor<16xf32>
    %44 = "hivm.hir.vcast"(%42, %43) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16xi32>, tensor<16xf32>) -> tensor<16xf32>
    %45 = "tensor.empty"() : () -> tensor<16x16xf32>
    %46 = "tensor.empty"() : () -> tensor<16x16xf32>
    %47 = "tensor.empty"() : () -> tensor<16x16xf32>
    %48 = "tensor.empty"() : () -> tensor<16x16xf32>
    %49 = "tensor.expand_shape"(%44) <{reassociation = [[0, 1]], static_output_shape = array<i64: 16, 1>}> : (tensor<16xf32>) -> tensor<16x1xf32>
    %50 = "hivm.hir.vbrc"(%49, %48) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %51 = "tensor.expand_shape"(%44) <{reassociation = [[0, 1]], static_output_shape = array<i64: 1, 16>}> : (tensor<16xf32>) -> tensor<1x16xf32>
    %52 = "hivm.hir.vbrc"(%51, %47) <{broadcast_dims = array<i64: 0>}> : (tensor<1x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %53 = "tensor.empty"() : () -> tensor<16x16xi1>
    %54 = "tensor.empty"() : () -> tensor<16x16xi1>
    %55 = "hivm.hir.vcmp"(%50, %52, %54) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<gt>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<16x16xf32>, tensor<16x16xf32>, tensor<16x16xi1>) -> tensor<16x16xi1>
    %56 = "hivm.hir.vcast"(%55, %46) <{broadcast = array<i64>, cast = #hivm.cast<cast_unsigned>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x16xi1>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %57 = "tensor.expand_shape"(%56) <{reassociation = [[0, 1], [2]], static_output_shape = array<i64: 1, 16, 16>}> : (tensor<16x16xf32>) -> tensor<1x16x16xf32>
    %58 = "hivm.hir.vcmp"(%50, %52, %53) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<16x16xf32>, tensor<16x16xf32>, tensor<16x16xi1>) -> tensor<16x16xi1>
    %59 = "tensor.expand_shape"(%58) <{reassociation = [[0, 1], [2]], static_output_shape = array<i64: 1, 16, 16>}> : (tensor<16x16xi1>) -> tensor<1x16x16xi1>
    %60 = "scf.for"(%5, %10, %0, %40) ({
    ^bb0(%arg9: i32, %arg10: i32):
      %61 = "arith.muli"(%arg9, %7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %62 = "arith.addi"(%arg10, %61) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %63 = "arith.remsi"(%62, %7) : (i32, i32) -> i32
      %64 = "arith.muli"(%62, %11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %65 = "arith.index_cast"(%64) : (i32) -> index
      %66 = "arith.addi"(%39, %65) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %67 = "arith.index_cast"(%63) : (i32) -> index
      %68 = "arith.addi"(%66, %67) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %69 = "memref.reinterpret_cast"(%arg3, %68) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 64, 1>}> : (memref<?xbf16>, index) -> memref<16x16xbf16, strided<[64, 1], offset: ?>>
      %70 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x16xbf16>
      %71 = "arith.index_cast"(%62) : (i32) -> index
      %72 = "arith.addi"(%71, %12) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %73 = "arith.index_cast"(%arg5) : (i32) -> index
      %74 = "arith.maxsi"(%71, %73) : (index, index) -> index
      %75 = "arith.minsi"(%72, %74) : (index, index) -> index
      %76 = "arith.subi"(%75, %71) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %77 = "arith.addi"(%67, %12) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %78 = "arith.maxsi"(%67, %12) : (index, index) -> index
      %79 = "arith.minsi"(%77, %78) : (index, index) -> index
      %80 = "arith.subi"(%79, %67) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %81 = "arith.minsi"(%76, %12) : (index, index) -> index
      %82 = "arith.minsi"(%80, %12) : (index, index) -> index
      %83 = "arith.cmpi"(%81, %12) <{predicate = 2 : i64}> : (index, index) -> i1
      %84 = "arith.cmpi"(%82, %12) <{predicate = 2 : i64}> : (index, index) -> i1
      %85 = "arith.ori"(%83, %84) : (i1, i1) -> i1
      %86 = "memref.subview"(%69, %81, %82) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16, strided<[64, 1], offset: ?>>, index, index) -> memref<?x?xbf16, strided<[64, 1], offset: ?>>
      %87 = "memref.subview"(%70, %81, %82) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16>, index, index) -> memref<?x?xbf16, strided<[16, 1]>>
      "hivm.hir.load"(%86, %87, %13, %2, %85) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x?xbf16, strided<[64, 1], offset: ?>>, memref<?x?xbf16, strided<[16, 1]>>, bf16, index, i1) -> ()
      %88 = "bufferization.to_tensor"(%70) <{restrict, writable}> : (memref<16x16xbf16>) -> tensor<16x16xbf16>
      %89 = "hivm.hir.vcast"(%88, %45) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x16xbf16>, tensor<16x16xf32>) -> tensor<16x16xf32>
      %90 = "tensor.expand_shape"(%89) <{reassociation = [[0, 1], [2]], static_output_shape = array<i64: 1, 16, 16>}> : (tensor<16x16xf32>) -> tensor<1x16x16xf32>
      %91 = "tensor.empty"() : () -> tensor<16x1x16xf32>
      %92 = "hivm.hir.vmul"(%90, %1, %29) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1x16x16xf32>, f32, tensor<1x16x16xf32>) -> tensor<1x16x16xf32>
      %93 = "hivm.hir.vadd"(%92, %4, %28) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1x16x16xf32>, f32, tensor<1x16x16xf32>) -> tensor<1x16x16xf32>
      %94 = "hivm.hir.vmul"(%93, %57, %27) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1x16x16xf32>, tensor<1x16x16xf32>, tensor<1x16x16xf32>) -> tensor<1x16x16xf32>
      %95 = "scf.for"(%0, %7, %0, %94) ({
      ^bb0(%arg11: i32, %arg12: tensor<1x16x16xf32>):
        %104 = "arith.index_cast"(%arg11) : (i32) -> index
        %105 = "tensor.extract_slice"(%89, %104) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808, 0>, static_sizes = array<i64: 1, 16>, static_strides = array<i64: 16, 1>}> : (tensor<16x16xf32>, index) -> tensor<1x16xf32>
        %106 = "hivm.hir.vmul"(%105, %1, %32) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1x16xf32>, f32, tensor<1x16xf32>) -> tensor<1x16xf32>
        %107 = "hivm.hir.vadd"(%106, %4, %31) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1x16xf32>, f32, tensor<1x16xf32>) -> tensor<1x16xf32>
        %108 = "tensor.expand_shape"(%107) <{reassociation = [[0], [1, 2]], static_output_shape = array<i64: 1, 16, 1>}> : (tensor<1x16xf32>) -> tensor<1x16x1xf32>
        %109 = "hivm.hir.vbrc"(%108, %26) <{broadcast_dims = array<i64: 2>}> : (tensor<1x16x1xf32>, tensor<1x16x16xf32>) -> tensor<1x16x16xf32>
        %110 = "hivm.hir.vmul"(%109, %arg12, %25) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1x16x16xf32>, tensor<1x16x16xf32>, tensor<1x16x16xf32>) -> tensor<1x16x16xf32>
        %111 = "hivm.hir.vtranspose"(%110, %91) <{disable_align = false, permutation = array<i64: 1, 0, 2>}> : (tensor<1x16x16xf32>, tensor<16x1x16xf32>) -> tensor<16x1x16xf32>
        %112 = "tensor.empty"() : () -> tensor<1x1x16xf32>
        %113 = "hivm.hir.vreduce"(%111, %112) <{arith = #hivm.reduce_op<sum>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 0>}> : (tensor<16x1x16xf32>, tensor<1x1x16xf32>) -> tensor<1x1x16xf32>
        %114 = "tensor.collapse_shape"(%113) <{reassociation = [[0, 1], [2]]}> : (tensor<1x1x16xf32>) -> tensor<1x16xf32>
        %115 = "hivm.hir.vadd"(%107, %114, %30) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1x16xf32>, tensor<1x16xf32>, tensor<1x16xf32>) -> tensor<1x16xf32>
        %116 = "tensor.expand_shape"(%115) <{reassociation = [[0], [1, 2]], static_output_shape = array<i64: 1, 1, 16>}> : (tensor<1x16xf32>) -> tensor<1x1x16xf32>
        %117 = "tensor.insert_slice"(%116, %arg12, %104) <{operandSegmentSizes = array<i32: 1, 1, 1, 0, 0>, static_offsets = array<i64: 0, -9223372036854775808, 0>, static_sizes = array<i64: 1, 1, 16>, static_strides = array<i64: 1, 1, 1>}> : (tensor<1x1x16xf32>, tensor<1x16x16xf32>, index) -> tensor<1x16x16xf32>
        "scf.yield"(%117) : (tensor<1x16x16xf32>) -> ()
      }) : (i32, i32, i32, tensor<1x16x16xf32>) -> tensor<1x16x16xf32>
      %96 = "hivm.hir.vadd"(%95, %9, %24) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1x16x16xf32>, f32, tensor<1x16x16xf32>) -> tensor<1x16x16xf32>
      %97 = "hivm.hir.vsel"(%59, %96, %95, %23) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<1x16x16xi1>, tensor<1x16x16xf32>, tensor<1x16x16xf32>, tensor<1x16x16xf32>) -> tensor<1x16x16xf32>
      %98 = "tensor.collapse_shape"(%97) <{reassociation = [[0, 1], [2]]}> : (tensor<1x16x16xf32>) -> tensor<16x16xf32>
      %99 = "memref.reinterpret_cast"(%arg4, %66) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 64, 1>}> : (memref<?xbf16>, index) -> memref<16x16xbf16, strided<[64, 1], offset: ?>>
      %100 = "tensor.empty"() : () -> tensor<16x16xbf16>
      %101 = "hivm.hir.vcast"(%98, %100) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x16xf32>, tensor<16x16xbf16>) -> tensor<16x16xbf16>
      %102 = "tensor.extract_slice"(%101, %76) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (tensor<16x16xbf16>, index) -> tensor<?x16xbf16>
      %103 = "memref.subview"(%99, %76) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 16>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16, strided<[64, 1], offset: ?>>, index) -> memref<?x16xbf16, strided<[64, 1], offset: ?>>
      "hivm.hir.store"(%102, %103) : (tensor<?x16xbf16>, memref<?x16xbf16, strided<[64, 1], offset: ?>>) -> ()
      "scf.yield"(%62) : (i32) -> ()
    }) : (i32, i32, i32, i32) -> i32
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, false, false, false, false]> : vector<9xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

