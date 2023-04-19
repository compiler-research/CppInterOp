#include "Utils.h"

#include "clang/AST/ASTContext.h"
#include "clang/Interpreter/InterOp.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Sema/Sema.h"

#include "gtest/gtest.h"

using namespace TestUtils;
using namespace llvm;
using namespace clang;

TEST(ScopeReflectionTest, IsEnumScope) {
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
  EXPECT_TRUE(InterOp::IsEnumScope(Decls[0]));
  EXPECT_FALSE(InterOp::IsEnumScope(Decls[1]));
  EXPECT_FALSE(InterOp::IsEnumScope(Decls[2]));
  EXPECT_FALSE(InterOp::IsEnumScope(SubDecls[0]));
  EXPECT_FALSE(InterOp::IsEnumScope(SubDecls[1]));
}

TEST(ScopeReflectionTest, IsEnumConstant) {
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
  EXPECT_FALSE(InterOp::IsEnumConstant(Decls[0]));
  EXPECT_FALSE(InterOp::IsEnumConstant(Decls[1]));
  EXPECT_FALSE(InterOp::IsEnumConstant(Decls[2]));
  EXPECT_TRUE(InterOp::IsEnumConstant(SubDecls[0]));
  EXPECT_TRUE(InterOp::IsEnumConstant(SubDecls[1]));
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

  EXPECT_TRUE(InterOp::IsEnumType(InterOp::GetVariableType(Decls[2])));
  EXPECT_TRUE(InterOp::IsEnumType(InterOp::GetVariableType(Decls[3])));
  EXPECT_TRUE(InterOp::IsEnumType(InterOp::GetVariableType(Decls[4])));
  EXPECT_TRUE(InterOp::IsEnumType(InterOp::GetVariableType(Decls[5])));
}

TEST(EnumReflectionTest, GetIntegerTypeFromEnumScope) {
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

  EXPECT_EQ(InterOp::GetTypeAsString(InterOp::GetIntegerTypeFromEnumScope(Decls[0])), "bool");
  EXPECT_EQ(InterOp::GetTypeAsString(InterOp::GetIntegerTypeFromEnumScope(Decls[1])), "char");
  EXPECT_EQ(InterOp::GetTypeAsString(InterOp::GetIntegerTypeFromEnumScope(Decls[2])), "int");
  EXPECT_EQ(InterOp::GetTypeAsString(InterOp::GetIntegerTypeFromEnumScope(Decls[3])), "long long");
  EXPECT_EQ(InterOp::GetTypeAsString(InterOp::GetIntegerTypeFromEnumScope(Decls[4])), "unsigned int");
}

TEST(EnumReflectionTest, GetIntegerTypeFromEnumType) {
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

    Switch s;
    CharEnum ch;
    IntEnum in;
    LongEnum lng;
    DefaultEnum def;
  )";

  GetAllTopLevelDecls(code, Decls);

  auto get_int_type_from_enum_var = [](Decl *D) {
    return InterOp::GetTypeAsString(InterOp::GetIntegerTypeFromEnumType(InterOp::GetVariableType(D)));
  };

  EXPECT_EQ(get_int_type_from_enum_var(Decls[5]), "bool");
  EXPECT_EQ(get_int_type_from_enum_var(Decls[6]), "char");
  EXPECT_EQ(get_int_type_from_enum_var(Decls[7]), "int");
  EXPECT_EQ(get_int_type_from_enum_var(Decls[8]), "long long");
  EXPECT_EQ(get_int_type_from_enum_var(Decls[9]), "unsigned int");
}

TEST(EnumReflectionTest, GetEnumConstants) {
  std::vector<Decl *> Decls;
  std::string code = R"(
    enum ZeroEnum {
    };

    enum OneEnum {
      One_OneEnum,
    };

    enum TwoEnum {
      One_TwoEnum,
      Two_TwoEnum,
    };

    enum ThreeEnum {
      One_ThreeEnum,
      Two_ThreeEnum,
      Three_ThreeEnum,
    };

    enum FourEnum {
      One_FourEnum,
      Two_FourEnum,
      Three_FourEnum,
      Four_FourEnum,
    };
  )";

  GetAllTopLevelDecls(code, Decls);

  EXPECT_EQ(InterOp::GetEnumConstants(Decls[0]).size(), 0);
  EXPECT_EQ(InterOp::GetEnumConstants(Decls[1]).size(), 1);
  EXPECT_EQ(InterOp::GetEnumConstants(Decls[2]).size(), 2);
  EXPECT_EQ(InterOp::GetEnumConstants(Decls[3]).size(), 3);
  EXPECT_EQ(InterOp::GetEnumConstants(Decls[4]).size(), 4);
}

TEST(EnumReflectionTest, GetEnumConstantType) {
  std::vector<Decl *> Decls;
  std::string code = R"(
    enum Enum0 {
      Constant0 = 0
    };

    enum class Enum1 {
      Constant1 = 1
    };
  )";

  GetAllTopLevelDecls(code, Decls);

  auto get_enum_constant_type_as_str = [](InterOp::TCppScope_t enum_constant) {
    return InterOp::GetTypeAsString(InterOp::GetEnumConstantType(enum_constant));
  };

  auto EnumConstants0 = InterOp::GetEnumConstants(Decls[0]);

  EXPECT_EQ(get_enum_constant_type_as_str(EnumConstants0[0]), "Enum0");

  auto EnumConstants1 = InterOp::GetEnumConstants(Decls[1]);

  EXPECT_EQ(get_enum_constant_type_as_str(EnumConstants1[0]), "Enum1");
}

TEST(EnumReflectionTest, GetEnumConstantValue) {
  std::vector<Decl *> Decls;
  std::string code = R"(
    enum Counter {
      Zero = 0,
      One,
      FiftyTwo = 52,
      FiftyThree,
      FiftyFour,
      MinusTen = -10,
      MinusNine
    };
  )";

  GetAllTopLevelDecls(code, Decls);
  auto EnumConstants = InterOp::GetEnumConstants(Decls[0]);

  EXPECT_EQ(InterOp::GetEnumConstantValue(EnumConstants[0]), 0);
  EXPECT_EQ(InterOp::GetEnumConstantValue(EnumConstants[1]), 1);
  EXPECT_EQ(InterOp::GetEnumConstantValue(EnumConstants[2]), 52);
  EXPECT_EQ(InterOp::GetEnumConstantValue(EnumConstants[3]), 53);
  EXPECT_EQ(InterOp::GetEnumConstantValue(EnumConstants[4]), 54);
  EXPECT_EQ(InterOp::GetEnumConstantValue(EnumConstants[5]), -10);
  EXPECT_EQ(InterOp::GetEnumConstantValue(EnumConstants[6]), -9);
}
