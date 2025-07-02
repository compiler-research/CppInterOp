#include "Utils.h"

#include "CppInterOp/CppInterOp.h"

#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/Basic/Version.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Sema/Lookup.h"
#include "clang/Sema/Sema.h"

#include <algorithm>
#include <string>
#include <vector>

using namespace clang;
using namespace llvm;

bool TestUtils::g_use_oop_jit = false;

void TestUtils::GetAllTopLevelDecls(
    const std::string& code, std::vector<Decl*>& Decls,
    bool filter_implicitGenerated /* = false */,
    const std::vector<const char*>& interpreter_args /* = {} */) {
  TestUtils::CreateInterpreter(interpreter_args);
#ifdef CPPINTEROP_USE_CLING
  cling::Transaction* T = nullptr;
  Interp->declare(code, &T);

  for (auto DCI = T->decls_begin(), E = T->decls_end(); DCI != E; ++DCI) {
    if (DCI->m_Call != cling::Transaction::kCCIHandleTopLevelDecl)
      continue;
    for (Decl* D : DCI->m_DGR) {
      if (filter_implicitGenerated && D->isImplicit())
        continue;
      Decls.push_back(D);
    }
  }
#else
  PartialTranslationUnit* T = nullptr;
  Interp->process(code, /*Value*/ nullptr, &T);
  for (auto* D : T->TUPart->decls()) {
    if (filter_implicitGenerated && D->isImplicit())
      continue;
    Decls.push_back(D);
  }
#endif
}

void TestUtils::GetAllSubDecls(Decl* D, std::vector<Decl*>& SubDecls,
                               bool filter_implicitGenerated /* = false */) {
  if (!isa_and_nonnull<DeclContext>(D))
    return;
  DeclContext* DC = cast<DeclContext>(D);
  for (auto* Di : DC->decls()) {
    if (filter_implicitGenerated && Di->isImplicit())
      continue;
    SubDecls.push_back(Di);
  }
}

TInterp_t TestUtils::CreateInterpreter(const std::vector<const char*>& Args,
                                       const std::vector<const char*>& GpuArgs,
                                       int stdin_fd, int stdout_fd,
                                       int stderr_fd) {
  if (TestUtils::g_use_oop_jit) {
    auto mergedArgs = Args;
    mergedArgs.push_back("--use-oop-jit");
    return Cpp::CreateInterpreter(mergedArgs, GpuArgs, stdin_fd, stdout_fd,
                                  stderr_fd);
  } else {
    return Cpp::CreateInterpreter(Args, GpuArgs, stdin_fd, stdout_fd,
                                  stderr_fd);
  }
}

const char* get_c_string(CXString string) {
  return static_cast<const char*>(string.data);
}

void dispose_string(CXString string) {
  if (string.private_flags == 1 && string.data)
    free(const_cast<void*>(string.data));
}

CXScope make_scope(const clang::Decl* D, const CXInterpreter I) {
  return {CXCursor_UnexposedDecl, 0, {D, nullptr, I}};
}
