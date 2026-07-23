"builtin.module"() ({
  "func.func"() <{function_type = (i32, i32, i32) -> (), sym_name = "grid_kernel"}> ({
  ^bb0(%arg0: i32, %arg1: i32, %arg2: i32):
    %c0 = "arith.constant"() <{value = 0.000000e+00 : f32}> : () -> f32
    %b = "memref.alloc"() : () -> memref<2048xf32, #hivm.address_space<UB>>
    "hivm.hir.set_mask_norm"() : () -> ()
    %n0 = "arith.muli"(%arg0, %arg1) : (i32, i32) -> i32
    %logical = "arith.muli"(%n0, %arg2) : (i32, i32) -> i32
    "annotation.mark"(%logical) {logical_block_num} : (i32) -> ()
    %bid = "hivm.hir.get_block_idx"() : () -> i64
    %t = "arith.trunci"(%bid) : (i64) -> i32
    %off = "arith.index_cast"(%t) : (i32) -> index
    "memref.store"(%c0, %b, %off) : (f32, memref<2048xf32, #hivm.address_space<UB>>, index) -> ()
    "func.return"() : () -> ()
  }) {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv"} : () -> ()
  "func.func"() <{function_type = () -> (), sym_name = "plain_kernel"}> ({
  ^bb0:
    %c0 = "arith.constant"() <{value = 0.000000e+00 : f32}> : () -> f32
    %idx = "arith.constant"() <{value = 0 : index}> : () -> index
    %logical = "arith.constant"() <{value = 1 : i32}> : () -> i32
    "annotation.mark"(%logical) {logical_block_num} : (i32) -> ()
    %b = "memref.alloc"() : () -> memref<1024xf32, #hivm.address_space<UB>>
    "memref.store"(%c0, %b, %idx) : (f32, memref<1024xf32, #hivm.address_space<UB>>, index) -> ()
    "func.return"() : () -> ()
  }) {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv"} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>, #dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>>>, hivm.module_core_type = #hivm.module_core_type<AIV>} : () -> ()
