// AIV function whose `hivm.hir.store` source is a `tensor.extract_slice` of a
// static tensor that yields a dynamic-shaped result.  The real
// TileAndBindSubBlock pass traces the store source through the extract_slice to
// the underlying static type (TileAndBindSubBlock.cpp:859-877) and proceeds to
// tile it rather than aborting on the dynamic-looking direct operand type.
// Because faithfully reproducing that trace-and-tile is out of scope, the model
// fails closed: it does NOT claim the recognized dynamic-store Exact rollback
// for a slice-wrapped source, and instead reports the function as incomplete.
"builtin.module"() ({
  "func.func"() <{function_type = () -> (), sym_name = "subblock_slice_wrapped_aiv"}> ({
  ^bb0:
    %full = "tensor.empty"() : () -> tensor<16x16xf16>
    %slice = "tensor.extract_slice"(%full) {static_offsets = [0, 0], static_sizes = [16, 16], static_strides = [1, 1]} : (tensor<16x16xf16>) -> tensor<?x16xf16>
    %store_dst = "memref.alloc"() : () -> memref<16x16xf16>
    "hivm.hir.store"(%slice, %store_dst) {case = "slice_wrapped_store"} : (tensor<?x16xf16>, memref<16x16xf16>) -> ()
    "func.return"() : () -> ()
  }) {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, hivm.part_of_mix, mix_mode = "aiv"} : () -> ()
}) : () -> ()
