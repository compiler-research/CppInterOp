
#include "Utils.h"

#include "clang/Interpreter/CppInterOp.h"

#include "gtest/gtest.h"


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
    )";

  GetAllTopLevelDecls(code, Decls);
  auto datamembers = Cpp::GetDatamembers(Decls[0]);

  EXPECT_EQ(Cpp::GetCompleteName(datamembers[0]), "C::a");
  EXPECT_EQ(Cpp::GetCompleteName(datamembers[1]), "C::c");
  EXPECT_EQ(Cpp::GetCompleteName(datamembers[2]), "C::e");
  EXPECT_EQ(datamembers.size(), 3);
}

TEST(VariableReflectionTest, GetVariableTypeAsString) {
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
    )";

  GetAllTopLevelDecls(code, Decls);

  EXPECT_EQ(Cpp::GetVariableTypeAsString(Decls[2]), "int");
  EXPECT_EQ(Cpp::GetVariableTypeAsString(Decls[3]), "char");
  EXPECT_EQ(Cpp::GetVariableTypeAsString(Decls[4]), "class C");
  EXPECT_EQ(Cpp::GetVariableTypeAsString(Decls[5]), "class C *");
  EXPECT_EQ(Cpp::GetVariableTypeAsString(Decls[6]), "E<int>");
  EXPECT_EQ(Cpp::GetVariableTypeAsString(Decls[7]), "E<int> *");
}

TEST(VariableReflectionTest, GetVariableOffset) {
  std::vector<Decl *> Decls;
  std::string code = R"(
    int a;
    class C {
    public:
      int a;
      double b;
      int *c;
      int d;
    };
    )";

  class {
  public:
    int a;
    double b;
    int *c;
    int d;
  } c;

  GetAllTopLevelDecls(code, Decls);
  auto datamembers = Cpp::GetDatamembers(Decls[1]);

  EXPECT_FALSE(Cpp::GetVariableOffset(Interp.get(), Decls[0]) == (intptr_t)0);

  EXPECT_EQ(Cpp::GetVariableOffset(Interp.get(), datamembers[0]), 0);
  EXPECT_EQ(Cpp::GetVariableOffset(Interp.get(), datamembers[1]),
            ((unsigned long)&(c.b)) - ((unsigned long)&(c.a)));
  EXPECT_EQ(Cpp::GetVariableOffset(Interp.get(), datamembers[2]),
            ((unsigned long)&(c.c)) - ((unsigned long)&(c.a)));
  EXPECT_EQ(Cpp::GetVariableOffset(Interp.get(), datamembers[3]),
            ((unsigned long)&(c.d)) - ((unsigned long)&(c.a)));
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
    };
    )";

  GetAllTopLevelDecls(code, Decls);
  GetAllSubDecls(Decls[0], SubDecls);

  EXPECT_TRUE(Cpp::IsPublicVariable(SubDecls[2]));
  EXPECT_FALSE(Cpp::IsPublicVariable(SubDecls[4]));
  EXPECT_FALSE(Cpp::IsPublicVariable(SubDecls[6]));
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

  EXPECT_FALSE(Cpp::IsConstVariable(SubDecls[1]));
  EXPECT_TRUE(Cpp::IsConstVariable(SubDecls[2]));
}
