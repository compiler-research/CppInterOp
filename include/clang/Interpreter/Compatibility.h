//--------------------------------------------------------------------*- C++ -*-
// CppInterOp Compatibility
// author:  Alexander Penev <alexander_penev@yahoo.com>
//------------------------------------------------------------------------------

#ifndef CPPINTEROP_COMPATIBILITY_H
#define CPPINTEROP_COMPATIBILITY_H

#include "clang/AST/GlobalDecl.h"
#include "clang/Basic/Version.h"
#include "clang/Config/config.h"

#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Config/llvm-config.h"
#include "llvm/ExecutionEngine/JITSymbol.h"
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/Support/Path.h"

#ifdef USE_CLING

#include "cling/Interpreter/DynamicLibraryManager.h"
#include "cling/Interpreter/Interpreter.h"
#include "cling/Interpreter/Transaction.h"
#include "cling/Interpreter/Value.h"

#include "cling/Utils/AST.h"

namespace Cpp {
  namespace Cpp_utils = cling::utils;
}

namespace compat {

  using Interpreter = cling::Interpreter;

  inline void maybeMangleDeclName(const clang::GlobalDecl& GD, std::string& mangledName) {
    cling::utils::Analyze::maybeMangleDeclName(GD, mangledName);
  }

  inline llvm::orc::LLJIT* getExecutionEngine(const cling::Interpreter& I) {
    // FIXME: This is a horrible hack finding the llvm::orc::LLJIT by computing
    // the object offsets in Cling. We should add getExecutionEngine interface
    // to directly.

    // sizeof (m_Opts) + sizeof(m_LLVMContext)
    const unsigned m_ExecutorOffset = 72;
    int* IncrementalExecutor = ((int*)(&I)) + m_ExecutorOffset;
    int* IncrementalJit = *(int**)IncrementalExecutor + 0;
    int* LLJIT = *(int**)IncrementalJit + 0;
    return *(llvm::orc::LLJIT**)LLJIT;
  }

  inline llvm::Expected<llvm::JITTargetAddress>
  getSymbolAddress(const cling::Interpreter &I, llvm::StringRef IRName) {
    if (void* Addr = I.getAddressOfGlobal(IRName))
      return (llvm::JITTargetAddress)Addr;

    llvm::orc::LLJIT& Jit = *compat::getExecutionEngine(I);
    llvm::orc::SymbolNameVector Names;
    Names.push_back(Jit.getExecutionSession().intern(IRName));
    return llvm::make_error<llvm::orc::SymbolsNotFound>(Names);
  }
}

#endif //USE_CLING


#ifdef USE_REPL

#include "clang/AST/Mangle.h"
#include "clang/Interpreter/DynamicLibraryManager.h"
#include "clang/Interpreter/Interpreter.h"
#include "clang/Interpreter/Value.h"

#include "llvm/Support/Error.h"

namespace compat {

inline std::unique_ptr<clang::Interpreter>
createClangInterpreter(std::vector<const char*>& args) {
#if CLANG_VERSION_MAJOR < 16
  auto ciOrErr = clang::IncrementalCompilerBuilder::create(args);
#else
  clang::IncrementalCompilerBuilder CB;
  CB.SetCompilerArgs(args);
  auto ciOrErr = CB.CreateCpp();
#endif // CLANG_VERSION_MAJOR < 16
  if (!ciOrErr) {
    llvm::logAllUnhandledErrors(ciOrErr.takeError(), llvm::errs(),
                                "Failed to build Incremental compiler:");
    return nullptr;
  }
  auto innerOrErr = clang::Interpreter::create(std::move(*ciOrErr));

  if (!innerOrErr) {
    llvm::logAllUnhandledErrors(innerOrErr.takeError(), llvm::errs(),
                                "Failed to build Interpreter:");
    return nullptr;
  }
  return std::move(*innerOrErr);
}

  inline void maybeMangleDeclName(const clang::GlobalDecl& GD, std::string& mangledName) {
    // copied and adapted from CodeGen::CodeGenModule::getMangledName

    clang::NamedDecl* D = llvm::cast<clang::NamedDecl>(const_cast<clang::Decl*>(GD.getDecl()));
    std::unique_ptr<clang::MangleContext> mangleCtx;
    mangleCtx.reset(D->getASTContext().createMangleContext());
    if (!mangleCtx->shouldMangleDeclName(D)) {
      clang::IdentifierInfo *II = D->getIdentifier();
      assert(II && "Attempt to mangle unnamed decl.");
      mangledName = II->getName().str();
      return;
    }

    llvm::raw_string_ostream RawStr(mangledName);

#if defined(_WIN32)
    // MicrosoftMangle.cpp:954 calls llvm_unreachable when mangling Dtor_Comdat
    if (isa<clang::CXXDestructorDecl>(GD.getDecl()) &&
        GD.getDtorType() == Dtor_Comdat) {
      if (const clang::IdentifierInfo* II = D->getIdentifier())
        RawStr << II->getName();
    } else
#endif
      mangleCtx->mangleName(GD, RawStr);
    RawStr.flush();
  }

  // Clang 13 - Initial implementation of Interpreter and clang-repl
  // Clang 14 - Add new Interpreter methods: getExecutionEngine,
  //            getSymbolAddress, getSymbolAddressFromLinkerName
  // Clang 15 - Add new Interpreter methods: Undo

  inline llvm::orc::LLJIT* getExecutionEngine(clang::Interpreter& I) {
#if CLANG_VERSION_MAJOR >= 14
    auto* engine = &llvm::cantFail(I.getExecutionEngine());
    return const_cast<llvm::orc::LLJIT*>(engine);
#else
    assert(0 && "Not implemented in Clang <14!");
    return nullptr;
#endif
  }

  inline llvm::Expected<llvm::JITTargetAddress>
  getSymbolAddress(clang::Interpreter& I, llvm::StringRef IRName) {
#if CLANG_VERSION_MAJOR < 14
    assert(0 && "Not implemented in Clang <14!");
    return llvm::createStringError(llvm::inconvertibleErrorCode(),
                                   "Not implemented in Clang <14!");
#endif // CLANG_VERSION_MAJOR < 14

    llvm::orc::LLJIT* Jit = compat::getExecutionEngine(I);
    llvm::orc::JITDylib& DyLib = Jit->getMainJITDylib();

    auto AddrOrErr = Jit->lookup(DyLib, IRName);
    if (llvm::Error Err = AddrOrErr.takeError())
      return std::move(Err);
    return AddrOrErr->getValue();
  }

  inline llvm::Expected<llvm::JITTargetAddress>
  getSymbolAddress(clang::Interpreter& I, clang::GlobalDecl GD) {
    std::string MangledName;
    compat::maybeMangleDeclName(GD, MangledName);
    return getSymbolAddress(I, llvm::StringRef(MangledName));
  }

  inline llvm::Expected<llvm::JITTargetAddress>
  getSymbolAddressFromLinkerName(const clang::Interpreter &I, llvm::StringRef LinkerName) {
#if CLANG_VERSION_MAJOR >= 14
    auto AddrOrErr = I.getSymbolAddressFromLinkerName(LinkerName);
    if (llvm::Error Err = AddrOrErr.takeError())
      return std::move(Err);
    return AddrOrErr->getValue();
#else
    assert(0 && "Not implemented in Clang <14!");
    return llvm::createStringError(llvm::inconvertibleErrorCode(),
                                   "Not implemented in Clang <14!");
#endif
  }

  inline llvm::Error Undo(clang::Interpreter &I, unsigned N = 1) {
#if CLANG_VERSION_MAJOR >= 15
    return I.Undo(N);
#else
    assert(0 && "Not implemented in Clang <15!");
    return llvm::createStringError(llvm::inconvertibleErrorCode(),
                                   "Not implemented in Clang <15!");
#endif
  }

}

#include "clang/Interpreter/CppInterOpInterpreter.h"

namespace Cpp {
  namespace Cpp_utils = Cpp::utils;
}

namespace compat {
  using Interpreter = Cpp::Interpreter;
}

#endif //USE_REPL


namespace compat {

// Clang >= 14 change type name to string (spaces formatting problem)
#if CLANG_VERSION_MAJOR >= 14
  inline std::string FixTypeName(const std::string type_name) {
    return type_name;
  }
#else
  inline std::string FixTypeName(const std::string type_name) {
    std::string result = type_name;
    size_t pos = 0;
    while ((pos = result.find(" [", pos)) != std::string::npos) {
      result.erase(pos, 1);
      pos++;
    }
    return result;
  }
#endif
  
// Clang >= 16 change CLANG_LIBDIR_SUFFIX to CLANG_INSTALL_LIBDIR_BASENAME
#if CLANG_VERSION_MAJOR < 16
  #define CLANG_INSTALL_LIBDIR_BASENAME (llvm::Twine("lib")+CLANG_LIBDIR_SUFFIX)
#endif
inline std::string MakeResourceDir(llvm::StringRef Dir) {
  llvm::SmallString<128> P(Dir);
  llvm::sys::path::append(P, CLANG_INSTALL_LIBDIR_BASENAME, "clang",
#if CLANG_VERSION_MAJOR < 16
    CLANG_VERSION_STRING
#else
    CLANG_VERSION_MAJOR_STRING
#endif
  );
  return std::string(P.str());
}

// Clang >= 16 (=16 with Value patch) change castAs to converTo
#if CLANG_VERSION_MAJOR >= 16
template <typename T>
inline T convertTo(
#ifdef USE_CLING
    cling::Value V
#else
    clang::Value V
#endif
) {
  return V.convertTo<T>();
}
#else
template <typename T>
inline T convertTo(
#ifdef USE_CLING
    cling::Value V
#else
    clang::Value V
#endif
) {
  return V.castAs<T>();
}
#endif

} // end compat namespace

#endif //CPPINTEROP_COMPATIBILITY_H
