#include "Utils.h"

#include "clang/Interpreter/CppInterOp.h"

#include "gtest/gtest.h"

TEST(ScopeReflectionTest, IsEnum) {
  std::vector<Decl *> Decls, SubDecls;
  std::string code = R"(
    enum Switch {
      OFF,
      ON
    };

    Switch s = Switch::OFF;

    int i = Switch::ON;
  )";

  GetAllTopLevelDecls(code, Decls);
  GetAllSubDecls(Decls[0], SubDecls);
  EXPECT_TRUE(Cpp::IsEnum(Decls[0]));
  EXPECT_FALSE(Cpp::IsEnum(Decls[1]));
  EXPECT_FALSE(Cpp::IsEnum(Decls[2]));
  EXPECT_TRUE(Cpp::IsEnum(SubDecls[0]));
  EXPECT_TRUE(Cpp::IsEnum(SubDecls[1]));
}

TEST(EnumReflectionTest, IsEnumType) {
  std::vector<Decl *> Decls;
  std::string code =  R"(
    enum class E {
      a,
      b
    };

    enum F {
      c,
      d
    };

    E e;
    F f;

    auto g = E::a;
    auto h = c;
    )";

  GetAllTopLevelDecls(code, Decls);

  EXPECT_TRUE(Cpp::IsEnumType(Cpp::GetVariableType(Decls[2])));
  EXPECT_TRUE(Cpp::IsEnumType(Cpp::GetVariableType(Decls[3])));
  EXPECT_TRUE(Cpp::IsEnumType(Cpp::GetVariableType(Decls[4])));
  EXPECT_TRUE(Cpp::IsEnumType(Cpp::GetVariableType(Decls[5])));
}

TEST(EnumReflectionTest, GetEnumIntegerType) {
  std::vector<Decl *> Decls;
  std::string code = R"(
    enum Switch : bool {
      OFF,
      ON
    };

    enum CharEnum : char {
      OneChar,
      TwoChar
    };

    enum IntEnum : int {
      OneInt,
      TwoInt
    };

    enum LongEnum : long long {
      OneLong,
      TwoLong
    };

    enum DefaultEnum {
      OneDefault,
      TwoDefault
    };
  )";

  GetAllTopLevelDecls(code, Decls);

  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetEnumIntegerType(Decls[0])), "_Bool");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetEnumIntegerType(Decls[1])), "char");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetEnumIntegerType(Decls[2])), "int");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetEnumIntegerType(Decls[3])),
            "long long");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetEnumIntegerType(Decls[4])),
            "unsigned int");
}
