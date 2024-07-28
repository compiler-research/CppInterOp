add_llvm_library(TestSharedLib3
  SHARED
  DISABLE_LLVM_LINK_LLVM_DYLIB
  BUILDTREE_ONLY
  TestSharedLib3.cpp)
# Put TestSharedLib3 next to the unit test executable.
set_output_directory(TestSharedLib3
  BINARY_DIR ${CMAKE_BINARY_DIR}/unittests/bin/$<CONFIG>/
  LIBRARY_DIR ${CMAKE_BINARY_DIR}/unittests/bin/$<CONFIG>/
  )
set_target_properties(TestSharedLib2 PROPERTIES FOLDER "Tests")
