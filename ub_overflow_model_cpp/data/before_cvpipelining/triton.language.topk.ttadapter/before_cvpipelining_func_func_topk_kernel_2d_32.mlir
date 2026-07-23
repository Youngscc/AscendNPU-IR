"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xf32>, memref<?xf32>, i32, i32, i32) -> (), sym_name = "topk_kernel_2d"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xf32>, %arg4: memref<?xf32>, %arg5: i32, %arg6: i32, %arg7: i32):
    %0 = "arith.constant"() <{value = 1 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = 0 : index}> : () -> index
    %2 = "arith.constant"() <{value = 1 : index}> : () -> index
    %3 = "arith.constant"() <{value = 4 : i32}> : () -> i32
    %4 = "arith.constant"() <{value = 8 : i32}> : () -> i32
    "hivm.hir.set_mask_norm"() : () -> ()
    %5 = "arith.muli"(%arg5, %arg6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %6 = "arith.muli"(%5, %arg7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%6) {logical_block_num} : (i32) -> ()
    %7 = "hivm.hir.get_block_idx"() : () -> i64
    %8 = "arith.trunci"(%7) : (i64) -> i32
    %9 = "arith.muli"(%arg7, %arg6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %10 = "arith.divsi"(%8, %9) : (i32, i32) -> i32
    %11 = "arith.remsi"(%10, %arg5) : (i32, i32) -> i32
    %12 = "arith.muli"(%11, %4) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %13 = "arith.index_cast"(%12) : (i32) -> index
    %14 = "memref.reinterpret_cast"(%arg3, %13) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 2, 2, 2>, static_strides = array<i64: 4, 2, 1>}> : (memref<?xf32>, index) -> memref<2x2x2xf32, strided<[4, 2, 1], offset: ?>>
    %15 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<2x2x2xf32>
    "hivm.hir.load"(%14, %15) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<2x2x2xf32, strided<[4, 2, 1], offset: ?>>, memref<2x2x2xf32>) -> ()
    %16 = "bufferization.to_tensor"(%15) <{restrict, writable}> : (memref<2x2x2xf32>) -> tensor<2x2x2xf32>
    %17 = "tensor.empty"() : () -> tensor<2xi32>
    %18 = "hivm.hir.varange"(%17, %1, %2) <{operandSegmentSizes = array<i32: 1, 1, 1>}> : (tensor<2xi32>, index, index) -> tensor<2xi32>
    %19 = "tensor.expand_shape"(%18) <{reassociation = [[0, 1]], static_output_shape = array<i64: 2, 1>}> : (tensor<2xi32>) -> tensor<2x1xi32>
    %20 = "tensor.expand_shape"(%18) <{reassociation = [[0, 1]], static_output_shape = array<i64: 1, 2>}> : (tensor<2xi32>) -> tensor<1x2xi32>
    %21 = "tensor.empty"() : () -> tensor<2x2x2xi32>
    %22 = "tensor.empty"() : () -> tensor<2x2x2xi32>
    %23 = "tensor.empty"() : () -> tensor<2x2x2xi32>
    %24 = "tensor.empty"() : () -> tensor<2x2x2xi32>
    %25 = "tensor.empty"() : () -> tensor<2x2x2xi32>
    %26 = "tensor.empty"() : () -> tensor<2x2x2xi32>
    %27 = "tensor.empty"() : () -> tensor<2x2x2xi32>
    %28 = "tensor.empty"() : () -> tensor<2x2x2xi32>
    %29 = "tensor.empty"() : () -> tensor<2x2x2xi32>
    %30 = "tensor.empty"() : () -> tensor<2x2x2xi32>
    %31 = "tensor.empty"() : () -> tensor<2x2x2xi32>
    %32 = "tensor.empty"() : () -> tensor<2x2x2xi32>
    %33 = "tensor.empty"() : () -> tensor<2x2x2xi32>
    %34 = "tensor.empty"() : () -> tensor<2x2x2xi32>
    %35 = "tensor.empty"() : () -> tensor<2x2x2xi32>
    %36 = "tensor.empty"() : () -> tensor<2x2x2xi32>
    %37 = "tensor.empty"() : () -> tensor<2x2x2xi32>
    %38 = "tensor.empty"() : () -> tensor<2x2x2xi32>
    %39 = "hivm.hir.bitcast"(%16) : (tensor<2x2x2xf32>) -> tensor<2x2x2xi32>
    %40 = "tensor.empty"() : () -> tensor<2x2xi32>
    %41 = "tensor.empty"() : () -> tensor<2x2xi32>
    %42 = "tensor.empty"() : () -> tensor<2x2xi32>
    %43 = "tensor.empty"() : () -> tensor<2x2xi32>
    %44 = "tensor.empty"() : () -> tensor<2x2xi32>
    %45 = "tensor.empty"() : () -> tensor<2x2xi32>
    %46 = "tensor.empty"() : () -> tensor<2x2xi32>
    %47 = "tensor.empty"() : () -> tensor<2x2xi32>
    %48 = "tensor.empty"() : () -> tensor<2x2xi32>
    %49 = "tensor.empty"() : () -> tensor<2x2xi32>
    %50 = "tensor.empty"() : () -> tensor<2x2xi32>
    %51 = "tensor.empty"() : () -> tensor<2x2xi32>
    %52 = "tensor.empty"() : () -> tensor<2x2x1xi32>
    %53 = "hivm.hir.vreduce"(%39, %52) <{arith = #hivm.reduce_op<xori>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 2>}> : (tensor<2x2x2xi32>, tensor<2x2x1xi32>) -> tensor<2x2x1xi32>
    %54 = "hivm.hir.vbrc"(%53, %38) <{broadcast_dims = array<i64: 2>}> : (tensor<2x2x1xi32>, tensor<2x2x2xi32>) -> tensor<2x2x2xi32>
    %55 = "hivm.hir.vor"(%39, %54, %37) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x2x2xi32>, tensor<2x2x2xi32>, tensor<2x2x2xi32>) -> tensor<2x2x2xi32>
    %56 = "hivm.hir.vand"(%39, %54, %36) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x2x2xi32>, tensor<2x2x2xi32>, tensor<2x2x2xi32>) -> tensor<2x2x2xi32>
    %57 = "hivm.hir.vnot"(%56, %56) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<2x2x2xi32>, tensor<2x2x2xi32>) -> tensor<2x2x2xi32>
    %58 = "hivm.hir.vand"(%57, %55, %35) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x2x2xi32>, tensor<2x2x2xi32>, tensor<2x2x2xi32>) -> tensor<2x2x2xi32>
    %59 = "tensor.empty"() : () -> tensor<2x2x2xf32>
    %60 = "tensor.empty"() : () -> tensor<2x2x2xf32>
    %61 = "tensor.empty"() : () -> tensor<2x2x2xf32>
    %62 = "hivm.hir.bitcast"(%58) : (tensor<2x2x2xi32>) -> tensor<2x2x2xf32>
    %63 = "tensor.empty"() : () -> tensor<2x2x2xi1>
    %64 = "tensor.empty"() : () -> tensor<2x2x2xi1>
    %65 = "tensor.empty"() : () -> tensor<2x2x2xi1>
    %66 = "tensor.empty"() : () -> tensor<2x2x2xi1>
    %67 = "tensor.empty"() : () -> tensor<2x2x2xi1>
    %68 = "tensor.empty"() : () -> tensor<2x2x2xi1>
    %69 = "tensor.empty"() : () -> tensor<2x2x2xi1>
    %70 = "tensor.empty"() : () -> tensor<2x2x2xi1>
    %71 = "tensor.empty"() : () -> tensor<2x2x2xi1>
    %72 = "hivm.hir.vcmp"(%16, %62, %71) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<gt>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<2x2x2xf32>, tensor<2x2x2xf32>, tensor<2x2x2xi1>) -> tensor<2x2x2xi1>
    %73 = "tensor.empty"() : () -> tensor<1x2x2xi32>
    %74 = "tensor.empty"() : () -> tensor<1x2x2xi32>
    %75 = "tensor.empty"() : () -> tensor<1x2x2xi32>
    %76 = "tensor.empty"() : () -> tensor<1x2x2xi32>
    %77 = "tensor.empty"() : () -> tensor<1x2x2xi32>
    %78 = "tensor.expand_shape"(%18) <{reassociation = [[0, 1, 2]], static_output_shape = array<i64: 1, 2, 1>}> : (tensor<2xi32>) -> tensor<1x2x1xi32>
    %79 = "hivm.hir.vbrc"(%78, %77) <{broadcast_dims = array<i64: 2>}> : (tensor<1x2x1xi32>, tensor<1x2x2xi32>) -> tensor<1x2x2xi32>
    %80 = "tensor.expand_shape"(%18) <{reassociation = [[0, 1, 2]], static_output_shape = array<i64: 1, 1, 2>}> : (tensor<2xi32>) -> tensor<1x1x2xi32>
    %81 = "hivm.hir.vbrc"(%80, %76) <{broadcast_dims = array<i64: 1>}> : (tensor<1x1x2xi32>, tensor<1x2x2xi32>) -> tensor<1x2x2xi32>
    %82 = "hivm.hir.vor"(%79, %81, %75) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1x2x2xi32>, tensor<1x2x2xi32>, tensor<1x2x2xi32>) -> tensor<1x2x2xi32>
    %83 = "hivm.hir.vand"(%79, %81, %74) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1x2x2xi32>, tensor<1x2x2xi32>, tensor<1x2x2xi32>) -> tensor<1x2x2xi32>
    %84 = "hivm.hir.vnot"(%83, %83) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<1x2x2xi32>, tensor<1x2x2xi32>) -> tensor<1x2x2xi32>
    %85 = "hivm.hir.vand"(%84, %82, %73) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1x2x2xi32>, tensor<1x2x2xi32>, tensor<1x2x2xi32>) -> tensor<1x2x2xi32>
    %86 = "hivm.hir.vcast"(%72, %34) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<2x2x2xi1>, tensor<2x2x2xi32>) -> tensor<2x2x2xi32>
    %87 = "hivm.hir.vbrc"(%85, %33) <{broadcast_dims = array<i64: 0>}> : (tensor<1x2x2xi32>, tensor<2x2x2xi32>) -> tensor<2x2x2xi32>
    %88 = "hivm.hir.vcmp"(%86, %87, %70) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<2x2x2xi32>, tensor<2x2x2xi32>, tensor<2x2x2xi1>) -> tensor<2x2x2xi1>
    %89 = "hivm.hir.vnot"(%88, %69) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<2x2x2xi1>, tensor<2x2x2xi1>) -> tensor<2x2x2xi1>
    %90 = "hivm.hir.vsel"(%89, %62, %16, %61) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<2x2x2xi1>, tensor<2x2x2xf32>, tensor<2x2x2xf32>, tensor<2x2x2xf32>) -> tensor<2x2x2xf32>
    %91 = "hivm.hir.bitcast"(%90) : (tensor<2x2x2xf32>) -> tensor<2x2x2xi32>
    %92 = "tensor.empty"() : () -> tensor<2x1x2xi32>
    %93 = "hivm.hir.vreduce"(%91, %92) <{arith = #hivm.reduce_op<xori>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 1>}> : (tensor<2x2x2xi32>, tensor<2x1x2xi32>) -> tensor<2x1x2xi32>
    %94 = "hivm.hir.vbrc"(%93, %32) <{broadcast_dims = array<i64: 1>}> : (tensor<2x1x2xi32>, tensor<2x2x2xi32>) -> tensor<2x2x2xi32>
    %95 = "hivm.hir.vor"(%91, %94, %31) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x2x2xi32>, tensor<2x2x2xi32>, tensor<2x2x2xi32>) -> tensor<2x2x2xi32>
    %96 = "hivm.hir.vand"(%91, %94, %30) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x2x2xi32>, tensor<2x2x2xi32>, tensor<2x2x2xi32>) -> tensor<2x2x2xi32>
    %97 = "hivm.hir.vnot"(%96, %96) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<2x2x2xi32>, tensor<2x2x2xi32>) -> tensor<2x2x2xi32>
    %98 = "hivm.hir.vand"(%97, %95, %29) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x2x2xi32>, tensor<2x2x2xi32>, tensor<2x2x2xi32>) -> tensor<2x2x2xi32>
    %99 = "hivm.hir.bitcast"(%98) : (tensor<2x2x2xi32>) -> tensor<2x2x2xf32>
    %100 = "hivm.hir.vcmp"(%90, %99, %68) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<gt>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<2x2x2xf32>, tensor<2x2x2xf32>, tensor<2x2x2xi1>) -> tensor<2x2x2xi1>
    %101 = "tensor.empty"() : () -> tensor<2x2x1xi32>
    %102 = "tensor.empty"() : () -> tensor<2x2x1xi32>
    %103 = "tensor.empty"() : () -> tensor<2x2x1xi32>
    %104 = "tensor.empty"() : () -> tensor<2x2x1xi32>
    %105 = "tensor.empty"() : () -> tensor<2x2x1xi32>
    %106 = "tensor.expand_shape"(%18) <{reassociation = [[0, 1, 2]], static_output_shape = array<i64: 2, 1, 1>}> : (tensor<2xi32>) -> tensor<2x1x1xi32>
    %107 = "hivm.hir.vbrc"(%106, %105) <{broadcast_dims = array<i64: 1>}> : (tensor<2x1x1xi32>, tensor<2x2x1xi32>) -> tensor<2x2x1xi32>
    %108 = "tensor.expand_shape"(%18) <{reassociation = [[0, 1, 2]], static_output_shape = array<i64: 1, 2, 1>}> : (tensor<2xi32>) -> tensor<1x2x1xi32>
    %109 = "hivm.hir.vbrc"(%108, %104) <{broadcast_dims = array<i64: 0>}> : (tensor<1x2x1xi32>, tensor<2x2x1xi32>) -> tensor<2x2x1xi32>
    %110 = "hivm.hir.vor"(%107, %109, %103) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x2x1xi32>, tensor<2x2x1xi32>, tensor<2x2x1xi32>) -> tensor<2x2x1xi32>
    %111 = "hivm.hir.vand"(%107, %109, %102) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x2x1xi32>, tensor<2x2x1xi32>, tensor<2x2x1xi32>) -> tensor<2x2x1xi32>
    %112 = "hivm.hir.vnot"(%111, %111) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<2x2x1xi32>, tensor<2x2x1xi32>) -> tensor<2x2x1xi32>
    %113 = "hivm.hir.vand"(%112, %110, %101) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x2x1xi32>, tensor<2x2x1xi32>, tensor<2x2x1xi32>) -> tensor<2x2x1xi32>
    %114 = "hivm.hir.vcast"(%100, %28) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<2x2x2xi1>, tensor<2x2x2xi32>) -> tensor<2x2x2xi32>
    %115 = "hivm.hir.vbrc"(%113, %27) <{broadcast_dims = array<i64: 2>}> : (tensor<2x2x1xi32>, tensor<2x2x2xi32>) -> tensor<2x2x2xi32>
    %116 = "hivm.hir.vcmp"(%114, %115, %67) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<2x2x2xi32>, tensor<2x2x2xi32>, tensor<2x2x2xi1>) -> tensor<2x2x2xi1>
    %117 = "hivm.hir.vnot"(%116, %66) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<2x2x2xi1>, tensor<2x2x2xi1>) -> tensor<2x2x2xi1>
    %118 = "hivm.hir.vsel"(%117, %99, %90, %60) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<2x2x2xi1>, tensor<2x2x2xf32>, tensor<2x2x2xf32>, tensor<2x2x2xf32>) -> tensor<2x2x2xf32>
    %119 = "hivm.hir.bitcast"(%118) : (tensor<2x2x2xf32>) -> tensor<2x2x2xi32>
    %120 = "tensor.empty"() : () -> tensor<2x2x1xi32>
    %121 = "hivm.hir.vreduce"(%119, %120) <{arith = #hivm.reduce_op<xori>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 2>}> : (tensor<2x2x2xi32>, tensor<2x2x1xi32>) -> tensor<2x2x1xi32>
    %122 = "hivm.hir.vbrc"(%121, %26) <{broadcast_dims = array<i64: 2>}> : (tensor<2x2x1xi32>, tensor<2x2x2xi32>) -> tensor<2x2x2xi32>
    %123 = "hivm.hir.vor"(%119, %122, %25) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x2x2xi32>, tensor<2x2x2xi32>, tensor<2x2x2xi32>) -> tensor<2x2x2xi32>
    %124 = "hivm.hir.vand"(%119, %122, %24) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x2x2xi32>, tensor<2x2x2xi32>, tensor<2x2x2xi32>) -> tensor<2x2x2xi32>
    %125 = "hivm.hir.vnot"(%124, %124) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<2x2x2xi32>, tensor<2x2x2xi32>) -> tensor<2x2x2xi32>
    %126 = "hivm.hir.vand"(%125, %123, %23) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x2x2xi32>, tensor<2x2x2xi32>, tensor<2x2x2xi32>) -> tensor<2x2x2xi32>
    %127 = "hivm.hir.bitcast"(%126) : (tensor<2x2x2xi32>) -> tensor<2x2x2xf32>
    %128 = "hivm.hir.vcmp"(%118, %127, %65) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<gt>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<2x2x2xf32>, tensor<2x2x2xf32>, tensor<2x2x2xi1>) -> tensor<2x2x2xi1>
    %129 = "tensor.empty"() : () -> tensor<2x1x2xi32>
    %130 = "tensor.empty"() : () -> tensor<2x1x2xi32>
    %131 = "tensor.empty"() : () -> tensor<2x1x2xi32>
    %132 = "tensor.empty"() : () -> tensor<2x1x2xi32>
    %133 = "tensor.empty"() : () -> tensor<2x1x2xi32>
    %134 = "tensor.expand_shape"(%18) <{reassociation = [[0, 1, 2]], static_output_shape = array<i64: 2, 1, 1>}> : (tensor<2xi32>) -> tensor<2x1x1xi32>
    %135 = "hivm.hir.vbrc"(%134, %133) <{broadcast_dims = array<i64: 2>}> : (tensor<2x1x1xi32>, tensor<2x1x2xi32>) -> tensor<2x1x2xi32>
    %136 = "tensor.expand_shape"(%18) <{reassociation = [[0, 1, 2]], static_output_shape = array<i64: 1, 1, 2>}> : (tensor<2xi32>) -> tensor<1x1x2xi32>
    %137 = "hivm.hir.vbrc"(%136, %132) <{broadcast_dims = array<i64: 0>}> : (tensor<1x1x2xi32>, tensor<2x1x2xi32>) -> tensor<2x1x2xi32>
    %138 = "hivm.hir.vor"(%135, %137, %131) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x1x2xi32>, tensor<2x1x2xi32>, tensor<2x1x2xi32>) -> tensor<2x1x2xi32>
    %139 = "hivm.hir.vand"(%135, %137, %130) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x1x2xi32>, tensor<2x1x2xi32>, tensor<2x1x2xi32>) -> tensor<2x1x2xi32>
    %140 = "hivm.hir.vnot"(%139, %139) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<2x1x2xi32>, tensor<2x1x2xi32>) -> tensor<2x1x2xi32>
    %141 = "hivm.hir.vand"(%140, %138, %129) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x1x2xi32>, tensor<2x1x2xi32>, tensor<2x1x2xi32>) -> tensor<2x1x2xi32>
    %142 = "hivm.hir.vcast"(%128, %22) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<2x2x2xi1>, tensor<2x2x2xi32>) -> tensor<2x2x2xi32>
    %143 = "hivm.hir.vbrc"(%141, %21) <{broadcast_dims = array<i64: 1>}> : (tensor<2x1x2xi32>, tensor<2x2x2xi32>) -> tensor<2x2x2xi32>
    %144 = "hivm.hir.vcmp"(%142, %143, %64) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<2x2x2xi32>, tensor<2x2x2xi32>, tensor<2x2x2xi1>) -> tensor<2x2x2xi1>
    %145 = "hivm.hir.vnot"(%144, %63) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<2x2x2xi1>, tensor<2x2x2xi1>) -> tensor<2x2x2xi1>
    %146 = "hivm.hir.vsel"(%145, %127, %118, %59) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<2x2x2xi1>, tensor<2x2x2xf32>, tensor<2x2x2xf32>, tensor<2x2x2xf32>) -> tensor<2x2x2xf32>
    %147 = "tensor.empty"() : () -> tensor<2x2xf32>
    %148 = "tensor.empty"() : () -> tensor<2x2xf32>
    %149 = "tensor.empty"() : () -> tensor<1x2x2xf32>
    %150 = "hivm.hir.vreduce"(%146, %149) <{arith = #hivm.reduce_op<max>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 0>}> : (tensor<2x2x2xf32>, tensor<1x2x2xf32>) -> tensor<1x2x2xf32>
    %151 = "tensor.collapse_shape"(%150) <{reassociation = [[0, 1], [2]]}> : (tensor<1x2x2xf32>) -> tensor<2x2xf32>
    %152 = "hivm.hir.bitcast"(%151) : (tensor<2x2xf32>) -> tensor<2x2xi32>
    %153 = "tensor.empty"() : () -> tensor<1x2xi32>
    %154 = "hivm.hir.vreduce"(%152, %153) <{arith = #hivm.reduce_op<xori>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 0>}> : (tensor<2x2xi32>, tensor<1x2xi32>) -> tensor<1x2xi32>
    %155 = "hivm.hir.vbrc"(%154, %51) <{broadcast_dims = array<i64: 0>}> : (tensor<1x2xi32>, tensor<2x2xi32>) -> tensor<2x2xi32>
    %156 = "hivm.hir.vor"(%152, %155, %50) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x2xi32>, tensor<2x2xi32>, tensor<2x2xi32>) -> tensor<2x2xi32>
    %157 = "hivm.hir.vand"(%152, %155, %49) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x2xi32>, tensor<2x2xi32>, tensor<2x2xi32>) -> tensor<2x2xi32>
    %158 = "hivm.hir.vnot"(%157, %157) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<2x2xi32>, tensor<2x2xi32>) -> tensor<2x2xi32>
    %159 = "hivm.hir.vand"(%158, %156, %48) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x2xi32>, tensor<2x2xi32>, tensor<2x2xi32>) -> tensor<2x2xi32>
    %160 = "hivm.hir.bitcast"(%159) : (tensor<2x2xi32>) -> tensor<2x2xf32>
    %161 = "tensor.empty"() : () -> tensor<2x2xi1>
    %162 = "tensor.empty"() : () -> tensor<2x2xi1>
    %163 = "tensor.empty"() : () -> tensor<2x2xi1>
    %164 = "tensor.empty"() : () -> tensor<2x2xi1>
    %165 = "tensor.empty"() : () -> tensor<2x2xi1>
    %166 = "tensor.empty"() : () -> tensor<2x2xi1>
    %167 = "hivm.hir.vcmp"(%151, %160, %166) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<gt>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<2x2xf32>, tensor<2x2xf32>, tensor<2x2xi1>) -> tensor<2x2xi1>
    %168 = "tensor.empty"() : () -> tensor<2x1xi32>
    %169 = "tensor.empty"() : () -> tensor<2x1xi32>
    %170 = "tensor.empty"() : () -> tensor<2x1xi32>
    %171 = "tensor.empty"() : () -> tensor<2x1xi32>
    %172 = "hivm.hir.vbrc"(%0, %171) <{broadcast_dims = array<i64>}> : (i32, tensor<2x1xi32>) -> tensor<2x1xi32>
    %173 = "hivm.hir.vor"(%19, %172, %170) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x1xi32>, tensor<2x1xi32>, tensor<2x1xi32>) -> tensor<2x1xi32>
    %174 = "tensor.empty"() : () -> tensor<2x1xi32>
    %175 = "hivm.hir.vbrc"(%0, %174) <{broadcast_dims = array<i64>}> : (i32, tensor<2x1xi32>) -> tensor<2x1xi32>
    %176 = "hivm.hir.vand"(%19, %175, %169) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x1xi32>, tensor<2x1xi32>, tensor<2x1xi32>) -> tensor<2x1xi32>
    %177 = "hivm.hir.vnot"(%176, %176) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<2x1xi32>, tensor<2x1xi32>) -> tensor<2x1xi32>
    %178 = "hivm.hir.vand"(%177, %173, %168) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x1xi32>, tensor<2x1xi32>, tensor<2x1xi32>) -> tensor<2x1xi32>
    %179 = "hivm.hir.vcast"(%167, %47) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<2x2xi1>, tensor<2x2xi32>) -> tensor<2x2xi32>
    %180 = "hivm.hir.vbrc"(%178, %46) <{broadcast_dims = array<i64: 1>}> : (tensor<2x1xi32>, tensor<2x2xi32>) -> tensor<2x2xi32>
    %181 = "hivm.hir.vcmp"(%179, %180, %165) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<2x2xi32>, tensor<2x2xi32>, tensor<2x2xi1>) -> tensor<2x2xi1>
    %182 = "hivm.hir.vnot"(%181, %164) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<2x2xi1>, tensor<2x2xi1>) -> tensor<2x2xi1>
    %183 = "hivm.hir.vsel"(%182, %160, %151, %148) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<2x2xi1>, tensor<2x2xf32>, tensor<2x2xf32>, tensor<2x2xf32>) -> tensor<2x2xf32>
    %184 = "hivm.hir.bitcast"(%183) : (tensor<2x2xf32>) -> tensor<2x2xi32>
    %185 = "tensor.empty"() : () -> tensor<2x1xi32>
    %186 = "hivm.hir.vreduce"(%184, %185) <{arith = #hivm.reduce_op<xori>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 1>}> : (tensor<2x2xi32>, tensor<2x1xi32>) -> tensor<2x1xi32>
    %187 = "hivm.hir.vbrc"(%186, %45) <{broadcast_dims = array<i64: 1>}> : (tensor<2x1xi32>, tensor<2x2xi32>) -> tensor<2x2xi32>
    %188 = "hivm.hir.vor"(%184, %187, %44) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x2xi32>, tensor<2x2xi32>, tensor<2x2xi32>) -> tensor<2x2xi32>
    %189 = "hivm.hir.vand"(%184, %187, %43) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x2xi32>, tensor<2x2xi32>, tensor<2x2xi32>) -> tensor<2x2xi32>
    %190 = "hivm.hir.vnot"(%189, %189) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<2x2xi32>, tensor<2x2xi32>) -> tensor<2x2xi32>
    %191 = "hivm.hir.vand"(%190, %188, %42) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x2xi32>, tensor<2x2xi32>, tensor<2x2xi32>) -> tensor<2x2xi32>
    %192 = "hivm.hir.bitcast"(%191) : (tensor<2x2xi32>) -> tensor<2x2xf32>
    %193 = "hivm.hir.vcmp"(%183, %192, %163) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<gt>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<2x2xf32>, tensor<2x2xf32>, tensor<2x2xi1>) -> tensor<2x2xi1>
    %194 = "tensor.empty"() : () -> tensor<1x2xi32>
    %195 = "tensor.empty"() : () -> tensor<1x2xi32>
    %196 = "tensor.empty"() : () -> tensor<1x2xi32>
    %197 = "tensor.empty"() : () -> tensor<1x2xi32>
    %198 = "hivm.hir.vbrc"(%0, %197) <{broadcast_dims = array<i64>}> : (i32, tensor<1x2xi32>) -> tensor<1x2xi32>
    %199 = "hivm.hir.vor"(%20, %198, %196) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1x2xi32>, tensor<1x2xi32>, tensor<1x2xi32>) -> tensor<1x2xi32>
    %200 = "tensor.empty"() : () -> tensor<1x2xi32>
    %201 = "hivm.hir.vbrc"(%0, %200) <{broadcast_dims = array<i64>}> : (i32, tensor<1x2xi32>) -> tensor<1x2xi32>
    %202 = "hivm.hir.vand"(%20, %201, %195) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1x2xi32>, tensor<1x2xi32>, tensor<1x2xi32>) -> tensor<1x2xi32>
    %203 = "hivm.hir.vnot"(%202, %202) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<1x2xi32>, tensor<1x2xi32>) -> tensor<1x2xi32>
    %204 = "hivm.hir.vand"(%203, %199, %194) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1x2xi32>, tensor<1x2xi32>, tensor<1x2xi32>) -> tensor<1x2xi32>
    %205 = "hivm.hir.vcast"(%193, %41) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<2x2xi1>, tensor<2x2xi32>) -> tensor<2x2xi32>
    %206 = "hivm.hir.vbrc"(%204, %40) <{broadcast_dims = array<i64: 0>}> : (tensor<1x2xi32>, tensor<2x2xi32>) -> tensor<2x2xi32>
    %207 = "hivm.hir.vcmp"(%205, %206, %162) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<2x2xi32>, tensor<2x2xi32>, tensor<2x2xi1>) -> tensor<2x2xi1>
    %208 = "hivm.hir.vnot"(%207, %161) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<2x2xi1>, tensor<2x2xi1>) -> tensor<2x2xi1>
    %209 = "hivm.hir.vsel"(%208, %192, %183, %147) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<2x2xi1>, tensor<2x2xf32>, tensor<2x2xf32>, tensor<2x2xf32>) -> tensor<2x2xf32>
    %210 = "arith.muli"(%11, %3) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %211 = "arith.index_cast"(%210) : (i32) -> index
    %212 = "memref.reinterpret_cast"(%arg4, %211) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 2, 2>, static_strides = array<i64: 2, 1>}> : (memref<?xf32>, index) -> memref<2x2xf32, strided<[2, 1], offset: ?>>
    "hivm.hir.store"(%209, %212) : (tensor<2x2xf32>, memref<2x2xf32, strided<[2, 1], offset: ?>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, false, false, false]> : vector<8xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

