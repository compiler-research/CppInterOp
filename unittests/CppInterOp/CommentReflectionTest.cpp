#include "Utils.h"

#include "CppInterOp/CppInterOp.h"
#include "gtest/gtest.h"

using namespace TestUtils;
using namespace llvm;
using namespace clang;

TYPED_TEST(CppInterOpTest, CommentReflectionDoxygenBlockAndLine) {
  std::vector<Decl*> Decls;
  std::string code = R"(
    /** Adds two integers.
     *  \param x first
     *  \param y second
     *  \return sum
     */
    int add(int x, int y) { return x + y; }
    /// A simple struct.
    struct S { int a; };
  )";

  GetAllTopLevelDecls(code, Decls);
  ASSERT_GE(Decls.size(), 2U);

  const std::string addDoc = Cpp::GetDoxygenComment(Decls[0], /*strip=*/true);
  EXPECT_NE(addDoc.find("Adds two integers."), std::string::npos);
  EXPECT_NE(addDoc.find("\\param x first"), std::string::npos);
  EXPECT_NE(addDoc.find("\\return sum"), std::string::npos);

  const std::string addDocRaw =
      Cpp::GetDoxygenComment(Decls[0], /*strip=*/false);
  EXPECT_NE(addDocRaw.find("/**"), std::string::npos);

  const std::string sDoc = Cpp::GetDoxygenComment(Decls[1], /*strip=*/true);
  EXPECT_EQ(sDoc, "A simple struct.");

  auto* I = clang_createInterpreterFromRawPtr(Cpp::GetInterpreter());
  CXScope S = make_scope(Decls[1], I);
  CXString Doc = clang_getDoxygenComment(S, /*strip_comment_markers=*/true);
  EXPECT_STREQ(get_c_string(Doc), sDoc.c_str());
  dispose_string(Doc);
  clang_Interpreter_takeInterpreterAsPtr(I);
  clang_Interpreter_dispose(I);
}

