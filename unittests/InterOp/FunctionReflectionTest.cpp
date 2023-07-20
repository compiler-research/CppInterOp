#include "Utils.h"

#include "clang/AST/ASTContext.h"
#include "clang/Interpreter/InterOp.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Sema/Sema.h"

#include "gtest/gtest.h"

#include <string>

using namespace TestUtils;
using namespace llvm;
using namespace clang;

TEST(FunctionReflectionTest, GetClassMethods) {
  std::vector<Decl*> Decls;
  std::string code = R"(
    class A {
    public:
      int f1(int a, int b) { return a + b; }
      const A *f2() const { return this; }
    private:
      int f3() { return 0; }
      void f4() {}
    protected:
      int f5(int i) { return i; }
    };

    typedef A shadow_A;
    class B {
    public:
        B(int n) : b{n} {}
        int b;
    };

    class C: public B {
    public:
        using B::B;
    };
    using MyInt = int;
    )";

  GetAllTopLevelDecls(code, Decls);

  auto get_method_name = [](InterOp::TCppFunction_t method) {
    return InterOp::GetFunctionSignature(method);
  };

  auto methods0 = InterOp::GetClassMethods(Decls[0]);

  EXPECT_EQ(methods0.size(), 11);
  EXPECT_EQ(get_method_name(methods0[0]), "int A::f1(int a, int b)");
  EXPECT_EQ(get_method_name(methods0[1]), "const A *A::f2() const");
  EXPECT_EQ(get_method_name(methods0[2]), "int A::f3()");
  EXPECT_EQ(get_method_name(methods0[3]), "void A::f4()");
  EXPECT_EQ(get_method_name(methods0[4]), "int A::f5(int i)");
  EXPECT_EQ(get_method_name(methods0[5]), "inline constexpr A::A()");
  EXPECT_EQ(get_method_name(methods0[6]), "inline constexpr A::A(const A &)");
  EXPECT_EQ(get_method_name(methods0[7]), "inline constexpr A &A::operator=(const A &)");
  EXPECT_EQ(get_method_name(methods0[8]), "inline constexpr A::A(A &&)");
  EXPECT_EQ(get_method_name(methods0[9]), "inline constexpr A &A::operator=(A &&)");
  EXPECT_EQ(get_method_name(methods0[10]), "inline A::~A()");

  auto methods1 = InterOp::GetClassMethods(Decls[1]);
  EXPECT_EQ(methods0.size(), methods1.size());
  EXPECT_EQ(methods0[0], methods1[0]);
  EXPECT_EQ(methods0[1], methods1[1]);
  EXPECT_EQ(methods0[2], methods1[2]);
  EXPECT_EQ(methods0[3], methods1[3]);
  EXPECT_EQ(methods0[4], methods1[4]);

  auto methods2 = InterOp::GetClassMethods(Decls[2]);

  EXPECT_EQ(methods2.size(), 6);
  EXPECT_EQ(get_method_name(methods2[0]), "B::B(int n)");
  EXPECT_EQ(get_method_name(methods2[1]), "inline constexpr B::B(const B &)");
  EXPECT_EQ(get_method_name(methods2[2]), "inline constexpr B::B(B &&)");
  EXPECT_EQ(get_method_name(methods2[3]), "inline B::~B()");
  EXPECT_EQ(get_method_name(methods2[4]), "inline B &B::operator=(const B &)");
  EXPECT_EQ(get_method_name(methods2[5]), "inline B &B::operator=(B &&)");

  auto methods3 = InterOp::GetClassMethods(Decls[3]);

  EXPECT_EQ(methods3.size(), 9);
  EXPECT_EQ(get_method_name(methods3[0]), "B::B(int n)");
  EXPECT_EQ(get_method_name(methods3[1]), "inline constexpr B::B(const B &)");
  EXPECT_EQ(get_method_name(methods3[3]), "inline C::C()");
  EXPECT_EQ(get_method_name(methods3[4]), "inline constexpr C::C(const C &)");
  EXPECT_EQ(get_method_name(methods3[5]), "inline constexpr C::C(C &&)");
  EXPECT_EQ(get_method_name(methods3[6]), "inline C &C::operator=(const C &)");
  EXPECT_EQ(get_method_name(methods3[7]), "inline C &C::operator=(C &&)");
  EXPECT_EQ(get_method_name(methods3[8]), "inline C::~C()");

  // Should not crash.
  auto methods4 = InterOp::GetClassMethods(Decls[4]);
  EXPECT_EQ(methods4.size(), 0);
}

TEST(FunctionReflectionTest, ConstructorInGetClassMethods) {
  std::vector<Decl*> Decls;
  std::string code = R"(
    struct S {
      int a;
      int b;
    };
    )";

  GetAllTopLevelDecls(code, Decls);

  auto has_constructor = [](Decl *D) {
    auto methods = InterOp::GetClassMethods(D);
    for (auto method : methods) {
      if (InterOp::IsConstructor(method))
        return true;
    }
    return false;
  };

  EXPECT_TRUE(has_constructor(Decls[0]));
}

TEST(FunctionReflectionTest, HasDefaultConstructor) {
  std::vector<Decl*> Decls;
  std::string code = R"(
    class A {
      private:
        int n;
    };
    class B {
      private:
        int n;
      public:
      B() : n(1) {}
    };
    class C {
      private:
        int n;
      public:
        C() = delete;
        C(int i) : n(i) {}
    };
    )";

  GetAllTopLevelDecls(code, Decls);
  EXPECT_TRUE(InterOp::HasDefaultConstructor(Decls[0]));
  EXPECT_TRUE(InterOp::HasDefaultConstructor(Decls[1]));
}

TEST(FunctionReflectionTest, GetDestructor) {
  std::vector<Decl*> Decls;
  std::string code = R"(
    class A {
    };
    class B {
      public:
      ~B() {}
    };
    class C {
      public:
      ~C() = delete;
    };
    )";

  GetAllTopLevelDecls(code, Decls);

  EXPECT_TRUE(InterOp::GetDestructor(Decls[0]));
  EXPECT_TRUE(InterOp::GetDestructor(Decls[1]));
  auto DeletedDtor = InterOp::GetDestructor(Decls[2]);
  EXPECT_TRUE(DeletedDtor);
  EXPECT_TRUE(InterOp::IsFunctionDeleted(DeletedDtor));
}

TEST(FunctionReflectionTest, GetFunctionsUsingName) {
  std::vector<Decl*> Decls;
  std::string code = R"(
    class A {
    public:
      int f1(int a, int b) { return a + b; }
      int f1(int a) { return f1(a, 10); }
      int f1() { return f1(10); }
    private:
      int f2() { return 0; }
    protected:
      int f3(int i) { return i; }
    };

    namespace N {
      int f4(int a) { return a + 1; }
      int f4() { return 0; }
    }

    typedef A shadow_A;
    )";

  GetAllTopLevelDecls(code, Decls);

  // This lambda can take in the scope and the name of the function
  // and returns the size of the vector returned by GetFunctionsUsingName
  auto get_number_of_funcs_using_name = [&](InterOp::TCppScope_t scope,
          const std::string &name) {
    auto Funcs = InterOp::GetFunctionsUsingName(scope, name);

    return Funcs.size();
  };

  EXPECT_EQ(get_number_of_funcs_using_name(Decls[0], "f1"), 3);
  EXPECT_EQ(get_number_of_funcs_using_name(Decls[0], "f2"), 1);
  EXPECT_EQ(get_number_of_funcs_using_name(Decls[0], "f3"), 1);
  EXPECT_EQ(get_number_of_funcs_using_name(Decls[1], "f4"), 2);
  EXPECT_EQ(get_number_of_funcs_using_name(Decls[2], "f1"), 3);
  EXPECT_EQ(get_number_of_funcs_using_name(Decls[2], "f2"), 1);
  EXPECT_EQ(get_number_of_funcs_using_name(Decls[2], "f3"), 1);
}

TEST(FunctionReflectionTest, GetFunctionReturnType) {
  std::vector<Decl*> Decls, SubDecls;
  std::string code = R"(
    namespace N { class C {}; }
    enum Switch { OFF, ON };

    class A {
      A (int i) { i++; }
      int f () { return 0; }
    };


    void f1() {}
    double f2() { return 0.2; }
    Switch f3() { return ON; }
    N::C f4() { return N::C(); }
    N::C *f5() { return new N::C(); }
    const N::C f6() { return N::C(); }
    volatile N::C f7() { return N::C(); }
    const volatile N::C f8() { return N::C(); }
    )";

  GetAllTopLevelDecls(code, Decls, true);
  GetAllSubDecls(Decls[2], SubDecls);

  EXPECT_EQ(InterOp::GetTypeAsString(InterOp::GetFunctionReturnType(Decls[3])), "void");
  EXPECT_EQ(InterOp::GetTypeAsString(InterOp::GetFunctionReturnType(Decls[4])), "double");
  EXPECT_EQ(InterOp::GetTypeAsString(InterOp::GetFunctionReturnType(Decls[5])), "Switch");
  EXPECT_EQ(InterOp::GetTypeAsString(InterOp::GetFunctionReturnType(Decls[6])), "N::C");
  EXPECT_EQ(InterOp::GetTypeAsString(InterOp::GetFunctionReturnType(Decls[7])), "N::C *");
  EXPECT_EQ(InterOp::GetTypeAsString(InterOp::GetFunctionReturnType(Decls[8])), "const N::C");
  EXPECT_EQ(InterOp::GetTypeAsString(InterOp::GetFunctionReturnType(Decls[9])), "volatile N::C");
  EXPECT_EQ(InterOp::GetTypeAsString(InterOp::GetFunctionReturnType(Decls[10])), "const volatile N::C");
  EXPECT_EQ(InterOp::GetTypeAsString(InterOp::GetFunctionReturnType(SubDecls[1])), "void");
  EXPECT_EQ(InterOp::GetTypeAsString(InterOp::GetFunctionReturnType(SubDecls[2])), "int");
}

TEST(FunctionReflectionTest, GetFunctionNumArgs) {
  std::vector<Decl*> Decls, SubDecls;
  std::string code = R"(
    void f1() {}
    void f2(int i, double d, long l, char ch) {}
    void f3(int i, double d, long l = 0, char ch = 'a') {}
    void f4(int i = 0, double d = 0.0, long l = 0, char ch = 'a') {}
    )";

  GetAllTopLevelDecls(code, Decls);
  EXPECT_EQ(InterOp::GetFunctionNumArgs(Decls[0]), (size_t) 0);
  EXPECT_EQ(InterOp::GetFunctionNumArgs(Decls[1]), (size_t) 4);
  EXPECT_EQ(InterOp::GetFunctionNumArgs(Decls[2]), (size_t) 4);
  EXPECT_EQ(InterOp::GetFunctionNumArgs(Decls[3]), (size_t) 4);
}

TEST(FunctionReflectionTest, GetFunctionRequiredArgs) {
  std::vector<Decl*> Decls, SubDecls;
  std::string code = R"(
    void f1() {}
    void f2(int i, double d, long l, char ch) {}
    void f3(int i, double d, long l = 0, char ch = 'a') {}
    void f4(int i = 0, double d = 0.0, long l = 0, char ch = 'a') {}
    )";

  GetAllTopLevelDecls(code, Decls);
  EXPECT_EQ(InterOp::GetFunctionRequiredArgs(Decls[0]), (size_t) 0);
  EXPECT_EQ(InterOp::GetFunctionRequiredArgs(Decls[1]), (size_t) 4);
  EXPECT_EQ(InterOp::GetFunctionRequiredArgs(Decls[2]), (size_t) 2);
  EXPECT_EQ(InterOp::GetFunctionRequiredArgs(Decls[3]), (size_t) 0);
}

TEST(FunctionReflectionTest, GetFunctionArgType) {
  std::vector<Decl*> Decls, SubDecls;
  std::string code = R"(
    void f1(int i, double d, long l, char ch) {}
    void f2(const int i, double d[], long *l, char ch[4]) {}
    )";

  GetAllTopLevelDecls(code, Decls);
  EXPECT_EQ(InterOp::GetTypeAsString(InterOp::GetFunctionArgType(Decls[0], 0)), "int");
  EXPECT_EQ(InterOp::GetTypeAsString(InterOp::GetFunctionArgType(Decls[0], 1)), "double");
  EXPECT_EQ(InterOp::GetTypeAsString(InterOp::GetFunctionArgType(Decls[0], 2)), "long");
  EXPECT_EQ(InterOp::GetTypeAsString(InterOp::GetFunctionArgType(Decls[0], 3)), "char");
  EXPECT_EQ(InterOp::GetTypeAsString(InterOp::GetFunctionArgType(Decls[1], 0)), "const int");
  EXPECT_EQ(InterOp::GetTypeAsString(InterOp::GetFunctionArgType(Decls[1], 1)), "double[]");
  EXPECT_EQ(InterOp::GetTypeAsString(InterOp::GetFunctionArgType(Decls[1], 2)), "long *");
  EXPECT_EQ(InterOp::GetTypeAsString(InterOp::GetFunctionArgType(Decls[1], 3)), "char[4]");
}

TEST(FunctionReflectionTest, GetFunctionSignature) {
  std::vector<Decl*> Decls, SubDecls;
  std::string code = R"(
    class C {
      void f(int i, double d, long l = 0, char ch = 'a') {}
    };

    namespace N
    {
      void f(int i, double d, long l = 0, char ch = 'a') {}
    }

    void f1() {}
    C f2(int i, double d, long l = 0, char ch = 'a') { return C(); }
    C *f3(int i, double d, long l = 0, char ch = 'a') { return new C(); }
    void f4(int i = 0, double d = 0.0, long l = 0, char ch = 'a') {}
    )";

  GetAllTopLevelDecls(code, Decls, true);
  GetAllSubDecls(Decls[0], Decls);
  GetAllSubDecls(Decls[1], Decls);

  EXPECT_EQ(InterOp::GetFunctionSignature(Decls[2]), "void f1()");
  EXPECT_EQ(InterOp::GetFunctionSignature(Decls[3]),
            "C f2(int i, double d, long l = 0, char ch = 'a')");
  EXPECT_EQ(InterOp::GetFunctionSignature(Decls[4]),
            "C *f3(int i, double d, long l = 0, char ch = 'a')");
  EXPECT_EQ(InterOp::GetFunctionSignature(Decls[5]),
            "void f4(int i = 0, double d = 0., long l = 0, char ch = 'a')");
  EXPECT_EQ(InterOp::GetFunctionSignature(Decls[7]),
            "void C::f(int i, double d, long l = 0, char ch = 'a')");
  EXPECT_EQ(InterOp::GetFunctionSignature(Decls[12]),
            "void N::f(int i, double d, long l = 0, char ch = 'a')");
}

TEST(FunctionReflectionTest, IsTemplatedFunction) {
  std::vector<Decl*> Decls, SubDeclsC1, SubDeclsC2;
  std::string code = R"(
    void f1(int a) {}

    template<typename T>
    void f2(T a) {}

    class C1 {
      void f1(int a) {}

      template<typename T>
      void f2(T a) {}
    };
    )";

  GetAllTopLevelDecls(code, Decls);
  GetAllSubDecls(Decls[2], SubDeclsC1);

  EXPECT_FALSE(InterOp::IsTemplatedFunction(Decls[0]));
  EXPECT_TRUE(InterOp::IsTemplatedFunction(Decls[1]));
  EXPECT_FALSE(InterOp::IsTemplatedFunction(SubDeclsC1[1]));
  EXPECT_TRUE(InterOp::IsTemplatedFunction(SubDeclsC1[2]));
}

TEST(FunctionReflectionTest, ExistsFunctionTemplate) {
  std::vector<Decl*> Decls;
  std::string code = R"(
    template<typename T>
    void f(T a) {}

    class C {
      template<typename T>
      void f(T a) {}
    };
    )";

  GetAllTopLevelDecls(code, Decls);
  EXPECT_TRUE(InterOp::ExistsFunctionTemplate("f", 0));
  EXPECT_TRUE(InterOp::ExistsFunctionTemplate("f", Decls[1]));
}

TEST(FunctionReflectionTest, IsPublicMethod) {
  std::vector<Decl *> Decls, SubDecls;
  std::string code = R"(
    class C {
    public:
      C() {}
      void pub_f() {}
      ~C() {}
    private:
      void pri_f() {}
    protected:
      void pro_f() {}
    };
    )";

  GetAllTopLevelDecls(code, Decls);
  GetAllSubDecls(Decls[0], SubDecls);

  EXPECT_TRUE(InterOp::IsPublicMethod(SubDecls[2]));
  EXPECT_TRUE(InterOp::IsPublicMethod(SubDecls[3]));
  EXPECT_TRUE(InterOp::IsPublicMethod(SubDecls[4]));
  EXPECT_FALSE(InterOp::IsPublicMethod(SubDecls[6]));
  EXPECT_FALSE(InterOp::IsPublicMethod(SubDecls[8]));
}

TEST(FunctionReflectionTest, IsProtectedMethod) {
  std::vector<Decl *> Decls, SubDecls;
  std::string code = R"(
    class C {
    public:
      C() {}
      void pub_f() {}
      ~C() {}
    private:
      void pri_f() {}
    protected:
      void pro_f() {}
    };
    )";

  GetAllTopLevelDecls(code, Decls);
  GetAllSubDecls(Decls[0], SubDecls);

  EXPECT_FALSE(InterOp::IsProtectedMethod(SubDecls[2]));
  EXPECT_FALSE(InterOp::IsProtectedMethod(SubDecls[3]));
  EXPECT_FALSE(InterOp::IsProtectedMethod(SubDecls[4]));
  EXPECT_FALSE(InterOp::IsProtectedMethod(SubDecls[6]));
  EXPECT_TRUE(InterOp::IsProtectedMethod(SubDecls[8]));
}

TEST(FunctionReflectionTest, IsPrivateMethod) {
  std::vector<Decl *> Decls, SubDecls;
  std::string code = R"(
    class C {
    public:
      C() {}
      void pub_f() {}
      ~C() {}
    private:
      void pri_f() {}
    protected:
      void pro_f() {}
    };
    )";

  GetAllTopLevelDecls(code, Decls);
  GetAllSubDecls(Decls[0], SubDecls);

  EXPECT_FALSE(InterOp::IsPrivateMethod(SubDecls[2]));
  EXPECT_FALSE(InterOp::IsPrivateMethod(SubDecls[3]));
  EXPECT_FALSE(InterOp::IsPrivateMethod(SubDecls[4]));
  EXPECT_TRUE(InterOp::IsPrivateMethod(SubDecls[6]));
  EXPECT_FALSE(InterOp::IsPrivateMethod(SubDecls[8]));
}

TEST(FunctionReflectionTest, IsConstructor) {
  std::vector<Decl *> Decls, SubDecls;
  std::string code = R"(
    class C {
    public:
      C() {}
      void pub_f() {}
      ~C() {}
    private:
      void pri_f() {}
    protected:
      void pro_f() {}
    };
    )";

  GetAllTopLevelDecls(code, Decls);
  GetAllSubDecls(Decls[0], SubDecls);

  EXPECT_TRUE(InterOp::IsConstructor(SubDecls[2]));
  EXPECT_FALSE(InterOp::IsConstructor(SubDecls[3]));
  EXPECT_FALSE(InterOp::IsConstructor(SubDecls[4]));
  EXPECT_FALSE(InterOp::IsConstructor(SubDecls[6]));
  EXPECT_FALSE(InterOp::IsConstructor(SubDecls[8]));
}

TEST(FunctionReflectionTest, IsDestructor) {
  std::vector<Decl *> Decls, SubDecls;
  std::string code = R"(
    class C {
    public:
      C() {}
      void pub_f() {}
      ~C() {}
    private:
      void pri_f() {}
    protected:
      void pro_f() {}
    };
    )";

  GetAllTopLevelDecls(code, Decls);
  GetAllSubDecls(Decls[0], SubDecls);

  EXPECT_FALSE(InterOp::IsDestructor(SubDecls[2]));
  EXPECT_FALSE(InterOp::IsDestructor(SubDecls[3]));
  EXPECT_TRUE(InterOp::IsDestructor(SubDecls[4]));
  EXPECT_FALSE(InterOp::IsDestructor(SubDecls[6]));
  EXPECT_FALSE(InterOp::IsDestructor(SubDecls[8]));
}

TEST(FunctionReflectionTest, IsStaticMethod) {
  std::vector<Decl *> Decls, SubDecls;
  std::string code = R"(
    class C {
      void f1() {}
      static void f2() {}
    };
    )";

  GetAllTopLevelDecls(code, Decls);
  GetAllSubDecls(Decls[0], SubDecls);

  EXPECT_FALSE(InterOp::IsStaticMethod(SubDecls[1]));
  EXPECT_TRUE(InterOp::IsStaticMethod(SubDecls[2]));
}

TEST(FunctionReflectionTest, GetFunctionAddress) {
  std::vector<Decl*> Decls, SubDecls;
  std::string code = "int f1(int i) { return i * i; }";

  GetAllTopLevelDecls(code, Decls);

  testing::internal::CaptureStdout();
  Interp->declare("#include <iostream>");
  Interp->process(
    "void * address = (void *) &f1; \n"
    "std::cout << address; \n"
    );

  std::string output = testing::internal::GetCapturedStdout();
  std::stringstream address;
  address << InterOp::GetFunctionAddress(Decls[0]);
  EXPECT_EQ(address.str(), output);
}

TEST(FunctionReflectionTest, IsVirtualMethod) {
  std::vector<Decl*> Decls, SubDecls;
  std::string code = R"(
    class A {
      public:
      virtual void x() {}
      void y() {}
    };
    )";

  GetAllTopLevelDecls(code, Decls);
  GetAllSubDecls(Decls[0], SubDecls);

  EXPECT_EQ(InterOp::GetName(SubDecls[2]), "x");
  EXPECT_TRUE(InterOp::IsVirtualMethod(SubDecls[2]));
  EXPECT_EQ(InterOp::GetName(SubDecls[3]), "y");
  EXPECT_FALSE(InterOp::IsVirtualMethod(SubDecls[3])); // y()
}

TEST(FunctionReflectionTest, JitCallAdvanced) {
  std::vector<Decl*> Decls;
  std::string code = R"(
      typedef struct _name {
        _name() { p[0] = (void*)0x1; p[1] = (void*)0x2; p[2] = (void*)0x3; }
        void* p[3];
      } name;
    )";

  GetAllTopLevelDecls(code, Decls);
  auto *CtorD
    = (clang::CXXConstructorDecl*)InterOp::GetDefaultConstructor(Decls[0]);
  auto Ctor = InterOp::MakeFunctionCallable(CtorD);
  EXPECT_TRUE((bool)Ctor) << "Failed to build a wrapper for the ctor";
  void* object = nullptr;
  Ctor.Invoke(&object);
  EXPECT_TRUE(object) << "Failed to call the ctor.";
  // Building a wrapper with a typedef decl must be possible.
  InterOp::Destruct(object, Decls[1]);

  // Test to check if typedefs are unnecessarily resolved during the creation
  // of the wrapper
  Interp->process(R"(
    class TDT {
      private:
          class PC {
          public:
              PC(int i) : m_val(i) {}
              int m_val;
          };

      public:
          typedef PC PP;
          PP f() { return PC(42); }
      };
  )");

  clang::NamedDecl *ClassTDT = (clang::NamedDecl*)InterOp::GetNamed("TDT");
  auto *CtorTDT = (clang::CXXConstructorDecl*)InterOp::GetDefaultConstructor(ClassTDT);
  auto FCI_CtorTDT =
      InterOp::MakeFunctionCallable(CtorTDT);
  void* objectTDT = nullptr;
  FCI_CtorTDT.Invoke((void*)&objectTDT);
  EXPECT_TRUE(objectTDT != nullptr);
  clang::NamedDecl *TDT_f = (clang::NamedDecl*)InterOp::GetNamed("f", ClassTDT);
  auto FCI_TDT_f =
      InterOp::MakeFunctionCallable(TDT_f);
  void* objectTDT_PP = nullptr;
  FCI_TDT_f.Invoke((void*)&objectTDT_PP, {}, objectTDT);
  EXPECT_TRUE(objectTDT_PP != nullptr);
  std::stringstream ss_mval;
  ss_mval << "(" << objectTDT_PP << ")->mval";
  EXPECT_EQ(InterOp::Evaluate(ss_mval.str().c_str()), 42);
}

TEST(FunctionReflectionTest, GetFunctionCallWrapper) {
  std::vector<Decl*> Decls;
  std::string code = R"(
    int f1(int i) { return i * i; }
    )";

  GetAllTopLevelDecls(code, Decls);

  Interp->process(R"(
    #include <string>
    void f2(std::string &s) { printf("%s", s.c_str()); };
  )");

  Interp->process(R"(
    namespace NS {
      int f3() { return 3; }

      extern "C" int f4() { return 4; }
    }
  )");

  InterOp::JitCall FCI1 =
      InterOp::MakeFunctionCallable(Decls[0]);
  EXPECT_TRUE(FCI1.getKind() == InterOp::JitCall::kGenericCall);
  InterOp::JitCall FCI2 =
      InterOp::MakeFunctionCallable(InterOp::GetNamed("f2"));
  EXPECT_TRUE(FCI2.getKind() == InterOp::JitCall::kGenericCall);
  InterOp::JitCall FCI3 =
    InterOp::MakeFunctionCallable(InterOp::GetNamed("f3", InterOp::GetNamed("NS")));
  EXPECT_TRUE(FCI3.getKind() == InterOp::JitCall::kGenericCall);
  InterOp::JitCall FCI4 =
      InterOp::MakeFunctionCallable(InterOp::GetNamed("f4", InterOp::GetNamed("NS")));
  EXPECT_TRUE(FCI4.getKind() == InterOp::JitCall::kGenericCall);

  int i = 9, ret1, ret3, ret4;
  std::string s("Hello World!\n");
  void *args0[1] = { (void *) &i };
  void *args1[1] = { (void *) &s };

  FCI1.Invoke(&ret1, {args0, /*args_size=*/1});
  EXPECT_EQ(ret1, i * i);

  testing::internal::CaptureStdout();
  FCI2.Invoke({args1, /*args_size=*/1});
  std::string output = testing::internal::GetCapturedStdout();
  EXPECT_EQ(output, s);

  FCI3.Invoke(&ret3);
  EXPECT_EQ(ret3, 3);

  FCI4.Invoke(&ret4);
  EXPECT_EQ(ret4, 4);

  // FIXME: Do we need to support private ctors?
  Interp->process(R"(
    class C {
    public:
      C() { printf("Default Ctor Called\n"); }
      ~C() { printf("Dtor Called\n"); }
    };
  )");

  clang::NamedDecl *ClassC = (clang::NamedDecl*)InterOp::GetNamed("C");
  auto *CtorD = (clang::CXXConstructorDecl*)InterOp::GetDefaultConstructor(ClassC);
  auto FCI_Ctor =
    InterOp::MakeFunctionCallable(CtorD);
  void* object = nullptr;
  testing::internal::CaptureStdout();
  FCI_Ctor.Invoke((void*)&object);
  output = testing::internal::GetCapturedStdout();
  EXPECT_EQ(output, "Default Ctor Called\n");
  EXPECT_TRUE(object != nullptr);

  auto *DtorD = (clang::CXXDestructorDecl*)InterOp::GetDestructor(ClassC);
  auto FCI_Dtor =
    InterOp::MakeFunctionCallable(DtorD);
  testing::internal::CaptureStdout();
  FCI_Dtor.Invoke(object);
  output = testing::internal::GetCapturedStdout();
  EXPECT_EQ(output, "Dtor Called\n");
}

TEST(FunctionReflectionTest, IsConstMethod) {
  std::vector<Decl*> Decls, SubDecls;
  std::string code = R"(
    class C {
      void f1() const {}
      void f2() {}
    };
    )";

  GetAllTopLevelDecls(code, Decls);
  GetAllSubDecls(Decls[0], SubDecls);

  EXPECT_TRUE(InterOp::IsConstMethod(SubDecls[1])); // f1
  EXPECT_FALSE(InterOp::IsConstMethod(SubDecls[2])); // f2
}

TEST(FunctionReflectionTest, GetFunctionArgName) {
  std::vector<Decl*> Decls, SubDecls;
  std::string code = R"(
    void f1(int i, double d, long l, char ch) {}
    void f2(const int i, double d[], long *l, char ch[4]) {}
    )";

  GetAllTopLevelDecls(code, Decls);
  EXPECT_EQ(InterOp::GetFunctionArgName(Decls[0], 0), "i");
  EXPECT_EQ(InterOp::GetFunctionArgName(Decls[0], 1), "d");
  EXPECT_EQ(InterOp::GetFunctionArgName(Decls[0], 2), "l");
  EXPECT_EQ(InterOp::GetFunctionArgName(Decls[0], 3), "ch");
  EXPECT_EQ(InterOp::GetFunctionArgName(Decls[1], 0), "i");
  EXPECT_EQ(InterOp::GetFunctionArgName(Decls[1], 1), "d");
  EXPECT_EQ(InterOp::GetFunctionArgName(Decls[1], 2), "l");
  EXPECT_EQ(InterOp::GetFunctionArgName(Decls[1], 3), "ch");
}

TEST(FunctionReflectionTest, GetFunctionArgDefault) {
  std::vector<Decl*> Decls, SubDecls;
  std::string code = R"(
    void f1(int i, double d = 4.0, const char *s = "default", char ch = 'c') {}
    void f2(float i = 0.0, double d = 3.123, long m = 34126) {}
    )";

  GetAllTopLevelDecls(code, Decls);
  EXPECT_EQ(InterOp::GetFunctionArgDefault(Decls[0], 0), "");
  EXPECT_EQ(InterOp::GetFunctionArgDefault(Decls[0], 1), "4.");
  EXPECT_EQ(InterOp::GetFunctionArgDefault(Decls[0], 2), "\"default\"");
  EXPECT_EQ(InterOp::GetFunctionArgDefault(Decls[0], 3), "\'c\'");
  EXPECT_EQ(InterOp::GetFunctionArgDefault(Decls[1], 0), "0.");
  EXPECT_EQ(InterOp::GetFunctionArgDefault(Decls[1], 1), "3.123");
  EXPECT_EQ(InterOp::GetFunctionArgDefault(Decls[1], 2), "34126");
}

TEST(FunctionReflectionTest, Construct) {
  InterOp::CreateInterpreter();

  Interp->declare(R"(
    #include <new>
    extern "C" int printf(const char*,...);
    class C {
      int x;
      C() {
        x = 12345;
        printf("Constructor Executed");
      }
    };
    )");

  testing::internal::CaptureStdout();
  InterOp::TCppScope_t scope = InterOp::GetNamed("C");
  InterOp::TCppObject_t object = InterOp::Construct(scope);
  EXPECT_TRUE(object != nullptr);
  std::string output = testing::internal::GetCapturedStdout();
  EXPECT_EQ(output, "Constructor Executed");
  output.clear();

  // Placement.
  testing::internal::CaptureStdout();
  void* where = InterOp::Allocate(scope);
  EXPECT_TRUE(where == InterOp::Construct(scope, where));
  // Check for the value of x which should be at the start of the object.
  EXPECT_TRUE(*(int *)where == 12345);
  InterOp::Deallocate(scope, where);
  output = testing::internal::GetCapturedStdout();
  EXPECT_EQ(output, "Constructor Executed");
}

TEST(FunctionReflectionTest, Destruct) {
  InterOp::CreateInterpreter();

  Interp->declare(R"(
    #include <new>
    extern "C" int printf(const char*,...);
    class C {
      C() {}
      ~C() {
        printf("Destructor Executed");
      }
    };
    )");

  testing::internal::CaptureStdout();
  InterOp::TCppScope_t scope = InterOp::GetNamed("C");
  InterOp::TCppObject_t object = InterOp::Construct(scope);
  InterOp::Destruct(object, scope);
  std::string output = testing::internal::GetCapturedStdout();

  EXPECT_EQ(output, "Destructor Executed");

  output.clear();
  testing::internal::CaptureStdout();
  object = InterOp::Construct(scope);
  // Make sure we do not call delete by adding an explicit Deallocate. If we
  // called delete the Deallocate will cause a double deletion error.
  InterOp::Destruct(object, scope, /*withFree=*/false);
  InterOp::Deallocate(scope, object);
  output = testing::internal::GetCapturedStdout();
  EXPECT_EQ(output, "Destructor Executed");
}
