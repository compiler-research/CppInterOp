add_executable(cppinterop-benchmarks benchmark.cpp)

add_dependencies(cppinterop-benchmarks clangCppInterOp)

target_link_libraries(cppinterop-benchmarks PRIVATE clangCppInterOp)

target_compile_options(cppinterop-benchmarks
  PUBLIC "SHELL: ${CPPINTEROP_EXTRA_WASM_FLAGS}"
)

target_link_options(cppinterop-benchmarks
  PUBLIC "SHELL: ${CPPINTEROP_EXTRA_WASM_FLAGS}"
  PUBLIC "SHELL: -s MAIN_MODULE=1"
  PUBLIC "SHELL: -s WASM_BIGINT"
  PUBLIC "SHELL: -s ALLOW_MEMORY_GROWTH=1"
  PUBLIC "SHELL: -s STACK_SIZE=32mb"
  PUBLIC "SHELL: -s INITIAL_MEMORY=64mb"
  PUBLIC "SHELL: --emrun"
)

set_output_directory(cppinterop-benchmarks
    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}
)

add_custom_command(TARGET cppinterop-benchmarks POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:clangCppInterOp> ${CMAKE_CURRENT_BINARY_DIR}
)

add_custom_target(run-cppinterop-benchmarks
    COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_CURRENT_BINARY_DIR} node cppinterop-benchmarks.js
    DEPENDS cppinterop-benchmarks
)
