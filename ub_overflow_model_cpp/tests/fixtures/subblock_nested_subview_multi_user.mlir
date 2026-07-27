// BubbleUpSubviewFromTiling must create a fresh 32-row parent view for the
// marked child. The original 8-row parent has another user and must keep its
// old type and inner-loop offset.
"builtin.module"() ({
  "func.func"() <{function_type = (memref<64x128xbf16, strided<[128, 1]>>) -> (), sym_name = "nested_subview_multi_user_aiv"}> ({
  ^bb0(%source: memref<64x128xbf16, strided<[128, 1]>>):
    %c0 = "arith.constant"() <{value = 0 : index}> : () -> index
    %c1 = "arith.constant"() <{value = 1 : index}> : () -> index
    %c2 = "arith.constant"() <{value = 2 : index}> : () -> index
    %c8 = "arith.constant"() <{value = 8 : index}> : () -> index
    "scf.for"(%c0, %c2, %c1) ({
    ^bb0(%subblock: index):
      %subblock_offset = "affine.apply"(%subblock) <{map = affine_map<()[s0] -> (s0 * 4)>}> : (index) -> index
      "scf.for"(%c0, %c8, %c1) ({
      ^bb0(%inner: index):
        %inner_offset = "affine.apply"(%inner) <{map = affine_map<()[s0] -> (s0 * 8)>}> : (index) -> index
        %parent = "memref.subview"(%source, %inner_offset) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808, 0>, static_sizes = array<i64: 8, 128>, static_strides = array<i64: 1, 1>}> : (memref<64x128xbf16, strided<[128, 1]>>, index) -> memref<8x128xbf16, strided<[128, 1], offset: ?>>
        %child = "memref.subview"(%parent, %subblock_offset) <{operandSegmentSizes = array<i32: 1, 1, 0, 0>, static_offsets = array<i64: -9223372036854775808, 0>, static_sizes = array<i64: 4, 128>, static_strides = array<i64: 1, 1>}> {to_be_bubbled_slice} : (memref<8x128xbf16, strided<[128, 1], offset: ?>>, index) -> memref<4x128xbf16, strided<[128, 1], offset: ?>>
        "annotation.mark"(%parent) <{effects = ["read"]}> : (memref<8x128xbf16, strided<[128, 1], offset: ?>>) -> ()
        "annotation.mark"(%child) <{effects = ["read"]}> : (memref<4x128xbf16, strided<[128, 1], offset: ?>>) -> ()
        "scf.yield"() : () -> ()
      }) : (index, index, index) -> ()
      "scf.yield"() : () -> ()
    }) {map_for_to_forall, mapping = [#hivm.sub_block<x>]} : (index, index, index) -> ()
    "func.return"() : () -> ()
  }) {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>, hivm.part_of_mix} : () -> ()
}) : () -> ()
