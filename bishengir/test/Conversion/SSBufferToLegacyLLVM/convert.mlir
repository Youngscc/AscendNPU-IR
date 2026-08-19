// RUN: bishengir-opt %s --convert-ssbuffer-to-legacy-llvm > %t
// RUN: FileCheck %s < %t
// RUN: not grep -E "address_space<ssbuf>|hivm.hir.pointer_cast|memref_ext.volatile" %t

module {
  func.func @lower_ssbuffer(
      %address: i64, %i8_value: i8, %f32_value: f32) -> (i8, f32) {
    %i8_ptr = hivm.hir.pointer_cast(%address)
        : memref<i8, #hivm.address_space<ssbuf>>
    memref.store %i8_value, %i8_ptr[]
        : memref<i8, #hivm.address_space<ssbuf>>
    %i8_loaded = memref.load %i8_ptr[]
        : memref<i8, #hivm.address_space<ssbuf>>
    annotation.mark %i8_loaded {memref_ext.volatile} : i8

    %offset = arith.constant 8 : i64
    %f32_address = arith.addi %address, %offset : i64
    %f32_ptr = hivm.hir.pointer_cast(%f32_address)
        : memref<f32, #hivm.address_space<ssbuf>>
    memref.store %f32_value, %f32_ptr[]
        : memref<f32, #hivm.address_space<ssbuf>>
    %f32_loaded = memref.load %f32_ptr[]
        : memref<f32, #hivm.address_space<ssbuf>>
    annotation.mark %f32_loaded {memref_ext.volatile} : f32
    return %i8_loaded, %f32_loaded : i8, f32
  }
}

// CHECK-LABEL: func.func @lower_ssbuffer(
// CHECK-SAME: %[[ADDRESS:.*]]: i64, %[[I8_VALUE:.*]]: i8, %[[F32_VALUE:.*]]: f32
// CHECK: %[[I8_PTR:.*]] = llvm.inttoptr %[[ADDRESS]] : i64 to !llvm.ptr<11>
// CHECK: llvm.store volatile %[[I8_VALUE]], %[[I8_PTR]] : i8, !llvm.ptr<11>
// CHECK: %[[I8_LOADED:.*]] = llvm.load volatile %[[I8_PTR]] : !llvm.ptr<11> -> i8
// CHECK: %[[F32_ADDRESS:.*]] = arith.addi %[[ADDRESS]], %{{.*}} : i64
// CHECK: %[[F32_PTR:.*]] = llvm.inttoptr %[[F32_ADDRESS]] : i64 to !llvm.ptr<11>
// CHECK: llvm.store volatile %[[F32_VALUE]], %[[F32_PTR]] : f32, !llvm.ptr<11>
// CHECK: %[[F32_LOADED:.*]] = llvm.load volatile %[[F32_PTR]] : !llvm.ptr<11> -> f32
// CHECK: return %[[I8_LOADED]], %[[F32_LOADED]] : i8, f32
