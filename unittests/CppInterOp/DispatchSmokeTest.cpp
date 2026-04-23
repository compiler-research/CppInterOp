#include "CppInterOp/Dispatch.h"

#include "gtest/gtest.h"

#include <string>
#include <vector>

// Smoke tests that verify the dispatch wrappers forward correctly for
// representative API functions across all categories. The full test
// suite runs against the direct API; these confirm the dispatch layer
// doesn't introduce regressions.

// --- Interpreter ---

TEST(DispatchSmokeTest, CreateAndDeleteInterpreter) {
  auto* I = Cpp::CreateInterpreter({});
  EXPECT_NE(I, nullptr);
  EXPECT_TRUE(Cpp::DeleteInterpreter());
}

TEST(DispatchSmokeTest, DeclareAndProcess) {
  Cpp::CreateInterpreter({});
  EXPECT_EQ(0, Cpp::Declare("int dispatch_x = 42;"));
  EXPECT_EQ(0, Cpp::Process("dispatch_x++;"));
}

// --- Scope queries ---

TEST(DispatchSmokeTest, ScopeLookup) {
  Cpp::CreateInterpreter({});
  Cpp::Declare("namespace DispNS { class Foo {}; }");

  auto* global = Cpp::GetGlobalScope();
  EXPECT_NE(global, nullptr);

  auto* ns = Cpp::GetNamed("DispNS");
  EXPECT_NE(ns, nullptr);
  EXPECT_TRUE(Cpp::IsNamespace(ns));

  auto* foo = Cpp::GetScope("Foo", ns);
  EXPECT_NE(foo, nullptr);
  EXPECT_TRUE(Cpp::IsClass(foo));
  EXPECT_FALSE(Cpp::IsNamespace(foo));
}

// --- Type queries ---

TEST(DispatchSmokeTest, TypeReflection) {
  Cpp::CreateInterpreter({});
  Cpp::Declare("class DispType { int x; };");

  auto* scope = Cpp::GetNamed("DispType");
  auto* type = Cpp::GetTypeFromScope(scope);
  EXPECT_NE(type, nullptr);

  auto* scope_back = Cpp::GetScopeFromType(type);
  EXPECT_EQ(scope, scope_back);

  EXPECT_GT(Cpp::SizeOf(scope), 0U);
  EXPECT_FALSE(Cpp::IsPointerType(type));
  EXPECT_FALSE(Cpp::IsEnumType(type));
}

// --- Function reflection ---

TEST(DispatchSmokeTest, FunctionReflection) {
  Cpp::CreateInterpreter({});
  Cpp::Declare("namespace DispFn { int add(int a, int b) { return a+b; } }");

  auto* ns = Cpp::GetNamed("DispFn");
  auto fns = Cpp::GetFunctionsUsingName(ns, "add");
  EXPECT_EQ(fns.size(), 1U);

  EXPECT_EQ(Cpp::GetFunctionNumArgs(fns[0]), 2U);
  EXPECT_EQ(Cpp::GetName(fns[0]), "add");
}

// --- Variable reflection ---

TEST(DispatchSmokeTest, VariableReflection) {
  Cpp::CreateInterpreter({});
  Cpp::Declare("namespace DispVar { int gval = 99; }");

  auto* ns = Cpp::GetNamed("DispVar");
  // GetNamed finds variables inside a namespace scope.
  auto* var = Cpp::GetNamed("gval", ns);
  EXPECT_NE(var, nullptr);
  EXPECT_TRUE(Cpp::IsVariable(var));
}

// --- Templates ---

TEST(DispatchSmokeTest, TemplateInstantiation) {
  Cpp::CreateInterpreter({});
  Cpp::Declare("template<typename T> struct DispTmpl { T val; };");

  auto* tmpl = Cpp::GetNamed("DispTmpl");
  EXPECT_TRUE(Cpp::IsTemplate(tmpl));
}

// --- Construct / Destruct ---

TEST(DispatchSmokeTest, ConstructDestruct) {
  Cpp::CreateInterpreter({"-include", "new"});
  Cpp::Declare("struct DispObj { int x = 7; };");

  auto* scope = Cpp::GetNamed("DispObj");
  EXPECT_TRUE(Cpp::IsComplete(scope));

  auto* obj = Cpp::Construct(scope);
  EXPECT_NE(obj, nullptr);
  Cpp::Destruct(obj, scope, /*withFree=*/true);
}

// --- Enum ---

TEST(DispatchSmokeTest, EnumReflection) {
  Cpp::CreateInterpreter({});
  Cpp::Declare("enum DispEnum { A, B, C };");

  auto* e = Cpp::GetNamed("DispEnum");
  EXPECT_TRUE(Cpp::IsEnumScope(e));

  auto constants = Cpp::GetEnumConstants(e);
  EXPECT_EQ(constants.size(), 3U);
}

// --- Demangle ---

TEST(DispatchSmokeTest, Demangle) {
  Cpp::CreateInterpreter({});
  // _Z3foov is the mangled name for "foo()"
  auto result = Cpp::Demangle("_Z3foov");
  EXPECT_EQ(result, "foo()");
}

// --- Version ---

TEST(DispatchSmokeTest, GetVersion) {
  auto ver = Cpp::GetVersion();
  EXPECT_FALSE(ver.empty());
}
