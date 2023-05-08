//--------------------------------------------------------------------*- C++ -*-
// InterOp Interpreter (clang-repl)
// author:  Alexander Penev <alexander_penev@yahoo.com>
//------------------------------------------------------------------------------

#ifndef INTEROP_INTERPRETER_H
#define INTEROP_INTERPRETER_H

#include "clang/Interpreter/Compatibility.h"
#include "clang/Interpreter/DynamicLibraryManager.h"
#include "clang/Interpreter/Interpreter.h"
#include "clang/Interpreter/PartialTranslationUnit.h"
#include "clang/Interpreter/Paths.h"

#include "clang/AST/Decl.h"
#include "clang/AST/DeclarationName.h"
#include "clang/AST/GlobalDecl.h"
#include "clang/Basic/LangOptions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendOptions.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Sema/Lookup.h"
#include "clang/Sema/Sema.h"

#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/TargetSelect.h"

namespace clang {
  class CompilerInstance;
}

namespace {
  template<typename D>
  static D* LookupResult2Decl(clang::LookupResult& R)
  {
    if (R.empty())
      return nullptr;

    R.resolveKind();

    if (R.isSingleResult())
      return llvm::dyn_cast<D>(R.getFoundDecl());
    return (D*)-1;
  }
}

namespace InterOp {
  namespace utils {
    namespace Lookup {

  inline clang::NamespaceDecl* Namespace(clang::Sema* S, const char* Name,
                                         const clang::DeclContext* Within) {
    clang::DeclarationName DName = &(S->Context.Idents.get(Name));
    clang::LookupResult R(*S, DName, clang::SourceLocation(),
                   clang::Sema::LookupNestedNameSpecifierName);
    R.suppressDiagnostics();
    if (!Within)
      S->LookupName(R, S->TUScope);
    else {
      if (const clang::TagDecl* TD = llvm::dyn_cast<clang::TagDecl>(Within)) {
        if (!TD->getDefinition()) {
          // No definition, no lookup result.
          return nullptr;
        }
      }
      S->LookupQualifiedName(R, const_cast<clang::DeclContext*>(Within));
    }

    if (R.empty())
      return nullptr;

    R.resolveKind();

    return llvm::dyn_cast<clang::NamespaceDecl>(R.getFoundDecl());
  }

  inline void Named(clang::Sema* S, clang::LookupResult& R,
                    const clang::DeclContext* Within = nullptr) {
    R.suppressDiagnostics();
    if (!Within)
      S->LookupName(R, S->TUScope);
    else {
      const clang::DeclContext* primaryWithin = nullptr;
      if (const clang::TagDecl *TD = llvm::dyn_cast<clang::TagDecl>(Within)) {
        primaryWithin = llvm::dyn_cast_or_null<clang::DeclContext>(TD->getDefinition());
      } else {
        primaryWithin = Within->getPrimaryContext();
      }
      if (!primaryWithin) {
        // No definition, no lookup result.
        return;
      }
      S->LookupQualifiedName(R, const_cast<clang::DeclContext*>(primaryWithin));
    }
  }

  inline clang::NamedDecl* Named(clang::Sema* S, const clang::DeclarationName& Name,
                                 const clang::DeclContext* Within = nullptr) {
    clang::LookupResult R(*S, Name, clang::SourceLocation(), clang::Sema::LookupOrdinaryName,
                          clang::Sema::ForVisibleRedeclaration);
    Named(S, R, Within);
    return LookupResult2Decl<clang::NamedDecl>(R);
  }

  inline clang::NamedDecl* Named(clang::Sema* S, llvm::StringRef Name,
                                 const clang::DeclContext* Within = nullptr) {
    clang::DeclarationName DName = &S->Context.Idents.get(Name);
    return Named(S, DName, Within);
  }

  inline clang::NamedDecl* Named(clang::Sema* S, const char* Name,
                                 const clang::DeclContext* Within = nullptr) {
    return Named(S, llvm::StringRef(Name), Within);
  }

    } // Lookup
  }  // utils
} // InterOp

namespace InterOp {

/// InterOp Interpreter
///
class Interpreter {
private:
  std::unique_ptr<clang::Interpreter> inner;

public:
  Interpreter(int argc, const char* const *argv,
              const char* llvmdir = 0,
              const std::vector<std::shared_ptr<clang::ModuleFileExtension>>& moduleExtensions = {},
              void *extraLibHandle = 0, bool noRuntime = true) {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();

    std::vector<const char *> vargs(argv + 1, argv + argc);
    if (auto ciOrErr = clang::IncrementalCompilerBuilder::create(vargs)) {
      if (auto innerOrErr = clang::Interpreter::create(std::move(*ciOrErr))) {
        inner = std::move(*innerOrErr);
      } else {
        llvm::logAllUnhandledErrors(innerOrErr.takeError(), llvm::errs(), "Failed to build Interpreter:");
      }
    } else {
      llvm::logAllUnhandledErrors(ciOrErr.takeError(), llvm::errs(), "Failed to build Incremental compiler:");
    }
  }

  ~Interpreter() {}

  operator const clang::Interpreter&() const { return *inner; }

  ///\brief Describes the return result of the different routines that do the
  /// incremental compilation.
  ///
  enum CompilationResult {
    kSuccess,
    kFailure,
    kMoreInputExpected
  };

  const clang::CompilerInstance *getCompilerInstance() const {
    return inner->getCompilerInstance();
  }

  const llvm::orc::LLJIT *getExecutionEngine() const {
    return compat::getExecutionEngine(*inner);
  }

  llvm::Expected<clang::PartialTranslationUnit &> Parse(llvm::StringRef Code) {
    return inner->Parse(Code);
  }

  llvm::Error Execute(clang::PartialTranslationUnit &T) {
    return inner->Execute(T);
  }

  llvm::Error ParseAndExecute(llvm::StringRef Code, clang::Value * V = nullptr) {
    return inner->ParseAndExecute(Code, V);
  }

  llvm::Error Undo(unsigned N = 1) {
    return compat::Undo(*inner, N);
  }

  void makeEngineOnce() const {
    static bool make_engine_once = true;
    if (make_engine_once) {
      if (auto Err = inner->ParseAndExecute(""))
        llvm::logAllUnhandledErrors(std::move(Err), llvm::errs(), "Error:");
      make_engine_once = false;
    }
  }

  /// \returns the \c ExecutorAddr of a \c GlobalDecl. This interface uses
  /// the CodeGenModule's internal mangling cache to avoid recomputing the
  /// mangled name.
  llvm::Expected<llvm::orc::ExecutorAddr>
  getSymbolAddress(clang::GlobalDecl GD) const {
    makeEngineOnce();
    auto AddrOrErr = compat::getSymbolAddress(*inner, GD);
    if (llvm::Error Err = AddrOrErr.takeError())
      return std::move(Err);
    return llvm::orc::ExecutorAddr(*AddrOrErr);
  }

  /// \returns the \c ExecutorAddr of a given name as written in the IR.
  llvm::Expected<llvm::orc::ExecutorAddr>
  getSymbolAddress(llvm::StringRef IRName) const {
    makeEngineOnce();
    auto AddrOrErr = compat::getSymbolAddress(*inner, IRName);
    if (llvm::Error Err = AddrOrErr.takeError())
      return std::move(Err);
    return llvm::orc::ExecutorAddr(*AddrOrErr);
  }

  /// \returns the \c ExecutorAddr of a given name as written in the object
  /// file.
  llvm::Expected<llvm::orc::ExecutorAddr>
  getSymbolAddressFromLinkerName(llvm::StringRef LinkerName) const {
    auto AddrOrErr = compat::getSymbolAddressFromLinkerName(*inner, LinkerName);
    if (llvm::Error Err = AddrOrErr.takeError())
      return std::move(Err);
    return llvm::orc::ExecutorAddr(*AddrOrErr);
  }

  bool isInSyntaxOnlyMode() const {
    return getCompilerInstance()->getFrontendOpts().ProgramAction
      == clang::frontend::ParseSyntaxOnly;
  }

  // FIXME: Mangle GD and call the other overload.
  void* getAddressOfGlobal(const clang::GlobalDecl& GD) const {
    auto addressOrErr = getSymbolAddress(GD);
    if (addressOrErr)
      return addressOrErr->toPtr<void*>();

    llvm::consumeError(addressOrErr.takeError()); // okay to be missing
    return nullptr;
  }

  void* getAddressOfGlobal(llvm::StringRef SymName) const {
    if (isInSyntaxOnlyMode())
      return nullptr;

    auto addressOrErr = getSymbolAddressFromLinkerName(SymName); //TODO: Or getSymbolAddress
    if (addressOrErr)
      return addressOrErr->toPtr<void*>();

    llvm::consumeError(addressOrErr.takeError()); // okay to be missing
    return nullptr;
  }

  CompilationResult
  declare(const std::string& input, clang::PartialTranslationUnit **PTU = nullptr) {
    return process(input, /*Value=*/nullptr, PTU);
  }

  ///\brief Maybe transform the input line to implement cint command line
  /// semantics (declarations are global) and compile to produce a module.
  ///
  CompilationResult
  process(const std::string& input, clang::Value* V = 0,
          clang::PartialTranslationUnit **PTU = nullptr,
          bool disableValuePrinting = false) {
    auto PTUOrErr = Parse(input);
    if (!PTUOrErr) {
      llvm::logAllUnhandledErrors(PTUOrErr.takeError(), llvm::errs(), "Failed to parse via ::process:");
      return Interpreter::kFailure;
    }
    if (PTU)
      *PTU = &*PTUOrErr;

    if (auto Err = Execute(*PTUOrErr)) {
      llvm::logAllUnhandledErrors(std::move(Err), llvm::errs(), "Failed to execute via ::process:");
      return Interpreter::kFailure;
    }
    return Interpreter::kSuccess;
  }

  CompilationResult
  evaluate(const std::string& input, clang::Value& V) {
    if (auto Err = ParseAndExecute(input, &V)) {
      llvm::logAllUnhandledErrors(std::move(Err), llvm::errs(), "Failed to execute via ::evaluate:");
      return Interpreter::kFailure;
    }
    return Interpreter::kSuccess;
  }

  void* compileFunction(llvm::StringRef name, llvm::StringRef code,
                        bool ifUnique, bool withAccessControl) {
    //
    //  Compile the wrapper code.
    //

    if (isInSyntaxOnlyMode())
      return nullptr;

    if (ifUnique) {
      if (void* Addr = (void*)getAddressOfGlobal(name)) {
        return Addr;
      }
    }

    clang::LangOptions& LO
      = const_cast<clang::LangOptions&>(getCompilerInstance()->getLangOpts());
    bool SavedAccessControl = LO.AccessControl;
    LO.AccessControl = withAccessControl;

    if (auto Err = ParseAndExecute(code)) {
      LO.AccessControl = SavedAccessControl;
      llvm::logAllUnhandledErrors(std::move(Err), llvm::errs(),
                                  "Failed to compileFunction: ");
      return nullptr;
    }

    LO.AccessControl = SavedAccessControl;

    return getAddressOfGlobal(name);
  }

  const clang::CompilerInstance *getCI() const {
    return getCompilerInstance();
  }

  clang::Sema& getSema() const {
    return getCI()->getSema();
  }

  const DynamicLibraryManager* getDynamicLibraryManager() const {
    assert(compat::getExecutionEngine(*inner) && "We must have an executor");
    static const DynamicLibraryManager *DLM = new DynamicLibraryManager();
    return DLM;
    //TODO: Add DLM to InternalExecutor and use executor->getDML()
    //      return inner->getExecutionEngine()->getDynamicLibraryManager();
  }

  DynamicLibraryManager* getDynamicLibraryManager() {
    return const_cast<DynamicLibraryManager*>(
      const_cast<const Interpreter*>(this)->getDynamicLibraryManager()
    );
  }

  ///\brief Adds multiple include paths separated by a delimter.
  ///
  ///\param[in] PathsStr - Path(s)
  ///\param[in] Delim - Delimiter to separate paths or NULL if a single path
  ///
  void AddIncludePaths(llvm::StringRef PathsStr, const char* Delim = ":") {
    const clang::CompilerInstance* CI = getCompilerInstance();
    clang::HeaderSearchOptions& HOpts =
      const_cast<clang::HeaderSearchOptions&>(CI->getHeaderSearchOpts());

    // Save the current number of entries
    size_t Idx = HOpts.UserEntries.size();
    InterOp::utils::AddIncludePaths(PathsStr, HOpts, Delim);

    clang::Preprocessor& PP = CI->getPreprocessor();
    clang::SourceManager& SM = PP.getSourceManager();
    clang::FileManager& FM = SM.getFileManager();
    clang::HeaderSearch& HSearch = PP.getHeaderSearchInfo();
    const bool isFramework = false;

    // Add all the new entries into Preprocessor
    for (const size_t N = HOpts.UserEntries.size(); Idx < N; ++Idx) {
      const clang::HeaderSearchOptions::Entry& E = HOpts.UserEntries[Idx];
      if (auto DE = FM.getOptionalDirectoryRef(E.Path))
        HSearch.AddSearchPath(clang::DirectoryLookup(*DE, clang::SrcMgr::C_User, isFramework),
                              E.Group == clang::frontend::Angled);
    }
  }

  ///\brief Adds a single include path (-I).
  ///
  void AddIncludePath(llvm::StringRef PathsStr) {
    return AddIncludePaths(PathsStr, nullptr);
  }

  CompilationResult
  loadLibrary(const std::string& filename, bool lookup) {
    DynamicLibraryManager* DLM = getDynamicLibraryManager();
    std::string canonicalLib;
    if (lookup)
      canonicalLib = DLM->lookupLibrary(filename);

    const std::string &library = lookup ? canonicalLib : filename;
    if (!library.empty()) {
      switch (DLM->loadLibrary(library, /*permanent*/false, /*resolved*/true)) {
      case DynamicLibraryManager::kLoadLibSuccess: // Intentional fall through
      case DynamicLibraryManager::kLoadLibAlreadyLoaded:
        return kSuccess;
      case DynamicLibraryManager::kLoadLibNotFound:
        assert(0 && "Cannot find library with existing canonical name!");
        return kFailure;
      default:
        // Not a source file (canonical name is non-empty) but can't load.
        return kFailure;
      }
    }
    return kMoreInputExpected;
  }

  std::string toString(const char* type, void* obj) {
    assert(0 && "toString is not implemented!");
    std::string ret;
    return ret; //TODO: Implement
  }

  }; //Interpreter
} //InterOp

#endif //INTEROP_INTERPRETER_H
