#include "clang/Interpreter/CppInterOp.h"

#include "llvm/ADT/SmallString.h"
#include "llvm/Support/Path.h"

#include <gmock/gmock.h>
#include "gtest/gtest.h"

using ::testing::StartsWith;

TEST(InterpreterTest, Version) {
  EXPECT_THAT(Cpp::GetVersion(), StartsWith("CppInterOp version"));
}

#ifdef NDEBUG
TEST(InterpreterTest, DISABLED_DebugFlag) {
#else
TEST(InterpreterTest, DebugFlag) {
#endif // NDEBUG
  Cpp::CreateInterpreter();
  EXPECT_FALSE(Cpp::IsDebugOutputEnabled());
  std::string cerrs;
  testing::internal::CaptureStderr();
  Cpp::Process("int a = 12;");
  cerrs = testing::internal::GetCapturedStderr();
  EXPECT_STREQ(cerrs.c_str(), "");
  Cpp::EnableDebugOutput();
  EXPECT_TRUE(Cpp::IsDebugOutputEnabled());
  testing::internal::CaptureStderr();
  Cpp::Process("int b = 12;");
  cerrs = testing::internal::GetCapturedStderr();
  EXPECT_STRNE(cerrs.c_str(), "");

  Cpp::EnableDebugOutput(false);
  EXPECT_FALSE(Cpp::IsDebugOutputEnabled());
  testing::internal::CaptureStderr();
  Cpp::Process("int c = 12;");
  cerrs = testing::internal::GetCapturedStderr();
  EXPECT_STREQ(cerrs.c_str(), "");
}

TEST(InterpreterTest, Evaluate) {
  //  EXPECT_TRUE(Cpp::Evaluate(I, "") == 0);
  //EXPECT_TRUE(Cpp::Evaluate(I, "__cplusplus;") == 201402);
  // Due to a deficiency in the clang-repl implementation to get the value we
  // always must omit the ;
  EXPECT_TRUE(Cpp::Evaluate("__cplusplus") == 201402);

  bool HadError;
  EXPECT_TRUE(Cpp::Evaluate("#error", &HadError) == (intptr_t)~0UL);
  EXPECT_TRUE(HadError);
  EXPECT_EQ(Cpp::Evaluate("int i = 11; ++i", &HadError), 12);
  EXPECT_FALSE(HadError) ;
}

#ifdef _WIN32
TEST(InterpreterTest, DISABLED_Process) {
#else
TEST(InterpreterTest, Process) {
#endif // _WIN32
  Cpp::CreateInterpreter();
  EXPECT_TRUE(Cpp::Process("") == 0);
  EXPECT_TRUE(Cpp::Process("int a = 12;") == 0);
  EXPECT_FALSE(Cpp::Process("error_here;") == 0);
  // Linker/JIT error.
  EXPECT_FALSE(Cpp::Process("int f(); int res = f();") == 0);
}

TEST(InterpreterTest, CreateInterpreter) {
  auto I = Cpp::CreateInterpreter();
  EXPECT_TRUE(I);
  // Check if the default standard is c++14

  Cpp::Declare("#if __cplusplus==201402L\n"
                   "int cpp14() { return 2014; }\n"
                   "#else\n"
                   "void cppUnknown() {}\n"
                   "#endif");
  EXPECT_TRUE(Cpp::GetNamed("cpp14"));
  EXPECT_FALSE(Cpp::GetNamed("cppUnknown"));

  I = Cpp::CreateInterpreter({"-std=c++17"});
  Cpp::Declare("#if __cplusplus==201703L\n"
                   "int cpp17() { return 2017; }\n"
                   "#else\n"
                   "void cppUnknown() {}\n"
                   "#endif");
  EXPECT_TRUE(Cpp::GetNamed("cpp17"));
  EXPECT_FALSE(Cpp::GetNamed("cppUnknown"));
}

#if defined(LLVM_BINARY_DIR) && !defined(_WIN32)
TEST(InterpreterTest, DetectResourceDir) {
#else
TEST(InterpreterTest, DISABLED_DetectResourceDir) {
#endif // LLVM_BINARY_DIR || !_WIN32
  Cpp::CreateInterpreter();
  EXPECT_STRNE(Cpp::DetectResourceDir().c_str(), Cpp::GetResourceDir());
  llvm::SmallString<256> Clang(LLVM_BINARY_DIR);
  llvm::sys::path::append(Clang, "bin", "clang");
  std::string DetectedPath = Cpp::DetectResourceDir(Clang.str().str().c_str());
  EXPECT_STREQ(DetectedPath.c_str(), Cpp::GetResourceDir());
}

#ifdef _WIN32
TEST(InterpreterTest, DISABLED_DetectSystemCompilerIncludePaths) {
#else
TEST(InterpreterTest, DetectSystemCompilerIncludePaths) {
#endif // _WIN32
  std::vector<std::string> includes;
  Cpp::DetectSystemCompilerIncludePaths(includes);
  EXPECT_FALSE(includes.empty());
}
