module attributes {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"AI_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"CUBE_CORE_COUNT", 24 : i32>, #dlti.dl_entry<"VECTOR_CORE_COUNT", 48 : i32>, #dlti.dl_entry<"UB_SIZE", 1572864 : i32>>>, hivm.module_core_type = #hivm.module_core_type<AIC>} {
  func.func @aic_entry(%logical: i32) attributes {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIC>} {
    annotation.mark %logical {logical_block_num} : i32
    %logical_twice = arith.addi %logical, %logical : i32
    annotation.mark %logical_twice {logical_block_num} : i32
    %block = hivm.hir.get_block_idx -> i64
    %used = arith.trunci %block : i64 to i32
    annotation.mark %used {keep} : i32
    return
  }
  func.func @logical_mark_without_block_idx(%logical: i32) attributes {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>} {
    annotation.mark %logical {logical_block_num} : i32
    %value = arith.addi %logical, %logical : i32
    annotation.mark %value {keep} : i32
    return
  }
  func.func @device_but_not_entry() attributes {hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>} {
    return
  }
  func.func @host_entry() attributes {hacc.entry, hacc.function_kind = #hacc.function_kind<HOST>, hivm.func_core_type = #hivm.func_core_type<AIV>} {
    return
  }
}
