"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xi32>, i32, i32, i32) -> (), sym_name = "kernel_randint"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xi32>, %arg4: i32, %arg5: i32, %arg6: i32):
    %0 = "arith.constant"() <{value = 1 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = 462789791 : i32}> : () -> i32
    %2 = "arith.constant"() <{value = 4 : i32}> : () -> i32
    %3 = "arith.constant"() <{value = 0 : index}> : () -> index
    %4 = "arith.constant"() <{value = 3 : i32}> : () -> i32
    %5 = "arith.constant"() <{value = 10 : i32}> : () -> i32
    %6 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %7 = "arith.constant"() <{value = -1879881850 : i32}> : () -> i32
    %8 = "arith.constant"() <{value = -616729560 : i32}> : () -> i32
    %9 = "arith.constant"() <{value = 534103459 : i32}> : () -> i32
    %10 = "arith.constant"() <{value = 1401181204 : i32}> : () -> i32
    %11 = "arith.constant"() <{value = 1684936478 : i32}> : () -> i32
    %12 = "arith.constant"() <{value = -1253254565 : i32}> : () -> i32
    %13 = "arith.constant"() <{value = -1459197799 : i32}> : () -> i32
    %14 = "arith.constant"() <{value = 387276962 : i32}> : () -> i32
    %15 = "arith.constant"() <{value = -308364780 : i32}> : () -> i32
    %16 = "arith.constant"() <{value = 2027808489 : i32}> : () -> i32
    %17 = "arith.constant"() <{value = 842468239 : i32}> : () -> i32
    %18 = "arith.constant"() <{value = -626627280 : i32}> : () -> i32
    %19 = "arith.constant"() <{value = 1993301258 : i32}> : () -> i32
    %20 = "arith.constant"() <{value = 1013904247 : i32}> : () -> i32
    %21 = "arith.constant"() <{value = -1150833019 : i32}> : () -> i32
    %22 = "arith.constant"() <{value = -1640531522 : i32}> : () -> i32
    %23 = "arith.constant"() <{value = -766435501 : i32}> : () -> i32
    %24 = "arith.constant"() <{value = -845247145 : i32}> : () -> i32
    "hivm.hir.set_mask_norm"() : () -> ()
    %25 = "arith.muli"(%arg4, %arg5) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %26 = "arith.muli"(%25, %arg6) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%26) {logical_block_num} : (i32) -> ()
    %27 = "hivm.hir.get_block_idx"() : () -> i64
    %28 = "arith.trunci"(%27) : (i64) -> i32
    %29 = "arith.muli"(%arg6, %arg5) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %30 = "arith.divsi"(%28, %29) : (i32, i32) -> i32
    %31 = "arith.remsi"(%30, %arg4) : (i32, i32) -> i32
    %32 = "arith.muli"(%31, %4) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %33 = "arith.addi"(%32, %4) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %34 = "arith.cmpi"(%33, %4) <{predicate = 3 : i64}> : (i32, i32) -> i1
    %35 = "scf.if"(%34) ({
      "scf.yield"(%4) : (i32) -> ()
    }, {
      %103 = "arith.subi"(%4, %32) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      "scf.yield"(%103) : (i32) -> ()
    }) : (i1) -> i32
    "scf.for"(%6, %35, %0) ({
    ^bb0(%arg7: i32):
      %36 = "arith.addi"(%32, %arg7) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %37 = "arith.addi"(%36, %5) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %38:2 = "arith.mului_extended"(%37, %23) : (i32, i32) -> (i32, i32)
      %39 = "arith.muli"(%37, %23) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %40:2 = "arith.mului_extended"(%38#1, %24) : (i32, i32) -> (i32, i32)
      %41 = "arith.xori"(%40#1, %22) : (i32, i32) -> i32
      %42 = "arith.xori"(%39, %2) : (i32, i32) -> i32
      %43 = "arith.xori"(%42, %21) : (i32, i32) -> i32
      %44 = "arith.muli"(%38#1, %24) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %45:2 = "arith.mului_extended"(%43, %24) : (i32, i32) -> (i32, i32)
      %46 = "arith.xori"(%45#1, %44) : (i32, i32) -> i32
      %47 = "arith.xori"(%46, %20) : (i32, i32) -> i32
      %48:2 = "arith.mului_extended"(%41, %23) : (i32, i32) -> (i32, i32)
      %49 = "arith.xori"(%48#1, %1) : (i32, i32) -> i32
      %50 = "arith.xori"(%49, %19) : (i32, i32) -> i32
      %51 = "arith.muli"(%43, %24) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %52 = "arith.muli"(%41, %23) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %53:2 = "arith.mului_extended"(%50, %24) : (i32, i32) -> (i32, i32)
      %54 = "arith.xori"(%53#1, %51) : (i32, i32) -> i32
      %55 = "arith.xori"(%54, %18) : (i32, i32) -> i32
      %56:2 = "arith.mului_extended"(%47, %23) : (i32, i32) -> (i32, i32)
      %57 = "arith.xori"(%56#1, %52) : (i32, i32) -> i32
      %58 = "arith.xori"(%57, %17) : (i32, i32) -> i32
      %59 = "arith.muli"(%50, %24) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %60 = "arith.muli"(%47, %23) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %61:2 = "arith.mului_extended"(%58, %24) : (i32, i32) -> (i32, i32)
      %62 = "arith.xori"(%61#1, %59) : (i32, i32) -> i32
      %63 = "arith.xori"(%62, %16) : (i32, i32) -> i32
      %64:2 = "arith.mului_extended"(%55, %23) : (i32, i32) -> (i32, i32)
      %65 = "arith.xori"(%64#1, %60) : (i32, i32) -> i32
      %66 = "arith.xori"(%65, %15) : (i32, i32) -> i32
      %67 = "arith.muli"(%58, %24) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %68 = "arith.muli"(%55, %23) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %69:2 = "arith.mului_extended"(%66, %24) : (i32, i32) -> (i32, i32)
      %70 = "arith.xori"(%69#1, %67) : (i32, i32) -> i32
      %71 = "arith.xori"(%70, %14) : (i32, i32) -> i32
      %72:2 = "arith.mului_extended"(%63, %23) : (i32, i32) -> (i32, i32)
      %73 = "arith.xori"(%72#1, %68) : (i32, i32) -> i32
      %74 = "arith.xori"(%73, %13) : (i32, i32) -> i32
      %75 = "arith.muli"(%66, %24) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %76 = "arith.muli"(%63, %23) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %77:2 = "arith.mului_extended"(%74, %24) : (i32, i32) -> (i32, i32)
      %78 = "arith.xori"(%77#1, %75) : (i32, i32) -> i32
      %79 = "arith.xori"(%78, %12) : (i32, i32) -> i32
      %80:2 = "arith.mului_extended"(%71, %23) : (i32, i32) -> (i32, i32)
      %81 = "arith.xori"(%80#1, %76) : (i32, i32) -> i32
      %82 = "arith.xori"(%81, %11) : (i32, i32) -> i32
      %83 = "arith.muli"(%74, %24) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %84 = "arith.muli"(%71, %23) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %85:2 = "arith.mului_extended"(%82, %24) : (i32, i32) -> (i32, i32)
      %86 = "arith.xori"(%85#1, %83) : (i32, i32) -> i32
      %87 = "arith.xori"(%86, %10) : (i32, i32) -> i32
      %88:2 = "arith.mului_extended"(%79, %23) : (i32, i32) -> (i32, i32)
      %89 = "arith.xori"(%88#1, %84) : (i32, i32) -> i32
      %90 = "arith.xori"(%89, %9) : (i32, i32) -> i32
      %91 = "arith.muli"(%79, %23) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %92:2 = "arith.mului_extended"(%87, %23) : (i32, i32) -> (i32, i32)
      %93 = "arith.xori"(%92#1, %91) : (i32, i32) -> i32
      %94 = "arith.xori"(%93, %8) : (i32, i32) -> i32
      %95 = "arith.muli"(%90, %24) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %96:2 = "arith.mului_extended"(%94, %24) : (i32, i32) -> (i32, i32)
      %97 = "arith.xori"(%96#1, %95) : (i32, i32) -> i32
      %98 = "arith.xori"(%97, %7) : (i32, i32) -> i32
      %99 = "arith.index_cast"(%36) : (i32) -> index
      %100 = "tensor.empty"() : () -> tensor<1xi32>
      %101 = "tensor.insert"(%98, %100, %3) : (i32, tensor<1xi32>, index) -> tensor<1xi32>
      %102 = "memref.reinterpret_cast"(%arg3, %99) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<1xi32, strided<[1], offset: ?>>
      "hivm.hir.store"(%101, %102) : (tensor<1xi32>, memref<1xi32, strided<[1], offset: ?>>) -> ()
      "scf.yield"() : () -> ()
    }) : (i32, i32, i32) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, false, false, false]> : vector<7xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

