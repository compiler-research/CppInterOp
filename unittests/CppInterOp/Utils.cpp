#include "Utils.h"

#include "CppInterOp/CppInterOp.h"

#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/Basic/Version.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Sema/Lookup.h"
#include "clang/Sema/Sema.h"

#include "llvm/TargetParser/Triple.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <future>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

using namespace clang;
using namespace llvm;
using namespace std::chrono_literals;

Cpp::TInterp_t TestUtils::GetAllTopLevelDecls(
    const std::string& code, std::vector<Decl*>& Decls,
    bool filter_implicitGenerated /* = false */,
    const std::vector<const char*>& interpreter_args /* = {} */) {
  Cpp::TInterp_t I = Cpp::CreateInterpreter(interpreter_args);
  auto* Interp = static_cast<compat::Interpreter*>(I);
#ifdef CPPINTEROP_USE_CLING
  cling::Transaction *T = nullptr;
  Interp->declare(code, &T);

  for (auto DCI = T->decls_begin(), E = T->decls_end(); DCI != E; ++DCI) {
    if (DCI->m_Call != cling::Transaction::kCCIHandleTopLevelDecl)
      continue;
    for (Decl *D : DCI->m_DGR) {
      if (filter_implicitGenerated && D->isImplicit())
        continue;
      Decls.push_back(D);
    }
  }
#else
  PartialTranslationUnit *T = nullptr;
  Interp->process(code, /*Value*/nullptr, &T);
  for (auto *D : T->TUPart->decls()) {
    if (filter_implicitGenerated && D->isImplicit())
      continue;
    Decls.push_back(D);
  }
#endif
  return I;
}

void TestUtils::GetAllSubDecls(Decl *D, std::vector<Decl*>& SubDecls,
                               bool filter_implicitGenerated /* = false */) {
  if (!isa_and_nonnull<DeclContext>(D))
    return;
  DeclContext *DC = cast<DeclContext>(D);
  for (auto *Di : DC->decls()) {
    if (filter_implicitGenerated && Di->isImplicit())
      continue;
    SubDecls.push_back(Di);
  }
}

bool IsTargetX86(Cpp::TInterp_t I) {
  auto* Interp = static_cast<compat::Interpreter*>(I);
#ifndef CPPINTEROP_USE_CLING
  llvm::Triple triple(Interp->getCompilerInstance()->getTargetOpts().Triple);
#else
  llvm::Triple triple(Interp->getCI()->getTargetOpts().Triple);
#endif
  return triple.isX86();
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

void ThreadPoolExecutor::run(
    std::vector<std::pair<const char*, void (*)()>>& fns, unsigned runners) {
#ifndef EMSCRIPTEN
  std::vector<std::future<void>> futures;
  std::vector<const char*> names;
  for (size_t i = 0, size = fns.size(); i < size;) {
    if (futures.size() != runners) {
      std::cout << "Running: " << std::get<0>(fns[i]) << "\n";
      futures.emplace_back(std::async(std::launch::async, std::get<1>(fns[i])));
      names.emplace_back(std::get<0>(fns[i]));
      i++;
      continue;
    }
    assert(futures.size() == names.size());
    for (size_t i = 0, size = futures.size(); i < size; i++) {
      if (futures[0].wait_for(0ms) == std::future_status::ready) {
        std::cout << "Finished: " << names[0] << "\n";
        futures.erase(futures.begin());
        names.erase(names.begin());
      }
    }
  }
  assert(futures.size() == names.size());
  for (size_t i = 0, size = futures.size(); i < size; i++) {
    futures[0].wait();
    std::cout << "Finished: " << names[0] << "\n";
    futures.erase(futures.begin());
    names.erase(names.begin());
  }
#else
  // emscripten does not support threads
  for (auto& i : fns) {
    std::cout << "Running: " << std::get<0>(i) << "\n";
    std::get<1>(i)();
    std::cout << "Finished: " << std::get<0>(i) << "\n";
  }
#endif
}
