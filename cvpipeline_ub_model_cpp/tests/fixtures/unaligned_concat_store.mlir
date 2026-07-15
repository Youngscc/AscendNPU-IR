// Post-split AIV fixture exercising the OTF inline load/store rewrite (stage 14),
// i.e. the real UnalignedLastDimConcatStorePattern.  Two AIV functions exercise
// both outcomes of the pattern:
//
//   * unaligned_concat_store_aiv: a hivm.hir.store whose source is a
//     hivm.hir.vconcat along the last dimension (dim = 1, rank 2).  Inputs are
//     tensor<4x4xf16>; the cumulative last-dim extents are 4 and 8 elements,
//     i.e. 8 and 16 bytes for f16 (2 bytes/element).  Neither is divisible by
//     the real pass's 32-byte block (utils::INTR_BYTES_PER_BLOCK), so the
//     pattern builds an insert-slice accumulator seeded from the vconcat
//     destination, repoints the store source at it, erases the buffer_size_in_byte
//     annotation on the (now dead) vconcat, and the vconcat itself is dropped by
//     dead-op cleanup.
//
//   * aligned_concat_store_aiv: same shape with tensor<4x16xf16> inputs.  The
//     cumulative last-dim extents are 16 and 32 elements -> 32 and 64 bytes,
//     both divisible by 32, so the pattern leaves the IR unchanged (no-op).
//
// Element byte width is taken from the element type bit width / 8 (f16 -> 2).
"builtin.module"() ({
  "func.func"() <{function_type = () -> (), sym_name = "unaligned_concat_store_aiv"}> ({
  ^bb0:
    %a = "tensor.empty"() {case = "unaligned_a"} : () -> tensor<4x4xf16>
    %b = "tensor.empty"() {case = "unaligned_b"} : () -> tensor<4x4xf16>
    %dst = "tensor.empty"() {case = "unaligned_dst"} : () -> tensor<4x8xf16>
    %cat = "hivm.hir.vconcat"(%a, %b, %dst) <{dim = 1 : i64, operandSegmentSizes = array<i32: 2, 1>}> {case = "unaligned_concat"} : (tensor<4x4xf16>, tensor<4x4xf16>, tensor<4x8xf16>) -> tensor<4x8xf16>
    "annotation.mark"(%cat) {buffer_size_in_byte = 64 : i64, case = "unaligned_size_mark"} : (tensor<4x8xf16>) -> ()
    %gm = "memref.alloc"() : () -> memref<4x8xf16>
    "hivm.hir.store"(%cat, %gm) {case = "unaligned_store"} : (tensor<4x8xf16>, memref<4x8xf16>) -> ()
    "func.return"() : () -> ()
  }) {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, hivm.part_of_mix, mix_mode = "aiv"} : () -> ()
  "func.func"() <{function_type = () -> (), sym_name = "aligned_concat_store_aiv"}> ({
  ^bb0:
    %a = "tensor.empty"() {case = "aligned_a"} : () -> tensor<4x16xf16>
    %b = "tensor.empty"() {case = "aligned_b"} : () -> tensor<4x16xf16>
    %dst = "tensor.empty"() {case = "aligned_dst"} : () -> tensor<4x32xf16>
    %cat = "hivm.hir.vconcat"(%a, %b, %dst) <{dim = 1 : i64, operandSegmentSizes = array<i32: 2, 1>}> {case = "aligned_concat"} : (tensor<4x16xf16>, tensor<4x16xf16>, tensor<4x32xf16>) -> tensor<4x32xf16>
    "annotation.mark"(%cat) {buffer_size_in_byte = 256 : i64, case = "aligned_size_mark"} : (tensor<4x32xf16>) -> ()
    %gm = "memref.alloc"() : () -> memref<4x32xf16>
    "hivm.hir.store"(%cat, %gm) {case = "aligned_store"} : (tensor<4x32xf16>, memref<4x32xf16>) -> ()
    "func.return"() : () -> ()
  }) {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, hivm.part_of_mix, mix_mode = "aiv"} : () -> ()
}) : () -> ()
