#include "Utils.h"

#include "clang/Interpreter/CppInterOp.h"
#include "clang/Basic/Version.h"

#include "clang-c/CXCppInterOp.h"

#include "llvm/ADT/SmallString.h"
#include "llvm/Support/Path.h"

#include <gmock/gmock.h>
#include "gtest/gtest.h"

#include <algorithm>

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
  if (llvm::sys::RunningOnValgrind())
    GTEST_SKIP() << "XFAIL due to Valgrind report";
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

TEST(InterpreterTest, Process) {
  if (llvm::sys::RunningOnValgrind())
    GTEST_SKIP() << "XFAIL due to Valgrind report";
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

  // C API
  const char* args[] = {"-std=c++14"};
  auto CXI = clang_createInterpreter(args, 1);
  EXPECT_TRUE(CXI);
  clang_interpreter_dispose(CXI);
}

#ifdef LLVM_BINARY_DIR
TEST(InterpreterTest, DetectResourceDir) {
#else
TEST(InterpreterTest, DISABLED_DetectResourceDir) {
#endif // LLVM_BINARY_DIR
  Cpp::CreateInterpreter();
  EXPECT_STRNE(Cpp::DetectResourceDir().c_str(), Cpp::GetResourceDir());
  llvm::SmallString<256> Clang(LLVM_BINARY_DIR);
  llvm::sys::path::append(Clang, "bin", "clang");
  std::string DetectedPath = Cpp::DetectResourceDir(Clang.str().str().c_str());
  EXPECT_STREQ(DetectedPath.c_str(), Cpp::GetResourceDir());
}

TEST(InterpreterTest, DetectSystemCompilerIncludePaths) {
  std::vector<std::string> includes;
  Cpp::DetectSystemCompilerIncludePaths(includes);
  EXPECT_FALSE(includes.empty());
}

TEST(InterpreterTest, CodeCompletion) {
#if CLANG_VERSION_MAJOR >= 18 || defined(USE_CLING)
  Cpp::CreateInterpreter();
  std::vector<std::string> cc;
  Cpp::Declare("int foo = 12;");
  Cpp::CodeComplete(cc, "f", 1, 2);
  // We check only for 'float' and 'foo', because they
  // must be present in the result. Other hints may appear
  // there, depending on the implementation, but these two
  // are required to say that the test is working.
  size_t cnt = 0;
  for (auto& r : cc)
    if (r == "float" || r == "foo")
      cnt++;
  EXPECT_EQ(2U, cnt); // float and foo
#else
  GTEST_SKIP();
#endif
}
