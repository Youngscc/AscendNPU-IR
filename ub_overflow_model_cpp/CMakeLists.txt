set(LLVM_REQUIRES_EH ON)
add_bishengir_library(BiShengIRUBOverflowModel
  src/api.cpp
)
unset(LLVM_REQUIRES_EH)

target_compile_features(BiShengIRUBOverflowModel PUBLIC cxx_std_17)
target_compile_definitions(BiShengIRUBOverflowModel PRIVATE
  CVUB_ENABLE_MLIR_API=1
)
target_compile_definitions(obj.BiShengIRUBOverflowModel PRIVATE
  CVUB_ENABLE_MLIR_API=1
)
target_include_directories(BiShengIRUBOverflowModel PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
set_target_properties(BiShengIRUBOverflowModel PROPERTIES
  POSITION_INDEPENDENT_CODE ON
)
target_link_libraries(BiShengIRUBOverflowModel PRIVATE
  MLIRIR
)

# The embedded path receives a ModuleOp from bishengir-compile.  This small
# standalone driver deliberately uses the same borrowed-ModuleOp API: parsing a
# file belongs at the executable boundary, not inside the model core.
add_executable(bishengir-ub-overflow-model
  src/mlir_main.cpp
)
target_compile_features(bishengir-ub-overflow-model PRIVATE cxx_std_17)
target_link_libraries(bishengir-ub-overflow-model PRIVATE
  BiShengIRUBOverflowModel
  MLIRIR
  MLIRParser
  MLIRSupport
)
mlir_check_all_link_libraries(bishengir-ub-overflow-model)

# LLVM is normally built without exception support. LLVM_REQUIRES_EH above is
# scoped to this target; the model catches all exceptions at its noexcept API.
