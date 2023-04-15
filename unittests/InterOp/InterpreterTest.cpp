#include "clang/Interpreter/InterOp.h"

#include "gtest/gtest.h"

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
