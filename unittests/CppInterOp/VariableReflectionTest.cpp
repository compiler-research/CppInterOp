#include "Utils.h"

#include "clang/AST/ASTContext.h"
#include "clang/Interpreter/CppInterOp.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Sema/Sema.h"

#include "gtest/gtest.h"

using namespace TestUtils;
using namespace llvm;
using namespace clang;

TEST(VariableReflectionTest, GetDatamembers) {
  std::vector<Decl*> Decls;
  std::string code = R"(
    class C {
    public:
      int a;
      static int b;
    private:
      int c;
      static int d;
    protected:
      int e;
      static int f;
    };
    void sum(int,int);
    )";

  GetAllTopLevelDecls(code, Decls);
  auto datamembers = Cpp::GetDatamembers(Decls[0]);
  auto datamembers1 = Cpp::GetDatamembers(Decls[1]);
  auto static_datamembers = Cpp::GetStaticDatamembers(Decls[0]);
  auto static_datamembers1 = Cpp::GetStaticDatamembers(Decls[1]);

  // non static field first
  EXPECT_EQ(Cpp::GetQualifiedName(datamembers[0]), "C::a");
  EXPECT_EQ(Cpp::GetQualifiedName(datamembers[1]), "C::c");
  EXPECT_EQ(Cpp::GetQualifiedName(datamembers[2]), "C::e");
  EXPECT_EQ(datamembers.size(), 3);
  EXPECT_EQ(datamembers1.size(), 0);

  // static fields
  EXPECT_EQ(Cpp::GetQualifiedName(static_datamembers[0]), "C::b");
  EXPECT_EQ(Cpp::GetQualifiedName(static_datamembers[1]), "C::d");
  EXPECT_EQ(Cpp::GetQualifiedName(static_datamembers[2]), "C::f");
  EXPECT_EQ(static_datamembers.size(), 3);
  EXPECT_EQ(static_datamembers1.size(), 0);
}

#define CODE                                                                   \
  struct Klass1 {                                                              \
    Klass1(int i) : num(1), b(i) {}                                            \
    int num;                                                                   \
    union {                                                                    \
      double a;                                                                \
      int b;                                                                   \
    };                                                                         \
  } const k1(5);                                                               \
  struct Klass2 {                                                              \
    Klass2(double d) : num(2), a(d) {}                                         \
    int num;                                                                   \
    struct {                                                                   \
      double a;                                                                \
      int b;                                                                   \
    };                                                                         \
  } const k2(2.5);                                                             \
  struct Klass3 {                                                              \
    Klass3(int i) : num(i) {}                                                  \
    int num;                                                                   \
    struct {                                                                   \
      double a;                                                                \
      union {                                                                  \
        float b;                                                               \
        int c;                                                                 \
      };                                                                       \
    };                                                                         \
    int num2;                                                                  \
  } const k3(5);

CODE

TEST(VariableReflectionTest, DatamembersWithAnonymousStructOrUnion) {
  if (llvm::sys::RunningOnValgrind())
    GTEST_SKIP() << "XFAIL due to Valgrind report";

  std::vector<Decl*> Decls;
#define Stringify(s) Stringifyx(s)
#define Stringifyx(...) #__VA_ARGS__
  GetAllTopLevelDecls(Stringify(CODE), Decls);
#undef Stringifyx
#undef Stringify
#undef CODE

  auto datamembers_klass1 = Cpp::GetDatamembers(Decls[0]);
  auto datamembers_klass2 = Cpp::GetDatamembers(Decls[2]);
  auto datamembers_klass3 = Cpp::GetDatamembers(Decls[4]);

  EXPECT_EQ(datamembers_klass1.size(), 3);
  EXPECT_EQ(datamembers_klass2.size(), 3);

  EXPECT_EQ(Cpp::GetVariableOffset(datamembers_klass1[0]), 0);
  EXPECT_EQ(Cpp::GetVariableOffset(datamembers_klass1[1]),
            ((intptr_t) & (k1.a)) - ((intptr_t) & (k1.num)));
  EXPECT_EQ(Cpp::GetVariableOffset(datamembers_klass1[2]),
            ((intptr_t) & (k1.b)) - ((intptr_t) & (k1.num)));

  EXPECT_EQ(Cpp::GetVariableOffset(datamembers_klass2[0]), 0);
  EXPECT_EQ(Cpp::GetVariableOffset(datamembers_klass2[1]),
            ((intptr_t) & (k2.a)) - ((intptr_t) & (k2.num)));
  EXPECT_EQ(Cpp::GetVariableOffset(datamembers_klass2[2]),
            ((intptr_t) & (k2.b)) - ((intptr_t) & (k2.num)));

  EXPECT_EQ(Cpp::GetVariableOffset(datamembers_klass3[0]), 0);
  EXPECT_EQ(Cpp::GetVariableOffset(datamembers_klass3[1]),
            ((intptr_t) & (k3.a)) - ((intptr_t) & (k3.num)));
  EXPECT_EQ(Cpp::GetVariableOffset(datamembers_klass3[2]),
            ((intptr_t) & (k3.b)) - ((intptr_t) & (k3.num)));
  EXPECT_EQ(Cpp::GetVariableOffset(datamembers_klass3[3]),
            ((intptr_t) & (k3.c)) - ((intptr_t) & (k3.num)));
  EXPECT_EQ(Cpp::GetVariableOffset(datamembers_klass3[4]),
            ((intptr_t) & (k3.num2)) - ((intptr_t) & (k3.num)));
}

TEST(VariableReflectionTest, LookupDatamember) {
  std::vector<Decl*> Decls;
  std::string code = R"(
    class C {
    public:
      int a;
      static int b;
    private:
      int c;
      static int d;
    protected:
      int e;
      static int f;
    };
    )";

  GetAllTopLevelDecls(code, Decls);

  EXPECT_EQ(Cpp::GetQualifiedName(Cpp::LookupDatamember("a", Decls[0])), "C::a");
  EXPECT_EQ(Cpp::GetQualifiedName(Cpp::LookupDatamember("c", Decls[0])), "C::c");
  EXPECT_EQ(Cpp::GetQualifiedName(Cpp::LookupDatamember("e", Decls[0])), "C::e");
  EXPECT_EQ(Cpp::GetQualifiedName(Cpp::LookupDatamember("k", Decls[0])), "<unnamed>");
}

TEST(VariableReflectionTest, GetVariableType) {
  std::vector<Decl*> Decls;
  std::string code = R"(
    class C {};

    template<typename T>
    class E {};

    int a;
    char b;
    C c;
    C *d;
    E<int> e;
    E<int> *f;
    int g[4];
    )";

  GetAllTopLevelDecls(code, Decls);

  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetVariableType(Decls[2])), "int");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetVariableType(Decls[3])), "char");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetVariableType(Decls[4])), "C");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetVariableType(Decls[5])), "C *");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetVariableType(Decls[6])), "E<int>");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetVariableType(Decls[7])), "E<int> *");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetVariableType(Decls[8])), "int[4]");
}

#define CODE                                                    \
  int a;                                                        \
  const int N = 5;                                              \
  class C {                                                     \
  public:                                                       \
    int a;                                                      \
    double b;                                                   \
    int *c;                                                     \
    int d;                                                      \
    static int s_a;                                             \
  } c;                                                            \
  int C::s_a = 7 + N;

CODE

TEST(VariableReflectionTest, GetVariableOffset) {
  std::vector<Decl *> Decls;
#define Stringify(s) Stringifyx(s)
#define Stringifyx(...) #__VA_ARGS__
  GetAllTopLevelDecls(Stringify(CODE), Decls);
#undef Stringifyx
#undef Stringify
#undef CODE

  auto datamembers = Cpp::GetDatamembers(Decls[2]);

  EXPECT_TRUE((bool) Cpp::GetVariableOffset(Decls[0])); // a
  EXPECT_TRUE((bool) Cpp::GetVariableOffset(Decls[1])); // N

  EXPECT_EQ(Cpp::GetVariableOffset(datamembers[0]), 0);

  EXPECT_EQ(Cpp::GetVariableOffset(datamembers[1]),
          ((intptr_t) &(c.b)) - ((intptr_t) &(c.a)));
  EXPECT_EQ(Cpp::GetVariableOffset(datamembers[2]),
          ((intptr_t) &(c.c)) - ((intptr_t) &(c.a)));
  EXPECT_EQ(Cpp::GetVariableOffset(datamembers[3]),
          ((intptr_t) &(c.d)) - ((intptr_t) &(c.a)));

  auto *VD_C_s_a = Cpp::GetNamed("s_a", Decls[2]); // C::s_a
  EXPECT_TRUE((bool) Cpp::GetVariableOffset(VD_C_s_a));
}

TEST(VariableReflectionTest, IsPublicVariable) {
  std::vector<Decl *> Decls, SubDecls;
  std::string code = R"(
    class C {
    public:
      int a;
    private:
      int b;
    protected:
      int c;
      int sum(int,int);
    };
    )";

  GetAllTopLevelDecls(code, Decls);
  GetAllSubDecls(Decls[0], SubDecls);

  EXPECT_TRUE(Cpp::IsPublicVariable(SubDecls[2]));
  EXPECT_FALSE(Cpp::IsPublicVariable(SubDecls[4]));
  EXPECT_FALSE(Cpp::IsPublicVariable(SubDecls[6]));
  EXPECT_FALSE(Cpp::IsPublicVariable(SubDecls[7]));
}

TEST(VariableReflectionTest, IsProtectedVariable) {
  std::vector<Decl *> Decls, SubDecls;
  std::string code = R"(
    class C {
    public:
      int a;
    private:
      int b;
    protected:
      int c;
    };
    )";

  GetAllTopLevelDecls(code, Decls);
  GetAllSubDecls(Decls[0], SubDecls);

  EXPECT_FALSE(Cpp::IsProtectedVariable(SubDecls[2]));
  EXPECT_FALSE(Cpp::IsProtectedVariable(SubDecls[4]));
  EXPECT_TRUE(Cpp::IsProtectedVariable(SubDecls[6]));
}

TEST(VariableReflectionTest, IsPrivateVariable) {
  std::vector<Decl *> Decls, SubDecls;
  std::string code = R"(
    class C {
    public:
      int a;
    private:
      int b;
    protected:
      int c;
    };
    )";

  GetAllTopLevelDecls(code, Decls);
  GetAllSubDecls(Decls[0], SubDecls);

  EXPECT_FALSE(Cpp::IsPrivateVariable(SubDecls[2]));
  EXPECT_TRUE(Cpp::IsPrivateVariable(SubDecls[4]));
  EXPECT_FALSE(Cpp::IsPrivateVariable(SubDecls[6]));
}

TEST(VariableReflectionTest, IsStaticVariable) {
  std::vector<Decl *> Decls, SubDecls;
  std::string code =  R"(
    class C {
      int a;
      static int b;
    };
    )";

  GetAllTopLevelDecls(code, Decls);
  GetAllSubDecls(Decls[0], SubDecls);

  EXPECT_FALSE(Cpp::IsStaticVariable(SubDecls[1]));
  EXPECT_TRUE(Cpp::IsStaticVariable(SubDecls[2]));
}

TEST(VariableReflectionTest, IsConstVariable) {
  std::vector<Decl *> Decls, SubDecls;
  std::string code =  R"(
    class C {
      int a;
      const int b = 2;
    };
    )";

  GetAllTopLevelDecls(code, Decls);
  GetAllSubDecls(Decls[0], SubDecls);

  EXPECT_FALSE(Cpp::IsConstVariable(Decls[0]));
  EXPECT_FALSE(Cpp::IsConstVariable(SubDecls[1]));
  EXPECT_TRUE(Cpp::IsConstVariable(SubDecls[2]));
}

TEST(VariableReflectionTest, DISABLED_GetArrayDimensions) {
  std::vector<Decl *> Decls, SubDecls;
  std::string code =  R"(
    int a;
    int b[1];
    int c[1][2];
    )";

  GetAllTopLevelDecls(code, Decls);

  //FIXME: is_vec_eq is an unused variable until test does something
  //auto is_vec_eq = [](const std::vector<size_t> &arr_dims,
  //                    const std::vector<size_t> &truth_vals) {
  //  if (arr_dims.size() != truth_vals.size()) 
  //    return false;
  //
  //  return std::equal(arr_dims.begin(), arr_dims.end(), truth_vals.begin());
  //};

  // EXPECT_TRUE(is_vec_eq(Cpp::GetArrayDimensions(Decls[0]), {}));
  // EXPECT_TRUE(is_vec_eq(Cpp::GetArrayDimensions(Decls[1]), {1}));
  // EXPECT_TRUE(is_vec_eq(Cpp::GetArrayDimensions(Decls[2]), {1,2}));
}
