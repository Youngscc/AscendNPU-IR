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
    "annotation.mark"(%17) {logical_block_num} : (i32) -> ()
    %18 = "hivm.hir.get_block_idx"() : () -> i64
    %19 = "arith.trunci"(%18) : (i64) -> i32
    %20 = "arith.muli"(%arg14, %arg13) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %21 = "arith.divsi"(%19, %20) : (i32, i32) -> i32
    %22 = "arith.remsi"(%21, %arg12) : (i32, i32) -> i32
    %23 = "tensor.empty"() : () -> tensor<64x32xi64>
    %24 = "tensor.empty"() : () -> tensor<64x32xi64>
    %25 = "tensor.empty"() : () -> tensor<64xi32>
    %26 = "tensor.empty"() : () -> tensor<64xi32>
    %27 = "tensor.empty"() : () -> tensor<64xi32>
    %28 = "hivm.hir.vbrc"(%6, %27) <{broadcast_dims = array<i64>}> : (i32, tensor<64xi32>) -> tensor<64xi32>
    %29 = "tensor.empty"() : () -> tensor<128xi32>
    %30 = "tensor.empty"() : () -> tensor<128xi32>
    %31 = "tensor.empty"() : () -> tensor<128xi32>
    %32 = "hivm.hir.vbrc"(%6, %31) <{broadcast_dims = array<i64>}> : (i32, tensor<128xi32>) -> tensor<128xi32>
    %33 = "tensor.empty"() : () -> tensor<64x32xf16>
    %34 = "hivm.hir.vbrc"(%1, %33) <{broadcast_dims = array<i64>}> : (f16, tensor<64x32xf16>) -> tensor<64x32xf16>
    %35 = "tensor.empty"() : () -> tensor<32x128xf16>
    %36 = "hivm.hir.vbrc"(%1, %35) <{broadcast_dims = array<i64>}> : (f16, tensor<32x128xf16>) -> tensor<32x128xf16>
    %37 = "arith.addi"(%arg6, %4) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %38 = "arith.divsi"(%37, %9) : (i32, i32) -> i32
    %39 = "arith.addi"(%arg7, %3) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %40 = "arith.divsi"(%39, %8) : (i32, i32) -> i32
    %41 = "arith.muli"(%40, %7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %42 = "arith.divsi"(%22, %41) : (i32, i32) -> i32
    %43 = "arith.muli"(%42, %7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %44 = "arith.subi"(%38, %43) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %45 = "arith.minsi"(%44, %7) : (i32, i32) -> i32
    %46 = "arith.remsi"(%22, %45) : (i32, i32) -> i32
    %47 = "arith.addi"(%43, %46) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %48 = "arith.remsi"(%22, %41) : (i32, i32) -> i32
    %49 = "arith.divsi"(%48, %45) : (i32, i32) -> i32
    %50 = "arith.muli"(%47, %9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %51 = "arith.muli"(%49, %8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %52 = "hivm.hir.varange"(%26, %14, %12) <{operandSegmentSizes = array<i32: 1, 1, 1>}> : (tensor<64xi32>, index, index) -> tensor<64xi32>
    %53 = "hivm.hir.vadd"(%52, %50, %25) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64xi32>, i32, tensor<64xi32>) -> tensor<64xi32>
    %54 = "hivm.hir.varange"(%30, %14, %12) <{operandSegmentSizes = array<i32: 1, 1, 1>}> : (tensor<128xi32>, index, index) -> tensor<128xi32>
    %55 = "hivm.hir.vadd"(%54, %51, %29) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<128xi32>, i32, tensor<128xi32>) -> tensor<128xi32>
    %56 = "arith.index_cast"(%50) : (i32) -> index
    %57 = "arith.addi"(%56, %11) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %58 = "arith.index_cast"(%arg6) : (i32) -> index
    %59 = "arith.maxsi"(%56, %58) : (index, index) -> index
    %60 = "arith.minsi"(%57, %59) : (index, index) -> index
    %61 = "arith.subi"(%60, %56) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %62 = "tensor.extract_slice"(%53, %61) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<64xi32>, index) -> tensor<?xi32>
    %63 = "tensor.insert_slice"(%62, %28, %61) <{operandSegmentSizes = array<i32: 1, 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<?xi32>, tensor<64xi32>, index) -> tensor<64xi32>
    %64 = "arith.index_cast"(%51) : (i32) -> index
    %65 = "arith.addi"(%64, %10) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %66 = "arith.index_cast"(%arg7) : (i32) -> index
    %67 = "arith.maxsi"(%64, %66) : (index, index) -> index
    %68 = "arith.minsi"(%65, %67) : (index, index) -> index
    %69 = "arith.subi"(%68, %64) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %70 = "tensor.extract_slice"(%55, %69) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<128xi32>, index) -> tensor<?xi32>
    %71 = "tensor.insert_slice"(%70, %32, %69) <{operandSegmentSizes = array<i32: 1, 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<?xi32>, tensor<128xi32>, index) -> tensor<128xi32>
    %72 = "tensor.empty"() : () -> tensor<32xi32>
    %73 = "hivm.hir.varange"(%72, %14, %12) <{operandSegmentSizes = array<i32: 1, 1, 1>}> : (tensor<32xi32>, index, index) -> tensor<32xi32>
    %74 = "tensor.expand_shape"(%63) <{reassociation = [[0, 1]], static_output_shape = array<i64: 64, 1>}> : (tensor<64xi32>) -> tensor<64x1xi32>
    %75 = "tensor.empty"() : () -> tensor<64x1xi32>
    %76 = "hivm.hir.vmul"(%74, %arg9, %75) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x1xi32>, i32, tensor<64x1xi32>) -> tensor<64x1xi32>
    %77 = "tensor.empty"() : () -> tensor<64x32xi32>
    %78 = "tensor.empty"() : () -> tensor<64x32xi32>
    %79 = "tensor.empty"() : () -> tensor<64x32xi32>
    %80 = "hivm.hir.vbrc"(%76, %79) <{broadcast_dims = array<i64: 1>}> : (tensor<64x1xi32>, tensor<64x32xi32>) -> tensor<64x32xi32>
    %81 = "tensor.expand_shape"(%73) <{reassociation = [[0, 1]], static_output_shape = array<i64: 1, 32>}> : (tensor<32xi32>) -> tensor<1x32xi32>
    %82 = "hivm.hir.vbrc"(%81, %78) <{broadcast_dims = array<i64: 0>}> : (tensor<1x32xi32>, tensor<64x32xi32>) -> tensor<64x32xi32>
    %83 = "hivm.hir.vadd"(%80, %82, %77) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x32xi32>, tensor<64x32xi32>, tensor<64x32xi32>) -> tensor<64x32xi32>
    %84 = "hivm.hir.vcast"(%83, %24) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<64x32xi32>, tensor<64x32xi64>) -> tensor<64x32xi64>
    %85 = "tensor.expand_shape"(%73) <{reassociation = [[0, 1]], static_output_shape = array<i64: 32, 1>}> : (tensor<32xi32>) -> tensor<32x1xi32>
    %86 = "tensor.empty"() : () -> tensor<32x1xi32>
    %87 = "hivm.hir.vmul"(%85, %arg10, %86) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32x1xi32>, i32, tensor<32x1xi32>) -> tensor<32x1xi32>
    %88 = "tensor.empty"() : () -> tensor<32x128xi32>
    %89 = "tensor.empty"() : () -> tensor<32x128xi32>
    %90 = "tensor.empty"() : () -> tensor<32x128xi32>
    %91 = "hivm.hir.vbrc"(%87, %90) <{broadcast_dims = array<i64: 1>}> : (tensor<32x1xi32>, tensor<32x128xi32>) -> tensor<32x128xi32>
    %92 = "tensor.expand_shape"(%71) <{reassociation = [[0, 1]], static_output_shape = array<i64: 1, 128>}> : (tensor<128xi32>) -> tensor<1x128xi32>
    %93 = "hivm.hir.vbrc"(%92, %89) <{broadcast_dims = array<i64: 0>}> : (tensor<1x128xi32>, tensor<32x128xi32>) -> tensor<32x128xi32>
    %94 = "hivm.hir.vadd"(%91, %93, %88) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32x128xi32>, tensor<32x128xi32>, tensor<32x128xi32>) -> tensor<32x128xi32>
    %95 = "tensor.empty"() : () -> tensor<32x128xi64>
    %96 = "tensor.empty"() : () -> tensor<32x128xi64>
    %97 = "hivm.hir.vcast"(%94, %96) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<32x128xi32>, tensor<32x128xi64>) -> tensor<32x128xi64>
    %98 = "arith.addi"(%arg8, %2) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %99 = "arith.divsi"(%98, %5) : (i32, i32) -> i32
    %100 = "arith.muli"(%arg10, %5) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %101 = "tensor.empty"() : () -> tensor<64x128xf32>
    %102:3 = "scf.for"(%6, %99, %0, %84, %97, %101) ({
    ^bb0(%arg15: i32, %arg16: tensor<64x32xi64>, %arg17: tensor<32x128xi64>, %arg18: tensor<64x128xf32>):
      %111 = "arith.muli"(%arg15, %5) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %112 = "arith.subi"(%arg8, %111) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %113 = "arith.index_cast"(%112) : (i32) -> index
      %114 = "arith.maxsi"(%113, %14) : (index, index) -> index
      %115 = "arith.minsi"(%114, %13) : (index, index) -> index
      %116 = "scf.for"(%14, %11, %12, %33) ({
      ^bb0(%arg23: index, %arg24: tensor<64x32xf16>):
        %144 = "arith.minsi"(%115, %13) : (index, index) -> index
        %145 = "scf.for"(%14, %144, %12, %arg24) ({
        ^bb0(%arg25: index, %arg26: tensor<64x32xf16>):
          %146 = "tensor.extract"(%arg16, %arg23, %arg25) {DiscreteMemAccess, "DuplicateTensorExtractForCube::visitedLabel" = 1 : i32} : (tensor<64x32xi64>, index, index) -> i64
          %147 = "arith.index_cast"(%146) : (i64) -> index
          %148 = "memref.reinterpret_cast"(%arg3, %147) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xf16>, index) -> memref<1xf16, strided<[1], offset: ?>>
          %149 = "memref.load"(%148, %14) : (memref<1xf16, strided<[1], offset: ?>>, index) -> f16
          %150 = "tensor.insert"(%149, %arg26, %arg23, %arg25) : (f16, tensor<64x32xf16>, index, index) -> tensor<64x32xf16>
          "scf.yield"(%150) {DiscreteMemAccess} : (tensor<64x32xf16>) -> ()
        }) {ExtractedLoadOrStore} : (index, index, index, tensor<64x32xf16>) -> tensor<64x32xf16>
        "scf.yield"(%145) : (tensor<64x32xf16>) -> ()
      }) {ExtractedLoadOrStore} : (index, index, index, tensor<64x32xf16>) -> tensor<64x32xf16>
      %117 = "tensor.extract_slice"(%116, %115) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: 64, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<64x32xf16>, index) -> tensor<64x?xf16>
      %118 = "tensor.insert_slice"(%117, %34, %115) <{operandSegmentSizes = array<i32: 1, 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: 64, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<64x?xf16>, tensor<64x32xf16>, index) -> tensor<64x32xf16>
      %119 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<64x32xf16>
      %120 = "bufferization.to_tensor"(%119) <{restrict, writable}> : (memref<64x32xf16>) -> tensor<64x32xf16>
      %121 = "hivm.hir.store"(%118, %120) {"inserted-store"} : (tensor<64x32xf16>, tensor<64x32xf16>) -> tensor<64x32xf16>
      %122 = "tensor.empty"() : () -> tensor<64x32xf16>
      %123 = "hivm.hir.load"(%121, %122) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<64x32xf16>, tensor<64x32xf16>) -> tensor<64x32xf16>
      %124 = "arith.minsi"(%115, %13) : (index, index) -> index
      %125 = "scf.for"(%14, %124, %12, %35) ({
      ^bb0(%arg19: index, %arg20: tensor<32x128xf16>):
        %138 = "scf.for"(%14, %10, %12, %arg20) ({
        ^bb0(%arg21: index, %arg22: tensor<32x128xf16>):
          %139 = "tensor.extract"(%arg17, %arg19, %arg21) {DiscreteMemAccess, "DuplicateTensorExtractForCube::visitedLabel" = 1 : i32} : (tensor<32x128xi64>, index, index) -> i64
          %140 = "arith.index_cast"(%139) : (i64) -> index
          %141 = "memref.reinterpret_cast"(%arg4, %140) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xf16>, index) -> memref<1xf16, strided<[1], offset: ?>>
          %142 = "memref.load"(%141, %14) : (memref<1xf16, strided<[1], offset: ?>>, index) -> f16
          %143 = "tensor.insert"(%142, %arg22, %arg19, %arg21) : (f16, tensor<32x128xf16>, index, index) -> tensor<32x128xf16>
          "scf.yield"(%143) {DiscreteMemAccess} : (tensor<32x128xf16>) -> ()
        }) {ExtractedLoadOrStore} : (index, index, index, tensor<32x128xf16>) -> tensor<32x128xf16>
        "scf.yield"(%138) : (tensor<32x128xf16>) -> ()
      }) {ExtractedLoadOrStore} : (index, index, index, tensor<32x128xf16>) -> tensor<32x128xf16>
      %126 = "tensor.extract_slice"(%125, %115) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 128>, static_strides = array<i64: 1, 1>}> : (tensor<32x128xf16>, index) -> tensor<?x128xf16>
      %127 = "tensor.insert_slice"(%126, %36, %115) <{operandSegmentSizes = array<i32: 1, 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 128>, static_strides = array<i64: 1, 1>}> : (tensor<?x128xf16>, tensor<32x128xf16>, index) -> tensor<32x128xf16>
      %128 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<32x128xf16>
      %129 = "bufferization.to_tensor"(%128) <{restrict, writable}> : (memref<32x128xf16>) -> tensor<32x128xf16>
      %130 = "hivm.hir.store"(%127, %129) {"inserted-store"} : (tensor<32x128xf16>, tensor<32x128xf16>) -> tensor<32x128xf16>
      %131 = "tensor.empty"() : () -> tensor<32x128xf16>
      %132 = "hivm.hir.load"(%130, %131) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<32x128xf16>, tensor<32x128xf16>) -> tensor<32x128xf16>
      %133 = "arith.cmpi"(%arg15, %6) <{predicate = 0 : i64}> : (i32, i32) -> i1
      %134 = "hivm.hir.mmadL1"(%123, %132, %133, %11, %13, %10, %arg18) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_already_inserted = true} : (tensor<64x32xf16>, tensor<32x128xf16>, i1, index, index, index, tensor<64x128xf32>) -> tensor<64x128xf32>
      %135 = "hivm.hir.vadd"(%arg16, %15, %23) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x32xi64>, i64, tensor<64x32xi64>) -> tensor<64x32xi64>
      %136 = "arith.extsi"(%100) : (i32) -> i64
      %137 = "hivm.hir.vadd"(%arg17, %136, %95) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32x128xi64>, i64, tensor<32x128xi64>) -> tensor<32x128xi64>
      "scf.yield"(%135, %137, %134) : (tensor<64x32xi64>, tensor<32x128xi64>, tensor<64x128xf32>) -> ()
    }) : (i32, i32, i32, tensor<64x32xi64>, tensor<32x128xi64>, tensor<64x128xf32>) -> (tensor<64x32xi64>, tensor<32x128xi64>, tensor<64x128xf32>)
    %103 = "arith.index_cast"(%arg11) : (i32) -> index
    %104 = "arith.muli"(%56, %103) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %105 = "arith.addi"(%104, %64) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %106 = "memref.reinterpret_cast"(%arg5, %105, %103) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 64, 128>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xf16>, index, index) -> memref<64x128xf16, strided<[?, 1], offset: ?>>
    %107 = "arith.minsi"(%61, %11) : (index, index) -> index
    %108 = "arith.minsi"(%69, %10) : (index, index) -> index
    %109 = "tensor.extract_slice"(%102#2, %107, %108) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<64x128xf32>, index, index) -> tensor<?x?xf32>
    %110 = "memref.subview"(%106, %107, %108) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<64x128xf16, strided<[?, 1], offset: ?>>, index, index) -> memref<?x?xf16, strided<[?, 1], offset: ?>>
    "hivm.hir.fixpipe"(%109, %110) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, pre_quant = #hivm.fixpipe_pre_quant_mode<F322F16>}> : (tensor<?x?xf32>, memref<?x?xf16, strided<[?, 1], offset: ?>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, false, false, false, false, false, false, false, false, false]> : vector<15xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<MIX>, mix_mode = "mix", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<MIX>} : () -> ()

