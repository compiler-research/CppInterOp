add_llvm_library(TestSharedLib
  SHARED
  DISABLE_LLVM_LINK_LLVM_DYLIB
  BUILDTREE_ONLY
  TestSharedLib.cpp)
# Put TestSharedLib next to the unit test executable.
set_output_directory(TestSharedLib
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/unittests/bin/$<CONFIG>/
  LIBRARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/unittests/bin/$<CONFIG>/
  )


if (EMSCRIPTEN)
  set_target_properties(TestSharedLib PROPERTIES
    NO_SONAME 1
    LINK_FLAGS "-s WASM_BIGINT  -s SIDE_MODULE=1"
  )
endif()

set_target_properties(TestSharedLib PROPERTIES FOLDER "Tests")
