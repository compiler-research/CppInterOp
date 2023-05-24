#include "clang/Interpreter/InterOp.h"

#include "gtest/gtest.h"

TEST(InterpreterTest, DebugFlag) {
  EXPECT_FALSE(InterOp::IsDebugOutputEnabled());
  std::string cerrs;
  testing::internal::CaptureStderr();
  InterOp::Process("int a = 12;");
  cerrs = testing::internal::GetCapturedStderr();
  EXPECT_STREQ(cerrs.c_str(), "");
  InterOp::EnableDebugOutput();
  EXPECT_TRUE(InterOp::IsDebugOutputEnabled());
  testing::internal::CaptureStderr();
  InterOp::Process("int b = 12;");
  cerrs = testing::internal::GetCapturedStderr();
  EXPECT_STRNE(cerrs.c_str(), "");

  InterOp::EnableDebugOutput(false);
  EXPECT_FALSE(InterOp::IsDebugOutputEnabled());
  testing::internal::CaptureStderr();
  InterOp::Process("int c = 12;");
  cerrs = testing::internal::GetCapturedStderr();
  EXPECT_STREQ(cerrs.c_str(), "");
}

TEST(InterpreterTest, Evaluate) {
  //  EXPECT_TRUE(InterOp::Evaluate(I, "") == 0);
  //EXPECT_TRUE(InterOp::Evaluate(I, "__cplusplus;") == 201402);
  // Due to a deficiency in the clang-repl implementation to get the value we
  // always must omit the ;
  EXPECT_TRUE(InterOp::Evaluate("__cplusplus") == 201402);

  bool HadError;
  EXPECT_TRUE(InterOp::Evaluate("#error", &HadError) == (intptr_t)~0UL);
  EXPECT_TRUE(HadError);
  EXPECT_EQ(InterOp::Evaluate("int i = 11; ++i", &HadError), 12);
  EXPECT_FALSE(HadError) ;
}

TEST(InterpreterTest, Process) {
  InterOp::CreateInterpreter();
  EXPECT_TRUE(InterOp::Process("") == 0);
  EXPECT_TRUE(InterOp::Process("int a = 12;") == 0);
  EXPECT_FALSE(InterOp::Process("error_here;") == 0);
  // Linker/JIT error.
  EXPECT_FALSE(InterOp::Process("int f(); int res = f();") == 0);
}

TEST(InterpreterTest, CreateInterpreter) {
  auto I = InterOp::CreateInterpreter();
  EXPECT_TRUE(I);
  // Check if the default standard is c++14

  InterOp::Declare("#if __cplusplus==201402L\n"
                   "int cpp14() { return 2014; }\n"
                   "#else\n"
                   "void cppUnknown() {}\n"
                   "#endif");
  EXPECT_TRUE(InterOp::GetNamed("cpp14"));
  EXPECT_FALSE(InterOp::GetNamed("cppUnknown"));

  I = InterOp::CreateInterpreter({"-std=c++17"});
  InterOp::Declare("#if __cplusplus==201703L\n"
                   "int cpp17() { return 2017; }\n"
                   "#else\n"
                   "void cppUnknown() {}\n"
                   "#endif");
  EXPECT_TRUE(InterOp::GetNamed("cpp17"));
  EXPECT_FALSE(InterOp::GetNamed("cppUnknown"));
}
