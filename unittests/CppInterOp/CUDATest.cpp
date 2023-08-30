#include "Utils.h"

#include "clang/Basic/Version.h"
#include "clang/Interpreter/CppInterOp.h"

#include "gtest/gtest.h"

using namespace TestUtils;

static bool HasCuda() {
  auto supportsCuda = []() {
#if CLANG_VERSION_MAJOR < 16
    // FIXME: Enable this for cling.
    return false;
#endif //CLANG_VERSION_MAJOR < 16
    Cpp::CreateInterpreter({}, {"--cuda"});
    if (Cpp::Process("__global__ void test_func() {}"
                     "test_func<<<1,1>>>();"))
      return false;
    intptr_t result = Cpp::Evaluate("(bool)cudaGetLastError()");
    return !(bool)result;
  };
  static bool hasCuda = supportsCuda();
  return hasCuda;
}

#if CLANG_VERSION_MAJOR < 16
TEST(DISABLED_CUDATest, Sanity) {
#else
TEST(CUDATest, Sanity) {
#endif // CLANG_VERSION_MAJOR < 16
  EXPECT_TRUE(Cpp::CreateInterpreter({}, {"--cuda"}));
}

TEST(CUDATest, CUDAH) {
  if (!HasCuda())
    return;

  Cpp::CreateInterpreter({}, {"--cuda"});
  bool success = !Cpp::Declare("#include <cuda.h>");
  EXPECT_TRUE(success);
}
