"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {}, {tt.divisibility = 16 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xi32>, memref<?xi32>, memref<?xi32>, memref<?xf32>, memref<?xi32>, memref<?xi32>, memref<?xf32>, memref<?xi32>, i32, i32, i32, i32, i32) -> (), sym_name = "rejection_random_sample_kernel"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xi32>, %arg4: memref<?xi32>, %arg5: memref<?xi32>, %arg6: memref<?xf32>, %arg7: memref<?xi32>, %arg8: memref<?xi32>, %arg9: memref<?xf32>, %arg10: memref<?xi32>, %arg11: i32, %arg12: i32, %arg13: i32, %arg14: i32, %arg15: i32):
    %0 = "arith.constant"() <{value = 1 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = true}> : () -> i1
    %2 = "arith.constant"() <{value = -1 : index}> : () -> index
    %3 = "arith.constant"() <{value = 0 : index}> : () -> index
    %4 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %5 = "arith.constant"() <{value = false}> : () -> i1
    "hivm.hir.set_mask_norm"() : () -> ()
    %6 = "arith.muli"(%arg13, %arg14) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %7 = "arith.muli"(%6, %arg15) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%7) {logical_block_num} : (i32) -> ()
    %8 = "hivm.hir.get_block_idx"() : () -> i64
    %9 = "arith.trunci"(%8) : (i64) -> i32
    %10 = "arith.muli"(%arg15, %arg14) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %11 = "arith.divsi"(%9, %10) : (i32, i32) -> i32
    %12 = "arith.remsi"(%11, %arg13) : (i32, i32) -> i32
    %13 = "arith.index_cast"(%12) : (i32) -> index
    %14 = "memref.reinterpret_cast"(%arg10, %13) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<1xi32, strided<[1], offset: ?>>
    %15 = "memref.load"(%14, %3) : (memref<1xi32, strided<[1], offset: ?>>, index) -> i32
    %16 = "arith.cmpi"(%15, %4) <{predicate = 1 : i64}> : (i32, i32) -> i1
    "scf.if"(%16) ({
      "scf.yield"() : () -> ()
    }, {
      %17 = "arith.cmpi"(%12, %4) <{predicate = 0 : i64}> : (i32, i32) -> i1
      %18 = "scf.if"(%17) ({
        "scf.yield"(%4) : (i32) -> ()
      }, {
        %60 = "arith.addi"(%13, %2) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
        %61 = "memref.reinterpret_cast"(%arg4, %60) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<1xi32, strided<[1], offset: ?>>
        %62 = "memref.load"(%61, %3) : (memref<1xi32, strided<[1], offset: ?>>, index) -> i32
        "scf.yield"(%62) : (i32) -> ()
      }) : (i1) -> i32
      %19 = "memref.reinterpret_cast"(%arg4, %13) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<1xi32, strided<[1], offset: ?>>
      %20 = "memref.load"(%19, %3) : (memref<1xi32, strided<[1], offset: ?>>, index) -> i32
      %21 = "arith.subi"(%20, %18) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %22 = "scf.for"(%4, %21, %0, %5) ({
      ^bb0(%arg16: i32, %arg17: i1):
        %33 = "scf.if"(%arg17) ({
          "scf.yield"(%1) : (i1) -> ()
        }, {
          %34 = "arith.index_cast"(%18) : (i32) -> index
          %35 = "arith.index_cast"(%arg16) : (i32) -> index
          %36 = "arith.addi"(%34, %35) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
          %37 = "memref.reinterpret_cast"(%arg5, %36) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<1xi32, strided<[1], offset: ?>>
          %38 = "memref.load"(%37, %3) : (memref<1xi32, strided<[1], offset: ?>>, index) -> i32
          %39 = "arith.addi"(%18, %arg16) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
          %40 = "arith.muli"(%39, %arg12) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
          %41 = "arith.index_cast"(%40) : (i32) -> index
          %42 = "arith.index_cast"(%38) : (i32) -> index
          %43 = "arith.addi"(%41, %42) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
          %44 = "memref.reinterpret_cast"(%arg6, %43) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<1xf32, strided<[1], offset: ?>>
          %45 = "memref.load"(%44, %3) : (memref<1xf32, strided<[1], offset: ?>>, index) -> f32
          %46 = "memref.reinterpret_cast"(%arg9, %36) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<1xf32, strided<[1], offset: ?>>
          %47 = "memref.load"(%46, %3) : (memref<1xf32, strided<[1], offset: ?>>, index) -> f32
          %48 = "arith.cmpf"(%45, %47) <{fastmath = #arith.fastmath<none>, predicate = 3 : i64}> : (f32, f32) -> i1
          %49 = "arith.xori"(%48, %1) : (i1, i1) -> i1
          %50 = "scf.if"(%48) ({
            "scf.yield"(%38) : (i32) -> ()
          }, {
            %58 = "memref.reinterpret_cast"(%arg8, %36) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<1xi32, strided<[1], offset: ?>>
            %59 = "memref.load"(%58, %3) : (memref<1xi32, strided<[1], offset: ?>>, index) -> i32
            "scf.yield"(%59) : (i32) -> ()
          }) : (i1) -> i32
          %51 = "arith.addi"(%arg11, %0) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
          %52 = "arith.muli"(%12, %51) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
          %53 = "arith.index_cast"(%52) : (i32) -> index
          %54 = "arith.addi"(%53, %35) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
          %55 = "tensor.empty"() : () -> tensor<1xi32>
          %56 = "tensor.insert"(%50, %55, %3) : (i32, tensor<1xi32>, index) -> tensor<1xi32>
          %57 = "memref.reinterpret_cast"(%arg3, %54) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<1xi32, strided<[1], offset: ?>>
          "hivm.hir.store"(%56, %57) : (tensor<1xi32>, memref<1xi32, strided<[1], offset: ?>>) -> ()
          "scf.yield"(%49) : (i1) -> ()
        }) : (i1) -> i1
        "scf.yield"(%33) : (i1) -> ()
      }) : (i32, i32, i32, i1) -> i1
      "scf.if"(%22) ({
        "scf.yield"() : () -> ()
      }, {
        %23 = "memref.reinterpret_cast"(%arg7, %13) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<1xi32, strided<[1], offset: ?>>
        %24 = "memref.load"(%23, %3) : (memref<1xi32, strided<[1], offset: ?>>, index) -> i32
        %25 = "arith.addi"(%arg11, %0) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
        %26 = "arith.muli"(%12, %25) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
        %27 = "arith.index_cast"(%26) : (i32) -> index
        %28 = "arith.index_cast"(%21) : (i32) -> index
        %29 = "arith.addi"(%27, %28) <{overflowFlags = #arith.overflow<none>}> : (index, index) -> index
        %30 = "tensor.empty"() : () -> tensor<1xi32>
        %31 = "tensor.insert"(%24, %30, %3) : (i32, tensor<1xi32>, index) -> tensor<1xi32>
        %32 = "memref.reinterpret_cast"(%arg3, %29) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<1xi32, strided<[1], offset: ?>>
        "hivm.hir.store"(%31, %32) : (tensor<1xi32>, memref<1xi32, strided<[1], offset: ?>>) -> ()
        "scf.yield"() : () -> ()
      }) : (i1) -> ()
      "scf.yield"() : () -> ()
    }) : (i1) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false]> : vector<16xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

