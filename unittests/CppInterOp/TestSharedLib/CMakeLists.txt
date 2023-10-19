add_llvm_library(TestSharedLib SHARED BUILDTREE_ONLY TestSharedLib.cpp)
# Put TestSharedLib next to the unit test executable.
set_output_directory(TestSharedLib
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/../
  LIBRARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/../
  )
set_target_properties(TestSharedLib PROPERTIES FOLDER "Tests")
