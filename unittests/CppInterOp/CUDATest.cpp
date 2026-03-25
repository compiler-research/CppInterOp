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

// demonstrate incremental CUDA compilation with CUB BlockReduce
// with a __device__ transform, multi-block atomicAdd
TEST_F(CUDATest, CUB_BlockReduceWithDeviceFunction) {
  if (!HasRuntime)
    return;

  Cpp::CreateInterpreter({}, {"--cuda"});

  EXPECT_EQ(0, Cpp::Declare(R"(
    #include <cub/block/block_reduce.cuh>

    constexpr int BLOCK_SIZE = 128;

    __device__ int transform(int x) { return x * x; }

    __global__ void sumOfSquaresKernel(const int* __restrict__ input,
                                       int* __restrict__ output, int N) {
      using BlockReduceT = cub::BlockReduce<int, BLOCK_SIZE>;
      __shared__ typename BlockReduceT::TempStorage temp_storage;

      int idx = blockIdx.x * BLOCK_SIZE + threadIdx.x;
      int val = (idx < N) ? transform(input[idx]) : 0;
      int block_sum = BlockReduceT(temp_storage).Sum(val);
      if (threadIdx.x == 0)
        atomicAdd(output, block_sum);
    }
  )" DFLT_FALSE))
      << "Failed to declare kernel";

  EXPECT_EQ(0, Cpp::Declare(R"(
    const int N = 256;
    int* d_in;
    int* d_out;
    cudaMallocManaged(&d_in, N * sizeof(int));
    cudaMallocManaged(&d_out, sizeof(int));
    for (int i = 0; i < N; i++) d_in[i] = i + 1; // 1..256
    *d_out = 0;
    int numBlocks = (N + BLOCK_SIZE - 1) / BLOCK_SIZE;
    sumOfSquaresKernel<<<numBlocks, BLOCK_SIZE>>>(d_in, d_out, N);
    cudaError_t launchErr = cudaGetLastError();
    cudaError_t syncErr = cudaDeviceSynchronize();
  )" DFLT_FALSE))
      << "Failed to launch kernel";

  bool err = false;
  EXPECT_EQ((int)Cpp::Evaluate("(int)launchErr", &err), 0);
  EXPECT_FALSE(err);
  EXPECT_EQ((int)Cpp::Evaluate("(int)syncErr", &err), 0);
  EXPECT_FALSE(err);

  // read result directly from managed memory
  intptr_t result = Cpp::Evaluate("*d_out", &err);
  EXPECT_FALSE(err);
  // sum of squares: 1^2 + 2^2 + ... + 256^2 = 5625216
  EXPECT_EQ((int)result, 5625216);

  Cpp::Declare("cudaFree(d_in); cudaFree(d_out);" DFLT_FALSE);
}
