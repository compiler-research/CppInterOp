#if 0
#include "Utils.h"

#include "clang/Basic/Version.h"
#include "clang/Interpreter/CppInterOp.h"

#include "gtest/gtest.h"

using namespace TestUtils;

static bool HasCudaSDK() {
  auto supportsCudaSDK = []() {
#if CLANG_VERSION_MAJOR < 16
    // FIXME: Enable this for cling.
    return false;
#endif // CLANG_VERSION_MAJOR < 16
    Cpp::CreateInterpreter({}, {"--cuda"});
    return Cpp::Declare("__global__ void test_func() {}"
                        "test_func<<<1,1>>>();") == 0;
  };
  static bool hasCuda = supportsCudaSDK();
  return hasCuda;
}

static bool HasCudaRuntime() {
  auto supportsCuda = []() {
#if CLANG_VERSION_MAJOR < 16
    // FIXME: Enable this for cling.
    return false;
#endif //CLANG_VERSION_MAJOR < 16
    if (!HasCudaSDK())
      return false;

    Cpp::CreateInterpreter({}, {"--cuda"});
    if (Cpp::Declare("__global__ void test_func() {}"
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
  if (!HasCudaSDK())
    return;

  Cpp::CreateInterpreter({}, {"--cuda"});
  bool success = !Cpp::Declare("#include <cuda.h>");
  EXPECT_TRUE(success);
}

TEST(CUDATest, CUDARuntime) {
  if (!HasCudaSDK())
    return;

  EXPECT_TRUE(HasCudaRuntime());
}
#endif
