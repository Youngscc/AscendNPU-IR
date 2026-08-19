add_bishengir_conversion_library(BiShengIRSSBufferToLegacyLLVM
  SSBufferToLegacyLLVM.cpp

  ADDITIONAL_HEADER_DIRS
  ${BISHENGIR_MAIN_INCLUDE_DIR}/bishengir/Conversion/SSBufferToLegacyLLVM

  DEPENDS
  BiShengIRConversionPassIncGen

  LINK_COMPONENTS
  Core

  LINK_LIBS PUBLIC
  BiShengIRAnnotationDialect
  BiShengIRHIVMDialect
  MLIRLLVMDialect
  MLIRMemRefDialect
)
