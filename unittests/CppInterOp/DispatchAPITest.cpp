#include "CppInterOp/Dispatch.h"

#include "Utils.h"

#include "gtest/gtest.h"

using namespace TestUtils;
using namespace llvm;
using namespace clang;

TEST(DispatchAPITestTest, Basic_IsClassSymbolLookup) {
  CppAPIType::IsClass IsClassFn = reinterpret_cast<CppAPIType::IsClass>(
      dlGetProcAddress("IsClass", CPPINTEROP_LIB_DIR));
  ASSERT_NE(IsClassFn, nullptr) << "failed to locate symbol: " << dlerror();
  std::vector<Decl*> Decls;
  GetAllTopLevelDecls("namespace N {} class C{}; int I;", Decls);
  EXPECT_FALSE(IsClassFn(Decls[0]));
  EXPECT_TRUE(IsClassFn(Decls[1]));
  EXPECT_FALSE(IsClassFn(Decls[2]));
}

TEST(DispatchAPITestTest,  Basic_Demangle) {

  std::string code = R"(
    int add(int x, int y) { return x + y; }
    int add(double x, double y) { return x + y; }
  )";

  std::vector<Decl*> Decls;
  GetAllTopLevelDecls(code, Decls);
  EXPECT_EQ(Decls.size(), 2);

  auto Add_int = clang::GlobalDecl(static_cast<clang::NamedDecl*>(Decls[0]));
  auto Add_double = clang::GlobalDecl(static_cast<clang::NamedDecl*>(Decls[1]));

  std::string mangled_add_int;
  std::string mangled_add_double;
  compat::maybeMangleDeclName(Add_int, mangled_add_int);
  compat::maybeMangleDeclName(Add_double, mangled_add_double);

  // CppAPIType:: gives us the specific function pointer types
  CppAPIType::Demangle DemangleFn = reinterpret_cast<CppAPIType::Demangle>(
      dlGetProcAddress("Demangle", CPPINTEROP_LIB_DIR));
  CppAPIType::GetQualifiedCompleteName GetQualifiedCompleteNameFn =
      reinterpret_cast<CppAPIType::GetQualifiedCompleteName>(
          dlGetProcAddress("GetQualifiedCompleteName"));

  std::string demangled_add_int = DemangleFn(mangled_add_int);
  std::string demangled_add_double = DemangleFn(mangled_add_double);

  EXPECT_NE(demangled_add_int.find(GetQualifiedCompleteNameFn(Decls[0])),
            std::string::npos);
  EXPECT_NE(demangled_add_double.find(GetQualifiedCompleteNameFn(Decls[1])),
            std::string::npos);
}
