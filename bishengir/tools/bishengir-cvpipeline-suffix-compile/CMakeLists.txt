if(MLIR_INCLUDE_TESTS)
  set(TEST_LIBS
    BiShengIRTestDialect
    MLIRTestDialect
    MLIRTestTransformDialect
    MLIRTestTransforms
  )
endif()

get_property(PRIVATE_BISHENGIR_LIBS GLOBAL PROPERTY BISHENGIR_LIBS)
get_property(PRIVATE_BISHENGIR_CONVERSION_LIBS GLOBAL PROPERTY BISHENGIR_CONVERSION_LIBS)
get_property(PRIVATE_BISHENGIR_TRANSLATION_LIBS GLOBAL PROPERTY BISHENGIR_TRANSLATION_LIBS)
get_property(PUBLIC_BISHENGIR_CONVERSION_LIBS GLOBAL PROPERTY BISHENGIR_CONVERSION_OPEN_SOURCE_LIBS)

set(LIBS
  ${TEST_LIBS}
  ${PRIVATE_BISHENGIR_LIBS}
  ${PRIVATE_BISHENGIR_CONVERSION_LIBS}
  ${PRIVATE_BISHENGIR_TRANSLATION_LIBS}
  ${PUBLIC_BISHENGIR_CONVERSION_LIBS}
)

add_bishengir_publish_tool(bishengir-cvpipeline-suffix-compile
  bishengir-cvpipeline-suffix-compile.cpp

  DEPENDS
  ${LIBS}
)

target_include_directories(bishengir-cvpipeline-suffix-compile
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(bishengir-cvpipeline-suffix-compile
    PRIVATE -Wshadow -Werror=shadow)
endif()
target_link_libraries(bishengir-cvpipeline-suffix-compile PRIVATE ${LIBS})
mlir_check_all_link_libraries(bishengir-cvpipeline-suffix-compile)

# Exact CVPipelining-before -> local PlanMemory compiler.  It intentionally
# shares the driver/diagnostic implementation with the experimental suffix
# oracle while selecting the unabridged production pass sequence at compile
# time, so both tools remain independently buildable.
add_bishengir_publish_tool(cv2pm-bishengir-compile
  bishengir-cvpipeline-suffix-compile.cpp

  DEPENDS
  ${LIBS}
)
target_compile_definitions(cv2pm-bishengir-compile
  PRIVATE BISHENGIR_CV2PM_FULL_PIPELINE=1)
target_include_directories(cv2pm-bishengir-compile
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(cv2pm-bishengir-compile
    PRIVATE -Wshadow -Werror=shadow)
endif()
target_link_libraries(cv2pm-bishengir-compile PRIVATE ${LIBS})
mlir_check_all_link_libraries(cv2pm-bishengir-compile)
