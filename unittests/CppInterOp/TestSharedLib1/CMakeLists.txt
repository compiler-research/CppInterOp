add_llvm_library(TestSharedLib1
  SHARED
  DISABLE_LLVM_LINK_LLVM_DYLIB
  BUILDTREE_ONLY
  TestSharedLib1.cpp)
# Put TestSharedLib1 next to the unit test executable.
set_output_directory(TestSharedLib1
  BINARY_DIR ${CMAKE_BINARY_DIR}/unittests/bin/$<CONFIG>/
  LIBRARY_DIR ${CMAKE_BINARY_DIR}/unittests/bin/$<CONFIG>/
  )
set_target_properties(TestSharedLib1 PROPERTIES FOLDER "Tests")
