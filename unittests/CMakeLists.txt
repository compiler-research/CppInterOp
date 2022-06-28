set(CTEST_BUILD_NAME ${CMAKE_SYSTEM_NAME}-${CMAKE_HOST_SYSTEM_PROCESSOR}-${CMAKE_BUILD_TYPE})
enable_testing()

# LLVM builds (not installed llvm) provides gtest.
if (NOT TARGET gtest)
  include(GoogleTest)
endif()

add_custom_target(InterOpUnitTests)
set_target_properties(InterOpUnitTests PROPERTIES FOLDER "InterOp tests")
 
# add_clang_unittest(test_dirname file1.cpp file2.cpp)
#
# Will compile the list of files together and link against the clang
# Produces a binary named 'basename(test_dirname)'.
function(add_interop_unittest test_dirname)
  add_unittest(InterOpUnitTests ${test_dirname} ${ARGN})
endfunction()
 
add_subdirectory(InterOp)

add_dependencies(InterOpUnitTests clangInterOp)
add_dependencies(InterOpTests clangInterOp)
