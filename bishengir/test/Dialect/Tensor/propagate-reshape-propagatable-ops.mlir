// RUN: bishengir-opt -propagate-reshape -allow-unregistered-dialect %s -split-input-file | FileCheck %s

// CHECK-LABEL: func.func @mulext_through_collapse
// CHECK-NOT: tensor.collapse_shape {{.*}} into tensor<6x4xi32>
// CHECK: %[[LOW:.*]], %[[HIGH:.*]] = hfusion.mulext %{{.*}}, %{{.*}} : tensor<2x3x4xi32>
// CHECK: %[[COLLAPSED:.*]] = tensor.collapse_shape %[[LOW]]
// CHECK-SAME: : tensor<2x3x4xi32> into tensor<6x4xi32>
// CHECK: return %[[COLLAPSED]]
func.func @mulext_through_collapse(%arg0: tensor<2x3x4xi32>,
                                   %arg1: tensor<2x3x4xi32>) -> tensor<6x4xi32> {
  %0 = tensor.empty() : tensor<2x3x4xi32>
  %1 = hfusion.elemwise_binary {fun = #hfusion.binary_fn<vxor>}
      ins(%arg0, %arg0 : tensor<2x3x4xi32>, tensor<2x3x4xi32>)
      outs(%0 : tensor<2x3x4xi32>) -> tensor<2x3x4xi32>
  %collapsed = tensor.collapse_shape %1 [[0, 1], [2]]
      : tensor<2x3x4xi32> into tensor<6x4xi32>
  %collapsed_arg = tensor.collapse_shape %arg1 [[0, 1], [2]]
      : tensor<2x3x4xi32> into tensor<6x4xi32>
  %low, %high = hfusion.mulext %collapsed, %collapsed_arg
      : tensor<6x4xi32>
  return %low : tensor<6x4xi32>
}

// -----

// Collapse with an extra user must stay for that user; MulExt still gets the
// high-rank source and emits a new collapse on its result.
// CHECK-LABEL: func.func @mulext_collapse_shared_user
// CHECK: %[[COLLAPSE:.*]] = tensor.collapse_shape
// CHECK: "test.keep"(%[[COLLAPSE]])
// CHECK: hfusion.mulext
// CHECK-SAME: tensor<2x3x4xi32>
// CHECK: tensor.collapse_shape
func.func @mulext_collapse_shared_user(%arg0: tensor<2x3x4xi32>,
                                       %arg1: tensor<2x3x4xi32>)
    -> (tensor<6x4xi32>, tensor<6x4xi32>) {
  %0 = tensor.empty() : tensor<2x3x4xi32>
  %1 = hfusion.elemwise_binary {fun = #hfusion.binary_fn<vxor>}
      ins(%arg0, %arg0 : tensor<2x3x4xi32>, tensor<2x3x4xi32>)
      outs(%0 : tensor<2x3x4xi32>) -> tensor<2x3x4xi32>
  %collapsed = tensor.collapse_shape %1 [[0, 1], [2]]
      : tensor<2x3x4xi32> into tensor<6x4xi32>
  %collapsed_arg = tensor.collapse_shape %arg1 [[0, 1], [2]]
      : tensor<2x3x4xi32> into tensor<6x4xi32>
  "test.keep"(%collapsed) : (tensor<6x4xi32>) -> ()
  %low, %high = hfusion.mulext %collapsed, %collapsed_arg
      : tensor<6x4xi32>
  return %low, %collapsed : tensor<6x4xi32>, tensor<6x4xi32>
}

// -----

// MarkOp has no results; collapse must be erased when mark was its only user.
// CHECK-LABEL: func.func @mark_through_collapse
// CHECK-NOT: tensor.collapse_shape
// CHECK: annotation.mark %{{.*}} : tensor<2x3xf32>
func.func @mark_through_collapse(%arg0: tensor<2x3xf32>) {
  %source = math.absf %arg0 : tensor<2x3xf32>
  %collapsed = tensor.collapse_shape %source [[0, 1]]
      : tensor<2x3xf32> into tensor<6xf32>
  annotation.mark %collapsed {overflow_mode = "saturate"} : tensor<6xf32>
  return
}

// -----

// CHECK-LABEL: func.func @mark_collapse_shared_user
// CHECK: %[[COLLAPSE:.*]] = tensor.collapse_shape
// CHECK: "test.keep"(%[[COLLAPSE]])
// CHECK: annotation.mark %{{.*}} : tensor<2x3xf32>
func.func @mark_collapse_shared_user(%arg0: tensor<2x3xf32>) -> tensor<6xf32> {
  %source = math.absf %arg0 : tensor<2x3xf32>
  %collapsed = tensor.collapse_shape %source [[0, 1]]
      : tensor<2x3xf32> into tensor<6xf32>
  "test.keep"(%collapsed) : (tensor<6xf32>) -> ()
  annotation.mark %collapsed {overflow_mode = "saturate"} : tensor<6xf32>
  return %collapsed : tensor<6xf32>
}
