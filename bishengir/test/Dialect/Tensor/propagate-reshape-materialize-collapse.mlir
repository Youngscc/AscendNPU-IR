// RUN: bishengir-opt -propagate-reshape=for-regbased=true -allow-unregistered-dialect %s | FileCheck %s

// Memref materialize has zero results; after rewiring to collapse src the
// original collapse must be erased when unused.
// CHECK-LABEL: func.func @materialize_through_collapse
// CHECK-NOT: tensor.collapse_shape
// CHECK: memref.expand_shape
// CHECK: bufferization.materialize_in_destination
// CHECK-SAME: tensor<2x3xf32>
func.func @materialize_through_collapse(
    %arg0: tensor<2x3xf32>, %dest: memref<6xf32>) {
  %source = math.absf %arg0 : tensor<2x3xf32>
  %collapsed = tensor.collapse_shape %source [[0, 1]]
      : tensor<2x3xf32> into tensor<6xf32>
  bufferization.materialize_in_destination %collapsed in writable %dest
      : (tensor<6xf32>, memref<6xf32>) -> ()
  return
}
