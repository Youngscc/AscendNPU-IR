module attributes {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>>>, hivm.module_core_type = #hivm.module_core_type<AIV>} {
  func.func @factor_two(%logical: i32) attributes {auto_blockify_size = 2 : i64, hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>} {
    annotation.mark %logical {logical_block_num} : i32
    %block = hivm.hir.get_block_idx -> i64
    %used = arith.trunci %block : i64 to i32
    annotation.mark %used {keep} : i32
    return
  }
}
