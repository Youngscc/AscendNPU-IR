"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {tt.divisibility = 16 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xf16>, memref<?xf16>, memref<?xf16>, i32, i32, i32, i32, i32, i32, i32, i32, i32) -> (), sym_name = "matmul_kernel"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xf16>, %arg4: memref<?xf16>, %arg5: memref<?xf16>, %arg6: i32, %arg7: i32, %arg8: i32, %arg9: i32, %arg10: i32, %arg11: i32, %arg12: i32, %arg13: i32, %arg14: i32):
    %0 = "arith.constant"() <{value = 1 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = -1 : i32}> : () -> i32
    %2 = "arith.constant"() <{value = 0.000000e+00 : f16}> : () -> f16
    %3 = "arith.constant"() <{value = 63 : i32}> : () -> i32
    %4 = "arith.constant"() <{value = 31 : i32}> : () -> i32
    %5 = "arith.constant"() <{value = 64 : i32}> : () -> i32
    %6 = "arith.constant"() <{value = true}> : () -> i1
    %7 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %8 = "arith.constant"() <{value = 6 : i32}> : () -> i32
    %9 = "arith.constant"() <{value = 32 : i32}> : () -> i32
    %10 = "arith.constant"() <{value = 32 : index}> : () -> index
    %11 = "arith.constant"() <{value = 1 : index}> : () -> index
    %12 = "arith.constant"() <{value = 64 : index}> : () -> index
    %13 = "arith.constant"() <{value = 0 : index}> : () -> index
    %14 = "arith.constant"() <{value = 64 : i64}> : () -> i64
    "hivm.hir.set_mask_norm"() : () -> ()
    %15 = "arith.muli"(%arg12, %arg13) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %16 = "arith.muli"(%15, %arg14) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%16) {logical_block_num} : (i32) -> ()
    %17 = "hivm.hir.get_block_idx"() : () -> i64
    %18 = "arith.trunci"(%17) : (i64) -> i32
    %19 = "arith.muli"(%arg14, %arg13) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %20 = "arith.divsi"(%18, %19) : (i32, i32) -> i32
    %21 = "arith.remsi"(%20, %arg12) : (i32, i32) -> i32
    %22 = "tensor.empty"() : () -> tensor<32x64xi64>
    %23 = "tensor.empty"() : () -> tensor<32x64xi64>
    %24 = "tensor.empty"() : () -> tensor<32x64xf16>
    %25 = "hivm.hir.vbrc"(%2, %24) <{broadcast_dims = array<i64>}> : (f16, tensor<32x64xf16>) -> tensor<32x64xf16>
    %26 = "tensor.empty"() : () -> tensor<64x32xf16>
    %27 = "hivm.hir.vbrc"(%2, %26) <{broadcast_dims = array<i64>}> : (f16, tensor<64x32xf16>) -> tensor<64x32xf16>
    %28 = "arith.addi"(%arg6, %4) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %29 = "arith.divsi"(%28, %9) : (i32, i32) -> i32
    %30 = "arith.addi"(%arg7, %4) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %31 = "arith.divsi"(%30, %9) : (i32, i32) -> i32
    %32 = "arith.muli"(%31, %8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %33 = "arith.divsi"(%21, %32) : (i32, i32) -> i32
    %34 = "arith.muli"(%33, %8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %35 = "arith.subi"(%29, %34) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %36 = "arith.minsi"(%35, %8) : (i32, i32) -> i32
    %37 = "arith.remsi"(%21, %32) : (i32, i32) -> i32
    %38 = "arith.remsi"(%37, %36) : (i32, i32) -> i32
    %39 = "arith.addi"(%34, %38) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %40 = "arith.divsi"(%37, %36) : (i32, i32) -> i32
    %41 = "arith.cmpi"(%39, %7) <{predicate = 5 : i64}> : (i32, i32) -> i1
    "llvm.intr.assume"(%41) : (i1) -> ()
    %42 = "arith.cmpi"(%40, %7) <{predicate = 5 : i64}> : (i32, i32) -> i1
    "llvm.intr.assume"(%42) : (i1) -> ()
    %43 = "arith.cmpi"(%arg9, %7) <{predicate = 4 : i64}> : (i32, i32) -> i1
    "llvm.intr.assume"(%43) : (i1) -> ()
    "llvm.intr.assume"(%6) : (i1) -> ()
    "llvm.intr.assume"(%6) : (i1) -> ()
    %44 = "arith.cmpi"(%arg10, %7) <{predicate = 4 : i64}> : (i32, i32) -> i1
    "llvm.intr.assume"(%44) : (i1) -> ()
    %45 = "arith.cmpi"(%arg11, %7) <{predicate = 4 : i64}> : (i32, i32) -> i1
    "llvm.intr.assume"(%45) : (i1) -> ()
    "llvm.intr.assume"(%6) : (i1) -> ()
    %46 = "arith.muli"(%39, %9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %47 = "tensor.empty"() : () -> tensor<32xi32>
    %48 = "tensor.empty"() : () -> tensor<32xi32>
    %49 = "tensor.empty"() : () -> tensor<32xi32>
    %50 = "tensor.empty"() : () -> tensor<32xi32>
    %51 = "tensor.empty"() : () -> tensor<32xi32>
    %52 = "tensor.empty"() : () -> tensor<32xi32>
    %53 = "tensor.empty"() : () -> tensor<32xi32>
    %54 = "tensor.empty"() : () -> tensor<32xi32>
    %55 = "tensor.empty"() : () -> tensor<32xi32>
    %56 = "hivm.hir.varange"(%55, %13, %11) <{operandSegmentSizes = array<i32: 1, 1, 1>}> : (tensor<32xi32>, index, index) -> tensor<32xi32>
    %57 = "hivm.hir.vadd"(%56, %46, %54) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32xi32>, i32, tensor<32xi32>) -> tensor<32xi32>
    %58 = "hivm.hir.vbrc"(%arg6, %53) <{broadcast_dims = array<i64>}> : (i32, tensor<32xi32>) -> tensor<32xi32>
    %59 = "hivm.hir.vmod"(%57, %arg6, %52) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<32xi32>, i32, tensor<32xi32>) -> tensor<32xi32>
    %60 = "tensor.empty"() : () -> tensor<32xi1>
    %61 = "tensor.empty"() : () -> tensor<32xi1>
    %62 = "hivm.hir.vcmp"(%58, %7, %61) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<32xi32>, i32, tensor<32xi1>) -> tensor<32xi1>
    %63 = "hivm.hir.vsel"(%62, %1, %59, %51) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<32xi1>, i32, tensor<32xi32>, tensor<32xi32>) -> tensor<32xi32>
    %64 = "arith.muli"(%40, %9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %65 = "hivm.hir.vadd"(%56, %64, %50) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32xi32>, i32, tensor<32xi32>) -> tensor<32xi32>
    %66 = "hivm.hir.vbrc"(%arg7, %49) <{broadcast_dims = array<i64>}> : (i32, tensor<32xi32>) -> tensor<32xi32>
    %67 = "hivm.hir.vmod"(%65, %arg7, %48) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<32xi32>, i32, tensor<32xi32>) -> tensor<32xi32>
    %68 = "hivm.hir.vcmp"(%66, %7, %60) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<32xi32>, i32, tensor<32xi1>) -> tensor<32xi1>
    %69 = "hivm.hir.vsel"(%68, %1, %67, %47) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<32xi1>, i32, tensor<32xi32>, tensor<32xi32>) -> tensor<32xi32>
    %70 = "tensor.empty"() : () -> tensor<64xi32>
    %71 = "hivm.hir.varange"(%70, %13, %11) <{operandSegmentSizes = array<i32: 1, 1, 1>}> : (tensor<64xi32>, index, index) -> tensor<64xi32>
    %72 = "tensor.expand_shape"(%63) <{reassociation = [[0, 1]], static_output_shape = array<i64: 32, 1>}> : (tensor<32xi32>) -> tensor<32x1xi32>
    %73 = "tensor.empty"() : () -> tensor<32x1xi32>
    %74 = "hivm.hir.vmul"(%72, %arg9, %73) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32x1xi32>, i32, tensor<32x1xi32>) -> tensor<32x1xi32>
    %75 = "tensor.empty"() : () -> tensor<32x64xi32>
    %76 = "tensor.empty"() : () -> tensor<32x64xi32>
    %77 = "tensor.empty"() : () -> tensor<32x64xi32>
    %78 = "hivm.hir.vbrc"(%74, %77) <{broadcast_dims = array<i64: 1>}> : (tensor<32x1xi32>, tensor<32x64xi32>) -> tensor<32x64xi32>
    %79 = "tensor.expand_shape"(%71) <{reassociation = [[0, 1]], static_output_shape = array<i64: 1, 64>}> : (tensor<64xi32>) -> tensor<1x64xi32>
    %80 = "hivm.hir.vbrc"(%79, %76) <{broadcast_dims = array<i64: 0>}> : (tensor<1x64xi32>, tensor<32x64xi32>) -> tensor<32x64xi32>
    %81 = "hivm.hir.vadd"(%78, %80, %75) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32x64xi32>, tensor<32x64xi32>, tensor<32x64xi32>) -> tensor<32x64xi32>
    %82 = "hivm.hir.vcast"(%81, %23) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<32x64xi32>, tensor<32x64xi64>) -> tensor<32x64xi64>
    %83 = "tensor.expand_shape"(%71) <{reassociation = [[0, 1]], static_output_shape = array<i64: 64, 1>}> : (tensor<64xi32>) -> tensor<64x1xi32>
    %84 = "tensor.empty"() : () -> tensor<64x1xi32>
    %85 = "hivm.hir.vmul"(%83, %arg10, %84) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x1xi32>, i32, tensor<64x1xi32>) -> tensor<64x1xi32>
    %86 = "tensor.empty"() : () -> tensor<64x32xi32>
    %87 = "tensor.empty"() : () -> tensor<64x32xi32>
    %88 = "tensor.empty"() : () -> tensor<64x32xi32>
    %89 = "hivm.hir.vbrc"(%85, %88) <{broadcast_dims = array<i64: 1>}> : (tensor<64x1xi32>, tensor<64x32xi32>) -> tensor<64x32xi32>
    %90 = "tensor.expand_shape"(%69) <{reassociation = [[0, 1]], static_output_shape = array<i64: 1, 32>}> : (tensor<32xi32>) -> tensor<1x32xi32>
    %91 = "hivm.hir.vbrc"(%90, %87) <{broadcast_dims = array<i64: 0>}> : (tensor<1x32xi32>, tensor<64x32xi32>) -> tensor<64x32xi32>
    %92 = "hivm.hir.vadd"(%89, %91, %86) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x32xi32>, tensor<64x32xi32>, tensor<64x32xi32>) -> tensor<64x32xi32>
    %93 = "tensor.empty"() : () -> tensor<64x32xi64>
    %94 = "tensor.empty"() : () -> tensor<64x32xi64>
    %95 = "hivm.hir.vcast"(%92, %94) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<64x32xi32>, tensor<64x32xi64>) -> tensor<64x32xi64>
    %96 = "arith.addi"(%arg8, %3) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %97 = "arith.divsi"(%96, %5) : (i32, i32) -> i32
    %98 = "arith.muli"(%arg10, %5) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %99 = "tensor.empty"() : () -> tensor<32x32xf32>
    %100:3 = "scf.for"(%7, %97, %0, %82, %95, %99) ({
    ^bb0(%arg15: i32, %arg16: tensor<32x64xi64>, %arg17: tensor<64x32xi64>, %arg18: tensor<32x32xf32>):
      %121 = "arith.muli"(%arg15, %5) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %122 = "arith.subi"(%arg8, %121) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %123 = "arith.index_cast"(%122) : (i32) -> index
      %124 = "arith.maxsi"(%123, %13) : (index, index) -> index
      %125 = "arith.minsi"(%124, %12) : (index, index) -> index
      %126 = "scf.for"(%13, %10, %11, %24) ({
      ^bb0(%arg23: index, %arg24: tensor<32x64xf16>):
        %154 = "arith.minsi"(%125, %12) : (index, index) -> index
        %155 = "scf.for"(%13, %154, %11, %arg24) ({
        ^bb0(%arg25: index, %arg26: tensor<32x64xf16>):
          %156 = "tensor.extract"(%arg16, %arg23, %arg25) {DiscreteMemAccess, "DuplicateTensorExtractForCube::visitedLabel" = 1 : i32} : (tensor<32x64xi64>, index, index) -> i64
          %157 = "arith.index_cast"(%156) : (i64) -> index
          %158 = "memref.reinterpret_cast"(%arg3, %157) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xf16>, index) -> memref<1xf16, strided<[1], offset: ?>>
          %159 = "memref.load"(%158, %13) : (memref<1xf16, strided<[1], offset: ?>>, index) -> f16
          %160 = "tensor.insert"(%159, %arg26, %arg23, %arg25) : (f16, tensor<32x64xf16>, index, index) -> tensor<32x64xf16>
          "scf.yield"(%160) {DiscreteMemAccess} : (tensor<32x64xf16>) -> ()
        }) {ExtractedLoadOrStore} : (index, index, index, tensor<32x64xf16>) -> tensor<32x64xf16>
        "scf.yield"(%155) : (tensor<32x64xf16>) -> ()
      }) {ExtractedLoadOrStore} : (index, index, index, tensor<32x64xf16>) -> tensor<32x64xf16>
      %127 = "tensor.extract_slice"(%126, %125) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: 32, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<32x64xf16>, index) -> tensor<32x?xf16>
      %128 = "tensor.insert_slice"(%127, %25, %125) <{operandSegmentSizes = array<i32: 1, 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: 32, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<32x?xf16>, tensor<32x64xf16>, index) -> tensor<32x64xf16>
      %129 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<32x64xf16>
      %130 = "bufferization.to_tensor"(%129) <{restrict, writable}> : (memref<32x64xf16>) -> tensor<32x64xf16>
      %131 = "hivm.hir.store"(%128, %130) {"inserted-store"} : (tensor<32x64xf16>, tensor<32x64xf16>) -> tensor<32x64xf16>
      %132 = "tensor.empty"() : () -> tensor<32x64xf16>
      %133 = "hivm.hir.load"(%131, %132) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<32x64xf16>, tensor<32x64xf16>) -> tensor<32x64xf16>
      %134 = "arith.minsi"(%125, %12) : (index, index) -> index
      %135 = "scf.for"(%13, %134, %11, %26) ({
      ^bb0(%arg19: index, %arg20: tensor<64x32xf16>):
        %148 = "scf.for"(%13, %10, %11, %arg20) ({
        ^bb0(%arg21: index, %arg22: tensor<64x32xf16>):
          %149 = "tensor.extract"(%arg17, %arg19, %arg21) {DiscreteMemAccess, "DuplicateTensorExtractForCube::visitedLabel" = 1 : i32} : (tensor<64x32xi64>, index, index) -> i64
          %150 = "arith.index_cast"(%149) : (i64) -> index
          %151 = "memref.reinterpret_cast"(%arg4, %150) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xf16>, index) -> memref<1xf16, strided<[1], offset: ?>>
          %152 = "memref.load"(%151, %13) : (memref<1xf16, strided<[1], offset: ?>>, index) -> f16
          %153 = "tensor.insert"(%152, %arg22, %arg19, %arg21) : (f16, tensor<64x32xf16>, index, index) -> tensor<64x32xf16>
          "scf.yield"(%153) {DiscreteMemAccess} : (tensor<64x32xf16>) -> ()
        }) {ExtractedLoadOrStore} : (index, index, index, tensor<64x32xf16>) -> tensor<64x32xf16>
        "scf.yield"(%148) : (tensor<64x32xf16>) -> ()
      }) {ExtractedLoadOrStore} : (index, index, index, tensor<64x32xf16>) -> tensor<64x32xf16>
      %136 = "tensor.extract_slice"(%135, %125) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 32>, static_strides = array<i64: 1, 1>}> : (tensor<64x32xf16>, index) -> tensor<?x32xf16>
      %137 = "tensor.insert_slice"(%136, %27, %125) <{operandSegmentSizes = array<i32: 1, 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 32>, static_strides = array<i64: 1, 1>}> : (tensor<?x32xf16>, tensor<64x32xf16>, index) -> tensor<64x32xf16>
      %138 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<64x32xf16>
      %139 = "bufferization.to_tensor"(%138) <{restrict, writable}> : (memref<64x32xf16>) -> tensor<64x32xf16>
      %140 = "hivm.hir.store"(%137, %139) {"inserted-store"} : (tensor<64x32xf16>, tensor<64x32xf16>) -> tensor<64x32xf16>
      %141 = "tensor.empty"() : () -> tensor<64x32xf16>
      %142 = "hivm.hir.load"(%140, %141) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<64x32xf16>, tensor<64x32xf16>) -> tensor<64x32xf16>
      %143 = "arith.cmpi"(%arg15, %7) <{predicate = 0 : i64}> : (i32, i32) -> i1
      %144 = "hivm.hir.mmadL1"(%133, %142, %143, %10, %12, %10, %arg18) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_already_inserted = true} : (tensor<32x64xf16>, tensor<64x32xf16>, i1, index, index, index, tensor<32x32xf32>) -> tensor<32x32xf32>
      %145 = "hivm.hir.vadd"(%arg16, %14, %22) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32x64xi64>, i64, tensor<32x64xi64>) -> tensor<32x64xi64>
      %146 = "arith.extsi"(%98) : (i32) -> i64
      %147 = "hivm.hir.vadd"(%arg17, %146, %93) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x32xi64>, i64, tensor<64x32xi64>) -> tensor<64x32xi64>
      "scf.yield"(%145, %147, %144) : (tensor<32x64xi64>, tensor<64x32xi64>, tensor<32x32xf32>) -> ()
    }) : (i32, i32, i32, tensor<32x64xi64>, tensor<64x32xi64>, tensor<32x32xf32>) -> (tensor<32x64xi64>, tensor<64x32xi64>, tensor<32x32xf32>)
    %101 = "arith.index_cast"(%arg11) : (i32) -> index
    %102 = "arith.index_cast"(%46) : (i32) -> index
    %103 = "arith.muli"(%102, %101) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %104 = "arith.index_cast"(%64) : (i32) -> index
    %105 = "arith.addi"(%103, %104) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %106 = "memref.reinterpret_cast"(%arg5, %105, %101) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 32, 32>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xf16>, index, index) -> memref<32x32xf16, strided<[?, 1], offset: ?>>
    %107 = "arith.addi"(%102, %10) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %108 = "arith.index_cast"(%arg6) : (i32) -> index
    %109 = "arith.maxsi"(%102, %108) : (index, index) -> index
    %110 = "arith.minsi"(%107, %109) : (index, index) -> index
    %111 = "arith.subi"(%110, %102) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %112 = "arith.addi"(%104, %10) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %113 = "arith.index_cast"(%arg7) : (i32) -> index
    %114 = "arith.maxsi"(%104, %113) : (index, index) -> index
    %115 = "arith.minsi"(%112, %114) : (index, index) -> index
    %116 = "arith.subi"(%115, %104) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %117 = "arith.minsi"(%111, %10) : (index, index) -> index
    %118 = "arith.minsi"(%116, %10) : (index, index) -> index
    %119 = "tensor.extract_slice"(%100#2, %117, %118) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<32x32xf32>, index, index) -> tensor<?x?xf32>
    %120 = "memref.subview"(%106, %117, %118) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<32x32xf16, strided<[?, 1], offset: ?>>, index, index) -> memref<?x?xf16, strided<[?, 1], offset: ?>>
    "hivm.hir.fixpipe"(%119, %120) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, pre_quant = #hivm.fixpipe_pre_quant_mode<F322F16>}> : (tensor<?x?xf32>, memref<?x?xf16, strided<[?, 1], offset: ?>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, false, false, false, false, false, false, false, false, false]> : vector<15xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<MIX>, mix_mode = "mix", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<MIX>} : () -> ()

