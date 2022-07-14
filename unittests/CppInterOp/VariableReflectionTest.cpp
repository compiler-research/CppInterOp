
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
