#map = affine_map<()[s0] -> (s0 + 64)>
#map1 = affine_map<()[s0, s1] -> (s0, s1)>
#map2 = affine_map<()[s0, s1] -> (s0 - s1)>
#map3 = affine_map<()[s0] -> (s0 + 128)>
#map4 = affine_map<()[s0] -> (s0, 0)>
#map5 = affine_map<()[s0] -> (s0, 32)>
#map6 = affine_map<()[s0, s1] -> (s0 * s1)>
#map7 = affine_map<()[s0, s1] -> (s0 + s1)>
#map8 = affine_map<()[s0] -> (s0, 64)>
#map9 = affine_map<()[s0] -> (s0, 128)>
"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xf16>, memref<?xf16>, memref<?xf16>, i32, i32, i32, i32, i32, i32, i32, i32, i32) -> (), sym_name = "matmul_kernel"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xf16>, %arg4: memref<?xf16>, %arg5: memref<?xf16>, %arg6: i32, %arg7: i32, %arg8: i32, %arg9: i32, %arg10: i32, %arg11: i32, %arg12: i32, %arg13: i32, %arg14: i32):
    %0 = "arith.constant"() <{value = 1 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = 0.000000e+00 : f16}> : () -> f16
    %2 = "arith.constant"() <{value = 31 : i32}> : () -> i32
    %3 = "arith.constant"() <{value = 127 : i32}> : () -> i32
    %4 = "arith.constant"() <{value = 63 : i32}> : () -> i32
    %5 = "arith.constant"() <{value = 32 : i32}> : () -> i32
    %6 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %7 = "arith.constant"() <{value = 4 : i32}> : () -> i32
    %8 = "arith.constant"() <{value = 128 : i32}> : () -> i32
    %9 = "arith.constant"() <{value = 64 : i32}> : () -> i32
    %10 = "arith.constant"() <{value = 128 : index}> : () -> index
    %11 = "arith.constant"() <{value = 64 : index}> : () -> index
    %12 = "arith.constant"() <{value = 1 : index}> : () -> index
    %13 = "arith.constant"() <{value = 32 : index}> : () -> index
    %14 = "arith.constant"() <{value = 0 : index}> : () -> index
    %15 = "arith.constant"() <{value = 32 : i64}> : () -> i64
    "hivm.hir.set_mask_norm"() : () -> ()
    %16 = "arith.muli"(%arg12, %arg13) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %17 = "arith.muli"(%16, %arg14) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%17) <{effects = ["write"]}> {logical_block_num} : (i32) -> ()
    %18 = "hivm.hir.get_block_idx"() : () -> i64
    %19 = "arith.trunci"(%18) : (i64) -> i32
    %20 = "arith.muli"(%arg14, %arg13) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %21 = "arith.divsi"(%19, %20) : (i32, i32) -> i32
    %22 = "arith.remsi"(%21, %arg12) : (i32, i32) -> i32
    %23 = "tensor.empty"() : () -> tensor<64x32xi64>
    %24 = "tensor.empty"() : () -> tensor<64xi32>
    %25 = "hivm.hir.vbrc"(%6, %24) <{broadcast_dims = array<i64>}> : (i32, tensor<64xi32>) -> tensor<64xi32>
    %26 = "tensor.empty"() : () -> tensor<128xi32>
    %27 = "hivm.hir.vbrc"(%6, %26) <{broadcast_dims = array<i64>}> : (i32, tensor<128xi32>) -> tensor<128xi32>
    %28 = "tensor.empty"() : () -> tensor<64x32xf16>
    %29 = "hivm.hir.vbrc"(%1, %28) <{broadcast_dims = array<i64>}> : (f16, tensor<64x32xf16>) -> tensor<64x32xf16>
    %30 = "tensor.empty"() : () -> tensor<32x128xf16>
    %31 = "hivm.hir.vbrc"(%1, %30) <{broadcast_dims = array<i64>}> : (f16, tensor<32x128xf16>) -> tensor<32x128xf16>
    %32 = "arith.addi"(%arg6, %4) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %33 = "arith.divsi"(%32, %9) : (i32, i32) -> i32
    %34 = "arith.addi"(%arg7, %3) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %35 = "arith.divsi"(%34, %8) : (i32, i32) -> i32
    %36 = "arith.muli"(%35, %7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %37 = "arith.divsi"(%22, %36) : (i32, i32) -> i32
    %38 = "arith.muli"(%37, %7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %39 = "arith.subi"(%33, %38) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %40 = "arith.minsi"(%39, %7) : (i32, i32) -> i32
    %41 = "arith.remsi"(%22, %40) : (i32, i32) -> i32
    %42 = "arith.addi"(%38, %41) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %43 = "arith.remsi"(%22, %36) : (i32, i32) -> i32
    %44 = "arith.divsi"(%43, %40) : (i32, i32) -> i32
    %45 = "arith.muli"(%42, %9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %46 = "arith.muli"(%44, %8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %47 = "hivm.hir.varange"(%24, %14, %12) <{operandSegmentSizes = array<i32: 1, 1, 1>}> : (tensor<64xi32>, index, index) -> tensor<64xi32>
    %48 = "hivm.hir.vadd"(%47, %45, %24) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xi32>, i32, tensor<64xi32>) -> tensor<64xi32>
    %49 = "hivm.hir.varange"(%26, %14, %12) <{operandSegmentSizes = array<i32: 1, 1, 1>}> : (tensor<128xi32>, index, index) -> tensor<128xi32>
    %50 = "hivm.hir.vadd"(%49, %46, %26) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xi32>, i32, tensor<128xi32>) -> tensor<128xi32>
    %51 = "arith.index_cast"(%45) : (i32) -> index
    %52 = "affine.apply"(%51) <{map = #map}> : (index) -> index
    %53 = "arith.index_cast"(%arg6) : (i32) -> index
    %54 = "affine.max"(%51, %53) <{map = #map1}> : (index, index) -> index
    %55 = "affine.min"(%52, %54) <{map = #map1}> : (index, index) -> index
    %56 = "affine.apply"(%55, %51) <{map = #map2}> : (index, index) -> index
    %57 = "tensor.extract_slice"(%48, %56) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<64xi32>, index) -> tensor<?xi32>
    %58 = "tensor.insert_slice"(%57, %25, %56) <{operandSegmentSizes = array<i32: 1, 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<?xi32>, tensor<64xi32>, index) -> tensor<64xi32>
    %59 = "arith.index_cast"(%46) : (i32) -> index
    %60 = "affine.apply"(%59) <{map = #map3}> : (index) -> index
    %61 = "arith.index_cast"(%arg7) : (i32) -> index
    %62 = "affine.max"(%59, %61) <{map = #map1}> : (index, index) -> index
    %63 = "affine.min"(%60, %62) <{map = #map1}> : (index, index) -> index
    %64 = "affine.apply"(%63, %59) <{map = #map2}> : (index, index) -> index
    %65 = "tensor.extract_slice"(%50, %64) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<128xi32>, index) -> tensor<?xi32>
    %66 = "tensor.insert_slice"(%65, %27, %64) <{operandSegmentSizes = array<i32: 1, 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<?xi32>, tensor<128xi32>, index) -> tensor<128xi32>
    %67 = "tensor.empty"() : () -> tensor<32xi32>
    %68 = "hivm.hir.varange"(%67, %14, %12) <{operandSegmentSizes = array<i32: 1, 1, 1>}> : (tensor<32xi32>, index, index) -> tensor<32xi32>
    %69 = "tensor.expand_shape"(%58) <{reassociation = [[0, 1]], static_output_shape = array<i64: 64, 1>}> : (tensor<64xi32>) -> tensor<64x1xi32>
    %70 = "tensor.empty"() : () -> tensor<64x1xi32>
    %71 = "hivm.hir.vmul"(%69, %arg9, %70) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x1xi32>, i32, tensor<64x1xi32>) -> tensor<64x1xi32>
    %72 = "tensor.empty"() : () -> tensor<64x32xi32>
    %73 = "hivm.hir.vbrc"(%71, %72) <{broadcast_dims = array<i64: 1>}> : (tensor<64x1xi32>, tensor<64x32xi32>) -> tensor<64x32xi32>
    %74 = "tensor.expand_shape"(%68) <{reassociation = [[0, 1]], static_output_shape = array<i64: 1, 32>}> : (tensor<32xi32>) -> tensor<1x32xi32>
    %75 = "hivm.hir.vbrc"(%74, %72) <{broadcast_dims = array<i64: 0>}> : (tensor<1x32xi32>, tensor<64x32xi32>) -> tensor<64x32xi32>
    %76 = "hivm.hir.vadd"(%73, %75, %72) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x32xi32>, tensor<64x32xi32>, tensor<64x32xi32>) -> tensor<64x32xi32>
    %77 = "hivm.hir.vcast"(%76, %23) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<64x32xi32>, tensor<64x32xi64>) -> tensor<64x32xi64>
    %78 = "tensor.expand_shape"(%68) <{reassociation = [[0, 1]], static_output_shape = array<i64: 32, 1>}> : (tensor<32xi32>) -> tensor<32x1xi32>
    %79 = "tensor.empty"() : () -> tensor<32x1xi32>
    %80 = "hivm.hir.vmul"(%78, %arg10, %79) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32x1xi32>, i32, tensor<32x1xi32>) -> tensor<32x1xi32>
    %81 = "tensor.empty"() : () -> tensor<32x128xi32>
    %82 = "hivm.hir.vbrc"(%80, %81) <{broadcast_dims = array<i64: 1>}> : (tensor<32x1xi32>, tensor<32x128xi32>) -> tensor<32x128xi32>
    %83 = "tensor.expand_shape"(%66) <{reassociation = [[0, 1]], static_output_shape = array<i64: 1, 128>}> : (tensor<128xi32>) -> tensor<1x128xi32>
    %84 = "hivm.hir.vbrc"(%83, %81) <{broadcast_dims = array<i64: 0>}> : (tensor<1x128xi32>, tensor<32x128xi32>) -> tensor<32x128xi32>
    %85 = "hivm.hir.vadd"(%82, %84, %81) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32x128xi32>, tensor<32x128xi32>, tensor<32x128xi32>) -> tensor<32x128xi32>
    %86 = "tensor.empty"() : () -> tensor<32x128xi64>
    %87 = "hivm.hir.vcast"(%85, %86) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<32x128xi32>, tensor<32x128xi64>) -> tensor<32x128xi64>
    %88 = "arith.addi"(%arg8, %2) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %89 = "arith.divsi"(%88, %5) : (i32, i32) -> i32
    %90 = "arith.muli"(%arg10, %5) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %91 = "tensor.empty"() : () -> tensor<64x128xf32>
    %92:3 = "scf.for"(%6, %89, %0, %77, %87, %91) ({
    ^bb0(%arg15: i32, %arg16: tensor<64x32xi64>, %arg17: tensor<32x128xi64>, %arg18: tensor<64x128xf32>):
      %101 = "arith.muli"(%arg15, %5) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %102 = "arith.subi"(%arg8, %101) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %103 = "arith.index_cast"(%102) : (i32) -> index
      %104 = "affine.max"(%103) <{map = #map4}> : (index) -> index
      %105 = "affine.min"(%104) <{map = #map5}> : (index) -> index
      %106 = "scf.for"(%14, %11, %12, %28) ({
      ^bb0(%arg23: index, %arg24: tensor<64x32xf16>):
        %132 = "affine.min"(%105) <{map = #map5}> : (index) -> index
        %133 = "scf.for"(%14, %132, %12, %arg24) ({
        ^bb0(%arg25: index, %arg26: tensor<64x32xf16>):
          %134 = "tensor.extract"(%arg16, %arg23, %arg25) {DiscreteMemAccess, "DuplicateTensorExtractForCube::visitedLabel" = 1 : i32} : (tensor<64x32xi64>, index, index) -> i64
          %135 = "arith.index_cast"(%134) : (i64) -> index
          %136 = "memref.reinterpret_cast"(%arg3, %135) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xf16>, index) -> memref<1xf16, strided<[1], offset: ?>>
          %137 = "memref.load"(%136, %14) : (memref<1xf16, strided<[1], offset: ?>>, index) -> f16
          %138 = "tensor.insert"(%137, %arg26, %arg23, %arg25) : (f16, tensor<64x32xf16>, index, index) -> tensor<64x32xf16>
          "scf.yield"(%138) {DiscreteMemAccess} : (tensor<64x32xf16>) -> ()
        }) {ExtractedLoadOrStore} : (index, index, index, tensor<64x32xf16>) -> tensor<64x32xf16>
        "scf.yield"(%133) : (tensor<64x32xf16>) -> ()
      }) {ExtractedLoadOrStore} : (index, index, index, tensor<64x32xf16>) -> tensor<64x32xf16>
      %107 = "tensor.extract_slice"(%106, %105) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: 64, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<64x32xf16>, index) -> tensor<64x?xf16>
      %108 = "tensor.insert_slice"(%107, %29, %105) <{operandSegmentSizes = array<i32: 1, 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: 64, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<64x?xf16>, tensor<64x32xf16>, index) -> tensor<64x32xf16>
      %109 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<64x32xf16>
      %110 = "bufferization.to_tensor"(%109) <{restrict, writable}> : (memref<64x32xf16>) -> tensor<64x32xf16>
      %111 = "hivm.hir.store"(%108, %110) {"inserted-store"} : (tensor<64x32xf16>, tensor<64x32xf16>) -> tensor<64x32xf16>
      %112 = "hivm.hir.load"(%111, %28) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<64x32xf16>, tensor<64x32xf16>) -> tensor<64x32xf16>
      %113 = "affine.min"(%105) <{map = #map5}> : (index) -> index
      %114 = "scf.for"(%14, %113, %12, %30) ({
      ^bb0(%arg19: index, %arg20: tensor<32x128xf16>):
        %126 = "scf.for"(%14, %10, %12, %arg20) ({
        ^bb0(%arg21: index, %arg22: tensor<32x128xf16>):
          %127 = "tensor.extract"(%arg17, %arg19, %arg21) {DiscreteMemAccess, "DuplicateTensorExtractForCube::visitedLabel" = 1 : i32} : (tensor<32x128xi64>, index, index) -> i64
          %128 = "arith.index_cast"(%127) : (i64) -> index
          %129 = "memref.reinterpret_cast"(%arg4, %128) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xf16>, index) -> memref<1xf16, strided<[1], offset: ?>>
          %130 = "memref.load"(%129, %14) : (memref<1xf16, strided<[1], offset: ?>>, index) -> f16
          %131 = "tensor.insert"(%130, %arg22, %arg19, %arg21) : (f16, tensor<32x128xf16>, index, index) -> tensor<32x128xf16>
          "scf.yield"(%131) {DiscreteMemAccess} : (tensor<32x128xf16>) -> ()
        }) {ExtractedLoadOrStore} : (index, index, index, tensor<32x128xf16>) -> tensor<32x128xf16>
        "scf.yield"(%126) : (tensor<32x128xf16>) -> ()
      }) {ExtractedLoadOrStore} : (index, index, index, tensor<32x128xf16>) -> tensor<32x128xf16>
      %115 = "tensor.extract_slice"(%114, %105) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 128>, static_strides = array<i64: 1, 1>}> : (tensor<32x128xf16>, index) -> tensor<?x128xf16>
      %116 = "tensor.insert_slice"(%115, %31, %105) <{operandSegmentSizes = array<i32: 1, 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 128>, static_strides = array<i64: 1, 1>}> : (tensor<?x128xf16>, tensor<32x128xf16>, index) -> tensor<32x128xf16>
      %117 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<32x128xf16>
      %118 = "bufferization.to_tensor"(%117) <{restrict, writable}> : (memref<32x128xf16>) -> tensor<32x128xf16>
      %119 = "hivm.hir.store"(%116, %118) {"inserted-store"} : (tensor<32x128xf16>, tensor<32x128xf16>) -> tensor<32x128xf16>
      %120 = "hivm.hir.load"(%119, %30) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<32x128xf16>, tensor<32x128xf16>) -> tensor<32x128xf16>
      %121 = "arith.cmpi"(%arg15, %6) <{predicate = 0 : i64}> : (i32, i32) -> i1
      %122 = "hivm.hir.mmadL1"(%112, %120, %121, %11, %13, %10, %arg18) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_for_result_already_inserted = true} : (tensor<64x32xf16>, tensor<32x128xf16>, i1, index, index, index, tensor<64x128xf32>) -> tensor<64x128xf32>
      %123 = "hivm.hir.vadd"(%arg16, %15, %23) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x32xi64>, i64, tensor<64x32xi64>) -> tensor<64x32xi64>
      %124 = "arith.extsi"(%90) : (i32) -> i64
      %125 = "hivm.hir.vadd"(%arg17, %124, %86) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32x128xi64>, i64, tensor<32x128xi64>) -> tensor<32x128xi64>
      "scf.yield"(%123, %125, %122) : (tensor<64x32xi64>, tensor<32x128xi64>, tensor<64x128xf32>) -> ()
    }) {fixpipe_for_mmad_result_already_inserted = true} : (i32, i32, i32, tensor<64x32xi64>, tensor<32x128xi64>, tensor<64x128xf32>) -> (tensor<64x32xi64>, tensor<32x128xi64>, tensor<64x128xf32>)
    %93 = "arith.index_cast"(%arg11) : (i32) -> index
    %94 = "affine.apply"(%51, %93) <{map = #map6}> : (index, index) -> index
    %95 = "affine.apply"(%94, %59) <{map = #map7}> : (index, index) -> index
    %96 = "memref.reinterpret_cast"(%arg5, %95, %93) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64, 128>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xf16>, index, index) -> memref<64x128xf16, strided<[?, 1], offset: ?>>
    %97 = "affine.min"(%56) <{map = #map8}> : (index) -> index
    %98 = "affine.min"(%64) <{map = #map9}> : (index) -> index
    %99 = "tensor.extract_slice"(%92#2, %97, %98) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<64x128xf32>, index, index) -> tensor<?x?xf32>
    %100 = "memref.subview"(%96, %97, %98) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<64x128xf16, strided<[?, 1], offset: ?>>, index, index) -> memref<?x?xf16, strided<[?, 1], offset: ?>>
    "hivm.hir.fixpipe"(%99, %100) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, pre_quant = #hivm.fixpipe_pre_quant_mode<F322F16>}> : (tensor<?x?xf32>, memref<?x?xf16, strided<[?, 1], offset: ?>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, false, false, false, false, false, false, false, false, false]> : vector<15xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<MIX>, mix_mode = "mix", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hacc.target = #hacc.target<"Ascend910B1">, hivm.module_core_type = #hivm.module_core_type<MIX>} : () -> ()

