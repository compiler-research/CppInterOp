# Regression guard for CppInterOpTypes.h staying header-only. This
# executable includes only CppInterOpTypes.h, exercises every helper,
# and DELIBERATELY does not link libclangCppInterOp or any LLVM library
# — if any header helper needs an out-of-line symbol, the link fails.
# See TypesSelfContainedTest.cpp for the full rationale.
#
# Not a runtime test — the built binary is not invoked. Success is
# purely that the link stage completes; ctest just runs it to confirm
# the process starts.

add_executable(CppInterOpTypesSelfContainedTest EXCLUDE_FROM_ALL
  TypesSelfContainedTest.cpp)

# The one and only include CppInterOpTypes.h needs is its own header
# directory. NOTHING from libclangCppInterOp or LLVM.
target_include_directories(CppInterOpTypesSelfContainedTest PRIVATE
  ${CMAKE_SOURCE_DIR}/include)

# Match the Release-ish flags real Dispatch consumers see so any
# NDEBUG-guarded regressions surface here too.
target_compile_definitions(CppInterOpTypesSelfContainedTest PRIVATE NDEBUG)

# Route the executable to the same bin dir the other cppinterop
# unittests land in, so ctest's default search path finds it. Follow
# the pattern from add_cppinterop_unittest (see unittests/CMakeLists.txt).
set_output_directory(CppInterOpTypesSelfContainedTest
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/../bin/$<CONFIG>/
  LIBRARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/../bin/$<CONFIG>/
)

# Register with ctest — the invocation just checks the process
# launches; the load-bearing signal is that the link succeeded at
# build time.
add_test(NAME cppinterop-CppInterOpTypesSelfContainedTest
         COMMAND CppInterOpTypesSelfContainedTest)

# Ensure the target is built as part of CppInterOpUnitTests (parallel
# to how add_cppinterop_unittest chains dependencies).
add_dependencies(CppInterOpUnitTests CppInterOpTypesSelfContainedTest)

set_target_properties(CppInterOpTypesSelfContainedTest PROPERTIES
  FOLDER "Tests")
