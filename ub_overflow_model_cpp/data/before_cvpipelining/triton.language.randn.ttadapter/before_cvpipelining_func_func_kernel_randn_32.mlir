#map = affine_map<()[s0] -> (s0 + 3)>
#map1 = affine_map<()[s0] -> (s0, 3)>
#map2 = affine_map<()[s0, s1] -> (s0, s1)>
#map3 = affine_map<()[s0, s1] -> (s0 - s1)>
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
    %23 = "arith.constant"() <{value = -845247145 : i32}> : () -> i32
    %24 = "arith.constant"() <{value = -766435501 : i32}> : () -> i32
    %25 = "arith.constant"() <{value = -1640531522 : i32}> : () -> i32
    %26 = "arith.constant"() <{value = -1150833019 : i32}> : () -> i32
    %27 = "arith.constant"() <{value = 1013904247 : i32}> : () -> i32
    %28 = "arith.constant"() <{value = 1993301258 : i32}> : () -> i32
    %29 = "arith.constant"() <{value = -626627280 : i32}> : () -> i32
    %30 = "arith.constant"() <{value = 842468239 : i32}> : () -> i32
    %31 = "arith.constant"() <{value = 2027808489 : i32}> : () -> i32
    %32 = "arith.constant"() <{value = -308364780 : i32}> : () -> i32
    %33 = "arith.constant"() <{value = 387276962 : i32}> : () -> i32
    %34 = "arith.constant"() <{value = -1459197799 : i32}> : () -> i32
    %35 = "arith.constant"() <{value = -1253254565 : i32}> : () -> i32
    %36 = "arith.constant"() <{value = 1684936478 : i32}> : () -> i32
    %37 = "arith.constant"() <{value = 1401181204 : i32}> : () -> i32
    %38 = "arith.constant"() <{value = 534103459 : i32}> : () -> i32
    %39 = "arith.constant"() <{value = -616729560 : i32}> : () -> i32
    %40 = "arith.constant"() <{value = -1879881850 : i32}> : () -> i32
    %41 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %42 = "arith.constant"() <{value = 4.6566126E-10 : f32}> : () -> f32
    %43 = "arith.constant"() <{value = -2.000000e+00 : f32}> : () -> f32
    %44 = "arith.constant"() <{value = 6.28318548 : f32}> : () -> f32
    %45 = "arith.constant"() <{value = 1.000000e-07 : f32}> : () -> f32
    %46 = "arith.constant"() <{value = 3 : i32}> : () -> i32
    %47 = "arith.constant"() <{value = 10 : i32}> : () -> i32
    "hivm.hir.set_mask_norm"() : () -> ()
    %48 = "arith.muli"(%arg4, %arg5) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %49 = "arith.muli"(%48, %arg6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%49) <{effects = ["write"]}> {logical_block_num} : (i32) -> ()
    %50 = "hivm.hir.get_block_idx"() : () -> i64
    %51 = "arith.trunci"(%50) : (i64) -> i32
    %52 = "arith.muli"(%arg6, %arg5) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %53 = "arith.divsi"(%51, %52) : (i32, i32) -> i32
    %54 = "arith.remsi"(%53, %arg4) : (i32, i32) -> i32
    %55 = "tensor.empty"() : () -> tensor<3xi32>
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
    %178 = "tensor.empty"() : () -> tensor<3xf32>
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
    %235 = "hivm.hir.vbrc"(%45, %234) <{broadcast_dims = array<i64>}> : (f32, tensor<3xf32>) -> tensor<3xf32>
    %236 = "hivm.hir.vbrc"(%24, %177) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %237 = "hivm.hir.vbrc"(%23, %176) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %238 = "arith.muli"(%54, %46) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %239 = "hivm.hir.varange"(%175, %19, %20) <{operandSegmentSizes = array<i32: 1, 1, 1>}> : (tensor<3xi32>, index, index) -> tensor<3xi32>
    %240 = "hivm.hir.vadd"(%239, %238, %174) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, i32, tensor<3xi32>) -> tensor<3xi32>
    %241 = "hivm.hir.vadd"(%240, %47, %173) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, i32, tensor<3xi32>) -> tensor<3xi32>
    %242 = "tensor.empty"() : () -> tensor<3xi32>
    %243 = "tensor.empty"() : () -> tensor<3xi32>
    %244:2 = "hivm.hir.vmulextui"(%236, %241, %242, %243) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 2>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> (tensor<3xi32>, tensor<3xi32>)
    %245 = "hivm.hir.vmul"(%241, %24, %172) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, i32, tensor<3xi32>) -> tensor<3xi32>
    %246 = "tensor.empty"() : () -> tensor<3xi32>
    %247 = "tensor.empty"() : () -> tensor<3xi32>
    %248:2 = "hivm.hir.vmulextui"(%237, %244#1, %246, %247) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 2>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> (tensor<3xi32>, tensor<3xi32>)
    %249 = "tensor.empty"() : () -> tensor<3xi32>
    %250 = "hivm.hir.vbrc"(%25, %249) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %251 = "hivm.hir.vor"(%248#1, %250, %171) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %252 = "tensor.empty"() : () -> tensor<3xi32>
    %253 = "hivm.hir.vbrc"(%25, %252) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %254 = "hivm.hir.vand"(%248#1, %253, %170) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %255 = "hivm.hir.vnot"(%254, %254) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %256 = "hivm.hir.vand"(%255, %251, %169) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %257 = "tensor.empty"() : () -> tensor<3xi32>
    %258 = "hivm.hir.vbrc"(%22, %257) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %259 = "hivm.hir.vor"(%258, %245, %168) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %260 = "tensor.empty"() : () -> tensor<3xi32>
    %261 = "hivm.hir.vbrc"(%22, %260) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %262 = "hivm.hir.vand"(%261, %245, %167) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %263 = "hivm.hir.vnot"(%262, %262) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %264 = "hivm.hir.vand"(%263, %259, %166) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %265 = "tensor.empty"() : () -> tensor<3xi32>
    %266 = "hivm.hir.vbrc"(%26, %265) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %267 = "hivm.hir.vor"(%264, %266, %165) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %268 = "tensor.empty"() : () -> tensor<3xi32>
    %269 = "hivm.hir.vbrc"(%26, %268) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %270 = "hivm.hir.vand"(%264, %269, %164) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %271 = "hivm.hir.vnot"(%270, %270) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %272 = "hivm.hir.vand"(%271, %267, %163) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %273 = "hivm.hir.vmul"(%244#1, %23, %162) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, i32, tensor<3xi32>) -> tensor<3xi32>
    %274 = "tensor.empty"() : () -> tensor<3xi32>
    %275 = "tensor.empty"() : () -> tensor<3xi32>
    %276:2 = "hivm.hir.vmulextui"(%237, %272, %274, %275) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 2>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> (tensor<3xi32>, tensor<3xi32>)
    %277 = "hivm.hir.vor"(%276#1, %273, %161) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %278 = "hivm.hir.vand"(%276#1, %273, %160) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %279 = "hivm.hir.vnot"(%278, %278) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %280 = "hivm.hir.vand"(%279, %277, %159) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %281 = "tensor.empty"() : () -> tensor<3xi32>
    %282 = "hivm.hir.vbrc"(%27, %281) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %283 = "hivm.hir.vor"(%280, %282, %158) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %284 = "tensor.empty"() : () -> tensor<3xi32>
    %285 = "hivm.hir.vbrc"(%27, %284) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %286 = "hivm.hir.vand"(%280, %285, %157) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %287 = "hivm.hir.vnot"(%286, %286) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %288 = "hivm.hir.vand"(%287, %283, %156) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %289 = "tensor.empty"() : () -> tensor<3xi32>
    %290 = "tensor.empty"() : () -> tensor<3xi32>
    %291:2 = "hivm.hir.vmulextui"(%236, %256, %289, %290) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 2>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> (tensor<3xi32>, tensor<3xi32>)
    %292 = "tensor.empty"() : () -> tensor<3xi32>
    %293 = "hivm.hir.vbrc"(%21, %292) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %294 = "hivm.hir.vor"(%291#1, %293, %155) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %295 = "tensor.empty"() : () -> tensor<3xi32>
    %296 = "hivm.hir.vbrc"(%21, %295) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %297 = "hivm.hir.vand"(%291#1, %296, %154) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %298 = "hivm.hir.vnot"(%297, %297) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %299 = "hivm.hir.vand"(%298, %294, %153) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %300 = "tensor.empty"() : () -> tensor<3xi32>
    %301 = "hivm.hir.vbrc"(%28, %300) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %302 = "hivm.hir.vor"(%299, %301, %152) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %303 = "tensor.empty"() : () -> tensor<3xi32>
    %304 = "hivm.hir.vbrc"(%28, %303) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %305 = "hivm.hir.vand"(%299, %304, %151) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %306 = "hivm.hir.vnot"(%305, %305) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %307 = "hivm.hir.vand"(%306, %302, %150) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %308 = "hivm.hir.vmul"(%272, %23, %149) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, i32, tensor<3xi32>) -> tensor<3xi32>
    %309 = "hivm.hir.vmul"(%256, %24, %148) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, i32, tensor<3xi32>) -> tensor<3xi32>
    %310 = "tensor.empty"() : () -> tensor<3xi32>
    %311 = "tensor.empty"() : () -> tensor<3xi32>
    %312:2 = "hivm.hir.vmulextui"(%237, %307, %310, %311) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 2>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> (tensor<3xi32>, tensor<3xi32>)
    %313 = "hivm.hir.vor"(%312#1, %308, %147) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %314 = "hivm.hir.vand"(%312#1, %308, %146) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %315 = "hivm.hir.vnot"(%314, %314) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %316 = "hivm.hir.vand"(%315, %313, %145) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %317 = "tensor.empty"() : () -> tensor<3xi32>
    %318 = "hivm.hir.vbrc"(%29, %317) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %319 = "hivm.hir.vor"(%316, %318, %144) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %320 = "tensor.empty"() : () -> tensor<3xi32>
    %321 = "hivm.hir.vbrc"(%29, %320) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %322 = "hivm.hir.vand"(%316, %321, %143) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %323 = "hivm.hir.vnot"(%322, %322) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %324 = "hivm.hir.vand"(%323, %319, %142) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %325 = "tensor.empty"() : () -> tensor<3xi32>
    %326 = "tensor.empty"() : () -> tensor<3xi32>
    %327:2 = "hivm.hir.vmulextui"(%236, %288, %325, %326) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 2>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> (tensor<3xi32>, tensor<3xi32>)
    %328 = "hivm.hir.vor"(%327#1, %309, %141) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %329 = "hivm.hir.vand"(%327#1, %309, %140) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %330 = "hivm.hir.vnot"(%329, %329) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %331 = "hivm.hir.vand"(%330, %328, %139) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %332 = "tensor.empty"() : () -> tensor<3xi32>
    %333 = "hivm.hir.vbrc"(%30, %332) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %334 = "hivm.hir.vor"(%331, %333, %138) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %335 = "tensor.empty"() : () -> tensor<3xi32>
    %336 = "hivm.hir.vbrc"(%30, %335) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %337 = "hivm.hir.vand"(%331, %336, %137) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %338 = "hivm.hir.vnot"(%337, %337) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %339 = "hivm.hir.vand"(%338, %334, %136) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %340 = "hivm.hir.vmul"(%307, %23, %135) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, i32, tensor<3xi32>) -> tensor<3xi32>
    %341 = "hivm.hir.vmul"(%288, %24, %134) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, i32, tensor<3xi32>) -> tensor<3xi32>
    %342 = "tensor.empty"() : () -> tensor<3xi32>
    %343 = "tensor.empty"() : () -> tensor<3xi32>
    %344:2 = "hivm.hir.vmulextui"(%237, %339, %342, %343) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 2>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> (tensor<3xi32>, tensor<3xi32>)
    %345 = "hivm.hir.vor"(%344#1, %340, %133) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %346 = "hivm.hir.vand"(%344#1, %340, %132) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %347 = "hivm.hir.vnot"(%346, %346) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %348 = "hivm.hir.vand"(%347, %345, %131) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %349 = "tensor.empty"() : () -> tensor<3xi32>
    %350 = "hivm.hir.vbrc"(%31, %349) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %351 = "hivm.hir.vor"(%348, %350, %130) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %352 = "tensor.empty"() : () -> tensor<3xi32>
    %353 = "hivm.hir.vbrc"(%31, %352) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %354 = "hivm.hir.vand"(%348, %353, %129) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %355 = "hivm.hir.vnot"(%354, %354) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %356 = "hivm.hir.vand"(%355, %351, %128) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %357 = "tensor.empty"() : () -> tensor<3xi32>
    %358 = "tensor.empty"() : () -> tensor<3xi32>
    %359:2 = "hivm.hir.vmulextui"(%236, %324, %357, %358) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 2>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> (tensor<3xi32>, tensor<3xi32>)
    %360 = "hivm.hir.vor"(%359#1, %341, %127) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %361 = "hivm.hir.vand"(%359#1, %341, %126) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %362 = "hivm.hir.vnot"(%361, %361) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %363 = "hivm.hir.vand"(%362, %360, %125) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %364 = "tensor.empty"() : () -> tensor<3xi32>
    %365 = "hivm.hir.vbrc"(%32, %364) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %366 = "hivm.hir.vor"(%363, %365, %124) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %367 = "tensor.empty"() : () -> tensor<3xi32>
    %368 = "hivm.hir.vbrc"(%32, %367) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %369 = "hivm.hir.vand"(%363, %368, %123) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %370 = "hivm.hir.vnot"(%369, %369) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %371 = "hivm.hir.vand"(%370, %366, %122) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %372 = "hivm.hir.vmul"(%339, %23, %121) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, i32, tensor<3xi32>) -> tensor<3xi32>
    %373 = "hivm.hir.vmul"(%324, %24, %120) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, i32, tensor<3xi32>) -> tensor<3xi32>
    %374 = "tensor.empty"() : () -> tensor<3xi32>
    %375 = "tensor.empty"() : () -> tensor<3xi32>
    %376:2 = "hivm.hir.vmulextui"(%237, %371, %374, %375) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 2>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> (tensor<3xi32>, tensor<3xi32>)
    %377 = "hivm.hir.vor"(%376#1, %372, %119) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %378 = "hivm.hir.vand"(%376#1, %372, %118) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %379 = "hivm.hir.vnot"(%378, %378) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %380 = "hivm.hir.vand"(%379, %377, %117) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %381 = "tensor.empty"() : () -> tensor<3xi32>
    %382 = "hivm.hir.vbrc"(%33, %381) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %383 = "hivm.hir.vor"(%380, %382, %116) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %384 = "tensor.empty"() : () -> tensor<3xi32>
    %385 = "hivm.hir.vbrc"(%33, %384) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %386 = "hivm.hir.vand"(%380, %385, %115) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %387 = "hivm.hir.vnot"(%386, %386) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %388 = "hivm.hir.vand"(%387, %383, %114) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %389 = "tensor.empty"() : () -> tensor<3xi32>
    %390 = "tensor.empty"() : () -> tensor<3xi32>
    %391:2 = "hivm.hir.vmulextui"(%236, %356, %389, %390) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 2>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> (tensor<3xi32>, tensor<3xi32>)
    %392 = "hivm.hir.vor"(%391#1, %373, %113) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %393 = "hivm.hir.vand"(%391#1, %373, %112) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %394 = "hivm.hir.vnot"(%393, %393) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %395 = "hivm.hir.vand"(%394, %392, %111) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %396 = "tensor.empty"() : () -> tensor<3xi32>
    %397 = "hivm.hir.vbrc"(%34, %396) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %398 = "hivm.hir.vor"(%395, %397, %110) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %399 = "tensor.empty"() : () -> tensor<3xi32>
    %400 = "hivm.hir.vbrc"(%34, %399) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %401 = "hivm.hir.vand"(%395, %400, %109) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %402 = "hivm.hir.vnot"(%401, %401) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %403 = "hivm.hir.vand"(%402, %398, %108) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %404 = "hivm.hir.vmul"(%371, %23, %107) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, i32, tensor<3xi32>) -> tensor<3xi32>
    %405 = "hivm.hir.vmul"(%356, %24, %106) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, i32, tensor<3xi32>) -> tensor<3xi32>
    %406 = "tensor.empty"() : () -> tensor<3xi32>
    %407 = "tensor.empty"() : () -> tensor<3xi32>
    %408:2 = "hivm.hir.vmulextui"(%237, %403, %406, %407) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 2>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> (tensor<3xi32>, tensor<3xi32>)
    %409 = "hivm.hir.vor"(%408#1, %404, %105) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %410 = "hivm.hir.vand"(%408#1, %404, %104) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %411 = "hivm.hir.vnot"(%410, %410) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %412 = "hivm.hir.vand"(%411, %409, %103) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %413 = "tensor.empty"() : () -> tensor<3xi32>
    %414 = "hivm.hir.vbrc"(%35, %413) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %415 = "hivm.hir.vor"(%412, %414, %102) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %416 = "tensor.empty"() : () -> tensor<3xi32>
    %417 = "hivm.hir.vbrc"(%35, %416) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %418 = "hivm.hir.vand"(%412, %417, %101) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %419 = "hivm.hir.vnot"(%418, %418) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %420 = "hivm.hir.vand"(%419, %415, %100) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %421 = "tensor.empty"() : () -> tensor<3xi32>
    %422 = "tensor.empty"() : () -> tensor<3xi32>
    %423:2 = "hivm.hir.vmulextui"(%236, %388, %421, %422) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 2>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> (tensor<3xi32>, tensor<3xi32>)
    %424 = "hivm.hir.vor"(%423#1, %405, %99) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %425 = "hivm.hir.vand"(%423#1, %405, %98) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %426 = "hivm.hir.vnot"(%425, %425) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %427 = "hivm.hir.vand"(%426, %424, %97) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %428 = "tensor.empty"() : () -> tensor<3xi32>
    %429 = "hivm.hir.vbrc"(%36, %428) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %430 = "hivm.hir.vor"(%427, %429, %96) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %431 = "tensor.empty"() : () -> tensor<3xi32>
    %432 = "hivm.hir.vbrc"(%36, %431) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %433 = "hivm.hir.vand"(%427, %432, %95) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %434 = "hivm.hir.vnot"(%433, %433) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %435 = "hivm.hir.vand"(%434, %430, %94) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %436 = "hivm.hir.vmul"(%403, %23, %93) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, i32, tensor<3xi32>) -> tensor<3xi32>
    %437 = "hivm.hir.vmul"(%388, %24, %92) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, i32, tensor<3xi32>) -> tensor<3xi32>
    %438 = "tensor.empty"() : () -> tensor<3xi32>
    %439 = "tensor.empty"() : () -> tensor<3xi32>
    %440:2 = "hivm.hir.vmulextui"(%237, %435, %438, %439) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 2>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> (tensor<3xi32>, tensor<3xi32>)
    %441 = "hivm.hir.vor"(%440#1, %436, %91) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %442 = "hivm.hir.vand"(%440#1, %436, %90) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %443 = "hivm.hir.vnot"(%442, %442) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %444 = "hivm.hir.vand"(%443, %441, %89) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %445 = "tensor.empty"() : () -> tensor<3xi32>
    %446 = "hivm.hir.vbrc"(%37, %445) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %447 = "hivm.hir.vor"(%444, %446, %88) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %448 = "tensor.empty"() : () -> tensor<3xi32>
    %449 = "hivm.hir.vbrc"(%37, %448) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %450 = "hivm.hir.vand"(%444, %449, %87) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %451 = "hivm.hir.vnot"(%450, %450) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %452 = "hivm.hir.vand"(%451, %447, %86) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %453 = "tensor.empty"() : () -> tensor<3xi32>
    %454 = "tensor.empty"() : () -> tensor<3xi32>
    %455:2 = "hivm.hir.vmulextui"(%236, %420, %453, %454) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 2>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> (tensor<3xi32>, tensor<3xi32>)
    %456 = "hivm.hir.vor"(%455#1, %437, %85) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %457 = "hivm.hir.vand"(%455#1, %437, %84) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %458 = "hivm.hir.vnot"(%457, %457) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %459 = "hivm.hir.vand"(%458, %456, %83) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %460 = "tensor.empty"() : () -> tensor<3xi32>
    %461 = "hivm.hir.vbrc"(%38, %460) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %462 = "hivm.hir.vor"(%459, %461, %82) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %463 = "tensor.empty"() : () -> tensor<3xi32>
    %464 = "hivm.hir.vbrc"(%38, %463) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %465 = "hivm.hir.vand"(%459, %464, %81) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %466 = "hivm.hir.vnot"(%465, %465) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %467 = "hivm.hir.vand"(%466, %462, %80) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %468 = "hivm.hir.vmul"(%420, %24, %79) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, i32, tensor<3xi32>) -> tensor<3xi32>
    %469 = "tensor.empty"() : () -> tensor<3xi32>
    %470 = "tensor.empty"() : () -> tensor<3xi32>
    %471:2 = "hivm.hir.vmulextui"(%236, %452, %469, %470) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 2>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> (tensor<3xi32>, tensor<3xi32>)
    %472 = "hivm.hir.vor"(%471#1, %468, %78) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %473 = "hivm.hir.vand"(%471#1, %468, %77) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %474 = "hivm.hir.vnot"(%473, %473) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %475 = "hivm.hir.vand"(%474, %472, %76) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %476 = "tensor.empty"() : () -> tensor<3xi32>
    %477 = "hivm.hir.vbrc"(%39, %476) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %478 = "hivm.hir.vor"(%475, %477, %75) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %479 = "tensor.empty"() : () -> tensor<3xi32>
    %480 = "hivm.hir.vbrc"(%39, %479) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %481 = "hivm.hir.vand"(%475, %480, %74) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %482 = "hivm.hir.vnot"(%481, %481) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %483 = "hivm.hir.vand"(%482, %478, %73) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %484 = "hivm.hir.vmul"(%467, %23, %72) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, i32, tensor<3xi32>) -> tensor<3xi32>
    %485 = "tensor.empty"() : () -> tensor<3xi32>
    %486 = "tensor.empty"() : () -> tensor<3xi32>
    %487:2 = "hivm.hir.vmulextui"(%237, %483, %485, %486) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 2>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> (tensor<3xi32>, tensor<3xi32>)
    %488 = "hivm.hir.vor"(%487#1, %484, %71) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %489 = "hivm.hir.vand"(%487#1, %484, %70) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %490 = "hivm.hir.vnot"(%489, %489) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %491 = "hivm.hir.vand"(%490, %488, %69) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %492 = "tensor.empty"() : () -> tensor<3xi32>
    %493 = "hivm.hir.vbrc"(%40, %492) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %494 = "hivm.hir.vor"(%491, %493, %68) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %495 = "tensor.empty"() : () -> tensor<3xi32>
    %496 = "hivm.hir.vbrc"(%40, %495) <{broadcast_dims = array<i64>}> : (i32, tensor<3xi32>) -> tensor<3xi32>
    %497 = "hivm.hir.vand"(%491, %496, %67) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %498 = "hivm.hir.vnot"(%497, %497) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %499 = "hivm.hir.vand"(%498, %494, %66) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %500 = "hivm.hir.vmul"(%483, %23, %65) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, i32, tensor<3xi32>) -> tensor<3xi32>
    %501 = "hivm.hir.vmax"(%499, %41, %64) <{broadcast = array<i64>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, i32, tensor<3xi32>) -> tensor<3xi32>
    %502 = "tensor.empty"() : () -> tensor<3xi1>
    %503 = "tensor.empty"() : () -> tensor<3xi1>
    %504 = "tensor.empty"() : () -> tensor<3xi1>
    %505 = "tensor.empty"() : () -> tensor<3xi1>
    %506 = "tensor.empty"() : () -> tensor<3xi1>
    %507 = "tensor.empty"() : () -> tensor<3xi1>
    %508 = "tensor.empty"() : () -> tensor<3xi1>
    %509 = "tensor.empty"() : () -> tensor<3xi1>
    %510 = "hivm.hir.vcmp"(%501, %499, %509) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi1>) -> tensor<3xi1>
    %511 = "hivm.hir.vnot"(%510, %508) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi1>, tensor<3xi1>) -> tensor<3xi1>
    %512 = "hivm.hir.vmul"(%499, %1, %63) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, i32, tensor<3xi32>) -> tensor<3xi32>
    %513 = "hivm.hir.vadd"(%512, %41, %62) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, i32, tensor<3xi32>) -> tensor<3xi32>
    %514 = "hivm.hir.vsub"(%513, %0, %61) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, i32, tensor<3xi32>) -> tensor<3xi32>
    %515 = "hivm.hir.vsel"(%511, %514, %499, %60) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<3xi1>, tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %516 = "hivm.hir.vcast"(%515, %233) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xf32>) -> tensor<3xf32>
    %517 = "hivm.hir.vmul"(%516, %42, %232) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %518 = "hivm.hir.vmax"(%500, %41, %59) <{broadcast = array<i64>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, i32, tensor<3xi32>) -> tensor<3xi32>
    %519 = "hivm.hir.vcmp"(%518, %500, %507) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xi32>, tensor<3xi1>) -> tensor<3xi1>
    %520 = "hivm.hir.vnot"(%519, %506) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi1>, tensor<3xi1>) -> tensor<3xi1>
    %521 = "hivm.hir.vmul"(%500, %1, %58) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, i32, tensor<3xi32>) -> tensor<3xi32>
    %522 = "hivm.hir.vadd"(%521, %41, %57) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, i32, tensor<3xi32>) -> tensor<3xi32>
    %523 = "hivm.hir.vsub"(%522, %0, %56) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xi32>, i32, tensor<3xi32>) -> tensor<3xi32>
    %524 = "hivm.hir.vsel"(%520, %523, %500, %55) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<3xi1>, tensor<3xi32>, tensor<3xi32>, tensor<3xi32>) -> tensor<3xi32>
    %525 = "hivm.hir.vcast"(%524, %231) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<rint>, transpose = array<i64>}> : (tensor<3xi32>, tensor<3xf32>) -> tensor<3xf32>
    %526 = "hivm.hir.vmul"(%525, %42, %230) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %527 = "hivm.hir.vcmp"(%517, %517, %505) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, tensor<3xf32>, tensor<3xi1>) -> tensor<3xi1>
    %528 = "hivm.hir.vnot"(%527, %504) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi1>, tensor<3xi1>) -> tensor<3xi1>
    %529 = "hivm.hir.vcmp"(%235, %235, %503) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<eq>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, tensor<3xf32>, tensor<3xi1>) -> tensor<3xi1>
    %530 = "hivm.hir.vnot"(%529, %502) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xi1>, tensor<3xi1>) -> tensor<3xi1>
    %531 = "hivm.hir.vmax"(%517, %45, %229) <{broadcast = array<i64>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %532 = "hivm.hir.vsel"(%528, %45, %531, %228) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<3xi1>, f32, tensor<3xf32>, tensor<3xf32>) -> tensor<3xf32>
    %533 = "hivm.hir.vsel"(%530, %517, %532, %227) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<3xi1>, tensor<3xf32>, tensor<3xf32>, tensor<3xf32>) -> tensor<3xf32>
    %534 = "hivm.hir.vmul"(%526, %44, %226) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %535 = "hivm.hir.vln"(%533, %225) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, tensor<3xf32>) -> tensor<3xf32>
    %536 = "hivm.hir.vmul"(%535, %43, %224) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %537 = "hivm.hir.vsqrt"(%536, %223) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, tensor<3xf32>) -> tensor<3xf32>
    %538 = "hivm.hir.vmul"(%534, %18, %222) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %539 = "hivm.hir.vadd"(%538, %17, %221) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %540 = "hivm.hir.vmul"(%538, %16, %220) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %541 = "hivm.hir.vcast"(%539, %219) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<round>, transpose = array<i64>}> : (tensor<3xf32>, tensor<3xf32>) -> tensor<3xf32>
    %542 = "hivm.hir.vcast"(%540, %218) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<round>, transpose = array<i64>}> : (tensor<3xf32>, tensor<3xf32>) -> tensor<3xf32>
    %543 = "hivm.hir.vmul"(%542, %15, %217) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %544 = "hivm.hir.vsub"(%541, %543, %216) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, tensor<3xf32>, tensor<3xf32>) -> tensor<3xf32>
    %545 = "hivm.hir.vmul"(%543, %14, %215) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %546 = "hivm.hir.vsub"(%534, %545, %214) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, tensor<3xf32>, tensor<3xf32>) -> tensor<3xf32>
    %547 = "hivm.hir.vmul"(%544, %14, %213) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %548 = "hivm.hir.vsub"(%546, %547, %212) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, tensor<3xf32>, tensor<3xf32>) -> tensor<3xf32>
    %549 = "hivm.hir.vmul"(%543, %13, %211) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %550 = "hivm.hir.vsub"(%548, %549, %210) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, tensor<3xf32>, tensor<3xf32>) -> tensor<3xf32>
    %551 = "hivm.hir.vadd"(%550, %12, %209) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %552 = "hivm.hir.vmul"(%544, %13, %208) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %553 = "hivm.hir.vsub"(%551, %552, %207) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, tensor<3xf32>, tensor<3xf32>) -> tensor<3xf32>
    %554 = "hivm.hir.vmul"(%543, %11, %206) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %555 = "hivm.hir.vsub"(%553, %554, %205) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, tensor<3xf32>, tensor<3xf32>) -> tensor<3xf32>
    %556 = "hivm.hir.vmul"(%544, %11, %204) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %557 = "hivm.hir.vsub"(%555, %556, %203) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, tensor<3xf32>, tensor<3xf32>) -> tensor<3xf32>
    %558 = "hivm.hir.vmul"(%543, %10, %202) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %559 = "hivm.hir.vsub"(%557, %558, %201) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, tensor<3xf32>, tensor<3xf32>) -> tensor<3xf32>
    %560 = "hivm.hir.vmul"(%544, %10, %200) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %561 = "hivm.hir.vsub"(%559, %560, %199) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, tensor<3xf32>, tensor<3xf32>) -> tensor<3xf32>
    %562 = "hivm.hir.vadd"(%561, %9, %198) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %563 = "hivm.hir.vmul"(%541, %17, %197) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %564 = "hivm.hir.vcast"(%563, %196) <{broadcast = array<i64>, cast = #hivm.cast<cast_signed>, operandSegmentSizes = array<i32: 1, 1, 0>, round_mode = #hivm.round_mode<floor>, transpose = array<i64>}> : (tensor<3xf32>, tensor<3xf32>) -> tensor<3xf32>
    %565 = "hivm.hir.vmul"(%564, %8, %195) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %566 = "hivm.hir.vmul"(%541, %43, %194) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %567 = "hivm.hir.vadd"(%565, %566, %193) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, tensor<3xf32>, tensor<3xf32>) -> tensor<3xf32>
    %568 = "hivm.hir.vadd"(%567, %7, %192) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %569 = "hivm.hir.vmul"(%562, %562, %191) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, tensor<3xf32>, tensor<3xf32>) -> tensor<3xf32>
    %570 = "hivm.hir.vmul"(%569, %6, %190) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %571 = "hivm.hir.vadd"(%570, %5, %189) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %572 = "hivm.hir.vmul"(%571, %569, %188) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, tensor<3xf32>, tensor<3xf32>) -> tensor<3xf32>
    %573 = "hivm.hir.vadd"(%572, %4, %187) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %574 = "hivm.hir.vmul"(%573, %569, %186) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, tensor<3xf32>, tensor<3xf32>) -> tensor<3xf32>
    %575 = "hivm.hir.vadd"(%574, %3, %185) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %576 = "hivm.hir.vmul"(%575, %569, %184) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, tensor<3xf32>, tensor<3xf32>) -> tensor<3xf32>
    %577 = "hivm.hir.vadd"(%576, %7, %183) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %578 = "hivm.hir.vmul"(%577, %562, %182) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, tensor<3xf32>, tensor<3xf32>) -> tensor<3xf32>
    %579 = "hivm.hir.vmul"(%578, %568, %181) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, tensor<3xf32>, tensor<3xf32>) -> tensor<3xf32>
    %580 = "hivm.hir.vmin"(%579, %7, %180) <{broadcast = array<i64>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %581 = "hivm.hir.vmax"(%580, %2, %179) <{broadcast = array<i64>, is_signed = true, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, f32, tensor<3xf32>) -> tensor<3xf32>
    %582 = "hivm.hir.vmul"(%537, %581, %178) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<3xf32>, tensor<3xf32>, tensor<3xf32>) -> tensor<3xf32>
    %583 = "arith.index_cast"(%238) : (i32) -> index
    %584 = "memref.reinterpret_cast"(%arg3, %583) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 3>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<3xf32, strided<[1], offset: ?>>
    %585 = "affine.apply"(%583) <{map = #map}> : (index) -> index
    %586 = "affine.max"(%583) <{map = #map1}> : (index) -> index
    %587 = "affine.min"(%585, %586) <{map = #map2}> : (index, index) -> index
    %588 = "affine.apply"(%587, %583) <{map = #map3}> : (index, index) -> index
    %589 = "tensor.extract_slice"(%582, %588) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<3xf32>, index) -> tensor<?xf32>
    %590 = "memref.subview"(%584, %588) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<3xf32, strided<[1], offset: ?>>, index) -> memref<?xf32, strided<[1], offset: ?>>
    "hivm.hir.store"(%589, %590) : (tensor<?xf32>, memref<?xf32, strided<[1], offset: ?>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, false, false, false]> : vector<7xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hacc.target = #hacc.target<"Ascend910B1">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

