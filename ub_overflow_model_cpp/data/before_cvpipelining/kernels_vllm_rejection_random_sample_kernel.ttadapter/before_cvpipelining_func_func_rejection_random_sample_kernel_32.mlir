#map = affine_map<()[s0] -> (s0 - 1)>
#map1 = affine_map<()[s0, s1] -> (s0 + s1)>
"builtin.module"() ({
  "func.func"() <{arg_attrs = [{hacc.arg_type = #hacc.arg_type<ffts_base_address>}, {hacc.arg_type = #hacc.arg_type<sync_block_lock>}, {hacc.arg_type = #hacc.arg_type<workspace>}, {tt.divisibility = 16 : i32, tt.tensor_kind = 1 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {tt.divisibility = 16 : i32, tt.tensor_kind = 0 : i32}, {}, {tt.divisibility = 16 : i32}, {}, {}, {}], function_type = (i64, memref<?xi8>, memref<?xi8>, memref<?xi32>, memref<?xi32>, memref<?xi32>, memref<?xf32>, memref<?xi32>, memref<?xi32>, memref<?xf32>, memref<?xi32>, i32, i32, i32, i32, i32) -> (), sym_name = "rejection_random_sample_kernel"}> ({
  ^bb0(%arg0: i64, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xi32>, %arg4: memref<?xi32>, %arg5: memref<?xi32>, %arg6: memref<?xf32>, %arg7: memref<?xi32>, %arg8: memref<?xi32>, %arg9: memref<?xf32>, %arg10: memref<?xi32>, %arg11: i32, %arg12: i32, %arg13: i32, %arg14: i32, %arg15: i32):
    %0 = "arith.constant"() <{value = 1 : i32}> : () -> i32
    %1 = "arith.constant"() <{value = true}> : () -> i1
    %2 = "arith.constant"() <{value = 0 : index}> : () -> index
    %3 = "arith.constant"() <{value = 0 : i32}> : () -> i32
    %4 = "arith.constant"() <{value = false}> : () -> i1
    "hivm.hir.set_mask_norm"() : () -> ()
    %5 = "arith.muli"(%arg13, %arg14) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %6 = "arith.muli"(%5, %arg15) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    "annotation.mark"(%6) <{effects = ["write"]}> {logical_block_num} : (i32) -> ()
    %7 = "hivm.hir.get_block_idx"() : () -> i64
    %8 = "arith.trunci"(%7) : (i64) -> i32
    %9 = "arith.muli"(%arg15, %arg14) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
    %10 = "arith.divsi"(%8, %9) : (i32, i32) -> i32
    %11 = "arith.remsi"(%10, %arg13) : (i32, i32) -> i32
    %12 = "arith.index_cast"(%11) : (i32) -> index
    %13 = "memref.reinterpret_cast"(%arg10, %12) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<1xi32, strided<[1], offset: ?>>
    %14 = "memref.load"(%13, %2) : (memref<1xi32, strided<[1], offset: ?>>, index) -> i32
    %15 = "arith.cmpi"(%14, %3) <{predicate = 1 : i64}> : (i32, i32) -> i1
    "scf.if"(%15) ({
      "scf.yield"() : () -> ()
    }, {
      %16 = "arith.cmpi"(%11, %3) <{predicate = 0 : i64}> : (i32, i32) -> i1
      %17 = "scf.if"(%16) ({
        "scf.yield"(%3) : (i32) -> ()
      }, {
        %59 = "affine.apply"(%12) <{map = #map}> : (index) -> index
        %60 = "memref.reinterpret_cast"(%arg4, %59) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<1xi32, strided<[1], offset: ?>>
        %61 = "memref.load"(%60, %2) : (memref<1xi32, strided<[1], offset: ?>>, index) -> i32
        "scf.yield"(%61) : (i32) -> ()
      }) : (i1) -> i32
      %18 = "memref.reinterpret_cast"(%arg4, %12) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<1xi32, strided<[1], offset: ?>>
      %19 = "memref.load"(%18, %2) : (memref<1xi32, strided<[1], offset: ?>>, index) -> i32
      %20 = "arith.subi"(%19, %17) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
      %21 = "scf.for"(%3, %20, %0, %4) ({
      ^bb0(%arg16: i32, %arg17: i1):
        %32 = "scf.if"(%arg17) ({
          "scf.yield"(%1) : (i1) -> ()
        }, {
          %33 = "arith.index_cast"(%17) : (i32) -> index
          %34 = "arith.index_cast"(%arg16) : (i32) -> index
          %35 = "affine.apply"(%33, %34) <{map = #map1}> : (index, index) -> index
          %36 = "memref.reinterpret_cast"(%arg5, %35) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<1xi32, strided<[1], offset: ?>>
          %37 = "memref.load"(%36, %2) : (memref<1xi32, strided<[1], offset: ?>>, index) -> i32
          %38 = "arith.addi"(%17, %arg16) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
          %39 = "arith.muli"(%38, %arg12) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
          %40 = "arith.index_cast"(%39) : (i32) -> index
          %41 = "arith.index_cast"(%37) : (i32) -> index
          %42 = "affine.apply"(%40, %41) <{map = #map1}> : (index, index) -> index
          %43 = "memref.reinterpret_cast"(%arg6, %42) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<1xf32, strided<[1], offset: ?>>
          %44 = "memref.load"(%43, %2) : (memref<1xf32, strided<[1], offset: ?>>, index) -> f32
          %45 = "memref.reinterpret_cast"(%arg9, %35) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xf32>, index) -> memref<1xf32, strided<[1], offset: ?>>
          %46 = "memref.load"(%45, %2) : (memref<1xf32, strided<[1], offset: ?>>, index) -> f32
          %47 = "arith.cmpf"(%44, %46) <{fastmath = #arith.fastmath<none>, predicate = 3 : i64}> : (f32, f32) -> i1
          %48 = "arith.xori"(%47, %1) : (i1, i1) -> i1
          %49 = "scf.if"(%47) ({
            "scf.yield"(%37) : (i32) -> ()
          }, {
            %57 = "memref.reinterpret_cast"(%arg8, %35) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<1xi32, strided<[1], offset: ?>>
            %58 = "memref.load"(%57, %2) : (memref<1xi32, strided<[1], offset: ?>>, index) -> i32
            "scf.yield"(%58) : (i32) -> ()
          }) : (i1) -> i32
          %50 = "arith.addi"(%arg11, %0) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
          %51 = "arith.muli"(%11, %50) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
          %52 = "arith.index_cast"(%51) : (i32) -> index
          %53 = "affine.apply"(%52, %34) <{map = #map1}> : (index, index) -> index
          %54 = "tensor.empty"() : () -> tensor<1xi32>
          %55 = "tensor.insert"(%49, %54, %2) : (i32, tensor<1xi32>, index) -> tensor<1xi32>
          %56 = "memref.reinterpret_cast"(%arg3, %53) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<1xi32, strided<[1], offset: ?>>
          "hivm.hir.store"(%55, %56) : (tensor<1xi32>, memref<1xi32, strided<[1], offset: ?>>) -> ()
          "scf.yield"(%48) : (i1) -> ()
        }) : (i1) -> i1
        "scf.yield"(%32) : (i1) -> ()
      }) : (i32, i32, i32, i1) -> i1
      "scf.if"(%21) ({
        "scf.yield"() : () -> ()
      }, {
        %22 = "memref.reinterpret_cast"(%arg7, %12) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<1xi32, strided<[1], offset: ?>>
        %23 = "memref.load"(%22, %2) : (memref<1xi32, strided<[1], offset: ?>>, index) -> i32
        %24 = "arith.addi"(%arg11, %0) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
        %25 = "arith.muli"(%11, %24) <{overflowFlags = #arith.overflow<none>}> : (i32, i32) -> i32
        %26 = "arith.index_cast"(%25) : (i32) -> index
        %27 = "arith.index_cast"(%20) : (i32) -> index
        %28 = "affine.apply"(%26, %27) <{map = #map1}> : (index, index) -> index
        %29 = "tensor.empty"() : () -> tensor<1xi32>
        %30 = "tensor.insert"(%23, %29, %2) : (i32, tensor<1xi32>, index) -> tensor<1xi32>
        %31 = "memref.reinterpret_cast"(%arg3, %28) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808>, static_sizes = array<i64: 1>, static_strides = array<i64: 1>}> : (memref<?xi32>, index) -> memref<1xi32, strided<[1], offset: ?>>
        "hivm.hir.store"(%30, %31) : (tensor<1xi32>, memref<1xi32, strided<[1], offset: ?>>) -> ()
        "scf.yield"() : () -> ()
      }) : (i1) -> ()
      "scf.yield"() : () -> ()
    }) : (i1) -> ()
    "func.return"() : () -> ()
  }) {SyncBlockLockArgIdx = 0 : i64, WorkspaceArgIdx = 1 : i64, func_dyn_memref_args = dense<[false, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false]> : vector<16xi1>, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv", parallel_mode = "simd"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L1_SIZE", 4194304 : i32>, #dlti.dl_entry<"L0A_SIZE", 524288 : i32>, #dlti.dl_entry<"L0B_SIZE", 524288 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L1_ALIGN_SIZE", 256 : i32>, #dlti.dl_entry<"L0C_ALIGN_SIZE", 4096 : i32>>>, hacc.hivmc_compatible_print = false, hacc.hivmc_version = #hacc.hivmc_version<"0.0.0">, hacc.target = #hacc.target<"Ascend910B1">, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()

