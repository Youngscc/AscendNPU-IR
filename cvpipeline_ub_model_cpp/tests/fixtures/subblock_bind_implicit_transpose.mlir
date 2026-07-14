// AIV function whose body carries an `annotation.mark` annotated with
// `MayImplicitTransposeWithLastAxis = false`.  The real pass's
// hasImplicitTransposeWithLastAxisInAiv consults `markOp.isAnnotatedBy(...)`,
// which reduces to `hasAttr(key)` (value-agnostic), so even an explicit
// `= false` spelling is a hazard that forces the pass to fall back to
// limitUniqueSubBlockToStore.  The model mirrors that recognized fallback
// exactly: it restricts the store to sub-block 0 with the
// `limit_sub_block_id0` mark and stays exact.
"builtin.module"() ({
  "func.func"() <{function_type = () -> (), sym_name = "subblock_implicit_transpose_aiv"}> ({
  ^bb0:
    %load_src = "memref.alloc"() : () -> memref<16x16xf16>
    %load_out = "tensor.empty"() : () -> tensor<16x16xf16>
    %loaded = "hivm.hir.load"(%load_src, %load_out) : (memref<16x16xf16>, tensor<16x16xf16>) -> tensor<16x16xf16>
    %sum_init = "tensor.empty"() : () -> tensor<16x16xf16>
    %sum = "hivm.hir.vadd"(%loaded, %loaded, %sum_init) : (tensor<16x16xf16>, tensor<16x16xf16>, tensor<16x16xf16>) -> tensor<16x16xf16>
    "annotation.mark"(%sum) {MayImplicitTransposeWithLastAxis = false} : (tensor<16x16xf16>) -> ()
    %store_dst = "memref.alloc"() : () -> memref<16x16xf16>
    "hivm.hir.store"(%sum, %store_dst) {case = "hazard_store"} : (tensor<16x16xf16>, memref<16x16xf16>) -> ()
    "func.return"() : () -> ()
  }) {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, hivm.part_of_mix, mix_mode = "aiv"} : () -> ()
}) : () -> ()
