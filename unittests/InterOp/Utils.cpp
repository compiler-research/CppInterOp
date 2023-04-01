#include "Utils.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"

#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/Basic/Version.h"
#include "clang/Config/config.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Interpreter/InterOp.h"
#include "clang/Sema/Lookup.h"
#include "clang/Sema/Sema.h"

#include <algorithm>
#include <string>

using namespace clang;
using namespace llvm;

// Valgrind complains about __cxa_pure_virtual called when deleting
// llvm::SectionMemoryManager::~SectionMemoryManager as part of the dtor chain
// of the Interpreter.
// This might fix the issue https://reviews.llvm.org/D107087
// FIXME: For now we just leak the Interpreter.
std::unique_ptr<compat::Interpreter> TestUtils::Interp;
struct InterpDeleter {
  ~InterpDeleter() { TestUtils::Interp.release(); }
} Deleter;

void TestUtils::GetAllTopLevelDecls(const std::string& code, std::vector<Decl*>& Decls,
                                    bool filter_implicitGenerated /* = false */) {
  Interp.reset(
      static_cast<compat::Interpreter *>(InterOp::CreateInterpreter()));
#ifdef USE_CLING
  cling::Transaction *T = nullptr;
  Interp->declare(code, &T);

  for (auto DCI = T->decls_begin(), E = T->decls_end(); DCI != E; ++DCI) {
    if (DCI->m_Call != cling::Transaction::kCCIHandleTopLevelDecl)
      continue;
    assert(DCI->m_DGR.isSingleDecl());
    if (filter_implicitGenerated && (DCI->m_DGR.getSingleDecl())->isImplicit())
      continue;
    Decls.push_back(DCI->m_DGR.getSingleDecl());
  }
#else
  PartialTranslationUnit *T = nullptr;
  Interp->process(code, /*Value*/nullptr, &T);
  for (auto DCI = T->TUPart->decls_begin(), E = T->TUPart->decls_end();
       DCI != E; ++DCI) {
    if (filter_implicitGenerated && (*DCI)->isImplicit())
      continue;
    Decls.push_back(*DCI);
  }
#endif
}

void TestUtils::GetAllSubDecls(Decl *D, std::vector<Decl*>& SubDecls,
                               bool filter_implicitGenerated /* = false */) {
  if (!isa_and_nonnull<DeclContext>(D))
    return;
  DeclContext *DC = Decl::castToDeclContext(D);
  for (auto DCI = DC->decls_begin(), E = DC->decls_end(); DCI != E; ++DCI) {
    if (filter_implicitGenerated && (*DCI)->isImplicit())
      continue;
    SubDecls.push_back(*DCI);
  }
}
