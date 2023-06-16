#include "Utils.h"
#include "clang/Interpreter/InterOp.h"

#include "clang/AST/ASTContext.h"
#include "clang/Interpreter/InterOp.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Sema/Sema.h"

#include "clang/AST/DeclBase.h"
#include "clang/AST/ASTDumper.h"

#include "gtest/gtest.h"

using namespace TestUtils;
using namespace llvm;
using namespace clang;

TEST(ScopeReflectionTest, IsAggregate) {
  std::vector<Decl *> Decls;
  std::string code = R"(
    char cv[4] = {};
    int x[] = {};
    union u { int a; const char* b; };
    struct S {
      int x;
      struct Foo {
        int i;
        int j;
        int a[3];
      } b;
   };
  )";

  GetAllTopLevelDecls(code, Decls);
  EXPECT_TRUE(InterOp::IsAggregate(Decls[0]));
  EXPECT_TRUE(InterOp::IsAggregate(Decls[1]));
  EXPECT_TRUE(InterOp::IsAggregate(Decls[2]));
  EXPECT_TRUE(InterOp::IsAggregate(Decls[3]));
}


// Check that the CharInfo table has been constructed reasonably.
TEST(ScopeReflectionTest, IsNamespace) {
  std::vector<Decl*> Decls;
  GetAllTopLevelDecls("namespace N {} class C{}; int I;", Decls);
  EXPECT_TRUE(InterOp::IsNamespace(Decls[0]));
  EXPECT_FALSE(InterOp::IsNamespace(Decls[1]));
  EXPECT_FALSE(InterOp::IsNamespace(Decls[2]));
}

TEST(ScopeReflectionTest, IsClass) {
  std::vector<Decl*> Decls;
  GetAllTopLevelDecls("namespace N {} class C{}; int I;", Decls);
  EXPECT_FALSE(InterOp::IsClass(Decls[0]));
  EXPECT_TRUE(InterOp::IsClass(Decls[1]));
  EXPECT_FALSE(InterOp::IsClass(Decls[2]));
}

TEST(ScopeReflectionTest, IsComplete) {
  std::vector<Decl*> Decls;
  std::string code = R"(
    namespace N {}
    class C{};
    int I;
    struct S;
    enum E : int;
    union U{};
    template<typename T> struct TemplatedS{ T x; };
    TemplatedS<int> templateF() { return {}; };
  )";
  GetAllTopLevelDecls(code,
                      Decls);
  EXPECT_TRUE(InterOp::IsComplete(Decls[0]));
  EXPECT_TRUE(InterOp::IsComplete(Decls[1]));
  EXPECT_TRUE(InterOp::IsComplete(Decls[2]));
  EXPECT_FALSE(InterOp::IsComplete(Decls[3]));
  EXPECT_FALSE(InterOp::IsComplete(Decls[4]));
  EXPECT_TRUE(InterOp::IsComplete(Decls[5]));
  InterOp::TCppType_t retTy = InterOp::GetFunctionReturnType(Decls[7]);
  EXPECT_TRUE(InterOp::IsComplete(InterOp::GetScopeFromType(retTy)));
}

TEST(ScopeReflectionTest, SizeOf) {
  std::vector<Decl*> Decls;
  std::string code = R"(namespace N {} class C{}; int I; struct S;
                        enum E : int; union U{}; class Size4{int i;};
                        struct Size16 {short a; double b;};
                       )";
  GetAllTopLevelDecls(code, Decls);
  EXPECT_EQ(InterOp::SizeOf(Decls[0]), (size_t)0);
  EXPECT_EQ(InterOp::SizeOf(Decls[1]), (size_t)1);
  EXPECT_EQ(InterOp::SizeOf(Decls[2]), (size_t)0);
  EXPECT_EQ(InterOp::SizeOf(Decls[3]), (size_t)0);
  EXPECT_EQ(InterOp::SizeOf(Decls[4]), (size_t)0);
  EXPECT_EQ(InterOp::SizeOf(Decls[5]), (size_t)1);
  EXPECT_EQ(InterOp::SizeOf(Decls[6]), (size_t)4);
  EXPECT_EQ(InterOp::SizeOf(Decls[7]), (size_t)16);
}

TEST(ScopeReflectionTest, IsBuiltin) {
  // static std::set<std::string> g_builtins =
  // {"bool", "char", "signed char", "unsigned char", "wchar_t", "short", "unsigned short",
  //  "int", "unsigned int", "long", "unsigned long", "long long", "unsigned long long",
  //  "float", "double", "long double", "void"}

  InterOp::CreateInterpreter();
  ASTContext &C = Interp->getCI()->getASTContext();
  EXPECT_TRUE(InterOp::IsBuiltin(C.BoolTy.getAsOpaquePtr()));
  EXPECT_TRUE(InterOp::IsBuiltin(C.CharTy.getAsOpaquePtr()));
  EXPECT_TRUE(InterOp::IsBuiltin(C.SignedCharTy.getAsOpaquePtr()));
  EXPECT_TRUE(InterOp::IsBuiltin(C.VoidTy.getAsOpaquePtr()));
  // ...

  // complex
  EXPECT_TRUE(InterOp::IsBuiltin(C.getComplexType(C.FloatTy).getAsOpaquePtr()));
  EXPECT_TRUE(InterOp::IsBuiltin(C.getComplexType(C.DoubleTy).getAsOpaquePtr()));
  EXPECT_TRUE(InterOp::IsBuiltin(C.getComplexType(C.LongDoubleTy).getAsOpaquePtr()));
  EXPECT_TRUE(InterOp::IsBuiltin(C.getComplexType(C.Float128Ty).getAsOpaquePtr()));

  // std::complex
  std::vector<Decl*> Decls;
  Interp->declare("#include <complex>");
  Sema &S = Interp->getCI()->getSema();
  auto lookup = S.getStdNamespace()->lookup(&C.Idents.get("complex"));
  auto *CTD = cast<ClassTemplateDecl>(lookup.front());
  for (ClassTemplateSpecializationDecl *CTSD : CTD->specializations())
    EXPECT_TRUE(InterOp::IsBuiltin(C.getTypeDeclType(CTSD).getAsOpaquePtr()));
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
  EXPECT_TRUE(InterOp::IsTemplate(Decls[0]));
  EXPECT_FALSE(InterOp::IsTemplate(Decls[1]));
  EXPECT_TRUE(InterOp::IsTemplate(Decls[2]));
  EXPECT_FALSE(InterOp::IsTemplate(Decls[3]));
}

TEST(ScopeReflectionTest, IsTemplateSpecialization) {
  std::vector<Decl *> Decls;
  std::string code = R"(
    template<typename T>
    class A{};

    A<int> a;
    )";

  GetAllTopLevelDecls(code, Decls);
  EXPECT_FALSE(InterOp::IsTemplateSpecialization(Decls[0]));
  EXPECT_FALSE(InterOp::IsTemplateSpecialization(Decls[1]));
  EXPECT_TRUE(InterOp::IsTemplateSpecialization(
          InterOp::GetScopeFromType(InterOp::GetVariableType(Decls[1]))));
}

TEST(ScopeReflectionTest, IsTypedefed) {
  std::vector<Decl *> Decls;
  std::string code = R"(
    typedef int I;
    using D = double;
    class C {};
    )";

  GetAllTopLevelDecls(code, Decls);
  EXPECT_TRUE(InterOp::IsTypedefed(Decls[0]));
  EXPECT_TRUE(InterOp::IsTypedefed(Decls[1]));
  EXPECT_FALSE(InterOp::IsTypedefed(Decls[2]));
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
  EXPECT_FALSE(InterOp::IsAbstract(Decls[0]));
  EXPECT_TRUE(InterOp::IsAbstract(Decls[1]));
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
  EXPECT_TRUE(InterOp::IsVariable(Decls[0]));
  EXPECT_FALSE(InterOp::IsVariable(Decls[1]));

  std::vector<Decl *> SubDecls;
  GetAllSubDecls(Decls[1], SubDecls);
  EXPECT_FALSE(InterOp::IsVariable(SubDecls[0]));
  EXPECT_FALSE(InterOp::IsVariable(SubDecls[1]));
  EXPECT_FALSE(InterOp::IsVariable(SubDecls[2]));
  EXPECT_TRUE(InterOp::IsVariable(SubDecls[3]));
}

TEST(ScopeReflectionTest, GetName) {
  std::vector<Decl*> Decls;
  std::string code = R"(namespace N {} class C{}; int I; struct S;
                        enum E : int; union U{}; class Size4{int i;};
                        struct Size16 {short a; double b;};
                       )";
  GetAllTopLevelDecls(code, Decls);
  EXPECT_EQ(InterOp::GetName(Decls[0]), "N");
  EXPECT_EQ(InterOp::GetName(Decls[1]), "C");
  EXPECT_EQ(InterOp::GetName(Decls[2]), "I");
  EXPECT_EQ(InterOp::GetName(Decls[3]), "S");
  EXPECT_EQ(InterOp::GetName(Decls[4]), "E");
  EXPECT_EQ(InterOp::GetName(Decls[5]), "U");
  EXPECT_EQ(InterOp::GetName(Decls[6]), "Size4");
  EXPECT_EQ(InterOp::GetName(Decls[7]), "Size16");
}

TEST(ScopeReflectionTest, GetCompleteName) {
  std::vector<Decl *> Decls;
  std::string code = R"(namespace N {}
                        class C{};
                        int I;
                        struct S;
                        enum E : int;
                        union U{};
                        class Size4{int i;};
                        struct Size16 {short a; double b;};

                        template<typename T>
                        class A {};
                        A<int> a;
                       )";
  GetAllTopLevelDecls(code, Decls);

  EXPECT_EQ(InterOp::GetCompleteName(Decls[0]), "N");
  EXPECT_EQ(InterOp::GetCompleteName(Decls[1]), "C");
  EXPECT_EQ(InterOp::GetCompleteName(Decls[2]), "I");
  EXPECT_EQ(InterOp::GetCompleteName(Decls[3]), "S");
  EXPECT_EQ(InterOp::GetCompleteName(Decls[4]), "E");
  EXPECT_EQ(InterOp::GetCompleteName(Decls[5]), "U");
  EXPECT_EQ(InterOp::GetCompleteName(Decls[6]), "Size4");
  EXPECT_EQ(InterOp::GetCompleteName(Decls[7]), "Size16");
  EXPECT_EQ(InterOp::GetCompleteName(Decls[8]), "A");
  EXPECT_EQ(InterOp::GetCompleteName(InterOp::GetScopeFromType(
                                             InterOp::GetVariableType(
                                                     Decls[9]))), "A<int>");
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

  EXPECT_EQ(InterOp::GetQualifiedName(0), "<unnamed>");
  EXPECT_EQ(InterOp::GetQualifiedName(Decls[0]), "N");
  EXPECT_EQ(InterOp::GetQualifiedName(Decls[1]), "N::C");
  EXPECT_EQ(InterOp::GetQualifiedName(Decls[3]), "N::C::i");
  EXPECT_EQ(InterOp::GetQualifiedName(Decls[4]), "N::C::E");
}

TEST(ScopeReflectionTest, GetQualifiedCompleteName) {
  std::vector<Decl*> Decls;
  std::string code = R"(namespace N {
                        class C {
                          int i;
                          enum E { A, B };
                        };
                        template<typename T>
                        class A {};
                        A<int> a;
                        }
                       )";
  GetAllTopLevelDecls(code, Decls);
  GetAllSubDecls(Decls[0], Decls);
  GetAllSubDecls(Decls[1], Decls);

  EXPECT_EQ(InterOp::GetQualifiedCompleteName(0), "<unnamed>");
  EXPECT_EQ(InterOp::GetQualifiedCompleteName(Decls[0]), "N");
  EXPECT_EQ(InterOp::GetQualifiedCompleteName(Decls[1]), "N::C");
  EXPECT_EQ(InterOp::GetQualifiedCompleteName(Decls[2]), "N::A");
  EXPECT_EQ(InterOp::GetQualifiedCompleteName(InterOp::GetScopeFromType(InterOp::GetVariableType(Decls[3]))), "N::A<int>");
  EXPECT_EQ(InterOp::GetQualifiedCompleteName(Decls[5]), "N::C::i");
  EXPECT_EQ(InterOp::GetQualifiedCompleteName(Decls[6]), "N::C::E");
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
  usingNamespaces = InterOp::GetUsingNamespaces(
          Decls[0]->getASTContext().getTranslationUnitDecl());

  //EXPECT_EQ(InterOp::GetName(usingNamespaces[0]), "runtime");
  EXPECT_EQ(InterOp::GetName(usingNamespaces[usingNamespaces.size()-2]), "std");
  EXPECT_EQ(InterOp::GetName(usingNamespaces[usingNamespaces.size()-1]), "abc");
}

TEST(ScopeReflectionTest, GetGlobalScope) {
  EXPECT_EQ(InterOp::GetQualifiedName(InterOp::GetGlobalScope()), "");
  EXPECT_EQ(InterOp::GetName(InterOp::GetGlobalScope()), "");
}

TEST(ScopeReflectionTest, GetUnderlyingScope) {
  std::vector<Decl *> Decls;
  std::string code = R"(
    namespace N {
      class C {};
    }

    typedef N::C NC;
    typedef int INT;
  )";

  GetAllTopLevelDecls(code, Decls);

  EXPECT_EQ(InterOp::GetQualifiedName(InterOp::GetUnderlyingScope(Decls[0])), "N");
  EXPECT_EQ(InterOp::GetQualifiedName(InterOp::GetUnderlyingScope(Decls[1])), "N::C");
  EXPECT_EQ(InterOp::GetQualifiedName(InterOp::GetUnderlyingScope(Decls[2])), "INT");
}

TEST(ScopeReflectionTest, GetScope) {
  std::string code = R"(namespace N {
                        class C {
                          int i;
                          enum E { A, B };
                        };
                        }
                       )";

  InterOp::CreateInterpreter();
  Interp->declare(code);
  InterOp::TCppScope_t tu = InterOp::GetScope("", 0);
  InterOp::TCppScope_t ns_N = InterOp::GetScope("N", 0);
  InterOp::TCppScope_t cl_C = InterOp::GetScope("C", ns_N);

  EXPECT_EQ(InterOp::GetQualifiedName(tu), "");
  EXPECT_EQ(InterOp::GetQualifiedName(ns_N), "N");
  EXPECT_EQ(InterOp::GetQualifiedName(cl_C), "N::C");
}

TEST(ScopeReflectionTest, GetScopefromCompleteName) {
  std::string code = R"(namespace N1 {
                        namespace N2 {
                          class C {
                            struct S {};
                          };
                        }
                        }
                       )";

  InterOp::CreateInterpreter();

  Interp->declare(code);
  EXPECT_EQ(InterOp::GetQualifiedName(InterOp::GetScopeFromCompleteName("N1")), "N1");
  EXPECT_EQ(InterOp::GetQualifiedName(InterOp::GetScopeFromCompleteName("N1::N2")), "N1::N2");
  EXPECT_EQ(InterOp::GetQualifiedName(InterOp::GetScopeFromCompleteName("N1::N2::C")), "N1::N2::C");
  EXPECT_EQ(InterOp::GetQualifiedName(InterOp::GetScopeFromCompleteName("N1::N2::C::S")), "N1::N2::C::S");
}

TEST(ScopeReflectionTest, GetNamed) {
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
  InterOp::CreateInterpreter();

  Interp->declare(code);
  InterOp::TCppScope_t ns_N1 = InterOp::GetNamed("N1", nullptr);
  InterOp::TCppScope_t ns_N2 = InterOp::GetNamed("N2", ns_N1);
  InterOp::TCppScope_t cl_C = InterOp::GetNamed("C", ns_N2);
  InterOp::TCppScope_t en_E = InterOp::GetNamed("E", cl_C);
  EXPECT_EQ(InterOp::GetQualifiedName(ns_N1), "N1");
  EXPECT_EQ(InterOp::GetQualifiedName(ns_N2), "N1::N2");
  EXPECT_EQ(InterOp::GetQualifiedName(cl_C), "N1::N2::C");
  EXPECT_EQ(InterOp::GetQualifiedName(InterOp::GetNamed("i", cl_C)), "N1::N2::C::i");
  EXPECT_EQ(InterOp::GetQualifiedName(en_E), "N1::N2::C::E");
  EXPECT_EQ(InterOp::GetQualifiedName(InterOp::GetNamed("A", en_E)), "N1::N2::C::A");
  EXPECT_EQ(InterOp::GetQualifiedName(InterOp::GetNamed("B", en_E)), "N1::N2::C::B");
  EXPECT_EQ(InterOp::GetQualifiedName(InterOp::GetNamed("A", cl_C)), "N1::N2::C::A");
  EXPECT_EQ(InterOp::GetQualifiedName(InterOp::GetNamed("B", cl_C)), "N1::N2::C::B");
  EXPECT_EQ(InterOp::GetQualifiedName(InterOp::GetNamed("S", cl_C)), "N1::N2::C::S");

  Interp->process("#include <string>");
  InterOp::TCppScope_t std_ns = InterOp::GetNamed("std", nullptr);
  InterOp::TCppScope_t std_string_class = InterOp::GetNamed("string", std_ns);
  InterOp::TCppScope_t std_string_npos_var = InterOp::GetNamed("npos", std_string_class);
  EXPECT_EQ(InterOp::GetQualifiedName(std_ns), "std");
  EXPECT_EQ(InterOp::GetQualifiedName(std_string_class), "std::string");
  EXPECT_EQ(InterOp::GetQualifiedName(std_string_npos_var), "std::basic_string<char>::npos");
}

TEST(ScopeReflectionTest, GetParentScope) {
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

  InterOp::CreateInterpreter();

  Interp->declare(code);
  InterOp::TCppScope_t ns_N1 = InterOp::GetNamed("N1");
  InterOp::TCppScope_t ns_N2 = InterOp::GetNamed("N2", ns_N1);
  InterOp::TCppScope_t cl_C = InterOp::GetNamed("C", ns_N2);
  InterOp::TCppScope_t int_i = InterOp::GetNamed("i", cl_C);
  InterOp::TCppScope_t en_E = InterOp::GetNamed("E", cl_C);
  InterOp::TCppScope_t en_A = InterOp::GetNamed("A", cl_C);
  InterOp::TCppScope_t en_B = InterOp::GetNamed("B", cl_C);

  EXPECT_EQ(InterOp::GetQualifiedName(InterOp::GetParentScope(ns_N1)), "");
  EXPECT_EQ(InterOp::GetQualifiedName(InterOp::GetParentScope(ns_N2)), "N1");
  EXPECT_EQ(InterOp::GetQualifiedName(InterOp::GetParentScope(cl_C)), "N1::N2");
  EXPECT_EQ(InterOp::GetQualifiedName(InterOp::GetParentScope(int_i)), "N1::N2::C");
  EXPECT_EQ(InterOp::GetQualifiedName(InterOp::GetParentScope(en_E)), "N1::N2::C");
  EXPECT_EQ(InterOp::GetQualifiedName(InterOp::GetParentScope(en_A)), "N1::N2::C::E");
  EXPECT_EQ(InterOp::GetQualifiedName(InterOp::GetParentScope(en_B)), "N1::N2::C::E");
}

TEST(ScopeReflectionTest, GetScopeFromType) {
  std::vector<Decl *> Decls;
  std::string code = R"(
    namespace N {
    class C {};
    struct S {};
    typedef C T;
    }

    N::C c;

    N::S s;

    int i;
    
    N::T t;
  )";

  GetAllTopLevelDecls(code, Decls);
  QualType QT1 = llvm::dyn_cast<VarDecl>(Decls[1])->getType();
  QualType QT2 = llvm::dyn_cast<VarDecl>(Decls[2])->getType();
  QualType QT3 = llvm::dyn_cast<VarDecl>(Decls[3])->getType();
  QualType QT4 = llvm::dyn_cast<VarDecl>(Decls[4])->getType();
  EXPECT_EQ(InterOp::GetQualifiedName(InterOp::GetScopeFromType(QT1.getAsOpaquePtr())),
          "N::C");
  EXPECT_EQ(InterOp::GetQualifiedName(InterOp::GetScopeFromType(QT2.getAsOpaquePtr())),
          "N::S");
  EXPECT_EQ(InterOp::GetScopeFromType(QT3.getAsOpaquePtr()),
          (InterOp::TCppScope_t) 0);
  EXPECT_EQ(InterOp::GetQualifiedName(InterOp::GetScopeFromType(QT4.getAsOpaquePtr())),
          "N::C");
}

TEST(ScopeReflectionTest, GetNumBases) {
  std::vector<Decl *> Decls;
  std::string code = R"(
    class A {};
    class B : virtual public A {};
    class C : virtual public A {};
    class D : public B, public C {};
    class E : public D {};
    class NoDef;
  )";

  GetAllTopLevelDecls(code, Decls);

  EXPECT_EQ(InterOp::GetNumBases(Decls[0]), 0);
  EXPECT_EQ(InterOp::GetNumBases(Decls[1]), 1);
  EXPECT_EQ(InterOp::GetNumBases(Decls[2]), 1);
  EXPECT_EQ(InterOp::GetNumBases(Decls[3]), 2);
  EXPECT_EQ(InterOp::GetNumBases(Decls[4]), 1);
  // FIXME: Perhaps we should have a special number or error out as this
  // operation is not well defined if a class has no definition.
  EXPECT_EQ(InterOp::GetNumBases(Decls[5]), 0);
}


TEST(ScopeReflectionTest, GetBaseClass) {
  std::vector<Decl *> Decls;
  std::string code = R"(
    class A {};
    class B : virtual public A {};
    class C : virtual public A {};
    class D : public B, public C {};
    class E : public D {};

    template <typename T>
    class TC1 {};

    template <typename T>
    class TC2 : TC1 <T> {};

    TC2<A> var;
  )";

  GetAllTopLevelDecls(code, Decls);

  auto get_base_class_name = [](Decl *D, int i) {
      return InterOp::GetQualifiedName(InterOp::GetBaseClass(D, i));
  };

  EXPECT_EQ(get_base_class_name(Decls[1], 0), "A");
  EXPECT_EQ(get_base_class_name(Decls[2], 0), "A");
  EXPECT_EQ(get_base_class_name(Decls[3], 0), "B");
  EXPECT_EQ(get_base_class_name(Decls[3], 1), "C");
  EXPECT_EQ(get_base_class_name(Decls[4], 0), "D");

  auto *VD = InterOp::GetNamed("var");
  auto *VT = InterOp::GetVariableType(VD);
  auto *TC2_A_Decl = InterOp::GetScopeFromType(VT);
  auto *TC1_A_Decl = InterOp::GetBaseClass(TC2_A_Decl, 0);

  EXPECT_EQ(InterOp::GetCompleteName(TC1_A_Decl), "TC1<A>");
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

  EXPECT_TRUE(InterOp::IsSubclass(Decls[0], Decls[0]));
  EXPECT_TRUE(InterOp::IsSubclass(Decls[1], Decls[0]));
  EXPECT_TRUE(InterOp::IsSubclass(Decls[2], Decls[0]));
  EXPECT_TRUE(InterOp::IsSubclass(Decls[3], Decls[0]));
  EXPECT_TRUE(InterOp::IsSubclass(Decls[4], Decls[0]));
  EXPECT_FALSE(InterOp::IsSubclass(Decls[0], Decls[1]));
  EXPECT_TRUE(InterOp::IsSubclass(Decls[1], Decls[1]));
  EXPECT_FALSE(InterOp::IsSubclass(Decls[2], Decls[1]));
  EXPECT_TRUE(InterOp::IsSubclass(Decls[3], Decls[1]));
  EXPECT_TRUE(InterOp::IsSubclass(Decls[4], Decls[1]));
  EXPECT_FALSE(InterOp::IsSubclass(Decls[0], Decls[2]));
  EXPECT_FALSE(InterOp::IsSubclass(Decls[1], Decls[2]));
  EXPECT_TRUE(InterOp::IsSubclass(Decls[2], Decls[2]));
  EXPECT_TRUE(InterOp::IsSubclass(Decls[3], Decls[2]));
  EXPECT_TRUE(InterOp::IsSubclass(Decls[4], Decls[2]));
  EXPECT_FALSE(InterOp::IsSubclass(Decls[0], Decls[3]));
  EXPECT_FALSE(InterOp::IsSubclass(Decls[1], Decls[3]));
  EXPECT_FALSE(InterOp::IsSubclass(Decls[2], Decls[3]));
  EXPECT_TRUE(InterOp::IsSubclass(Decls[3], Decls[3]));
  EXPECT_TRUE(InterOp::IsSubclass(Decls[4], Decls[3]));
  EXPECT_FALSE(InterOp::IsSubclass(Decls[0], Decls[4]));
  EXPECT_FALSE(InterOp::IsSubclass(Decls[1], Decls[4]));
  EXPECT_FALSE(InterOp::IsSubclass(Decls[2], Decls[4]));
  EXPECT_FALSE(InterOp::IsSubclass(Decls[3], Decls[4]));
  EXPECT_TRUE(InterOp::IsSubclass(Decls[4], Decls[4]));
}

TEST(ScopeReflectionTest, GetBaseClassOffset) {
  std::vector<Decl *> Decls;
#define Stringify(s) Stringifyx(s)
#define Stringifyx(...) #__VA_ARGS__
#define CODE                                                    \
  class A { int m_a; };                                         \
  class B { int m_b; };                                         \
  class C : virtual A, virtual B { int m_c; };                  \
  class D : virtual A, virtual B, public C { int m_d; };        \
  class E : public A, public B { int m_e; };                    \
  class F : public A { int m_f; };                              \
  class G : public F { int m_g; };

  CODE;

  GetAllTopLevelDecls(Stringify(CODE), Decls);
#undef Stringifyx
#undef Stringify
#undef CODE

  auto *c = new C();
  EXPECT_EQ(InterOp::GetBaseClassOffset(Decls[2], Decls[0]), (char *)(A*)c - (char *)c);
  EXPECT_EQ(InterOp::GetBaseClassOffset(Decls[2], Decls[1]), (char *)(B*)c - (char *)c);

  auto *d = new D();
  EXPECT_EQ(InterOp::GetBaseClassOffset(Decls[3], Decls[0]), (char *)(A*)d - (char *)d);
  EXPECT_EQ(InterOp::GetBaseClassOffset(Decls[3], Decls[1]), (char *)(B*)d - (char *)d);
  EXPECT_EQ(InterOp::GetBaseClassOffset(Decls[3], Decls[2]), (char *)(C*)d - (char *)d);

  auto *e = new E();
  EXPECT_EQ(InterOp::GetBaseClassOffset(Decls[4], Decls[0]), (char *)(A*)e - (char *)e);
  EXPECT_EQ(InterOp::GetBaseClassOffset(Decls[4], Decls[1]), (char *)(B*)e - (char *)e);

  auto *g = new G();
  EXPECT_EQ(InterOp::GetBaseClassOffset(Decls[6], Decls[0]), (char *)(A*)g - (char *)g);
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
    auto names = InterOp::GetAllCppNames(D);
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

TEST(ScopeReflectionTest, InstantiateNNTPClassTemplate) {
  InterOp::CreateInterpreter();

  std::vector<Decl *> Decls;
  std::string code = R"(
    template <int N>
    struct Factorial {
      enum { value = N * Factorial<N - 1>::value };
    };

    template <>
    struct Factorial<0> {
      enum { value = 1 };
    };)";

  ASTContext &C = Interp->getCI()->getASTContext();
  GetAllTopLevelDecls(code, Decls);

  InterOp::TCppType_t IntTy = C.IntTy.getAsOpaquePtr();
  std::vector<InterOp::TemplateArgInfo> args1 = {{IntTy, "5"}};
  EXPECT_TRUE(InterOp::InstantiateClassTemplate(Decls[0], args1.data(),
                                                /*type_size*/ args1.size()));
}

TEST(ScopeReflectionTest, InstantiateClassTemplate) {
  std::vector<Decl *> Decls;
  std::string code = R"(
    template<typename T>
    class AllDefault {
    public:
      AllDefault(int val) : m_t(val) {}
      template<int aap=1, int noot=2>
      int do_stuff() { return m_t+aap+noot; }

    public:
      T m_t;
    };

    template<typename T = int>
    class C0 {
    public:
      C0(T val) : m_t(val) {}
      template<int aap=1, int noot=2>
      T do_stuff() { return m_t+aap+noot; }

    public:
      T m_t;
    };

    template<typename T, typename R = C0<T>>
    class C1 {
    public:
      C1(const R & val = R()) : m_t(val.m_t) {}
      template<int aap=1, int noot=2>
      T do_stuff() { return m_t+aap+noot; }

    public:
      T m_t;
    };

    template<typename T, unsigned N>
    class C2{};
  )";

  GetAllTopLevelDecls(code, Decls);
  ASTContext &C = Interp->getCI()->getASTContext();

  std::vector<InterOp::TemplateArgInfo> args1 = {C.IntTy.getAsOpaquePtr()};
  auto Instance1 = InterOp::InstantiateClassTemplate(Decls[0],
                                                     args1.data(),
                                                     /*type_size*/args1.size());
  EXPECT_TRUE(isa<ClassTemplateSpecializationDecl>((Decl*)Instance1));
  auto *CTSD1 = static_cast<ClassTemplateSpecializationDecl*>(Instance1);
  EXPECT_TRUE(CTSD1->hasDefinition());
  TemplateArgument TA1 = CTSD1->getTemplateArgs().get(0);
  EXPECT_TRUE(TA1.getAsType()->isIntegerType());
  EXPECT_TRUE(CTSD1->hasDefinition());

  auto Instance2 = InterOp::InstantiateClassTemplate(Decls[1],
                                                     nullptr,
                                                    /*type_size*/0);
  EXPECT_TRUE(isa<ClassTemplateSpecializationDecl>((Decl*)Instance2));
  auto *CTSD2 = static_cast<ClassTemplateSpecializationDecl*>(Instance2);
  EXPECT_TRUE(CTSD2->hasDefinition());
  TemplateArgument TA2 = CTSD2->getTemplateArgs().get(0);
  EXPECT_TRUE(TA2.getAsType()->isIntegerType());

  std::vector<InterOp::TemplateArgInfo> args3 = {C.IntTy.getAsOpaquePtr()};
  auto Instance3 = InterOp::InstantiateClassTemplate(Decls[2],
                                                     args3.data(),
                                                     /*type_size*/args3.size());
  EXPECT_TRUE(isa<ClassTemplateSpecializationDecl>((Decl*)Instance3));
  auto *CTSD3 = static_cast<ClassTemplateSpecializationDecl*>(Instance3);
  EXPECT_TRUE(CTSD3->hasDefinition());
  TemplateArgument TA3_0 = CTSD3->getTemplateArgs().get(0);
  TemplateArgument TA3_1 = CTSD3->getTemplateArgs().get(1);
  EXPECT_TRUE(TA3_0.getAsType()->isIntegerType());
  EXPECT_TRUE(InterOp::IsRecordType(TA3_1.getAsType().getAsOpaquePtr()));

  auto Inst3_methods = InterOp::GetClassMethods(Instance3);
  EXPECT_EQ(InterOp::GetFunctionSignature(Inst3_methods[0]),
            "C1<int>::C1(const C0<int> &val)");

  std::vector<InterOp::TemplateArgInfo> args4 = {C.IntTy.getAsOpaquePtr(),
                                               {C.IntTy.getAsOpaquePtr(), "3"}};
  auto Instance4 = InterOp::InstantiateClassTemplate(Decls[3],
                                                     args4.data(),
                                                     /*type_size*/args4.size());

  EXPECT_TRUE(isa<ClassTemplateSpecializationDecl>((Decl*)Instance4));
  auto *CTSD4 = static_cast<ClassTemplateSpecializationDecl*>(Instance4);
  EXPECT_TRUE(CTSD4->hasDefinition());
  TemplateArgument TA4_0 = CTSD4->getTemplateArgs().get(0);
  TemplateArgument TA4_1 = CTSD4->getTemplateArgs().get(1);
  EXPECT_TRUE(TA4_0.getAsType()->isIntegerType());
  EXPECT_TRUE(TA4_1.getAsIntegral() == 3);
}

TEST(ScopeReflectionTest, IncludeVector) {
  std::string code = R"(
    #include <vector>
    #include <iostream>
  )";
  Interp->process(code);
}
