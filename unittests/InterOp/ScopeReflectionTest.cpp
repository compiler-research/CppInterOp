
#include "cling/Interpreter/Interpreter.h"
#include "cling/Interpreter/Transaction.h"

#include "clang/AST/Decl.h"
#include "clang/Basic/Version.h"
#include "clang/Config/config.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Casting.h"

#include "gtest/gtest.h"

namespace InterOp {
using TCppScope_t = void*;
bool IsNamespace(TCppScope_t scope) {
  clang::Decl *D = static_cast<clang::Decl*>(scope);
  return llvm::isa<clang::NamespaceDecl>(D);
}
}

static clang::Decl* GetDeclN(cling::Transaction &T, unsigned N) {
  cling::Transaction::DelayCallInfo DCI = *(T.decls_begin() + N);
  assert(DCI.m_DGR.isSingleDecl());
  return DCI.m_DGR.getSingleDecl();

}

// This function isn't referenced outside its translation unit, but it
// can't use the "static" keyword because its address is used for
// GetMainExecutable (since some platforms don't support taking the
// address of main, and some platforms can't implement GetMainExecutable
// without being given the address of a function in the main executable).
std::string GetExecutablePath(const char *Argv0, void *MainAddr) {
  return llvm::sys::fs::getMainExecutable(Argv0, MainAddr);
}

static std::string MakeResourcesPath() {
  // Dir is bin/ or lib/, depending on where BinaryPath is.
  void *MainAddr = (void *)(intptr_t)GetExecutablePath;
  std::string BinaryPath = GetExecutablePath(/*Argv0=*/nullptr, MainAddr);

  // build/tools/clang/unittests/Interpreter/Executable -> build/
  llvm::StringRef Dir = llvm::sys::path::parent_path(BinaryPath);

  Dir = llvm::sys::path::parent_path(Dir);
  Dir = llvm::sys::path::parent_path(Dir);
  Dir = llvm::sys::path::parent_path(Dir);
  Dir = llvm::sys::path::parent_path(Dir);
  Dir = llvm::sys::path::parent_path(Dir);
  llvm::SmallString<128> P(Dir);
  llvm::sys::path::append(P, llvm::Twine("lib") + CLANG_LIBDIR_SUFFIX, "clang",
                          CLANG_VERSION_STRING);

  return std::string(P.str());
}


static std::unique_ptr<cling::Interpreter> createInterpreter() {
  std::string MainExecutableName =
    llvm::sys::fs::getMainExecutable(nullptr, nullptr);
  std::string ResourceDir = MakeResourcesPath();
  std::vector<const char *> ClingArgv = {"-resource-dir", ResourceDir.c_str()};
  ClingArgv.insert(ClingArgv.begin(), MainExecutableName.c_str());
  return llvm::make_unique<cling::Interpreter>(ClingArgv.size(), &ClingArgv[0]);
}


// Check that the CharInfo table has been constructed reasonably.
TEST(ScopeReflectionTest, IsNamespace) {
  auto Interp = createInterpreter();
  cling::Transaction *T = nullptr;
  Interp->declare("namespace N {} class C{}; int I;", &T);
  EXPECT_TRUE(InterOp::IsNamespace(GetDeclN(*T, 0)));
  EXPECT_FALSE(InterOp::IsNamespace(GetDeclN(*T, 1)));
  EXPECT_FALSE(InterOp::IsNamespace(GetDeclN(*T, 2)));
}
