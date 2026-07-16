add_bishengir_dialect_library(MLIRHIVMAVEPipelines
  HIVMAVEPipelines.cpp

  ADDITIONAL_HEADER_DIRS
  ${MLIR_MAIN_INCLUDE_DIR}/bishengir/Dialect/HIVM
  ${MLIR_MAIN_INCLUDE_DIR}/bishengir/Dialect/HIVMAVE

  LINK_LIBS PUBLIC
  BiShengIRTensorTransforms
  BiShengIRMemRefTransforms
  BiShengIRSCFTransforms
  BiShengIRTransform
  MLIRArithDialect
  BiShengIRHIVMTransforms
  MLIRMathDialect
  BiShengIRMathExtDialect
  BiShengIRMemRefDialect
  MLIRMemRefTransforms
  MLIRFuncDialect
  MLIRPass
  MLIRTransforms
  BiShengIRHFusionTransforms
  MLIRReconcileUnrealizedCasts
)