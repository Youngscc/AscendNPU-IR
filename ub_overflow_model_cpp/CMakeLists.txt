set(LLVM_REQUIRES_EH ON)
add_bishengir_library(BiShengIRUBOverflowModel
  src/api.cpp
)
unset(LLVM_REQUIRES_EH)

target_compile_features(BiShengIRUBOverflowModel PUBLIC cxx_std_17)
target_include_directories(BiShengIRUBOverflowModel PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
set_target_properties(BiShengIRUBOverflowModel PROPERTIES
  POSITION_INDEPENDENT_CODE ON
)

# LLVM is normally built without exception support. LLVM_REQUIRES_EH above is
# scoped to this target; the model catches all exceptions at its noexcept API.
