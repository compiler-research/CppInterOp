#include "Utils.h"

#include "clang-c/CXCppInterOp.h"

#include "clang/AST/ASTContext.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Interpreter/CppInterOp.h"
#include "clang/Sema/Sema.h"

#include "gtest/gtest.h"

using namespace TestUtils;
using namespace llvm;
using namespace clang;

TEST(TypeReflectionTest, GetTypeAsString) {
  std::vector<Decl*> Decls;
  std::string code = R"(
    namespace N {
    class C {};
    struct S {};
    }

    N::C c;

    N::S s;

    int i;

    char ch = 'c';
    char & ch_r = ch;
    const char *ch_cp = "Hello!";
    char ch_arr[4];

  )";

  GetAllTopLevelDecls(code, Decls);
  QualType QT1 = llvm::dyn_cast<VarDecl>(Decls[1])->getType();
  QualType QT2 = llvm::dyn_cast<VarDecl>(Decls[2])->getType();
  QualType QT3 = llvm::dyn_cast<VarDecl>(Decls[3])->getType();
  QualType QT4 = llvm::dyn_cast<VarDecl>(Decls[4])->getType();
  QualType QT5 = llvm::dyn_cast<VarDecl>(Decls[5])->getType();
  QualType QT6 = llvm::dyn_cast<VarDecl>(Decls[6])->getType();
  QualType QT7 = llvm::dyn_cast<VarDecl>(Decls[7])->getType();
  EXPECT_EQ(Cpp::GetTypeAsString(QT1.getAsOpaquePtr()), "N::C");
  EXPECT_EQ(Cpp::GetTypeAsString(QT2.getAsOpaquePtr()), "N::S");
  EXPECT_EQ(Cpp::GetTypeAsString(QT3.getAsOpaquePtr()), "int");
  EXPECT_EQ(Cpp::GetTypeAsString(QT4.getAsOpaquePtr()), "char");
  EXPECT_EQ(Cpp::GetTypeAsString(QT5.getAsOpaquePtr()), "char &");
  EXPECT_EQ(Cpp::GetTypeAsString(QT6.getAsOpaquePtr()), "const char *");
  EXPECT_EQ(Cpp::GetTypeAsString(QT7.getAsOpaquePtr()), "char[4]");

  // C API
  auto I = clang_createInterpreterFromPtr(Cpp::GetInterpreter());
  auto C_API_SHIM = [&](auto Type) {
    auto Ty = CXQualType{CXType_Unexposed, Type.getAsOpaquePtr(), I};
    auto Str = clang_qualtype_getTypeAsString(Ty);
    auto Res = std::string(clang_getCString(Str));
    clang_disposeString(Str);
    return Res;
  };
  EXPECT_EQ(C_API_SHIM(QT1), "N::C");
  EXPECT_EQ(C_API_SHIM(QT2), "N::S");
  EXPECT_EQ(C_API_SHIM(QT3), "int");
  EXPECT_EQ(C_API_SHIM(QT4), "char");
  EXPECT_EQ(C_API_SHIM(QT5), "char &");
  EXPECT_EQ(C_API_SHIM(QT6), "const char *");
  EXPECT_EQ(C_API_SHIM(QT7), "char[4]");
  // Clean up resources
  clang_interpreter_takeInterpreterAsPtr(I);
  clang_interpreter_dispose(I);
}

TEST(TypeReflectionTest, GetSizeOfType) {
  std::vector<Decl*> Decls;
  std::string code = R"(
    struct S {
      int a;
      double b;
    };

    char ch;
    int n;
    double d;
    S s;
    struct FwdDecl;
    FwdDecl *f;
    )";

  GetAllTopLevelDecls(code, Decls);

  EXPECT_EQ(Cpp::GetSizeOfType(Cpp::GetVariableType(Decls[1])), 1);
  EXPECT_EQ(Cpp::GetSizeOfType(Cpp::GetVariableType(Decls[2])), 4);
  EXPECT_EQ(Cpp::GetSizeOfType(Cpp::GetVariableType(Decls[3])), 8);
  EXPECT_EQ(Cpp::GetSizeOfType(Cpp::GetVariableType(Decls[4])), 16);
  EXPECT_EQ(Cpp::GetSizeOfType(Cpp::GetTypeFromScope(Decls[5])), 0);
  EXPECT_EQ(Cpp::GetSizeOfType(Cpp::GetVariableType(Decls[6])), 8);

  // C API
  auto I = clang_createInterpreterFromPtr(Cpp::GetInterpreter());
  auto C_API_SHIM = [&](auto Decl) {
    auto Ty = clang_scope_getVariableType(CXScope{CXScope_Unexposed, Decl, I});
    return clang_qualtype_getSizeOfType(Ty);
  };
  EXPECT_EQ(C_API_SHIM(Decls[1]), 1);
  EXPECT_EQ(C_API_SHIM(Decls[2]), 4);
  EXPECT_EQ(C_API_SHIM(Decls[3]), 8);
  EXPECT_EQ(C_API_SHIM(Decls[4]), 16);
  EXPECT_EQ(C_API_SHIM(Decls[6]), 8);
  // Clean up resources
  clang_interpreter_takeInterpreterAsPtr(I);
  clang_interpreter_dispose(I);
}

TEST(TypeReflectionTest, GetCanonicalType) {
  std::vector<Decl*> Decls;
  std::string code = R"(
    typedef int I;
    typedef double D;

    I n;
    D d;
    )";

  GetAllTopLevelDecls(code, Decls);

  auto D2 = Cpp::GetVariableType(Decls[2]);
  auto D3 = Cpp::GetVariableType(Decls[3]);
  auto D4 = nullptr;

  EXPECT_EQ(Cpp::GetTypeAsString(D2), "I");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetCanonicalType(D2)), "int");
  EXPECT_EQ(Cpp::GetTypeAsString(D3), "D");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetCanonicalType(D3)), "double");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetCanonicalType(D4)), "NULL TYPE");

  // C API
  auto I = clang_createInterpreterFromPtr(Cpp::GetInterpreter());
  auto C_API_SHIM = [&](auto Type) {
    auto Ty =
        clang_qualtype_getCanonicalType(CXQualType{CXType_Unexposed, Type, I});
    auto Str = clang_qualtype_getTypeAsString(Ty);
    auto Res = std::string(clang_getCString(Str));
    clang_disposeString(Str);
    return Res;
  };
  EXPECT_EQ(C_API_SHIM(D2), "int");
  EXPECT_EQ(C_API_SHIM(D3), "double");
  EXPECT_EQ(C_API_SHIM(D4), "NULL TYPE");
  // Clean up resources
  clang_interpreter_takeInterpreterAsPtr(I);
  clang_interpreter_dispose(I);
}

TEST(TypeReflectionTest, GetType) {
  Cpp::CreateInterpreter();

  std::string code = R"(
    class A {};
    )";

  Interp->declare(code);

  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetType("int")), "int");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetType("double")), "double");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetType("A")), "A");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetType("unsigned char")),
            "unsigned char");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetType("signed char")), "signed char");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetType("unsigned short")),
            "unsigned short");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetType("unsigned int")), "unsigned int");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetType("unsigned long")),
            "unsigned long");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetType("unsigned long long")),
            "unsigned long long");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetType("signed short")), "short");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetType("signed int")), "int");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetType("signed long")), "long");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetType("signed long long")),
            "long long");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetType("struct")), "NULL TYPE");

  // C API
  auto I = clang_createInterpreterFromPtr(Cpp::GetInterpreter());
  auto C_API_SHIM = [&](const char* name) {
    auto Ty = clang_qualtype_getType(I, name);
    auto Str = clang_qualtype_getTypeAsString(Ty);
    auto Res = std::string(clang_getCString(Str));
    clang_disposeString(Str);
    return Res;
  };
  EXPECT_EQ(C_API_SHIM("int"), "int");
  EXPECT_EQ(C_API_SHIM("double"), "double");
  EXPECT_EQ(C_API_SHIM("A"), "A");
  EXPECT_EQ(C_API_SHIM("unsigned char"), "unsigned char");
  EXPECT_EQ(C_API_SHIM("signed char"), "signed char");
  EXPECT_EQ(C_API_SHIM("unsigned short"), "unsigned short");
  EXPECT_EQ(C_API_SHIM("unsigned int"), "unsigned int");
  EXPECT_EQ(C_API_SHIM("unsigned long"), "unsigned long");
  EXPECT_EQ(C_API_SHIM("unsigned long long"), "unsigned long long");
  EXPECT_EQ(C_API_SHIM("signed short"), "short");
  EXPECT_EQ(C_API_SHIM("signed int"), "int");
  EXPECT_EQ(C_API_SHIM("signed long"), "long");
  EXPECT_EQ(C_API_SHIM("signed long long"), "long long");
  EXPECT_EQ(C_API_SHIM("struct"), "NULL TYPE");
  // Clean up resources
  clang_interpreter_takeInterpreterAsPtr(I);
  clang_interpreter_dispose(I);
}

TEST(TypeReflectionTest, IsRecordType) {
  std::vector<Decl*> Decls;

  std::string code = R"(
    const int var0 = 0;
    const int &var1 = var0;
    const int *var2 = &var1;
    const int *&var3 = var2;
    const int var4[] = {};
    const int *var5[] = {var2};
    int var6 = 0;
    int &var7 = var6;
    int *var8 = &var7;
    int *&var9 = var8;
    int var10[] = {};
    int *var11[] = {var8};

    class C {
      public:
        int i;
    };
    const C cvar0{0};
    const C &cvar1 = cvar0;
    const C *cvar2 = &cvar1;
    const C *&cvar3 = cvar2;
    const C cvar4[] = {};
    const C *cvar5[] = {cvar2};
    C cvar6;
    C &cvar7 = cvar6;
    C *cvar8 = &cvar7;
    C *&cvar9 = cvar8;
    C cvar10[] = {};
    C *cvar11[] = {cvar8};
    )";
  GetAllTopLevelDecls(code, Decls);

  auto is_var_of_record_ty = [](Decl* D) {
    return Cpp::IsRecordType(Cpp::GetVariableType(D));
  };

  EXPECT_FALSE(is_var_of_record_ty(Decls[0]));
  EXPECT_FALSE(is_var_of_record_ty(Decls[1]));
  EXPECT_FALSE(is_var_of_record_ty(Decls[2]));
  EXPECT_FALSE(is_var_of_record_ty(Decls[3]));
  EXPECT_FALSE(is_var_of_record_ty(Decls[4]));
  EXPECT_FALSE(is_var_of_record_ty(Decls[5]));
  EXPECT_FALSE(is_var_of_record_ty(Decls[6]));
  EXPECT_FALSE(is_var_of_record_ty(Decls[7]));
  EXPECT_FALSE(is_var_of_record_ty(Decls[8]));
  EXPECT_FALSE(is_var_of_record_ty(Decls[9]));
  EXPECT_FALSE(is_var_of_record_ty(Decls[10]));
  EXPECT_FALSE(is_var_of_record_ty(Decls[11]));

  EXPECT_TRUE(is_var_of_record_ty(Decls[13]));
  EXPECT_FALSE(is_var_of_record_ty(Decls[14]));
  EXPECT_FALSE(is_var_of_record_ty(Decls[15]));
  EXPECT_FALSE(is_var_of_record_ty(Decls[16]));
  EXPECT_FALSE(is_var_of_record_ty(Decls[17]));
  EXPECT_FALSE(is_var_of_record_ty(Decls[18]));
  EXPECT_TRUE(is_var_of_record_ty(Decls[19]));
  EXPECT_FALSE(is_var_of_record_ty(Decls[20]));
  EXPECT_FALSE(is_var_of_record_ty(Decls[21]));
  EXPECT_FALSE(is_var_of_record_ty(Decls[22]));
  EXPECT_FALSE(is_var_of_record_ty(Decls[23]));
  EXPECT_FALSE(is_var_of_record_ty(Decls[24]));

  // C API
  auto I = clang_createInterpreterFromPtr(Cpp::GetInterpreter());
  auto C_API_SHIM = [&](auto Decl) {
    auto Ty = clang_scope_getVariableType(CXScope{CXScope_Unexposed, Decl, I});
    return clang_scope_isRecordType(Ty);
  };
  EXPECT_FALSE(C_API_SHIM(Decls[0]));
  EXPECT_FALSE(C_API_SHIM(Decls[1]));
  EXPECT_FALSE(C_API_SHIM(Decls[2]));
  EXPECT_FALSE(C_API_SHIM(Decls[3]));
  EXPECT_FALSE(C_API_SHIM(Decls[4]));
  EXPECT_FALSE(C_API_SHIM(Decls[5]));
  EXPECT_FALSE(C_API_SHIM(Decls[6]));
  EXPECT_FALSE(C_API_SHIM(Decls[7]));
  EXPECT_FALSE(C_API_SHIM(Decls[8]));
  EXPECT_FALSE(C_API_SHIM(Decls[9]));
  EXPECT_FALSE(C_API_SHIM(Decls[10]));
  EXPECT_FALSE(C_API_SHIM(Decls[11]));
  EXPECT_TRUE(C_API_SHIM(Decls[13]));
  EXPECT_FALSE(C_API_SHIM(Decls[14]));
  EXPECT_FALSE(C_API_SHIM(Decls[15]));
  EXPECT_FALSE(C_API_SHIM(Decls[16]));
  EXPECT_FALSE(C_API_SHIM(Decls[17]));
  EXPECT_FALSE(C_API_SHIM(Decls[18]));
  EXPECT_TRUE(C_API_SHIM(Decls[19]));
  EXPECT_FALSE(C_API_SHIM(Decls[20]));
  EXPECT_FALSE(C_API_SHIM(Decls[21]));
  EXPECT_FALSE(C_API_SHIM(Decls[22]));
  EXPECT_FALSE(C_API_SHIM(Decls[23]));
  EXPECT_FALSE(C_API_SHIM(Decls[24]));
  // Clean up resources
  clang_interpreter_takeInterpreterAsPtr(I);
  clang_interpreter_dispose(I);
}

TEST(TypeReflectionTest, GetUnderlyingType) {
  std::vector<Decl*> Decls;

  std::string code = R"(
    const int var0 = 0;
    const int &var1 = var0;
    const int *var2 = &var1;
    const int *&var3 = var2;
    const int var4[] = {};
    const int *var5[] = {var2};
    int var6 = 0;
    int &var7 = var6;
    int *var8 = &var7;
    int *&var9 = var8;
    int var10[] = {};
    int *var11[] = {var8};
    int var12[2][5];
    int ***var13;

    class C {
      public:
        int i;
    };
    const C cvar0{0};
    const C &cvar1 = cvar0;
    const C *cvar2 = &cvar1;
    const C *&cvar3 = cvar2;
    const C cvar4[] = {};
    const C *cvar5[] = {cvar2};
    C cvar6;
    C &cvar7 = cvar6;
    C *cvar8 = &cvar7;
    C *&cvar9 = cvar8;
    C cvar10[] = {};
    C *cvar11[] = {cvar8};
    C cvar12[2][5];
    C ***cvar13;

    enum E { e1, e2 };
    E evar0 = e1;
    )";
  GetAllTopLevelDecls(code, Decls);
  auto get_underly_var_type_as_str = [](Decl* D) {
    return Cpp::GetTypeAsString(
        Cpp::GetUnderlyingType(Cpp::GetVariableType(D)));
  };
  EXPECT_EQ(get_underly_var_type_as_str(Decls[0]), "int");
  EXPECT_EQ(get_underly_var_type_as_str(Decls[1]), "int");
  EXPECT_EQ(get_underly_var_type_as_str(Decls[2]), "int");
  EXPECT_EQ(get_underly_var_type_as_str(Decls[3]), "int");
  EXPECT_EQ(get_underly_var_type_as_str(Decls[4]), "int");
  EXPECT_EQ(get_underly_var_type_as_str(Decls[5]), "int");
  EXPECT_EQ(get_underly_var_type_as_str(Decls[6]), "int");
  EXPECT_EQ(get_underly_var_type_as_str(Decls[7]), "int");
  EXPECT_EQ(get_underly_var_type_as_str(Decls[8]), "int");
  EXPECT_EQ(get_underly_var_type_as_str(Decls[9]), "int");
  EXPECT_EQ(get_underly_var_type_as_str(Decls[10]), "int");
  EXPECT_EQ(get_underly_var_type_as_str(Decls[11]), "int");
  EXPECT_EQ(get_underly_var_type_as_str(Decls[12]), "int");
  EXPECT_EQ(get_underly_var_type_as_str(Decls[13]), "int");

  EXPECT_EQ(get_underly_var_type_as_str(Decls[15]), "C");
  EXPECT_EQ(get_underly_var_type_as_str(Decls[16]), "C");
  EXPECT_EQ(get_underly_var_type_as_str(Decls[17]), "C");
  EXPECT_EQ(get_underly_var_type_as_str(Decls[18]), "C");
  EXPECT_EQ(get_underly_var_type_as_str(Decls[19]), "C");
  EXPECT_EQ(get_underly_var_type_as_str(Decls[20]), "C");
  EXPECT_EQ(get_underly_var_type_as_str(Decls[21]), "C");
  EXPECT_EQ(get_underly_var_type_as_str(Decls[22]), "C");
  EXPECT_EQ(get_underly_var_type_as_str(Decls[23]), "C");
  EXPECT_EQ(get_underly_var_type_as_str(Decls[24]), "C");
  EXPECT_EQ(get_underly_var_type_as_str(Decls[25]), "C");
  EXPECT_EQ(get_underly_var_type_as_str(Decls[26]), "C");
  EXPECT_EQ(get_underly_var_type_as_str(Decls[27]), "C");
  EXPECT_EQ(get_underly_var_type_as_str(Decls[28]), "C");

  EXPECT_EQ(get_underly_var_type_as_str(Decls[30]), "E");

  // C API
  auto I = clang_createInterpreterFromPtr(Cpp::GetInterpreter());
  auto C_API_SHIM = [&](auto Decl) {
    auto Ty = clang_scope_getVariableType(CXScope{CXScope_Unexposed, Decl, I});
    auto Str =
        clang_qualtype_getTypeAsString(clang_qualtype_getUnderlyingType(Ty));
    auto Res = std::string(clang_getCString(Str));
    clang_disposeString(Str);
    return Res;
  };
  EXPECT_EQ(C_API_SHIM(Decls[0]), "int");
  EXPECT_EQ(C_API_SHIM(Decls[1]), "int");
  EXPECT_EQ(C_API_SHIM(Decls[2]), "int");
  EXPECT_EQ(C_API_SHIM(Decls[3]), "int");
  EXPECT_EQ(C_API_SHIM(Decls[4]), "int");
  EXPECT_EQ(C_API_SHIM(Decls[5]), "int");
  EXPECT_EQ(C_API_SHIM(Decls[6]), "int");
  EXPECT_EQ(C_API_SHIM(Decls[7]), "int");
  EXPECT_EQ(C_API_SHIM(Decls[8]), "int");
  EXPECT_EQ(C_API_SHIM(Decls[9]), "int");
  EXPECT_EQ(C_API_SHIM(Decls[10]), "int");
  EXPECT_EQ(C_API_SHIM(Decls[11]), "int");
  EXPECT_EQ(C_API_SHIM(Decls[12]), "int");
  EXPECT_EQ(C_API_SHIM(Decls[13]), "int");

  EXPECT_EQ(C_API_SHIM(Decls[15]), "C");
  EXPECT_EQ(C_API_SHIM(Decls[16]), "C");
  EXPECT_EQ(C_API_SHIM(Decls[17]), "C");
  EXPECT_EQ(C_API_SHIM(Decls[18]), "C");
  EXPECT_EQ(C_API_SHIM(Decls[19]), "C");
  EXPECT_EQ(C_API_SHIM(Decls[20]), "C");
  EXPECT_EQ(C_API_SHIM(Decls[21]), "C");
  EXPECT_EQ(C_API_SHIM(Decls[22]), "C");
  EXPECT_EQ(C_API_SHIM(Decls[23]), "C");
  EXPECT_EQ(C_API_SHIM(Decls[24]), "C");
  EXPECT_EQ(C_API_SHIM(Decls[25]), "C");
  EXPECT_EQ(C_API_SHIM(Decls[26]), "C");
  EXPECT_EQ(C_API_SHIM(Decls[27]), "C");
  EXPECT_EQ(C_API_SHIM(Decls[28]), "C");

  EXPECT_EQ(C_API_SHIM(Decls[30]), "E");
  // Clean up resources
  clang_interpreter_takeInterpreterAsPtr(I);
  clang_interpreter_dispose(I);
}

TEST(TypeReflectionTest, IsUnderlyingTypeRecordType) {
  std::vector<Decl*> Decls;

  std::string code = R"(
    const int var0 = 0;
    const int &var1 = var0;
    const int *var2 = &var1;
    const int *&var3 = var2;
    const int var4[] = {};
    const int *var5[] = {var2};
    int var6 = 0;
    int &var7 = var6;
    int *var8 = &var7;
    int *&var9 = var8;
    int var10[] = {};
    int *var11[] = {var8};

    class C {
      public:
        int i;
    };
    const C cvar0{0};
    const C &cvar1 = cvar0;
    const C *cvar2 = &cvar1;
    const C *&cvar3 = cvar2;
    const C cvar4[] = {};
    const C *cvar5[] = {cvar2};
    C cvar6;
    C &cvar7 = cvar6;
    C *cvar8 = &cvar7;
    C *&cvar9 = cvar8;
    C cvar10[] = {};
    C *cvar11[] = {cvar8};
    )";
  GetAllTopLevelDecls(code, Decls);

  auto is_var_of_underly_record_ty = [](Decl* D) {
    return Cpp::IsRecordType(Cpp::GetUnderlyingType(Cpp::GetVariableType(D)));
  };

  EXPECT_FALSE(is_var_of_underly_record_ty(Decls[0]));
  EXPECT_FALSE(is_var_of_underly_record_ty(Decls[1]));
  EXPECT_FALSE(is_var_of_underly_record_ty(Decls[2]));
  EXPECT_FALSE(is_var_of_underly_record_ty(Decls[3]));
  EXPECT_FALSE(is_var_of_underly_record_ty(Decls[4]));
  EXPECT_FALSE(is_var_of_underly_record_ty(Decls[5]));
  EXPECT_FALSE(is_var_of_underly_record_ty(Decls[6]));
  EXPECT_FALSE(is_var_of_underly_record_ty(Decls[7]));
  EXPECT_FALSE(is_var_of_underly_record_ty(Decls[8]));
  EXPECT_FALSE(is_var_of_underly_record_ty(Decls[9]));
  EXPECT_FALSE(is_var_of_underly_record_ty(Decls[10]));
  EXPECT_FALSE(is_var_of_underly_record_ty(Decls[11]));

  EXPECT_TRUE(is_var_of_underly_record_ty(Decls[13]));
  EXPECT_TRUE(is_var_of_underly_record_ty(Decls[14]));
  EXPECT_TRUE(is_var_of_underly_record_ty(Decls[15]));
  EXPECT_TRUE(is_var_of_underly_record_ty(Decls[16]));
  EXPECT_TRUE(is_var_of_underly_record_ty(Decls[17]));
  EXPECT_TRUE(is_var_of_underly_record_ty(Decls[18]));
  EXPECT_TRUE(is_var_of_underly_record_ty(Decls[19]));
  EXPECT_TRUE(is_var_of_underly_record_ty(Decls[20]));
  EXPECT_TRUE(is_var_of_underly_record_ty(Decls[21]));
  EXPECT_TRUE(is_var_of_underly_record_ty(Decls[22]));
  EXPECT_TRUE(is_var_of_underly_record_ty(Decls[23]));
  EXPECT_TRUE(is_var_of_underly_record_ty(Decls[24]));
}

TEST(TypeReflectionTest, GetComplexType) {
  Cpp::CreateInterpreter();

  auto get_complex_type_as_string = [&](const std::string& element_type) {
    auto ElementQT = Cpp::GetType(element_type);
    auto ComplexQT = Cpp::GetComplexType(ElementQT);
    return Cpp::GetTypeAsString(Cpp::GetCanonicalType(ComplexQT));
  };

  EXPECT_EQ(get_complex_type_as_string("int"), "_Complex int");
  EXPECT_EQ(get_complex_type_as_string("float"), "_Complex float");
  EXPECT_EQ(get_complex_type_as_string("double"), "_Complex double");
}

TEST(TypeReflectionTest, GetTypeFromScope) {
  std::vector<Decl*> Decls;

  std::string code = R"(
    class C {};
    struct S {};
    int a = 10;
    )";

  GetAllTopLevelDecls(code, Decls);

  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetTypeFromScope(Decls[0])), "C");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetTypeFromScope(Decls[1])), "S");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetTypeFromScope(Decls[2])), "int");
  EXPECT_EQ(Cpp::GetTypeAsString(Cpp::GetTypeFromScope(nullptr)), "NULL TYPE");
}

TEST(TypeReflectionTest, IsTypeDerivedFrom) {
  std::vector<Decl*> Decls;

  std::string code = R"(
      class A {};
      class B : A {};
      class C {};
      class D : B {};
      class E : A {};

      A a;
      B b;
      C c;
      D d;
      E e;
    )";

  GetAllTopLevelDecls(code, Decls);

  Cpp::TCppType_t type_A = Cpp::GetVariableType(Decls[5]);
  Cpp::TCppType_t type_B = Cpp::GetVariableType(Decls[6]);
  Cpp::TCppType_t type_C = Cpp::GetVariableType(Decls[7]);
  Cpp::TCppType_t type_D = Cpp::GetVariableType(Decls[8]);
  Cpp::TCppType_t type_E = Cpp::GetVariableType(Decls[9]);

  EXPECT_TRUE(Cpp::IsTypeDerivedFrom(type_B, type_A));
  EXPECT_TRUE(Cpp::IsTypeDerivedFrom(type_D, type_B));
  EXPECT_TRUE(Cpp::IsTypeDerivedFrom(type_D, type_A));
  EXPECT_TRUE(Cpp::IsTypeDerivedFrom(type_E, type_A));

  EXPECT_FALSE(Cpp::IsTypeDerivedFrom(type_A, type_B));
  EXPECT_FALSE(Cpp::IsTypeDerivedFrom(type_C, type_A));
  EXPECT_FALSE(Cpp::IsTypeDerivedFrom(type_D, type_C));
  EXPECT_FALSE(Cpp::IsTypeDerivedFrom(type_B, type_D));
  EXPECT_FALSE(Cpp::IsTypeDerivedFrom(type_A, type_E));
}

TEST(TypeReflectionTest, GetDimensions) {
  std::vector<Decl*> Decls, SubDecls;

  std::string code = R"(
      int a;
      int b[1];
      int c[1][2];
      int d[1][2][3];

      struct S1 {
        char ch[];
      };


      struct S2 {
        char ch[][3][4];
      };

      template <typename T>
      struct S3 {
        typedef T type;
      };
      S3<int[6]>::type arr;
    )";

  GetAllTopLevelDecls(code, Decls);

  std::vector<long int> dims, truth_dims;

  // Variable a
  dims = Cpp::GetDimensions(Cpp::GetVariableType(Decls[0]));
  truth_dims = std::vector<long int>({});
  EXPECT_EQ(dims.size(), truth_dims.size());
  for (unsigned i = 0; i < truth_dims.size() && i < dims.size(); i++) {
    EXPECT_EQ(dims[i], truth_dims[i]);
  }

  // Variable b
  dims = Cpp::GetDimensions(Cpp::GetVariableType(Decls[1]));
  truth_dims = std::vector<long int>({1});
  EXPECT_EQ(dims.size(), truth_dims.size());
  for (unsigned i = 0; i < truth_dims.size() && i < dims.size(); i++) {
    EXPECT_EQ(dims[i], truth_dims[i]);
  }

  // Variable c
  dims = Cpp::GetDimensions(Cpp::GetVariableType(Decls[2]));
  truth_dims = std::vector<long int>({1, 2});
  EXPECT_EQ(dims.size(), truth_dims.size());
  for (unsigned i = 0; i < truth_dims.size() && i < dims.size(); i++) {
    EXPECT_EQ(dims[i], truth_dims[i]);
  }

  // Variable d
  dims = Cpp::GetDimensions(Cpp::GetVariableType(Decls[3]));
  truth_dims = std::vector<long int>({1, 2, 3});
  EXPECT_EQ(dims.size(), truth_dims.size());
  for (unsigned i = 0; i < truth_dims.size() && i < dims.size(); i++) {
    EXPECT_EQ(dims[i], truth_dims[i]);
  }

  // Field S1::ch
  GetAllSubDecls(Decls[4], SubDecls);
  dims = Cpp::GetDimensions(Cpp::GetVariableType(SubDecls[1]));
  truth_dims = std::vector<long int>({Cpp::DimensionValue::UNKNOWN_SIZE});
  EXPECT_EQ(dims.size(), truth_dims.size());
  for (unsigned i = 0; i < truth_dims.size() && i < dims.size(); i++) {
    EXPECT_EQ(dims[i], truth_dims[i]);
  }

  // Field S2::ch
  GetAllSubDecls(Decls[5], SubDecls);
  dims = Cpp::GetDimensions(Cpp::GetVariableType(SubDecls[3]));
  truth_dims = std::vector<long int>({Cpp::DimensionValue::UNKNOWN_SIZE, 3, 4});
  EXPECT_EQ(dims.size(), truth_dims.size());
  for (unsigned i = 0; i < truth_dims.size() && i < dims.size(); i++) {
    EXPECT_EQ(dims[i], truth_dims[i]);
  }

  // Variable arr
  dims = Cpp::GetDimensions(Cpp::GetVariableType(Decls[7]));
  truth_dims = std::vector<long int>({6});
  EXPECT_EQ(dims.size(), truth_dims.size());
  for (unsigned i = 0; i < truth_dims.size() && i < dims.size(); i++) {
    EXPECT_EQ(dims[i], truth_dims[i]);
  }
}

TEST(TypeReflectionTest, IsPODType) {
  std::vector<Decl*> Decls;

  std::string code = R"(
    struct A {};
    struct B {
      int x;

     private:
      int y;
    };

    A a;
    B b;
    )";

  GetAllTopLevelDecls(code, Decls);
  EXPECT_TRUE(Cpp::IsPODType(Cpp::GetVariableType(Decls[2])));
  EXPECT_FALSE(Cpp::IsPODType(Cpp::GetVariableType(Decls[3])));
  EXPECT_FALSE(Cpp::IsPODType(0));
}

TEST(TypeReflectionTest, IsSmartPtrType) {
  if (llvm::sys::RunningOnValgrind())
    GTEST_SKIP() << "XFAIL due to Valgrind report";
  Cpp::CreateInterpreter();

  Interp->declare(R"(
    #include <memory>

    template<typename T>
    class derived_shared_ptr : public std::shared_ptr<T> {};
    template<typename T>
    class derived_unique_ptr : public std::unique_ptr<T> {};

    class C {};

    // std::auto_ptr<C> smart_ptr1; // Deprecated but passes the checks.
    std::shared_ptr<C> smart_ptr2;
    std::unique_ptr<C> smart_ptr3;
    std::weak_ptr<C> smart_ptr4;
    derived_shared_ptr<C> smart_ptr5;
    derived_unique_ptr<C> smart_ptr6;

    C *raw_ptr;
    C object();
  )");

  auto get_type_from_varname = [&](const std::string& varname) {
    return Cpp::GetVariableType(Cpp::GetNamed(varname));
  };

  // EXPECT_TRUE(Cpp::IsSmartPtrType(get_type_from_varname("smart_ptr1")));
  EXPECT_TRUE(Cpp::IsSmartPtrType(get_type_from_varname("smart_ptr2")));
  EXPECT_TRUE(Cpp::IsSmartPtrType(get_type_from_varname("smart_ptr3")));
  EXPECT_TRUE(Cpp::IsSmartPtrType(get_type_from_varname("smart_ptr4")));
  EXPECT_TRUE(Cpp::IsSmartPtrType(get_type_from_varname("smart_ptr5")));
  EXPECT_TRUE(Cpp::IsSmartPtrType(get_type_from_varname("smart_ptr6")));
  EXPECT_FALSE(Cpp::IsSmartPtrType(get_type_from_varname("raw_ptr")));
  EXPECT_FALSE(Cpp::IsSmartPtrType(get_type_from_varname("object")));
}