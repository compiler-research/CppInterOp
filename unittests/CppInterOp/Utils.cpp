#include "Utils.h"

#include "cling/Interpreter/Interpreter.h"
#include "cling/Interpreter/Transaction.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"

#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/Basic/Version.h"
#include "clang/Config/config.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Interpreter/CppInterOp.h"
#include "clang/Sema/Sema.h"

using namespace cling;
using namespace clang;
using namespace llvm;

std::unique_ptr<Interpreter> TestUtils::Interp;

void TestUtils::GetAllTopLevelDecls(const std::string &code,
                                    std::vector<Decl *> &Decls) {
  Interp.reset(static_cast<Interpreter *>(Cpp::CreateInterpreter()));
  Transaction *T = nullptr;
  Interp->declare(code, &T);
  for (auto DCI = T->decls_begin(), E = T->decls_end(); DCI != E; ++DCI) {
    if (DCI->m_Call != Transaction::kCCIHandleTopLevelDecl)
      continue;
    assert(DCI->m_DGR.isSingleDecl());
    Decls.push_back(DCI->m_DGR.getSingleDecl());
  }
}

void TestUtils::GetAllSubDecls(Decl *D, std::vector<Decl *> &SubDecls) {
  if (!isa_and_nonnull<DeclContext>(D))
    return;
  DeclContext *DC = Decl::castToDeclContext(D);
  for (auto DCI = DC->decls_begin(), E = DC->decls_end(); DCI != E; ++DCI) {
    SubDecls.push_back(*DCI);
  }
}
