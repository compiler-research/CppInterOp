#include "clang/Interpreter/CppInterOp.h"

#include "gtest/gtest.h"

TEST(InterpreterTest, Process) {
  auto I = Cpp::CreateInterpreter();
  EXPECT_TRUE(Cpp::Process(I, "") == 0);
  EXPECT_TRUE(Cpp::Process(I, "int a = 12;") == 0);
  EXPECT_FALSE(Cpp::Process(I, "error_here;") == 0);
  // Linker/JIT error.
  EXPECT_FALSE(Cpp::Process(I, "int f(); int res = f();") == 0);
}

TEST(InterpreterTest, CreateInterpreter) {
  auto I = Cpp::CreateInterpreter();
  EXPECT_TRUE(I);
  // Check if the default standard is c++14

  Cpp::Declare(I, "#if __cplusplus==201402L\n"
                  "int cpp14() { return 2014; }\n"
                  "#else\n"
                  "void cppUnknown() {}\n"
                  "#endif");
  EXPECT_TRUE(Cpp::GetNamed(Cpp::GetSema(I), "cpp14"));
  EXPECT_FALSE(Cpp::GetNamed(Cpp::GetSema(I), "cppUnknown"));

  I = Cpp::CreateInterpreter({"-std=c++17"});
  Cpp::Declare(I, "#if __cplusplus==201703L\n"
                  "int cpp17() { return 2017; }\n"
                  "#else\n"
                  "void cppUnknown() {}\n"
                  "#endif");
  EXPECT_TRUE(Cpp::GetNamed(Cpp::GetSema(I), "cpp17"));
  EXPECT_FALSE(Cpp::GetNamed(Cpp::GetSema(I), "cppUnknown"));
}
