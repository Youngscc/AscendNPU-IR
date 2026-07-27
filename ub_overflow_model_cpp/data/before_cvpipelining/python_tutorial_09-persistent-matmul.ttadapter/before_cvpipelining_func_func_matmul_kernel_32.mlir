#map = affine_map<()[s0] -> (s0 + 128)>
#map1 = affine_map<()[s0, s1] -> (s0, s1)>
#map2 = affine_map<()[s0, s1] -> (s0 - s1)>
#map3 = affine_map<()[s0] -> (s0, 0)>
#map4 = affine_map<()[s0] -> (s0, 64)>
#map5 = affine_map<()[s0, s1] -> (s0 * s1)>
#map6 = affine_map<()[s0, s1] -> (s0 + s1)>
#map7 = affine_map<()[s0] -> (s0, 128)>
"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xf16>, memref<?xf16>, memref<?xf16>, i32, i32, i32, i32, i32, i32, i32, i32, i32) -> (), sym_name = "matmul_kernel"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xf16>, %arg4: memref<?xf16>, %arg5: memref<?xf16>, %arg6: i32, %arg7: i32, %arg8: i32, %arg9: i32, %arg10: i32, %arg11: i32, %arg12: i32, %arg13: i32, %arg14: i32):
    %0 = "arith.constant"() <{value = 1 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = 0.000000e+00 : f16}> : () -> f16
    %2 = "arith.constant"() <{value = 63 : i32}> : () -> i32
    %3 = "arith.constant"() <{value = 127 : i32}> : () -> i32
    %4 = "arith.constant"() <{value = 64 : i32}> : () -> i32
    %5 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %6 = "arith.constant"() <{value = 8 : i32}> : () -> i32
    %7 = "arith.constant"() <{value = 128 : i32}> : () -> i32
    %8 = "arith.constant"() <{value = 128 : index}> : () -> index
    %9 = "arith.constant"() <{value = 1 : index}> : () -> index
    %10 = "arith.constant"() <{value = 64 : index}> : () -> index
    %11 = "arith.constant"() <{value = 0 : index}> : () -> index
    %12 = "arith.constant"() <{value = 64 : i64}> : () -> i64
    "hivm.hir.set_mask_norm"() : () -> ()
    %13 = "arith.muli"(%arg12, %arg13) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %14 = "arith.muli"(%13, %arg14) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%14) <{effects = ["write"]}> {logical_block_num} : (i32) -> ()
    %15 = "hivm.hir.get_block_idx"() : () -> i64
    %16 = "arith.trunci"(%15) : (i64) -> i32
    %17 = "arith.muli"(%arg14, %arg13) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %18 = "arith.divsi"(%16, %17) : (i32, i32) -> i32
    %19 = "arith.remsi"(%18, %arg12) : (i32, i32) -> i32
    %20 = "tensor.empty"() : () -> tensor<128x64xi64>
    %21 = "tensor.empty"() : () -> tensor<128xi32>
    %22 = "hivm.hir.vbrc"(%5, %21) <{broadcast_dims = array<i64>}> : (i32, tensor<128xi32>) -> tensor<128xi32>
    %23 = "tensor.empty"() : () -> tensor<128x64xf16>
    %24 = "hivm.hir.vbrc"(%1, %23) <{broadcast_dims = array<i64>}> : (f16, tensor<128x64xf16>) -> tensor<128x64xf16>
    %25 = "tensor.empty"() : () -> tensor<64x128xf16>
    %26 = "hivm.hir.vbrc"(%1, %25) <{broadcast_dims = array<i64>}> : (f16, tensor<64x128xf16>) -> tensor<64x128xf16>
    %27 = "arith.addi"(%arg6, %3) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %28 = "arith.divsi"(%27, %7) : (i32, i32) -> i32
    %29 = "arith.addi"(%arg7, %3) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %30 = "arith.divsi"(%29, %7) : (i32, i32) -> i32
    %31 = "arith.muli"(%30, %6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %32 = "arith.divsi"(%19, %31) : (i32, i32) -> i32
    %33 = "arith.muli"(%32, %6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %34 = "arith.subi"(%28, %33) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %35 = "arith.minsi"(%34, %6) : (i32, i32) -> i32
    %36 = "arith.remsi"(%19, %35) : (i32, i32) -> i32
    %37 = "arith.addi"(%33, %36) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %38 = "arith.remsi"(%19, %31) : (i32, i32) -> i32
    %39 = "arith.divsi"(%38, %35) : (i32, i32) -> i32
    %40 = "arith.muli"(%37, %7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %41 = "arith.muli"(%39, %7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %42 = "hivm.hir.varange"(%21, %11, %9) <{operandSegmentSizes = array<i32: 1, 1, 1>}> : (tensor<128xi32>, index, index) -> tensor<128xi32>
    %43 = "hivm.hir.vadd"(%42, %40, %21) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xi32>, i32, tensor<128xi32>) -> tensor<128xi32>
    %44 = "hivm.hir.vadd"(%42, %41, %21) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xi32>, i32, tensor<128xi32>) -> tensor<128xi32>
    %45 = "arith.index_cast"(%40) : (i32) -> index
    %46 = "affine.apply"(%45) <{map = #map}> : (index) -> index
    %47 = "arith.index_cast"(%arg6) : (i32) -> index
    %48 = "affine.max"(%45, %47) <{map = #map1}> : (index, index) -> index
    %49 = "affine.min"(%46, %48) <{map = #map1}> : (index, index) -> index
    %50 = "affine.apply"(%49, %45) <{map = #map2}> : (index, index) -> index
    %51 = "tensor.extract_slice"(%43, %50) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<128xi32>, index) -> tensor<?xi32>
    %52 = "tensor.insert_slice"(%51, %22, %50) <{operandSegmentSizes = array<i32: 1, 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<?xi32>, tensor<128xi32>, index) -> tensor<128xi32>
    %53 = "arith.index_cast"(%41) : (i32) -> index
    %54 = "affine.apply"(%53) <{map = #map}> : (index) -> index
    %55 = "arith.index_cast"(%arg7) : (i32) -> index
    %56 = "affine.max"(%53, %55) <{map = #map1}> : (index, index) -> index
    %57 = "affine.min"(%54, %56) <{map = #map1}> : (index, index) -> index
    %58 = "affine.apply"(%57, %53) <{map = #map2}> : (index, index) -> index
    %59 = "tensor.extract_slice"(%44, %58) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<128xi32>, index) -> tensor<?xi32>
    %60 = "tensor.insert_slice"(%59, %22, %58) <{operandSegmentSizes = array<i32: 1, 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<?xi32>, tensor<128xi32>, index) -> tensor<128xi32>
    %61 = "tensor.empty"() : () -> tensor<64xi32>
    %62 = "hivm.hir.varange"(%61, %11, %9) <{operandSegmentSizes = array<i32: 1, 1, 1>}> : (tensor<64xi32>, index, index) -> tensor<64xi32>
    %63 = "tensor.expand_shape"(%52) <{reassociation = [[0, 1]], static_output_shape = array<i64: 128, 1>}> : (tensor<128xi32>) -> tensor<128x1xi32>
    %64 = "tensor.empty"() : () -> tensor<128x1xi32>
    %65 = "hivm.hir.vmul"(%63, %arg9, %64) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128x1xi32>, i32, tensor<128x1xi32>) -> tensor<128x1xi32>
    %66 = "tensor.empty"() : () -> tensor<128x64xi32>
    %67 = "hivm.hir.vbrc"(%65, %66) <{broadcast_dims = array<i64: 1>}> : (tensor<128x1xi32>, tensor<128x64xi32>) -> tensor<128x64xi32>
    %68 = "tensor.expand_shape"(%62) <{reassociation = [[0, 1]], static_output_shape = array<i64: 1, 64>}> : (tensor<64xi32>) -> tensor<1x64xi32>
    %69 = "hivm.hir.vbrc"(%68, %66) <{broadcast_dims = array<i64: 0>}> : (tensor<1x64xi32>, tensor<128x64xi32>) -> tensor<128x64xi32>
    %70 = "hivm.hir.vadd"(%67, %69, %66) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128x64xi32>, tensor<128x64xi32>, tensor<128x64xi32>) -> tensor<128x64xi32>
    %71 = "hivm.hir.vcast"(%70, %20) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<128x64xi32>, tensor<128x64xi64>) -> tensor<128x64xi64>
    %72 = "tensor.expand_shape"(%62) <{reassociation = [[0, 1]], static_output_shape = array<i64: 64, 1>}> : (tensor<64xi32>) -> tensor<64x1xi32>
    %73 = "tensor.empty"() : () -> tensor<64x1xi32>
    %74 = "hivm.hir.vmul"(%72, %arg10, %73) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x1xi32>, i32, tensor<64x1xi32>) -> tensor<64x1xi32>
    %75 = "tensor.empty"() : () -> tensor<64x128xi32>
    %76 = "hivm.hir.vbrc"(%74, %75) <{broadcast_dims = array<i64: 1>}> : (tensor<64x1xi32>, tensor<64x128xi32>) -> tensor<64x128xi32>
    %77 = "tensor.expand_shape"(%60) <{reassociation = [[0, 1]], static_output_shape = array<i64: 1, 128>}> : (tensor<128xi32>) -> tensor<1x128xi32>
    %78 = "hivm.hir.vbrc"(%77, %75) <{broadcast_dims = array<i64: 0>}> : (tensor<1x128xi32>, tensor<64x128xi32>) -> tensor<64x128xi32>
    %79 = "hivm.hir.vadd"(%76, %78, %75) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x128xi32>, tensor<64x128xi32>, tensor<64x128xi32>) -> tensor<64x128xi32>
    %80 = "tensor.empty"() : () -> tensor<64x128xi64>
    %81 = "hivm.hir.vcast"(%79, %80) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<64x128xi32>, tensor<64x128xi64>) -> tensor<64x128xi64>
    %82 = "arith.addi"(%arg8, %2) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %83 = "arith.divsi"(%82, %4) : (i32, i32) -> i32
    %84 = "arith.muli"(%arg10, %4) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %85 = "tensor.empty"() : () -> tensor<128x128xf32>
    %86:3 = "scf.for"(%5, %83, %0, %71, %81, %85) ({
    ^bb0(%arg15: i32, %arg16: tensor<128x64xi64>, %arg17: tensor<64x128xi64>, %arg18: tensor<128x128xf32>):
      %95 = "arith.muli"(%arg15, %4) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %96 = "arith.subi"(%arg8, %95) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %97 = "arith.index_cast"(%96) : (i32) -> index
      %98 = "affine.max"(%97) <{map = #map3}> : (index) -> index
      %99 = "affine.min"(%98) <{map = #map4}> : (index) -> index
      %100 = "scf.for"(%11, %8, %9, %23) ({
      ^bb0(%arg23: index, %arg24: tensor<128x64xf16>):
        %126 = "affine.min"(%99) <{map = #map4}> : (index) -> index
        %127 = "scf.for"(%11, %126, %9, %arg24) ({
        ^bb0(%arg25: index, %arg26: tensor<128x64xf16>):
          %128 = "tensor.extract"(%arg16, %arg23, %arg25) {DiscreteMemAccess, "DuplicateTensorExtractForCube::visitedLabel" = 1 : i32} : (tensor<128x64xi64>, index, index) -> i64
          %129 = "arith.index_cast"(%128) : (i64) -> index
          %130 = "memref.reinterpret_cast"(%arg3, %129) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xf16>, index) -> memref<1xf16, strided<[1], offset: ?>>
          %131 = "memref.load"(%130, %11) : (memref<1xf16, strided<[1], offset: ?>>, index) -> f16
          %132 = "tensor.insert"(%131, %arg26, %arg23, %arg25) : (f16, tensor<128x64xf16>, index, index) -> tensor<128x64xf16>
          "scf.yield"(%132) {DiscreteMemAccess} : (tensor<128x64xf16>) -> ()
        }) {ExtractedLoadOrStore} : (index, index, index, tensor<128x64xf16>) -> tensor<128x64xf16>
        "scf.yield"(%127) : (tensor<128x64xf16>) -> ()
      }) {ExtractedLoadOrStore} : (index, index, index, tensor<128x64xf16>) -> tensor<128x64xf16>
      %101 = "tensor.extract_slice"(%100, %99) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: 128, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<128x64xf16>, index) -> tensor<128x?xf16>
      %102 = "tensor.insert_slice"(%101, %24, %99) <{operandSegmentSizes = array<i32: 1, 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: 128, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<128x?xf16>, tensor<128x64xf16>, index) -> tensor<128x64xf16>
      %103 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<128x64xf16>
      %104 = "bufferization.to_tensor"(%103) <{restrict, writable}> : (memref<128x64xf16>) -> tensor<128x64xf16>
      %105 = "hivm.hir.store"(%102, %104) {"inserted-store"} : (tensor<128x64xf16>, tensor<128x64xf16>) -> tensor<128x64xf16>
      %106 = "hivm.hir.load"(%105, %23) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<128x64xf16>, tensor<128x64xf16>) -> tensor<128x64xf16>
      %107 = "affine.min"(%99) <{map = #map4}> : (index) -> index
      %108 = "scf.for"(%11, %107, %9, %25) ({
      ^bb0(%arg19: index, %arg20: tensor<64x128xf16>):
        %120 = "scf.for"(%11, %8, %9, %arg20) ({
        ^bb0(%arg21: index, %arg22: tensor<64x128xf16>):
          %121 = "tensor.extract"(%arg17, %arg19, %arg21) {DiscreteMemAccess, "DuplicateTensorExtractForCube::visitedLabel" = 1 : i32} : (tensor<64x128xi64>, index, index) -> i64
          %122 = "arith.index_cast"(%121) : (i64) -> index
          %123 = "memref.reinterpret_cast"(%arg4, %122) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xf16>, index) -> memref<1xf16, strided<[1], offset: ?>>
          %124 = "memref.load"(%123, %11) : (memref<1xf16, strided<[1], offset: ?>>, index) -> f16
          %125 = "tensor.insert"(%124, %arg22, %arg19, %arg21) : (f16, tensor<64x128xf16>, index, index) -> tensor<64x128xf16>
          "scf.yield"(%125) {DiscreteMemAccess} : (tensor<64x128xf16>) -> ()
        }) {ExtractedLoadOrStore} : (index, index, index, tensor<64x128xf16>) -> tensor<64x128xf16>
        "scf.yield"(%120) : (tensor<64x128xf16>) -> ()
      }) {ExtractedLoadOrStore} : (index, index, index, tensor<64x128xf16>) -> tensor<64x128xf16>
      %109 = "tensor.extract_slice"(%108, %99) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 128>, static_strides = array<i64: 1, 1>}> : (tensor<64x128xf16>, index) -> tensor<?x128xf16>
      %110 = "tensor.insert_slice"(%109, %26, %99) <{operandSegmentSizes = array<i32: 1, 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 128>, static_strides = array<i64: 1, 1>}> : (tensor<?x128xf16>, tensor<64x128xf16>, index) -> tensor<64x128xf16>
      %111 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<64x128xf16>
      %112 = "bufferization.to_tensor"(%111) <{restrict, writable}> : (memref<64x128xf16>) -> tensor<64x128xf16>
      %113 = "hivm.hir.store"(%110, %112) {"inserted-store"} : (tensor<64x128xf16>, tensor<64x128xf16>) -> tensor<64x128xf16>
      %114 = "hivm.hir.load"(%113, %25) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<64x128xf16>, tensor<64x128xf16>) -> tensor<64x128xf16>
      %115 = "arith.cmpi"(%arg15, %5) <{predicate = 0 : i64}> : (i32, i32) -> i1
      %116 = "hivm.hir.mmadL1"(%106, %114, %115, %8, %10, %8, %arg18) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_for_result_already_inserted = true} : (tensor<128x64xf16>, tensor<64x128xf16>, i1, index, index, index, tensor<128x128xf32>) -> tensor<128x128xf32>
      %117 = "hivm.hir.vadd"(%arg16, %12, %20) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128x64xi64>, i64, tensor<128x64xi64>) -> tensor<128x64xi64>
      %118 = "arith.extsi"(%84) : (i32) -> i64
      %119 = "hivm.hir.vadd"(%arg17, %118, %80) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x128xi64>, i64, tensor<64x128xi64>) -> tensor<64x128xi64>
      "scf.yield"(%117, %119, %116) : (tensor<128x64xi64>, tensor<64x128xi64>, tensor<128x128xf32>) -> ()
    }) {fixpipe_for_mmad_result_already_inserted = true} : (i32, i32, i32, tensor<128x64xi64>, tensor<64x128xi64>, tensor<128x128xf32>) -> (tensor<128x64xi64>, tensor<64x128xi64>, tensor<128x128xf32>)
    %87 = "arith.index_cast"(%arg11) : (i32) -> index
    %88 = "affine.apply"(%45, %87) <{map = #map5}> : (index, index) -> index
    %89 = "affine.apply"(%88, %53) <{map = #map6}> : (index, index) -> index
    %90 = "memref.reinterpret_cast"(%arg5, %89, %87) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 128, 128>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xf16>, index, index) -> memref<128x128xf16, strided<[?, 1], offset: ?>>
    %91 = "affine.min"(%50) <{map = #map7}> : (index) -> index
    %92 = "affine.min"(%58) <{map = #map7}> : (index) -> index
    %93 = "tensor.extract_slice"(%86#2, %91, %92) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<128x128xf32>, index, index) -> tensor<?x?xf32>
    %94 = "memref.subview"(%90, %91, %92) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<128x128xf16, strided<[?, 1], offset: ?>>, index, index) -> memref<?x?xf16, strided<[?, 1], offset: ?>>
    "hivm.hir.fixpipe"(%93, %94) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, pre_quant = #hivm.fixpipe_pre_quant_mode<F322F16>}> : (tensor<?x?xf32>, memref<?x?xf16, strided<[?, 1], offset: ?>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, false, false, false, false, false, false, false, false, false]> : vector<15xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<MIX>, mix_mode = "mix", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hacc.target = #hacc.target<"Ascend910B1">, hivm.module_core_type = #hivm.module_core_type<MIX>} : () -> ()

