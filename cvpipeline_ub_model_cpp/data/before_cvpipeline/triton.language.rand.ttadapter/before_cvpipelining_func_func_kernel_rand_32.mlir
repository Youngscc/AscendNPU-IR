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
    "annotation.mark"(%28) {logical_block_num} : (i32) -> ()
    %29 = "hivm.hir.get_block_idx"() : () -> i64
    %30 = "arith.trunci"(%29) : (i64) -> i32
    %31 = "arith.muli"(%arg6, %arg5) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %32 = "arith.divsi"(%30, %31) : (i32, i32) -> i32
    %33 = "arith.remsi"(%32, %arg4) : (i32, i32) -> i32
    %34 = "tensor.empty"() : () -> tensor<1xf32>
    %35 = "tensor.empty"() : () -> tensor<1xf32>
    %36 = "tensor.empty"() : () -> tensor<1xf32>
    %37 = "arith.muli"(%33, %24) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %38 = "arith.addi"(%37, %24) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %39 = "arith.cmpi"(%38, %24) <{predicate = 3 : i64}> : (i32, i32) -> i1
    %40 = "scf.if"(%39) ({
      "scf.yield"(%24) : (i32) -> ()
    }, {
      %114 = "arith.subi"(%24, %37) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      "scf.yield"(%114) : (i32) -> ()
    }) : (i1) -> i32
    "scf.for"(%22, %40, %0) ({
    ^bb0(%arg7: i32):
      %41 = "arith.addi"(%37, %arg7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %42 = "arith.addi"(%41, %23) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %43:2 = "arith.mului_extended"(%42, %4) : (i32, i32) -> (i32, i32)
      %44 = "arith.muli"(%42, %4) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %45:2 = "arith.mului_extended"(%43#1, %3) : (i32, i32) -> (i32, i32)
      %46 = "arith.xori"(%45#1, %5) : (i32, i32) -> i32
      %47 = "arith.xori"(%44, %2) : (i32, i32) -> i32
      %48 = "arith.xori"(%47, %6) : (i32, i32) -> i32
      %49 = "arith.muli"(%43#1, %3) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %50:2 = "arith.mului_extended"(%48, %3) : (i32, i32) -> (i32, i32)
      %51 = "arith.xori"(%50#1, %49) : (i32, i32) -> i32
      %52 = "arith.xori"(%51, %7) : (i32, i32) -> i32
      %53:2 = "arith.mului_extended"(%46, %4) : (i32, i32) -> (i32, i32)
      %54 = "arith.xori"(%53#1, %1) : (i32, i32) -> i32
      %55 = "arith.xori"(%54, %8) : (i32, i32) -> i32
      %56 = "arith.muli"(%48, %3) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %57 = "arith.muli"(%46, %4) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %58:2 = "arith.mului_extended"(%55, %3) : (i32, i32) -> (i32, i32)
      %59 = "arith.xori"(%58#1, %56) : (i32, i32) -> i32
      %60 = "arith.xori"(%59, %9) : (i32, i32) -> i32
      %61:2 = "arith.mului_extended"(%52, %4) : (i32, i32) -> (i32, i32)
      %62 = "arith.xori"(%61#1, %57) : (i32, i32) -> i32
      %63 = "arith.xori"(%62, %10) : (i32, i32) -> i32
      %64 = "arith.muli"(%55, %3) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %65 = "arith.muli"(%52, %4) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %66:2 = "arith.mului_extended"(%63, %3) : (i32, i32) -> (i32, i32)
      %67 = "arith.xori"(%66#1, %64) : (i32, i32) -> i32
      %68 = "arith.xori"(%67, %11) : (i32, i32) -> i32
      %69:2 = "arith.mului_extended"(%60, %4) : (i32, i32) -> (i32, i32)
      %70 = "arith.xori"(%69#1, %65) : (i32, i32) -> i32
      %71 = "arith.xori"(%70, %12) : (i32, i32) -> i32
      %72 = "arith.muli"(%63, %3) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %73 = "arith.muli"(%60, %4) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %74:2 = "arith.mului_extended"(%71, %3) : (i32, i32) -> (i32, i32)
      %75 = "arith.xori"(%74#1, %72) : (i32, i32) -> i32
      %76 = "arith.xori"(%75, %13) : (i32, i32) -> i32
      %77:2 = "arith.mului_extended"(%68, %4) : (i32, i32) -> (i32, i32)
      %78 = "arith.xori"(%77#1, %73) : (i32, i32) -> i32
      %79 = "arith.xori"(%78, %14) : (i32, i32) -> i32
      %80 = "arith.muli"(%71, %3) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %81 = "arith.muli"(%68, %4) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %82:2 = "arith.mului_extended"(%79, %3) : (i32, i32) -> (i32, i32)
      %83 = "arith.xori"(%82#1, %80) : (i32, i32) -> i32
      %84 = "arith.xori"(%83, %15) : (i32, i32) -> i32
      %85:2 = "arith.mului_extended"(%76, %4) : (i32, i32) -> (i32, i32)
      %86 = "arith.xori"(%85#1, %81) : (i32, i32) -> i32
      %87 = "arith.xori"(%86, %16) : (i32, i32) -> i32
      %88 = "arith.muli"(%79, %3) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %89 = "arith.muli"(%76, %4) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %90:2 = "arith.mului_extended"(%87, %3) : (i32, i32) -> (i32, i32)
      %91 = "arith.xori"(%90#1, %88) : (i32, i32) -> i32
      %92 = "arith.xori"(%91, %17) : (i32, i32) -> i32
      %93:2 = "arith.mului_extended"(%84, %4) : (i32, i32) -> (i32, i32)
      %94 = "arith.xori"(%93#1, %89) : (i32, i32) -> i32
      %95 = "arith.xori"(%94, %18) : (i32, i32) -> i32
      %96 = "arith.muli"(%84, %4) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %97:2 = "arith.mului_extended"(%92, %4) : (i32, i32) -> (i32, i32)
      %98 = "arith.xori"(%97#1, %96) : (i32, i32) -> i32
      %99 = "arith.xori"(%98, %19) : (i32, i32) -> i32
      %100 = "arith.muli"(%95, %3) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %101:2 = "arith.mului_extended"(%99, %3) : (i32, i32) -> (i32, i32)
      %102 = "arith.xori"(%101#1, %100) : (i32, i32) -> i32
      %103 = "arith.xori"(%102, %20) : (i32, i32) -> i32
      %104 = "arith.cmpi"(%103, %22) <{predicate = 2 : i64}> : (i32, i32) -> i1
      %105 = "arith.subi"(%21, %103) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %106 = "arith.select"(%104, %105, %103) : (i1, i32, i32) -> i32
      %107 = "arith.sitofp"(%106) : (i32) -> f32
      %108 = "tensor.insert"(%107, %36, %25) : (f32, tensor<1xf32>, index) -> tensor<1xf32>
      %109 = "hivm.hir.vmul"(%108, %26, %34) <{broadcast = array<i64>, operandSegmentSizes = array<i32: 2, 1, 0>, transpose = array<i64>}> : (tensor<1xf32>, f32, tensor<1xf32>) -> tensor<1xf32>
      %110 = "tensor.extract"(%109, %25) : (tensor<1xf32>, index) -> f32
      %111 = "arith.index_cast"(%41) : (i32) -> index
      %112 = "tensor.insert"(%110, %35, %25) : (f32, tensor<1xf32>, index) -> tensor<1xf32>
      %113 = "memref.reinterpret_cast"(%arg3, %111) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<1xf32, strided<[1], offset: ?>>
      "hivm.hir.store"(%112, %113) : (tensor<1xf32>, memref<1xf32, strided<[1], offset: ?>>) -> ()
      "scf.yield"() : () -> ()
    }) : (i32, i32, i32) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, false, false, false]> : vector<7xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

