"builtin.module"() ({
  "func.func"() <{function_type = () -> (), sym_name = "cleanup"}> ({
  ^bb0:
    %base = "tensor.empty"() : () -> tensor<4xf32>
    %index = "arith.constant"() <{value = 0 : index}> : () -> index

    %removable_source = "hivm.hir.vabs"(%base, %base) <{case = "removable_source"}> : (tensor<4xf32>, tensor<4xf32>) -> tensor<4xf32>
    "annotation.mark"(%removable_source) : (tensor<4xf32>) -> ()
    %removable_user = "hivm.hir.vabs"(%removable_source, %base) : (tensor<4xf32>, tensor<4xf32>) -> tensor<4xf32>

    %only_mark_source = "tensor.empty"() <{case = "only_mark_source"}> : () -> tensor<4xf32>
    "annotation.mark"(%only_mark_source) : (tensor<4xf32>) -> ()

    %fixpipe_source = "hivm.hir.fixpipe"(%base, %base) <{case = "fixpipe_source"}> : (tensor<4xf32>, tensor<4xf32>) -> tensor<4xf32>
    "annotation.mark"(%fixpipe_source) : (tensor<4xf32>) -> ()
    %fixpipe_user = "hivm.hir.vabs"(%fixpipe_source, %base) : (tensor<4xf32>, tensor<4xf32>) -> tensor<4xf32>

    %store_source = "hivm.hir.store"(%base, %base) <{case = "store_source"}> : (tensor<4xf32>, tensor<4xf32>) -> tensor<4xf32>
    "annotation.mark"(%store_source) : (tensor<4xf32>) -> ()
    %store_user = "hivm.hir.vabs"(%store_source, %base) : (tensor<4xf32>, tensor<4xf32>) -> tensor<4xf32>

    %attributed_source = "hivm.hir.vabs"(%base, %base) <{case = "attributed_source"}> : (tensor<4xf32>, tensor<4xf32>) -> tensor<4xf32>
    "annotation.mark"(%attributed_source) {keep = true} : (tensor<4xf32>) -> ()
    %attributed_user = "hivm.hir.vabs"(%attributed_source, %base) : (tensor<4xf32>, tensor<4xf32>) -> tensor<4xf32>

    %original_extract = "tensor.extract"(%base, %index) <{case = "original_extract"}> : (tensor<4xf32>, index) -> f32
    %workspace = "tensor.empty"() : () -> tensor<4xf32>
    %topology_store = "hivm.hir.store"(%base, %workspace) : (tensor<4xf32>, tensor<4xf32>) -> tensor<4xf32>
    "annotation.mark"(%topology_store) {tcoretype = #hivm.tcore_type<VECTOR>} : (tensor<4xf32>) -> ()
    %new_extract = "tensor.extract"(%topology_store, %index) {"DuplicateTensorExtractForCube::newExtractLabel" = 1 : i32, case = "dead_new_extract"} : (tensor<4xf32>, index) -> f32
    "annotation.mark"(%original_extract, %new_extract) {"DuplicateTensorExtractForCube::replacementLabel" = 1 : i32} : (f32, f32) -> ()

    %unsafe_extract = "tensor.extract"(%base, %index) {"DuplicateTensorExtractForCube::newExtractLabel" = 1 : i32, case = "live_new_extract"} : (tensor<4xf32>, index) -> f32
    %unsupported_cube = "hivm.hir.fixpipe"(%unsafe_extract) : (f32) -> f32
    "func.return"() : () -> ()
  }) {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<MIX>, mix_mode = "mix"} : () -> ()
}) : () -> ()
