#include "Utils.h"

#include "cling/Interpreter/Interpreter.h"

#include "clang/AST/ASTContext.h"
#include "clang/Interpreter/InterOp.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Sema/Sema.h"

#include "gtest/gtest.h"

using namespace TestUtils;
using namespace llvm;
using namespace clang;
using namespace cling;

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
  EXPECT_EQ(InterOp::GetTypeAsString(QT1.getAsOpaquePtr()),
          "N::C");
  EXPECT_EQ(InterOp::GetTypeAsString(QT2.getAsOpaquePtr()),
          "N::S");
  EXPECT_EQ(InterOp::GetTypeAsString(QT3.getAsOpaquePtr()), "int");
}

TEST(TypeReflectionTest, GetSizeOfType) {
  std::vector<Decl *> Decls;
  std::string code =  R"(
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

  EXPECT_EQ(InterOp::GetSizeOfType(S, InterOp::GetVariableType(Decls[1])), 1);
  EXPECT_EQ(InterOp::GetSizeOfType(S, InterOp::GetVariableType(Decls[2])), 4);
  EXPECT_EQ(InterOp::GetSizeOfType(S, InterOp::GetVariableType(Decls[3])), 8);
  EXPECT_EQ(InterOp::GetSizeOfType(S, InterOp::GetVariableType(Decls[4])), 16);
}

TEST(TypeReflectionTest, GetCanonicalType) {
  std::vector<Decl *> Decls;
  std::string code =  R"(
    typedef int I;
    typedef double D;

    I n;
    D d;
    )";

  GetAllTopLevelDecls(code, Decls);

  auto D2 = InterOp::GetVariableType(Decls[2]);
  auto D3 = InterOp::GetVariableType(Decls[3]);

  EXPECT_EQ(InterOp::GetTypeAsString(D2), "I");
  EXPECT_EQ(InterOp::GetTypeAsString(InterOp::GetCanonicalType(D2)), "int");
  EXPECT_EQ(InterOp::GetTypeAsString(D3), "D");
  EXPECT_EQ(InterOp::GetTypeAsString(InterOp::GetCanonicalType(D3)), "double");
}

TEST(TypeReflectionTest, GetType) {
  Interp.reset();
  Interp = createInterpreter();
  Sema *S = &Interp->getCI()->getSema();

  std::string code =  R"(
    #include <string>
    )";
  
  Interp->declare(code);

  EXPECT_EQ(InterOp::GetTypeAsString(InterOp::GetType(S, "int")), "int");
  EXPECT_EQ(InterOp::GetTypeAsString(InterOp::GetType(S, "double")), "double");
  EXPECT_EQ(InterOp::GetTypeAsString(InterOp::GetType(S, "std::string")), "std::string");
}

TEST(TypeReflectionTest, GetComplexType) {
  Interp.reset();
  Interp = createInterpreter();
  Sema *S = &Interp->getCI()->getSema();

  auto get_complex_type_as_string = [&](const std::string &element_type) {
    auto ElementQT = InterOp::GetType(S, element_type);
    auto ComplexQT = InterOp::GetComplexType(S, ElementQT);
    return InterOp::GetTypeAsString(InterOp::GetCanonicalType(ComplexQT));
  };

  EXPECT_EQ(get_complex_type_as_string("int"), "_Complex int");
  EXPECT_EQ(get_complex_type_as_string("float"), "_Complex float");
  EXPECT_EQ(get_complex_type_as_string("double"), "_Complex double");
}

TEST(TypeReflectionTest, GetTypeFromScope) {
  std::vector<Decl *> Decls;

  std::string code =  R"(
    class C {};
    struct S {};
    )";
  
  GetAllTopLevelDecls(code, Decls);

  EXPECT_EQ(InterOp::GetTypeAsString(InterOp::GetTypeFromScope(Decls[0])), "class C");
  EXPECT_EQ(InterOp::GetTypeAsString(InterOp::GetTypeFromScope(Decls[1])), "struct S");
}

TEST(TypeReflectionTest, DISABLED_IsSubType) {
  std::vector<Decl *> Decls;

  std::string code = R"(
      class A {};
      class B : A {};
      class C {};

      A a;
      B b;
      C c;
    )";

  GetAllTopLevelDecls(code, Decls);
  
  InterOp::TCppType_t type_A = InterOp::GetVariableType(Decls[3]);
  InterOp::TCppType_t type_B = InterOp::GetVariableType(Decls[4]);
  InterOp::TCppType_t type_C = InterOp::GetVariableType(Decls[5]);

  // EXPECT_TRUE(InterOp::IsSubType(type_B, type_A));
  // EXPECT_FALSE(InterOp::IsSubType(type_A, type_B));
  // EXPECT_FALSE(InterOp::IsSubType(type_C, type_A));
}

