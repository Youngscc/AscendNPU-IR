#map = affine_map<()[s0] -> (s0, 0)>
#map1 = affine_map<()[s0] -> (s0, 64)>
#map2 = affine_map<()[s0, s1] -> (s0 * s1)>
#map3 = affine_map<()[s0, s1] -> (s0 + s1)>
#map4 = affine_map<()[s0] -> (s0 + 32)>
#map5 = affine_map<()[s0, s1] -> (s0, s1)>
#map6 = affine_map<()[s0, s1] -> (s0 - s1)>
#map7 = affine_map<()[s0] -> (s0, 32)>
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
    "annotation.mark"(%16) <{effects = ["write"]}> {logical_block_num} : (i32) -> ()
    %17 = "hivm.hir.get_block_idx"() : () -> i64
    %18 = "arith.trunci"(%17) : (i64) -> i32
    %19 = "arith.muli"(%arg14, %arg13) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %20 = "arith.divsi"(%18, %19) : (i32, i32) -> i32
    %21 = "arith.remsi"(%20, %arg12) : (i32, i32) -> i32
    %22 = "tensor.empty"() : () -> tensor<32x64xi64>
    %23 = "tensor.empty"() : () -> tensor<32x64xf16>
    %24 = "hivm.hir.vbrc"(%2, %23) <{broadcast_dims = array<i64>}> : (f16, tensor<32x64xf16>) -> tensor<32x64xf16>
    %25 = "tensor.empty"() : () -> tensor<64x32xf16>
    %26 = "hivm.hir.vbrc"(%2, %25) <{broadcast_dims = array<i64>}> : (f16, tensor<64x32xf16>) -> tensor<64x32xf16>
    %27 = "arith.addi"(%arg6, %4) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %28 = "arith.divsi"(%27, %9) : (i32, i32) -> i32
    %29 = "arith.addi"(%arg7, %4) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %30 = "arith.divsi"(%29, %9) : (i32, i32) -> i32
    %31 = "arith.muli"(%30, %8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %32 = "arith.divsi"(%21, %31) : (i32, i32) -> i32
    %33 = "arith.muli"(%32, %8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %34 = "arith.subi"(%28, %33) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %35 = "arith.minsi"(%34, %8) : (i32, i32) -> i32
    %36 = "arith.remsi"(%21, %31) : (i32, i32) -> i32
    %37 = "arith.remsi"(%36, %35) : (i32, i32) -> i32
    %38 = "arith.addi"(%33, %37) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %39 = "arith.divsi"(%36, %35) : (i32, i32) -> i32
    %40 = "arith.cmpi"(%38, %7) <{predicate = 5 : i64}> : (i32, i32) -> i1
    "llvm.intr.assume"(%40) : (i1) -> ()
    %41 = "arith.cmpi"(%39, %7) <{predicate = 5 : i64}> : (i32, i32) -> i1
    "llvm.intr.assume"(%41) : (i1) -> ()
    %42 = "arith.cmpi"(%arg9, %7) <{predicate = 4 : i64}> : (i32, i32) -> i1
    "llvm.intr.assume"(%42) : (i1) -> ()
    "llvm.intr.assume"(%6) : (i1) -> ()
    "llvm.intr.assume"(%6) : (i1) -> ()
    %43 = "arith.cmpi"(%arg10, %7) <{predicate = 4 : i64}> : (i32, i32) -> i1
    "llvm.intr.assume"(%43) : (i1) -> ()
    %44 = "arith.cmpi"(%arg11, %7) <{predicate = 4 : i64}> : (i32, i32) -> i1
    "llvm.intr.assume"(%44) : (i1) -> ()
    "llvm.intr.assume"(%6) : (i1) -> ()
    %45 = "arith.muli"(%38, %9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %46 = "tensor.empty"() : () -> tensor<32xi32>
    %47 = "hivm.hir.varange"(%46, %13, %11) <{operandSegmentSizes = array<i32: 1, 1, 1>}> : (tensor<32xi32>, index, index) -> tensor<32xi32>
    %48 = "hivm.hir.vadd"(%47, %45, %46) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32xi32>, i32, tensor<32xi32>) -> tensor<32xi32>
    %49 = "hivm.hir.vbrc"(%arg6, %46) <{broadcast_dims = array<i64>}> : (i32, tensor<32xi32>) -> tensor<32xi32>
    %50 = "hivm.hir.vmod"(%48, %arg6, %46) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<32xi32>, i32, tensor<32xi32>) -> tensor<32xi32>
    %51 = "tensor.empty"() : () -> tensor<32xi1>
    %52 = "hivm.hir.vcmp"(%49, %7, %51) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32xi32>, i32, tensor<32xi1>) -> tensor<32xi1>
    %53 = "hivm.hir.vsel"(%52, %1, %50, %46) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<32xi1>, i32, tensor<32xi32>, tensor<32xi32>) -> tensor<32xi32>
    %54 = "arith.muli"(%39, %9) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %55 = "hivm.hir.vadd"(%47, %54, %46) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32xi32>, i32, tensor<32xi32>) -> tensor<32xi32>
    %56 = "hivm.hir.vbrc"(%arg7, %46) <{broadcast_dims = array<i64>}> : (i32, tensor<32xi32>) -> tensor<32xi32>
    %57 = "hivm.hir.vmod"(%55, %arg7, %46) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<32xi32>, i32, tensor<32xi32>) -> tensor<32xi32>
    %58 = "hivm.hir.vcmp"(%56, %7, %51) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32xi32>, i32, tensor<32xi1>) -> tensor<32xi1>
    %59 = "hivm.hir.vsel"(%58, %1, %57, %46) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<32xi1>, i32, tensor<32xi32>, tensor<32xi32>) -> tensor<32xi32>
    %60 = "tensor.empty"() : () -> tensor<64xi32>
    %61 = "hivm.hir.varange"(%60, %13, %11) <{operandSegmentSizes = array<i32: 1, 1, 1>}> : (tensor<64xi32>, index, index) -> tensor<64xi32>
    %62 = "tensor.expand_shape"(%53) <{reassociation = [[0, 1]], static_output_shape = array<i64: 32, 1>}> : (tensor<32xi32>) -> tensor<32x1xi32>
    %63 = "tensor.empty"() : () -> tensor<32x1xi32>
    %64 = "hivm.hir.vmul"(%62, %arg9, %63) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32x1xi32>, i32, tensor<32x1xi32>) -> tensor<32x1xi32>
    %65 = "tensor.empty"() : () -> tensor<32x64xi32>
    %66 = "hivm.hir.vbrc"(%64, %65) <{broadcast_dims = array<i64: 1>}> : (tensor<32x1xi32>, tensor<32x64xi32>) -> tensor<32x64xi32>
    %67 = "tensor.expand_shape"(%61) <{reassociation = [[0, 1]], static_output_shape = array<i64: 1, 64>}> : (tensor<64xi32>) -> tensor<1x64xi32>
    %68 = "hivm.hir.vbrc"(%67, %65) <{broadcast_dims = array<i64: 0>}> : (tensor<1x64xi32>, tensor<32x64xi32>) -> tensor<32x64xi32>
    %69 = "hivm.hir.vadd"(%66, %68, %65) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32x64xi32>, tensor<32x64xi32>, tensor<32x64xi32>) -> tensor<32x64xi32>
    %70 = "hivm.hir.vcast"(%69, %22) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<32x64xi32>, tensor<32x64xi64>) -> tensor<32x64xi64>
    %71 = "tensor.expand_shape"(%61) <{reassociation = [[0, 1]], static_output_shape = array<i64: 64, 1>}> : (tensor<64xi32>) -> tensor<64x1xi32>
    %72 = "tensor.empty"() : () -> tensor<64x1xi32>
    %73 = "hivm.hir.vmul"(%71, %arg10, %72) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x1xi32>, i32, tensor<64x1xi32>) -> tensor<64x1xi32>
    %74 = "tensor.empty"() : () -> tensor<64x32xi32>
    %75 = "hivm.hir.vbrc"(%73, %74) <{broadcast_dims = array<i64: 1>}> : (tensor<64x1xi32>, tensor<64x32xi32>) -> tensor<64x32xi32>
    %76 = "tensor.expand_shape"(%59) <{reassociation = [[0, 1]], static_output_shape = array<i64: 1, 32>}> : (tensor<32xi32>) -> tensor<1x32xi32>
    %77 = "hivm.hir.vbrc"(%76, %74) <{broadcast_dims = array<i64: 0>}> : (tensor<1x32xi32>, tensor<64x32xi32>) -> tensor<64x32xi32>
    %78 = "hivm.hir.vadd"(%75, %77, %74) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x32xi32>, tensor<64x32xi32>, tensor<64x32xi32>) -> tensor<64x32xi32>
    %79 = "tensor.empty"() : () -> tensor<64x32xi64>
    %80 = "hivm.hir.vcast"(%78, %79) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<64x32xi32>, tensor<64x32xi64>) -> tensor<64x32xi64>
    %81 = "arith.addi"(%arg8, %3) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %82 = "arith.divsi"(%81, %5) : (i32, i32) -> i32
    %83 = "arith.muli"(%arg10, %5) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %84 = "tensor.empty"() : () -> tensor<32x32xf32>
    %85:3 = "scf.for"(%7, %82, %0, %70, %80, %84) ({
    ^bb0(%arg15: i32, %arg16: tensor<32x64xi64>, %arg17: tensor<64x32xi64>, %arg18: tensor<32x32xf32>):
      %106 = "arith.muli"(%arg15, %5) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %107 = "arith.subi"(%arg8, %106) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %108 = "arith.index_cast"(%107) : (i32) -> index
      %109 = "affine.max"(%108) <{map = #map}> : (index) -> index
      %110 = "affine.min"(%109) <{map = #map1}> : (index) -> index
      %111 = "scf.for"(%13, %10, %11, %23) ({
      ^bb0(%arg23: index, %arg24: tensor<32x64xf16>):
        %137 = "affine.min"(%110) <{map = #map1}> : (index) -> index
        %138 = "scf.for"(%13, %137, %11, %arg24) ({
        ^bb0(%arg25: index, %arg26: tensor<32x64xf16>):
          %139 = "tensor.extract"(%arg16, %arg23, %arg25) {DiscreteMemAccess, "DuplicateTensorExtractForCube::visitedLabel" = 1 : i32} : (tensor<32x64xi64>, index, index) -> i64
          %140 = "arith.index_cast"(%139) : (i64) -> index
          %141 = "memref.reinterpret_cast"(%arg3, %140) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xf16>, index) -> memref<1xf16, strided<[1], offset: ?>>
          %142 = "memref.load"(%141, %13) : (memref<1xf16, strided<[1], offset: ?>>, index) -> f16
          %143 = "tensor.insert"(%142, %arg26, %arg23, %arg25) : (f16, tensor<32x64xf16>, index, index) -> tensor<32x64xf16>
          "scf.yield"(%143) {DiscreteMemAccess} : (tensor<32x64xf16>) -> ()
        }) {ExtractedLoadOrStore} : (index, index, index, tensor<32x64xf16>) -> tensor<32x64xf16>
        "scf.yield"(%138) : (tensor<32x64xf16>) -> ()
      }) {ExtractedLoadOrStore} : (index, index, index, tensor<32x64xf16>) -> tensor<32x64xf16>
      %112 = "tensor.extract_slice"(%111, %110) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: 32, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<32x64xf16>, index) -> tensor<32x?xf16>
      %113 = "tensor.insert_slice"(%112, %24, %110) <{operandSegmentSizes = array<i32: 1, 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: 32, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<32x?xf16>, tensor<32x64xf16>, index) -> tensor<32x64xf16>
      %114 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<32x64xf16>
      %115 = "bufferization.to_tensor"(%114) <{restrict, writable}> : (memref<32x64xf16>) -> tensor<32x64xf16>
      %116 = "hivm.hir.store"(%113, %115) {"inserted-store"} : (tensor<32x64xf16>, tensor<32x64xf16>) -> tensor<32x64xf16>
      %117 = "hivm.hir.load"(%116, %23) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<32x64xf16>, tensor<32x64xf16>) -> tensor<32x64xf16>
      %118 = "affine.min"(%110) <{map = #map1}> : (index) -> index
      %119 = "scf.for"(%13, %118, %11, %25) ({
      ^bb0(%arg19: index, %arg20: tensor<64x32xf16>):
        %131 = "scf.for"(%13, %10, %11, %arg20) ({
        ^bb0(%arg21: index, %arg22: tensor<64x32xf16>):
          %132 = "tensor.extract"(%arg17, %arg19, %arg21) {DiscreteMemAccess, "DuplicateTensorExtractForCube::visitedLabel" = 1 : i32} : (tensor<64x32xi64>, index, index) -> i64
          %133 = "arith.index_cast"(%132) : (i64) -> index
          %134 = "memref.reinterpret_cast"(%arg4, %133) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xf16>, index) -> memref<1xf16, strided<[1], offset: ?>>
          %135 = "memref.load"(%134, %13) : (memref<1xf16, strided<[1], offset: ?>>, index) -> f16
          %136 = "tensor.insert"(%135, %arg22, %arg19, %arg21) : (f16, tensor<64x32xf16>, index, index) -> tensor<64x32xf16>
          "scf.yield"(%136) {DiscreteMemAccess} : (tensor<64x32xf16>) -> ()
        }) {ExtractedLoadOrStore} : (index, index, index, tensor<64x32xf16>) -> tensor<64x32xf16>
        "scf.yield"(%131) : (tensor<64x32xf16>) -> ()
      }) {ExtractedLoadOrStore} : (index, index, index, tensor<64x32xf16>) -> tensor<64x32xf16>
      %120 = "tensor.extract_slice"(%119, %110) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 32>, static_strides = array<i64: 1, 1>}> : (tensor<64x32xf16>, index) -> tensor<?x32xf16>
      %121 = "tensor.insert_slice"(%120, %26, %110) <{operandSegmentSizes = array<i32: 1, 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 32>, static_strides = array<i64: 1, 1>}> : (tensor<?x32xf16>, tensor<64x32xf16>, index) -> tensor<64x32xf16>
      %122 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<64x32xf16>
      %123 = "bufferization.to_tensor"(%122) <{restrict, writable}> : (memref<64x32xf16>) -> tensor<64x32xf16>
      %124 = "hivm.hir.store"(%121, %123) {"inserted-store"} : (tensor<64x32xf16>, tensor<64x32xf16>) -> tensor<64x32xf16>
      %125 = "hivm.hir.load"(%124, %25) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<64x32xf16>, tensor<64x32xf16>) -> tensor<64x32xf16>
      %126 = "arith.cmpi"(%arg15, %7) <{predicate = 0 : i64}> : (i32, i32) -> i1
      %127 = "hivm.hir.mmadL1"(%117, %125, %126, %10, %12, %10, %arg18) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_for_result_already_inserted = true} : (tensor<32x64xf16>, tensor<64x32xf16>, i1, index, index, index, tensor<32x32xf32>) -> tensor<32x32xf32>
      %128 = "hivm.hir.vadd"(%arg16, %14, %22) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32x64xi64>, i64, tensor<32x64xi64>) -> tensor<32x64xi64>
      %129 = "arith.extsi"(%83) : (i32) -> i64
      %130 = "hivm.hir.vadd"(%arg17, %129, %79) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<64x32xi64>, i64, tensor<64x32xi64>) -> tensor<64x32xi64>
      "scf.yield"(%128, %130, %127) : (tensor<32x64xi64>, tensor<64x32xi64>, tensor<32x32xf32>) -> ()
    }) {fixpipe_for_mmad_result_already_inserted = true} : (i32, i32, i32, tensor<32x64xi64>, tensor<64x32xi64>, tensor<32x32xf32>) -> (tensor<32x64xi64>, tensor<64x32xi64>, tensor<32x32xf32>)
    %86 = "arith.index_cast"(%arg11) : (i32) -> index
    %87 = "arith.index_cast"(%45) : (i32) -> index
    %88 = "affine.apply"(%87, %86) <{map = #map2}> : (index, index) -> index
    %89 = "arith.index_cast"(%54) : (i32) -> index
    %90 = "affine.apply"(%88, %89) <{map = #map3}> : (index, index) -> index
    %91 = "memref.reinterpret_cast"(%arg5, %90, %86) <{operandSegmentSizes = array<i32: 1, 1, 0, 1>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 32, 32>, static_strides = array<i64: -9223372036854775808, 1>}> : (memref<?xf16>, index, index) -> memref<32x32xf16, strided<[?, 1], offset: ?>>
    %92 = "affine.apply"(%87) <{map = #map4}> : (index) -> index
    %93 = "arith.index_cast"(%arg6) : (i32) -> index
    %94 = "affine.max"(%87, %93) <{map = #map5}> : (index, index) -> index
    %95 = "affine.min"(%92, %94) <{map = #map5}> : (index, index) -> index
    %96 = "affine.apply"(%95, %87) <{map = #map6}> : (index, index) -> index
    %97 = "affine.apply"(%89) <{map = #map4}> : (index) -> index
    %98 = "arith.index_cast"(%arg7) : (i32) -> index
    %99 = "affine.max"(%89, %98) <{map = #map5}> : (index, index) -> index
    %100 = "affine.min"(%97, %99) <{map = #map5}> : (index, index) -> index
    %101 = "affine.apply"(%100, %89) <{map = #map6}> : (index, index) -> index
    %102 = "affine.min"(%96) <{map = #map7}> : (index) -> index
    %103 = "affine.min"(%101) <{map = #map7}> : (index) -> index
    %104 = "tensor.extract_slice"(%85#2, %102, %103) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<32x32xf32>, index, index) -> tensor<?x?xf32>
    %105 = "memref.subview"(%91, %102, %103) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<32x32xf16, strided<[?, 1], offset: ?>>, index, index) -> memref<?x?xf16, strided<[?, 1], offset: ?>>
    "hivm.hir.fixpipe"(%104, %105) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, pre_quant = #hivm.fixpipe_pre_quant_mode<F322F16>}> : (tensor<?x?xf32>, memref<?x?xf16, strided<[?, 1], offset: ?>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, false, false, false, false, false, false, false, false, false]> : vector<15xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<MIX>, mix_mode = "mix", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hacc.target = #hacc.target<"Ascend910B1">, hivm.module_core_type = #hivm.module_core_type<MIX>} : () -> ()

