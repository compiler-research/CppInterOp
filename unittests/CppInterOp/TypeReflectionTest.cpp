#include "Utils.h"

#include "clang/Interpreter/CppInterOp.h"

#include "gtest/gtest.h"

TEST(TypeReflectionTest, GetTypeAsString) {
  std::vector<Decl *> Decls;
  std::string code = R"(
    namespace N {
    class C {};
    struct S {};
    }

    N::C c;

    N::S s;

    int i;
  )";

  GetAllTopLevelDecls(code, Decls);
  QualType QT1 = llvm::dyn_cast<VarDecl>(Decls[1])->getType();
  QualType QT2 = llvm::dyn_cast<VarDecl>(Decls[2])->getType();
  QualType QT3 = llvm::dyn_cast<VarDecl>(Decls[3])->getType();
  EXPECT_EQ(Cpp::GetTypeAsString(QT1.getAsOpaquePtr()),
          "N::C");
  EXPECT_EQ(Cpp::GetTypeAsString(QT2.getAsOpaquePtr()),
          "N::S");
  EXPECT_EQ(Cpp::GetTypeAsString(QT3.getAsOpaquePtr()), "int");
}

TEST(TypeReflectionTest, IsEnumType) {
  std::vector<Decl *> Decls, SubDecls0, SubDecls1;
  std::string code = R"(
    enum class E {
      a,
      b
    };

    enum F {
      c,
      d
    };

    E e;
    F f;

    auto g = E::a;
    auto h = c;
    )";

  GetAllTopLevelDecls(code, Decls);

  EXPECT_TRUE(Cpp::IsEnumType(Cpp::GetVariableType(Decls[2])));
  EXPECT_TRUE(Cpp::IsEnumType(Cpp::GetVariableType(Decls[3])));
  EXPECT_TRUE(Cpp::IsEnumType(Cpp::GetVariableType(Decls[4])));
  EXPECT_TRUE(Cpp::IsEnumType(Cpp::GetVariableType(Decls[5])));
}

TEST(TypeReflectionTest, GetSizeOfType) {
  std::vector<Decl *> Decls, SubDecls0, SubDecls1;
  std::string code = R"(
    struct S {
      int a;
      double b;
    };

    char ch;
    int n;
    double d;
    S s;
    )";

  GetAllTopLevelDecls(code, Decls);
  Sema *S = &Interp->getCI()->getSema();

  EXPECT_EQ(Cpp::GetSizeOfType(S, Cpp::GetVariableType(Decls[1])), 1);
  EXPECT_EQ(Cpp::GetSizeOfType(S, Cpp::GetVariableType(Decls[2])), 4);
  EXPECT_EQ(Cpp::GetSizeOfType(S, Cpp::GetVariableType(Decls[3])), 8);
  EXPECT_EQ(Cpp::GetSizeOfType(S, Cpp::GetVariableType(Decls[4])), 16);
}
