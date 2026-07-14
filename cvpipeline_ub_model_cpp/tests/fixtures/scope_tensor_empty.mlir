"builtin.module"() ({
  "func.func"() <{function_type = () -> (), sym_name = "scope_tensor_empty"}> ({
  ^bb0:
    %c0 = "arith.constant"() {value = 0 : index} : () -> index
    %c1 = "arith.constant"() {value = 1 : index} : () -> index
    %c2 = "arith.constant"() {value = 2 : index} : () -> index
    %scalar = "arith.constant"() {value = 1.000000e+00 : f32} : () -> f32
    %s:2 = "scope.scope"() ({
      %s0 = "tensor.empty"() {case = "scope_first"} : () -> tensor<64xf32>
      %s1 = "tensor.empty"() {case = "scope_second"} : () -> tensor<64xf32>
      "scope.return"(%s0, %s1) : (tensor<64xf32>, tensor<64xf32>) -> ()
    }) {case = "inline_scope"} : () -> (tensor<64xf32>, tensor<64xf32>)
    %scope_user = "hivm.hir.vadd"(%s#0, %s#1, %s#0) <{operandSegmentSizes = array<i32: 2, 1>}> {case = "scope_user"} : (tensor<64xf32>, tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
    %kept = "scope.scope"() ({
      %kept_empty = "tensor.empty"() {case = "kept_empty"} : () -> tensor<64xf32>
      "scope.return"(%kept_empty) : (tensor<64xf32>) -> ()
    }) {case = "kept_scope", no_inline} : () -> tensor<64xf32>
    %fold_src = "tensor.empty"() {case = "fold_source"} : () -> tensor<64xf32>
    %fold_dst = "tensor.empty"() {case = "fold_destination"} : () -> tensor<64xf32>
    %folded = "tensor.insert_slice"(%fold_src, %fold_dst) <{static_offsets = [0], static_sizes = [64], static_strides = [1]}> {case = "fold_insert_slice"} : (tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
    %fold_user = "hivm.hir.vabs"(%folded, %fold_dst) <{operandSegmentSizes = array<i32: 1, 1>}> {case = "fold_user"} : (tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
    %shared = "tensor.empty"() {case = "shared_destination"} : () -> tensor<64xf32>
    "annotation.mark"(%shared) {buffer_size_in_byte = 256 : i64, case = "size_mark"} : (tensor<64xf32>) -> ()
    "annotation.mark"(%shared) {case = "other_mark", logical_block_num} : (tensor<64xf32>) -> ()
    %v0 = "hivm.hir.vadd"(%scope_user, %scope_user, %shared) <{operandSegmentSizes = array<i32: 2, 1>}> {case = "owner_zero"} : (tensor<64xf32>, tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
    %v1 = "hivm.hir.vadd"(%scope_user, %scope_user, %shared) <{operandSegmentSizes = array<i32: 2, 1>}> {case = "owner_one"} : (tensor<64xf32>, tensor<64xf32>, tensor<64xf32>) -> tensor<64xf32>
    %for_result = "scf.for"(%c0, %c2, %c1, %shared) ({
    ^bb0(%iv: index, %iter: tensor<64xf32>):
      "scf.yield"(%iter) : (tensor<64xf32>) -> ()
    }) {case = "for_owner"} : (index, index, index, tensor<64xf32>) -> tensor<64xf32>
    %while_result = "scf.while"(%shared) ({
    ^bb0(%before: tensor<64xf32>):
      %true = "arith.constant"() {value = true} : () -> i1
      "scf.condition"(%true, %before) : (i1, tensor<64xf32>) -> ()
    }, {
    ^bb0(%after: tensor<64xf32>):
      "scf.yield"(%after) : (tensor<64xf32>) -> ()
    }) {case = "while_owner"} : (tensor<64xf32>) -> tensor<64xf32>
    %inserted = "tensor.insert"(%scalar, %shared, %c0) {case = "insert_owner"} : (f32, tensor<64xf32>, index) -> tensor<64xf32>
    "func.return"() : () -> ()
  }) {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, mix_mode = "aiv"} : () -> ()
}) : () -> ()
