#include "CppInterOp/Dispatch.h"

#include "Utils.h"

#include "gtest/gtest.h"

using namespace TestUtils;
using namespace llvm;
using namespace clang;

#define X(name, type) CppAPIType::name Cpp::name = nullptr;
CPPINTEROP_API_MAP
#undef X

class DispatchAPITest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        if (!Cpp::LoadDispatchAPI(CPPINTEROP_LIB_DIR))
            GTEST_FAIL();
    }
    static void TearDownTestSuite() {
      Cpp::UnloadDispatchAPI();
    }
};

TEST_F(DispatchAPITest, Basic_IsClassSymbolLookup) {
  std::vector<Decl*> Decls;
  GetAllTopLevelDecls("namespace N {} class C{}; int I;", Decls);
  EXPECT_FALSE(Cpp::IsClass(Decls[0]));
  EXPECT_TRUE(Cpp::IsClass(Decls[1]));
  EXPECT_FALSE(Cpp::IsClass(Decls[2]));
}

TEST_F(DispatchAPITest, Basic_Demangle) {

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

  std::string demangled_add_int = Cpp::Demangle(mangled_add_int);
  std::string demangled_add_double = Cpp::Demangle(mangled_add_double);

  EXPECT_NE(demangled_add_int.find(Cpp::GetQualifiedCompleteName(Decls[0])),
            std::string::npos);
  EXPECT_NE(demangled_add_double.find(Cpp::GetQualifiedCompleteName(Decls[1])),
            std::string::npos);
}
