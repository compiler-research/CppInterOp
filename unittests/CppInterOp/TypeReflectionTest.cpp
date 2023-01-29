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

  EXPECT_EQ(Cpp::GetSizeOfType(S, Cpp::GetVariableType(Decls[1])), 1);
  EXPECT_EQ(Cpp::GetSizeOfType(S, Cpp::GetVariableType(Decls[2])), 4);
  EXPECT_EQ(Cpp::GetSizeOfType(S, Cpp::GetVariableType(Decls[3])), 8);
  EXPECT_EQ(Cpp::GetSizeOfType(S, Cpp::GetVariableType(Decls[4])), 16);
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

  auto D2 = Cpp::GetVariableType(Decls[2]);
  auto D3 = Cpp::GetVariableType(Decls[3]);

  EXPECT_EQ(Cpp::GetTypeAsString(D2), "I");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetCanonicalType(D2)), "int");
  EXPECT_EQ(Cpp::GetTypeAsString(D3), "D");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetCanonicalType(D3)), "double");
}

TEST(TypeReflectionTest, GetType) {
  Interp.reset();
  Interp = createInterpreter();
  Sema *S = &Interp->getCI()->getSema();

  std::string code = R"(
    #include <string>
    )";

  Interp->declare(code);

  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetType(S, "int")), "int");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetType(S, "double")), "double");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetType(S, "std::string")),
            "std::string");
}

TEST(TypeReflectionTest, GetComplexType) {
  Interp.reset();
  Interp = createInterpreter();
  Sema *S = &Interp->getCI()->getSema();

  auto get_complex_type_as_string = [&](const std::string &element_type) {
    auto ElementQT = Cpp::GetType(S, element_type);
    auto ComplexQT = Cpp::GetComplexType(S, ElementQT);
    return Cpp::GetTypeAsString(Cpp::GetCanonicalType(ComplexQT));
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

  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetTypeFromScope(Decls[0])), "class C");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetTypeFromScope(Decls[1])), "struct S");
}
