add_llvm_library(TestSharedLib2
  SHARED
  DISABLE_LLVM_LINK_LLVM_DYLIB
  BUILDTREE_ONLY
  TestSharedLib2.cpp)
# Put TestSharedLib2 next to the unit test executable.
set_output_directory(TestSharedLib2
  BINARY_DIR ${CMAKE_BINARY_DIR}/unittests/bin/$<CONFIG>/
  LIBRARY_DIR ${CMAKE_BINARY_DIR}/unittests/bin/$<CONFIG>/
  )
set_target_properties(TestSharedLib2 PROPERTIES FOLDER "Tests")
