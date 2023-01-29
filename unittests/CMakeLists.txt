set(CTEST_BUILD_NAME ${CMAKE_SYSTEM_NAME}-${CMAKE_HOST_SYSTEM_PROCESSOR}-${CMAKE_BUILD_TYPE})
enable_testing()

include(GoogleTest)

add_custom_target(CppInterOpUnitTests)
set_target_properties(CppInterOpUnitTests PROPERTIES FOLDER "CppInterOp tests")
 
# add_clang_unittest(test_dirname file1.cpp file2.cpp)
#
# Will compile the list of files together and link against the clang
# Produces a binary named 'basename(test_dirname)'.
function(add_cppinterop_unittest test_dirname)
  add_unittest(CppInterOpUnitTests ${test_dirname} ${ARGN})
endfunction()
 
add_subdirectory(CppInterOp)

