#include "clang/Interpreter/InterOp.h"

#include "gtest/gtest.h"

TEST(InterpreterTest, Process) {
  auto I = InterOp::CreateInterpreter();
  EXPECT_TRUE(InterOp::Process(I, "") == 0);
  EXPECT_TRUE(InterOp::Process(I, "int a = 12;") == 0);
  EXPECT_FALSE(InterOp::Process(I, "error_here;") == 0);
  // Linker/JIT error.
  EXPECT_FALSE(InterOp::Process(I, "int f(); int res = f();") == 0);
}

TEST(InterpreterTest, CreateInterpreter) {
  auto I = InterOp::CreateInterpreter();
  EXPECT_TRUE(I);
  // Check if the default standard is c++14

  InterOp::Declare(I, "#if __cplusplus==201402L\n"
                      "int cpp14() { return 2014; }\n"
                      "#else\n"
                      "void cppUnknown() {}\n"
                      "#endif");
  EXPECT_TRUE(InterOp::GetNamed(InterOp::GetSema(I), "cpp14"));
  EXPECT_FALSE(InterOp::GetNamed(InterOp::GetSema(I), "cppUnknown"));

  I = InterOp::CreateInterpreter({"-std=c++17"});
  InterOp::Declare(I, "#if __cplusplus==201703L\n"
                      "int cpp17() { return 2017; }\n"
                      "#else\n"
                      "void cppUnknown() {}\n"
                      "#endif");
  EXPECT_TRUE(InterOp::GetNamed(InterOp::GetSema(I), "cpp17"));
  EXPECT_FALSE(InterOp::GetNamed(InterOp::GetSema(I), "cppUnknown"));
}
