add_llvm_library(TestSharedLib2
  SHARED
  DISABLE_LLVM_LINK_LLVM_DYLIB
  BUILDTREE_ONLY
  TestSharedLib.cpp)
# Put TestSharedLib2 next to the unit test executable.
set_output_directory(TestSharedLib2
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/../TestSharedLib/unittests/bin/$<CONFIG>/
  LIBRARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/../TestSharedLib/unittests/bin/$<CONFIG>/
  )


if (EMSCRIPTEN)
  set_target_properties(TestSharedLib2
    PROPERTIES NO_SONAME 1
  )
  target_link_options(TestSharedLib2
    PRIVATE "SHELL: -s WASM_BIGINT"
    PRIVATE "SHELL: -s SIDE_MODULE=1"
  )
endif()

set_target_properties(TestSharedLib2 PROPERTIES FOLDER "Tests")
