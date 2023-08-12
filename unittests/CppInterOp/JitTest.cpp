#include "Utils.h"

#include "clang/Interpreter/CppInterOp.h"

#include "gtest/gtest.h"

using namespace TestUtils;

static int printf_jit(const char* format, ...) {
  llvm::errs() << "printf_jit called!\n";
  return 0;
}

TEST(JitTest, InsertOrReplaceJitSymbol) {
  std::vector<Decl*> Decls;
  std::string code = R"(
    extern "C" int printf(const char*,...);
    )";

  GetAllTopLevelDecls(code, Decls);

  EXPECT_FALSE(Cpp::InsertOrReplaceJitSymbol("printf", (uint64_t)&printf_jit));

  testing::internal::CaptureStderr();
  Cpp::Process("printf(\"Blah\");");
  std::string cerrs = testing::internal::GetCapturedStderr();
  EXPECT_STREQ(cerrs.c_str(), "printf_jit called!\n");

  EXPECT_TRUE(
      Cpp::InsertOrReplaceJitSymbol("non_existent", (uint64_t)&printf_jit));
  EXPECT_TRUE(Cpp::InsertOrReplaceJitSymbol("non_existent", 0));
}
