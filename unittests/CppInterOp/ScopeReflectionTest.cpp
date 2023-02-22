#include "Utils.h"

#include "cling/Interpreter/Interpreter.h"

#include "clang/AST/ASTContext.h"
#include "clang/Interpreter/CppInterOp.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Sema/Sema.h"

#include "gtest/gtest.h"

using namespace TestUtils;
using namespace llvm;
using namespace clang;
using namespace cling;

// Check that the CharInfo table has been constructed reasonably.
TEST(ScopeReflectionTest, IsNamespace) {
  std::vector<Decl*> Decls;
  GetAllTopLevelDecls("namespace N {} class C{}; int I;", Decls);
  EXPECT_TRUE(Cpp::IsNamespace(Decls[0]));
  EXPECT_FALSE(Cpp::IsNamespace(Decls[1]));
  EXPECT_FALSE(Cpp::IsNamespace(Decls[2]));
}

TEST(ScopeReflectionTest, IsComplete) {
  std::vector<Decl*> Decls;
  GetAllTopLevelDecls(
      "namespace N {} class C{}; int I; struct S; enum E : int; union U{};",
      Decls);
  EXPECT_TRUE(Cpp::IsComplete(Decls[0]));
  EXPECT_TRUE(Cpp::IsComplete(Decls[1]));
  EXPECT_TRUE(Cpp::IsComplete(Decls[2]));
  EXPECT_FALSE(Cpp::IsComplete(Decls[3]));
  EXPECT_FALSE(Cpp::IsComplete(Decls[4]));
  EXPECT_TRUE(Cpp::IsComplete(Decls[5]));
}

TEST(ScopeReflectionTest, SizeOf) {
  std::vector<Decl*> Decls;
  std::string code = R"(namespace N {} class C{}; int I; struct S;
                        enum E : int; union U{}; class Size4{int i;};
                        struct Size16 {short a; double b;};
                       )";
  GetAllTopLevelDecls(code, Decls);
  EXPECT_EQ(Cpp::SizeOf(Decls[0]), (size_t)0);
  EXPECT_EQ(Cpp::SizeOf(Decls[1]), (size_t)1);
  EXPECT_EQ(Cpp::SizeOf(Decls[2]), (size_t)0);
  EXPECT_EQ(Cpp::SizeOf(Decls[3]), (size_t)0);
  EXPECT_EQ(Cpp::SizeOf(Decls[4]), (size_t)0);
  EXPECT_EQ(Cpp::SizeOf(Decls[5]), (size_t)1);
  EXPECT_EQ(Cpp::SizeOf(Decls[6]), (size_t)4);
  EXPECT_EQ(Cpp::SizeOf(Decls[7]), (size_t)16);
}

TEST(ScopeReflectionTest, IsBuiltin) {
  // static std::set<std::string> g_builtins =
  // {"bool", "char", "signed char", "unsigned char", "wchar_t", "short", "unsigned short",
  //  "int", "unsigned int", "long", "unsigned long", "long long", "unsigned long long",
  //  "float", "double", "long double", "void"}

  Interp.reset();
  Interp = createInterpreter();
  ASTContext &C = Interp->getCI()->getASTContext();
  EXPECT_TRUE(Cpp::IsBuiltin(C.BoolTy.getAsOpaquePtr()));
  EXPECT_TRUE(Cpp::IsBuiltin(C.CharTy.getAsOpaquePtr()));
  EXPECT_TRUE(Cpp::IsBuiltin(C.SignedCharTy.getAsOpaquePtr()));
  EXPECT_TRUE(Cpp::IsBuiltin(C.VoidTy.getAsOpaquePtr()));
  // ...

  // complex
  EXPECT_TRUE(Cpp::IsBuiltin(C.getComplexType(C.FloatTy).getAsOpaquePtr()));
  EXPECT_TRUE(Cpp::IsBuiltin(C.getComplexType(C.DoubleTy).getAsOpaquePtr()));
  EXPECT_TRUE(Cpp::IsBuiltin(C.getComplexType(C.LongDoubleTy).getAsOpaquePtr()));
  EXPECT_TRUE(Cpp::IsBuiltin(C.getComplexType(C.Float128Ty).getAsOpaquePtr()));

  // std::complex
  std::vector<Decl*> Decls;
  Interp->declare("#include <complex>");
  Sema &S = Interp->getCI()->getSema();
  auto lookup = S.getStdNamespace()->lookup(&C.Idents.get("complex"));
  auto *CTD = cast<ClassTemplateDecl>(lookup.front());
  for (ClassTemplateSpecializationDecl *CTSD : CTD->specializations())
    EXPECT_TRUE(Cpp::IsBuiltin(C.getTypeDeclType(CTSD).getAsOpaquePtr()));
}

TEST(ScopeReflectionTest, IsTemplate) {
  std::vector<Decl *> Decls;
  std::string code = R"(template<typename T>
                        class A{};

                        class C{
                          template<typename T>
                          int func(T t) {
                            return 0;
                          }
                        };

                        template<typename T>
                        T f(T t) {
                          return t;
                        }

                        void g() {} )";

  GetAllTopLevelDecls(code, Decls);
  EXPECT_TRUE(Cpp::IsTemplate(Decls[0]));
  EXPECT_FALSE(Cpp::IsTemplate(Decls[1]));
  EXPECT_TRUE(Cpp::IsTemplate(Decls[2]));
  EXPECT_FALSE(Cpp::IsTemplate(Decls[3]));
}

TEST(ScopeReflectionTest, IsTemplateSpecialization) {
  std::vector<Decl *> Decls;
  std::string code = R"(
    template<typename T>
    class A{};

    A<int> a;
    )";

  GetAllTopLevelDecls(code, Decls);
  EXPECT_FALSE(Cpp::IsTemplateSpecialization(Decls[0]));
  EXPECT_FALSE(Cpp::IsTemplateSpecialization(Decls[1]));
  EXPECT_TRUE(Cpp::IsTemplateSpecialization(
          Cpp::GetScopeFromType(Cpp::GetVariableType(Decls[1]))));
}

TEST(ScopeReflectionTest, IsAbstract) {
  std::vector<Decl *> Decls;
  std::string code = R"(
    class A {};

    class B {
      virtual int f() = 0;
    };
  )";

  GetAllTopLevelDecls(code, Decls);
  EXPECT_FALSE(Cpp::IsAbstract(Decls[0]));
  EXPECT_TRUE(Cpp::IsAbstract(Decls[1]));
}

TEST(ScopeReflectionTest, IsVariable) {
  std::vector<Decl *> Decls;
  std::string code = R"(
    int i;

    class C {
    public:
      int a;
      static int b;
    };
  )";

  GetAllTopLevelDecls(code, Decls);
  EXPECT_TRUE(Cpp::IsVariable(Decls[0]));
  EXPECT_FALSE(Cpp::IsVariable(Decls[1]));

  std::vector<Decl *> SubDecls;
  GetAllSubDecls(Decls[1], SubDecls);
  EXPECT_FALSE(Cpp::IsVariable(SubDecls[0]));
  EXPECT_FALSE(Cpp::IsVariable(SubDecls[1]));
  EXPECT_FALSE(Cpp::IsVariable(SubDecls[2]));
  EXPECT_TRUE(Cpp::IsVariable(SubDecls[3]));
}

TEST(ScopeReflectionTest, GetName) {
  std::vector<Decl*> Decls;
  std::string code = R"(namespace N {} class C{}; int I; struct S;
                        enum E : int; union U{}; class Size4{int i;};
                        struct Size16 {short a; double b;};
                       )";
  GetAllTopLevelDecls(code, Decls);
  EXPECT_EQ(Cpp::GetName(Decls[0]), "N");
  EXPECT_EQ(Cpp::GetName(Decls[1]), "C");
  EXPECT_EQ(Cpp::GetName(Decls[2]), "I");
  EXPECT_EQ(Cpp::GetName(Decls[3]), "S");
  EXPECT_EQ(Cpp::GetName(Decls[4]), "E");
  EXPECT_EQ(Cpp::GetName(Decls[5]), "U");
  EXPECT_EQ(Cpp::GetName(Decls[6]), "Size4");
  EXPECT_EQ(Cpp::GetName(Decls[7]), "Size16");
}

TEST(ScopeReflectionTest, GetQualifiedName) {
  std::vector<Decl*> Decls;
  std::string code = R"(namespace N {
                        class C {
                          int i;
                          enum E { A, B };
                        };
                        }
                       )";
  GetAllTopLevelDecls(code, Decls);
  GetAllSubDecls(Decls[0], Decls);
  GetAllSubDecls(Decls[1], Decls);

  EXPECT_EQ(Cpp::GetQualifiedName(0), "<unnamed>");
  EXPECT_EQ(Cpp::GetQualifiedName(Decls[0]), "N");
  EXPECT_EQ(Cpp::GetQualifiedName(Decls[1]), "N::C");
  EXPECT_EQ(Cpp::GetQualifiedName(Decls[3]), "N::C::i");
  EXPECT_EQ(Cpp::GetQualifiedName(Decls[4]), "N::C::E");
}

TEST(ScopeReflectionTest, GetUsingNamespaces) {
  std::vector<Decl *> Decls;
  std::string code = R"(
    namespace abc {

    class C {};

    }
    using namespace std;
    using namespace abc;

    using I = int;
  )";

  GetAllTopLevelDecls(code, Decls);
  std::vector<void *> usingNamespaces;
  usingNamespaces = Cpp::GetUsingNamespaces(
          Decls[0]->getASTContext().getTranslationUnitDecl());

  EXPECT_EQ(Cpp::GetName(usingNamespaces[0]), "runtime");
  EXPECT_EQ(Cpp::GetName(usingNamespaces[1]), "std");
  EXPECT_EQ(Cpp::GetName(usingNamespaces[2]), "abc");
}

TEST(ScopeReflectionTest, GetGlobalScope) {
  Interp = createInterpreter();
  Sema *S = &Interp->getCI()->getSema();

  EXPECT_EQ(Cpp::GetQualifiedName(Cpp::GetGlobalScope(S)), "");
  EXPECT_EQ(Cpp::GetName(Cpp::GetGlobalScope(S)), "");
}

TEST(ScopeReflectionTest, GetScope) {
  std::vector<Decl *> Decls;
  std::string code = R"(namespace N {
                        class C {
                          int i;
                          enum E { A, B };
                        };
                        }
                       )";

  Interp = createInterpreter();
  Interp->declare(code);
  Sema *S = &Interp->getCI()->getSema();
  Cpp::TCppScope_t tu = Cpp::GetScope(S, "", 0);
  Cpp::TCppScope_t ns_N = Cpp::GetScope(S, "N", 0);
  Cpp::TCppScope_t cl_C = Cpp::GetScope(S, "C", ns_N);

  EXPECT_EQ(Cpp::GetQualifiedName(tu), "");
  EXPECT_EQ(Cpp::GetQualifiedName(ns_N), "N");
  EXPECT_EQ(Cpp::GetQualifiedName(cl_C), "N::C");
}

TEST(ScopeReflectionTest, GetScopefromCompleteName) {
  std::vector<Decl *> Decls;
  std::string code = R"(namespace N1 {
                        namespace N2 {
                          class C {
                            struct S {};
                          };
                        }
                        }
                       )";

  Interp = createInterpreter();
  Interp->declare(code);
  Sema *S = &Interp->getCI()->getSema();
  EXPECT_EQ(Cpp::GetQualifiedName(Cpp::GetScopeFromCompleteName(S, "N1")),
            "N1");
  EXPECT_EQ(Cpp::GetQualifiedName(Cpp::GetScopeFromCompleteName(S, "N1::N2")),
            "N1::N2");
  EXPECT_EQ(
      Cpp::GetQualifiedName(Cpp::GetScopeFromCompleteName(S, "N1::N2::C")),
      "N1::N2::C");
  EXPECT_EQ(
      Cpp::GetQualifiedName(Cpp::GetScopeFromCompleteName(S, "N1::N2::C::S")),
      "N1::N2::C::S");
}

TEST(ScopeReflectionTest, GetNamed) {
  std::vector<Decl *> Decls;
  std::string code = R"(namespace N1 {
                        namespace N2 {
                          class C {
                            int i;
                            enum E { A, B };
                            struct S {};
                          };
                        }
                        }
                       )";

  Interp = createInterpreter();
  Interp->declare(code);
  Sema *S = &Interp->getCI()->getSema();
  Cpp::TCppScope_t ns_N1 = Cpp::GetNamed(S, "N1", 0);
  Cpp::TCppScope_t ns_N2 = Cpp::GetNamed(S, "N2", ns_N1);
  Cpp::TCppScope_t cl_C = Cpp::GetNamed(S, "C", ns_N2);
  Cpp::TCppScope_t en_E = Cpp::GetNamed(S, "E", cl_C);
  EXPECT_EQ(Cpp::GetQualifiedName(ns_N1), "N1");
  EXPECT_EQ(Cpp::GetQualifiedName(ns_N2), "N1::N2");
  EXPECT_EQ(Cpp::GetQualifiedName(cl_C), "N1::N2::C");
  EXPECT_EQ(Cpp::GetQualifiedName(Cpp::GetNamed(S, "i", cl_C)), "N1::N2::C::i");
  EXPECT_EQ(Cpp::GetQualifiedName(en_E), "N1::N2::C::E");
  EXPECT_EQ(Cpp::GetQualifiedName(Cpp::GetNamed(S, "A", en_E)), "N1::N2::C::A");
  EXPECT_EQ(Cpp::GetQualifiedName(Cpp::GetNamed(S, "B", en_E)), "N1::N2::C::B");
  EXPECT_EQ(Cpp::GetQualifiedName(Cpp::GetNamed(S, "A", cl_C)), "N1::N2::C::A");
  EXPECT_EQ(Cpp::GetQualifiedName(Cpp::GetNamed(S, "B", cl_C)), "N1::N2::C::B");
  EXPECT_EQ(Cpp::GetQualifiedName(Cpp::GetNamed(S, "S", cl_C)), "N1::N2::C::S");
}

TEST(ScopeReflectionTest, GetParentScope) {
  std::vector<Decl *> Decls;
  std::string code = R"(namespace N1 {
                        namespace N2 {
                          class C {
                            int i;
                            enum E { A, B };
                            struct S {};
                          };
                        }
                        }
                       )";

  Interp = createInterpreter();
  Interp->declare(code);
  Sema *S = &Interp->getCI()->getSema();
  Cpp::TCppScope_t ns_N1 = Cpp::GetNamed(S, "N1", 0);
  Cpp::TCppScope_t ns_N2 = Cpp::GetNamed(S, "N2", ns_N1);
  Cpp::TCppScope_t cl_C = Cpp::GetNamed(S, "C", ns_N2);
  Cpp::TCppScope_t int_i = Cpp::GetNamed(S, "i", cl_C);
  Cpp::TCppScope_t en_E = Cpp::GetNamed(S, "E", cl_C);
  Cpp::TCppScope_t en_A = Cpp::GetNamed(S, "A", cl_C);
  Cpp::TCppScope_t en_B = Cpp::GetNamed(S, "B", cl_C);

  EXPECT_EQ(Cpp::GetQualifiedName(Cpp::GetParentScope(ns_N1)), "");
  EXPECT_EQ(Cpp::GetQualifiedName(Cpp::GetParentScope(ns_N2)), "N1");
  EXPECT_EQ(Cpp::GetQualifiedName(Cpp::GetParentScope(cl_C)), "N1::N2");
  EXPECT_EQ(Cpp::GetQualifiedName(Cpp::GetParentScope(int_i)), "N1::N2::C");
  EXPECT_EQ(Cpp::GetQualifiedName(Cpp::GetParentScope(en_E)), "N1::N2::C");
  EXPECT_EQ(Cpp::GetQualifiedName(Cpp::GetParentScope(en_A)), "N1::N2::C::E");
  EXPECT_EQ(Cpp::GetQualifiedName(Cpp::GetParentScope(en_B)), "N1::N2::C::E");
}

TEST(ScopeReflectionTest, GetScopeFromType) {
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
  EXPECT_EQ(Cpp::GetQualifiedName(Cpp::GetScopeFromType(QT1.getAsOpaquePtr())),
          "N::C");
  EXPECT_EQ(Cpp::GetQualifiedName(Cpp::GetScopeFromType(QT2.getAsOpaquePtr())),
          "N::S");
  EXPECT_EQ(Cpp::GetScopeFromType(QT3.getAsOpaquePtr()),
          (Cpp::TCppScope_t) 0);
}

TEST(ScopeReflectionTest, GetNumBases) {
  std::vector<Decl *> Decls;
  std::string code = R"(
    class A {};
    class B : virtual public A {};
    class C : virtual public A {};
    class D : public B, public C {};
    class E : public D {};
  )";

  GetAllTopLevelDecls(code, Decls);

  EXPECT_EQ(Cpp::GetNumBases(Decls[0]), 0);
  EXPECT_EQ(Cpp::GetNumBases(Decls[1]), 1);
  EXPECT_EQ(Cpp::GetNumBases(Decls[2]), 1);
  EXPECT_EQ(Cpp::GetNumBases(Decls[3]), 2);
  EXPECT_EQ(Cpp::GetNumBases(Decls[4]), 1);
}


TEST(ScopeReflectionTest, GetBaseClass) {
  std::vector<Decl *> Decls;
  std::string code = R"(
    class A {};
    class B : virtual public A {};
    class C : virtual public A {};
    class D : public B, public C {};
    class E : public D {};
  )";

  GetAllTopLevelDecls(code, Decls);

  auto get_base_class_name = [](Decl *D, int i) {
      return Cpp::GetQualifiedName(Cpp::GetBaseClass(D, i));
  };

  EXPECT_EQ(get_base_class_name(Decls[1], 0), "A");
  EXPECT_EQ(get_base_class_name(Decls[2], 0), "A");
  EXPECT_EQ(get_base_class_name(Decls[3], 0), "B");
  EXPECT_EQ(get_base_class_name(Decls[3], 1), "C");
  EXPECT_EQ(get_base_class_name(Decls[4], 0), "D");
}

TEST(ScopeReflectionTest, IsSubclass) {
  std::vector<Decl *> Decls;
  std::string code = R"(
    class A {};
    class B : virtual public A {};
    class C : virtual public A {};
    class D : public B, public C {};
    class E : public D {};
  )";

  GetAllTopLevelDecls(code, Decls);

  auto check_subclass = [](Decl *derived_D, Decl *base_D) {
    return Cpp::IsSubclass(derived_D, base_D);
  };

  EXPECT_TRUE(check_subclass(Decls[0], Decls[0]));
  EXPECT_TRUE(check_subclass(Decls[1], Decls[0]));
  EXPECT_TRUE(check_subclass(Decls[2], Decls[0]));
  EXPECT_TRUE(check_subclass(Decls[3], Decls[0]));
  EXPECT_TRUE(check_subclass(Decls[4], Decls[0]));
  EXPECT_FALSE(check_subclass(Decls[0], Decls[1]));
  EXPECT_TRUE(check_subclass(Decls[1], Decls[1]));
  EXPECT_FALSE(check_subclass(Decls[2], Decls[1]));
  EXPECT_TRUE(check_subclass(Decls[3], Decls[1]));
  EXPECT_TRUE(check_subclass(Decls[4], Decls[1]));
  EXPECT_FALSE(check_subclass(Decls[0], Decls[2]));
  EXPECT_FALSE(check_subclass(Decls[1], Decls[2]));
  EXPECT_TRUE(check_subclass(Decls[2], Decls[2]));
  EXPECT_TRUE(check_subclass(Decls[3], Decls[2]));
  EXPECT_TRUE(check_subclass(Decls[4], Decls[2]));
  EXPECT_FALSE(check_subclass(Decls[0], Decls[3]));
  EXPECT_FALSE(check_subclass(Decls[1], Decls[3]));
  EXPECT_FALSE(check_subclass(Decls[2], Decls[3]));
  EXPECT_TRUE(check_subclass(Decls[3], Decls[3]));
  EXPECT_TRUE(check_subclass(Decls[4], Decls[3]));
  EXPECT_FALSE(check_subclass(Decls[0], Decls[4]));
  EXPECT_FALSE(check_subclass(Decls[1], Decls[4]));
  EXPECT_FALSE(check_subclass(Decls[2], Decls[4]));
  EXPECT_FALSE(check_subclass(Decls[3], Decls[4]));
  EXPECT_TRUE(check_subclass(Decls[4], Decls[4]));
}

TEST(ScopeReflectionTest, GetBaseClassOffset) {
  std::vector<Decl *> Decls;
  std::string code = R"(
    class A { int a; };
    class B { int b; };
    class C : public A, public B { int c; };
    class D : public A, public B, public C { int d; };
  )";

  GetAllTopLevelDecls(code, Decls);
  Sema *S = &Interp->getCI()->getSema();

  EXPECT_EQ(Cpp::GetBaseClassOffset(S, Decls[2], Decls[0]), 0);
  EXPECT_EQ(Cpp::GetBaseClassOffset(S, Decls[2], Decls[1]), 4);
  EXPECT_EQ(Cpp::GetBaseClassOffset(S, Decls[3], Decls[0]), 0);
  EXPECT_EQ(Cpp::GetBaseClassOffset(S, Decls[3], Decls[1]), 4);
  EXPECT_EQ(Cpp::GetBaseClassOffset(S, Decls[3], Decls[2]), 8);
}

TEST(ScopeReflectionTest, GetAllCppNames) {
  std::vector<Decl *> Decls;
  std::string code = R"(
    class A { int a; };
    class B { int b; };
    class C : public A, public B { int c; };
    class D : public A, public B, public C { int d; };
    namespace N {
      class A { int a; };
      class B { int b; };
      class C : public A, public B { int c; };
      class D : public A, public B, public C { int d; };
    }
  )";

  GetAllTopLevelDecls(code, Decls);

  auto test_get_all_cpp_names = [](Decl *D, const std::vector<std::string> &truth_names) {
    auto names = Cpp::GetAllCppNames(D);
    EXPECT_EQ(names.size(), truth_names.size());

    for (unsigned i = 0; i < truth_names.size() && i < names.size(); i++) {
      EXPECT_EQ(names[i], truth_names[i]);
    }
  };

  test_get_all_cpp_names(Decls[0], {"a"});
  test_get_all_cpp_names(Decls[1], {"b"});
  test_get_all_cpp_names(Decls[2], {"c"});
  test_get_all_cpp_names(Decls[3], {"d"});
  test_get_all_cpp_names(Decls[4], {"A", "B", "C", "D"});
}
