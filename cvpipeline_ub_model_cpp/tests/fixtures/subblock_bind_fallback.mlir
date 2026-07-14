// AIV function where the `hivm.hir.load` source and the `hivm.hir.store`
// destination both trace, through `memref.reinterpret_cast`, to the same
// function block argument.  The real TileAndBindSubBlock pass detects this
// same-address hazard (`areLoadAndStoreSameAddress`) and falls back to
// `limitUniqueSubBlockToStore`, leaving buffer sizes unchanged.  The model
// mirrors that recognized fallback exactly: it restricts every store/copy to
// sub-block 0 with the `limit_sub_block_id0` mark and stays exact.
"builtin.module"() ({
  "func.func"() <{function_type = (memref<?xf16>) -> (), sym_name = "subblock_bind_fallback_aiv"}> ({
  ^bb0(%arg0: memref<?xf16>):
    %c0 = "arith.constant"() {value = 0 : index} : () -> index
    %load_view = "memref.reinterpret_cast"(%arg0, %c0) {offset = array<i64: 0>, sizes = array<i64: 16, 16>, strides = array<i64: 16, 1>} : (memref<?xf16>, index) -> memref<16x16xf16, strided<[16, 1], offset: ?>>
    %load_out = "tensor.empty"() : () -> tensor<16x16xf16>
    %loaded = "hivm.hir.load"(%load_view, %load_out) {may_implicit_transpose_with_last_axis = false} : (memref<16x16xf16, strided<[16, 1], offset: ?>>, tensor<16x16xf16>) -> tensor<16x16xf16>
    %sum_init = "tensor.empty"() : () -> tensor<16x16xf16>
    %sum = "hivm.hir.vadd"(%loaded, %loaded, %sum_init) : (tensor<16x16xf16>, tensor<16x16xf16>, tensor<16x16xf16>) -> tensor<16x16xf16>
    %store_view = "memref.reinterpret_cast"(%arg0, %c0) {offset = array<i64: 0>, sizes = array<i64: 16, 16>, strides = array<i64: 16, 1>} : (memref<?xf16>, index) -> memref<16x16xf16, strided<[16, 1], offset: ?>>
    "hivm.hir.store"(%sum, %store_view) {case = "same_address_store"} : (tensor<16x16xf16>, memref<16x16xf16, strided<[16, 1], offset: ?>>) -> ()
    "func.return"() : () -> ()
  }) {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, hivm.part_of_mix, mix_mode = "aiv"} : () -> ()
}) : () -> ()
