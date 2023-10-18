#include "Utils.h"

#include "clang/AST/ASTContext.h"
#include "clang/Interpreter/CppInterOp.h"
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

  auto get_method_name = [](Cpp::TCppFunction_t method) {
    return Cpp::GetFunctionSignature(method);
  };

  auto methods0 = Cpp::GetClassMethods(Decls[0]);

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

  auto methods1 = Cpp::GetClassMethods(Decls[1]);
  EXPECT_EQ(methods0.size(), methods1.size());
  EXPECT_EQ(methods0[0], methods1[0]);
  EXPECT_EQ(methods0[1], methods1[1]);
  EXPECT_EQ(methods0[2], methods1[2]);
  EXPECT_EQ(methods0[3], methods1[3]);
  EXPECT_EQ(methods0[4], methods1[4]);

  auto methods2 = Cpp::GetClassMethods(Decls[2]);

  EXPECT_EQ(methods2.size(), 6);
  EXPECT_EQ(get_method_name(methods2[0]), "B::B(int n)");
  EXPECT_EQ(get_method_name(methods2[1]), "inline constexpr B::B(const B &)");
  EXPECT_EQ(get_method_name(methods2[2]), "inline constexpr B::B(B &&)");
  EXPECT_EQ(get_method_name(methods2[3]), "inline B::~B()");
  EXPECT_EQ(get_method_name(methods2[4]), "inline B &B::operator=(const B &)");
  EXPECT_EQ(get_method_name(methods2[5]), "inline B &B::operator=(B &&)");

  auto methods3 = Cpp::GetClassMethods(Decls[3]);

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
  auto methods4 = Cpp::GetClassMethods(Decls[4]);
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
    auto methods = Cpp::GetClassMethods(D);
    for (auto method : methods) {
      if (Cpp::IsConstructor(method))
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
  EXPECT_TRUE(Cpp::HasDefaultConstructor(Decls[0]));
  EXPECT_TRUE(Cpp::HasDefaultConstructor(Decls[1]));
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

  EXPECT_TRUE(Cpp::GetDestructor(Decls[0]));
  EXPECT_TRUE(Cpp::GetDestructor(Decls[1]));
  auto DeletedDtor = Cpp::GetDestructor(Decls[2]);
  EXPECT_TRUE(DeletedDtor);
  EXPECT_TRUE(Cpp::IsFunctionDeleted(DeletedDtor));
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
  auto get_number_of_funcs_using_name = [&](Cpp::TCppScope_t scope,
          const std::string &name) {
    auto Funcs = Cpp::GetFunctionsUsingName(scope, name);

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
    int n;
    )";

  GetAllTopLevelDecls(code, Decls, true);
  GetAllSubDecls(Decls[2], SubDecls);

  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetFunctionReturnType(Decls[3])), "void");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetFunctionReturnType(Decls[4])), "double");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetFunctionReturnType(Decls[5])), "Switch");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetFunctionReturnType(Decls[6])), "N::C");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetFunctionReturnType(Decls[7])), "N::C *");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetFunctionReturnType(Decls[8])), "const N::C");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetFunctionReturnType(Decls[9])), "volatile N::C");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetFunctionReturnType(Decls[10])), "const volatile N::C");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetFunctionReturnType(Decls[11])), "NULL TYPE");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetFunctionReturnType(SubDecls[1])), "void");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetFunctionReturnType(SubDecls[2])), "int");
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
  EXPECT_EQ(Cpp::GetFunctionNumArgs(Decls[0]), (size_t) 0);
  EXPECT_EQ(Cpp::GetFunctionNumArgs(Decls[1]), (size_t) 4);
  EXPECT_EQ(Cpp::GetFunctionNumArgs(Decls[2]), (size_t) 4);
  EXPECT_EQ(Cpp::GetFunctionNumArgs(Decls[3]), (size_t) 4);
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
  EXPECT_EQ(Cpp::GetFunctionRequiredArgs(Decls[0]), (size_t) 0);
  EXPECT_EQ(Cpp::GetFunctionRequiredArgs(Decls[1]), (size_t) 4);
  EXPECT_EQ(Cpp::GetFunctionRequiredArgs(Decls[2]), (size_t) 2);
  EXPECT_EQ(Cpp::GetFunctionRequiredArgs(Decls[3]), (size_t) 0);
}

TEST(FunctionReflectionTest, GetFunctionArgType) {
  std::vector<Decl*> Decls, SubDecls;
  std::string code = R"(
    void f1(int i, double d, long l, char ch) {}
    void f2(const int i, double d[], long *l, char ch[4]) {}
    )";

  GetAllTopLevelDecls(code, Decls);
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetFunctionArgType(Decls[0], 0)), "int");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetFunctionArgType(Decls[0], 1)), "double");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetFunctionArgType(Decls[0], 2)), "long");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetFunctionArgType(Decls[0], 3)), "char");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetFunctionArgType(Decls[1], 0)), "const int");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetFunctionArgType(Decls[1], 1)), "double[]");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetFunctionArgType(Decls[1], 2)), "long *");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetFunctionArgType(Decls[1], 3)), "char[4]");
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

  EXPECT_EQ(Cpp::GetFunctionSignature(Decls[2]), "void f1()");
  EXPECT_EQ(Cpp::GetFunctionSignature(Decls[3]),
            "C f2(int i, double d, long l = 0, char ch = 'a')");
  EXPECT_EQ(Cpp::GetFunctionSignature(Decls[4]),
            "C *f3(int i, double d, long l = 0, char ch = 'a')");
  EXPECT_EQ(Cpp::GetFunctionSignature(Decls[5]),
            "void f4(int i = 0, double d = 0., long l = 0, char ch = 'a')");
  EXPECT_EQ(Cpp::GetFunctionSignature(Decls[7]),
            "void C::f(int i, double d, long l = 0, char ch = 'a')");
  EXPECT_EQ(Cpp::GetFunctionSignature(Decls[12]),
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

  EXPECT_FALSE(Cpp::IsTemplatedFunction(Decls[0]));
  EXPECT_TRUE(Cpp::IsTemplatedFunction(Decls[1]));
  EXPECT_FALSE(Cpp::IsTemplatedFunction(SubDeclsC1[1]));
  EXPECT_TRUE(Cpp::IsTemplatedFunction(SubDeclsC1[2]));
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
  EXPECT_TRUE(Cpp::ExistsFunctionTemplate("f", 0));
  EXPECT_TRUE(Cpp::ExistsFunctionTemplate("f", Decls[1]));
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

  EXPECT_TRUE(Cpp::IsPublicMethod(SubDecls[2]));
  EXPECT_TRUE(Cpp::IsPublicMethod(SubDecls[3]));
  EXPECT_TRUE(Cpp::IsPublicMethod(SubDecls[4]));
  EXPECT_FALSE(Cpp::IsPublicMethod(SubDecls[6]));
  EXPECT_FALSE(Cpp::IsPublicMethod(SubDecls[8]));
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

  EXPECT_FALSE(Cpp::IsProtectedMethod(SubDecls[2]));
  EXPECT_FALSE(Cpp::IsProtectedMethod(SubDecls[3]));
  EXPECT_FALSE(Cpp::IsProtectedMethod(SubDecls[4]));
  EXPECT_FALSE(Cpp::IsProtectedMethod(SubDecls[6]));
  EXPECT_TRUE(Cpp::IsProtectedMethod(SubDecls[8]));
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

  EXPECT_FALSE(Cpp::IsPrivateMethod(SubDecls[2]));
  EXPECT_FALSE(Cpp::IsPrivateMethod(SubDecls[3]));
  EXPECT_FALSE(Cpp::IsPrivateMethod(SubDecls[4]));
  EXPECT_TRUE(Cpp::IsPrivateMethod(SubDecls[6]));
  EXPECT_FALSE(Cpp::IsPrivateMethod(SubDecls[8]));
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

  EXPECT_TRUE(Cpp::IsConstructor(SubDecls[2]));
  EXPECT_FALSE(Cpp::IsConstructor(SubDecls[3]));
  EXPECT_FALSE(Cpp::IsConstructor(SubDecls[4]));
  EXPECT_FALSE(Cpp::IsConstructor(SubDecls[6]));
  EXPECT_FALSE(Cpp::IsConstructor(SubDecls[8]));
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

  EXPECT_FALSE(Cpp::IsDestructor(SubDecls[2]));
  EXPECT_FALSE(Cpp::IsDestructor(SubDecls[3]));
  EXPECT_TRUE(Cpp::IsDestructor(SubDecls[4]));
  EXPECT_FALSE(Cpp::IsDestructor(SubDecls[6]));
  EXPECT_FALSE(Cpp::IsDestructor(SubDecls[8]));
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

  EXPECT_FALSE(Cpp::IsStaticMethod(SubDecls[1]));
  EXPECT_TRUE(Cpp::IsStaticMethod(SubDecls[2]));
}

#ifdef __APPLE__
TEST(FunctionReflectionTest, DISABLED_GetFunctionAddress) {
#else
TEST(FunctionReflectionTest, GetFunctionAddress) {
#endif
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
  address << Cpp::GetFunctionAddress(Decls[0]);
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

  EXPECT_EQ(Cpp::GetName(SubDecls[2]), "x");
  EXPECT_TRUE(Cpp::IsVirtualMethod(SubDecls[2]));
  EXPECT_EQ(Cpp::GetName(SubDecls[3]), "y");
  EXPECT_FALSE(Cpp::IsVirtualMethod(SubDecls[3])); // y()
}

#ifdef __APPLE__
TEST(FunctionReflectionTest, DISABLED_JitCallAdvanced) {
#else
TEST(FunctionReflectionTest, JitCallAdvanced) {
#endif
  std::vector<Decl*> Decls;
  std::string code = R"(
      typedef struct _name {
        _name() { p[0] = (void*)0x1; p[1] = (void*)0x2; p[2] = (void*)0x3; }
        void* p[3];
      } name;
    )";

  GetAllTopLevelDecls(code, Decls);
  auto *CtorD
    = (clang::CXXConstructorDecl*)Cpp::GetDefaultConstructor(Decls[0]);
  auto Ctor = Cpp::MakeFunctionCallable(CtorD);
  EXPECT_TRUE((bool)Ctor) << "Failed to build a wrapper for the ctor";
  void* object = nullptr;
  Ctor.Invoke(&object);
  EXPECT_TRUE(object) << "Failed to call the ctor.";
  // Building a wrapper with a typedef decl must be possible.
  Cpp::Destruct(object, Decls[1]);
}

#ifdef __APPLE__
TEST(FunctionReflectionTest, DISABLED_GetFunctionCallWrapper) {
#else
TEST(FunctionReflectionTest, GetFunctionCallWrapper) {
#endif
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

  Cpp::JitCall FCI1 =
      Cpp::MakeFunctionCallable(Decls[0]);
  EXPECT_TRUE(FCI1.getKind() == Cpp::JitCall::kGenericCall);
  Cpp::JitCall FCI2 =
      Cpp::MakeFunctionCallable(Cpp::GetNamed("f2"));
  EXPECT_TRUE(FCI2.getKind() == Cpp::JitCall::kGenericCall);
  Cpp::JitCall FCI3 =
    Cpp::MakeFunctionCallable(Cpp::GetNamed("f3", Cpp::GetNamed("NS")));
  EXPECT_TRUE(FCI3.getKind() == Cpp::JitCall::kGenericCall);
  Cpp::JitCall FCI4 =
      Cpp::MakeFunctionCallable(Cpp::GetNamed("f4", Cpp::GetNamed("NS")));
  EXPECT_TRUE(FCI4.getKind() == Cpp::JitCall::kGenericCall);

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

  clang::NamedDecl *ClassC = (clang::NamedDecl*)Cpp::GetNamed("C");
  auto *CtorD = (clang::CXXConstructorDecl*)Cpp::GetDefaultConstructor(ClassC);
  auto FCI_Ctor =
    Cpp::MakeFunctionCallable(CtorD);
  void* object = nullptr;
  testing::internal::CaptureStdout();
  FCI_Ctor.Invoke((void*)&object);
  output = testing::internal::GetCapturedStdout();
  EXPECT_EQ(output, "Default Ctor Called\n");
  EXPECT_TRUE(object != nullptr);

  auto *DtorD = (clang::CXXDestructorDecl*)Cpp::GetDestructor(ClassC);
  auto FCI_Dtor =
    Cpp::MakeFunctionCallable(DtorD);
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
  Cpp::TCppFunction_t method = nullptr; // Simulate an invalid method pointer

  EXPECT_TRUE(Cpp::IsConstMethod(SubDecls[1]));  // f1
  EXPECT_FALSE(Cpp::IsConstMethod(SubDecls[2])); // f2
  EXPECT_FALSE(Cpp::IsConstMethod(method));
}

TEST(FunctionReflectionTest, GetFunctionArgName) {
  std::vector<Decl*> Decls, SubDecls;
  std::string code = R"(
    void f1(int i, double d, long l, char ch) {}
    void f2(const int i, double d[], long *l, char ch[4]) {}
    )";

  GetAllTopLevelDecls(code, Decls);
  EXPECT_EQ(Cpp::GetFunctionArgName(Decls[0], 0), "i");
  EXPECT_EQ(Cpp::GetFunctionArgName(Decls[0], 1), "d");
  EXPECT_EQ(Cpp::GetFunctionArgName(Decls[0], 2), "l");
  EXPECT_EQ(Cpp::GetFunctionArgName(Decls[0], 3), "ch");
  EXPECT_EQ(Cpp::GetFunctionArgName(Decls[1], 0), "i");
  EXPECT_EQ(Cpp::GetFunctionArgName(Decls[1], 1), "d");
  EXPECT_EQ(Cpp::GetFunctionArgName(Decls[1], 2), "l");
  EXPECT_EQ(Cpp::GetFunctionArgName(Decls[1], 3), "ch");
}

TEST(FunctionReflectionTest, GetFunctionArgDefault) {
  std::vector<Decl*> Decls, SubDecls;
  std::string code = R"(
    void f1(int i, double d = 4.0, const char *s = "default", char ch = 'c') {}
    void f2(float i = 0.0, double d = 3.123, long m = 34126) {}
    )";

  GetAllTopLevelDecls(code, Decls);
  EXPECT_EQ(Cpp::GetFunctionArgDefault(Decls[0], 0), "");
  EXPECT_EQ(Cpp::GetFunctionArgDefault(Decls[0], 1), "4.");
  EXPECT_EQ(Cpp::GetFunctionArgDefault(Decls[0], 2), "\"default\"");
  EXPECT_EQ(Cpp::GetFunctionArgDefault(Decls[0], 3), "\'c\'");
  EXPECT_EQ(Cpp::GetFunctionArgDefault(Decls[1], 0), "0.");
  EXPECT_EQ(Cpp::GetFunctionArgDefault(Decls[1], 1), "3.123");
  EXPECT_EQ(Cpp::GetFunctionArgDefault(Decls[1], 2), "34126");
}

#ifdef __APPLE__
TEST(FunctionReflectionTest, DISABLED_Construct) {
#else
TEST(FunctionReflectionTest, Construct) {
#endif
  Cpp::CreateInterpreter();

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
  Cpp::TCppScope_t scope = Cpp::GetNamed("C");
  Cpp::TCppObject_t object = Cpp::Construct(scope);
  EXPECT_TRUE(object != nullptr);
  std::string output = testing::internal::GetCapturedStdout();
  EXPECT_EQ(output, "Constructor Executed");
  output.clear();

  // Placement.
  testing::internal::CaptureStdout();
  void* where = Cpp::Allocate(scope);
  EXPECT_TRUE(where == Cpp::Construct(scope, where));
  // Check for the value of x which should be at the start of the object.
  EXPECT_TRUE(*(int *)where == 12345);
  Cpp::Deallocate(scope, where);
  output = testing::internal::GetCapturedStdout();
  EXPECT_EQ(output, "Constructor Executed");
}

#ifdef __APPLE__
TEST(FunctionReflectionTest, DISABLED_Destruct) {
#else
TEST(FunctionReflectionTest, Destruct) {
#endif
  Cpp::CreateInterpreter();

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
  Cpp::TCppScope_t scope = Cpp::GetNamed("C");
  Cpp::TCppObject_t object = Cpp::Construct(scope);
  Cpp::Destruct(object, scope);
  std::string output = testing::internal::GetCapturedStdout();

  EXPECT_EQ(output, "Destructor Executed");

  output.clear();
  testing::internal::CaptureStdout();
  object = Cpp::Construct(scope);
  // Make sure we do not call delete by adding an explicit Deallocate. If we
  // called delete the Deallocate will cause a double deletion error.
  Cpp::Destruct(object, scope, /*withFree=*/false);
  Cpp::Deallocate(scope, object);
  output = testing::internal::GetCapturedStdout();
  EXPECT_EQ(output, "Destructor Executed");
}
