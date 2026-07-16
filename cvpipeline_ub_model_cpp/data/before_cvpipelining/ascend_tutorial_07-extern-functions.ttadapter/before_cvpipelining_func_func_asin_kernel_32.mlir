"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xf32>, memref<?xf32>, i32, i32, i32, i32) -> (), sym_name = "asin_kernel"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xf32>, %arg4: memref<?xf32>, %arg5: i32, %arg6: i32, %arg7: i32, %arg8: i32):
    %0 = "arith.constant"() <{value = -1.000000e+00 : f32}> : () -> f32
    %1 = "arith.constant"() <{value = 1024 : index}> : () -> index
    %2 = "arith.constant"() <{value = 6.000000e-01 : f32}> : () -> f32
    %3 = "arith.constant"() <{value = 3.14159274 : f32}> : () -> f32
    %4 = "arith.constant"() <{value = 2.000000e+00 : f32}> : () -> f32
    %5 = "arith.constant"() <{value = -3.333310e-01 : f32}> : () -> f32
    %6 = "arith.constant"() <{value = 0.199934095 : f32}> : () -> f32
    %7 = "arith.constant"() <{value = -1.420890e-01 : f32}> : () -> f32
    %8 = "arith.constant"() <{value = 0.106597602 : f32}> : () -> f32
    %9 = "arith.constant"() <{value = 2.237200e-02 : f32}> : () -> f32
    %10 = "arith.constant"() <{value = 3.038000e-02 : f32}> : () -> f32
    %11 = "arith.constant"() <{value = 4.464300e-02 : f32}> : () -> f32
    %12 = "arith.constant"() <{value = 7.500000e-02 : f32}> : () -> f32
    %13 = "arith.constant"() <{value = 1.000000e+00 : f32}> : () -> f32
    %14 = "arith.constant"() <{value = 1.666670e-01 : f32}> : () -> f32
    %15 = "arith.constant"() <{value = 1.57079637 : f32}> : () -> f32
    %16 = "arith.constant"() <{value = 1024 : i32}> : () -> i32
    %17 = "arith.constant"() <{value = 0.000000e+00 : f32}> : () -> f32
    %18 = "arith.constant"() <{value = 0 : index}> : () -> index
    "hivm.hir.set_mask_norm"() : () -> ()
    %19 = "arith.muli"(%arg6, %arg7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %20 = "arith.muli"(%19, %arg8) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%20) {logical_block_num} : (i32) -> ()
    %21 = "hivm.hir.get_block_idx"() : () -> i64
    %22 = "arith.trunci"(%21) : (i64) -> i32
    %23 = "arith.muli"(%arg8, %arg7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %24 = "arith.divsi"(%22, %23) : (i32, i32) -> i32
    %25 = "arith.remsi"(%24, %arg6) : (i32, i32) -> i32
    %26 = "tensor.empty"() : () -> tensor<1024xf32>
    %27 = "tensor.empty"() : () -> tensor<1024xf32>
    %28 = "tensor.empty"() : () -> tensor<1024xf32>
    %29 = "tensor.empty"() : () -> tensor<1024xf32>
    %30 = "tensor.empty"() : () -> tensor<1024xf32>
    %31 = "tensor.empty"() : () -> tensor<1024xf32>
    %32 = "tensor.empty"() : () -> tensor<1024xf32>
    %33 = "tensor.empty"() : () -> tensor<1024xf32>
    %34 = "tensor.empty"() : () -> tensor<1024xf32>
    %35 = "tensor.empty"() : () -> tensor<1024xf32>
    %36 = "tensor.empty"() : () -> tensor<1024xf32>
    %37 = "tensor.empty"() : () -> tensor<1024xf32>
    %38 = "tensor.empty"() : () -> tensor<1024xf32>
    %39 = "tensor.empty"() : () -> tensor<1024xf32>
    %40 = "tensor.empty"() : () -> tensor<1024xf32>
    %41 = "tensor.empty"() : () -> tensor<1024xf32>
    %42 = "tensor.empty"() : () -> tensor<1024xf32>
    %43 = "tensor.empty"() : () -> tensor<1024xf32>
    %44 = "tensor.empty"() : () -> tensor<1024xf32>
    %45 = "tensor.empty"() : () -> tensor<1024xf32>
    %46 = "tensor.empty"() : () -> tensor<1024xf32>
    %47 = "tensor.empty"() : () -> tensor<1024xf32>
    %48 = "tensor.empty"() : () -> tensor<1024xf32>
    %49 = "tensor.empty"() : () -> tensor<1024xf32>
    %50 = "tensor.empty"() : () -> tensor<1024xf32>
    %51 = "tensor.empty"() : () -> tensor<1024xf32>
    %52 = "tensor.empty"() : () -> tensor<1024xf32>
    %53 = "tensor.empty"() : () -> tensor<1024xf32>
    %54 = "tensor.empty"() : () -> tensor<1024xf32>
    %55 = "tensor.empty"() : () -> tensor<1024xf32>
    %56 = "tensor.empty"() : () -> tensor<1024xf32>
    %57 = "tensor.empty"() : () -> tensor<1024xf32>
    %58 = "tensor.empty"() : () -> tensor<1024xf32>
    %59 = "tensor.empty"() : () -> tensor<1024xf32>
    %60 = "tensor.empty"() : () -> tensor<1024xf32>
    %61 = "tensor.empty"() : () -> tensor<1024xf32>
    %62 = "tensor.empty"() : () -> tensor<1024xf32>
    %63 = "tensor.empty"() : () -> tensor<1024xf32>
    %64 = "tensor.empty"() : () -> tensor<1024xf32>
    %65 = "tensor.empty"() : () -> tensor<1024xf32>
    %66 = "tensor.empty"() : () -> tensor<1024xf32>
    %67 = "tensor.empty"() : () -> tensor<1024xf32>
    %68 = "tensor.empty"() : () -> tensor<1024xf32>
    %69 = "hivm.hir.vbrc"(%17, %68) <{broadcast_dims = array<i64>}> : (f32, tensor<1024xf32>) -> tensor<1024xf32>
    %70 = "hivm.hir.vbrc"(%2, %67) <{broadcast_dims = array<i64>}> : (f32, tensor<1024xf32>) -> tensor<1024xf32>
    %71 = "arith.muli"(%25, %16) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %72 = "arith.index_cast"(%71) : (i32) -> index
    %73 = "memref.reinterpret_cast"(%arg3, %72) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1024>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<1024xf32, strided<[1], offset: ?>>
    %74 = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<1024xf32>
    %75 = "arith.addi"(%72, %1) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %76 = "arith.index_cast"(%arg5) : (i32) -> index
    %77 = "arith.maxsi"(%72, %76) : (index, index) -> index
    %78 = "arith.minsi"(%75, %77) : (index, index) -> index
    %79 = "arith.subi"(%78, %72) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
    %80 = "arith.cmpi"(%79, %1) <{predicate = 2 : i64}> : (index, index) -> i1
    %81 = "memref.subview"(%73, %79) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<1024xf32, strided<[1], offset: ?>>, index) -> memref<?xf32, strided<[1], offset: ?>>
    %82 = "memref.subview"(%74, %79) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<1024xf32>, index) -> memref<?xf32, strided<[1]>>
    "hivm.hir.load"(%81, %82, %17, %18, %80) <{init_out_buffer = true, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 1, 1, 0, 1>, pad_mode = #hivm.padmode<PadValue>, tcoretype = #hivm.tcore_type<VECTOR>}> : (memref<?xf32, strided<[1], offset: ?>>, memref<?xf32, strided<[1]>>, f32, index, i1) -> ()
    %83 = "bufferization.to_tensor"(%74) <{restrict, writable}> : (memref<1024xf32>) -> tensor<1024xf32>
    %84 = "hivm.hir.vabs"(%83, %66) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, tensor<1024xf32>) -> tensor<1024xf32>
    %85 = "hivm.hir.vmul"(%83, %83, %65) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, tensor<1024xf32>, tensor<1024xf32>) -> tensor<1024xf32>
    %86 = "hivm.hir.vmul"(%85, %85, %64) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, tensor<1024xf32>, tensor<1024xf32>) -> tensor<1024xf32>
    %87 = "hivm.hir.vmul"(%86, %85, %63) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, tensor<1024xf32>, tensor<1024xf32>) -> tensor<1024xf32>
    %88 = "hivm.hir.vmul"(%87, %85, %62) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, tensor<1024xf32>, tensor<1024xf32>) -> tensor<1024xf32>
    %89 = "hivm.hir.vmul"(%88, %85, %61) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, tensor<1024xf32>, tensor<1024xf32>) -> tensor<1024xf32>
    %90 = "hivm.hir.vmul"(%85, %14, %60) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, f32, tensor<1024xf32>) -> tensor<1024xf32>
    %91 = "hivm.hir.vadd"(%90, %13, %59) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, f32, tensor<1024xf32>) -> tensor<1024xf32>
    %92 = "hivm.hir.vmul"(%86, %12, %58) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, f32, tensor<1024xf32>) -> tensor<1024xf32>
    %93 = "hivm.hir.vadd"(%91, %92, %57) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, tensor<1024xf32>, tensor<1024xf32>) -> tensor<1024xf32>
    %94 = "hivm.hir.vmul"(%87, %11, %56) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, f32, tensor<1024xf32>) -> tensor<1024xf32>
    %95 = "hivm.hir.vadd"(%93, %94, %55) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, tensor<1024xf32>, tensor<1024xf32>) -> tensor<1024xf32>
    %96 = "hivm.hir.vmul"(%88, %10, %54) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, f32, tensor<1024xf32>) -> tensor<1024xf32>
    %97 = "hivm.hir.vadd"(%95, %96, %53) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, tensor<1024xf32>, tensor<1024xf32>) -> tensor<1024xf32>
    %98 = "hivm.hir.vmul"(%89, %9, %52) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, f32, tensor<1024xf32>) -> tensor<1024xf32>
    %99 = "hivm.hir.vadd"(%97, %98, %51) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, tensor<1024xf32>, tensor<1024xf32>) -> tensor<1024xf32>
    %100 = "hivm.hir.vmul"(%83, %99, %50) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, tensor<1024xf32>, tensor<1024xf32>) -> tensor<1024xf32>
    %101 = "hivm.hir.vmul"(%100, %0, %49) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, f32, tensor<1024xf32>) -> tensor<1024xf32>
    %102 = "hivm.hir.vadd"(%101, %15, %48) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, f32, tensor<1024xf32>) -> tensor<1024xf32>
    %103 = "hivm.hir.vmul"(%84, %0, %47) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, f32, tensor<1024xf32>) -> tensor<1024xf32>
    %104 = "hivm.hir.vadd"(%103, %13, %46) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, f32, tensor<1024xf32>) -> tensor<1024xf32>
    %105 = "hivm.hir.vadd"(%84, %13, %45) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, f32, tensor<1024xf32>) -> tensor<1024xf32>
    %106 = "hivm.hir.vdiv"(%104, %105, %44) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, tensor<1024xf32>, tensor<1024xf32>) -> tensor<1024xf32>
    %107 = "hivm.hir.vsqrt"(%106, %43) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 1, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, tensor<1024xf32>) -> tensor<1024xf32>
    %108 = "hivm.hir.vmul"(%107, %107, %42) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, tensor<1024xf32>, tensor<1024xf32>) -> tensor<1024xf32>
    %109 = "hivm.hir.vmul"(%108, %8, %41) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, f32, tensor<1024xf32>) -> tensor<1024xf32>
    %110 = "hivm.hir.vadd"(%109, %7, %40) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, f32, tensor<1024xf32>) -> tensor<1024xf32>
    %111 = "hivm.hir.vmul"(%110, %108, %39) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, tensor<1024xf32>, tensor<1024xf32>) -> tensor<1024xf32>
    %112 = "hivm.hir.vadd"(%111, %6, %38) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, f32, tensor<1024xf32>) -> tensor<1024xf32>
    %113 = "hivm.hir.vmul"(%112, %108, %37) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, tensor<1024xf32>, tensor<1024xf32>) -> tensor<1024xf32>
    %114 = "hivm.hir.vadd"(%113, %5, %36) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, f32, tensor<1024xf32>) -> tensor<1024xf32>
    %115 = "hivm.hir.vmul"(%114, %108, %35) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, tensor<1024xf32>, tensor<1024xf32>) -> tensor<1024xf32>
    %116 = "hivm.hir.vadd"(%115, %13, %34) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, f32, tensor<1024xf32>) -> tensor<1024xf32>
    %117 = "hivm.hir.vmul"(%107, %116, %33) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, tensor<1024xf32>, tensor<1024xf32>) -> tensor<1024xf32>
    %118 = "hivm.hir.vmul"(%117, %4, %32) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, f32, tensor<1024xf32>) -> tensor<1024xf32>
    %119 = "tensor.empty"() : () -> tensor<1024xi1>
    %120 = "tensor.empty"() : () -> tensor<1024xi1>
    %121 = "hivm.hir.vcmp"(%83, %69, %120) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<lt>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<1024xf32>, tensor<1024xf32>, tensor<1024xi1>) -> tensor<1024xi1>
    %122 = "hivm.hir.vmul"(%118, %0, %31) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, f32, tensor<1024xf32>) -> tensor<1024xf32>
    %123 = "hivm.hir.vadd"(%122, %3, %30) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, f32, tensor<1024xf32>) -> tensor<1024xf32>
    %124 = "hivm.hir.vsel"(%121, %123, %118, %29) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<1024xi1>, tensor<1024xf32>, tensor<1024xf32>, tensor<1024xf32>) -> tensor<1024xf32>
    %125 = "hivm.hir.vcmp"(%84, %70, %119) <{broadcast = array<i64>, compare_mode = #hivm.compare_mode<lt>, operandSegmentSizes = array<i32: 2, 1>, transpose = array<i64>}> : (tensor<1024xf32>, tensor<1024xf32>, tensor<1024xi1>) -> tensor<1024xi1>
    %126 = "hivm.hir.vsel"(%125, %102, %124, %28) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 3, 1, 0>, transpose = array<i64>}> : (tensor<1024xi1>, tensor<1024xf32>, tensor<1024xf32>, tensor<1024xf32>) -> tensor<1024xf32>
    %127 = "hivm.hir.vmul"(%126, %0, %27) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, f32, tensor<1024xf32>) -> tensor<1024xf32>
    %128 = "hivm.hir.vadd"(%127, %15, %26) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1024xf32>, f32, tensor<1024xf32>) -> tensor<1024xf32>
    %129 = "memref.reinterpret_cast"(%arg4, %72) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1024>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<1024xf32, strided<[1], offset: ?>>
    %130 = "tensor.extract_slice"(%128, %79) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (tensor<1024xf32>, index) -> tensor<?xf32>
    %131 = "memref.subview"(%129, %79) <{operandSegmentSizes = array<i32: 1, 0, 1, 0>, static_offsets = array<i64: 0>, static_sizes = array<i64: -9223372036854775808>, static_strides = array<i64: 1>}> : (memref<1024xf32, strided<[1], offset: ?>>, index) -> memref<?xf32, strided<[1], offset: ?>>
    "hivm.hir.store"(%130, %131) : (tensor<?xf32>, memref<?xf32, strided<[1], offset: ?>>) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, false, false, false, false]> : vector<9xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

