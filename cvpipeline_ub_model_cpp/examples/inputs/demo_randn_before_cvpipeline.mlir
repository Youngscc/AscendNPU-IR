"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xf32>, i32, i32, i32) -> (), sym_name = "kernel_randn"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xf32>, %arg4: i32, %arg5: i32, %arg6: i32):
    %0 = "arith.constant"() <{value = 1 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = -1 : i32}> : () -> i32
    %2 = "arith.constant"() <{value = -1.000000e+00 : f32}> : () -> f32
    %3 = "arith.constant"() <{value = -0.166666582 : f32}> : () -> f32
    %4 = "arith.constant"() <{value = 8.333050e-03 : f32}> : () -> f32
    %5 = "arith.constant"() <{value = -1.98089445E-4 : f32}> : () -> f32
    %6 = "arith.constant"() <{value = 2.60492652E-6 : f32}> : () -> f32
    %7 = "arith.constant"() <{value = 1.000000e+00 : f32}> : () -> f32
    %8 = "arith.constant"() <{value = 4.000000e+00 : f32}> : () -> f32
    %9 = "arith.constant"() <{value = -4.37113883E-8 : f32}> : () -> f32
    %10 = "arith.constant"() <{value = 1.24467439E-13 : f32}> : () -> f32
    %11 = "arith.constant"() <{value = -1.74122761E-9 : f32}> : () -> f32
    %12 = "arith.constant"() <{value = 1.57079637 : f32}> : () -> f32
    %13 = "arith.constant"() <{value = -8.9071691E-6 : f32}> : () -> f32
    %14 = "arith.constant"() <{value = 3.14160156 : f32}> : () -> f32
    %15 = "arith.constant"() <{value = 2.048000e+03 : f32}> : () -> f32
    %16 = "arith.constant"() <{value = 4.8828125E-4 : f32}> : () -> f32
    %17 = "arith.constant"() <{value = 5.000000e-01 : f32}> : () -> f32
    %18 = "arith.constant"() <{value = 0.318309873 : f32}> : () -> f32
    %19 = "arith.constant"() <{value = 0 : index}> : () -> index
    %20 = "arith.constant"() <{value = 1 : index}> : () -> index
    %21 = "arith.constant"() <{value = 462789791 : i32}> : () -> i32
    %22 = "arith.constant"() <{value = 4 : i32}> : () -> i32
    %23 = "arith.constant"() <{value = 3 : index}> : () -> index
    %24 = "arith.constant"() <{value = -845247145 : i32}> : () -> i32
    %25 = "arith.constant"() <{value = -766435501 : i32}> : () -> i32
    %26 = "arith.constant"() <{value = -1640531522 : i32}> : () -> i32
    %27 = "arith.constant"() <{value = -1150833019 : i32}> : () -> i32
    %28 = "arith.constant"() <{value = 1013904247 : i32}> : () -> i32
    %29 = "arith.constant"() <{value = 1993301258 : i32}> : () -> i32
    %30 = "arith.constant"() <{value = -626627280 : i32}> : () -> i32
    %31 = "arith.constant"() <{value = 842468239 : i32}> : () -> i32
    %32 = "arith.constant"() <{value = 2027808489 : i32}> : () -> i32
    %33 = "arith.constant"() <{value = -308364780 : i32}> : () -> i32
    %34 = "arith.constant"() <{value = 387276962 : i32}> : () -> i32
    %35 = "arith.constant"() <{value = -1459197799 : i32}> : () -> i32
    %36 = "arith.constant"() <{value = -1253254565 : i32}> : () -> i32
    %37 = "arith.constant"() <{value = 1684936478 : i32}> : () -> i32
    %38 = "arith.constant"() <{value = 1401181204 : i32}> : () -> i32
    %39 = "arith.constant"() <{value = 534103459 : i32}> : () -> i32
    %40 = "arith.constant"() <{value = -616729560 : i32}> : () -> i32
    %41 = "arith.constant"() <{value = -1879881850 : i32}> : () -> i32
    %42 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %43 = "arith.constant"() <{value = 4.6566126E-10 : f32}> : () -> f32
    %44 = "arith.constant"() <{value = -2.000000e+00 : f32}> : () -> f32
    %45 = "arith.constant"() <{value = 6.28318548 : f32}> : () -> f32
    %46 = "arith.constant"() <{value = 1.000000e-07 : f32}> : () -> f32
    %47 = "arith.constant"() <{value = 3 : i32}> : () -> i32
    %48 = "arith.constant"() <{value = 10 : i32}> : () -> i32
    "hivm.hir.set_mask_norm"() : () -> ()
    %49 = "arith.muli"(%arg4, %arg5) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %50 = "arith.muli"(%49, %arg6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%50) {logical_block_num} : (i32) -> ()
    %51 = "hivm.hir.get_block_idx"() : () -> i64
    %52 = "arith.trunci"(%51) : (i64) -> i32
    %53 = "arith.muli"(%arg6, %arg5) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %54 = "arith.divsi"(%52, %53) : (i32, i32) -> i32
    %55 = "arith.remsi"(%54, %arg4) : (i32, i32) -> i32
    %56 = "tensor.empty"() : () -> tensor<3xi32>
    %57 = "tensor.empty"() : () -> tensor<3xi32>
    %58 = "tensor.empty"() : () -> tensor<3xi32>
    %59 = "tensor.empty"() : () -> tensor<3xi32>
    %60 = "tensor.empty"() : () -> tensor<3xi32>
    %61 = "tensor.empty"() : () -> tensor<3xi32>
    %62 = "tensor.empty"() : () -> tensor<3xi32>
    %63 = "tensor.empty"() : () -> tensor<3xi32>
    %64 = "tensor.empty"() : () -> tensor<3xi32>
    %65 = "tensor.empty"() : () -> tensor<3xi32>
    %66 = "tensor.empty"() : () -> tensor<3xi32>
    %67 = "tensor.empty"() : () -> tensor<3xi32>
    %68 = "tensor.empty"() : () -> tensor<3xi32>
    %69 = "tensor.empty"() : () -> tensor<3xi32>
    %70 = "tensor.empty"() : () -> tensor<3xi32>
    %71 = "tensor.empty"() : () -> tensor<3xi32>
    %72 = "tensor.empty"() : () -> tensor<3xi32>
    %73 = "tensor.empty"() : () -> tensor<3xi32>
    %74 = "tensor.empty"() : () -> tensor<3xi32>
    %75 = "tensor.empty"() : () -> tensor<3xi32>
    %76 = "tensor.empty"() : () -> tensor<3xi32>
    %77 = "tensor.empty"() : () -> tensor<3xi32>
    %78 = "tensor.empty"() : () -> tensor<3xi32>
    %79 = "tensor.empty"() : () -> tensor<3xi32>
    %80 = "tensor.empty"() : () -> tensor<3xi32>
    %81 = "tensor.empty"() : () -> tensor<3xi32>
    %82 = "tensor.empty"() : () -> tensor<3xi32>
    %83 = "tensor.empty"() : () -> tensor<3xi32>
    %84 = "tensor.empty"() : () -> tensor<3xi32>
    %85 = "tensor.empty"() : () -> tensor<3xi32>
    %86 = "tensor.empty"() : () -> tensor<3xi32>
    %87 = "tensor.empty"() : () -> tensor<3xi32>
    %88 = "tensor.empty"() : () -> tensor<3xi32>
    %89 = "tensor.empty"() : () -> tensor<3xi32>
    %90 = "tensor.empty"() : () -> tensor<3xi32>
    %91 = "tensor.empty"() : () -> tensor<3xi32>
    %92 = "tensor.empty"() : () -> tensor<3xi32>
    %93 = "tensor.empty"() : () -> tensor<3xi32>
    %94 = "tensor.empty"() : () -> tensor<3xi32>
    %95 = "tensor.empty"() : () -> tensor<3xi32>
    %96 = "tensor.empty"() : () -> tensor<3xi32>
    %97 = "tensor.empty"() : () -> tensor<3xi32>
    %98 = "tensor.empty"() : () -> tensor<3xi32>
    %99 = "tensor.empty"() : () -> tensor<3xi32>
    %100 = "tensor.empty"() : () -> tensor<3xi32>
    %101 = "tensor.empty"() : () -> tensor<3xi32>
    %102 = "tensor.empty"() : () -> tensor<3xi32>
    %103 = "tensor.empty"() : () -> tensor<3xi32>
    %104 = "tensor.empty"() : () -> tensor<3xi32>
    %105 = "tensor.empty"() : () -> tensor<3xi32>
    %106 = "tensor.empty"() : () -> tensor<3xi32>
    %107 = "tensor.empty"() : () -> tensor<3xi32>
    %108 = "tensor.empty"() : () -> tensor<3xi32>
    %109 = "tensor.empty"() : () -> tensor<3xi32>
    %110 = "tensor.empty"() : () -> tensor<3xi32>
    %111 = "tensor.empty"() : () -> tensor<3xi32>
    %112 = "tensor.empty"() : () -> tensor<3xi32>
    %113 = "tensor.empty"() : () -> tensor<3xi32>
    %114 = "tensor.empty"() : () -> tensor<3xi32>
    %115 = "tensor.empty"() : () -> tensor<3xi32>
    %116 = "tensor.empty"() : () -> tensor<3xi32>
    %117 = "tensor.empty"() : () -> tensor<3xi32>
    %118 = "tensor.empty"() : () -> tensor<3xi32>
    %119 = "tensor.empty"() : () -> tensor<3xi32>
    %120 = "tensor.empty"() : () -> tensor<3xi32>
    %121 = "tensor.empty"() : () -> tensor<3xi32>
    %122 = "tensor.empty"() : () -> tensor<3xi32>
    %123 = "tensor.empty"() : () -> tensor<3xi32>
    %124 = "tensor.empty"() : () -> tensor<3xi32>
    %125 = "tensor.empty"() : () -> tensor<3xi32>
    %126 = "tensor.empty"() : () -> tensor<3xi32>
    %127 = "tensor.empty"() : () -> tensor<3xi32>
    %128 = "tensor.empty"() : () -> tensor<3xi32>
    %129 = "tensor.empty"() : () -> tensor<3xi32>
    %130 = "tensor.empty"() : () -> tensor<3xi32>
    %131 = "tensor.empty"() : () -> tensor<3xi32>
    %132 = "tensor.empty"() : () -> tensor<3xi32>
    %133 = "tensor.empty"() : () -> tensor<3xi32>
    %134 = "tensor.empty"() : () -> tensor<3xi32>
    %135 = "tensor.empty"() : () -> tensor<3xi32>
    %136 = "tensor.empty"() : () -> tensor<3xi32>
    %137 = "tensor.empty"() : () -> tensor<3xi32>
    %138 = "tensor.empty"() : () -> tensor<3xi32>
    %139 = "tensor.empty"() : () -> tensor<3xi32>
    %140 = "tensor.empty"() : () -> tensor<3xi32>
    %141 = "tensor.empty"() : () -> tensor<3xi32>
    %142 = "tensor.empty"() : () -> tensor<3xi32>
    %143 = "tensor.empty"() : () -> tensor<3xi32>
    %144 = "tensor.empty"() : () -> tensor<3xi32>
    %145 = "tensor.empty"() : () -> tensor<3xi32>
    %146 = "tensor.empty"() : () -> tensor<3xi32>
    %147 = "tensor.empty"() : () -> tensor<3xi32>
    %148 = "tensor.empty"() : () -> tensor<3xi32>
    %149 = "tensor.empty"() : () -> tensor<3xi32>
    %150 = "tensor.empty"() : () -> tensor<3xi32>
    %151 = "tensor.empty"() : () -> tensor<3xi32>
    %152 = "tensor.empty"() : () -> tensor<3xi32>
    %153 = "tensor.empty"() : () -> tensor<3xi32>
    %154 = "tensor.empty"() : () -> tensor<3xi32>
    %155 = "tensor.empty"() : () -> tensor<3xi32>
    %156 = "tensor.empty"() : () -> tensor<3xi32>
    %157 = "tensor.empty"() : () -> tensor<3xi32>
    %158 = "tensor.empty"() : () -> tensor<3xi32>
    %159 = "tensor.empty"() : () -> tensor<3xi32>
    %160 = "tensor.empty"() : () -> tensor<3xi32>
    %161 = "tensor.empty"() : () -> tensor<3xi32>
    %162 = "tensor.empty"() : () -> tensor<3xi32>
    %163 = "tensor.empty"() : () -> tensor<3xi32>
    %164 = "tensor.empty"() : () -> tensor<3xi32>
    %165 = "tensor.empty"() : () -> tensor<3xi32>
    %166 = "tensor.empty"() : () -> tensor<3xi32>
    %167 = "tensor.empty"() : () -> tensor<3xi32>
    %168 = "tensor.empty"() : () -> tensor<3xi32>
    %169 = "tensor.empty"() : () -> tensor<3xi32>
    %170 = "tensor.empty"() : () -> tensor<3xi32>
    %171 = "tensor.empty"() : () -> tensor<3xi32>
    %172 = "tensor.empty"() : () -> tensor<3xi32>
    %173 = "tensor.empty"() : () -> tensor<3xi32>
    %174 = "tensor.empty"() : () -> tensor<3xi32>
    %175 = "tensor.empty"() : () -> tensor<3xi32>
    %176 = "tensor.empty"() : () -> tensor<3xi32>
    %177 = "tensor.empty"() : () -> tensor<3xi32>
    %178 = "tensor.empty"() : () -> tensor<3xi32>
    %179 = "tensor.empty"() : () -> tensor<3xf32>
    %180 = "tensor.empty"() : () -> tensor<3xf32>
    %181 = "tensor.empty"() : () -> tensor<3xf32>
    %182 = "tensor.empty"() : () -> tensor<3xf32>
    %183 = "tensor.empty"() : () -> tensor<3xf32>
    %184 = "tensor.empty"() : () -> tensor<3xf32>
    %185 = "tensor.empty"() : () -> tensor<3xf32>
    %186 = "tensor.empty"() : () -> tensor<3xf32>
    %187 = "tensor.empty"() : () -> tensor<3xf32>
    %188 = "tensor.empty"() : () -> tensor<3xf32>
    %189 = "tensor.empty"() : () -> tensor<3xf32>
    %190 = "tensor.empty"() : () -> tensor<3xf32>
    %191 = "tensor.empty"() : () -> tensor<3xf32>
    %192 = "tensor.empty"() : () -> tensor<3xf32>
    %193 = "tensor.empty"() : () -> tensor<3xf32>
    %194 = "tensor.empty"() : () -> tensor<3xf32>
    %195 = "tensor.empty"() : () -> tensor<3xf32>
    %196 = "tensor.empty"() : () -> tensor<3xf32>
    %197 = "tensor.empty"() : () -> tensor<3xf32>
    %198 = "tensor.empty"() : () -> tensor<3xf32>
    %199 = "tensor.empty"() : () -> tensor<3xf32>
    %200 = "tensor.empty"() : () -> tensor<3xf32>
    %201 = "tensor.empty"() : () -> tensor<3xf32>
    %202 = "tensor.empty"() : () -> tensor<3xf32>
    %203 = "tensor.empty"() : () -> tensor<3xf32>
    %204 = "tensor.empty"() : () -> tensor<3xf32>
    %205 = "tensor.empty"() : () -> tensor<3xf32>
    %206 = "tensor.empty"() : () -> tensor<3xf32>
    %207 = "tensor.empty"() : () -> tensor<3xf32>
    %208 = "tensor.empty"() : () -> tensor<3xf32>
    %209 = "tensor.empty"() : () -> tensor<3xf32>
    %210 = "tensor.empty"() : () -> tensor<3xf32>
    %211 = "tensor.empty"() : () -> tensor<3xf32>
    %212 = "tensor.empty"() : () -> tensor<3xf32>
    %213 = "tensor.empty"() : () -> tensor<3xf32>
    %214 = "tensor.empty"() : () -> tensor<3xf32>
    %215 = "tensor.empty"() : () -> tensor<3xf32>
    %216 = "tensor.empty"() : () -> tensor<3xf32>
    %217 = "tensor.empty"() : () -> tensor<3xf32>
    %218 = "tensor.empty"() : () -> tensor<3xf32>
    %219 = "tensor.empty"() : () -> tensor<3xf32>
    %220 = "tensor.empty"() : () -> tensor<3xf32>
    %221 = "tensor.empty"() : () -> tensor<3xf32>
    %222 = "tensor.empty"() : () -> tensor<3xf32>
    %223 = "tensor.empty"() : () -> tensor<3xf32>
    %224 = "tensor.empty"() : () -> tensor<3xf32>
    %225 = "tensor.empty"() : () -> tensor<3xf32>
    %226 = "tensor.empty"() : () -> tensor<3xf32>
    %227 = "tensor.empty"() : () -> tensor<3xf32>
    %228 = "tensor.empty"() : () -> tensor<3xf32>
    %229 = "tensor.empty"() : () -> tensor<3xf32>
    %230 = "tensor.empty"() : () -> tensor<3xf32>
    %231 = "tensor.empty"() : () -> tensor<3xf32>
    %232 = "tensor.empty"() : () -> tensor<3xf32>
    %233 = "tensor.empty"() : () -> tensor<3xf32>
    %234 = "tensor.empty"() : () -> tensor<3xf32>
    %235 = "tensor.empty"() : () -> tensor<3xf32>
    %236 = "hivm.hir.vbrc"(%46, %235) <{broadcast_dims = array<i64>}> : (f32, tensor<3xf32>) -> tensor<3xf32>
    %237 = "hivm.hir.vbrc"(%25, %178) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %238 = "hivm.hir.vbrc"(%24, %177) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %239 = "arith.muli"(%55, %47) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %240 = "hivm.hir.varange"(%176, %19, %20) <{operandSegmentSizes = array<i32: 1, 1, 1>}> : (tensor<3xi32>, index, index) -> tensor<3xi32>
    %241 = "hivm.hir.vadd"(%240, %239, %175) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, i32, tensor<3xi32>) -> tensor<3xi32>
    %242 = "hivm.hir.vadd"(%241, %48, %174) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, i32, tensor<3xi32>) -> tensor<3xi32>
    %243 = "tensor.empty"() : () -> tensor<3xi32>
    %244 = "tensor.empty"() : () -> tensor<3xi32>
    %245:2 = "hivm.hir.vmulextui"(%237, %242, %243, %244) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 2>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> (tensor<3xi32>, tensor<3xi32>)
    %246 = "hivm.hir.vmul"(%242, %25, %173) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, i32, tensor<3xi32>) -> tensor<3xi32>
    %247 = "tensor.empty"() : () -> tensor<3xi32>
    %248 = "tensor.empty"() : () -> tensor<3xi32>
    %249:2 = "hivm.hir.vmulextui"(%238, %245#1, %247, %248) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 2>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> (tensor<3xi32>, tensor<3xi32>)
    %250 = "tensor.empty"() : () -> tensor<3xi32>
    %251 = "hivm.hir.vbrc"(%26, %250) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %252 = "hivm.hir.vor"(%249#1, %251, %172) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %253 = "tensor.empty"() : () -> tensor<3xi32>
    %254 = "hivm.hir.vbrc"(%26, %253) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %255 = "hivm.hir.vand"(%249#1, %254, %171) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %256 = "hivm.hir.vnot"(%255, %255) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %257 = "hivm.hir.vand"(%256, %252, %170) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %258 = "tensor.empty"() : () -> tensor<3xi32>
    %259 = "hivm.hir.vbrc"(%22, %258) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %260 = "hivm.hir.vor"(%259, %246, %169) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %261 = "tensor.empty"() : () -> tensor<3xi32>
    %262 = "hivm.hir.vbrc"(%22, %261) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %263 = "hivm.hir.vand"(%262, %246, %168) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %264 = "hivm.hir.vnot"(%263, %263) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %265 = "hivm.hir.vand"(%264, %260, %167) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %266 = "tensor.empty"() : () -> tensor<3xi32>
    %267 = "hivm.hir.vbrc"(%27, %266) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %268 = "hivm.hir.vor"(%265, %267, %166) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %269 = "tensor.empty"() : () -> tensor<3xi32>
    %270 = "hivm.hir.vbrc"(%27, %269) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %271 = "hivm.hir.vand"(%265, %270, %165) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %272 = "hivm.hir.vnot"(%271, %271) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %273 = "hivm.hir.vand"(%272, %268, %164) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %274 = "hivm.hir.vmul"(%245#1, %24, %163) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, i32, tensor<3xi32>) -> tensor<3xi32>
    %275 = "tensor.empty"() : () -> tensor<3xi32>
    %276 = "tensor.empty"() : () -> tensor<3xi32>
    %277:2 = "hivm.hir.vmulextui"(%238, %273, %275, %276) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 2>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> (tensor<3xi32>, tensor<3xi32>)
    %278 = "hivm.hir.vor"(%277#1, %274, %162) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %279 = "hivm.hir.vand"(%277#1, %274, %161) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %280 = "hivm.hir.vnot"(%279, %279) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %281 = "hivm.hir.vand"(%280, %278, %160) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %282 = "tensor.empty"() : () -> tensor<3xi32>
    %283 = "hivm.hir.vbrc"(%28, %282) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %284 = "hivm.hir.vor"(%281, %283, %159) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %285 = "tensor.empty"() : () -> tensor<3xi32>
    %286 = "hivm.hir.vbrc"(%28, %285) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %287 = "hivm.hir.vand"(%281, %286, %158) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %288 = "hivm.hir.vnot"(%287, %287) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %289 = "hivm.hir.vand"(%288, %284, %157) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %290 = "tensor.empty"() : () -> tensor<3xi32>
    %291 = "tensor.empty"() : () -> tensor<3xi32>
    %292:2 = "hivm.hir.vmulextui"(%237, %257, %290, %291) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 2>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> (tensor<3xi32>, tensor<3xi32>)
    %293 = "tensor.empty"() : () -> tensor<3xi32>
    %294 = "hivm.hir.vbrc"(%21, %293) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %295 = "hivm.hir.vor"(%292#1, %294, %156) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %296 = "tensor.empty"() : () -> tensor<3xi32>
    %297 = "hivm.hir.vbrc"(%21, %296) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %298 = "hivm.hir.vand"(%292#1, %297, %155) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %299 = "hivm.hir.vnot"(%298, %298) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %300 = "hivm.hir.vand"(%299, %295, %154) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %301 = "tensor.empty"() : () -> tensor<3xi32>
    %302 = "hivm.hir.vbrc"(%29, %301) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %303 = "hivm.hir.vor"(%300, %302, %153) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %304 = "tensor.empty"() : () -> tensor<3xi32>
    %305 = "hivm.hir.vbrc"(%29, %304) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %306 = "hivm.hir.vand"(%300, %305, %152) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %307 = "hivm.hir.vnot"(%306, %306) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %308 = "hivm.hir.vand"(%307, %303, %151) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %309 = "hivm.hir.vmul"(%273, %24, %150) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, i32, tensor<3xi32>) -> tensor<3xi32>
    %310 = "hivm.hir.vmul"(%257, %25, %149) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, i32, tensor<3xi32>) -> tensor<3xi32>
    %311 = "tensor.empty"() : () -> tensor<3xi32>
    %312 = "tensor.empty"() : () -> tensor<3xi32>
    %313:2 = "hivm.hir.vmulextui"(%238, %308, %311, %312) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 2>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> (tensor<3xi32>, tensor<3xi32>)
    %314 = "hivm.hir.vor"(%313#1, %309, %148) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %315 = "hivm.hir.vand"(%313#1, %309, %147) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %316 = "hivm.hir.vnot"(%315, %315) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %317 = "hivm.hir.vand"(%316, %314, %146) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %318 = "tensor.empty"() : () -> tensor<3xi32>
    %319 = "hivm.hir.vbrc"(%30, %318) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %320 = "hivm.hir.vor"(%317, %319, %145) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %321 = "tensor.empty"() : () -> tensor<3xi32>
    %322 = "hivm.hir.vbrc"(%30, %321) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %323 = "hivm.hir.vand"(%317, %322, %144) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %324 = "hivm.hir.vnot"(%323, %323) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %325 = "hivm.hir.vand"(%324, %320, %143) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %326 = "tensor.empty"() : () -> tensor<3xi32>
    %327 = "tensor.empty"() : () -> tensor<3xi32>
    %328:2 = "hivm.hir.vmulextui"(%237, %289, %326, %327) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 2>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> (tensor<3xi32>, tensor<3xi32>)
    %329 = "hivm.hir.vor"(%328#1, %310, %142) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %330 = "hivm.hir.vand"(%328#1, %310, %141) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %331 = "hivm.hir.vnot"(%330, %330) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %332 = "hivm.hir.vand"(%331, %329, %140) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %333 = "tensor.empty"() : () -> tensor<3xi32>
    %334 = "hivm.hir.vbrc"(%31, %333) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %335 = "hivm.hir.vor"(%332, %334, %139) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %336 = "tensor.empty"() : () -> tensor<3xi32>
    %337 = "hivm.hir.vbrc"(%31, %336) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %338 = "hivm.hir.vand"(%332, %337, %138) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %339 = "hivm.hir.vnot"(%338, %338) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %340 = "hivm.hir.vand"(%339, %335, %137) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %341 = "hivm.hir.vmul"(%308, %24, %136) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, i32, tensor<3xi32>) -> tensor<3xi32>
    %342 = "hivm.hir.vmul"(%289, %25, %135) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, i32, tensor<3xi32>) -> tensor<3xi32>
    %343 = "tensor.empty"() : () -> tensor<3xi32>
    %344 = "tensor.empty"() : () -> tensor<3xi32>
    %345:2 = "hivm.hir.vmulextui"(%238, %340, %343, %344) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 2>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> (tensor<3xi32>, tensor<3xi32>)
    %346 = "hivm.hir.vor"(%345#1, %341, %134) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %347 = "hivm.hir.vand"(%345#1, %341, %133) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %348 = "hivm.hir.vnot"(%347, %347) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %349 = "hivm.hir.vand"(%348, %346, %132) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %350 = "tensor.empty"() : () -> tensor<3xi32>
    %351 = "hivm.hir.vbrc"(%32, %350) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %352 = "hivm.hir.vor"(%349, %351, %131) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %353 = "tensor.empty"() : () -> tensor<3xi32>
    %354 = "hivm.hir.vbrc"(%32, %353) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %355 = "hivm.hir.vand"(%349, %354, %130) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %356 = "hivm.hir.vnot"(%355, %355) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %357 = "hivm.hir.vand"(%356, %352, %129) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %358 = "tensor.empty"() : () -> tensor<3xi32>
    %359 = "tensor.empty"() : () -> tensor<3xi32>
    %360:2 = "hivm.hir.vmulextui"(%237, %325, %358, %359) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 2>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> (tensor<3xi32>, tensor<3xi32>)
    %361 = "hivm.hir.vor"(%360#1, %342, %128) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %362 = "hivm.hir.vand"(%360#1, %342, %127) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %363 = "hivm.hir.vnot"(%362, %362) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %364 = "hivm.hir.vand"(%363, %361, %126) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %365 = "tensor.empty"() : () -> tensor<3xi32>
    %366 = "hivm.hir.vbrc"(%33, %365) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %367 = "hivm.hir.vor"(%364, %366, %125) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %368 = "tensor.empty"() : () -> tensor<3xi32>
    %369 = "hivm.hir.vbrc"(%33, %368) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %370 = "hivm.hir.vand"(%364, %369, %124) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %371 = "hivm.hir.vnot"(%370, %370) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %372 = "hivm.hir.vand"(%371, %367, %123) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %373 = "hivm.hir.vmul"(%340, %24, %122) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, i32, tensor<3xi32>) -> tensor<3xi32>
    %374 = "hivm.hir.vmul"(%325, %25, %121) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, i32, tensor<3xi32>) -> tensor<3xi32>
    %375 = "tensor.empty"() : () -> tensor<3xi32>
    %376 = "tensor.empty"() : () -> tensor<3xi32>
    %377:2 = "hivm.hir.vmulextui"(%238, %372, %375, %376) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 2>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> (tensor<3xi32>, tensor<3xi32>)
    %378 = "hivm.hir.vor"(%377#1, %373, %120) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %379 = "hivm.hir.vand"(%377#1, %373, %119) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %380 = "hivm.hir.vnot"(%379, %379) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %381 = "hivm.hir.vand"(%380, %378, %118) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %382 = "tensor.empty"() : () -> tensor<3xi32>
    %383 = "hivm.hir.vbrc"(%34, %382) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %384 = "hivm.hir.vor"(%381, %383, %117) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %385 = "tensor.empty"() : () -> tensor<3xi32>
    %386 = "hivm.hir.vbrc"(%34, %385) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %387 = "hivm.hir.vand"(%381, %386, %116) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %388 = "hivm.hir.vnot"(%387, %387) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %389 = "hivm.hir.vand"(%388, %384, %115) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %390 = "tensor.empty"() : () -> tensor<3xi32>
    %391 = "tensor.empty"() : () -> tensor<3xi32>
    %392:2 = "hivm.hir.vmulextui"(%237, %357, %390, %391) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 2>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> (tensor<3xi32>, tensor<3xi32>)
    %393 = "hivm.hir.vor"(%392#1, %374, %114) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %394 = "hivm.hir.vand"(%392#1, %374, %113) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %395 = "hivm.hir.vnot"(%394, %394) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %396 = "hivm.hir.vand"(%395, %393, %112) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %397 = "tensor.empty"() : () -> tensor<3xi32>
    %398 = "hivm.hir.vbrc"(%35, %397) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %399 = "hivm.hir.vor"(%396, %398, %111) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %400 = "tensor.empty"() : () -> tensor<3xi32>
    %401 = "hivm.hir.vbrc"(%35, %400) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %402 = "hivm.hir.vand"(%396, %401, %110) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %403 = "hivm.hir.vnot"(%402, %402) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %404 = "hivm.hir.vand"(%403, %399, %109) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %405 = "hivm.hir.vmul"(%372, %24, %108) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, i32, tensor<3xi32>) -> tensor<3xi32>
    %406 = "hivm.hir.vmul"(%357, %25, %107) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, i32, tensor<3xi32>) -> tensor<3xi32>
    %407 = "tensor.empty"() : () -> tensor<3xi32>
    %408 = "tensor.empty"() : () -> tensor<3xi32>
    %409:2 = "hivm.hir.vmulextui"(%238, %404, %407, %408) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 2>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> (tensor<3xi32>, tensor<3xi32>)
    %410 = "hivm.hir.vor"(%409#1, %405, %106) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %411 = "hivm.hir.vand"(%409#1, %405, %105) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %412 = "hivm.hir.vnot"(%411, %411) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %413 = "hivm.hir.vand"(%412, %410, %104) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %414 = "tensor.empty"() : () -> tensor<3xi32>
    %415 = "hivm.hir.vbrc"(%36, %414) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %416 = "hivm.hir.vor"(%413, %415, %103) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %417 = "tensor.empty"() : () -> tensor<3xi32>
    %418 = "hivm.hir.vbrc"(%36, %417) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %419 = "hivm.hir.vand"(%413, %418, %102) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %420 = "hivm.hir.vnot"(%419, %419) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %421 = "hivm.hir.vand"(%420, %416, %101) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %422 = "tensor.empty"() : () -> tensor<3xi32>
    %423 = "tensor.empty"() : () -> tensor<3xi32>
    %424:2 = "hivm.hir.vmulextui"(%237, %389, %422, %423) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 2>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> (tensor<3xi32>, tensor<3xi32>)
    %425 = "hivm.hir.vor"(%424#1, %406, %100) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %426 = "hivm.hir.vand"(%424#1, %406, %99) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %427 = "hivm.hir.vnot"(%426, %426) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %428 = "hivm.hir.vand"(%427, %425, %98) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %429 = "tensor.empty"() : () -> tensor<3xi32>
    %430 = "hivm.hir.vbrc"(%37, %429) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %431 = "hivm.hir.vor"(%428, %430, %97) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %432 = "tensor.empty"() : () -> tensor<3xi32>
    %433 = "hivm.hir.vbrc"(%37, %432) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %434 = "hivm.hir.vand"(%428, %433, %96) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %435 = "hivm.hir.vnot"(%434, %434) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %436 = "hivm.hir.vand"(%435, %431, %95) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %437 = "hivm.hir.vmul"(%404, %24, %94) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, i32, tensor<3xi32>) -> tensor<3xi32>
    %438 = "hivm.hir.vmul"(%389, %25, %93) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, i32, tensor<3xi32>) -> tensor<3xi32>
    %439 = "tensor.empty"() : () -> tensor<3xi32>
    %440 = "tensor.empty"() : () -> tensor<3xi32>
    %441:2 = "hivm.hir.vmulextui"(%238, %436, %439, %440) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 2>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> (tensor<3xi32>, tensor<3xi32>)
    %442 = "hivm.hir.vor"(%441#1, %437, %92) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %443 = "hivm.hir.vand"(%441#1, %437, %91) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %444 = "hivm.hir.vnot"(%443, %443) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %445 = "hivm.hir.vand"(%444, %442, %90) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %446 = "tensor.empty"() : () -> tensor<3xi32>
    %447 = "hivm.hir.vbrc"(%38, %446) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %448 = "hivm.hir.vor"(%445, %447, %89) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %449 = "tensor.empty"() : () -> tensor<3xi32>
    %450 = "hivm.hir.vbrc"(%38, %449) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %451 = "hivm.hir.vand"(%445, %450, %88) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %452 = "hivm.hir.vnot"(%451, %451) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %453 = "hivm.hir.vand"(%452, %448, %87) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %454 = "tensor.empty"() : () -> tensor<3xi32>
    %455 = "tensor.empty"() : () -> tensor<3xi32>
    %456:2 = "hivm.hir.vmulextui"(%237, %421, %454, %455) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 2>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> (tensor<3xi32>, tensor<3xi32>)
    %457 = "hivm.hir.vor"(%456#1, %438, %86) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %458 = "hivm.hir.vand"(%456#1, %438, %85) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %459 = "hivm.hir.vnot"(%458, %458) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %460 = "hivm.hir.vand"(%459, %457, %84) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %461 = "tensor.empty"() : () -> tensor<3xi32>
    %462 = "hivm.hir.vbrc"(%39, %461) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %463 = "hivm.hir.vor"(%460, %462, %83) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %464 = "tensor.empty"() : () -> tensor<3xi32>
    %465 = "hivm.hir.vbrc"(%39, %464) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %466 = "hivm.hir.vand"(%460, %465, %82) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %467 = "hivm.hir.vnot"(%466, %466) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %468 = "hivm.hir.vand"(%467, %463, %81) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %469 = "hivm.hir.vmul"(%421, %25, %80) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, i32, tensor<3xi32>) -> tensor<3xi32>
    %470 = "tensor.empty"() : () -> tensor<3xi32>
    %471 = "tensor.empty"() : () -> tensor<3xi32>
    %472:2 = "hivm.hir.vmulextui"(%237, %453, %470, %471) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 2>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> (tensor<3xi32>, tensor<3xi32>)
    %473 = "hivm.hir.vor"(%472#1, %469, %79) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %474 = "hivm.hir.vand"(%472#1, %469, %78) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %475 = "hivm.hir.vnot"(%474, %474) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %476 = "hivm.hir.vand"(%475, %473, %77) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %477 = "tensor.empty"() : () -> tensor<3xi32>
    %478 = "hivm.hir.vbrc"(%40, %477) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %479 = "hivm.hir.vor"(%476, %478, %76) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %480 = "tensor.empty"() : () -> tensor<3xi32>
    %481 = "hivm.hir.vbrc"(%40, %480) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %482 = "hivm.hir.vand"(%476, %481, %75) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %483 = "hivm.hir.vnot"(%482, %482) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %484 = "hivm.hir.vand"(%483, %479, %74) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %485 = "hivm.hir.vmul"(%468, %24, %73) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, i32, tensor<3xi32>) -> tensor<3xi32>
    %486 = "tensor.empty"() : () -> tensor<3xi32>
    %487 = "tensor.empty"() : () -> tensor<3xi32>
    %488:2 = "hivm.hir.vmulextui"(%238, %484, %486, %487) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 2>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> (tensor<3xi32>, tensor<3xi32>)
    %489 = "hivm.hir.vor"(%488#1, %485, %72) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %490 = "hivm.hir.vand"(%488#1, %485, %71) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %491 = "hivm.hir.vnot"(%490, %490) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %492 = "hivm.hir.vand"(%491, %489, %70) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %493 = "tensor.empty"() : () -> tensor<3xi32>
    %494 = "hivm.hir.vbrc"(%41, %493) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %495 = "hivm.hir.vor"(%492, %494, %69) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %496 = "tensor.empty"() : () -> tensor<3xi32>
    %497 = "hivm.hir.vbrc"(%41, %496) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %498 = "hivm.hir.vand"(%492, %497, %68) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %499 = "hivm.hir.vnot"(%498, %498) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %500 = "hivm.hir.vand"(%499, %495, %67) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %501 = "hivm.hir.vmul"(%484, %24, %66) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, i32, tensor<3xi32>) -> tensor<3xi32>
    %502 = "hivm.hir.vmax"(%500, %42, %65) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, i32, tensor<3xi32>) -> tensor<3xi32>
    %503 = "tensor.empty"() : () -> tensor<3xi1>
    %504 = "tensor.empty"() : () -> tensor<3xi1>
    %505 = "tensor.empty"() : () -> tensor<3xi1>
    %506 = "tensor.empty"() : () -> tensor<3xi1>
    %507 = "tensor.empty"() : () -> tensor<3xi1>
    %508 = "tensor.empty"() : () -> tensor<3xi1>
    %509 = "tensor.empty"() : () -> tensor<3xi1>
    %510 = "tensor.empty"() : () -> tensor<3xi1>
    %511 = "hivm.hir.vcmp"(%502, %500, %510) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi1>) -> tensor<3xi1>
    %512 = "hivm.hir.vnot"(%511, %509) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi1>, tensor<3xi1>) -> tensor<3xi1>
    %513 = "hivm.hir.vmul"(%500, %1, %64) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, i32, tensor<3xi32>) -> tensor<3xi32>
    %514 = "hivm.hir.vadd"(%513, %42, %63) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, i32, tensor<3xi32>) -> tensor<3xi32>
    %515 = "hivm.hir.vsub"(%514, %0, %62) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, i32, tensor<3xi32>) -> tensor<3xi32>
    %516 = "hivm.hir.vsel"(%512, %515, %500, %61) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<3xi1>, tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %517 = "hivm.hir.vcast"(%516, %234) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xf32>) -> tensor<3xf32>
    %518 = "hivm.hir.vmul"(%517, %43, %233) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %519 = "hivm.hir.vmax"(%501, %42, %60) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, i32, tensor<3xi32>) -> tensor<3xi32>
    %520 = "hivm.hir.vcmp"(%519, %501, %508) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi1>) -> tensor<3xi1>
    %521 = "hivm.hir.vnot"(%520, %507) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi1>, tensor<3xi1>) -> tensor<3xi1>
    %522 = "hivm.hir.vmul"(%501, %1, %59) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, i32, tensor<3xi32>) -> tensor<3xi32>
    %523 = "hivm.hir.vadd"(%522, %42, %58) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, i32, tensor<3xi32>) -> tensor<3xi32>
    %524 = "hivm.hir.vsub"(%523, %0, %57) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, i32, tensor<3xi32>) -> tensor<3xi32>
    %525 = "hivm.hir.vsel"(%521, %524, %501, %56) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<3xi1>, tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %526 = "hivm.hir.vcast"(%525, %232) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xf32>) -> tensor<3xf32>
    %527 = "hivm.hir.vmul"(%526, %43, %231) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %528 = "hivm.hir.vcmp"(%518, %518, %506) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<3xf32>, tensor<3xf32>, tensor<3xi1>) -> tensor<3xi1>
    %529 = "hivm.hir.vnot"(%528, %505) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi1>, tensor<3xi1>) -> tensor<3xi1>
    %530 = "hivm.hir.vcmp"(%236, %236, %504) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<3xf32>, tensor<3xf32>, tensor<3xi1>) -> tensor<3xi1>
    %531 = "hivm.hir.vnot"(%530, %503) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi1>, tensor<3xi1>) -> tensor<3xi1>
    %532 = "hivm.hir.vmax"(%518, %46, %230) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %533 = "hivm.hir.vsel"(%529, %46, %532, %229) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<3xi1>, f32, tensor<3xf32>, tensor<3xf32>) -> tensor<3xf32>
    %534 = "hivm.hir.vsel"(%531, %518, %533, %228) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<3xi1>, tensor<3xf32>, tensor<3xf32>, tensor<3xf32>) -> tensor<3xf32>
    %535 = "hivm.hir.vmul"(%527, %45, %227) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %536 = "hivm.hir.vln"(%534, %226) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, tensor<3xf32>) -> tensor<3xf32>
    %537 = "hivm.hir.vmul"(%536, %44, %225) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %538 = "hivm.hir.vsqrt"(%537, %224) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, tensor<3xf32>) -> tensor<3xf32>
    %539 = "hivm.hir.vmul"(%535, %18, %223) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %540 = "hivm.hir.vadd"(%539, %17, %222) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %541 = "hivm.hir.vmul"(%539, %16, %221) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %542 = "hivm.hir.vcast"(%540, %220) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<round>, transpose = array<i64>}> : (tensor<3xf32>, tensor<3xf32>) -> tensor<3xf32>
    %543 = "hivm.hir.vcast"(%541, %219) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<round>, transpose = array<i64>}> : (tensor<3xf32>, tensor<3xf32>) -> tensor<3xf32>
    %544 = "hivm.hir.vmul"(%543, %15, %218) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %545 = "hivm.hir.vsub"(%542, %544, %217) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, tensor<3xf32>, tensor<3xf32>) -> tensor<3xf32>
    %546 = "hivm.hir.vmul"(%544, %14, %216) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %547 = "hivm.hir.vsub"(%535, %546, %215) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, tensor<3xf32>, tensor<3xf32>) -> tensor<3xf32>
    %548 = "hivm.hir.vmul"(%545, %14, %214) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %549 = "hivm.hir.vsub"(%547, %548, %213) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, tensor<3xf32>, tensor<3xf32>) -> tensor<3xf32>
    %550 = "hivm.hir.vmul"(%544, %13, %212) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %551 = "hivm.hir.vsub"(%549, %550, %211) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, tensor<3xf32>, tensor<3xf32>) -> tensor<3xf32>
    %552 = "hivm.hir.vadd"(%551, %12, %210) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %553 = "hivm.hir.vmul"(%545, %13, %209) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %554 = "hivm.hir.vsub"(%552, %553, %208) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, tensor<3xf32>, tensor<3xf32>) -> tensor<3xf32>
    %555 = "hivm.hir.vmul"(%544, %11, %207) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %556 = "hivm.hir.vsub"(%554, %555, %206) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, tensor<3xf32>, tensor<3xf32>) -> tensor<3xf32>
    %557 = "hivm.hir.vmul"(%545, %11, %205) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %558 = "hivm.hir.vsub"(%556, %557, %204) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, tensor<3xf32>, tensor<3xf32>) -> tensor<3xf32>
    %559 = "hivm.hir.vmul"(%544, %10, %203) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %560 = "hivm.hir.vsub"(%558, %559, %202) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, tensor<3xf32>, tensor<3xf32>) -> tensor<3xf32>
    %561 = "hivm.hir.vmul"(%545, %10, %201) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %562 = "hivm.hir.vsub"(%560, %561, %200) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, tensor<3xf32>, tensor<3xf32>) -> tensor<3xf32>
    %563 = "hivm.hir.vadd"(%562, %9, %199) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %564 = "hivm.hir.vmul"(%542, %17, %198) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %565 = "hivm.hir.vcast"(%564, %197) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<floor>, transpose = array<i64>}> : (tensor<3xf32>, tensor<3xf32>) -> tensor<3xf32>
    %566 = "hivm.hir.vmul"(%565, %8, %196) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %567 = "hivm.hir.vmul"(%542, %44, %195) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %568 = "hivm.hir.vadd"(%566, %567, %194) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, tensor<3xf32>, tensor<3xf32>) -> tensor<3xf32>
    %569 = "hivm.hir.vadd"(%568, %7, %193) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %570 = "hivm.hir.vmul"(%563, %563, %192) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, tensor<3xf32>, tensor<3xf32>) -> tensor<3xf32>
    %571 = "hivm.hir.vmul"(%570, %6, %191) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %572 = "hivm.hir.vadd"(%571, %5, %190) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %573 = "hivm.hir.vmul"(%572, %570, %189) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, tensor<3xf32>, tensor<3xf32>) -> tensor<3xf32>
    %574 = "hivm.hir.vadd"(%573, %4, %188) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %575 = "hivm.hir.vmul"(%574, %570, %187) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, tensor<3xf32>, tensor<3xf32>) -> tensor<3xf32>
    %576 = "hivm.hir.vadd"(%575, %3, %186) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %577 = "hivm.hir.vmul"(%576, %570, %185) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, tensor<3xf32>, tensor<3xf32>) -> tensor<3xf32>
    %578 = "hivm.hir.vadd"(%577, %7, %184) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %579 = "hivm.hir.vmul"(%578, %563, %183) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, tensor<3xf32>, tensor<3xf32>) -> tensor<3xf32>
    %580 = "hivm.hir.vmul"(%579, %569, %182) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, tensor<3xf32>, tensor<3xf32>) -> tensor<3xf32>
    %581 = "hivm.hir.vmin"(%580, %7, %181) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %582 = "hivm.hir.vmax"(%581, %2, %180) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %583 = "hivm.hir.vmul"(%538, %582, %179) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, tensor<3xf32>, tensor<3xf32>) -> tensor<3xf32>
    %584 = "arith.index_cast"(%239) : (i32) -> index
    %585 = "memref.reinterpret_cast"(%arg3, %584) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 3>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<3xf32, strided<[1], offset: ?>>
    %586 = "arith.addi"(%584, %23) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %587 = "arith.maxsi"(%584, %23) : (index, index) -> index
    %588 = "arith.minsi"(%586, %587) : (index, index) -> index
    %589 = "arith.subi"(%588, %584) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %590 = "tensor.extract_slice"(%583, %589) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<3xf32>, index) -> tensor<?xf32>
    %591 = "memref.subview"(%585, %589) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<3xf32, strided<[1], offset: ?>>, index) -> memref<?xf32, strided<[1], offset: ?>>
    "hivm.hir.store"(%590, %591) : (tensor<?xf32>, memref<?xf32, strided<[1], offset: ?>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, false, false, false]> : vector<7xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

