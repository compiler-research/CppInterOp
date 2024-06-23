#include "clang-c/CXCppInterOp.h"
#include "Compatibility.h"
#include "clang/AST/CXXInheritance.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/Mangle.h"
#include "clang/AST/RecordLayout.h"
#include "clang/AST/Type.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Interpreter/CppInterOp.h"
#include "clang/Sema/Lookup.h"
#include "clang/Sema/Sema.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/Support/Casting.h"
#include <cstring>
#include <iterator>
#include "clang-c/CXString.h"

CXString makeCXString(const std::string& S) {
  CXString Str;
  if (S.empty()) {
    Str.data = "";
    Str.private_flags = 0; // CXS_Unmanaged
  } else {
    Str.data = strdup(S.c_str());
    Str.private_flags = 1; // CXS_Malloc
  }
  return Str;
}

CXStringSet* makeCXStringSet(const std::vector<std::string>& Strs) {
  auto* Set = new CXStringSet; // NOLINT(*-owning-memory)
  Set->Count = Strs.size();
  Set->Strings = new CXString[Set->Count]; // NOLINT(*-owning-memory)
  for (auto En : llvm::enumerate(Strs)) {
    Set->Strings[En.index()] = makeCXString(En.value());
  }
  return Set;
}

struct CXInterpreterImpl {
  std::unique_ptr<compat::Interpreter> Interp;
  // reserved for future use
};

static inline compat::Interpreter* getInterpreter(const CXInterpreterImpl* I) {
  assert(I && "Invalid interpreter");
  return I->Interp.get();
}

CXInterpreter clang_createInterpreter(const char* const* argv, int argc) {
  auto* I = new CXInterpreterImpl(); // NOLINT(*-owning-memory)
  I->Interp = std::make_unique<compat::Interpreter>(argc, argv);
  // reserved for future use
  return I;
}

CXInterpreter clang_createInterpreterFromPtr(TInterp_t I) {
  auto* II = new CXInterpreterImpl(); // NOLINT(*-owning-memory)
  II->Interp.reset(static_cast<compat::Interpreter*>(I)); // NOLINT(*-cast)
  return II;
}

TInterp_t clang_interpreter_getInterpreterAsPtr(CXInterpreter I) {
  return getInterpreter(I);
}

TInterp_t clang_interpreter_takeInterpreterAsPtr(CXInterpreter I) {
  return static_cast<CXInterpreterImpl*>(I)->Interp.release();
}

CXErrorCode clang_interpreter_Undo(CXInterpreter I, unsigned int N) {
  return getInterpreter(I)->Undo(N) ? CXError_Failure : CXError_Success;
}

void clang_interpreter_dispose(CXInterpreter I) {
  delete I; // NOLINT(*-owning-memory)
}

void clang_interpreter_addSearchPath(CXInterpreter I, const char* dir,
                                     bool isUser, bool prepend) {
  auto* interp = getInterpreter(I);
  interp->getDynamicLibraryManager()->addSearchPath(dir, isUser, prepend);
}

void clang_interpreter_addIncludePath(CXInterpreter I, const char* dir) {
  getInterpreter(I)->AddIncludePath(dir);
}

const char* clang_interpreter_getResourceDir(CXInterpreter I) {
  const auto* interp = getInterpreter(I);
  return interp->getCI()->getHeaderSearchOpts().ResourceDir.c_str();
}

enum CXErrorCode clang_interpreter_declare(CXInterpreter I, const char* code,
                                           bool silent) {
  auto* interp = getInterpreter(I);
  auto& diag = interp->getSema().getDiagnostics();

  const bool is_silent_old = diag.getSuppressAllDiagnostics();

  diag.setSuppressAllDiagnostics(silent);
  const auto result = interp->declare(code);
  diag.setSuppressAllDiagnostics(is_silent_old);

  if (result)
    return CXError_Failure;

  return CXError_Success;
}

enum CXErrorCode clang_interpreter_process(CXInterpreter I, const char* code) {
  if (getInterpreter(I)->process(code))
    return CXError_Failure;

  return CXError_Success;
}

CXValue clang_createValue(void) {
#ifdef USE_CLING
  auto val = std::make_unique<cling::Value>();
#else
  auto val = std::make_unique<clang::Value>();
#endif // USE_CLING

  return val.release();
}

void clang_value_dispose(CXValue V) {
#ifdef USE_CLING
  delete static_cast<cling::Value*>(V); // NOLINT(*-owning-memory)
#else
  delete static_cast<clang::Value*>(V); // NOLINT(*-owning-memory)
#endif // USE_CLING
}

enum CXErrorCode clang_interpreter_evaluate(CXInterpreter I, const char* code,
                                            CXValue V) {
#ifdef USE_CLING
  auto* val = static_cast<cling::Value*>(V);
#else
  auto* val = static_cast<clang::Value*>(V);
#endif // USE_CLING

  if (getInterpreter(I)->evaluate(code, *val))
    return CXError_Failure;

  return CXError_Success;
}

CXString clang_interpreter_lookupLibrary(CXInterpreter I,
                                         const char* lib_name) {
  auto* interp = getInterpreter(I);
  return makeCXString(
      interp->getDynamicLibraryManager()->lookupLibrary(lib_name));
}

CXInterpreter_CompilationResult
clang_interpreter_loadLibrary(CXInterpreter I, const char* lib_stem,
                              bool lookup) {
  auto* interp = getInterpreter(I);
  return static_cast<CXInterpreter_CompilationResult>(
      interp->loadLibrary(lib_stem, lookup));
}

void clang_interpreter_unloadLibrary(CXInterpreter I, const char* lib_stem) {
  auto* interp = getInterpreter(I);
  interp->getDynamicLibraryManager()->unloadLibrary(lib_stem);
}

CXString clang_interpreter_searchLibrariesForSymbol(CXInterpreter I,
                                                    const char* mangled_name,
                                                    bool search_system) {
  auto* interp = getInterpreter(I);
  return makeCXString(
      interp->getDynamicLibraryManager()->searchLibrariesForSymbol(
          mangled_name, search_system));
}

namespace Cpp {
bool InsertOrReplaceJitSymbolImpl(compat::Interpreter& I,
                                  const char* linker_mangled_name,
                                  uint64_t address);
} // namespace Cpp

bool clang_interpreter_insertOrReplaceJitSymbol(CXInterpreter I,
                                                const char* linker_mangled_name,
                                                uint64_t address) {
  return Cpp::InsertOrReplaceJitSymbolImpl(*getInterpreter(I),
                                           linker_mangled_name, address);
}

CXFuncAddr
clang_interpreter_getFunctionAddressFromMangledName(CXInterpreter I,
                                                    const char* mangled_name) {
  auto* interp = getInterpreter(I);
  auto FDAorErr = compat::getSymbolAddress(*interp, mangled_name);
  if (llvm::Error Err = FDAorErr.takeError())
    llvm::consumeError(std::move(Err)); // nullptr if missing
  else
    return llvm::jitTargetAddressToPointer<void*>(*FDAorErr);

  return nullptr;
}

static inline CXQualType makeCXQualType(const CXInterpreterImpl* I,
                                        const clang::QualType Ty,
                                        const CXTypeKind K = CXType_Unexposed) {
  assert(I && "Invalid interpreter");
  return CXQualType{K, Ty.getAsOpaquePtr(), static_cast<const void*>(I)};
}

static inline clang::QualType getType(const CXQualType& Ty) {
  return clang::QualType::getFromOpaquePtr(Ty.data);
}

static inline const CXInterpreterImpl* getMeta(const CXQualType& Ty) {
  return static_cast<const CXInterpreterImpl*>(Ty.meta);
}

static inline compat::Interpreter* getInterpreter(const CXQualType& Ty) {
  return getInterpreter(static_cast<const CXInterpreterImpl*>(Ty.meta));
}

bool clang_qualtype_isBuiltin(CXQualType type) {
  return Cpp::IsBuiltin(type.data);
}

bool clang_qualtype_isEnumType(CXQualType type) {
  return getType(type)->isEnumeralType();
}

bool clang_qualtype_isSmartPtrType(CXQualType type) {
  return Cpp::IsSmartPtrType(type.data);
}

bool clang_scope_isRecordType(CXQualType type) {
  return getType(type)->isRecordType();
}

bool clang_scope_isPODType(CXQualType type) {
  const clang::QualType QT = getType(type);
  if (QT.isNull())
    return false;
  return QT.isPODType(getInterpreter(type)->getSema().getASTContext());
}

CXQualType clang_qualtype_getIntegerTypeFromEnumType(CXQualType type) {
  const void* Ptr = Cpp::GetIntegerTypeFromEnumType(type.data);
  return makeCXQualType(getMeta(type), clang::QualType::getFromOpaquePtr(Ptr));
}

CXQualType clang_qualtype_getUnderlyingType(CXQualType type) {
  clang::QualType QT = getType(type);
  QT = QT->getCanonicalTypeUnqualified();

  // Recursively remove array dimensions
  while (QT->isArrayType())
    QT = clang::QualType(QT->getArrayElementTypeNoTypeQual(), 0);

  // Recursively reduce pointer depth till we are left with a pointerless
  // type.
  for (auto PT = QT->getPointeeType(); !PT.isNull();
       PT = QT->getPointeeType()) {
    QT = PT;
  }
  QT = QT->getCanonicalTypeUnqualified();
  return makeCXQualType(getMeta(type), QT);
}

CXString clang_qualtype_getTypeAsString(CXQualType type) {
  const clang::QualType QT = getType(type);
  const auto& C = getInterpreter(type)->getSema().getASTContext();
  clang::PrintingPolicy Policy = C.getPrintingPolicy();
  Policy.Bool = true;               // Print bool instead of _Bool.
  Policy.SuppressTagKeyword = true; // Do not print `class std::string`.
  return makeCXString(compat::FixTypeName(QT.getAsString(Policy)));
}

CXQualType clang_qualtype_getCanonicalType(CXQualType type) {
  const clang::QualType QT = getType(type);
  if (QT.isNull())
    return makeCXQualType(getMeta(type), clang::QualType(), CXType_Invalid);

  return makeCXQualType(getMeta(type), QT.getCanonicalType());
}

namespace Cpp {
clang::QualType GetType(const std::string& name, clang::Sema& sema);
} // namespace Cpp

CXQualType clang_qualtype_getType(CXInterpreter I, const char* name) {
  auto& S = getInterpreter(I)->getSema();
  const clang::QualType QT = Cpp::GetType(std::string(name), S);
  if (QT.isNull())
    return makeCXQualType(I, QT, CXType_Invalid);

  return makeCXQualType(I, QT);
}

CXQualType clang_qualtype_getComplexType(CXQualType eltype) {
  const auto& C = getInterpreter(eltype)->getSema().getASTContext();
  return makeCXQualType(getMeta(eltype), C.getComplexType(getType(eltype)));
}

static inline CXScope makeCXScope(const CXInterpreterImpl* I, clang::Decl* D,
                                  CXScopeKind K = CXScope_Unexposed) {
  assert(I && "Invalid interpreter");
  return CXScope{K, D, static_cast<const void*>(I)};
}

size_t clang_qualtype_getSizeOfType(CXQualType type) {
  const clang::QualType QT = getType(type);
  if (const auto* TT = QT->getAs<clang::TagType>())
    return clang_scope_sizeOf(makeCXScope(getMeta(type), TT->getDecl()));

  // FIXME: Can we get the size of a non-tag type?
  const auto* I = getInterpreter(type);
  const auto TI = I->getSema().getASTContext().getTypeInfo(QT);
  const size_t TypeSize = TI.Width;
  return TypeSize / 8;
}

bool clang_qualtype_isTypeDerivedFrom(CXQualType derived, CXQualType base) {
  auto* I = getInterpreter(derived);
  const auto& SM = I->getSema().getSourceManager();
  const auto fakeLoc = SM.getLocForStartOfFile(SM.getMainFileID());
  const auto derivedType = getType(derived);
  const auto baseType = getType(base);

#ifdef USE_CLING
  cling::Interpreter::PushTransactionRAII RAII(I);
#endif
  return I->getSema().IsDerivedFrom(fakeLoc, derivedType, baseType);
}

static inline bool isNull(const CXScope& S) { return !S.data; }

static inline clang::Decl* getDecl(const CXScope& S) {
  return static_cast<clang::Decl*>(S.data);
}

static inline const CXInterpreterImpl* getMeta(const CXScope& S) {
  return static_cast<const CXInterpreterImpl*>(S.meta);
}

static inline CXScopeKind kind(const CXScope& S) { return S.kind; }

static inline compat::Interpreter* getInterpreter(const CXScope& S) {
  return getInterpreter(static_cast<const CXInterpreterImpl*>(S.meta));
}

void clang_scope_dump(CXScope S) { getDecl(S)->dump(); }

CXQualType clang_scope_getTypeFromScope(CXScope S) {
  if (isNull(S))
    return makeCXQualType(getMeta(S), clang::QualType(), CXType_Invalid);

  auto* D = getDecl(S);
  if (const auto* VD = llvm::dyn_cast<clang::ValueDecl>(D))
    return makeCXQualType(getMeta(S), VD->getType());

  const auto& Ctx = getInterpreter(S)->getCI()->getASTContext();
  return makeCXQualType(getMeta(S),
                        Ctx.getTypeDeclType(llvm::cast<clang::TypeDecl>(D)));
}

bool clang_scope_isAggregate(CXScope S) { return Cpp::IsAggregate(S.data); }

bool clang_scope_isNamespace(CXScope S) {
  return llvm::isa<clang::NamespaceDecl>(getDecl(S));
}

bool clang_scope_isClass(CXScope S) {
  return llvm::isa<clang::CXXRecordDecl>(getDecl(S));
}

bool clang_scope_isComplete(CXScope S) {
  if (isNull(S))
    return false;

  auto* D = getDecl(S);
  if (llvm::isa<clang::ClassTemplateSpecializationDecl>(D)) {
    const clang::QualType QT = getType(clang_scope_getTypeFromScope(S));
    const auto* CI = getInterpreter(S)->getCI();
    auto& Sema = CI->getSema();
    const auto& SM = Sema.getSourceManager();
    const clang::SourceLocation fakeLoc =
        SM.getLocForStartOfFile(SM.getMainFileID());
    return Sema.isCompleteType(fakeLoc, QT);
  }

  if (const auto* CXXRD = llvm::dyn_cast<clang::CXXRecordDecl>(D))
    return CXXRD->hasDefinition();

  if (const auto* TD = llvm::dyn_cast<clang::TagDecl>(D))
    return TD->getDefinition();

  // Everything else is considered complete.
  return true;
}

size_t clang_scope_sizeOf(CXScope S) {
  if (!clang_scope_isComplete(S))
    return 0;

  if (const auto* RD = llvm::dyn_cast<clang::RecordDecl>(getDecl(S))) {
    const clang::ASTContext& Context = RD->getASTContext();
    const clang::ASTRecordLayout& Layout = Context.getASTRecordLayout(RD);
    return Layout.getSize().getQuantity();
  }

  return 0;
}

bool clang_scope_isTemplate(CXScope S) {
  return llvm::isa_and_nonnull<clang::TemplateDecl>(getDecl(S));
}

bool clang_scope_isTemplateSpecialization(CXScope S) {
  return llvm::isa_and_nonnull<clang::ClassTemplateSpecializationDecl>(
      getDecl(S));
}

bool clang_scope_isTypedefed(CXScope S) {
  return llvm::isa_and_nonnull<clang::TypedefNameDecl>(getDecl(S));
}

bool clang_scope_isAbstract(CXScope S) {
  if (auto* CXXRD = llvm::dyn_cast_or_null<clang::CXXRecordDecl>(getDecl(S)))
    return CXXRD->isAbstract();
  return false;
}

bool clang_scope_isEnumScope(CXScope S) {
  return llvm::isa_and_nonnull<clang::EnumDecl>(getDecl(S));
}

bool clang_scope_isEnumConstant(CXScope S) {
  return llvm::isa_and_nonnull<clang::EnumConstantDecl>(getDecl(S));
}

CXStringSet* clang_scope_getEnums(CXScope S) {
  std::vector<std::string> EnumNames;
  Cpp::GetEnums(S.data, EnumNames);
  return makeCXStringSet(EnumNames);
}

CXQualType clang_scope_getIntegerTypeFromEnumScope(CXScope S) {
  if (const auto* ED = llvm::dyn_cast_or_null<clang::EnumDecl>(getDecl(S)))
    return makeCXQualType(getMeta(S), ED->getIntegerType());

  return makeCXQualType(getMeta(S), clang::QualType(), CXType_Invalid);
}

void clang_disposeScopeSet(CXScopeSet* set) {
  if (!set)
    return;

  delete[] set->Scopes; // NOLINT(*-owning-memory)
  delete set;           // NOLINT(*-owning-memory)
}

CXScopeSet* clang_scope_getEnumConstants(CXScope S) {
  const auto* ED = llvm::dyn_cast_or_null<clang::EnumDecl>(getDecl(S));
  if (!ED)
    return new CXScopeSet{nullptr, 0}; // NOLINT(*-owning-memory)

  auto EI = ED->enumerator_begin();
  auto EE = ED->enumerator_end();
  if (EI == EE)
    return new CXScopeSet{nullptr, 0}; // NOLINT(*-owning-memory)

  auto* Set = new CXScopeSet; // NOLINT(*-owning-memory)
  Set->Count = std::distance(EI, EE);
  Set->Scopes = new CXScope[Set->Count]; // NOLINT(*-owning-memory)
  for (auto I = EI; I != EE; ++I) {
    auto Idx = std::distance(EI, I);
    Set->Scopes[Idx] = makeCXScope(getMeta(S), *I, CXScope_EnumConstant);
  }

  return Set;
}

CXQualType clang_scope_getEnumConstantType(CXScope S) {
  if (isNull(S))
    return makeCXQualType(getMeta(S), clang::QualType(), CXType_Invalid);

  if (const auto* ECD =
          llvm::dyn_cast_or_null<clang::EnumConstantDecl>(getDecl(S)))
    return makeCXQualType(getMeta(S), ECD->getType());

  return makeCXQualType(getMeta(S), clang::QualType(), CXType_Invalid);
}

size_t clang_scope_getEnumConstantValue(CXScope S) {
  if (const auto* ECD =
          llvm::dyn_cast_or_null<clang::EnumConstantDecl>(getDecl(S))) {
    const llvm::APSInt& Val = ECD->getInitVal();
    return Val.getExtValue();
  }
  return 0;
}

bool clang_scope_isVariable(CXScope S) {
  return llvm::isa_and_nonnull<clang::VarDecl>(getDecl(S));
}

CXString clang_scope_getName(CXScope S) {
  auto* D = getDecl(S);

  if (llvm::isa_and_nonnull<clang::TranslationUnitDecl>(D))
    return makeCXString("");

  if (const auto* ND = llvm::dyn_cast_or_null<clang::NamedDecl>(D))
    return makeCXString(ND->getNameAsString());

  return makeCXString("<unnamed>");
}

CXString clang_scope_getCompleteName(CXScope S) {
  const auto& C = getInterpreter(S)->getSema().getASTContext();
  auto* D = getDecl(S);

  if (auto* ND = llvm::dyn_cast_or_null<clang::NamedDecl>(D)) {
    if (const auto* TD = llvm::dyn_cast<clang::TagDecl>(ND)) {
      std::string type_name;
      const clang::QualType QT = C.getTagDeclType(TD);
      clang::PrintingPolicy Policy = C.getPrintingPolicy();
      Policy.SuppressUnwrittenScope = true;
      Policy.SuppressScope = true;
      Policy.AnonymousTagLocations = false;
      QT.getAsStringInternal(type_name, Policy);

      return makeCXString(type_name);
    }

    return makeCXString(ND->getNameAsString());
  }

  if (llvm::isa_and_nonnull<clang::TranslationUnitDecl>(D))
    return makeCXString("");

  return makeCXString("<unnamed>");
}

CXString clang_scope_getQualifiedName(CXScope S) {
  auto* D = getDecl(S);

  if (llvm::isa_and_nonnull<clang::TranslationUnitDecl>(D))
    return makeCXString("");

  if (const auto* ND = llvm::dyn_cast_or_null<clang::NamedDecl>(D))
    return makeCXString(ND->getQualifiedNameAsString());

  return makeCXString("<unnamed>");
}

CXString clang_scope_getQualifiedCompleteName(CXScope S) {
  const auto& C = getInterpreter(S)->getSema().getASTContext();
  auto* D = getDecl(S);

  if (auto* ND = llvm::dyn_cast_or_null<clang::NamedDecl>(D)) {
    if (const auto* TD = llvm::dyn_cast<clang::TagDecl>(ND)) {
      std::string type_name;
      const clang::QualType QT = C.getTagDeclType(TD);
      QT.getAsStringInternal(type_name, C.getPrintingPolicy());

      return makeCXString(type_name);
    }

    return makeCXString(ND->getQualifiedNameAsString());
  }

  if (llvm::isa_and_nonnull<clang::TranslationUnitDecl>(D)) {
    return makeCXString("");
  }

  return makeCXString("<unnamed>");
}

CXScopeSet* clang_scope_getUsingNamespaces(CXScope S) {
  const auto* DC = llvm::dyn_cast_or_null<clang::DeclContext>(getDecl(S));
  if (!DC)
    return new CXScopeSet{nullptr, 0}; // NOLINT(*-owning-memory)

  auto DI = DC->using_directives().begin();
  auto DE = DC->using_directives().end();
  if (DI == DE)
    return new CXScopeSet{nullptr, 0}; // NOLINT(*-owning-memory)

  auto* Set = new CXScopeSet; // NOLINT(*-owning-memory)
  Set->Count = std::distance(DI, DE);
  Set->Scopes = new CXScope[Set->Count]; // NOLINT(*-owning-memory)
  for (auto I = DI; I != DE; ++I) {
    auto Idx = std::distance(DI, I);
    Set->Scopes[Idx] = makeCXScope(getMeta(S), (*I)->getNominatedNamespace(),
                                   CXScope_Namespace);
  }

  return Set;
}

CXScope clang_scope_getGlobalScope(CXInterpreter I) {
  const auto& C = getInterpreter(I)->getSema().getASTContext();
  auto* DC = C.getTranslationUnitDecl();
  return makeCXScope(I, DC, CXScope_Global);
}

CXScope clang_scope_getUnderlyingScope(CXScope S) {
  const auto* TND = llvm::dyn_cast_or_null<clang::TypedefNameDecl>(getDecl(S));
  if (!TND)
    return S;

  const clang::QualType QT = TND->getUnderlyingType();
  auto* D = Cpp::GetScopeFromType(QT.getAsOpaquePtr());
  if (!D)
    return S;

  return makeCXScope(getMeta(S), static_cast<clang::Decl*>(D));
}

CXScope clang_scope_getScope(const char* name, CXScope parent) {
  const auto S = clang_scope_getNamed(name, parent);
  if (kind(S) == CXScope_Invalid)
    return S;

  auto* ND = llvm::dyn_cast<clang::NamedDecl>(getDecl(S));
  if (llvm::isa<clang::NamespaceDecl>(ND) || llvm::isa<clang::RecordDecl>(ND) ||
      llvm::isa<clang::ClassTemplateDecl>(ND) ||
      llvm::isa<clang::TypedefNameDecl>(ND)) {
    return makeCXScope(getMeta(S), ND->getCanonicalDecl());
  }

  return S;
}

CXScope clang_scope_getNamed(const char* name, CXScope parent) {
  const clang::DeclContext* Within = nullptr;
  if (kind(parent) != CXScope_Invalid) {
    const auto US = clang_scope_getUnderlyingScope(parent);
    if (kind(US) != CXScope_Invalid)
      Within = llvm::dyn_cast<clang::DeclContext>(getDecl(US));
  }

  auto& sema = getInterpreter(parent)->getSema();
  auto* ND = Cpp::Cpp_utils::Lookup::Named(&sema, std::string(name), Within);
  if (ND && intptr_t(ND) != (intptr_t)-1) {
    return makeCXScope(getMeta(parent), ND->getCanonicalDecl());
  }

  return makeCXScope(getMeta(parent), nullptr, CXScope_Invalid);
}

CXScope clang_scope_getParentScope(CXScope parent) {
  auto* D = getDecl(parent);

  if (llvm::isa_and_nonnull<clang::TranslationUnitDecl>(D))
    return makeCXScope(getMeta(parent), nullptr, CXScope_Invalid);

  const auto* ParentDC = D->getDeclContext();
  if (!ParentDC)
    return makeCXScope(getMeta(parent), nullptr, CXScope_Invalid);

  auto* CD = clang::Decl::castFromDeclContext(ParentDC)->getCanonicalDecl();

  return makeCXScope(getMeta(parent), CD);
}

CXScope clang_scope_getScopeFromType(CXQualType type) {
  auto* D = Cpp::GetScopeFromType(type.data);
  return makeCXScope(getMeta(type), static_cast<clang::Decl*>(D));
}

size_t clang_scope_getNumBases(CXScope S) {
  auto* D = getDecl(S);

  if (const auto* CXXRD = llvm::dyn_cast_or_null<clang::CXXRecordDecl>(D)) {
    if (CXXRD->hasDefinition())
      return CXXRD->getNumBases();
  }

  return 0;
}

CXScope clang_scope_getBaseClass(CXScope S, size_t ibase) {
  auto* D = getDecl(S);
  auto* CXXRD = llvm::dyn_cast_or_null<clang::CXXRecordDecl>(D);
  if (!CXXRD || CXXRD->getNumBases() <= ibase)
    return makeCXScope(getMeta(S), nullptr, CXScope_Invalid);

  const auto type = (CXXRD->bases_begin() + ibase)->getType();
  if (const auto* RT = type->getAs<clang::RecordType>())
    return makeCXScope(getMeta(S), RT->getDecl());

  return makeCXScope(getMeta(S), nullptr, CXScope_Invalid);
}

bool clang_scope_isSubclass(CXScope derived, CXScope base) {
  if (isNull(derived) || isNull(base))
    return false;

  auto* D = getDecl(derived);
  auto* B = getDecl(base);

  if (D == B)
    return true;

  if (!llvm::isa<clang::CXXRecordDecl>(D) ||
      !llvm::isa<clang::CXXRecordDecl>(B))
    return false;

  return clang_qualtype_isTypeDerivedFrom(clang_scope_getTypeFromScope(derived),
                                          clang_scope_getTypeFromScope(base));
}

namespace Cpp {
unsigned ComputeBaseOffset(const clang::ASTContext& Context,
                           const clang::CXXRecordDecl* DerivedRD,
                           const clang::CXXBasePath& Path);
} // namespace Cpp

int64_t clang_scope_getBaseClassOffset(CXScope derived, CXScope base) {
  auto* D = getDecl(derived);
  auto* B = getDecl(base);

  if (D == B)
    return 0;

  assert(D || B);

  if (!llvm::isa<clang::CXXRecordDecl>(D) ||
      !llvm::isa<clang::CXXRecordDecl>(B))
    return -1;

  const auto* DCXXRD = llvm::cast<clang::CXXRecordDecl>(D);
  const auto* BCXXRD = llvm::cast<clang::CXXRecordDecl>(B);
  clang::CXXBasePaths Paths(/*FindAmbiguities=*/false, /*RecordPaths=*/true,
                            /*DetectVirtual=*/false);
  DCXXRD->isDerivedFrom(BCXXRD, Paths);

  const auto* I = getInterpreter(derived);
  return Cpp::ComputeBaseOffset(I->getSema().getASTContext(), DCXXRD,
                                Paths.front());
}

CXScopeSet* clang_scope_getClassMethods(CXScope S) {
  if (kind(S) == CXScope_Invalid)
    return new CXScopeSet{nullptr, 0}; // NOLINT(*-owning-memory)

  const auto US = clang_scope_getUnderlyingScope(S);
  if (kind(US) == CXScope_Invalid || !clang_scope_isClass(US))
    return new CXScopeSet{nullptr, 0}; // NOLINT(*-owning-memory)

  auto* CXXRD = llvm::dyn_cast<clang::CXXRecordDecl>(getDecl(US));

  auto* I = getInterpreter(S);
#ifdef USE_CLING
  cling::Interpreter::PushTransactionRAII RAII(I);
#endif // USE_CLING
  I->getSema().ForceDeclarationOfImplicitMembers(CXXRD);
  llvm::SmallVector<clang::CXXMethodDecl*> Methods;
  for (clang::Decl* DI : CXXRD->decls()) {
    if (auto* MD = llvm::dyn_cast<clang::CXXMethodDecl>(DI)) {
      Methods.push_back(MD);
    } else if (auto* USD = llvm::dyn_cast<clang::UsingShadowDecl>(DI)) {
      if (auto* TMD =
              llvm::dyn_cast<clang::CXXMethodDecl>(USD->getTargetDecl()))
        Methods.push_back(TMD);
    }
  }

  auto* Set = new CXScopeSet; // NOLINT(*-owning-memory)
  Set->Count = Methods.size();
  Set->Scopes = new CXScope[Set->Count]; // NOLINT(*-owning-memory)
  for (auto En : llvm::enumerate(Methods)) {
    auto Idx = En.index();
    auto* Val = En.value();
    Set->Scopes[Idx] = makeCXScope(getMeta(S), Val, CXScope_Function);
  }

  return Set;
}

CXScopeSet* clang_scope_getFunctionTemplatedDecls(CXScope S) {
  if (kind(S) == CXScope_Invalid)
    return new CXScopeSet{nullptr, 0}; // NOLINT(*-owning-memory)

  const auto US = clang_scope_getUnderlyingScope(S);
  if (kind(US) == CXScope_Invalid || !clang_scope_isClass(US))
    return new CXScopeSet{nullptr, 0}; // NOLINT(*-owning-memory)

  auto* CXXRD = llvm::dyn_cast<clang::CXXRecordDecl>(getDecl(US));

  auto* I = getInterpreter(S);
#ifdef USE_CLING
  cling::Interpreter::PushTransactionRAII RAII(I);
#endif // USE_CLING
  I->getSema().ForceDeclarationOfImplicitMembers(CXXRD);
  llvm::SmallVector<clang::FunctionTemplateDecl*> Methods;
  for (clang::Decl* DI : CXXRD->decls()) {
    if (auto* MD = llvm::dyn_cast<clang::FunctionTemplateDecl>(DI)) {
      Methods.push_back(MD);
    } else if (const auto* USD = llvm::dyn_cast<clang::UsingShadowDecl>(DI)) {
      if (auto* TMD =
              llvm::dyn_cast<clang::FunctionTemplateDecl>(USD->getTargetDecl()))
        Methods.push_back(TMD);
    }
  }

  auto* Set = new CXScopeSet; // NOLINT(*-owning-memory)
  Set->Count = Methods.size();
  Set->Scopes = new CXScope[Set->Count]; // NOLINT(*-owning-memory)
  for (auto En : llvm::enumerate(Methods)) {
    auto Idx = En.index();
    auto* Val = En.value();
    Set->Scopes[Idx] = makeCXScope(getMeta(S), Val, CXScope_Function);
  }

  return Set;
}

bool clang_scope_hasDefaultConstructor(CXScope S) {
  auto* D = getDecl(S);

  if (const auto* CXXRD = llvm::dyn_cast_or_null<clang::CXXRecordDecl>(D))
    return CXXRD->hasDefaultConstructor();

  return false;
}

CXScope clang_scope_getDefaultConstructor(CXScope S) {
  if (!clang_scope_hasDefaultConstructor(S))
    return makeCXScope(getMeta(S), nullptr, CXScope_Invalid);

  auto* CXXRD = llvm::dyn_cast_or_null<clang::CXXRecordDecl>(getDecl(S));
  if (!CXXRD)
    return makeCXScope(getMeta(S), nullptr, CXScope_Invalid);

  auto* Res = getInterpreter(S)->getSema().LookupDefaultConstructor(CXXRD);
  return makeCXScope(getMeta(S), Res, CXScope_Function);
}

CXScope clang_scope_getDestructor(CXScope S) {
  auto* D = getDecl(S);

  if (auto* CXXRD = llvm::dyn_cast_or_null<clang::CXXRecordDecl>(D)) {
    getInterpreter(S)->getSema().ForceDeclarationOfImplicitMembers(CXXRD);
    return makeCXScope(getMeta(S), CXXRD->getDestructor(), CXScope_Function);
  }

  return makeCXScope(getMeta(S), nullptr, CXScope_Invalid);
}

CXScopeSet* clang_scope_getFunctionsUsingName(CXScope S, const char* name) {
  const auto* D = getDecl(clang_scope_getUnderlyingScope(S));

  if (!D || !name)
    return new CXScopeSet{nullptr, 0}; // NOLINT(*-owning-memory)

  const llvm::StringRef Name(name);
  const auto* I = getInterpreter(S);
  auto& SM = I->getSema();
  const clang::DeclarationName DName = &SM.getASTContext().Idents.get(Name);
  clang::LookupResult R(SM, DName, clang::SourceLocation(),
                        clang::Sema::LookupOrdinaryName,
                        clang::Sema::ForVisibleRedeclaration);

  Cpp::Cpp_utils::Lookup::Named(&SM, R, clang::Decl::castToDeclContext(D));

  if (R.empty())
    return new CXScopeSet{nullptr, 0}; // NOLINT(*-owning-memory)

  R.resolveKind();

  llvm::SmallVector<clang::Decl*> Funcs;
  for (auto* Found : R)
    if (llvm::isa<clang::FunctionDecl>(Found))
      Funcs.push_back(Found);

  auto* Set = new CXScopeSet; // NOLINT(*-owning-memory)
  Set->Count = Funcs.size();
  Set->Scopes = new CXScope[Set->Count]; // NOLINT(*-owning-memory)
  for (auto En : llvm::enumerate(Funcs)) {
    auto Idx = En.index();
    auto* Val = En.value();
    Set->Scopes[Idx] = makeCXScope(getMeta(S), Val, CXScope_Function);
  }

  return Set;
}

CXQualType clang_scope_getFunctionReturnType(CXScope func) {
  auto* D = getDecl(func);
  if (const auto* FD = llvm::dyn_cast_or_null<clang::FunctionDecl>(D))
    return makeCXQualType(getMeta(func), FD->getReturnType());

  if (const auto* FD = llvm::dyn_cast_or_null<clang::FunctionTemplateDecl>(D)) {
    const auto* FTD = FD->getTemplatedDecl();
    return makeCXQualType(getMeta(func), FTD->getReturnType());
  }

  return makeCXQualType(getMeta(func), clang::QualType(), CXType_Invalid);
}

size_t clang_scope_getFunctionNumArgs(CXScope func) {
  auto* D = getDecl(func);
  if (const auto* FD = llvm::dyn_cast_or_null<clang::FunctionDecl>(D))
    return FD->getNumParams();

  if (const auto* FD = llvm::dyn_cast_or_null<clang::FunctionTemplateDecl>(D))
    return FD->getTemplatedDecl()->getNumParams();

  return 0;
}

size_t clang_scope_getFunctionRequiredArgs(CXScope func) {
  auto* D = getDecl(func);
  if (const auto* FD = llvm::dyn_cast_or_null<clang::FunctionDecl>(D))
    return FD->getMinRequiredArguments();

  if (const auto* FD = llvm::dyn_cast_or_null<clang::FunctionTemplateDecl>(D))
    return FD->getTemplatedDecl()->getMinRequiredArguments();

  return 0;
}

CXQualType clang_scope_getFunctionArgType(CXScope func, size_t iarg) {
  auto* D = getDecl(func);

  if (auto* FD = llvm::dyn_cast_or_null<clang::FunctionDecl>(D)) {
    if (iarg < FD->getNumParams()) {
      const auto* PVD = FD->getParamDecl(iarg);
      return makeCXQualType(getMeta(func), PVD->getOriginalType());
    }
  }

  return makeCXQualType(getMeta(func), clang::QualType(), CXType_Invalid);
}

CXString clang_scope_getFunctionSignature(CXScope func) {
  if (isNull(func))
    return makeCXString("");

  auto* D = getDecl(func);
  if (const auto* FD = llvm::dyn_cast<clang::FunctionDecl>(D)) {
    std::string Signature;
    llvm::raw_string_ostream SS(Signature);
    const auto& C = getInterpreter(func)->getSema().getASTContext();
    clang::PrintingPolicy Policy = C.getPrintingPolicy();
    // Skip printing the body
    Policy.TerseOutput = true;
    Policy.FullyQualifiedName = true;
    Policy.SuppressDefaultTemplateArgs = false;
    FD->print(SS, Policy);
    SS.flush();
    return makeCXString(Signature);
  }

  return makeCXString("");
}

bool clang_scope_isFunctionDeleted(CXScope func) {
  const auto* FD = llvm::cast<clang::FunctionDecl>(getDecl(func));
  return FD->isDeleted();
}

bool clang_scope_isTemplatedFunction(CXScope func) {
  auto* D = getDecl(func);
  if (llvm::isa_and_nonnull<clang::FunctionTemplateDecl>(D))
    return true;

  if (const auto* FD = llvm::dyn_cast_or_null<clang::FunctionDecl>(D)) {
    const auto TK = FD->getTemplatedKind();
    return TK == clang::FunctionDecl::TemplatedKind::
                     TK_FunctionTemplateSpecialization ||
           TK == clang::FunctionDecl::TemplatedKind::
                     TK_DependentFunctionTemplateSpecialization ||
           TK == clang::FunctionDecl::TemplatedKind::TK_FunctionTemplate;
  }

  return false;
}

bool clang_scope_existsFunctionTemplate(const char* name, CXScope parent) {
  if (kind(parent) == CXScope_Invalid || !name)
    return false;

  const auto* Within = llvm::dyn_cast<clang::DeclContext>(getDecl(parent));

  auto& S = getInterpreter(parent)->getSema();
  auto* ND = Cpp::Cpp_utils::Lookup::Named(&S, name, Within);

  if (!ND)
    return false;

  if (intptr_t(ND) != (intptr_t)-1)
    return clang_scope_isTemplatedFunction(makeCXScope(getMeta(parent), ND));

  // FIXME: Cycle through the Decls and check if there is a templated function
  return true;
}

CXScopeSet* clang_scope_getClassTemplatedMethods(const char* name,
                                                 CXScope parent) {
  const auto* D = getDecl(clang_scope_getUnderlyingScope(parent));

  if (!D || !name)
    return new CXScopeSet{nullptr, 0}; // NOLINT(*-owning-memory)

  const llvm::StringRef Name(name);
  const auto* I = getInterpreter(parent);
  auto& SM = I->getSema();
  const clang::DeclarationName DName = &SM.getASTContext().Idents.get(Name);
  clang::LookupResult R(SM, DName, clang::SourceLocation(),
                        clang::Sema::LookupOrdinaryName,
                        clang::Sema::ForVisibleRedeclaration);

  Cpp::Cpp_utils::Lookup::Named(&SM, R, clang::Decl::castToDeclContext(D));

  if (R.empty())
    return new CXScopeSet{nullptr, 0}; // NOLINT(*-owning-memory)

  R.resolveKind();

  llvm::SmallVector<clang::Decl*> Funcs;
  for (auto* Found : R)
    if (llvm::isa<clang::FunctionTemplateDecl>(Found))
      Funcs.push_back(Found);

  auto* Set = new CXScopeSet; // NOLINT(*-owning-memory)
  Set->Count = Funcs.size();
  Set->Scopes = new CXScope[Set->Count]; // NOLINT(*-owning-memory)
  for (auto En : llvm::enumerate(Funcs)) {
    auto Idx = En.index();
    auto* Val = En.value();
    Set->Scopes[Idx] = makeCXScope(getMeta(parent), Val, CXScope_Function);
  }

  return Set;
}

bool clang_scope_isMethod(CXScope method) {
  auto* D = getDecl(method);
  return llvm::dyn_cast_or_null<clang::CXXMethodDecl>(D);
}

bool clang_scope_isPublicMethod(CXScope method) {
  auto* D = getDecl(method);
  if (const auto* CXXMD = llvm::dyn_cast_or_null<clang::CXXMethodDecl>(D))
    return CXXMD->getAccess() == clang::AccessSpecifier::AS_public;

  return false;
}

bool clang_scope_isProtectedMethod(CXScope method) {
  auto* D = getDecl(method);
  if (const auto* CXXMD = llvm::dyn_cast_or_null<clang::CXXMethodDecl>(D))
    return CXXMD->getAccess() == clang::AccessSpecifier::AS_protected;

  return false;
}

bool clang_scope_isPrivateMethod(CXScope method) {
  auto* D = getDecl(method);
  if (const auto* CXXMD = llvm::dyn_cast_or_null<clang::CXXMethodDecl>(D))
    return CXXMD->getAccess() == clang::AccessSpecifier::AS_private;

  return false;
}

bool clang_scope_isConstructor(CXScope method) {
  auto* D = getDecl(method);
  return llvm::isa_and_nonnull<clang::CXXConstructorDecl>(D);
}

bool clang_scope_isDestructor(CXScope method) {
  auto* D = getDecl(method);
  return llvm::isa_and_nonnull<clang::CXXDestructorDecl>(D);
}

bool clang_scope_isStaticMethod(CXScope method) {
  auto* D = getDecl(method);
  if (const auto* CXXMD = llvm::dyn_cast_or_null<clang::CXXMethodDecl>(D))
    return CXXMD->isStatic();

  return false;
}

CXFuncAddr clang_scope_getFunctionAddress(CXScope method) {
  auto* D = getDecl(method);
  auto* I = getInterpreter(method);

  const auto get_mangled_name = [I](clang::FunctionDecl* FD) {
    auto* MangleCtx = I->getSema().getASTContext().createMangleContext();

    if (!MangleCtx->shouldMangleDeclName(FD)) {
      return FD->getNameInfo().getName().getAsString();
    }

    std::string mangled_name;
    llvm::raw_string_ostream ostream(mangled_name);

    MangleCtx->mangleName(FD, ostream);

    ostream.flush();
    delete MangleCtx; // NOLINT(*-owning-memory)

    return mangled_name;
  };

  if (auto* FD = llvm::dyn_cast_or_null<clang::FunctionDecl>(D)) {
    auto FDAorErr = compat::getSymbolAddress(*I, get_mangled_name(FD));
    if (llvm::Error Err = FDAorErr.takeError())
      llvm::consumeError(std::move(Err)); // nullptr if missing
    else
      return llvm::jitTargetAddressToPointer<void*>(*FDAorErr);
  }

  return nullptr;
}

bool clang_scope_isVirtualMethod(CXScope method) {
  auto* D = getDecl(method);
  if (const auto* CXXMD = llvm::dyn_cast_or_null<clang::CXXMethodDecl>(D))
    return CXXMD->isVirtual();

  return false;
}

bool clang_scope_isConstMethod(CXScope method) {
  auto* D = getDecl(method);
  if (const auto* CXXMD = llvm::dyn_cast_or_null<clang::CXXMethodDecl>(D))
    return CXXMD->getMethodQualifiers().hasConst();

  return false;
}

CXString clang_scope_getFunctionArgDefault(CXScope func, size_t param_index) {
  return makeCXString(Cpp::GetFunctionArgDefault(func.data, param_index));
}

CXString clang_scope_getFunctionArgName(CXScope func, size_t param_index) {
  return makeCXString(Cpp::GetFunctionArgName(func.data, param_index));
}

CXScopeSet* clang_scope_getDatamembers(CXScope S) {
  const auto* CXXRD = llvm::dyn_cast_or_null<clang::CXXRecordDecl>(getDecl(S));
  if (!CXXRD)
    return new CXScopeSet{nullptr, 0}; // NOLINT(*-owning-memory)

  auto FI = CXXRD->field_begin();
  auto FE = CXXRD->field_end();
  if (FI == FE)
    return new CXScopeSet{nullptr, 0}; // NOLINT(*-owning-memory)

  auto* Set = new CXScopeSet; // NOLINT(*-owning-memory)
  Set->Count = std::distance(FI, FE);
  Set->Scopes = new CXScope[Set->Count]; // NOLINT(*-owning-memory)
  for (auto I = FE; I != FE; ++I) {
    auto Idx = std::distance(FI, I);
    Set->Scopes[Idx] = makeCXScope(getMeta(S), *I, CXScope_Field);
  }

  return Set;
}

CXScope clang_scope_lookupDatamember(const char* name, CXScope parent) {
  if (kind(parent) == CXScope_Invalid || !name)
    return makeCXScope(getMeta(parent), nullptr, CXScope_Invalid);

  const auto* Within = llvm::dyn_cast<clang::DeclContext>(getDecl(parent));

  auto& S = getInterpreter(parent)->getSema();
  auto* ND = Cpp::Cpp_utils::Lookup::Named(&S, name, Within);
  if (ND && intptr_t(ND) != (intptr_t)-1) {
    if (llvm::isa_and_nonnull<clang::FieldDecl>(ND)) {
      return makeCXScope(getMeta(parent), ND, CXScope_Field);
    }
  }

  return makeCXScope(getMeta(parent), nullptr, CXScope_Invalid);
}

namespace Cpp {
clang::Decl*
InstantiateTemplate(clang::TemplateDecl* TemplateD,
                    llvm::ArrayRef<clang::TemplateArgument> TemplateArgs,
                    clang::Sema& S);
} // namespace Cpp

CXScope clang_scope_instantiateTemplate(CXScope tmpl,
                                        CXTemplateArgInfo* template_args,
                                        size_t template_args_size) {
  auto* I = getInterpreter(tmpl);
  auto& S = I->getSema();
  auto& C = S.getASTContext();

  llvm::SmallVector<clang::TemplateArgument> TemplateArgs;
  TemplateArgs.reserve(template_args_size);
  for (size_t i = 0; i < template_args_size; ++i) {
    const clang::QualType ArgTy =
        clang::QualType::getFromOpaquePtr(template_args[i].Type);
    if (template_args[i].IntegralValue) {
      // We have a non-type template parameter. Create an integral value from
      // the string representation.
      auto Res = llvm::APSInt(template_args[i].IntegralValue);
      Res = Res.extOrTrunc(C.getIntWidth(ArgTy));
      TemplateArgs.push_back(clang::TemplateArgument(C, Res, ArgTy));
    } else {
      TemplateArgs.push_back(ArgTy);
    }
  }

  auto* TmplD = llvm::dyn_cast<clang::TemplateDecl>(getDecl(tmpl));

  // We will create a new decl, push a transaction.
#ifdef USE_CLING
  cling::Interpreter::PushTransactionRAII RAII(I);
#endif
  return makeCXScope(getMeta(tmpl),
                     Cpp::InstantiateTemplate(TmplD, TemplateArgs, S));
}

CXQualType clang_scope_getVariableType(CXScope var) {
  auto* D = getDecl(var);

  if (const auto* DD = llvm::dyn_cast_or_null<clang::DeclaratorDecl>(D))
    return makeCXQualType(getMeta(var), DD->getType());

  return makeCXQualType(getMeta(var), clang::QualType(), CXType_Invalid);
}

namespace Cpp {
intptr_t GetVariableOffsetImpl(compat::Interpreter& I, clang::Decl* D);
} // namespace Cpp

intptr_t clang_scope_getVariableOffset(CXScope var) {
  return Cpp::GetVariableOffsetImpl(*getInterpreter(var), getDecl(var));
}

bool clang_scope_isPublicVariable(CXScope var) {
  auto* D = getDecl(var);
  if (const auto* CXXMD = llvm::dyn_cast_or_null<clang::DeclaratorDecl>(D))
    return CXXMD->getAccess() == clang::AccessSpecifier::AS_public;

  return false;
}

bool clang_scope_isProtectedVariable(CXScope var) {
  auto* D = getDecl(var);
  if (const auto* CXXMD = llvm::dyn_cast_or_null<clang::DeclaratorDecl>(D))
    return CXXMD->getAccess() == clang::AccessSpecifier::AS_protected;

  return false;
}

bool clang_scope_isPrivateVariable(CXScope var) {
  auto* D = getDecl(var);
  if (const auto* CXXMD = llvm::dyn_cast_or_null<clang::DeclaratorDecl>(D))
    return CXXMD->getAccess() == clang::AccessSpecifier::AS_private;

  return false;
}

bool clang_scope_isStaticVariable(CXScope var) {
  auto* D = getDecl(var);
  return llvm::isa_and_nonnull<clang::VarDecl>(D);
}

bool clang_scope_isConstVariable(CXScope var) {
  auto* D = getDecl(var);
  if (const auto* VD = llvm::dyn_cast_or_null<clang::ValueDecl>(D)) {
    return VD->getType().isConstQualified();
  }

  return false;
}

CXObject clang_allocate(CXScope S) {
  return ::operator new(clang_scope_sizeOf(S));
}

void clang_deallocate(CXObject address) { ::operator delete(address); }

namespace Cpp {
JitCall MakeFunctionCallableImpl(TInterp_t I, TCppConstFunction_t func);
} // namespace Cpp

CXObject clang_construct(CXScope scope, void* arena) {
  if (!clang_scope_hasDefaultConstructor(scope))
    return nullptr;

  const auto Ctor = clang_scope_getDefaultConstructor(scope);
  if (kind(Ctor) == CXScope_Invalid)
    return nullptr;

  auto* I = getInterpreter(scope);
  if (const Cpp::JitCall JC = Cpp::MakeFunctionCallableImpl(I, getDecl(Ctor))) {
    if (arena) {
      JC.Invoke(arena, {}, (void*)~0); // Tell Invoke to use placement new.
      return arena;
    }

    void* obj = nullptr;
    JC.Invoke(obj);
    return obj;
  }

  return nullptr;
}

namespace Cpp {
void DestructImpl(compat::Interpreter& interp, TCppObject_t This,
                  clang::Decl* Class, bool withFree);
} // namespace Cpp

void clang_destruct(CXObject This, CXScope S, bool withFree) {
  Cpp::DestructImpl(*getInterpreter(S), This, getDecl(S), withFree);
}

CXJitCallKind clang_jitcall_getKind(CXJitCall J) {
  const auto* call = reinterpret_cast<Cpp::JitCall*>(J); // NOLINT(*-cast)
  return static_cast<CXJitCallKind>(call->getKind());
}

bool clang_jitcall_isValid(CXJitCall J) {
  return reinterpret_cast<Cpp::JitCall*>(J)->isValid(); // NOLINT(*-cast)
}

void clang_jitcall_invoke(CXJitCall J, void* result, CXJitCallArgList args,
                          void* self) {
  const auto* call = reinterpret_cast<Cpp::JitCall*>(J); // NOLINT(*-cast)
  call->Invoke(result, {args.data, args.numArgs}, self);
}

CXJitCall clang_jitcall_makeFunctionCallable(CXScope func) {
  auto J = Cpp::MakeFunctionCallableImpl(getInterpreter(func), getDecl(func));
  auto Ptr = std::make_unique<Cpp::JitCall>(J);
  return reinterpret_cast<CXJitCall>(Ptr.release()); // NOLINT(*-cast)
}

void clang_jitcall_dispose(CXJitCall J) {
  delete reinterpret_cast<Cpp::JitCall*>(J); // NOLINT(*-owning-memory, *-cast)
}