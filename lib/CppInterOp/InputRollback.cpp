//===--- InputRollback.cpp - Undo state an input leaves outside its decls -===//
//
// Part of the compiler-research project, under the Apache License v2.0 with
// LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "InputRollback.h"

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclTemplate.h"
#include "clang/Basic/FileEntry.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/Specifiers.h"
#include "clang/Basic/Version.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/HeaderSearch.h"
#include "clang/Lex/MacroInfo.h"
#include "clang/Lex/PPCallbacks.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Sema/Sema.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"

#if CLANG_VERSION_MAJOR >= 22
#include "clang/Frontend/MultiplexConsumer.h"
#endif

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace CppInternal {

struct InputRollback::State {
  clang::CompilerInstance* CI;
  size_t Input = 0;
  // Implicit instantiations the current input requested.
  llvm::SmallVector<clang::FunctionDecl*, 8> Funcs;
  llvm::SmallVector<clang::VarDecl*, 4> Vars;
  struct MacroEvent {
    size_t Input;
    clang::IdentifierInfo* II;
    const clang::MacroDirective* MD;
  };
  std::vector<MacroEvent> Macros;
  // Files first entered by an input.
  struct FileEvent {
    size_t Input;
    clang::FileEntryRef File;
  };
  std::vector<FileEvent> Files;
  llvm::SmallPtrSet<const clang::FileEntry*, 32> Seen;

  explicit State(clang::CompilerInstance& CI) : CI(&CI) {}

  void resetInstantiations();
  void revertPreprocessor(size_t From);
};

namespace {

class InstantiationRecorder : public clang::ASTConsumer {
  std::shared_ptr<InputRollback::State> S;

public:
  explicit InstantiationRecorder(std::shared_ptr<InputRollback::State> S)
      : S(std::move(S)) {}
  void
  HandleCXXImplicitFunctionInstantiation(clang::FunctionDecl* FD) override {
    S->Funcs.push_back(FD);
  }
  void HandleCXXStaticMemberVarInstantiation(clang::VarDecl* VD) override {
    S->Vars.push_back(VD);
  }
};

class PreprocessorRecorder : public clang::PPCallbacks {
  std::shared_ptr<InputRollback::State> S;

public:
  explicit PreprocessorRecorder(std::shared_ptr<InputRollback::State> S)
      : S(std::move(S)) {}
  void MacroDefined(const clang::Token& Name,
                    const clang::MacroDirective* MD) override {
    S->Macros.push_back({S->Input, Name.getIdentifierInfo(), MD});
  }
  void MacroUndefined(const clang::Token& Name, const clang::MacroDefinition&,
                      const clang::MacroDirective* Undef) override {
    if (Undef)
      S->Macros.push_back({S->Input, Name.getIdentifierInfo(), Undef});
  }
  void FileChanged(clang::SourceLocation Loc, FileChangeReason Reason,
                   clang::SrcMgr::CharacteristicKind, clang::FileID) override {
    if (Reason != EnterFile)
      return;
    clang::SourceManager& SM = S->CI->getSourceManager();
    if (clang::OptionalFileEntryRef FE =
            SM.getFileEntryRefForID(SM.getFileID(Loc)))
      if (S->Seen.insert(&FE->getFileEntry()).second)
        S->Files.push_back({S->Input, *FE});
  }
};

} // namespace

void InputRollback::State::resetInstantiations() {
  // Sema treats these as done, but their bodies never reached CodeGen.
  llvm::SmallPtrSet<const clang::Decl*, 8> Reset;
  for (clang::FunctionDecl* FD : Funcs) {
    if (FD->getTemplateSpecializationKind() !=
            clang::TSK_ImplicitInstantiation ||
        FD->isLateTemplateParsed())
      continue;
    FD->setBody(nullptr);
    FD->setWillHaveBody(false);
    FD->setInstantiationIsPending(false);
    if (auto* FTSI = FD->getTemplateSpecializationInfo())
      FTSI->setPointOfInstantiation(clang::SourceLocation());
    else if (auto* MSI = FD->getMemberSpecializationInfo())
      MSI->setPointOfInstantiation(clang::SourceLocation());
    Reset.insert(FD);
  }
  for (clang::VarDecl* VD : Vars) {
    if (VD->getTemplateSpecializationKind() != clang::TSK_ImplicitInstantiation)
      continue;
    if (auto* MSI = VD->getMemberSpecializationInfo())
      MSI->setPointOfInstantiation(clang::SourceLocation());
    Reset.insert(VD);
  }
  clang::Sema& Sema = CI->getSema();
  auto Queued = [&](const auto& P) { return Reset.count(P.first); };
  llvm::erase_if(Sema.PendingInstantiations, Queued);
  llvm::erase_if(Sema.PendingLocalImplicitInstantiations, Queued);
  Funcs.clear();
  Vars.clear();
}

void InputRollback::State::revertPreprocessor(size_t From) {
  clang::Preprocessor& PP = CI->getPreprocessor();
  // Newest first: each step restores the state before its directive.
  while (!Macros.empty() && Macros.back().Input >= From) {
    MacroEvent E = Macros.back();
    Macros.pop_back();
    const clang::MacroDirective* Prev = E.MD->getPrevious();
    const clang::MacroInfo* MI = Prev ? Prev->getMacroInfo() : nullptr;
    if (MI)
      PP.appendDefMacroDirective(
          // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
          E.II, const_cast<clang::MacroInfo*>(MI), E.MD->getLocation());
    else if (PP.isMacroDefined(E.II))
      PP.appendMacroDirective(
          // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
          E.II, new (PP.getPreprocessorAllocator())
                    clang::UndefMacroDirective(E.MD->getLocation()));
  }
  // Include guards are macros; #pragma once and #import need this.
  clang::HeaderSearch& HS = PP.getHeaderSearchInfo();
  while (!Files.empty() && Files.back().Input >= From) {
    clang::FileEntryRef File = Files.back().File;
    Files.pop_back();
    Seen.erase(&File.getFileEntry());
    PP.getIncludedFiles().erase(&File.getFileEntry());
    clang::HeaderFileInfo& HFI = HS.getFileInfo(File);
    HFI.isPragmaOnce = false;
    HFI.isImport = false;
  }
}

InputRollback::InputRollback(std::shared_ptr<State> S) : S(std::move(S)) {}
InputRollback::~InputRollback() = default;

std::unique_ptr<InputRollback>
InputRollback::install(clang::CompilerInstance& CI) {
#if CLANG_VERSION_MAJOR >= 22
  // The consumer forwards these callbacks also in a failed input.
  struct Access : clang::MultiplexConsumer {
    static void add(clang::MultiplexConsumer& M,
                    std::unique_ptr<clang::ASTConsumer> C) {
      (M.*(&Access::Consumers)).push_back(std::move(C));
    }
  };
  auto St = std::make_shared<State>(CI);
  // clang builds without RTTI.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
  Access::add(static_cast<clang::MultiplexConsumer&>(CI.getASTConsumer()),
              std::make_unique<InstantiationRecorder>(St));
  CI.getPreprocessor().addPPCallbacks(
      std::make_unique<PreprocessorRecorder>(St));
  return std::make_unique<InputRollback>(std::move(St));
#else
  (void)CI;
  return nullptr;
#endif
}

void InputRollback::beginInput(size_t Index) {
  S->Input = Index;
  S->Funcs.clear();
  S->Vars.clear();
}

void InputRollback::failedParse() {
  S->resetInstantiations();
  S->revertPreprocessor(S->Input);
}

void InputRollback::withdrawFrom(size_t Keep) { S->revertPreprocessor(Keep); }

} // namespace CppInternal
