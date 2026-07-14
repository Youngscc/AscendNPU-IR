"builtin.module"() ({
  "func.func"() <{function_type = () -> (), sym_name = "cube_no_branch"}> ({
  ^bb0:
    %c0 = "arith.constant"() {value = 0 : index} : () -> index
    %c1 = "arith.constant"() {value = 1 : index} : () -> index
    "scf.for"(%c0, %c1, %c1) ({
    ^bb1(%iv: index):
      "scf.yield"() : () -> ()
    }) {hivm.loop_core_type = #hivm.tcore_type<CUBE>} : (index, index, index) -> ()
    "func.return"() : () -> ()
  }) {hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<MIX>} : () -> ()
}) {dlti.target_system_spec = #dlti.target_system_spec<"NPU" : #hacc.target_device_spec<#dlti.dl_entry<"L0C_SIZE", 1048576 : i32>, #dlti.dl_entry<"UB_ALIGN_SIZE", 256 : i32>>>} : () -> ()
