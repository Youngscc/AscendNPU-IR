"builtin.module"() ({
  "func.func"() <{function_type = () -> (), sym_name = "local_buffer"}> ({
  ^bb0:
    %c0 = "arith.constant"() {value = 0 : index} : () -> index
    %c1 = "arith.constant"() {value = 1 : index} : () -> index
    %c2 = "arith.constant"() {value = 2 : index} : () -> index
    "scf.for"(%c0, %c2, %c1) ({
    ^bb1(%iv: index):
      %gm = "memref.alloc"() : () -> memref<16xf16, #hivm.address_space<gm>>
      %ub = "memref.alloc"() : () -> memref<16xf16, #hivm.address_space<ub>>
      "hivm.hir.load"(%gm, %ub) <{init_out_buffer = false, may_implicit_transpose_with_last_axis = false, operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>}> : (memref<16xf16, #hivm.address_space<gm>>, memref<16xf16, #hivm.address_space<ub>>) -> ()
      %cc = "memref.alloc"() : () -> memref<16xf32, #hivm.address_space<cc>>
      %gm_out = "memref.alloc"() : () -> memref<16xf32, #hivm.address_space<gm>>
      "hivm.hir.fixpipe"(%cc, %gm_out) <{dma_mode = #hivm.dma_mode<nz2nd>, operandSegmentSizes = array<i32: 1, 1, 0, 0>}> : (memref<16xf32, #hivm.address_space<cc>>, memref<16xf32, #hivm.address_space<gm>>) -> ()
      "scf.yield"() : () -> ()
    }) : (index, index, index) -> ()
    "func.return"() : () -> ()
  }) {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>} : () -> ()

  "func.func"() <{function_type = () -> (), sym_name = "preload_scope"}> ({
  ^bb0:
    %result = "scope.scope"() ({
      %ub = "memref.alloc"() : () -> memref<32xf16, #hivm.address_space<ub>>
      "scope.return"(%ub) : (memref<32xf16, #hivm.address_space<ub>>) -> ()
    }) {hivm.loop_core_type = #hivm.tcore_type<VECTOR>, hivm.preload_num = 1 : i64} : () -> memref<32xf16, #hivm.address_space<ub>>
    "scope.scope"() ({
      "annotation.mark"(%result) <{effects = ["write"]}> {case = "v1_use"} : (memref<32xf16, #hivm.address_space<ub>>) -> ()
      "scope.return"() : () -> ()
    }) : () -> ()
    "func.return"() : () -> ()
  }) {hacc.entry, hacc.function_kind = #hacc.function_kind<DEVICE>, hivm.func_core_type = #hivm.func_core_type<AIV>} : () -> ()
}) : () -> ()
