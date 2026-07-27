"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xf32>, i32, i32, i32) -> (), sym_name = "kernel_rand"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xf32>, %arg4: i32, %arg5: i32, %arg6: i32):
    %0 = "arith.constant"() <{value = 1 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = 462789791 : i32}> : () -> i32
    %2 = "arith.constant"() <{value = 4 : i32}> : () -> i32
    %3 = "arith.constant"() <{value = -845247145 : i32}> : () -> i32
    %4 = "arith.constant"() <{value = -766435501 : i32}> : () -> i32
    %5 = "arith.constant"() <{value = -1640531522 : i32}> : () -> i32
    %6 = "arith.constant"() <{value = -1150833019 : i32}> : () -> i32
    %7 = "arith.constant"() <{value = 1013904247 : i32}> : () -> i32
    %8 = "arith.constant"() <{value = 1993301258 : i32}> : () -> i32
    %9 = "arith.constant"() <{value = -626627280 : i32}> : () -> i32
    %10 = "arith.constant"() <{value = 842468239 : i32}> : () -> i32
    %11 = "arith.constant"() <{value = 2027808489 : i32}> : () -> i32
    %12 = "arith.constant"() <{value = -308364780 : i32}> : () -> i32
    %13 = "arith.constant"() <{value = 387276962 : i32}> : () -> i32
    %14 = "arith.constant"() <{value = -1459197799 : i32}> : () -> i32
    %15 = "arith.constant"() <{value = -1253254565 : i32}> : () -> i32
    %16 = "arith.constant"() <{value = 1684936478 : i32}> : () -> i32
    %17 = "arith.constant"() <{value = 1401181204 : i32}> : () -> i32
    %18 = "arith.constant"() <{value = 534103459 : i32}> : () -> i32
    %19 = "arith.constant"() <{value = -616729560 : i32}> : () -> i32
    %20 = "arith.constant"() <{value = -1879881850 : i32}> : () -> i32
    %21 = "arith.constant"() <{value = -1 : i32}> : () -> i32
    %22 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %23 = "arith.constant"() <{value = 10 : i32}> : () -> i32
    %24 = "arith.constant"() <{value = 3 : i32}> : () -> i32
    %25 = "arith.constant"() <{value = 0 : index}> : () -> index
    %26 = "arith.constant"() <{value = 4.6566126E-10 : f32}> : () -> f32
    "hivm.hir.set_mask_norm"() : () -> ()
    %27 = "arith.muli"(%arg4, %arg5) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %28 = "arith.muli"(%27, %arg6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%28) <{effects = ["write"]}> {logical_block_num} : (i32) -> ()
    %29 = "hivm.hir.get_block_idx"() : () -> i64
    %30 = "arith.trunci"(%29) : (i64) -> i32
    %31 = "arith.muli"(%arg6, %arg5) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %32 = "arith.divsi"(%30, %31) : (i32, i32) -> i32
    %33 = "arith.remsi"(%32, %arg4) : (i32, i32) -> i32
    %34 = "tensor.empty"() : () -> tensor<1xf32>
    %35 = "arith.muli"(%33, %24) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %36 = "arith.addi"(%35, %24) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %37 = "arith.cmpi"(%36, %24) <{predicate = 3 : i64}> : (i32, i32) -> i1
    %38 = "scf.if"(%37) ({
      "scf.yield"(%24) : (i32) -> ()
    }, {
      %112 = "arith.subi"(%24, %35) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      "scf.yield"(%112) : (i32) -> ()
    }) : (i1) -> i32
    "scf.for"(%22, %38, %0) ({
    ^bb0(%arg7: i32):
      %39 = "arith.addi"(%35, %arg7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %40 = "arith.addi"(%39, %23) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %41:2 = "arith.mului_extended"(%40, %4) : (i32, i32) -> (i32, i32)
      %42 = "arith.muli"(%40, %4) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %43:2 = "arith.mului_extended"(%41#1, %3) : (i32, i32) -> (i32, i32)
      %44 = "arith.xori"(%43#1, %5) : (i32, i32) -> i32
      %45 = "arith.xori"(%42, %2) : (i32, i32) -> i32
      %46 = "arith.xori"(%45, %6) : (i32, i32) -> i32
      %47 = "arith.muli"(%41#1, %3) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %48:2 = "arith.mului_extended"(%46, %3) : (i32, i32) -> (i32, i32)
      %49 = "arith.xori"(%48#1, %47) : (i32, i32) -> i32
      %50 = "arith.xori"(%49, %7) : (i32, i32) -> i32
      %51:2 = "arith.mului_extended"(%44, %4) : (i32, i32) -> (i32, i32)
      %52 = "arith.xori"(%51#1, %1) : (i32, i32) -> i32
      %53 = "arith.xori"(%52, %8) : (i32, i32) -> i32
      %54 = "arith.muli"(%46, %3) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %55 = "arith.muli"(%44, %4) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %56:2 = "arith.mului_extended"(%53, %3) : (i32, i32) -> (i32, i32)
      %57 = "arith.xori"(%56#1, %54) : (i32, i32) -> i32
      %58 = "arith.xori"(%57, %9) : (i32, i32) -> i32
      %59:2 = "arith.mului_extended"(%50, %4) : (i32, i32) -> (i32, i32)
      %60 = "arith.xori"(%59#1, %55) : (i32, i32) -> i32
      %61 = "arith.xori"(%60, %10) : (i32, i32) -> i32
      %62 = "arith.muli"(%53, %3) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %63 = "arith.muli"(%50, %4) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %64:2 = "arith.mului_extended"(%61, %3) : (i32, i32) -> (i32, i32)
      %65 = "arith.xori"(%64#1, %62) : (i32, i32) -> i32
      %66 = "arith.xori"(%65, %11) : (i32, i32) -> i32
      %67:2 = "arith.mului_extended"(%58, %4) : (i32, i32) -> (i32, i32)
      %68 = "arith.xori"(%67#1, %63) : (i32, i32) -> i32
      %69 = "arith.xori"(%68, %12) : (i32, i32) -> i32
      %70 = "arith.muli"(%61, %3) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %71 = "arith.muli"(%58, %4) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %72:2 = "arith.mului_extended"(%69, %3) : (i32, i32) -> (i32, i32)
      %73 = "arith.xori"(%72#1, %70) : (i32, i32) -> i32
      %74 = "arith.xori"(%73, %13) : (i32, i32) -> i32
      %75:2 = "arith.mului_extended"(%66, %4) : (i32, i32) -> (i32, i32)
      %76 = "arith.xori"(%75#1, %71) : (i32, i32) -> i32
      %77 = "arith.xori"(%76, %14) : (i32, i32) -> i32
      %78 = "arith.muli"(%69, %3) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %79 = "arith.muli"(%66, %4) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %80:2 = "arith.mului_extended"(%77, %3) : (i32, i32) -> (i32, i32)
      %81 = "arith.xori"(%80#1, %78) : (i32, i32) -> i32
      %82 = "arith.xori"(%81, %15) : (i32, i32) -> i32
      %83:2 = "arith.mului_extended"(%74, %4) : (i32, i32) -> (i32, i32)
      %84 = "arith.xori"(%83#1, %79) : (i32, i32) -> i32
      %85 = "arith.xori"(%84, %16) : (i32, i32) -> i32
      %86 = "arith.muli"(%77, %3) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %87 = "arith.muli"(%74, %4) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %88:2 = "arith.mului_extended"(%85, %3) : (i32, i32) -> (i32, i32)
      %89 = "arith.xori"(%88#1, %86) : (i32, i32) -> i32
      %90 = "arith.xori"(%89, %17) : (i32, i32) -> i32
      %91:2 = "arith.mului_extended"(%82, %4) : (i32, i32) -> (i32, i32)
      %92 = "arith.xori"(%91#1, %87) : (i32, i32) -> i32
      %93 = "arith.xori"(%92, %18) : (i32, i32) -> i32
      %94 = "arith.muli"(%82, %4) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %95:2 = "arith.mului_extended"(%90, %4) : (i32, i32) -> (i32, i32)
      %96 = "arith.xori"(%95#1, %94) : (i32, i32) -> i32
      %97 = "arith.xori"(%96, %19) : (i32, i32) -> i32
      %98 = "arith.muli"(%93, %3) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %99:2 = "arith.mului_extended"(%97, %3) : (i32, i32) -> (i32, i32)
      %100 = "arith.xori"(%99#1, %98) : (i32, i32) -> i32
      %101 = "arith.xori"(%100, %20) : (i32, i32) -> i32
      %102 = "arith.cmpi"(%101, %22) <{predicate = 2 : i64}> : (i32, i32) -> i1
      %103 = "arith.subi"(%21, %101) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %104 = "arith.select"(%102, %103, %101) : (i1, i32, i32) -> i32
      %105 = "arith.sitofp"(%104) : (i32) -> f32
      %106 = "tensor.insert"(%105, %34, %25) : (f32, tensor<1xf32>, index) -> tensor<1xf32>
      %107 = "hivm.hir.vmul"(%106, %26, %34) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1xf32>, f32, tensor<1xf32>) -> tensor<1xf32>
      %108 = "tensor.extract"(%107, %25) : (tensor<1xf32>, index) -> f32
      %109 = "arith.index_cast"(%39) : (i32) -> index
      %110 = "tensor.insert"(%108, %34, %25) : (f32, tensor<1xf32>, index) -> tensor<1xf32>
      %111 = "memref.reinterpret_cast"(%arg3, %109) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<1xf32, strided<[1], offset: ?>>
      "hivm.hir.store"(%110, %111) : (tensor<1xf32>, memref<1xf32, strided<[1], offset: ?>>) -> ()
      "scf.yield"() : () -> ()
    }) : (i32, i32, i32) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, false, false, false]> : vector<7xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hacc.target = #hacc.target<"Ascend910B1">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

