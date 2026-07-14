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
    "annotation.mark"(%14) {logical_block_num} : (i32) -> ()
    %15 = "hivm.hir.get_block_idx"() : () -> i64
    %16 = "arith.trunci"(%15) : (i64) -> i32
    %17 = "arith.muli"(%arg14, %arg13) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %18 = "arith.divsi"(%16, %17) : (i32, i32) -> i32
    %19 = "arith.remsi"(%18, %arg12) : (i32, i32) -> i32
    %20 = "tensor.empty"() : () -> tensor<128x64xi64>
    %21 = "tensor.empty"() : () -> tensor<128x64xi64>
    %22 = "tensor.empty"() : () -> tensor<128xi32>
    %23 = "tensor.empty"() : () -> tensor<128xi32>
    %24 = "tensor.empty"() : () -> tensor<128xi32>
    %25 = "tensor.empty"() : () -> tensor<128xi32>
    %26 = "hivm.hir.vbrc"(%5, %25) <{broadcast_dims = array<i64>}> : (i32, tensor<128xi32>) -> tensor<128xi32>
    %27 = "tensor.empty"() : () -> tensor<128x64xf16>
    %28 = "hivm.hir.vbrc"(%1, %27) <{broadcast_dims = array<i64>}> : (f16, tensor<128x64xf16>) -> tensor<128x64xf16>
    %29 = "tensor.empty"() : () -> tensor<64x128xf16>
    %30 = "hivm.hir.vbrc"(%1, %29) <{broadcast_dims = array<i64>}> : (f16, tensor<64x128xf16>) -> tensor<64x128xf16>
    %31 = "arith.addi"(%arg6, %3) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %32 = "arith.divsi"(%31, %7) : (i32, i32) -> i32
    %33 = "arith.addi"(%arg7, %3) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %34 = "arith.divsi"(%33, %7) : (i32, i32) -> i32
    %35 = "arith.muli"(%34, %6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %36 = "arith.divsi"(%19, %35) : (i32, i32) -> i32
    %37 = "arith.muli"(%36, %6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %38 = "arith.subi"(%32, %37) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %39 = "arith.minsi"(%38, %6) : (i32, i32) -> i32
    %40 = "arith.remsi"(%19, %39) : (i32, i32) -> i32
    %41 = "arith.addi"(%37, %40) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %42 = "arith.remsi"(%19, %35) : (i32, i32) -> i32
    %43 = "arith.divsi"(%42, %39) : (i32, i32) -> i32
    %44 = "arith.muli"(%41, %7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %45 = "arith.muli"(%43, %7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %46 = "hivm.hir.varange"(%24, %11, %9) <{operandSegmentSizes = array<i32: 1, 1, 1>}> : (tensor<128xi32>, index, index) -> tensor<128xi32>
    %47 = "hivm.hir.vadd"(%46, %44, %23) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xi32>, i32, tensor<128xi32>) -> tensor<128xi32>
    %48 = "hivm.hir.vadd"(%46, %45, %22) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xi32>, i32, tensor<128xi32>) -> tensor<128xi32>
    %49 = "arith.index_cast"(%44) : (i32) -> index
    %50 = "arith.addi"(%49, %8) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %51 = "arith.index_cast"(%arg6) : (i32) -> index
    %52 = "arith.maxsi"(%49, %51) : (index, index) -> index
    %53 = "arith.minsi"(%50, %52) : (index, index) -> index
    %54 = "arith.subi"(%53, %49) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %55 = "tensor.extract_slice"(%47, %54) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<128xi32>, index) -> tensor<?xi32>
    %56 = "tensor.insert_slice"(%55, %26, %54) <{operandSegmentSizes = array<i32: 1, 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<?xi32>, tensor<128xi32>, index) -> tensor<128xi32>
    %57 = "arith.index_cast"(%45) : (i32) -> index
    %58 = "arith.addi"(%57, %8) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %59 = "arith.index_cast"(%arg7) : (i32) -> index
    %60 = "arith.maxsi"(%57, %59) : (index, index) -> index
    %61 = "arith.minsi"(%58, %60) : (index, index) -> index
    %62 = "arith.subi"(%61, %57) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %63 = "tensor.extract_slice"(%48, %62) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<128xi32>, index) -> tensor<?xi32>
    %64 = "tensor.insert_slice"(%63, %26, %62) <{operandSegmentSizes = array<i32: 1, 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<?xi32>, tensor<128xi32>, index) -> tensor<128xi32>
    %65 = "tensor.empty"() : () -> tensor<64xi32>
    %66 = "hivm.hir.varange"(%65, %11, %9) <{operandSegmentSizes = array<i32: 1, 1, 1>}> : (tensor<64xi32>, index, index) -> tensor<64xi32>
    %67 = "tensor.expand_shape"(%56) <{reassociation = [[0, 1]], static_output_shape = array<i64: 128, 1>}> : (tensor<128xi32>) -> tensor<128x1xi32>
    %68 = "tensor.empty"() : () -> tensor<128x1xi32>
    %69 = "hivm.hir.vmul"(%67, %arg9, %68) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128x1xi32>, i32, tensor<128x1xi32>) -> tensor<128x1xi32>
    %70 = "tensor.empty"() : () -> tensor<128x64xi32>
    %71 = "tensor.empty"() : () -> tensor<128x64xi32>
    %72 = "tensor.empty"() : () -> tensor<128x64xi32>
    %73 = "hivm.hir.vbrc"(%69, %72) <{broadcast_dims = array<i64: 1>}> : (tensor<128x1xi32>, tensor<128x64xi32>) -> tensor<128x64xi32>
    %74 = "tensor.expand_shape"(%66) <{reassociation = [[0, 1]], static_output_shape = array<i64: 1, 64>}> : (tensor<64xi32>) -> tensor<1x64xi32>
    %75 = "hivm.hir.vbrc"(%74, %71) <{broadcast_dims = array<i64: 0>}> : (tensor<1x64xi32>, tensor<128x64xi32>) -> tensor<128x64xi32>
    %76 = "hivm.hir.vadd"(%73, %75, %70) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128x64xi32>, tensor<128x64xi32>, tensor<128x64xi32>) -> tensor<128x64xi32>
    %77 = "hivm.hir.vcast"(%76, %21) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<128x64xi32>, tensor<128x64xi64>) -> tensor<128x64xi64>
    %78 = "tensor.expand_shape"(%66) <{reassociation = [[0, 1]], static_output_shape = array<i64: 64, 1>}> : (tensor<64xi32>) -> tensor<64x1xi32>
    %79 = "tensor.empty"() : () -> tensor<64x1xi32>
    %80 = "hivm.hir.vmul"(%78, %arg10, %79) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x1xi32>, i32, tensor<64x1xi32>) -> tensor<64x1xi32>
    %81 = "tensor.empty"() : () -> tensor<64x128xi32>
    %82 = "tensor.empty"() : () -> tensor<64x128xi32>
    %83 = "tensor.empty"() : () -> tensor<64x128xi32>
    %84 = "hivm.hir.vbrc"(%80, %83) <{broadcast_dims = array<i64: 1>}> : (tensor<64x1xi32>, tensor<64x128xi32>) -> tensor<64x128xi32>
    %85 = "tensor.expand_shape"(%64) <{reassociation = [[0, 1]], static_output_shape = array<i64: 1, 128>}> : (tensor<128xi32>) -> tensor<1x128xi32>
    %86 = "hivm.hir.vbrc"(%85, %82) <{broadcast_dims = array<i64: 0>}> : (tensor<1x128xi32>, tensor<64x128xi32>) -> tensor<64x128xi32>
    %87 = "hivm.hir.vadd"(%84, %86, %81) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x128xi32>, tensor<64x128xi32>, tensor<64x128xi32>) -> tensor<64x128xi32>
    %88 = "tensor.empty"() : () -> tensor<64x128xi64>
    %89 = "tensor.empty"() : () -> tensor<64x128xi64>
    %90 = "hivm.hir.vcast"(%87, %89) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<64x128xi32>, tensor<64x128xi64>) -> tensor<64x128xi64>
    %91 = "arith.addi"(%arg8, %2) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %92 = "arith.divsi"(%91, %4) : (i32, i32) -> i32
    %93 = "arith.muli"(%arg10, %4) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %94 = "tensor.empty"() : () -> tensor<128x128xf32>
    %95:3 = "scf.for"(%5, %92, %0, %77, %90, %94) ({
    ^bb0(%arg15: i32, %arg16: tensor<128x64xi64>, %arg17: tensor<64x128xi64>, %arg18: tensor<128x128xf32>):
      %104 = "arith.muli"(%arg15, %4) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %105 = "arith.subi"(%arg8, %104) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %106 = "arith.index_cast"(%105) : (i32) -> index
      %107 = "arith.maxsi"(%106, %11) : (index, index) -> index
      %108 = "arith.minsi"(%107, %10) : (index, index) -> index
      %109 = "scf.for"(%11, %8, %9, %27) ({
      ^bb0(%arg23: index, %arg24: tensor<128x64xf16>):
        %137 = "arith.minsi"(%108, %10) : (index, index) -> index
        %138 = "scf.for"(%11, %137, %9, %arg24) ({
        ^bb0(%arg25: index, %arg26: tensor<128x64xf16>):
          %139 = "tensor.extract"(%arg16, %arg23, %arg25) {DiscreteMemAccess, "DuplicateTensorExtractForCube::visitedLabel" = 1 : i32} : (tensor<128x64xi64>, index, index) -> i64
          %140 = "arith.index_cast"(%139) : (i64) -> index
          %141 = "memref.reinterpret_cast"(%arg3, %140) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xf16>, index) -> memref<1xf16, strided<[1], offset: ?>>
          %142 = "memref.load"(%141, %11) : (memref<1xf16, strided<[1], offset: ?>>, index) -> f16
          %143 = "tensor.insert"(%142, %arg26, %arg23, %arg25) : (f16, tensor<128x64xf16>, index, index) -> tensor<128x64xf16>
          "scf.yield"(%143) {DiscreteMemAccess} : (tensor<128x64xf16>) -> ()
        }) {ExtractedLoadOrStore} : (index, index, index, tensor<128x64xf16>) -> tensor<128x64xf16>
        "scf.yield"(%138) : (tensor<128x64xf16>) -> ()
      }) {ExtractedLoadOrStore} : (index, index, index, tensor<128x64xf16>) -> tensor<128x64xf16>
      %110 = "tensor.extract_slice"(%109, %108) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: 128, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<128x64xf16>, index) -> tensor<128x?xf16>
      %111 = "tensor.insert_slice"(%110, %28, %108) <{operandSegmentSizes = array<i32: 1, 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: 128, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<128x?xf16>, tensor<128x64xf16>, index) -> tensor<128x64xf16>
      %112 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<128x64xf16>
      %113 = "bufferization.to_tensor"(%112) <{restrict, writable}> : (memref<128x64xf16>) -> tensor<128x64xf16>
      %114 = "hivm.hir.store"(%111, %113) {"inserted-store"} : (tensor<128x64xf16>, tensor<128x64xf16>) -> tensor<128x64xf16>
      %115 = "tensor.empty"() : () -> tensor<128x64xf16>
      %116 = "hivm.hir.load"(%114, %115) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<128x64xf16>, tensor<128x64xf16>) -> tensor<128x64xf16>
      %117 = "arith.minsi"(%108, %10) : (index, index) -> index
      %118 = "scf.for"(%11, %117, %9, %29) ({
      ^bb0(%arg19: index, %arg20: tensor<64x128xf16>):
        %131 = "scf.for"(%11, %8, %9, %arg20) ({
        ^bb0(%arg21: index, %arg22: tensor<64x128xf16>):
          %132 = "tensor.extract"(%arg17, %arg19, %arg21) {DiscreteMemAccess, "DuplicateTensorExtractForCube::visitedLabel" = 1 : i32} : (tensor<64x128xi64>, index, index) -> i64
          %133 = "arith.index_cast"(%132) : (i64) -> index
          %134 = "memref.reinterpret_cast"(%arg4, %133) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xf16>, index) -> memref<1xf16, strided<[1], offset: ?>>
          %135 = "memref.load"(%134, %11) : (memref<1xf16, strided<[1], offset: ?>>, index) -> f16
          %136 = "tensor.insert"(%135, %arg22, %arg19, %arg21) : (f16, tensor<64x128xf16>, index, index) -> tensor<64x128xf16>
          "scf.yield"(%136) {DiscreteMemAccess} : (tensor<64x128xf16>) -> ()
        }) {ExtractedLoadOrStore} : (index, index, index, tensor<64x128xf16>) -> tensor<64x128xf16>
        "scf.yield"(%131) : (tensor<64x128xf16>) -> ()
      }) {ExtractedLoadOrStore} : (index, index, index, tensor<64x128xf16>) -> tensor<64x128xf16>
      %119 = "tensor.extract_slice"(%118, %108) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 128>, static_strides = array<i64: 1, 1>}> : (tensor<64x128xf16>, index) -> tensor<?x128xf16>
      %120 = "tensor.insert_slice"(%119, %30, %108) <{operandSegmentSizes = array<i32: 1, 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 128>, static_strides = array<i64: 1, 1>}> : (tensor<?x128xf16>, tensor<64x128xf16>, index) -> tensor<64x128xf16>
      %121 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<64x128xf16>
      %122 = "bufferization.to_tensor"(%121) <{restrict, writable}> : (memref<64x128xf16>) -> tensor<64x128xf16>
      %123 = "hivm.hir.store"(%120, %122) {"inserted-store"} : (tensor<64x128xf16>, tensor<64x128xf16>) -> tensor<64x128xf16>
      %124 = "tensor.empty"() : () -> tensor<64x128xf16>
      %125 = "hivm.hir.load"(%123, %124) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<64x128xf16>, tensor<64x128xf16>) -> tensor<64x128xf16>
      %126 = "arith.cmpi"(%arg15, %5) <{predicate = 0 : i64}> : (i32, i32) -> i1
      %127 = "hivm.hir.mmadL1"(%116, %125, %126, %8, %10, %8, %arg18) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_already_inserted = true} : (tensor<128x64xf16>, tensor<64x128xf16>, i1, index, index, index, tensor<128x128xf32>) -> tensor<128x128xf32>
      %128 = "hivm.hir.vadd"(%arg16, %12, %20) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128x64xi64>, i64, tensor<128x64xi64>) -> tensor<128x64xi64>
      %129 = "arith.extsi"(%93) : (i32) -> i64
      %130 = "hivm.hir.vadd"(%arg17, %129, %88) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x128xi64>, i64, tensor<64x128xi64>) -> tensor<64x128xi64>
      "scf.yield"(%128, %130, %127) : (tensor<128x64xi64>, tensor<64x128xi64>, tensor<128x128xf32>) -> ()
    }) : (i32, i32, i32, tensor<128x64xi64>, tensor<64x128xi64>, tensor<128x128xf32>) -> (tensor<128x64xi64>, tensor<64x128xi64>, tensor<128x128xf32>)
    %96 = "arith.index_cast"(%arg11) : (i32) -> index
    %97 = "arith.muli"(%49, %96) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %98 = "arith.addi"(%97, %57) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %99 = "memref.reinterpret_cast"(%arg5, %98, %96) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 128, 128>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xf16>, index, index) -> memref<128x128xf16, strided<[?, 1], offset: ?>>
    %100 = "arith.minsi"(%54, %8) : (index, index) -> index
    %101 = "arith.minsi"(%62, %8) : (index, index) -> index
    %102 = "tensor.extract_slice"(%95#2, %100, %101) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<128x128xf32>, index, index) -> tensor<?x?xf32>
    %103 = "memref.subview"(%99, %100, %101) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<128x128xf16, strided<[?, 1], offset: ?>>, index, index) -> memref<?x?xf16, strided<[?, 1], offset: ?>>
    "hivm.hir.fixpipe"(%102, %103) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, pre_quant = #hivm.fixpipe_pre_quant_mode<F322F16>}> : (tensor<?x?xf32>, memref<?x?xf16, strided<[?, 1], offset: ?>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, false, false, false, false, false, false, false, false, false]> : vector<15xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<MIX>, mix_mode = "mix", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<MIX>} : () -> ()

