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
    )";

  GetAllTopLevelDecls(code, Decls);
  Sema *S = &Interp->getCI()->getSema();
  auto methods = Cpp::GetClassMethods(S, Decls[0]);

  auto get_method_name = [](Cpp::TCppFunction_t method) {
    return Cpp::GetQualifiedName(method);
  };

  EXPECT_EQ(get_method_name(methods[0]), "A::f1");
  EXPECT_EQ(get_method_name(methods[1]), "A::f2");
  EXPECT_EQ(get_method_name(methods[2]), "A::f3");
  EXPECT_EQ(get_method_name(methods[3]), "A::f4");
  EXPECT_EQ(get_method_name(methods[4]), "A::f5");
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
    Sema *S = &Interp->getCI()->getSema();
    auto methods = Cpp::GetClassMethods(S, D);
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
  EXPECT_FALSE(Cpp::GetDestructor(Decls[0]));
  EXPECT_TRUE(Cpp::GetDestructor(Decls[1]));
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
    )";

  GetAllTopLevelDecls(code, Decls);

  // This lambda can take in the scope and the name of the function
  // and check if GetFunctionsUsingName is returning a vector of functions
  // with size equal to number_of_overloads
  auto test_get_funcs_using_name = [&](Cpp::TCppScope_t scope,
                                       const std::string name,
                                       std::size_t number_of_overloads) {
    Sema *S = &Interp->getCI()->getSema();
    auto Funcs = Cpp::GetFunctionsUsingName(S, scope, name);

    // Check if the number of functions returned is equal to the
    // number_of_overloads given by the user
    EXPECT_TRUE(Funcs.size() == number_of_overloads);
    for (auto *F : Funcs) {
      // Check if the fully scoped name of the function matches its
      // expected fully scoped name
      EXPECT_EQ(Cpp::GetQualifiedName(F),
                Cpp::GetQualifiedName(scope) + "::" + name);
    }
  };

  test_get_funcs_using_name(Decls[0], "f1", 3);
  test_get_funcs_using_name(Decls[0], "f2", 1);
  test_get_funcs_using_name(Decls[0], "f3", 1);
  test_get_funcs_using_name(Decls[1], "f4", 2);
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

  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetFunctionReturnType(Decls[3])), "void");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetFunctionReturnType(Decls[4])), "double");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetFunctionReturnType(Decls[5])), "Switch");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetFunctionReturnType(Decls[6])), "N::C");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetFunctionReturnType(Decls[7])), "N::C *");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetFunctionReturnType(Decls[8])), "const N::C");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetFunctionReturnType(Decls[9])), "volatile N::C");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetFunctionReturnType(Decls[10])), "const volatile N::C");
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

TEST(FunctionReflectionTest, GetFunctionPrototype) {
  std::vector<Decl *> Decls, SubDecls;
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

  auto test_func_proto = [](Decl *D, bool formal_args, std::string proto) {
    EXPECT_EQ(Cpp::GetFunctionPrototype(D, formal_args), proto);
  };

  test_func_proto(Decls[2], false, "void f1()"); // f1
  test_func_proto(Decls[2], true, "void f1()");  // f1
  test_func_proto(Decls[3], false,
                  "C f2(int, double, long, char)"); // f2
  test_func_proto(Decls[3], true,
                  "C f2(int i, double d, long l = 0, char ch = 'a')"); // f2
  test_func_proto(Decls[4], false,
                  "C *f3(int, double, long, char)"); // f3
  test_func_proto(Decls[4], true,
                  "C *f3(int i, double d, long l = 0, char ch = 'a')"); // f3
  test_func_proto(Decls[5], false,
                  "void f4(int, double, long, char)"); // f4
  test_func_proto(
      Decls[5], true,
      "void f4(int i = 0, double d = 0., long l = 0, char ch = 'a')"); // f4
  test_func_proto(Decls[7], false,
                  "void C::f(int, double, long, char)"); // C::f
  test_func_proto(
      Decls[7], true,
      "void C::f(int i, double d, long l = 0, char ch = 'a')"); // C::f
  test_func_proto(Decls[12], false,
                  "void N::f(int, double, long, char)"); // N::f
  test_func_proto(
      Decls[12], true,
      "void N::f(int i, double d, long l = 0, char ch = 'a')"); // N::f
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
  auto *S = &Interp->getCI()->getSema();

  EXPECT_TRUE(Cpp::ExistsFunctionTemplate(S, "f", 0));
  EXPECT_TRUE(Cpp::ExistsFunctionTemplate(S, "f", Decls[1]));
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

TEST(FunctionReflectionTest, GetFunctionAddress) {
  std::vector<Decl*> Decls, SubDecls;
  std::string code = "int f1(int i) { return i * i; }";

  GetAllTopLevelDecls(code, Decls);

  testing::internal::CaptureStdout();
  Interp->declare("#include <iostream>");
  Interp->process(
      "void * address = (void *) &f1; \n"
#if USE_REPL && CLANG_VERSION_MAJOR < 16
      "int run() {std::cout << address; return 0;} int result = run(); \n"
#else
      "std::cout << address; \n"
#endif
  );

  std::string output = testing::internal::GetCapturedStdout();
  std::stringstream address;
  address << Cpp::GetFunctionAddress((Cpp::TInterp_t)Interp.get(), Decls[0]);
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

  Sema *S = &Interp->getCI()->getSema();

  Cpp::CallFuncWrapper_t wrapper0 =
      Cpp::GetFunctionCallWrapper((Cpp::TInterp_t)Interp.get(), Decls[0]);
  Cpp::CallFuncWrapper_t wrapper1 = Cpp::GetFunctionCallWrapper(
      (Cpp::TInterp_t)Interp.get(), Cpp::GetNamed(S, "f2"));
  int i = 9, ret;
  std::string s("Hello World!\n");
  void *args0[1] = { (void *) &i };
  void *args1[1] = { (void *) &s };

  wrapper0(0, 1, args0, &ret);

  testing::internal::CaptureStdout();
  wrapper1(0, 1, args1, 0);
  std::string output = testing::internal::GetCapturedStdout();

  EXPECT_EQ(ret, i * i);
  EXPECT_EQ(output, s);
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

  EXPECT_TRUE(Cpp::IsConstMethod(SubDecls[1])); // f1
  EXPECT_FALSE(Cpp::IsConstMethod(SubDecls[2])); // f2
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

TEST(FunctionReflectionTest, DISABLED_Construct) {
  Interp.reset(static_cast<compat::Interpreter *>(Cpp::CreateInterpreter()));
  Sema *S = &Interp->getCI()->getSema();

  Interp->declare(R"(
    class C {
      C() {
        printf("Constructor Executed");
      }
    };
    )");

  testing::internal::CaptureStdout();
  Cpp::TCppType_t type = Cpp::GetTypeFromScope(Cpp::GetNamed(S, "C"));
  // TCppObject_t object = Cpp::Construct(type);
  std::string output = testing::internal::GetCapturedStdout();

  EXPECT_EQ(output, "Constructor Executed");
}

TEST(FunctionReflectionTest, DISABLED_Destruct) {
  Interp.reset(static_cast<compat::Interpreter *>(Cpp::CreateInterpreter()));
  Sema *S = &Interp->getCI()->getSema();

  Interp->declare(R"(
    class C {
      C() {}
      ~C() {
        printf("Destructor Executed");
      }
    };
    )");

  testing::internal::CaptureStdout();
  Cpp::TCppType_t type = Cpp::GetTypeFromScope(Cpp::GetNamed(S, "C"));
  // TCppObject_t object = Cpp::Construct(type);
  // Cpp::Destruct(object, type)
  std::string output = testing::internal::GetCapturedStdout();

  EXPECT_EQ(output, "Destructor Executed");
}
