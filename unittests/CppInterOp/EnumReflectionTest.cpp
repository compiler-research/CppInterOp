#include "Utils.h"

#include "clang/AST/ASTContext.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Interpreter/CppInterOp.h"
#include "clang/Sema/Sema.h"
#include "clang-c/CXCppInterOp.h"

#include "gtest/gtest.h"

using namespace TestUtils;
using namespace llvm;
using namespace clang;

TEST(ScopeReflectionTest, IsEnumScope) {
  std::vector<Decl*> Decls, SubDecls;
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

  // C API
  auto I = clang_createInterpreterFromPtr(Cpp::GetInterpreter());
  auto C_API_SHIM = [&](auto Decl) {
    return clang_scope_isEnumScope(CXScope{CXScope_Unexposed, Decl, I});
  };
  EXPECT_TRUE(C_API_SHIM(Decls[0]));
  EXPECT_FALSE(C_API_SHIM(Decls[1]));
  EXPECT_FALSE(C_API_SHIM(Decls[2]));
  EXPECT_FALSE(C_API_SHIM(SubDecls[0]));
  EXPECT_FALSE(C_API_SHIM(SubDecls[1]));
  // Clean up resources
  clang_interpreter_takeInterpreterAsPtr(I);
  clang_interpreter_dispose(I);
}

TEST(ScopeReflectionTest, IsEnumConstant) {
  std::vector<Decl*> Decls, SubDecls;
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

  // C API
  auto I = clang_createInterpreterFromPtr(Cpp::GetInterpreter());
  auto C_API_SHIM = [&](auto Decl) {
    return clang_scope_isEnumConstant(CXScope{CXScope_Unexposed, Decl, I});
  };
  EXPECT_FALSE(C_API_SHIM(Decls[0]));
  EXPECT_FALSE(C_API_SHIM(Decls[1]));
  EXPECT_FALSE(C_API_SHIM(Decls[2]));
  EXPECT_TRUE(C_API_SHIM(SubDecls[0]));
  EXPECT_TRUE(C_API_SHIM(SubDecls[1]));
  // Clean up resources
  clang_interpreter_takeInterpreterAsPtr(I);
  clang_interpreter_dispose(I);
}

TEST(EnumReflectionTest, IsEnumType) {
  std::vector<Decl*> Decls;
  std::string code = R"(
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

  // C API
  auto I = clang_createInterpreterFromPtr(Cpp::GetInterpreter());
  auto C_API_SHIM = [&](auto Decl) {
    auto Ty = clang_scope_getVariableType(CXScope{CXScope_Unexposed, Decl, I});
    return clang_qualtype_isEnumType(Ty);
  };
  EXPECT_TRUE(C_API_SHIM(Decls[2]));
  EXPECT_TRUE(C_API_SHIM(Decls[3]));
  EXPECT_TRUE(C_API_SHIM(Decls[4]));
  EXPECT_TRUE(C_API_SHIM(Decls[5]));
  // Clean up resources
  clang_interpreter_takeInterpreterAsPtr(I);
  clang_interpreter_dispose(I);
}

TEST(EnumReflectionTest, GetIntegerTypeFromEnumScope) {
  std::vector<Decl*> Decls;
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

  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetIntegerTypeFromEnumScope(Decls[0])),
            "bool");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetIntegerTypeFromEnumScope(Decls[1])),
            "char");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetIntegerTypeFromEnumScope(Decls[2])),
            "int");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetIntegerTypeFromEnumScope(Decls[3])),
            "long long");
#ifdef _WIN32
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetIntegerTypeFromEnumScope(Decls[4])),
            "int");
#else
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetIntegerTypeFromEnumScope(Decls[4])),
            "unsigned int");
#endif
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetIntegerTypeFromEnumScope(Decls[5])),
            "NULL TYPE");

  // C API
  auto I = clang_createInterpreterFromPtr(Cpp::GetInterpreter());
  auto C_API_SHIM = [&](auto Decl) {
    auto Ty = clang_scope_getIntegerTypeFromEnumScope(
        CXScope{CXScope_Unexposed, Decl, I});
    auto Str = clang_qualtype_getTypeAsString(Ty);
    auto Res = std::string(clang_getCString(Str));
    clang_disposeString(Str);
    return Res;
  };
  EXPECT_EQ(C_API_SHIM(Decls[0]), "bool");
  EXPECT_EQ(C_API_SHIM(Decls[1]), "char");
  EXPECT_EQ(C_API_SHIM(Decls[2]), "int");
  EXPECT_EQ(C_API_SHIM(Decls[3]), "long long");
#ifdef _WIN32
  EXPECT_EQ(C_API_SHIM(Decls[4]), "int");
#else
  EXPECT_EQ(C_API_SHIM(Decls[4]), "unsigned int");
#endif
  EXPECT_EQ(C_API_SHIM(Decls[5]), "NULL TYPE");
  // Clean up resources
  clang_interpreter_takeInterpreterAsPtr(I);
  clang_interpreter_dispose(I);
}

TEST(EnumReflectionTest, GetIntegerTypeFromEnumType) {
  std::vector<Decl*> Decls;
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

  auto get_int_type_from_enum_var = [](Decl* D) {
    return Cpp::GetTypeAsString(
        Cpp::GetIntegerTypeFromEnumType(Cpp::GetVariableType(D)));
  };

  EXPECT_EQ(get_int_type_from_enum_var(Decls[5]),
            "NULL TYPE"); // When a nullptr is returned by GetVariableType()
  EXPECT_EQ(get_int_type_from_enum_var(Decls[6]), "bool");
  EXPECT_EQ(get_int_type_from_enum_var(Decls[7]), "char");
  EXPECT_EQ(get_int_type_from_enum_var(Decls[8]), "int");
  EXPECT_EQ(get_int_type_from_enum_var(Decls[9]), "long long");
#ifdef _WIN32
  EXPECT_EQ(get_int_type_from_enum_var(Decls[10]), "int");
#else
  EXPECT_EQ(get_int_type_from_enum_var(Decls[10]), "unsigned int");
#endif
  EXPECT_EQ(get_int_type_from_enum_var(Decls[11]),
            "NULL TYPE"); // When a non Enum Type variable is used

  // C API
  auto I = clang_createInterpreterFromPtr(Cpp::GetInterpreter());
  auto C_API_SHIM = [&](auto Decl) {
    auto Ty = clang_scope_getVariableType(CXScope{CXScope_Unexposed, Decl, I});
    auto IntTy = clang_qualtype_getIntegerTypeFromEnumType(Ty);
    auto Str = clang_qualtype_getTypeAsString(IntTy);
    auto Res = std::string(clang_getCString(Str));
    clang_disposeString(Str);
    return Res;
  };
  EXPECT_EQ(C_API_SHIM(Decls[5]), "NULL TYPE");
  EXPECT_EQ(C_API_SHIM(Decls[6]), "bool");
  EXPECT_EQ(C_API_SHIM(Decls[7]), "char");
  EXPECT_EQ(C_API_SHIM(Decls[8]), "int");
  EXPECT_EQ(C_API_SHIM(Decls[9]), "long long");
#ifdef _WIN32
  EXPECT_EQ(C_API_SHIM(Decls[10]), "int");
#else
  EXPECT_EQ(C_API_SHIM(Decls[10]), "unsigned int");
#endif
  EXPECT_EQ(C_API_SHIM(Decls[11]), "NULL TYPE");
  // Clean up resources
  clang_interpreter_takeInterpreterAsPtr(I);
  clang_interpreter_dispose(I);
}

TEST(EnumReflectionTest, GetEnumConstants) {
  std::vector<Decl*> Decls;
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

  // C API
  auto I = clang_createInterpreterFromPtr(Cpp::GetInterpreter());
  auto C_API_SHIM = [&](auto Decl) {
    auto ECs =
        clang_scope_getEnumConstants(CXScope{CXScope_Unexposed, Decl, I});
    auto Res = ECs->Count;
    clang_disposeScopeSet(ECs);
    return Res;
  };
  EXPECT_EQ(C_API_SHIM(Decls[0]), 0);
  EXPECT_EQ(C_API_SHIM(Decls[1]), 1);
  EXPECT_EQ(C_API_SHIM(Decls[2]), 2);
  EXPECT_EQ(C_API_SHIM(Decls[3]), 3);
  EXPECT_EQ(C_API_SHIM(Decls[4]), 4);
  EXPECT_EQ(C_API_SHIM(Decls[5]), 0);
  // Clean up resources
  clang_interpreter_takeInterpreterAsPtr(I);
  clang_interpreter_dispose(I);
}

TEST(EnumReflectionTest, GetEnumConstantType) {
  std::vector<Decl*> Decls;
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

  // C API
  auto I = clang_createInterpreterFromPtr(Cpp::GetInterpreter());
  auto C_API_SHIM = [&](auto Decl) {
    auto ECs =
        clang_scope_getEnumConstants(CXScope{CXScope_Unexposed, Decl, I});
    auto Ty = clang_scope_getEnumConstantType(ECs->Scopes[0]);
    clang_disposeScopeSet(ECs);
    auto Str = clang_qualtype_getTypeAsString(Ty);
    auto Res = std::string(clang_getCString(Str));
    clang_disposeString(Str);
    return Res;
  };
  EXPECT_EQ(C_API_SHIM(Decls[0]), "Enum0");
  EXPECT_EQ(C_API_SHIM(Decls[1]), "Enum1");
  // Clean up resources
  clang_interpreter_takeInterpreterAsPtr(I);
  clang_interpreter_dispose(I);
}

TEST(EnumReflectionTest, GetEnumConstantValue) {
  std::vector<Decl*> Decls;
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
  EXPECT_EQ(Cpp::GetEnumConstantValue(Decls[1]),
            0); // Checking value of non enum constant

  // C API
  auto I = clang_createInterpreterFromPtr(Cpp::GetInterpreter());
  auto ECs =
      clang_scope_getEnumConstants(CXScope{CXScope_Unexposed, Decls[0], I});
  EXPECT_EQ(clang_scope_getEnumConstantValue(ECs->Scopes[0]), 0);
  EXPECT_EQ(clang_scope_getEnumConstantValue(ECs->Scopes[1]), 1);
  EXPECT_EQ(clang_scope_getEnumConstantValue(ECs->Scopes[2]), 52);
  EXPECT_EQ(clang_scope_getEnumConstantValue(ECs->Scopes[3]), 53);
  EXPECT_EQ(clang_scope_getEnumConstantValue(ECs->Scopes[4]), 54);
  EXPECT_EQ(clang_scope_getEnumConstantValue(ECs->Scopes[5]), -10);
  EXPECT_EQ(clang_scope_getEnumConstantValue(ECs->Scopes[6]), -9);
  EXPECT_EQ(clang_scope_getEnumConstantValue(
                CXScope{CXScope_EnumConstant, Decls[1], I}),
            0);
  clang_disposeScopeSet(ECs);
  // Clean up resources
  clang_interpreter_takeInterpreterAsPtr(I);
  clang_interpreter_dispose(I);
}

TEST(EnumReflectionTest, GetEnums) {
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

  Cpp::CreateInterpreter();
  Interp->declare(code);
  std::vector<std::string> enumNames1, enumNames2, enumNames3, enumNames4;
  Cpp::TCppScope_t globalscope = Cpp::GetScope("", 0);
  Cpp::TCppScope_t Animals_scope = Cpp::GetScope("Animals", 0);
  Cpp::TCppScope_t myClass_scope = Cpp::GetScope("myClass", 0);
  Cpp::TCppScope_t unsupported_scope = Cpp::GetScope("myVariable", 0);

  Cpp::GetEnums(globalscope, enumNames1);
  Cpp::GetEnums(Animals_scope, enumNames2);
  Cpp::GetEnums(myClass_scope, enumNames3);
  Cpp::GetEnums(unsupported_scope, enumNames4);

  // Check if the enum names are correctly retrieved
  EXPECT_TRUE(std::find(enumNames1.begin(), enumNames1.end(), "Color") !=
              enumNames1.end());
  EXPECT_TRUE(std::find(enumNames1.begin(), enumNames1.end(), "Days") !=
              enumNames1.end());
  EXPECT_TRUE(std::find(enumNames2.begin(), enumNames2.end(), "AnimalType") !=
              enumNames2.end());
  EXPECT_TRUE(std::find(enumNames2.begin(), enumNames2.end(), "Months") !=
              enumNames2.end());
  EXPECT_TRUE(std::find(enumNames3.begin(), enumNames3.end(), "Color") !=
              enumNames3.end());
  EXPECT_TRUE(enumNames4.empty());

  // C API
  auto I = clang_createInterpreterFromPtr(Cpp::GetInterpreter());
  auto GS = clang_scope_getGlobalScope(I);
  auto C_API_SHIM = [](const auto& Scope, auto EnumName) -> bool {
    auto EMStrSet = clang_scope_getEnums(Scope);
    if (!EMStrSet)
      return false;

    std::vector<std::string> Names;
    for (size_t i = 0; i < EMStrSet->Count; ++i)
      Names.push_back(clang_getCString(EMStrSet->Strings[i]));

    return std::find(Names.begin(), Names.end(), std::string(EnumName)) !=
           Names.end();
  };
  EXPECT_TRUE(C_API_SHIM(GS, "Color"));
  EXPECT_TRUE(C_API_SHIM(GS, "Days"));
  EXPECT_TRUE(C_API_SHIM(clang_scope_getScope("Animals", GS), "AnimalType"));
  EXPECT_TRUE(C_API_SHIM(clang_scope_getScope("Animals", GS), "Months"));
  EXPECT_TRUE(C_API_SHIM(clang_scope_getScope("myClass", GS), "Color"));
  EXPECT_EQ(clang_scope_getEnums(clang_scope_getScope("myVariable", GS))->Count,
            0);
  // Clean up resources
  clang_interpreter_takeInterpreterAsPtr(I);
  clang_interpreter_dispose(I);
}