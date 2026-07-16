"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xf32>, memref<?xf32>, i32, i32, i32) -> (), sym_name = "cos_kernel"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xf32>, %arg4: memref<?xf32>, %arg5: i32, %arg6: i32, %arg7: i32):
    %0 = "arith.constant"() <{value = -1.000000e+00 : f32}> : () -> f32
    %1 = "arith.constant"() <{value = -0.166666582 : f32}> : () -> f32
    %2 = "arith.constant"() <{value = 8.333050e-03 : f32}> : () -> f32
    %3 = "arith.constant"() <{value = -1.98089445E-4 : f32}> : () -> f32
    %4 = "arith.constant"() <{value = 2.60492652E-6 : f32}> : () -> f32
    %5 = "arith.constant"() <{value = 1.000000e+00 : f32}> : () -> f32
    %6 = "arith.constant"() <{value = -2.000000e+00 : f32}> : () -> f32
    %7 = "arith.constant"() <{value = 4.000000e+00 : f32}> : () -> f32
    %8 = "arith.constant"() <{value = -4.37113883E-8 : f32}> : () -> f32
    %9 = "arith.constant"() <{value = 1.24467439E-13 : f32}> : () -> f32
    %10 = "arith.constant"() <{value = -1.74122761E-9 : f32}> : () -> f32
    %11 = "arith.constant"() <{value = 1.57079637 : f32}> : () -> f32
    %12 = "arith.constant"() <{value = -8.9071691E-6 : f32}> : () -> f32
    %13 = "arith.constant"() <{value = 3.14160156 : f32}> : () -> f32
    %14 = "arith.constant"() <{value = 2.048000e+03 : f32}> : () -> f32
    %15 = "arith.constant"() <{value = 4.8828125E-4 : f32}> : () -> f32
    %16 = "arith.constant"() <{value = 5.000000e-01 : f32}> : () -> f32
    %17 = "arith.constant"() <{value = 0.318309873 : f32}> : () -> f32
    %18 = "arith.constant"() <{value = 32 : index}> : () -> index
    %19 = "arith.constant"() <{value = 8 : index}> : () -> index
    %20 = "arith.constant"() <{value = 2 : i32}> : () -> i32
    %21 = "arith.constant"() <{value = 4 : i32}> : () -> i32
    %22 = "arith.constant"() <{value = 8 : i32}> : () -> i32
    "hivm.hir.set_mask_norm"() : () -> ()
    %23 = "arith.muli"(%arg5, %arg6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %24 = "arith.muli"(%23, %arg7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%24) {logical_block_num} : (i32) -> ()
    %25 = "hivm.hir.get_block_idx"() : () -> i64
    %26 = "arith.trunci"(%25) : (i64) -> i32
    %27 = "arith.remsi"(%26, %arg7) : (i32, i32) -> i32
    %28 = "arith.divsi"(%26, %arg7) : (i32, i32) -> i32
    %29 = "arith.remsi"(%28, %arg6) : (i32, i32) -> i32
    %30 = "arith.muli"(%arg7, %arg6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %31 = "arith.divsi"(%26, %30) : (i32, i32) -> i32
    %32 = "arith.remsi"(%31, %arg5) : (i32, i32) -> i32
    %33 = "arith.muli"(%32, %20) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %34 = "arith.muli"(%29, %21) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %35 = "arith.muli"(%27, %22) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %36 = "arith.index_cast"(%33) : (i32) -> index
    %37 = "arith.muli"(%36, %18) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %38 = "arith.index_cast"(%34) : (i32) -> index
    %39 = "arith.muli"(%38, %19) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %40 = "arith.index_cast"(%35) : (i32) -> index
    %41 = "arith.addi"(%37, %39) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %42 = "arith.addi"(%41, %40) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %43 = "memref.reinterpret_cast"(%arg4, %42) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 2, 4, 8>, static_strides = array<i64: 32, 8, 1>}> : (memref<?xf32>, index) -> memref<2x4x8xf32, strided<[32, 8, 1], offset: ?>>
    %44 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<2x4x8xf32>
    "hivm.hir.load"(%43, %44) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<2x4x8xf32, strided<[32, 8, 1], offset: ?>>, memref<2x4x8xf32>) -> ()
    %45 = "bufferization.to_tensor"(%44) <{restrict, writable}> : (memref<2x4x8xf32>) -> tensor<2x4x8xf32>
    %46 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %47 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %48 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %49 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %50 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %51 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %52 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %53 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %54 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %55 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %56 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %57 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %58 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %59 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %60 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %61 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %62 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %63 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %64 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %65 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %66 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %67 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %68 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %69 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %70 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %71 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %72 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %73 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %74 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %75 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %76 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %77 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %78 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %79 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %80 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %81 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %82 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %83 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %84 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %85 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %86 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %87 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %88 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %89 = "tensor.empty"() : () -> tensor<2x4x8xf32>
    %90 = "hivm.hir.vmul"(%45, %17, %89) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, f32, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %91 = "hivm.hir.vadd"(%90, %16, %88) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, f32, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %92 = "hivm.hir.vmul"(%90, %15, %87) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, f32, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %93 = "hivm.hir.vcast"(%91, %86) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<round>, transpose = array<i64>}> : (tensor<2x4x8xf32>, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %94 = "hivm.hir.vcast"(%92, %85) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<round>, transpose = array<i64>}> : (tensor<2x4x8xf32>, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %95 = "hivm.hir.vmul"(%94, %14, %84) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, f32, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %96 = "hivm.hir.vsub"(%93, %95, %83) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, tensor<2x4x8xf32>, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %97 = "hivm.hir.vmul"(%95, %13, %82) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, f32, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %98 = "hivm.hir.vsub"(%45, %97, %81) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, tensor<2x4x8xf32>, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %99 = "hivm.hir.vmul"(%96, %13, %80) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, f32, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %100 = "hivm.hir.vsub"(%98, %99, %79) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, tensor<2x4x8xf32>, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %101 = "hivm.hir.vmul"(%95, %12, %78) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, f32, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %102 = "hivm.hir.vsub"(%100, %101, %77) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, tensor<2x4x8xf32>, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %103 = "hivm.hir.vadd"(%102, %11, %76) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, f32, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %104 = "hivm.hir.vmul"(%96, %12, %75) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, f32, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %105 = "hivm.hir.vsub"(%103, %104, %74) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, tensor<2x4x8xf32>, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %106 = "hivm.hir.vmul"(%95, %10, %73) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, f32, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %107 = "hivm.hir.vsub"(%105, %106, %72) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, tensor<2x4x8xf32>, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %108 = "hivm.hir.vmul"(%96, %10, %71) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, f32, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %109 = "hivm.hir.vsub"(%107, %108, %70) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, tensor<2x4x8xf32>, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %110 = "hivm.hir.vmul"(%95, %9, %69) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, f32, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %111 = "hivm.hir.vsub"(%109, %110, %68) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, tensor<2x4x8xf32>, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %112 = "hivm.hir.vmul"(%96, %9, %67) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, f32, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %113 = "hivm.hir.vsub"(%111, %112, %66) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, tensor<2x4x8xf32>, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %114 = "hivm.hir.vadd"(%113, %8, %65) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, f32, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %115 = "hivm.hir.vmul"(%93, %16, %64) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, f32, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %116 = "hivm.hir.vcast"(%115, %63) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<floor>, transpose = array<i64>}> : (tensor<2x4x8xf32>, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %117 = "hivm.hir.vmul"(%116, %7, %62) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, f32, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %118 = "hivm.hir.vmul"(%93, %6, %61) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, f32, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %119 = "hivm.hir.vadd"(%117, %118, %60) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, tensor<2x4x8xf32>, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %120 = "hivm.hir.vadd"(%119, %5, %59) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, f32, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %121 = "hivm.hir.vmul"(%114, %114, %58) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, tensor<2x4x8xf32>, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %122 = "hivm.hir.vmul"(%121, %4, %57) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, f32, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %123 = "hivm.hir.vadd"(%122, %3, %56) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, f32, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %124 = "hivm.hir.vmul"(%123, %121, %55) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, tensor<2x4x8xf32>, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %125 = "hivm.hir.vadd"(%124, %2, %54) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, f32, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %126 = "hivm.hir.vmul"(%125, %121, %53) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, tensor<2x4x8xf32>, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %127 = "hivm.hir.vadd"(%126, %1, %52) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, f32, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %128 = "hivm.hir.vmul"(%127, %121, %51) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, tensor<2x4x8xf32>, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %129 = "hivm.hir.vadd"(%128, %5, %50) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, f32, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %130 = "hivm.hir.vmul"(%129, %114, %49) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, tensor<2x4x8xf32>, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %131 = "hivm.hir.vmul"(%130, %120, %48) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, tensor<2x4x8xf32>, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %132 = "hivm.hir.vmin"(%131, %5, %47) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, f32, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %133 = "hivm.hir.vmax"(%132, %0, %46) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<2x4x8xf32>, f32, tensor<2x4x8xf32>) -> tensor<2x4x8xf32>
    %134 = "memref.reinterpret_cast"(%arg3, %42) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 2, 4, 8>, static_strides = array<i64: 32, 8, 1>}> : (memref<?xf32>, index) -> memref<2x4x8xf32, strided<[32, 8, 1], offset: ?>>
    "hivm.hir.store"(%133, %134) : (tensor<2x4x8xf32>, memref<2x4x8xf32, strided<[32, 8, 1], offset: ?>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, false, false, false]> : vector<8xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

