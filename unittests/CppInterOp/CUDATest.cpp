#include "Utils.h"

#include "CppInterOp/CppInterOp.h"

#include "clang/Basic/Version.h"
#include "llvm/Support/FileSystem.h"

#include "gtest/gtest.h"

#include <cstdint>
#include <string>
#include <vector>

using namespace TestUtils;

// CUDA test fixture that runs SDK and runtime checks once for the entire suite.
class CUDATest : public ::testing::Test {
protected:
  static inline bool HasSDK = false;
  static inline bool HasRuntime = false;

  static void SetUpTestSuite() {
#if defined(CPPINTEROP_USE_CLING) || defined(_WIN32)
    return; // FIXME: Enable for cling and Windows.
#endif
    auto *I = Cpp::CreateInterpreter({}, {"--cuda"});
    if (!I)
      return;
    if (Cpp::Declare("__global__ void test_func() {}"
                     "test_func<<<1,1>>>();" DFLT_FALSE) != 0)
      return;

    HasSDK = true;
    HasRuntime =
        !Cpp::Declare("int __cuda_err = (int)cudaGetLastError();" DFLT_FALSE);
    Cpp::DeleteInterpreter(I);
  }

};


TEST_F(CUDATest, Sanity) {
  if (!HasSDK)
    return;
  EXPECT_TRUE(Cpp::CreateInterpreter({}, {"--cuda"}));
}

// Test CUDA install path and GPU arch detection used by createClangInterpreter
#ifndef CPPINTEROP_USE_CLING
TEST_F(CUDATest, DetectCudaInstallAndArch) {
  if (!HasSDK)
    return;

  std::string path;
  std::vector<const char*> args;

  // no args: rely on auto-detect
  EXPECT_TRUE(compat::detectCudaInstallPath(args, path));
  EXPECT_TRUE(llvm::StringRef(path).starts_with("/usr/local/cuda"));

  // explicit path: returns cuda-12.3 if it exists, otherwise may auto-detect
  args = {"--cuda-path=/usr/local/cuda-12.3"};
  if (compat::detectCudaInstallPath(args, path)) {
    if (llvm::sys::fs::is_directory("/usr/local/cuda-12.3/include"))
      EXPECT_EQ(path, "/usr/local/cuda-12.3");
    else
      EXPECT_TRUE(llvm::StringRef(path).starts_with("/usr/local/cuda"));
  }

  // user provided path that doesn't exist should fail
  args = {"--cuda-path=/nonexistent"};
  EXPECT_FALSE(compat::detectCudaInstallPath(args, path));

  // GPU offload arch detection
  std::string arch;
  EXPECT_TRUE(compat::detectNVPTXArch(arch));
  EXPECT_TRUE(llvm::StringRef(arch).starts_with("sm_"));

  EXPECT_TRUE(Cpp::CreateInterpreter(
      {}, {"--cuda", "--cuda-path=/usr/local/cuda-12.3",
           "--offload-arch=sm_70"}));
}
#endif

TEST_F(CUDATest, CUDAH) {
  if (!HasSDK)
    return;

  Cpp::CreateInterpreter({}, {"--cuda"});
  bool success = !Cpp::Declare("#include <cuda_runtime.h>" DFLT_FALSE);
  EXPECT_TRUE(success);
}

TEST_F(CUDATest, CUDARuntime) {
  if (!HasRuntime)
    return;
  EXPECT_TRUE(HasRuntime);
}

TEST_F(CUDATest, Interpreter_GetLanguageCUDA) {
  if (!HasRuntime)
    return;

  auto* I = Cpp::CreateInterpreter({}, {"--cuda"});
  if (!I)
    GTEST_SKIP() << "CUDA interpreter creation failed";
  EXPECT_EQ(Cpp::GetLanguage(nullptr), Cpp::InterpreterLanguage::CUDA);
}
