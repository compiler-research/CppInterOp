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
  # Explaination of compile and link flags
  # To dynamically link to a shared library for Emscripten builds you must use the MAIN_MODULE flag (see both https://github.com/emscripten-core/emscripten/issues/23543#issuecomment-2625334414
  # and https://emscripten.org/docs/compiling/Dynamic-Linking.html)
  # Without WASM_BIGINT flag then you get fatal errors when trying to run compiled Javascript about trying to convert between BigInt and non BigInt types
  # EXPORTED_RUNTIME_METHODS='[\"FS\",\"PATH\",\"LDSO\",\"loadDynamicLibrary\",\"ERRNO_CODES\"]' and --preload-file ${SYSROOT_PATH}/include@/include are not allow the Javascript that is 
  # compiled to have access to the standard library headers (approach taken from xeus-cpp)
  # Without ALLOW_MEMORY_GROWTH=1 tests will fail with aborted(OOM). Approach to fix taken from answers to
  # https://stackoverflow.com/questions/67222200/runtimeerror-abortoom-build-with-s-assertions-1-for-more-info
  set_target_properties(CppInterOpTests PROPERTIES
    LINK_FLAGS "-s MAIN_MODULE=1 -s WASM_BIGINT -s EXPORTED_RUNTIME_METHODS='[\"FS\",\"PATH\",\"LDSO\",\"loadDynamicLibrary\",\"ERRNO_CODES\"]' --preload-file ${SYSROOT_PATH}/include@/include -s ALLOW_MEMORY_GROWTH=1"
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
  # Explaination of compile and link flags
  # To dynamically link to a shared library for Emscripten builds you must use the MAIN_MODULE flag (see both https://github.com/emscripten-core/emscripten/issues/23543#issuecomment-2625334414
  # and https://emscripten.org/docs/compiling/Dynamic-Linking.html)
  # Without WASM_BIGINT flag then you get fatal errors when trying to run compiled Javascript about trying to convert between BigInt and non BigInt types
  # EXPORTED_RUNTIME_METHODS='[\"FS\",\"PATH\",\"LDSO\",\"loadDynamicLibrary\",\"ERRNO_CODES\"]' and --preload-file ${SYSROOT_PATH}/include@/include are not allow the Javascript that is 
  # compiled to have access to the standard library headers (approach taken from xeus-cpp)
  # Without ALLOW_MEMORY_GROWTH=1 tests will fail with aborted(OOM). Approach to fix taken from answers to
  # https://stackoverflow.com/questions/67222200/runtimeerror-abortoom-build-with-s-assertions-1-for-more-info
  set_target_properties(DynamicLibraryManagerTests PROPERTIES
    LINK_FLAGS "-s MAIN_MODULE=1 -s WASM_BIGINT -s EXPORTED_RUNTIME_METHODS='[\"FS\",\"PATH\",\"LDSO\",\"loadDynamicLibrary\",\"ERRNO_CODES\"]' --preload-file ${SYSROOT_PATH}/include@/include -s ALLOW_MEMORY_GROWTH=1"
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
