"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {}, {}, {}, {}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, memref<?xf32>, memref<?xf32>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, memref<?xbf16>, memref<?xf32>, memref<?xf32>, memref<?xbf16>, memref<?xbf16>, memref<?xf32>, memref<?xf32>, memref<?xf32>, f32, i32, f32, i32, i32, i32, i32) -> (), sym_name = "chunk_kda_bwd_kernel_wy_dqkg_fused_opt_v2"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xbf16>, %arg4: memref<?xbf16>, %arg5: memref<?xbf16>, %arg6: memref<?xbf16>, %arg7: memref<?xf32>, %arg8: memref<?xf32>, %arg9: memref<?xbf16>, %arg10: memref<?xbf16>, %arg11: memref<?xbf16>, %arg12: memref<?xbf16>, %arg13: memref<?xf32>, %arg14: memref<?xf32>, %arg15: memref<?xbf16>, %arg16: memref<?xbf16>, %arg17: memref<?xf32>, %arg18: memref<?xf32>, %arg19: memref<?xf32>, %arg20: f32, %arg21: i32, %arg22: f32, %arg23: i32, %arg24: i32, %arg25: i32, %arg26: i32):
    %0 = "arith.constant"() <{value = 1 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = true}> : () -> i1
    %2 = "arith.constant"() <{value = -1.000000e+00 : f32}> : () -> f32
    %3 = "arith.constant"() <{value = 0.693147182 : f32}> : () -> f32
    %4 = "arith.constant"() <{value = 0.000000e+00 : f16}> : () -> f16
    %5 = "arith.constant"() <{value = 128 : index}> : () -> index
    %6 = "arith.constant"() <{value = 4096 : i64}> : () -> i64
    %7 = "arith.constant"() <{value = 15 : i32}> : () -> i32
    %8 = "arith.constant"() <{value = 0.000000e+00 : bf16}> : () -> bf16
    %9 = "arith.constant"() <{value = 128 : i64}> : () -> i64
    %10 = "arith.constant"() <{value = 64 : i32}> : () -> i32
    %11 = "arith.constant"() <{value = 32 : i32}> : () -> i32
    %12 = "arith.constant"() <{value = 0.000000e+00 : f32}> : () -> f32
    %13 = "arith.constant"() <{value = 128 : i32}> : () -> i32
    %14 = "arith.constant"() <{value = 64 : index}> : () -> index
    %15 = "arith.constant"() <{value = 16 : index}> : () -> index
    %16 = "arith.constant"() <{value = 32 : index}> : () -> index
    %17 = "arith.constant"() <{value = 1 : index}> : () -> index
    %18 = "arith.constant"() <{value = 0 : index}> : () -> index
    %19 = "arith.constant"() <{value = 2 : i32}> : () -> i32
    %20 = "arith.constant"() <{value = 16 : i32}> : () -> i32
    %21 = "arith.constant"() <{value = 64 : i64}> : () -> i64
    %22 = "arith.constant"() <{value = 2 : i64}> : () -> i64
    %23 = "arith.constant"() <{value = 16 : i64}> : () -> i64
    %24 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    "hivm.hir.set_mask_norm"() : () -> ()
    %25 = "arith.muli"(%arg24, %arg25) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %26 = "arith.muli"(%25, %arg26) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%26) {logical_block_num} : (i32) -> ()
    %27 = "hivm.hir.get_block_idx"() : () -> i64
    %28 = "arith.trunci"(%27) : (i64) -> i32
    %29 = "arith.divsi"(%28, %arg26) : (i32, i32) -> i32
    %30 = "arith.remsi"(%29, %arg25) : (i32, i32) -> i32
    %31 = "arith.muli"(%arg26, %arg25) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %32 = "arith.divsi"(%28, %31) : (i32, i32) -> i32
    %33 = "arith.remsi"(%32, %arg24) : (i32, i32) -> i32
    %34 = "tensor.empty"() : () -> tensor<16xi32>
    %35 = "tensor.empty"() : () -> tensor<16xi32>
    %36 = "tensor.empty"() : () -> tensor<16xi32>
    %37 = "tensor.empty"() : () -> tensor<16xi32>
    %38 = "tensor.empty"() : () -> tensor<16xi32>
    %39 = "tensor.empty"() : () -> tensor<16xf32>
    %40 = "tensor.empty"() : () -> tensor<16xf32>
    %41 = "tensor.empty"() : () -> tensor<16xf32>
    %42 = "hivm.hir.vbrc"(%12, %41) <{broadcast_dims = array<i64>}> : (f32, tensor<16xf32>) -> tensor<16xf32>
    %43 = "tensor.empty"() : () -> tensor<16x16xf32>
    %44 = "tensor.empty"() : () -> tensor<16x16xf32>
    %45 = "tensor.empty"() : () -> tensor<16x16xf32>
    %46 = "tensor.empty"() : () -> tensor<16x16xf32>
    %47 = "tensor.empty"() : () -> tensor<16x16xf32>
    %48 = "tensor.empty"() : () -> tensor<16x16xf32>
    %49 = "tensor.empty"() : () -> tensor<16x16xf32>
    %50 = "tensor.empty"() : () -> tensor<16x16xf32>
    %51 = "tensor.empty"() : () -> tensor<16x16xf32>
    %52 = "tensor.empty"() : () -> tensor<16x16xf32>
    %53 = "tensor.empty"() : () -> tensor<16x16xf32>
    %54 = "hivm.hir.vbrc"(%12, %53) <{broadcast_dims = array<i64>}> : (f32, tensor<16x16xf32>) -> tensor<16x16xf32>
    %55 = "tensor.empty"() : () -> tensor<32xi32>
    %56 = "tensor.empty"() : () -> tensor<32xi32>
    %57 = "tensor.empty"() : () -> tensor<32xi32>
    %58 = "tensor.empty"() : () -> tensor<32xi32>
    %59 = "tensor.empty"() : () -> tensor<32xf32>
    %60 = "tensor.empty"() : () -> tensor<32xf32>
    %61 = "tensor.empty"() : () -> tensor<32xf32>
    %62 = "tensor.empty"() : () -> tensor<32xf32>
    %63 = "tensor.empty"() : () -> tensor<32xf32>
    %64 = "tensor.empty"() : () -> tensor<32xf32>
    %65 = "hivm.hir.vbrc"(%12, %64) <{broadcast_dims = array<i64>}> : (f32, tensor<32xf32>) -> tensor<32xf32>
    %66 = "tensor.empty"() : () -> tensor<16x32xf32>
    %67 = "tensor.empty"() : () -> tensor<16x32xf32>
    %68 = "tensor.empty"() : () -> tensor<16x32xf32>
    %69 = "tensor.empty"() : () -> tensor<16x32xf32>
    %70 = "tensor.empty"() : () -> tensor<16x32xf32>
    %71 = "tensor.empty"() : () -> tensor<16x32xf32>
    %72 = "tensor.empty"() : () -> tensor<16x32xf32>
    %73 = "tensor.empty"() : () -> tensor<16x32xf32>
    %74 = "tensor.empty"() : () -> tensor<16x32xf32>
    %75 = "tensor.empty"() : () -> tensor<16x32xf32>
    %76 = "tensor.empty"() : () -> tensor<16x32xf32>
    %77 = "tensor.empty"() : () -> tensor<16x32xf32>
    %78 = "tensor.empty"() : () -> tensor<16x32xf32>
    %79 = "tensor.empty"() : () -> tensor<16x32xf32>
    %80 = "tensor.empty"() : () -> tensor<16x32xf32>
    %81 = "tensor.empty"() : () -> tensor<16x32xf32>
    %82 = "tensor.empty"() : () -> tensor<16x32xf32>
    %83 = "tensor.empty"() : () -> tensor<16x32xf32>
    %84 = "tensor.empty"() : () -> tensor<16x32xf32>
    %85 = "tensor.empty"() : () -> tensor<16x32xf32>
    %86 = "tensor.empty"() : () -> tensor<16x32xf32>
    %87 = "tensor.empty"() : () -> tensor<16x32xf32>
    %88 = "tensor.empty"() : () -> tensor<16x32xf32>
    %89 = "tensor.empty"() : () -> tensor<16x32xf32>
    %90 = "tensor.empty"() : () -> tensor<16x32xf32>
    %91 = "tensor.empty"() : () -> tensor<16x32xf32>
    %92 = "tensor.empty"() : () -> tensor<16x32xf32>
    %93 = "tensor.empty"() : () -> tensor<16x32xf32>
    %94 = "tensor.empty"() : () -> tensor<16x32xf32>
    %95 = "tensor.empty"() : () -> tensor<16x32xf32>
    %96 = "tensor.empty"() : () -> tensor<16x32xf32>
    %97 = "tensor.empty"() : () -> tensor<16x32xf32>
    %98 = "tensor.empty"() : () -> tensor<16x32xf32>
    %99 = "tensor.empty"() : () -> tensor<16x32xf32>
    %100 = "tensor.empty"() : () -> tensor<16x32xf32>
    %101 = "tensor.empty"() : () -> tensor<16x32xf32>
    %102 = "tensor.empty"() : () -> tensor<16x32xf32>
    %103 = "tensor.empty"() : () -> tensor<16x32xf32>
    %104 = "tensor.empty"() : () -> tensor<16x32xf32>
    %105 = "hivm.hir.vbrc"(%12, %104) <{broadcast_dims = array<i64>}> : (f32, tensor<16x32xf32>) -> tensor<16x32xf32>
    %106 = "tensor.empty"() : () -> tensor<16x32xbf16>
    %107 = "tensor.empty"() : () -> tensor<16x32xbf16>
    %108 = "tensor.empty"() : () -> tensor<16x32xbf16>
    %109 = "tensor.empty"() : () -> tensor<16x32xbf16>
    %110 = "tensor.empty"() : () -> tensor<16x32xbf16>
    %111 = "tensor.empty"() : () -> tensor<16x32xbf16>
    %112 = "tensor.empty"() : () -> tensor<16x32xbf16>
    %113 = "tensor.empty"() : () -> tensor<16x32xbf16>
    %114 = "arith.divsi"(%30, %19) : (i32, i32) -> i32
    %115 = "arith.remsi"(%30, %19) : (i32, i32) -> i32
    %116 = "arith.addi"(%arg21, %7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %117 = "arith.divsi"(%116, %20) : (i32, i32) -> i32
    %118 = "arith.muli"(%114, %117) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %119 = "arith.addi"(%118, %33) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %120 = "arith.extsi"(%119) : (i32) -> i64
    %121 = "arith.muli"(%114, %arg21) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %122 = "arith.extsi"(%121) : (i32) -> i64
    %123 = "arith.muli"(%33, %20) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %124 = "hivm.hir.varange"(%38, %18, %17) <{operandSegmentSizes = array<i32: 1, 1, 1>}> : (tensor<16xi32>, index, index) -> tensor<16xi32>
    %125 = "hivm.hir.vadd"(%124, %123, %37) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xi32>, i32, tensor<16xi32>) -> tensor<16xi32>
    %126 = "tensor.empty"() : () -> tensor<16xi1>
    %127 = "tensor.empty"() : () -> tensor<16xi1>
    %128 = "tensor.empty"() : () -> tensor<16xi1>
    %129 = "tensor.empty"() : () -> tensor<16xi1>
    %130 = "tensor.empty"() : () -> tensor<16xi1>
    %131 = "hivm.hir.vmax"(%125, %arg21, %36) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xi32>, i32, tensor<16xi32>) -> tensor<16xi32>
    %132 = "hivm.hir.vcmp"(%131, %125, %130) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<16xi32>, tensor<16xi32>, tensor<16xi1>) -> tensor<16xi1>
    %133 = "hivm.hir.vnot"(%132, %129) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16xi1>, tensor<16xi1>) -> tensor<16xi1>
    %134 = "arith.addi"(%123, %20) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %135 = "arith.minsi"(%arg21, %134) : (i32, i32) -> i32
    %136 = "arith.subi"(%135, %0) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %137 = "hivm.hir.vbrc"(%136, %35) <{broadcast_dims = array<i64>}> : (i32, tensor<16xi32>) -> tensor<16xi32>
    %138 = "hivm.hir.vcmp"(%125, %137, %128) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<16xi32>, tensor<16xi32>, tensor<16xi1>) -> tensor<16xi1>
    %139 = "arith.muli"(%115, %arg23) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %140 = "arith.extsi"(%139) : (i32) -> i64
    %141 = "arith.addi"(%140, %122) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
    %142 = "arith.muli"(%141, %21) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
    %143 = "arith.index_cast"(%142) : (i64) -> index
    %144 = "arith.muli"(%122, %22) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
    %145 = "arith.extsi"(%115) : (i32) -> i64
    %146 = "arith.addi"(%144, %145) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
    %147 = "arith.muli"(%146, %21) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
    %148 = "arith.index_cast"(%147) : (i64) -> index
    %149 = "arith.index_cast"(%141) : (i64) -> index
    %150 = "arith.muli"(%141, %23) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
    %151 = "arith.index_cast"(%150) : (i64) -> index
    %152 = "arith.muli"(%120, %22) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
    %153 = "arith.addi"(%152, %145) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
    %154 = "arith.muli"(%153, %6) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
    %155 = "arith.index_cast"(%154) : (i64) -> index
    %156 = "hivm.hir.vmax"(%125, %24, %34) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xi32>, i32, tensor<16xi32>) -> tensor<16xi32>
    %157 = "hivm.hir.vcmp"(%156, %125, %127) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<16xi32>, tensor<16xi32>, tensor<16xi1>) -> tensor<16xi1>
    %158 = "hivm.hir.vand"(%133, %157, %126) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xi1>, tensor<16xi1>, tensor<16xi1>) -> tensor<16xi1>
    %159 = "arith.index_cast"(%123) : (i32) -> index
    %160 = "arith.addi"(%149, %159) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %161 = "memref.reinterpret_cast"(%arg8, %160) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1, 16>, static_strides = array<i64: 16, 1>}> : (memref<?xf32>, index) -> memref<1x16xf32, strided<[16, 1], offset: ?>>
    %162 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<1x16xf32>
    %163 = "arith.addi"(%159, %15) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %164 = "arith.index_cast"(%arg21) : (i32) -> index
    %165 = "arith.maxsi"(%159, %164) : (index, index) -> index
    %166 = "arith.minsi"(%163, %165) : (index, index) -> index
    %167 = "arith.subi"(%166, %159) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %168 = "arith.maxsi"(%159, %18) : (index, index) -> index
    %169 = "arith.minsi"(%163, %168) : (index, index) -> index
    %170 = "arith.subi"(%169, %159) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %171 = "arith.subi"(%163, %169) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %172 = "arith.maxsi"(%170, %18) : (index, index) -> index
    %173 = "arith.addi"(%170, %171) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %174 = "arith.minsi"(%167, %173) : (index, index) -> index
    %175 = "arith.subi"(%174, %172) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %176 = "arith.cmpi"(%175, %15) <{predicate = 2 : i64}> : (index, index) -> i1
    %177 = "memref.subview"(%161, %172, %175) <{operandSegmentSizes = array<i32: 1, 1, 1, 0>, static_offsets = array<i64: 0, -9223372036854775808>, static_sizes = array<i64: 1, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<1x16xf32, strided<[16, 1], offset: ?>>, index, index) -> memref<1x?xf32, strided<[16, 1], offset: ?>>
    %178 = "memref.subview"(%162, %172, %175) <{operandSegmentSizes = array<i32: 1, 1, 1, 0>, static_offsets = array<i64: 0, -9223372036854775808>, static_sizes = array<i64: 1, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<1x16xf32>, index, index) -> memref<1x?xf32, strided<[16, 1], offset: ?>>
    "hivm.hir.load"(%177, %178, %12, %172, %176) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<1x?xf32, strided<[16, 1], offset: ?>>, memref<1x?xf32, strided<[16, 1], offset: ?>>, f32, index, i1) -> ()
    %179 = "bufferization.to_tensor"(%162) <{restrict, writable}> : (memref<1x16xf32>) -> tensor<1x16xf32>
    %180 = "arith.muli"(%159, %15) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %181 = "arith.addi"(%151, %180) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %182 = "memref.reinterpret_cast"(%arg9, %181) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 1, 16>}> : (memref<?xbf16>, index) -> memref<16x16xbf16, strided<[1, 16], offset: ?>>
    %183 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x16xbf16>
    %184 = "arith.maxsi"(%172, %18) : (index, index) -> index
    %185 = "arith.minsi"(%174, %15) : (index, index) -> index
    %186 = "arith.subi"(%185, %184) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %187 = "arith.cmpi"(%186, %15) <{predicate = 2 : i64}> : (index, index) -> i1
    %188 = "memref.subview"(%182, %184, %186) <{operandSegmentSizes = array<i32: 1, 1, 1, 0>, static_offsets = array<i64: 0, -9223372036854775808>, static_sizes = array<i64: 16, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16, strided<[1, 16], offset: ?>>, index, index) -> memref<16x?xbf16, strided<[1, 16], offset: ?>>
    %189 = "memref.subview"(%183, %184, %186) <{operandSegmentSizes = array<i32: 1, 1, 1, 0>, static_offsets = array<i64: 0, -9223372036854775808>, static_sizes = array<i64: 16, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x16xbf16>, index, index) -> memref<16x?xbf16, strided<[16, 1], offset: ?>>
    "hivm.hir.load"(%188, %189, %8, %184, %187) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = true, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<16x?xbf16, strided<[1, 16], offset: ?>>, memref<16x?xbf16, strided<[16, 1], offset: ?>>, bf16, index, i1) -> ()
    "annotation.mark"(%183) {MayImplicitTransposeWithLastAxis} : (memref<16x16xbf16>) -> ()
    %190 = "bufferization.to_tensor"(%183) <{restrict, writable}> : (memref<16x16xbf16>) -> tensor<16x16xbf16>
    %191 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xbf16>
    %192 = "bufferization.to_tensor"(%191) <{restrict, writable}> : (memref<16x16xbf16>) -> tensor<16x16xbf16>
    %193 = "hivm.hir.store"(%190, %192) {"inserted-store"} : (tensor<16x16xbf16>, tensor<16x16xbf16>) -> tensor<16x16xbf16>
    %194 = "tensor.empty"() : () -> tensor<16x16xbf16>
    %195 = "hivm.hir.load"(%193, %194) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x16xbf16>, tensor<16x16xbf16>) -> tensor<16x16xbf16>
    %196 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xbf16>
    %197 = "bufferization.to_tensor"(%196) <{restrict, writable}> : (memref<16x16xbf16>) -> tensor<16x16xbf16>
    %198 = "hivm.hir.store"(%190, %197) {"inserted-store"} : (tensor<16x16xbf16>, tensor<16x16xbf16>) -> tensor<16x16xbf16>
    %199 = "tensor.empty"() : () -> tensor<16x16xbf16>
    %200 = "hivm.hir.load"(%198, %199) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x16xbf16>, tensor<16x16xbf16>) -> tensor<16x16xbf16>
    %201 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xbf16>
    %202 = "bufferization.to_tensor"(%201) <{restrict, writable}> : (memref<16x16xbf16>) -> tensor<16x16xbf16>
    %203 = "hivm.hir.store"(%190, %202) {"inserted-store"} : (tensor<16x16xbf16>, tensor<16x16xbf16>) -> tensor<16x16xbf16>
    %204 = "tensor.empty"() : () -> tensor<16x16xbf16>
    %205 = "hivm.hir.load"(%203, %204) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x16xbf16>, tensor<16x16xbf16>) -> tensor<16x16xbf16>
    %206 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xbf16>
    %207 = "bufferization.to_tensor"(%206) <{restrict, writable}> : (memref<16x16xbf16>) -> tensor<16x16xbf16>
    %208 = "hivm.hir.store"(%190, %207) {"inserted-store"} : (tensor<16x16xbf16>, tensor<16x16xbf16>) -> tensor<16x16xbf16>
    %209 = "tensor.empty"() : () -> tensor<16x16xbf16>
    %210 = "hivm.hir.load"(%208, %209) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x16xbf16>, tensor<16x16xbf16>) -> tensor<16x16xbf16>
    "annotation.mark"(%190) {MayImplicitTransposeWithLastAxis} : (tensor<16x16xbf16>) -> ()
    %211 = "hivm.hir.varange"(%58, %18, %17) <{operandSegmentSizes = array<i32: 1, 1, 1>}> : (tensor<32xi32>, index, index) -> tensor<32xi32>
    %212 = "arith.extsi"(%136) : (i32) -> i64
    %213 = "arith.muli"(%212, %9) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
    %214 = "arith.addi"(%147, %213) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
    %215 = "arith.index_cast"(%214) : (i64) -> index
    %216 = "arith.muli"(%159, %14) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %217 = "arith.addi"(%143, %216) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %218 = "tensor.empty"() : () -> tensor<16x32xi1>
    %219 = "tensor.empty"() : () -> tensor<16x32xi1>
    %220 = "tensor.empty"() : () -> tensor<16x32xi1>
    %221 = "tensor.empty"() : () -> tensor<16x32xi1>
    %222 = "tensor.empty"() : () -> tensor<16x32xi1>
    %223 = "tensor.empty"() : () -> tensor<16xf16>
    %224 = "tensor.empty"() : () -> tensor<16xf16>
    %225 = "hivm.hir.vcast"(%158, %224) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<trunc>, transpose = array<i64>}> : (tensor<16xi1>, tensor<16xf16>) -> tensor<16xf16>
    %226 = "tensor.empty"() : () -> tensor<16x32xf16>
    %227 = "tensor.empty"() : () -> tensor<16x32xf16>
    %228 = "tensor.expand_shape"(%225) <{reassociation = [[0, 1]], static_output_shape = array<i64: 16, 1>}> : (tensor<16xf16>) -> tensor<16x1xf16>
    %229 = "hivm.hir.vbrc"(%228, %227) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xf16>, tensor<16x32xf16>) -> tensor<16x32xf16>
    %230 = "hivm.hir.vcmp"(%229, %4, %222) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<16x32xf16>, f16, tensor<16x32xi1>) -> tensor<16x32xi1>
    %231 = "hivm.hir.vnot"(%230, %221) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16x32xi1>, tensor<16x32xi1>) -> tensor<16x32xi1>
    %232 = "tensor.expand_shape"(%179) <{reassociation = [[0], [1, 2]], static_output_shape = array<i64: 1, 16, 1>}> : (tensor<1x16xf32>) -> tensor<1x16x1xf32>
    %233 = "tensor.collapse_shape"(%232) <{reassociation = [[0, 1], [2]]}> : (tensor<1x16x1xf32>) -> tensor<16x1xf32>
    %234 = "hivm.hir.vbrc"(%233, %103) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
    %235 = "tensor.expand_shape"(%138) <{reassociation = [[0, 1]], static_output_shape = array<i64: 16, 1>}> : (tensor<16xi1>) -> tensor<16x1xi1>
    %236 = "tensor.empty"() : () -> tensor<16x1xf32>
    %237 = "hivm.hir.vcast"(%235, %236) <{broadcast = array<i64>, cast = #hivm.cast<cast_unsigned>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x1xi1>, tensor<16x1xf32>) -> tensor<16x1xf32>
    %238 = "hivm.hir.vbrc"(%237, %102) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
    %239:2 = "scf.for"(%24, %19, %0, %54, %42) ({
    ^bb0(%arg27: i32, %arg28: tensor<16x16xf32>, %arg29: tensor<16xf32>):
      %329 = "arith.muli"(%arg27, %11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %330 = "arith.maxsi"(%123, %24) : (i32, i32) -> i32
      %331 = "arith.index_cast"(%330) : (i32) -> index
      %332 = "arith.maxsi"(%329, %24) : (i32, i32) -> i32
      %333 = "arith.index_cast"(%332) : (i32) -> index
      %334 = "arith.muli"(%331, %5) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %335 = "arith.addi"(%334, %148) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %336 = "arith.addi"(%335, %333) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %337 = "memref.reinterpret_cast"(%arg4, %336) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 32>, static_strides = array<i64: 128, 1>}> : (memref<?xbf16>, index) -> memref<16x32xbf16, strided<[128, 1], offset: ?>>
      %338 = "memref.reinterpret_cast"(%arg7, %336) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 32>, static_strides = array<i64: 128, 1>}> : (memref<?xf32>, index) -> memref<16x32xf32, strided<[128, 1], offset: ?>>
      %339 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x32xbf16>
      %340 = "arith.subi"(%336, %148) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %341 = "arith.divsi"(%340, %5) : (index, index) -> index
      %342 = "arith.subi"(%164, %341) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %343 = "arith.maxsi"(%342, %18) : (index, index) -> index
      %344 = "arith.minsi"(%343, %15) : (index, index) -> index
      %345 = "arith.remsi"(%340, %5) : (index, index) -> index
      %346 = "arith.subi"(%14, %345) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %347 = "arith.maxsi"(%346, %18) : (index, index) -> index
      %348 = "arith.minsi"(%347, %16) : (index, index) -> index
      %349 = "arith.subi"(%24, %123) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %350 = "arith.maxsi"(%349, %24) : (i32, i32) -> i32
      %351 = "arith.index_cast"(%350) : (i32) -> index
      %352 = "arith.minsi"(%351, %344) : (index, index) -> index
      %353 = "arith.subi"(%344, %352) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %354 = "arith.subi"(%24, %329) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %355 = "arith.maxsi"(%354, %24) : (i32, i32) -> i32
      %356 = "arith.index_cast"(%355) : (i32) -> index
      %357 = "arith.minsi"(%356, %348) : (index, index) -> index
      %358 = "arith.subi"(%348, %357) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %359 = "arith.cmpi"(%353, %15) <{predicate = 2 : i64}> : (index, index) -> i1
      %360 = "arith.cmpi"(%358, %16) <{predicate = 2 : i64}> : (index, index) -> i1
      %361 = "arith.ori"(%359, %360) : (i1, i1) -> i1
      %362 = "memref.subview"(%337, %353, %358) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x32xbf16, strided<[128, 1], offset: ?>>, index, index) -> memref<?x?xbf16, strided<[128, 1], offset: ?>>
      %363 = "memref.subview"(%339, %352, %357, %353, %358) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x32xbf16>, index, index, index, index) -> memref<?x?xbf16, strided<[32, 1], offset: ?>>
      "hivm.hir.load"(%362, %363, %8, %357, %361) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x?xbf16, strided<[128, 1], offset: ?>>, memref<?x?xbf16, strided<[32, 1], offset: ?>>, bf16, index, i1) -> ()
      %364 = "bufferization.to_tensor"(%339) <{restrict, writable}> : (memref<16x32xbf16>) -> tensor<16x32xbf16>
      %365 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x32xf32>
      %366 = "memref.subview"(%338, %353, %358) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x32xf32, strided<[128, 1], offset: ?>>, index, index) -> memref<?x?xf32, strided<[128, 1], offset: ?>>
      %367 = "memref.subview"(%365, %352, %357, %353, %358) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x32xf32>, index, index, index, index) -> memref<?x?xf32, strided<[32, 1], offset: ?>>
      "hivm.hir.load"(%366, %367, %12, %357, %361) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x?xf32, strided<[128, 1], offset: ?>>, memref<?x?xf32, strided<[32, 1], offset: ?>>, f32, index, i1) -> ()
      %368 = "bufferization.to_tensor"(%365) <{restrict, writable}> : (memref<16x32xf32>) -> tensor<16x32xf32>
      %369 = "arith.index_cast"(%329) : (i32) -> index
      %370 = "arith.addi"(%215, %369) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %371 = "memref.reinterpret_cast"(%arg7, %370) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 32>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<32xf32, strided<[1], offset: ?>>
      %372 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<32xf32>
      %373 = "arith.addi"(%369, %16) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %374 = "arith.maxsi"(%369, %14) : (index, index) -> index
      %375 = "arith.minsi"(%373, %374) : (index, index) -> index
      %376 = "arith.subi"(%375, %369) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %377 = "arith.cmpi"(%376, %16) <{predicate = 2 : i64}> : (index, index) -> i1
      %378 = "memref.subview"(%371, %376) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<32xf32, strided<[1], offset: ?>>, index) -> memref<?xf32, strided<[1], offset: ?>>
      %379 = "memref.subview"(%372, %376) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<32xf32>, index) -> memref<?xf32, strided<[1]>>
      "hivm.hir.load"(%378, %379, %12, %18, %377) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xf32, strided<[1], offset: ?>>, memref<?xf32, strided<[1]>>, f32, index, i1) -> ()
      %380 = "bufferization.to_tensor"(%372) <{restrict, writable}> : (memref<32xf32>) -> tensor<32xf32>
      %381 = "tensor.extract_slice"(%380, %376) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<32xf32>, index) -> tensor<?xf32>
      %382 = "tensor.insert_slice"(%381, %65, %376) <{operandSegmentSizes = array<i32: 1, 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<?xf32>, tensor<32xf32>, index) -> tensor<32xf32>
      %383 = "arith.cmpi"(%arg27, %24) <{predicate = 0 : i64}> : (i32, i32) -> i1
      %384:6 = "scf.for"(%24, %19, %0, %arg28, %arg29, %105, %105, %105, %65) ({
      ^bb0(%arg30: i32, %arg31: tensor<16x16xf32>, %arg32: tensor<16xf32>, %arg33: tensor<16x32xf32>, %arg34: tensor<16x32xf32>, %arg35: tensor<16x32xf32>, %arg36: tensor<32xf32>):
        %496 = "arith.muli"(%arg30, %11) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
        %497 = "hivm.hir.vadd"(%211, %496, %57) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32xi32>, i32, tensor<32xi32>) -> tensor<32xi32>
        %498 = "tensor.empty"() : () -> tensor<32xi1>
        %499 = "tensor.empty"() : () -> tensor<32xi1>
        %500 = "tensor.empty"() : () -> tensor<32xi1>
        %501 = "tensor.empty"() : () -> tensor<32xi1>
        %502 = "hivm.hir.vmax"(%497, %10, %56) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32xi32>, i32, tensor<32xi32>) -> tensor<32xi32>
        %503 = "hivm.hir.vcmp"(%502, %497, %501) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<32xi32>, tensor<32xi32>, tensor<32xi1>) -> tensor<32xi1>
        %504 = "hivm.hir.vnot"(%503, %500) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<32xi1>, tensor<32xi1>) -> tensor<32xi1>
        %505 = "hivm.hir.vmax"(%497, %24, %55) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32xi32>, i32, tensor<32xi32>) -> tensor<32xi32>
        %506 = "hivm.hir.vcmp"(%505, %497, %499) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<32xi32>, tensor<32xi32>, tensor<32xi1>) -> tensor<32xi1>
        %507 = "hivm.hir.vand"(%504, %506, %498) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32xi1>, tensor<32xi1>, tensor<32xi1>) -> tensor<32xi1>
        %508 = "arith.index_cast"(%496) : (i32) -> index
        %509 = "arith.addi"(%217, %508) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
        %510 = "memref.reinterpret_cast"(%arg6, %509) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 32>, static_strides = array<i64: 64, 1>}> : (memref<?xbf16>, index) -> memref<16x32xbf16, strided<[64, 1], offset: ?>>
        %511 = "memref.reinterpret_cast"(%arg11, %509) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 32>, static_strides = array<i64: 64, 1>}> : (memref<?xbf16>, index) -> memref<16x32xbf16, strided<[64, 1], offset: ?>>
        %512 = "tensor.empty"() : () -> tensor<32xf16>
        %513 = "hivm.hir.vcast"(%507, %512) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<trunc>, transpose = array<i64>}> : (tensor<32xi1>, tensor<32xf16>) -> tensor<32xf16>
        %514 = "tensor.expand_shape"(%513) <{reassociation = [[0, 1]], static_output_shape = array<i64: 1, 32>}> : (tensor<32xf16>) -> tensor<1x32xf16>
        %515 = "hivm.hir.vbrc"(%514, %226) <{broadcast_dims = array<i64: 0>}> : (tensor<1x32xf16>, tensor<16x32xf16>) -> tensor<16x32xf16>
        %516 = "hivm.hir.vcmp"(%515, %4, %220) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<16x32xf16>, f16, tensor<16x32xi1>) -> tensor<16x32xi1>
        %517 = "hivm.hir.vnot"(%516, %219) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16x32xi1>, tensor<16x32xi1>) -> tensor<16x32xi1>
        %518 = "hivm.hir.vand"(%231, %517, %218) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xi1>, tensor<16x32xi1>, tensor<16x32xi1>) -> tensor<16x32xi1>
        %519 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x32xbf16>
        %520 = "arith.addi"(%508, %16) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
        %521 = "arith.maxsi"(%508, %14) : (index, index) -> index
        %522 = "arith.minsi"(%520, %521) : (index, index) -> index
        %523 = "arith.subi"(%522, %508) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
        %524 = "arith.maxsi"(%508, %18) : (index, index) -> index
        %525 = "arith.minsi"(%520, %524) : (index, index) -> index
        %526 = "arith.subi"(%525, %508) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
        %527 = "arith.subi"(%520, %525) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
        %528 = "arith.maxsi"(%526, %18) : (index, index) -> index
        %529 = "arith.addi"(%526, %527) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
        %530 = "arith.minsi"(%523, %529) : (index, index) -> index
        %531 = "arith.maxsi"(%528, %18) : (index, index) -> index
        %532 = "arith.minsi"(%530, %16) : (index, index) -> index
        %533 = "arith.subi"(%532, %531) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
        %534 = "arith.cmpi"(%533, %16) <{predicate = 2 : i64}> : (index, index) -> i1
        %535 = "arith.ori"(%187, %534) : (i1, i1) -> i1
        %536 = "memref.subview"(%510, %184, %531, %186, %533) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x32xbf16, strided<[64, 1], offset: ?>>, index, index, index, index) -> memref<?x?xbf16, strided<[64, 1], offset: ?>>
        %537 = "memref.subview"(%519, %184, %531, %186, %533) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x32xbf16>, index, index, index, index) -> memref<?x?xbf16, strided<[32, 1], offset: ?>>
        "hivm.hir.load"(%536, %537, %8, %531, %535) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x?xbf16, strided<[64, 1], offset: ?>>, memref<?x?xbf16, strided<[32, 1], offset: ?>>, bf16, index, i1) -> ()
        %538 = "bufferization.to_tensor"(%519) <{restrict, writable}> : (memref<16x32xbf16>) -> tensor<16x32xbf16>
        %539 = "hivm.hir.vsel"(%518, %538, %8, %112) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<16x32xi1>, tensor<16x32xbf16>, bf16, tensor<16x32xbf16>) -> tensor<16x32xbf16>
        %540 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x32xbf16>
        %541 = "bufferization.to_tensor"(%540) <{restrict, writable}> : (memref<16x32xbf16>) -> tensor<16x32xbf16>
        %542 = "hivm.hir.store"(%539, %541) {"inserted-store"} : (tensor<16x32xbf16>, tensor<16x32xbf16>) -> tensor<16x32xbf16>
        %543 = "tensor.empty"() : () -> tensor<16x32xbf16>
        %544 = "hivm.hir.load"(%542, %543) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x32xbf16>, tensor<16x32xbf16>) -> tensor<16x32xbf16>
        %545 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x32xbf16>
        %546 = "memref.subview"(%511, %184, %531, %186, %533) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x32xbf16, strided<[64, 1], offset: ?>>, index, index, index, index) -> memref<?x?xbf16, strided<[64, 1], offset: ?>>
        %547 = "memref.subview"(%545, %184, %531, %186, %533) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x32xbf16>, index, index, index, index) -> memref<?x?xbf16, strided<[32, 1], offset: ?>>
        "hivm.hir.load"(%546, %547, %8, %531, %535) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x?xbf16, strided<[64, 1], offset: ?>>, memref<?x?xbf16, strided<[32, 1], offset: ?>>, bf16, index, i1) -> ()
        %548 = "bufferization.to_tensor"(%545) <{restrict, writable}> : (memref<16x32xbf16>) -> tensor<16x32xbf16>
        %549 = "hivm.hir.vsel"(%518, %548, %8, %111) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<16x32xi1>, tensor<16x32xbf16>, bf16, tensor<16x32xbf16>) -> tensor<16x32xbf16>
        %550 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x32xbf16>
        %551 = "bufferization.to_tensor"(%550) <{restrict, writable}> : (memref<16x32xbf16>) -> tensor<16x32xbf16>
        %552 = "hivm.hir.store"(%549, %551) {"inserted-store"} : (tensor<16x32xbf16>, tensor<16x32xbf16>) -> tensor<16x32xbf16>
        %553 = "tensor.empty"() : () -> tensor<16x32xbf16>
        %554 = "hivm.hir.load"(%552, %553) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x32xbf16>, tensor<16x32xbf16>) -> tensor<16x32xbf16>
        %555 = "tensor.empty"() : () -> tensor<32x32xbf16>
        %556 = "scf.for"(%18, %16, %17, %555) ({
        ^bb0(%arg45: index, %arg46: tensor<32x32xbf16>):
          %724 = "scf.for"(%18, %16, %17, %arg46) ({
          ^bb0(%arg47: index, %arg48: tensor<32x32xbf16>):
            %725 = "arith.index_cast"(%arg45) : (index) -> i64
            %726 = "arith.extsi"(%496) : (i32) -> i64
            %727 = "arith.addi"(%726, %725) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
            %728 = "arith.index_cast"(%arg47) : (index) -> i64
            %729 = "arith.extsi"(%329) : (i32) -> i64
            %730 = "arith.muli"(%729, %21) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
            %731 = "arith.muli"(%728, %21) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
            %732 = "arith.addi"(%727, %730) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
            %733 = "arith.addi"(%732, %731) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
            %734 = "arith.index_cast"(%733) : (i64) -> index
            %735 = "arith.addi"(%155, %734) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
            %736 = "memref.reinterpret_cast"(%arg10, %735) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xbf16>, index) -> memref<1xbf16, strided<[1], offset: ?>>
            %737 = "memref.load"(%736, %18) : (memref<1xbf16, strided<[1], offset: ?>>, index) -> bf16
            %738 = "tensor.insert"(%737, %arg48, %arg45, %arg47) : (bf16, tensor<32x32xbf16>, index, index) -> tensor<32x32xbf16>
            "scf.yield"(%738) {DiscreteMemAccess} : (tensor<32x32xbf16>) -> ()
          }) {ExtractedLoadOrStore} : (index, index, index, tensor<32x32xbf16>) -> tensor<32x32xbf16>
          "scf.yield"(%724) : (tensor<32x32xbf16>) -> ()
        }) {ExtractedLoadOrStore} : (index, index, index, tensor<32x32xbf16>) -> tensor<32x32xbf16>
        %557 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<32x32xbf16>
        %558 = "bufferization.to_tensor"(%557) <{restrict, writable}> : (memref<32x32xbf16>) -> tensor<32x32xbf16>
        %559 = "hivm.hir.store"(%556, %558) {"inserted-store"} : (tensor<32x32xbf16>, tensor<32x32xbf16>) -> tensor<32x32xbf16>
        %560 = "tensor.empty"() : () -> tensor<32x32xbf16>
        %561 = "hivm.hir.load"(%559, %560) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<32x32xbf16>, tensor<32x32xbf16>) -> tensor<32x32xbf16>
        %562 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<32x32xbf16>
        %563 = "bufferization.to_tensor"(%562) <{restrict, writable}> : (memref<32x32xbf16>) -> tensor<32x32xbf16>
        %564 = "hivm.hir.store"(%556, %563) {"inserted-store"} : (tensor<32x32xbf16>, tensor<32x32xbf16>) -> tensor<32x32xbf16>
        %565 = "tensor.empty"() : () -> tensor<32x32xbf16>
        %566 = "hivm.hir.load"(%564, %565) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<32x32xbf16>, tensor<32x32xbf16>) -> tensor<32x32xbf16>
        %567 = "scf.for"(%18, %16, %17, %555) ({
        ^bb0(%arg41: index, %arg42: tensor<32x32xbf16>):
          %709 = "scf.for"(%18, %16, %17, %arg42) ({
          ^bb0(%arg43: index, %arg44: tensor<32x32xbf16>):
            %710 = "arith.index_cast"(%arg41) : (index) -> i64
            %711 = "arith.extsi"(%496) : (i32) -> i64
            %712 = "arith.addi"(%711, %710) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
            %713 = "arith.index_cast"(%arg43) : (index) -> i64
            %714 = "arith.extsi"(%329) : (i32) -> i64
            %715 = "arith.muli"(%714, %21) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
            %716 = "arith.muli"(%713, %21) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
            %717 = "arith.addi"(%712, %715) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
            %718 = "arith.addi"(%717, %716) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
            %719 = "arith.index_cast"(%718) : (i64) -> index
            %720 = "arith.addi"(%155, %719) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
            %721 = "memref.reinterpret_cast"(%arg12, %720) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xbf16>, index) -> memref<1xbf16, strided<[1], offset: ?>>
            %722 = "memref.load"(%721, %18) : (memref<1xbf16, strided<[1], offset: ?>>, index) -> bf16
            %723 = "tensor.insert"(%722, %arg44, %arg41, %arg43) : (bf16, tensor<32x32xbf16>, index, index) -> tensor<32x32xbf16>
            "scf.yield"(%723) {DiscreteMemAccess} : (tensor<32x32xbf16>) -> ()
          }) {ExtractedLoadOrStore} : (index, index, index, tensor<32x32xbf16>) -> tensor<32x32xbf16>
          "scf.yield"(%709) : (tensor<32x32xbf16>) -> ()
        }) {ExtractedLoadOrStore} : (index, index, index, tensor<32x32xbf16>) -> tensor<32x32xbf16>
        %568 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<32x32xbf16>
        %569 = "bufferization.to_tensor"(%568) <{restrict, writable}> : (memref<32x32xbf16>) -> tensor<32x32xbf16>
        %570 = "hivm.hir.store"(%567, %569) {"inserted-store"} : (tensor<32x32xbf16>, tensor<32x32xbf16>) -> tensor<32x32xbf16>
        %571 = "tensor.empty"() : () -> tensor<32x32xbf16>
        %572 = "hivm.hir.load"(%570, %571) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<32x32xbf16>, tensor<32x32xbf16>) -> tensor<32x32xbf16>
        %573 = "arith.maxsi"(%184, %18) : (index, index) -> index
        %574 = "arith.minsi"(%573, %15) : (index, index) -> index
        %575 = "arith.addi"(%574, %186) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
        %576 = "arith.minsi"(%575, %15) : (index, index) -> index
        %577 = "scf.for"(%574, %576, %17, %113) ({
        ^bb0(%arg37: index, %arg38: tensor<16x32xbf16>):
          %691 = "arith.maxsi"(%531, %18) : (index, index) -> index
          %692 = "arith.minsi"(%691, %16) : (index, index) -> index
          %693 = "arith.addi"(%692, %533) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
          %694 = "arith.minsi"(%693, %16) : (index, index) -> index
          %695 = "scf.for"(%692, %694, %17, %arg38) ({
          ^bb0(%arg39: index, %arg40: tensor<16x32xbf16>):
            %696 = "arith.index_cast"(%arg37) : (index) -> i32
            %697 = "arith.addi"(%123, %696) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
            %698 = "arith.muli"(%697, %13) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
            %699 = "arith.extsi"(%698) : (i32) -> i64
            %700 = "arith.addi"(%147, %699) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
            %701 = "arith.index_cast"(%arg39) : (index) -> i32
            %702 = "arith.addi"(%496, %701) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
            %703 = "arith.extsi"(%702) : (i32) -> i64
            %704 = "arith.addi"(%700, %703) <{overflowFlags = #arith.overflow<none>}> : (i64, i64) -> i64
            %705 = "arith.index_cast"(%704) : (i64) -> index
            %706 = "memref.reinterpret_cast"(%arg15, %705) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xbf16>, index) -> memref<1xbf16, strided<[1], offset: ?>>
            %707 = "memref.load"(%706, %18) : (memref<1xbf16, strided<[1], offset: ?>>, index) -> bf16
            %708 = "tensor.insert"(%707, %arg40, %arg37, %arg39) : (bf16, tensor<16x32xbf16>, index, index) -> tensor<16x32xbf16>
            "scf.yield"(%708) {DiscreteMemAccess} : (tensor<16x32xbf16>) -> ()
          }) {ExtractedLoadOrStore} : (index, index, index, tensor<16x32xbf16>) -> tensor<16x32xbf16>
          "scf.yield"(%695) : (tensor<16x32xbf16>) -> ()
        }) {ExtractedLoadOrStore} : (index, index, index, tensor<16x32xbf16>) -> tensor<16x32xbf16>
        %578 = "hivm.hir.vsel"(%518, %577, %8, %110) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<16x32xi1>, tensor<16x32xbf16>, bf16, tensor<16x32xbf16>) -> tensor<16x32xbf16>
        %579 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x32xbf16>
        %580 = "bufferization.to_tensor"(%579) <{restrict, writable}> : (memref<16x32xbf16>) -> tensor<16x32xbf16>
        %581 = "hivm.hir.store"(%578, %580) {"inserted-store"} : (tensor<16x32xbf16>, tensor<16x32xbf16>) -> tensor<16x32xbf16>
        %582 = "tensor.empty"() : () -> tensor<16x32xbf16>
        %583 = "hivm.hir.load"(%581, %582) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x32xbf16>, tensor<16x32xbf16>) -> tensor<16x32xbf16>
        %584 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x32xbf16>
        %585 = "bufferization.to_tensor"(%584) <{restrict, writable}> : (memref<16x32xbf16>) -> tensor<16x32xbf16>
        %586 = "hivm.hir.store"(%578, %585) {"inserted-store"} : (tensor<16x32xbf16>, tensor<16x32xbf16>) -> tensor<16x32xbf16>
        %587 = "tensor.empty"() : () -> tensor<16x32xbf16>
        %588 = "hivm.hir.load"(%586, %587) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x32xbf16>, tensor<16x32xbf16>) -> tensor<16x32xbf16>
        %589 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x32xbf16>
        %590 = "bufferization.to_tensor"(%589) <{restrict, writable}> : (memref<16x32xbf16>) -> tensor<16x32xbf16>
        %591 = "hivm.hir.store"(%578, %590) {"inserted-store"} : (tensor<16x32xbf16>, tensor<16x32xbf16>) -> tensor<16x32xbf16>
        %592 = "tensor.empty"() : () -> tensor<16x32xbf16>
        %593 = "hivm.hir.load"(%591, %592) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x32xbf16>, tensor<16x32xbf16>) -> tensor<16x32xbf16>
        %594 = "tensor.empty"() : () -> tensor<32x32xf32>
        %595 = "tensor.empty"() : () -> tensor<32x32xf32>
        %596 = "tensor.empty"() : () -> tensor<32x32xf32>
        %597 = "hivm.hir.vcast"(%556, %596) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<32x32xbf16>, tensor<32x32xf32>) -> tensor<32x32xf32>
        %598 = "hivm.hir.vcast"(%567, %595) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<32x32xbf16>, tensor<32x32xf32>) -> tensor<32x32xf32>
        %599 = "hivm.hir.vmul"(%597, %598, %594) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32x32xf32>, tensor<32x32xf32>, tensor<32x32xf32>) -> tensor<32x32xf32>
        %600 = "tensor.empty"() : () -> tensor<1x32xf32>
        %601 = "hivm.hir.vreduce"(%599, %600) <{arith = #hivm.reduce_op<sum>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 0>}> : (tensor<32x32xf32>, tensor<1x32xf32>) -> tensor<1x32xf32>
        %602 = "tensor.collapse_shape"(%601) <{reassociation = [[0, 1]]}> : (tensor<1x32xf32>) -> tensor<32xf32>
        %603 = "hivm.hir.vadd"(%arg36, %602, %63) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32xf32>, tensor<32xf32>, tensor<32xf32>) -> tensor<32xf32>
        %604 = "tensor.empty"() : () -> tensor<16x32xf32>
        %605 = "hivm.hir.mmadL1"(%554, %561, %1, %15, %16, %16, %604) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_already_inserted = true} : (tensor<16x32xbf16>, tensor<32x32xbf16>, i1, index, index, index, tensor<16x32xf32>) -> tensor<16x32xf32>
        %606 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x32xf32>
        %607 = "bufferization.to_tensor"(%606) <{restrict, writable}> : (memref<16x32xf32>) -> tensor<16x32xf32>
        %608 = "hivm.hir.fixpipe"(%605, %607) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
        %609 = "tensor.empty"() : () -> tensor<16x32xf32>
        %610 = "hivm.hir.load"(%608, %609) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
        %611 = "hivm.hir.vmul"(%610, %arg22, %101) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, f32, tensor<16x32xf32>) -> tensor<16x32xf32>
        %612 = "hivm.hir.vadd"(%arg33, %611, %100) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
        %613 = "tensor.empty"() : () -> tensor<16x32xf32>
        %614 = "hivm.hir.mmadL1"(%544, %572, %1, %15, %16, %16, %613) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_already_inserted = true} : (tensor<16x32xbf16>, tensor<32x32xbf16>, i1, index, index, index, tensor<16x32xf32>) -> tensor<16x32xf32>
        %615 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x32xf32>
        %616 = "bufferization.to_tensor"(%615) <{restrict, writable}> : (memref<16x32xf32>) -> tensor<16x32xf32>
        %617 = "hivm.hir.fixpipe"(%614, %616) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
        %618 = "tensor.empty"() : () -> tensor<16x32xf32>
        %619 = "hivm.hir.load"(%617, %618) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
        %620 = "hivm.hir.vmul"(%619, %arg22, %99) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, f32, tensor<16x32xf32>) -> tensor<16x32xf32>
        %621 = "hivm.hir.vadd"(%arg34, %620, %98) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
        %622 = "tensor.empty"() : () -> tensor<16x32xf32>
        %623 = "hivm.hir.mmadL1"(%583, %566, %1, %15, %16, %16, %622) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_already_inserted = true} : (tensor<16x32xbf16>, tensor<32x32xbf16>, i1, index, index, index, tensor<16x32xf32>) -> tensor<16x32xf32>
        %624 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x32xf32>
        %625 = "bufferization.to_tensor"(%624) <{restrict, writable}> : (memref<16x32xf32>) -> tensor<16x32xf32>
        %626 = "hivm.hir.fixpipe"(%623, %625) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
        %627 = "tensor.empty"() : () -> tensor<16x32xf32>
        %628 = "hivm.hir.load"(%626, %627) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
        %629 = "hivm.hir.vmul"(%628, %arg22, %97) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, f32, tensor<16x32xf32>) -> tensor<16x32xf32>
        %630 = "hivm.hir.vadd"(%arg35, %629, %96) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
        "hivm.hir.pipe_barrier"() <{pipe = #hivm.pipe<PIPE_ALL>}> : () -> ()
        %631:2 = "scf.if"(%383) ({
          %634 = "arith.muli"(%159, %5) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
          %635 = "arith.addi"(%148, %634) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
          %636 = "arith.addi"(%635, %508) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
          %637 = "memref.reinterpret_cast"(%arg5, %636) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 32>, static_strides = array<i64: 128, 1>}> : (memref<?xbf16>, index) -> memref<16x32xbf16, strided<[128, 1], offset: ?>>
          %638 = "arith.maxsi"(%496, %24) : (i32, i32) -> i32
          %639 = "arith.index_cast"(%638) : (i32) -> index
          %640 = "arith.addi"(%335, %639) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
          %641 = "memref.reinterpret_cast"(%arg16, %640) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 32>, static_strides = array<i64: 128, 1>}> : (memref<?xbf16>, index) -> memref<16x32xbf16, strided<[128, 1], offset: ?>>
          %642 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x32xbf16>
          %643 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x32xbf16>
          %644 = "memref.subview"(%637, %184, %531, %186, %533) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x32xbf16, strided<[128, 1], offset: ?>>, index, index, index, index) -> memref<?x?xbf16, strided<[128, 1], offset: ?>>
          %645 = "memref.subview"(%642, %184, %531, %186, %533) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x32xbf16>, index, index, index, index) -> memref<?x?xbf16, strided<[32, 1], offset: ?>>
          %646 = "memref.subview"(%643, %184, %531, %186, %533) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x32xbf16>, index, index, index, index) -> memref<?x?xbf16, strided<[32, 1], offset: ?>>
          "hivm.hir.load"(%644, %645, %8, %531, %535) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<CUBE>}> : (memref<?x?xbf16, strided<[128, 1], offset: ?>>, memref<?x?xbf16, strided<[32, 1], offset: ?>>, bf16, index, i1) -> ()
          "hivm.hir.load"(%644, %646, %8, %531, %535) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x?xbf16, strided<[128, 1], offset: ?>>, memref<?x?xbf16, strided<[32, 1], offset: ?>>, bf16, index, i1) -> ()
          %647 = "bufferization.to_tensor"(%642) <{restrict, writable}> : (memref<16x32xbf16>) -> tensor<16x32xbf16>
          %648 = "bufferization.to_tensor"(%643) <{restrict, writable}> : (memref<16x32xbf16>) -> tensor<16x32xbf16>
          %649 = "tensor.empty"() : () -> tensor<16x16xf32>
          %650 = "hivm.hir.mmadL1"(%588, %647, %1, %15, %16, %186, %649) <{b_transpose, operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_already_inserted = true} : (tensor<16x32xbf16>, tensor<16x32xbf16>, i1, index, index, index, tensor<16x16xf32>) -> tensor<16x16xf32>
          %651 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xf32>
          %652 = "bufferization.to_tensor"(%651) <{restrict, writable}> : (memref<16x16xf32>) -> tensor<16x16xf32>
          %653 = "hivm.hir.fixpipe"(%650, %652) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
          %654 = "tensor.empty"() : () -> tensor<16x16xf32>
          %655 = "hivm.hir.load"(%653, %654) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
          %656 = "hivm.hir.vmul"(%655, %arg22, %52) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x16xf32>, f32, tensor<16x16xf32>) -> tensor<16x16xf32>
          %657 = "tensor.empty"() : () -> tensor<16x32xf32>
          %658 = "hivm.hir.mmadL1"(%195, %593, %1, %15, %15, %16, %657) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_already_inserted = true} : (tensor<16x16xbf16>, tensor<16x32xbf16>, i1, index, index, index, tensor<16x32xf32>) -> tensor<16x32xf32>
          %659 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x32xf32>
          %660 = "bufferization.to_tensor"(%659) <{restrict, writable}> : (memref<16x32xf32>) -> tensor<16x32xf32>
          %661 = "hivm.hir.fixpipe"(%658, %660) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
          %662 = "tensor.empty"() : () -> tensor<16x32xf32>
          %663 = "hivm.hir.load"(%661, %662) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
          %664 = "tensor.empty"() : () -> tensor<16x32xf32>
          %665 = "hivm.hir.load"(%661, %664) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
          %666 = "hivm.hir.vmul"(%663, %234, %95) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
          %667 = "hivm.hir.vcast"(%648, %94) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x32xbf16>, tensor<16x32xf32>) -> tensor<16x32xf32>
          %668 = "hivm.hir.vmul"(%665, %667, %93) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
          %669 = "tensor.empty"() : () -> tensor<16x1xf32>
          %670 = "hivm.hir.vreduce"(%668, %669) <{arith = #hivm.reduce_op<sum>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 1>}> : (tensor<16x32xf32>, tensor<16x1xf32>) -> tensor<16x1xf32>
          %671 = "tensor.collapse_shape"(%670) <{reassociation = [[0, 1]]}> : (tensor<16x1xf32>) -> tensor<16xf32>
          %672 = "hivm.hir.vcast"(%666, %109) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xbf16>) -> tensor<16x32xbf16>
          %673 = "arith.subi"(%640, %148) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
          %674 = "arith.divsi"(%673, %5) : (index, index) -> index
          %675 = "arith.subi"(%164, %674) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
          %676 = "arith.maxsi"(%675, %18) : (index, index) -> index
          %677 = "arith.minsi"(%676, %15) : (index, index) -> index
          %678 = "arith.remsi"(%673, %5) : (index, index) -> index
          %679 = "arith.subi"(%14, %678) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
          %680 = "arith.maxsi"(%679, %18) : (index, index) -> index
          %681 = "arith.minsi"(%680, %16) : (index, index) -> index
          %682 = "arith.minsi"(%351, %677) : (index, index) -> index
          %683 = "arith.subi"(%677, %682) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
          %684 = "arith.subi"(%24, %496) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
          %685 = "arith.maxsi"(%684, %24) : (i32, i32) -> i32
          %686 = "arith.index_cast"(%685) : (i32) -> index
          %687 = "arith.minsi"(%686, %681) : (index, index) -> index
          %688 = "arith.subi"(%681, %687) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
          %689 = "tensor.extract_slice"(%672, %682, %687, %683, %688) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<16x32xbf16>, index, index, index, index) -> tensor<?x?xbf16>
          %690 = "memref.subview"(%641, %683, %688) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x32xbf16, strided<[128, 1], offset: ?>>, index, index) -> memref<?x?xbf16, strided<[128, 1], offset: ?>>
          "hivm.hir.store"(%689, %690) : (tensor<?x?xbf16>, memref<?x?xbf16, strided<[128, 1], offset: ?>>) -> ()
          "scf.yield"(%656, %671) : (tensor<16x16xf32>, tensor<16xf32>) -> ()
        }, {
          "scf.yield"(%54, %42) : (tensor<16x16xf32>, tensor<16xf32>) -> ()
        }) : (i1) -> (tensor<16x16xf32>, tensor<16xf32>)
        %632 = "hivm.hir.vadd"(%arg31, %631#0, %51) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x16xf32>, tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
        %633 = "hivm.hir.vadd"(%arg32, %631#1, %40) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
        "scf.yield"(%632, %633, %612, %621, %630, %603) : (tensor<16x16xf32>, tensor<16xf32>, tensor<16x32xf32>, tensor<16x32xf32>, tensor<16x32xf32>, tensor<32xf32>) -> ()
      }) : (i32, i32, i32, tensor<16x16xf32>, tensor<16xf32>, tensor<16x32xf32>, tensor<16x32xf32>, tensor<16x32xf32>, tensor<32xf32>) -> (tensor<16x16xf32>, tensor<16xf32>, tensor<16x32xf32>, tensor<16x32xf32>, tensor<16x32xf32>, tensor<32xf32>)
      %385 = "hivm.hir.vmul"(%368, %3, %92) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, f32, tensor<16x32xf32>) -> tensor<16x32xf32>
      %386 = "hivm.hir.vexp"(%385, %91) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
      %387 = "hivm.hir.vmul"(%386, %234, %90) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
      %388 = "hivm.hir.vmul"(%382, %3, %62) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32xf32>, f32, tensor<32xf32>) -> tensor<32xf32>
      %389 = "hivm.hir.vexp"(%388, %61) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<32xf32>, tensor<32xf32>) -> tensor<32xf32>
      %390 = "hivm.hir.vmul"(%384#5, %389, %60) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32xf32>, tensor<32xf32>, tensor<32xf32>) -> tensor<32xf32>
      %391 = "hivm.hir.vmul"(%384#2, %386, %89) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
      %392 = "hivm.hir.vmul"(%391, %arg20, %88) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, f32, tensor<16x32xf32>) -> tensor<16x32xf32>
      %393 = "tensor.expand_shape"(%382) <{reassociation = [[0, 1]], static_output_shape = array<i64: 1, 32>}> : (tensor<32xf32>) -> tensor<1x32xf32>
      %394 = "hivm.hir.vbrc"(%393, %87) <{broadcast_dims = array<i64: 0>}> : (tensor<1x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
      %395 = "hivm.hir.vsub"(%394, %368, %86) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
      %396 = "hivm.hir.vmul"(%395, %3, %85) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, f32, tensor<16x32xf32>) -> tensor<16x32xf32>
      %397 = "hivm.hir.vexp"(%396, %84) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
      %398 = "tensor.extract_slice"(%397, %167) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 32>, static_strides = array<i64: 1, 1>}> : (tensor<16x32xf32>, index) -> tensor<?x32xf32>
      %399 = "tensor.insert_slice"(%398, %105, %167) <{operandSegmentSizes = array<i32: 1, 1, 0, 1, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, 32>, static_strides = array<i64: 1, 1>}> : (tensor<?x32xf32>, tensor<16x32xf32>, index) -> tensor<16x32xf32>
      %400 = "hivm.hir.vmul"(%384#3, %399, %83) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
      %401 = "hivm.hir.vcast"(%364, %82) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x32xbf16>, tensor<16x32xf32>) -> tensor<16x32xf32>
      %402 = "hivm.hir.vmul"(%401, %386, %81) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
      %403 = "hivm.hir.vcast"(%384#4, %108) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xbf16>) -> tensor<16x32xbf16>
      %404 = "hivm.hir.vcast"(%403, %80) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x32xbf16>, tensor<16x32xf32>) -> tensor<16x32xf32>
      %405 = "hivm.hir.vmul"(%404, %2, %79) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, f32, tensor<16x32xf32>) -> tensor<16x32xf32>
      %406 = "hivm.hir.vadd"(%405, %12, %78) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, f32, tensor<16x32xf32>) -> tensor<16x32xf32>
      %407 = "hivm.hir.vcast"(%406, %107) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xbf16>) -> tensor<16x32xbf16>
      %408 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x32xbf16>
      %409 = "bufferization.to_tensor"(%408) <{restrict, writable}> : (memref<16x32xbf16>) -> tensor<16x32xbf16>
      %410 = "hivm.hir.store"(%407, %409) {"inserted-store"} : (tensor<16x32xbf16>, tensor<16x32xbf16>) -> tensor<16x32xbf16>
      %411 = "tensor.empty"() : () -> tensor<16x32xbf16>
      %412 = "hivm.hir.load"(%410, %411) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x32xbf16>, tensor<16x32xbf16>) -> tensor<16x32xbf16>
      %413 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x32xbf16>
      %414 = "bufferization.to_tensor"(%413) <{restrict, writable}> : (memref<16x32xbf16>) -> tensor<16x32xbf16>
      %415 = "hivm.hir.store"(%407, %414) {"inserted-store"} : (tensor<16x32xbf16>, tensor<16x32xbf16>) -> tensor<16x32xbf16>
      %416 = "tensor.empty"() : () -> tensor<16x32xbf16>
      %417 = "hivm.hir.load"(%415, %416) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x32xbf16>, tensor<16x32xbf16>) -> tensor<16x32xbf16>
      %418 = "hivm.hir.vcast"(%402, %106) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xbf16>) -> tensor<16x32xbf16>
      %419 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x32xbf16>
      %420 = "bufferization.to_tensor"(%419) <{restrict, writable}> : (memref<16x32xbf16>) -> tensor<16x32xbf16>
      %421 = "hivm.hir.store"(%418, %420) {"inserted-store"} : (tensor<16x32xbf16>, tensor<16x32xbf16>) -> tensor<16x32xbf16>
      %422 = "tensor.empty"() : () -> tensor<16x32xbf16>
      %423 = "hivm.hir.load"(%421, %422) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x32xbf16>, tensor<16x32xbf16>) -> tensor<16x32xbf16>
      %424 = "tensor.empty"() : () -> tensor<16x16xf32>
      %425 = "hivm.hir.mmadL1"(%412, %423, %1, %15, %16, %15, %424) <{b_transpose, operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_already_inserted = true} : (tensor<16x32xbf16>, tensor<16x32xbf16>, i1, index, index, index, tensor<16x16xf32>) -> tensor<16x16xf32>
      %426 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xf32>
      %427 = "bufferization.to_tensor"(%426) <{restrict, writable}> : (memref<16x16xf32>) -> tensor<16x16xf32>
      %428 = "hivm.hir.fixpipe"(%425, %427) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
      %429 = "tensor.empty"() : () -> tensor<16x16xf32>
      %430 = "hivm.hir.load"(%428, %429) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
      %431 = "hivm.hir.vmul"(%430, %arg22, %50) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x16xf32>, f32, tensor<16x16xf32>) -> tensor<16x16xf32>
      %432 = "hivm.hir.vadd"(%384#0, %431, %49) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x16xf32>, tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
      %433 = "tensor.empty"() : () -> tensor<16x32xf32>
      %434 = "hivm.hir.mmadL1"(%200, %417, %1, %15, %15, %16, %433) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_already_inserted = true} : (tensor<16x16xbf16>, tensor<16x32xbf16>, i1, index, index, index, tensor<16x32xf32>) -> tensor<16x32xf32>
      %435 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x32xf32>
      %436 = "bufferization.to_tensor"(%435) <{restrict, writable}> : (memref<16x32xf32>) -> tensor<16x32xf32>
      %437 = "hivm.hir.fixpipe"(%434, %436) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
      %438 = "tensor.empty"() : () -> tensor<16x32xf32>
      %439 = "hivm.hir.load"(%437, %438) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
      %440 = "tensor.empty"() : () -> tensor<16x32xf32>
      %441 = "hivm.hir.load"(%437, %440) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
      %442 = "hivm.hir.vmul"(%439, %402, %77) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
      %443 = "tensor.empty"() : () -> tensor<16x1xf32>
      %444 = "hivm.hir.vreduce"(%442, %443) <{arith = #hivm.reduce_op<sum>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 1>}> : (tensor<16x32xf32>, tensor<16x1xf32>) -> tensor<16x1xf32>
      %445 = "tensor.collapse_shape"(%444) <{reassociation = [[0, 1]]}> : (tensor<16x1xf32>) -> tensor<16xf32>
      %446 = "hivm.hir.vadd"(%384#1, %445, %39) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16xf32>, tensor<16xf32>, tensor<16xf32>) -> tensor<16xf32>
      %447 = "arith.muli"(%331, %14) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %448 = "arith.addi"(%447, %143) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %449 = "arith.addi"(%448, %333) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %450 = "memref.reinterpret_cast"(%arg3, %449) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 32>, static_strides = array<i64: 64, 1>}> : (memref<?xbf16>, index) -> memref<16x32xbf16, strided<[64, 1], offset: ?>>
      %451 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<16x32xbf16>
      %452 = "arith.subi"(%449, %143) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %453 = "arith.divsi"(%452, %14) : (index, index) -> index
      %454 = "arith.subi"(%164, %453) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %455 = "arith.maxsi"(%454, %18) : (index, index) -> index
      %456 = "arith.minsi"(%455, %15) : (index, index) -> index
      %457 = "arith.remsi"(%452, %14) : (index, index) -> index
      %458 = "arith.subi"(%14, %457) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %459 = "arith.maxsi"(%458, %18) : (index, index) -> index
      %460 = "arith.minsi"(%459, %16) : (index, index) -> index
      %461 = "arith.minsi"(%351, %456) : (index, index) -> index
      %462 = "arith.subi"(%456, %461) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %463 = "arith.minsi"(%356, %460) : (index, index) -> index
      %464 = "arith.subi"(%460, %463) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
      %465 = "arith.cmpi"(%462, %15) <{predicate = 2 : i64}> : (index, index) -> i1
      %466 = "arith.cmpi"(%464, %16) <{predicate = 2 : i64}> : (index, index) -> i1
      %467 = "arith.ori"(%465, %466) : (i1, i1) -> i1
      %468 = "memref.subview"(%450, %462, %464) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x32xbf16, strided<[64, 1], offset: ?>>, index, index) -> memref<?x?xbf16, strided<[64, 1], offset: ?>>
      %469 = "memref.subview"(%451, %461, %463, %462, %464) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x32xbf16>, index, index, index, index) -> memref<?x?xbf16, strided<[32, 1], offset: ?>>
      "hivm.hir.load"(%468, %469, %8, %463, %467) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?x?xbf16, strided<[64, 1], offset: ?>>, memref<?x?xbf16, strided<[32, 1], offset: ?>>, bf16, index, i1) -> ()
      %470 = "bufferization.to_tensor"(%451) <{restrict, writable}> : (memref<16x32xbf16>) -> tensor<16x32xbf16>
      %471 = "hivm.hir.vmul"(%401, %400, %76) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
      %472 = "tensor.empty"() : () -> tensor<1x32xf32>
      %473 = "hivm.hir.vreduce"(%471, %472) <{arith = #hivm.reduce_op<sum>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, reduce_dims = array<i64: 0>}> : (tensor<16x32xf32>, tensor<1x32xf32>) -> tensor<1x32xf32>
      %474 = "tensor.collapse_shape"(%473) <{reassociation = [[0, 1]]}> : (tensor<1x32xf32>) -> tensor<32xf32>
      %475 = "hivm.hir.vadd"(%390, %474, %59) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<32xf32>, tensor<32xf32>, tensor<32xf32>) -> tensor<32xf32>
      %476 = "hivm.hir.vcast"(%470, %75) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x32xbf16>, tensor<16x32xf32>) -> tensor<16x32xf32>
      %477 = "hivm.hir.vmul"(%476, %392, %74) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
      %478 = "hivm.hir.vsub"(%477, %471, %73) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
      %479 = "tensor.expand_shape"(%475) <{reassociation = [[0, 1]], static_output_shape = array<i64: 1, 32>}> : (tensor<32xf32>) -> tensor<1x32xf32>
      %480 = "hivm.hir.vbrc"(%479, %72) <{broadcast_dims = array<i64: 0>}> : (tensor<1x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
      %481 = "hivm.hir.vmul"(%238, %480, %71) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
      %482 = "hivm.hir.vadd"(%478, %481, %70) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
      %483 = "hivm.hir.vmul"(%442, %234, %69) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
      %484 = "hivm.hir.vadd"(%482, %483, %68) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
      %485 = "hivm.hir.vmul"(%441, %387, %67) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
      %486 = "hivm.hir.vadd"(%400, %485, %66) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x32xf32>, tensor<16x32xf32>, tensor<16x32xf32>) -> tensor<16x32xf32>
      %487 = "memref.reinterpret_cast"(%arg13, %336) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 32>, static_strides = array<i64: 128, 1>}> : (memref<?xf32>, index) -> memref<16x32xf32, strided<[128, 1], offset: ?>>
      %488 = "memref.reinterpret_cast"(%arg14, %336) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 32>, static_strides = array<i64: 128, 1>}> : (memref<?xf32>, index) -> memref<16x32xf32, strided<[128, 1], offset: ?>>
      %489 = "memref.reinterpret_cast"(%arg17, %336) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 32>, static_strides = array<i64: 128, 1>}> : (memref<?xf32>, index) -> memref<16x32xf32, strided<[128, 1], offset: ?>>
      %490 = "tensor.extract_slice"(%392, %352, %357, %353, %358) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<16x32xf32>, index, index, index, index) -> tensor<?x?xf32>
      %491 = "memref.subview"(%487, %353, %358) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x32xf32, strided<[128, 1], offset: ?>>, index, index) -> memref<?x?xf32, strided<[128, 1], offset: ?>>
      "hivm.hir.store"(%490, %491) : (tensor<?x?xf32>, memref<?x?xf32, strided<[128, 1], offset: ?>>) -> ()
      %492 = "tensor.extract_slice"(%486, %352, %357, %353, %358) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<16x32xf32>, index, index, index, index) -> tensor<?x?xf32>
      %493 = "memref.subview"(%488, %353, %358) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x32xf32, strided<[128, 1], offset: ?>>, index, index) -> memref<?x?xf32, strided<[128, 1], offset: ?>>
      "hivm.hir.store"(%492, %493) : (tensor<?x?xf32>, memref<?x?xf32, strided<[128, 1], offset: ?>>) -> ()
      %494 = "tensor.extract_slice"(%484, %352, %357, %353, %358) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<16x32xf32>, index, index, index, index) -> tensor<?x?xf32>
      %495 = "memref.subview"(%489, %353, %358) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x32xf32, strided<[128, 1], offset: ?>>, index, index) -> memref<?x?xf32, strided<[128, 1], offset: ?>>
      "hivm.hir.store"(%494, %495) : (tensor<?x?xf32>, memref<?x?xf32, strided<[128, 1], offset: ?>>) -> ()
      "scf.yield"(%432, %446) : (tensor<16x16xf32>, tensor<16xf32>) -> ()
    }) : (i32, i32, i32, tensor<16x16xf32>, tensor<16xf32>) -> (tensor<16x16xf32>, tensor<16xf32>)
    %240 = "tensor.empty"() : () -> tensor<16x16xi32>
    %241 = "tensor.empty"() : () -> tensor<16x16xi32>
    %242 = "tensor.empty"() : () -> tensor<16x16xi32>
    %243 = "tensor.expand_shape"(%125) <{reassociation = [[0, 1]], static_output_shape = array<i64: 16, 1>}> : (tensor<16xi32>) -> tensor<16x1xi32>
    %244 = "hivm.hir.vbrc"(%243, %242) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xi32>, tensor<16x16xi32>) -> tensor<16x16xi32>
    %245 = "tensor.expand_shape"(%125) <{reassociation = [[0, 1]], static_output_shape = array<i64: 1, 16>}> : (tensor<16xi32>) -> tensor<1x16xi32>
    %246 = "hivm.hir.vbrc"(%245, %241) <{broadcast_dims = array<i64: 0>}> : (tensor<1x16xi32>, tensor<16x16xi32>) -> tensor<16x16xi32>
    %247 = "tensor.empty"() : () -> tensor<16x16xi1>
    %248 = "tensor.empty"() : () -> tensor<16x16xi1>
    %249 = "tensor.empty"() : () -> tensor<16x16xi1>
    %250 = "tensor.empty"() : () -> tensor<16x16xi1>
    %251 = "tensor.empty"() : () -> tensor<16x16xi1>
    %252 = "tensor.empty"() : () -> tensor<16x16xi1>
    %253 = "tensor.empty"() : () -> tensor<16x16xi1>
    %254 = "tensor.empty"() : () -> tensor<16x16xi1>
    %255 = "hivm.hir.vmax"(%244, %246, %240) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x16xi32>, tensor<16x16xi32>, tensor<16x16xi32>) -> tensor<16x16xi32>
    %256 = "hivm.hir.vcmp"(%255, %246, %254) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<16x16xi32>, tensor<16x16xi32>, tensor<16x16xi1>) -> tensor<16x16xi1>
    %257 = "hivm.hir.vnot"(%256, %253) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16x16xi1>, tensor<16x16xi1>) -> tensor<16x16xi1>
    %258 = "hivm.hir.vcast"(%133, %223) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<trunc>, transpose = array<i64>}> : (tensor<16xi1>, tensor<16xf16>) -> tensor<16xf16>
    %259 = "tensor.empty"() : () -> tensor<16x16xf16>
    %260 = "tensor.empty"() : () -> tensor<16x16xf16>
    %261 = "tensor.expand_shape"(%258) <{reassociation = [[0, 1]], static_output_shape = array<i64: 16, 1>}> : (tensor<16xf16>) -> tensor<16x1xf16>
    %262 = "hivm.hir.vbrc"(%261, %260) <{broadcast_dims = array<i64: 1>}> : (tensor<16x1xf16>, tensor<16x16xf16>) -> tensor<16x16xf16>
    %263 = "hivm.hir.vcmp"(%262, %4, %252) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<16x16xf16>, f16, tensor<16x16xi1>) -> tensor<16x16xi1>
    %264 = "hivm.hir.vnot"(%263, %251) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16x16xi1>, tensor<16x16xi1>) -> tensor<16x16xi1>
    %265 = "tensor.expand_shape"(%258) <{reassociation = [[0, 1]], static_output_shape = array<i64: 1, 16>}> : (tensor<16xf16>) -> tensor<1x16xf16>
    %266 = "hivm.hir.vbrc"(%265, %259) <{broadcast_dims = array<i64: 0>}> : (tensor<1x16xf16>, tensor<16x16xf16>) -> tensor<16x16xf16>
    %267 = "hivm.hir.vcmp"(%266, %4, %250) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<16x16xf16>, f16, tensor<16x16xi1>) -> tensor<16x16xi1>
    %268 = "hivm.hir.vnot"(%267, %249) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<16x16xi1>, tensor<16x16xi1>) -> tensor<16x16xi1>
    %269 = "hivm.hir.vand"(%264, %268, %248) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x16xi1>, tensor<16x16xi1>, tensor<16x16xi1>) -> tensor<16x16xi1>
    %270 = "hivm.hir.vand"(%257, %269, %247) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x16xi1>, tensor<16x16xi1>, tensor<16x16xi1>) -> tensor<16x16xi1>
    %271 = "hivm.hir.vbrc"(%179, %48) <{broadcast_dims = array<i64: 0>}> : (tensor<1x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %272 = "hivm.hir.vmul"(%239#0, %271, %47) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x16xf32>, tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %273 = "hivm.hir.vsel"(%270, %272, %12, %46) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<16x16xi1>, tensor<16x16xf32>, f32, tensor<16x16xf32>) -> tensor<16x16xf32>
    %274 = "tensor.empty"() : () -> tensor<16x16xbf16>
    %275 = "hivm.hir.vcast"(%273, %274) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<16x16xf32>, tensor<16x16xbf16>) -> tensor<16x16xbf16>
    %276 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xbf16>
    %277 = "bufferization.to_tensor"(%276) <{restrict, writable}> : (memref<16x16xbf16>) -> tensor<16x16xbf16>
    %278 = "hivm.hir.store"(%275, %277) {"inserted-store"} : (tensor<16x16xbf16>, tensor<16x16xbf16>) -> tensor<16x16xbf16>
    %279 = "tensor.empty"() : () -> tensor<16x16xbf16>
    %280 = "hivm.hir.load"(%278, %279) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x16xbf16>, tensor<16x16xbf16>) -> tensor<16x16xbf16>
    %281 = "tensor.empty"() : () -> tensor<16x16xf32>
    %282 = "hivm.hir.mmadL1"(%280, %205, %1, %15, %15, %15, %281) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_already_inserted = true} : (tensor<16x16xbf16>, tensor<16x16xbf16>, i1, index, index, index, tensor<16x16xf32>) -> tensor<16x16xf32>
    %283 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xbf16>
    %284 = "bufferization.to_tensor"(%283) <{restrict, writable}> : (memref<16x16xbf16>) -> tensor<16x16xbf16>
    %285 = "hivm.hir.fixpipe"(%282, %284) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>, pre_quant = #hivm.fixpipe_pre_quant_mode<F322BF16>}> : (tensor<16x16xf32>, tensor<16x16xbf16>) -> tensor<16x16xbf16>
    %286 = "tensor.empty"() : () -> tensor<16x16xbf16>
    %287 = "hivm.hir.load"(%285, %286) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<CUBE>}> {"inserted-load"} : (tensor<16x16xbf16>, tensor<16x16xbf16>) -> tensor<16x16xbf16>
    %288 = "tensor.empty"() : () -> tensor<16x16xf32>
    %289 = "hivm.hir.mmadL1"(%210, %287, %1, %15, %15, %15, %288) <{operandSegmentSizes = array<i32: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0>}> {fixpipe_already_inserted = true} : (tensor<16x16xbf16>, tensor<16x16xbf16>, i1, index, index, index, tensor<16x16xf32>) -> tensor<16x16xf32>
    %290 = "memref_ext.alloc_workspace"(%arg2) <{operandSegmentSizes = array<i32: 1, 0, 0>}> : (memref<?xi8>) -> memref<16x16xf32>
    %291 = "bufferization.to_tensor"(%290) <{restrict, writable}> : (memref<16x16xf32>) -> tensor<16x16xf32>
    %292 = "hivm.hir.fixpipe"(%289, %291) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %293 = "tensor.empty"() : () -> tensor<16x16xf32>
    %294 = "hivm.hir.load"(%292, %293) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>, tcoretype = #hivm.tcore_type<VECTOR>}> {"inserted-load"} : (tensor<16x16xf32>, tensor<16x16xf32>) -> tensor<16x16xf32>
    %295 = "hivm.hir.vmul"(%294, %2, %45) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x16xf32>, f32, tensor<16x16xf32>) -> tensor<16x16xf32>
    %296 = "hivm.hir.vadd"(%295, %12, %44) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<16x16xf32>, f32, tensor<16x16xf32>) -> tensor<16x16xf32>
    %297 = "hivm.hir.vsel"(%270, %296, %12, %43) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<16x16xi1>, tensor<16x16xf32>, f32, tensor<16x16xf32>) -> tensor<16x16xf32>
    %298 = "arith.maxsi"(%123, %24) : (i32, i32) -> i32
    %299 = "arith.index_cast"(%298) : (i32) -> index
    %300 = "arith.muli"(%299, %15) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %301 = "arith.addi"(%300, %151) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %302 = "memref.reinterpret_cast"(%arg19, %301) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16, 16>, static_strides = array<i64: 16, 1>}> : (memref<?xf32>, index) -> memref<16x16xf32, strided<[16, 1], offset: ?>>
    %303 = "arith.addi"(%299, %149) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %304 = "memref.reinterpret_cast"(%arg18, %303) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 16>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<16xf32, strided<[1], offset: ?>>
    %305 = "arith.divsi"(%300, %15) : (index, index) -> index
    %306 = "arith.subi"(%164, %305) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %307 = "arith.maxsi"(%306, %18) : (index, index) -> index
    %308 = "arith.minsi"(%307, %15) : (index, index) -> index
    %309 = "arith.remsi"(%300, %15) : (index, index) -> index
    %310 = "arith.subi"(%15, %309) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %311 = "arith.maxsi"(%310, %18) : (index, index) -> index
    %312 = "arith.minsi"(%311, %15) : (index, index) -> index
    %313 = "arith.subi"(%24, %123) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %314 = "arith.maxsi"(%313, %24) : (i32, i32) -> i32
    %315 = "arith.index_cast"(%314) : (i32) -> index
    %316 = "arith.minsi"(%315, %308) : (index, index) -> index
    %317 = "arith.subi"(%308, %316) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %318 = "arith.minsi"(%312, %18) : (index, index) -> index
    %319 = "arith.subi"(%312, %318) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %320 = "tensor.extract_slice"(%297, %316, %318, %317, %319) <{operandSegmentSizes = array<i32: 1, 2, 2, 0>, static_offsets = array<i64: -9223372036854775808, -9223372036854775808>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (tensor<16x16xf32>, index, index, index, index) -> tensor<?x?xf32>
    %321 = "memref.subview"(%302, %317, %319) <{operandSegmentSizes = array<i32: 1, 0, 2, 0>, static_offsets = array<i64: 0, 0>, static_sizes = array<i64: -9223372036854775808, -9223372036854775808>, static_strides = array<i64: 1, 1>}> : (memref<16x16xf32, strided<[16, 1], offset: ?>>, index, index) -> memref<?x?xf32, strided<[16, 1], offset: ?>>
    "hivm.hir.store"(%320, %321) : (tensor<?x?xf32>, memref<?x?xf32, strided<[16, 1], offset: ?>>) -> ()
    %322 = "arith.subi"(%164, %299) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %323 = "arith.maxsi"(%322, %18) : (index, index) -> index
    %324 = "arith.minsi"(%323, %15) : (index, index) -> index
    %325 = "arith.minsi"(%315, %324) : (index, index) -> index
    %326 = "arith.subi"(%324, %325) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %327 = "tensor.extract_slice"(%239#1, %325, %326) <{operandSegmentSizes = array<i32: 1, 1, 1, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<16xf32>, index, index) -> tensor<?xf32>
    %328 = "memref.subview"(%304, %326) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<16xf32, strided<[1], offset: ?>>, index) -> memref<?xf32, strided<[1], offset: ?>>
    "hivm.hir.store"(%327, %328) : (tensor<?xf32>, memref<?xf32, strided<[1], offset: ?>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false]> : vector<27xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<MIX>, mix_mode = "mix", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<MIX>} : () -> ()

