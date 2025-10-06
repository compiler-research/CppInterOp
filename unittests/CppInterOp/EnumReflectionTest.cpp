#include "Utils.h"

#include "CppInterOp/CppInterOp.h"

#include "clang/AST/ASTContext.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Sema/Sema.h"

#include "gtest/gtest.h"

#include <utility>

using namespace TestUtils;
using namespace llvm;
using namespace clang;

static void EnumReflectionTest_IsEnumScope() {
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
  EXPECT_TRUE(Cpp::IsEnumScope(Decls[0]));
  EXPECT_FALSE(Cpp::IsEnumScope(Decls[1]));
  EXPECT_FALSE(Cpp::IsEnumScope(Decls[2]));
  EXPECT_FALSE(Cpp::IsEnumScope(SubDecls[0]));
  EXPECT_FALSE(Cpp::IsEnumScope(SubDecls[1]));
}

static void EnumReflectionTest_IsEnumConstant() {
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
  EXPECT_FALSE(Cpp::IsEnumConstant(Decls[0]));
  EXPECT_FALSE(Cpp::IsEnumConstant(Decls[1]));
  EXPECT_FALSE(Cpp::IsEnumConstant(Decls[2]));
  EXPECT_TRUE(Cpp::IsEnumConstant(SubDecls[0]));
  EXPECT_TRUE(Cpp::IsEnumConstant(SubDecls[1]));
}

static void EnumReflectionTest_IsEnumType() {
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

static void EnumReflectionTest_GetIntegerTypeFromEnumScope() {
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

    // Non enum type
    struct Employee {
      int id;
      int salary;
    };
  )";

  GetAllTopLevelDecls(code, Decls);

  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetIntegerTypeFromEnumScope(Decls[0])), "bool");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetIntegerTypeFromEnumScope(Decls[1])), "char");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetIntegerTypeFromEnumScope(Decls[2])), "int");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetIntegerTypeFromEnumScope(Decls[3])), "long long");
#ifdef _WIN32
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetIntegerTypeFromEnumScope(Decls[4])),
            "int");
#else
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetIntegerTypeFromEnumScope(Decls[4])), "unsigned int");
#endif
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetIntegerTypeFromEnumScope(Decls[5])),"NULL TYPE");
}

static void EnumReflectionTest_GetIntegerTypeFromEnumType() {
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

    struct Employee {
      int id;
      int salary;
    };

    Switch s;
    CharEnum ch;
    IntEnum in;
    LongEnum lng;
    DefaultEnum def;
    Employee emp;
  )";

  GetAllTopLevelDecls(code, Decls);

  auto get_int_type_from_enum_var = [](Decl *D) {
    return Cpp::GetTypeAsString(Cpp::GetIntegerTypeFromEnumType(Cpp::GetVariableType(D)));
  };

  EXPECT_EQ(get_int_type_from_enum_var(Decls[5]), "NULL TYPE"); // When a nullptr is returned by GetVariableType()
  EXPECT_EQ(get_int_type_from_enum_var(Decls[6]), "bool");
  EXPECT_EQ(get_int_type_from_enum_var(Decls[7]), "char");
  EXPECT_EQ(get_int_type_from_enum_var(Decls[8]), "int");
  EXPECT_EQ(get_int_type_from_enum_var(Decls[9]), "long long");
#ifdef _WIN32
  EXPECT_EQ(get_int_type_from_enum_var(Decls[10]), "int");
#else
  EXPECT_EQ(get_int_type_from_enum_var(Decls[10]), "unsigned int");
#endif
  EXPECT_EQ(get_int_type_from_enum_var(Decls[11]), "NULL TYPE"); // When a non Enum Type variable is used
}

static void EnumReflectionTest_GetEnumConstants() {
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

    struct Employee {
      int id;
      int salary;
    };
  )";

  GetAllTopLevelDecls(code, Decls);

  EXPECT_EQ(Cpp::GetEnumConstants(Decls[0]).size(), 0);
  EXPECT_EQ(Cpp::GetEnumConstants(Decls[1]).size(), 1);
  EXPECT_EQ(Cpp::GetEnumConstants(Decls[2]).size(), 2);
  EXPECT_EQ(Cpp::GetEnumConstants(Decls[3]).size(), 3);
  EXPECT_EQ(Cpp::GetEnumConstants(Decls[4]).size(), 4);
  EXPECT_EQ(Cpp::GetEnumConstants(Decls[5]).size(), 0);
}

static void EnumReflectionTest_GetEnumConstantType() {
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

  auto get_enum_constant_type_as_str = [](Cpp::TCppScope_t enum_constant) {
    return Cpp::GetTypeAsString(Cpp::GetEnumConstantType(enum_constant));
  };

  auto EnumConstants0 = Cpp::GetEnumConstants(Decls[0]);

  EXPECT_EQ(get_enum_constant_type_as_str(EnumConstants0[0]), "Enum0");

  auto EnumConstants1 = Cpp::GetEnumConstants(Decls[1]);

  EXPECT_EQ(get_enum_constant_type_as_str(EnumConstants1[0]), "Enum1");

  EXPECT_EQ(get_enum_constant_type_as_str(Decls[1]), "NULL TYPE");

  EXPECT_EQ(get_enum_constant_type_as_str(nullptr), "NULL TYPE");
}

static void EnumReflectionTest_GetEnumConstantValue() {
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
    int a = 10;
  )";

  GetAllTopLevelDecls(code, Decls);
  auto EnumConstants = Cpp::GetEnumConstants(Decls[0]);

  EXPECT_EQ(Cpp::GetEnumConstantValue(EnumConstants[0]), 0);
  EXPECT_EQ(Cpp::GetEnumConstantValue(EnumConstants[1]), 1);
  EXPECT_EQ(Cpp::GetEnumConstantValue(EnumConstants[2]), 52);
  EXPECT_EQ(Cpp::GetEnumConstantValue(EnumConstants[3]), 53);
  EXPECT_EQ(Cpp::GetEnumConstantValue(EnumConstants[4]), 54);
  EXPECT_EQ(Cpp::GetEnumConstantValue(EnumConstants[5]), -10);
  EXPECT_EQ(Cpp::GetEnumConstantValue(EnumConstants[6]), -9);
  EXPECT_EQ(Cpp::GetEnumConstantValue(Decls[1]), 0); // Checking value of non enum constant
}

static void EnumReflectionTest_GetEnums() {
  std::string code = R"(
    enum Color {
      Red,
      Green,
      Blue
    };

    enum Days {
      Monday,
      Tuesday,
      Wednesday,
      Thursday,
    };

    namespace Animals {
      enum AnimalType {
        Dog,
        Cat,
        Bird
      };

      enum Months {
        January,
        February,
        March
      };
    }

    class myClass {
      public:
        enum Color {
          Red,
          Green,
          Blue
        };
    };

    int myVariable;
    )";

  Cpp::TInterp_t I = Cpp::CreateInterpreter();
  Cpp::Declare(code.c_str(), false, I);
  std::vector<std::string> enumNames1, enumNames2, enumNames3, enumNames4;
  Cpp::TCppScope_t globalscope = Cpp::GetScope("", nullptr, I);
  Cpp::TCppScope_t Animals_scope = Cpp::GetScope("Animals", nullptr, I);
  Cpp::TCppScope_t myClass_scope = Cpp::GetScope("myClass", nullptr, I);
  Cpp::TCppScope_t unsupported_scope = Cpp::GetScope("myVariable", nullptr, I);

  Cpp::GetEnums(globalscope,enumNames1);
  Cpp::GetEnums(Animals_scope,enumNames2);
  Cpp::GetEnums(myClass_scope, enumNames3);
  Cpp::GetEnums(unsupported_scope, enumNames4);

  // Check if the enum names are correctly retrieved
  EXPECT_TRUE(std::find(enumNames1.begin(), enumNames1.end(), "Color") != enumNames1.end());
  EXPECT_TRUE(std::find(enumNames1.begin(), enumNames1.end(), "Days") != enumNames1.end());
  EXPECT_TRUE(std::find(enumNames2.begin(), enumNames2.end(), "AnimalType") != enumNames2.end());
  EXPECT_TRUE(std::find(enumNames2.begin(), enumNames2.end(), "Months") != enumNames2.end());
  EXPECT_TRUE(std::find(enumNames3.begin(), enumNames3.end(), "Color") != enumNames3.end());
  EXPECT_TRUE(enumNames4.empty());
}

TEST(EnumReflectionTest, EnumReflectionTest) {
  std::vector<std::pair<const char*, void (*)()>> fns = {
      {"EnumReflectionTest_IsEnumScope", EnumReflectionTest_IsEnumScope},
      {"EnumReflectionTest_IsEnumConstant", EnumReflectionTest_IsEnumConstant},
      {"EnumReflectionTest_IsEnumType", EnumReflectionTest_IsEnumType},
      {"EnumReflectionTest_GetIntegerTypeFromEnumScope",
       EnumReflectionTest_GetIntegerTypeFromEnumScope},
      {"EnumReflectionTest_GetIntegerTypeFromEnumType",
       EnumReflectionTest_GetIntegerTypeFromEnumType},
      {"EnumReflectionTest_GetEnumConstants",
       EnumReflectionTest_GetEnumConstants},
      {"EnumReflectionTest_GetEnumConstantType",
       EnumReflectionTest_GetEnumConstantType},
      {"EnumReflectionTest_GetEnumConstantValue",
       EnumReflectionTest_GetEnumConstantValue},
      {"EnumReflectionTest_GetEnums", EnumReflectionTest_GetEnums},
  };
  ThreadPoolExecutor::run(fns);
}
