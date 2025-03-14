set(LLVM_LINK_COMPONENTS
  Support
)
if (EMSCRIPTEN)
  # Omitting CUDATest.cpp since Emscripten build currently has no GPU support
  # For Emscripten builds linking to gtest_main will not suffice for gtest to run
  # the tests and an explictly main.cpp is needed
  set(EXTRA_TEST_SOURCE_FILES main.cpp)
else()
  # Do not need main.cpp for native builds, but we do have GPU support for native builds
  set(EXTRA_TEST_SOURCE_FILES CUDATest.cpp)
  set(EXTRA_PATH_TEST_BINARIES /CppInterOpTests/unittests/bin/$<CONFIG>/)
endif()

add_cppinterop_unittest(CppInterOpTests
  EnumReflectionTest.cpp
  FunctionReflectionTest.cpp
  InterpreterTest.cpp
  JitTest.cpp
  ScopeReflectionTest.cpp
  TypeReflectionTest.cpp
  Utils.cpp
  VariableReflectionTest.cpp
  ${EXTRA_TEST_SOURCE_FILES}
)

if(EMSCRIPTEN)
  target_compile_options(CppInterOpTests
    PUBLIC "SHELL: -fexceptions"
  )

  target_link_options(CppInterOpTests
    PUBLIC "SHELL: -fexceptions"
    PUBLIC "SHELL: -s MAIN_MODULE=1"
    PUBLIC "SHELL: -s WASM=1"
    PUBLIC "SHELL: -s WASM_BIGINT"
    PUBLIC "SHELL: -s ASSERTIONS=0"
    PUBLIC "SHELL: -s ALLOW_MEMORY_GROWTH=1"
    PUBLIC "SHELL: -s EXIT_RUNTIME=1"
    PUBLIC "SHELL: -s STACK_SIZE=32mb"
    PUBLIC "SHELL: -s INITIAL_MEMORY=128mb"
    PUBLIC "SHELL: --preload-file ${SYSROOT_PATH}/include@/include"
  )
endif()

target_link_libraries(CppInterOpTests
  PRIVATE
  clangCppInterOp
)

set_output_directory(CppInterOpTests
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/${EXTRA_PATH_TEST_BINARIES}
  LIBRARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/${EXTRA_PATH_TEST_BINARIES}
)

if(NOT WIN32)
  set_source_files_properties(VariableReflectionTest.cpp PROPERTIES COMPILE_FLAGS
    "-Wno-pedantic"
)
endif()

set_source_files_properties(InterpreterTest.cpp PROPERTIES COMPILE_DEFINITIONS
  "LLVM_BINARY_DIR=\"${LLVM_BINARY_DIR}\""
)
export_executable_symbols(CppInterOpTests)

unset(LLVM_LINK_COMPONENTS)

add_cppinterop_unittest(DynamicLibraryManagerTests DynamicLibraryManagerTest.cpp ${EXTRA_TEST_SOURCE_FILES})

target_link_libraries(DynamicLibraryManagerTests
  PRIVATE
  clangCppInterOp
)

if(EMSCRIPTEN)
  target_compile_options(DynamicLibraryManagerTests
    PUBLIC "SHELL: -fexceptions"
  )

  target_link_options(DynamicLibraryManagerTests
    PUBLIC "SHELL: -fexceptions"
    PUBLIC "SHELL: -s MAIN_MODULE=1"
    PUBLIC "SHELL: -s WASM=1"
    PUBLIC "SHELL: -s WASM_BIGINT"
    PUBLIC "SHELL: -s ASSERTIONS=0"
    PUBLIC "SHELL: -s ALLOW_MEMORY_GROWTH=1"
    PUBLIC "SHELL: -s EXIT_RUNTIME=1"
    PUBLIC "SHELL: -s STACK_SIZE=32mb"
    PUBLIC "SHELL: -s INITIAL_MEMORY=128mb"
    PUBLIC "SHELL: --preload-file ${SYSROOT_PATH}/include@/include"
  )
endif()

 if (NOT EMSCRIPTEN)
  set(EXTRA_PATH_TEST_BINARIES /TestSharedLib/unittests/bin/$<CONFIG>/)
endif()

set_output_directory(DynamicLibraryManagerTests
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/${EXTRA_PATH_TEST_BINARIES}
  LIBRARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/${EXTRA_PATH_TEST_BINARIES}
)

add_dependencies(DynamicLibraryManagerTests TestSharedLib)

#export_executable_symbols_for_plugins(TestSharedLib)
add_subdirectory(TestSharedLib)
