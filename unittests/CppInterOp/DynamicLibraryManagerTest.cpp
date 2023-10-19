#include "gtest/gtest.h"

#include "clang/Interpreter/CppInterOp.h"

TEST(DynamicLibraryManagerTest, Sanity) {
  EXPECT_TRUE(Cpp::CreateInterpreter());
  EXPECT_TRUE(Cpp::LoadLibrary("TestSharedLib"));
}
