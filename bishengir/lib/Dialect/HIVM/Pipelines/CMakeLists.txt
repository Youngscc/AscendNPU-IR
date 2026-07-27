add_bishengir_dialect_library(BiShengIRHIVMPipelines
  ConvertToHIVMPipeline.cpp
  HIVMPipelines.cpp
  UBOverflowPrediction.cpp

  ADDITIONAL_HEADER_DIRS
  ${MLIR_MAIN_INCLUDE_DIR}/bishengir/Dialect/HIVM

  LINK_LIBS PUBLIC
  BiShengIRHFusionToHIVM
  BiShengIRHFusionTransforms
  BiShengIRHIVMTransforms
  BiShengIRMathExtDialect
  BiShengIRMemRefTransforms
  BiShengIRSCFTransforms
  BiShengIRTensorToHIVM
  BiShengIRTensorTransforms
  BiShengIRTransform
  BiShengIRUBOverflowModel
  MLIRArithDialect
  MLIRBytecodeWriter
  MLIRFuncDialect
  MLIRMathDialect
  MLIRMemRefDialect
  MLIRMemRefTransforms
  MLIRPass
  MLIRTransforms
)
