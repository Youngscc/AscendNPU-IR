// Post-split AIV fixture exercising sub-byte element types in the OTF inline
// load/store rewrite (stage 14), i.e. the real UnalignedLastDimConcatStorePattern.
//
// The vconcat element type is i4 (4 bits).  The real pass computes
// elemSizeInBytes = getElementTypeBitWidth() / 8 = 4 / 8 = 0 (integer
// division).  With elemSizeInBytes == 0 every cumulative-extent alignment check
// (accumOffset * 0) % 32 == 0 is trivially true, so isLastDimConcatAligned
// returns true and the pattern treats the store as an aligned no-op (returns
// failure, IR unchanged).  The model mirrors that exactly (Exact) rather than
// spuriously routing the zero byte width to Incomplete.
"builtin.module"() ({
  "func.func"() <{function_type = () -> (), sym_name = "subbyte_concat_store_aiv"}> ({
  ^bb0:
    %a = "tensor.empty"() {case = "sb_a"} : () -> tensor<4x4xi4>
    %b = "tensor.empty"() {case = "sb_b"} : () -> tensor<4x4xi4>
    %dst = "tensor.empty"() {case = "sb_dst"} : () -> tensor<4x8xi4>
    %cat = "hivm.hir.vconcat"(%a, %b, %dst) <{dim = 1 : i64, operandSegmentSizes = array<i32: 2, 1>}> {case = "sb_concat"} : (tensor<4x4xi4>, tensor<4x4xi4>, tensor<4x8xi4>) -> tensor<4x8xi4>
    "annotation.mark"(%cat) {buffer_size_in_byte = 16 : i64, case = "sb_size_mark"} : (tensor<4x8xi4>) -> ()
    %gm = "memref.alloc"() : () -> memref<4x8xi4>
    "hivm.hir.store"(%cat, %gm) {case = "sb_store"} : (tensor<4x8xi4>, memref<4x8xi4>) -> ()
    "func.return"() : () -> ()
  }) {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, hivm.part_of_mix, mix_mode = "aiv"} : () -> ()
}) : () -> ()
