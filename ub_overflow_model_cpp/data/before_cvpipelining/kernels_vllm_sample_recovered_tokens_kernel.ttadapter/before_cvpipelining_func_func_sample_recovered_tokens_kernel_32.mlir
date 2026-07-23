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
    %7 = "arith.constant"() <{value = -1 : index}> : () -> index
    %8 = "arith.constant"() <{value = 15 : i32}> : () -> i32
    %9 = "arith.constant"() <{value = -1.000000e+00 : f32}> : () -> f32
    %10 = "arith.constant"() <{value = -1 : i32}> : () -> i32
    %11 = "arith.constant"() <{value = 16 : i32}> : () -> i32
    %12 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %13 = "arith.constant"() <{value = 0.000000e+00 : f32}> : () -> f32
    "hivm.hir.set_mask_norm"() : () -> ()
    %14 = "arith.muli"(%arg9, %arg10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %15 = "arith.muli"(%14, %arg11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%15) {logical_block_num} : (i32) -> ()
    %16 = "hivm.hir.get_block_idx"() : () -> i64
    %17 = "arith.trunci"(%16) : (i64) -> i32
    %18 = "arith.divsi"(%17, %arg11) : (i32, i32) -> i32
    %19 = "arith.remsi"(%18, %arg10) : (i32, i32) -> i32
    %20 = "arith.muli"(%arg11, %arg10) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %21 = "arith.divsi"(%17, %20) : (i32, i32) -> i32
    %22 = "arith.remsi"(%21, %arg9) : (i32, i32) -> i32
    %23 = "tensor.empty"() : () -> tensor<1xf32>
    %24 = "tensor.empty"() : () -> tensor<1xf32>
    %25 = "hivm.hir.vbrc"(%13, %23) <{broadcast_dims = array<i64>}> : (f32, tensor<1xf32>) -> tensor<1xf32>
    %26 = "arith.cmpi"(%22, %12) <{predicate = 0 : i64}> : (i32, i32) -> i1
    %27 = "scf.if"(%26) ({
      "scf.yield"(%12) : (i32) -> ()
    }, {
      %124 = "arith.index_cast"(%22) : (i32) -> index
      %125 = "arith.addi"(%124, %7) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %126 = "memref.reinterpret_cast"(%arg4, %125) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<1xi32, strided<[1], offset: ?>>
      %127 = "memref.load"(%126, %6) : (memref<1xi32, strided<[1], offset: ?>>, index) -> i32
      "scf.yield"(%127) : (i32) -> ()
    }) : (i1) -> i32
    %28 = "arith.index_cast"(%22) : (i32) -> index
    %29 = "memref.reinterpret_cast"(%arg4, %28) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<1xi32, strided<[1], offset: ?>>
    %30 = "memref.load"(%29, %6) : (memref<1xi32, strided<[1], offset: ?>>, index) -> i32
    %31 = "arith.subi"(%30, %27) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %32 = "arith.cmpi"(%19, %31) <{predicate = 5 : i64}> : (i32, i32) -> i1
    "scf.if"(%32) ({
      "scf.yield"() : () -> ()
    }, {
      %33 = "arith.addi"(%arg8, %8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %34 = "arith.divsi"(%33, %11) : (i32, i32) -> i32
      %35 = "arith.index_cast"(%27) : (i32) -> index
      %36 = "arith.index_cast"(%19) : (i32) -> index
      %37 = "arith.addi"(%35, %36) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %38 = "memref.reinterpret_cast"(%arg5, %37) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<1xi32, strided<[1], offset: ?>>
      %39 = "memref.load"(%38, %6) : (memref<1xi32, strided<[1], offset: ?>>, index) -> i32
      %40 = "arith.addi"(%27, %19) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %41 = "arith.muli"(%40, %arg8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %42 = "arith.index_cast"(%41) : (i32) -> index
      %43 = "arith.index_cast"(%39) : (i32) -> index
      %44 = "arith.addi"(%42, %43) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %45 = "memref.reinterpret_cast"(%arg6, %44) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<1xf32, strided<[1], offset: ?>>
      %46 = "memref.load"(%45, %6) : (memref<1xf32, strided<[1], offset: ?>>, index) -> f32
      "hivm.hir.store"(%25, %45) : (tensor<1xf32>, memref<1xf32, strided<[1], offset: ?>>) -> ()
      %47 = "arith.muli"(%22, %arg8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %48 = "arith.index_cast"(%47) : (i32) -> index
      %49:2 = "scf.for"(%12, %34, %0, %10, %9) ({
      ^bb0(%arg12: i32, %arg13: i32, %arg14: f32):
        %54 = "arith.muli"(%arg12, %11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
        %55 = "arith.index_cast"(%54) : (i32) -> index
        %56 = "arith.addi"(%42, %55) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
        %57 = "memref.reinterpret_cast"(%arg6, %56) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<16xf32, strided<[1], offset: ?>>
        %58 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16xf32>
        %59 = "arith.addi"(%55, %5) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
        %60 = "arith.index_cast"(%arg8) : (i32) -> index
        %61 = "arith.maxsi"(%55, %60) : (index, index) -> index
        %62 = "arith.minsi"(%59, %61) : (index, index) -> index
        %63 = "arith.subi"(%62, %55) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
        %64 = "arith.cmpi"(%63, %5) <{predicate = 2 : i64}> : (index, index) -> i1
        %65 = "memref.subview"(%57, %63) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<16xf32, strided<[1], offset: ?>>, index) -> memref<?xf32, strided<[1], offset: ?>>
        %66 = "memref.subview"(%58, %63) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<16xf32>, index) -> memref<?xf32, strided<[1]>>
        "hivm.hir.load"(%65, %66, %13, %6, %64) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xf32, strided<[1], offset: ?>>, memref<?xf32, strided<[1]>>, f32, index, i1) -> ()
        %67 = "bufferization.to_tensor"(%58) <{restrict, writable}> : (memref<16xf32>) -> tensor<16xf32>
        %68 = "arith.addi"(%48, %55) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
        %69 = "memref.reinterpret_cast"(%arg7, %68) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<16xf32, strided<[1], offset: ?>>
        %70 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16xf32>
        %71 = "memref.subview"(%69, %63) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<16xf32, strided<[1], offset: ?>>, index) -> memref<?xf32, strided<[1], offset: ?>>
        %72 = "memref.subview"(%70, %63) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<16xf32>, index) -> memref<?xf32, strided<[1]>>
        "hivm.hir.load"(%71, %72, %4, %6, %64) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xf32, strided<[1], offset: ?>>, memref<?xf32, strided<[1]>>, f32, index, i1) -> ()
        %73 = "bufferization.to_tensor"(%70) <{restrict, writable}> : (memref<16xf32>) -> tensor<16xf32>
        %74 = "tensor.empty"() : () -> tensor<16xf32>
        %75 = "tensor.empty"() : () -> tensor<16xf32>
        %76 = "hivm.hir.vdiv"(%67, %73, %75) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        %77 = "tensor.empty"() : () -> tensor<f32>
        %78 = "tensor.expand_shape"(%77) <{reassociation = [], static_output_shape = array<i64: 1>}> : (tensor<f32>) -> tensor<1xf32>
        %79 = "tensor.expand_shape"(%77) <{reassociation = [], static_output_shape = array<i64: 1>}> : (tensor<f32>) -> tensor<1xf32>
        %80 = "tensor.empty"() : () -> tensor<i32>
        %81 = "tensor.expand_shape"(%80) <{reassociation = [], static_output_shape = array<i64: 1>}> : (tensor<i32>) -> tensor<1xi32>
        %82 = "tensor.expand_shape"(%80) <{reassociation = [], static_output_shape = array<i64: 1>}> : (tensor<i32>) -> tensor<1xi32>
        %83 = "tensor.expand_shape"(%80) <{reassociation = [], static_output_shape = array<i64: 1>}> : (tensor<i32>) -> tensor<1xi32>
        %84 = "tensor.expand_shape"(%80) <{reassociation = [], static_output_shape = array<i64: 1>}> : (tensor<i32>) -> tensor<1xi32>
        %85 = "tensor.empty"() : () -> tensor<1xf32>
        %86 = "tensor.empty"() : () -> tensor<1xi32>
        %87:2 = "hivm.hir.vreduce"(%76, %85, %86) <{arith = #hivm.reduce_op<max_with_index_left>, operandSegmentSizes = array<i32: 1, 2, 0, 0>, reduce_dims = array<i64: 0>}> : (tensor<16xf32>, tensor<1xf32>, tensor<1xi32>) -> (tensor<1xf32>, tensor<1xi32>)
        %88 = "tensor.empty"() : () -> tensor<16xi1>
        %89 = "tensor.empty"() : () -> tensor<16xi1>
        %90 = "hivm.hir.vcmp"(%76, %76, %89) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xi1>) -> tensor<16xi1>
        %91 = "hivm.hir.vnot"(%90, %88) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xi1>, tensor<16xi1>) -> tensor<16xi1>
        %92 = "hivm.hir.vsel"(%91, %2, %13, %74) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<16xi1>, f32, f32, tensor<16xf32>) -> tensor<16xf32>
        %93 = "tensor.expand_shape"(%77) <{reassociation = [], static_output_shape = array<i64: 1>}> : (tensor<f32>) -> tensor<1xf32>
        %94 = "tensor.expand_shape"(%80) <{reassociation = [], static_output_shape = array<i64: 1>}> : (tensor<i32>) -> tensor<1xi32>
        %95:2 = "hivm.hir.vreduce"(%92, %93, %94) <{arith = #hivm.reduce_op<max_with_index_left>, operandSegmentSizes = array<i32: 1, 2, 0, 0>, reduce_dims = array<i64: 0>}> : (tensor<16xf32>, tensor<1xf32>, tensor<1xi32>) -> (tensor<1xf32>, tensor<1xi32>)
        %96 = "hivm.hir.bitcast"(%95#0) : (tensor<1xf32>) -> tensor<1xi32>
        %97 = "tensor.empty"() : () -> tensor<i32>
        %98 = "tensor.expand_shape"(%97) <{reassociation = [], static_output_shape = array<i64: 1>}> : (tensor<i32>) -> tensor<1xi32>
        %99 = "hivm.hir.vbrc"(%3, %98) <{broadcast_dims = array<i64>}> : (i32, tensor<1xi32>) -> tensor<1xi32>
        %100 = "hivm.hir.vand"(%96, %99, %81) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1xi32>, tensor<1xi32>, tensor<1xi32>) -> tensor<1xi32>
        %101 = "hivm.hir.vadd"(%100, %1, %82) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1xi32>, i32, tensor<1xi32>) -> tensor<1xi32>
        %102 = "hivm.hir.bitcast"(%101) : (tensor<1xi32>) -> tensor<1xf32>
        %103 = "hivm.hir.vabs"(%102, %78) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<1xf32>, tensor<1xf32>) -> tensor<1xf32>
        %104 = "hivm.hir.bitcast"(%103) : (tensor<1xf32>) -> tensor<1xi32>
        %105 = "hivm.hir.vmin"(%104, %0, %83) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1xi32>, i32, tensor<1xi32>) -> tensor<1xi32>
        %106 = "hivm.hir.vmul"(%105, %10, %105) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1xi32>, i32, tensor<1xi32>) -> tensor<1xi32>
        %107 = "hivm.hir.vadd"(%106, %0, %106) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1xi32>, i32, tensor<1xi32>) -> tensor<1xi32>
        %108 = "hivm.hir.vcast"(%107, %79) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<1xi32>, tensor<1xf32>) -> tensor<1xf32>
        %109 = "tensor.empty"() : () -> tensor<i1>
        %110 = "tensor.expand_shape"(%109) <{reassociation = [], static_output_shape = array<i64: 1>}> : (tensor<i1>) -> tensor<1xi1>
        %111 = "tensor.expand_shape"(%109) <{reassociation = [], static_output_shape = array<i64: 1>}> : (tensor<i1>) -> tensor<1xi1>
        %112 = "hivm.hir.vcmp"(%108, %13, %110) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<1xf32>, f32, tensor<1xi1>) -> tensor<1xi1>
        %113 = "hivm.hir.vnot"(%112, %111) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<1xi1>, tensor<1xi1>) -> tensor<1xi1>
        %114 = "hivm.hir.vsel"(%113, %95#1, %87#1, %84) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<1xi1>, tensor<1xi32>, tensor<1xi32>, tensor<1xi32>) -> tensor<1xi32>
        %115 = "tensor.extract"(%114, %6) : (tensor<1xi32>, index) -> i32
        %116 = "arith.index_cast"(%115) : (i32) -> index
        %117 = "tensor.extract"(%67, %116) {DiscreteMemAccess} : (tensor<16xf32>, index) -> f32
        %118 = "tensor.extract"(%73, %116) {DiscreteMemAccess} : (tensor<16xf32>, index) -> f32
        %119 = "arith.divf"(%117, %118) <{fastmath = #arith.fastmath<none>}> : (f32, f32) -> f32
        %120 = "arith.cmpf"(%119, %arg14) <{fastmath = #arith.fastmath<none>, predicate = 2 : i64}> : (f32, f32) -> i1
        %121 = "arith.select"(%120, %119, %arg14) : (i1, f32, f32) -> f32
        %122 = "scf.if"(%120) ({
          %123 = "arith.addi"(%54, %115) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
          "scf.yield"(%123) : (i32) -> ()
        }, {
          "scf.yield"(%arg13) : (i32) -> ()
        }) : (i1) -> i32
        "scf.yield"(%122, %121) : (i32, f32) -> ()
      }) : (i32, i32, i32, i32, f32) -> (i32, f32)
      %50 = "tensor.empty"() : () -> tensor<1xi32>
      %51 = "tensor.insert"(%49#0, %50, %6) : (i32, tensor<1xi32>, index) -> tensor<1xi32>
      %52 = "memref.reinterpret_cast"(%arg3, %37) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<1xi32, strided<[1], offset: ?>>
      "hivm.hir.store"(%51, %52) : (tensor<1xi32>, memref<1xi32, strided<[1], offset: ?>>) -> ()
      %53 = "tensor.insert"(%46, %24, %6) : (f32, tensor<1xf32>, index) -> tensor<1xf32>
      "hivm.hir.store"(%53, %45) : (tensor<1xf32>, memref<1xf32, strided<[1], offset: ?>>) -> ()
      "scf.yield"() : () -> ()
    }) : (i1) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, true, true, false, false, false, false]> : vector<12xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

