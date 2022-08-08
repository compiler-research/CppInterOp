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

