set(LLVM_LINK_COMPONENTS
  Support
  )

option(TEST_INTEROP_CUDA "Enables the CUDA Tests" ON)

if (TEST_INTEROP_CUDA)
    add_definitions(-DTEST_INTEROP_CUDA)
    add_cppinterop_unittest(CppInterOpCUDATests CUDATest.cpp)
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
  )

target_link_libraries(CppInterOpTests
  PRIVATE
  clangCppInterOp
  )

target_link_libraries(CppInterOpCUDATests
  PRIVATE
  clangCppInterOp
  )

set_source_files_properties(InterpreterTest.cpp PROPERTIES COMPILE_DEFINITIONS
  "LLVM_BINARY_DIR=\"${LLVM_BINARY_DIR}\""
  )
  
export_executable_symbols(CppInterOpTests
  CppInterOpCUDATests
  )

unset(LLVM_LINK_COMPONENTS)

add_cppinterop_unittest(DynamicLibraryManagerTests DynamicLibraryManagerTest.cpp)
target_link_libraries(DynamicLibraryManagerTests
  PRIVATE
  clangCppInterOp
  )

set_output_directory(DynamicLibraryManagerTests BINARY_DIR ${CMAKE_BINARY_DIR}/unittests/bin/$<CONFIG>/)

add_dependencies(DynamicLibraryManagerTests TestSharedLib)
#export_executable_symbols_for_plugins(TestSharedLib)
add_subdirectory(TestSharedLib)
