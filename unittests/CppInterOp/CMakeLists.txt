set(LLVM_LINK_COMPONENTS
  Support
)

add_cppinterop_unittest(CppInterOpTests
  CUDATest.cpp
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

set_output_directory(CppInterOpTests
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/CppInterOpTests/unittests/bin/$<CONFIG>/
  LIBRARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/CppInterOpTests/unittests/bin/$<CONFIG>/
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

add_cppinterop_unittest(DynamicLibraryManagerTests DynamicLibraryManagerTest.cpp)
target_link_libraries(DynamicLibraryManagerTests
  PRIVATE
  clangCppInterOp
)

set_source_files_properties(DynamicLibraryManagerTest.cpp PROPERTIES COMPILE_DEFINITIONS
  "LLVM_BINARY_DIR=\"${LLVM_BINARY_DIR}\""
  )
set_source_files_properties(DynamicLibraryManagerTest.cpp PROPERTIES COMPILE_DEFINITIONS
  "CPPINTEROP_VERSION=\"${CPPINTEROP_VERSION}\""
  )

set_output_directory(DynamicLibraryManagerTests BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/TestSharedLib/unittests/bin/$<CONFIG>/)

set_output_directory(DynamicLibraryManagerTests
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/TestSharedLib/unittests/bin/$<CONFIG>/
  LIBRARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/TestSharedLib/unittests/bin/$<CONFIG>/
)
add_dependencies(DynamicLibraryManagerTests TestSharedLib)
#export_executable_symbols_for_plugins(TestSharedLib)
add_subdirectory(TestSharedLib)
add_dependencies(DynamicLibraryManagerTests TestSharedLib1)
#export_executable_symbols_for_plugins(TestSharedLib1)
add_subdirectory(TestSharedLib1)
add_dependencies(DynamicLibraryManagerTests TestSharedLib2)
#export_executable_symbols_for_plugins(TestSharedLib2)
add_subdirectory(TestSharedLib2)
add_dependencies(DynamicLibraryManagerTests TestSharedLib3)
#export_executable_symbols_for_plugins(TestSharedLib3)
add_subdirectory(TestSharedLib3)
