"builtin.module"() ({
  "func.func"() <{function_type = (memref<64xf16>) -> (), sym_name = "lift_load"}> ({
  ^bb0(%src: memref<64xf16>):
    %dst = "memref.alloc"() <{operandSegmentSizes = array<i32: 0, 0>}> : () -> memref<64xf16>
    "hivm.hir.load"(%src, %dst) <{operandSegmentSizes = array<i32: 1, 1, 0, 0, 0, 0>}> : (memref<64xf16>, memref<64xf16>) -> ()
    %tensor = "bufferization.to_tensor"(%dst) <{restrict, writable}> : (memref<64xf16>) -> tensor<64xf16>
    "func.return"() : () -> ()
  }) : () -> ()
}) : () -> ()
