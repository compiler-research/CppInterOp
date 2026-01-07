//--------------------------------------------------------------------*- C++ -*-
// CppInterOp Dispatch Mechanism
// author:  Aaron Jomy <aaron.jomy@cern.ch>
//===----------------------------------------------------------------------===//
//
// Defines the mechanism which enables dispatching of the CppInterOp API
// without linking to it, preventing any LLVM or Clang symbols from being leaked
// into the client application.
//
//===----------------------------------------------------------------------===//
#ifndef CPPINTEROP_DISPATCH_H
#define CPPINTEROP_DISPATCH_H

#include <cstdlib>
#include <dlfcn.h>
#include <iostream>
#include <mutex>
#include <string>

#ifdef CPPINTEROP_CPPINTEROP_H
#error "To use the Dispatch mechanism, do not include CppInterOp.h"
#endif

#include <CppInterOp/CppInterOp.h>

using __CPP_FUNC = void (*)();

///\param[in] procname - the name of the FunctionEntry in the symbol lookup
/// table.
///
///\returns the function address of the requested API, or nullptr if not found
extern "C" CPPINTEROP_API void (
    *CppGetProcAddress(const unsigned char* procname))(void);

// macro that allows declaration and loading of all CppInterOp API functions in
// a consistent way. This is used as our dispatched API list, along with the
// name-address pair table
#define CPPINTEROP_API_MAP                                                     \
  X(CreateInterpreter, decltype(&CppImpl::CreateInterpreter))                  \
  X(GetInterpreter, decltype(&CppImpl::GetInterpreter))                        \
  X(Process, decltype(&CppImpl::Process))                                      \
  X(GetResourceDir, decltype(&CppImpl::GetResourceDir))                        \
  X(AddIncludePath, decltype(&CppImpl::AddIncludePath))                        \
  X(LoadLibrary, decltype(&CppImpl::LoadLibrary))                              \
  X(Declare, decltype(&CppImpl::Declare))                                      \
  X(DeleteInterpreter, decltype(&CppImpl::DeleteInterpreter))                  \
  X(IsNamespace, decltype(&CppImpl::IsNamespace))                              \
  X(ObjToString, decltype(&CppImpl::ObjToString))                              \
  X(GetQualifiedCompleteName, decltype(&CppImpl::GetQualifiedCompleteName))    \
  X(GetValueKind, decltype(&CppImpl::GetValueKind))                            \
  X(GetNonReferenceType, decltype(&CppImpl::GetNonReferenceType))              \
  X(IsEnumType, decltype(&CppImpl::IsEnumType))                                \
  X(GetIntegerTypeFromEnumType,                                                \
    decltype(&CppImpl::GetIntegerTypeFromEnumType))                            \
  X(GetReferencedType, decltype(&CppImpl::GetReferencedType))                  \
  X(IsPointerType, decltype(&CppImpl::IsPointerType))                          \
  X(GetPointeeType, decltype(&CppImpl::GetPointeeType))                        \
  X(GetPointerType, decltype(&CppImpl::GetPointerType))                        \
  X(IsReferenceType, decltype(&CppImpl::IsReferenceType))                      \
  X(GetTypeAsString, decltype(&CppImpl::GetTypeAsString))                      \
  X(GetCanonicalType, decltype(&CppImpl::GetCanonicalType))                    \
  X(HasTypeQualifier, decltype(&CppImpl::HasTypeQualifier))                    \
  X(RemoveTypeQualifier, decltype(&CppImpl::RemoveTypeQualifier))              \
  X(GetUnderlyingType, decltype(&CppImpl::GetUnderlyingType))                  \
  X(IsRecordType, decltype(&CppImpl::IsRecordType))                            \
  X(IsFunctionPointerType, decltype(&CppImpl::IsFunctionPointerType))          \
  X(GetVariableType, decltype(&CppImpl::GetVariableType))                      \
  X(GetNamed, decltype(&CppImpl::GetNamed))                                    \
  X(GetScopeFromType, decltype(&CppImpl::GetScopeFromType))                    \
  X(GetClassTemplateInstantiationArgs,                                         \
    decltype(&CppImpl::GetClassTemplateInstantiationArgs))                     \
  X(IsClass, decltype(&CppImpl::IsClass))                                      \
  X(GetType, decltype(&CppImpl::GetType))                                      \
  X(GetTypeFromScope, decltype(&CppImpl::GetTypeFromScope))                    \
  X(GetComplexType, decltype(&CppImpl::GetComplexType))                        \
  X(GetIntegerTypeFromEnumScope,                                               \
    decltype(&CppImpl::GetIntegerTypeFromEnumScope))                           \
  X(GetUnderlyingScope, decltype(&CppImpl::GetUnderlyingScope))                \
  X(GetScope, decltype(&CppImpl::GetScope))                                    \
  X(GetGlobalScope, decltype(&CppImpl::GetGlobalScope))                        \
  X(GetScopeFromCompleteName, decltype(&CppImpl::GetScopeFromCompleteName))    \
  X(InstantiateTemplate, decltype(&CppImpl::InstantiateTemplate))              \
  X(GetParentScope, decltype(&CppImpl::GetParentScope))                        \
  X(IsTemplate, decltype(&CppImpl::IsTemplate))                                \
  X(IsTemplateSpecialization, decltype(&CppImpl::IsTemplateSpecialization))    \
  X(IsTypedefed, decltype(&CppImpl::IsTypedefed))                              \
  X(IsClassPolymorphic, decltype(&CppImpl::IsClassPolymorphic))                \
  X(Demangle, decltype(&CppImpl::Demangle))                                    \
  X(SizeOf, decltype(&CppImpl::SizeOf))                                        \
  X(GetSizeOfType, decltype(&CppImpl::GetSizeOfType))                          \
  X(IsBuiltin, decltype(&CppImpl::IsBuiltin))                                  \
  X(IsComplete, decltype(&CppImpl::IsComplete))                                \
  X(Allocate, decltype(&CppImpl::Allocate))                                    \
  X(Deallocate, decltype(&CppImpl::Deallocate))                                \
  X(Construct, decltype(&CppImpl::Construct))                                  \
  X(Destruct, decltype(&CppImpl::Destruct))                                    \
  X(IsAbstract, decltype(&CppImpl::IsAbstract))                                \
  X(IsEnumScope, decltype(&CppImpl::IsEnumScope))                              \
  X(IsEnumConstant, decltype(&CppImpl::IsEnumConstant))                        \
  X(IsAggregate, decltype(&CppImpl::IsAggregate))                              \
  X(HasDefaultConstructor, decltype(&CppImpl::HasDefaultConstructor))          \
  X(IsVariable, decltype(&CppImpl::IsVariable))                                \
  X(GetAllCppNames, decltype(&CppImpl::GetAllCppNames))                        \
  X(GetUsingNamespaces, decltype(&CppImpl::GetUsingNamespaces))                \
  X(GetCompleteName, decltype(&CppImpl::GetCompleteName))                      \
  X(GetDestructor, decltype(&CppImpl::GetDestructor))                          \
  X(IsVirtualMethod, decltype(&CppImpl::IsVirtualMethod))                      \
  X(GetNumBases, decltype(&CppImpl::GetNumBases))                              \
  X(GetName, decltype(&CppImpl::GetName))                                      \
  X(GetBaseClass, decltype(&CppImpl::GetBaseClass))                            \
  X(IsSubclass, decltype(&CppImpl::IsSubclass))                                \
  X(GetOperator, decltype(&CppImpl::GetOperator))                              \
  X(GetFunctionReturnType, decltype(&CppImpl::GetFunctionReturnType))          \
  X(GetBaseClassOffset, decltype(&CppImpl::GetBaseClassOffset))                \
  X(GetClassMethods, decltype(&CppImpl::GetClassMethods))                      \
  X(GetFunctionsUsingName, decltype(&CppImpl::GetFunctionsUsingName))          \
  X(GetFunctionNumArgs, decltype(&CppImpl::GetFunctionNumArgs))                \
  X(GetFunctionRequiredArgs, decltype(&CppImpl::GetFunctionRequiredArgs))      \
  X(GetFunctionArgName, decltype(&CppImpl::GetFunctionArgName))                \
  X(GetFunctionArgType, decltype(&CppImpl::GetFunctionArgType))                \
  X(GetFunctionArgDefault, decltype(&CppImpl::GetFunctionArgDefault))          \
  X(IsConstMethod, decltype(&CppImpl::IsConstMethod))                          \
  X(GetFunctionTemplatedDecls, decltype(&CppImpl::GetFunctionTemplatedDecls))  \
  X(ExistsFunctionTemplate, decltype(&CppImpl::ExistsFunctionTemplate))        \
  X(IsTemplatedFunction, decltype(&CppImpl::IsTemplatedFunction))              \
  X(IsStaticMethod, decltype(&CppImpl::IsStaticMethod))                        \
  X(GetClassTemplatedMethods, decltype(&CppImpl::GetClassTemplatedMethods))    \
  X(BestOverloadFunctionMatch, decltype(&CppImpl::BestOverloadFunctionMatch))  \
  X(GetOperatorFromSpelling, decltype(&CppImpl::GetOperatorFromSpelling))      \
  X(IsFunctionDeleted, decltype(&CppImpl::IsFunctionDeleted))                  \
  X(IsPublicMethod, decltype(&CppImpl::IsPublicMethod))                        \
  X(IsProtectedMethod, decltype(&CppImpl::IsProtectedMethod))                  \
  X(IsPrivateMethod, decltype(&CppImpl::IsPrivateMethod))                      \
  X(IsConstructor, decltype(&CppImpl::IsConstructor))                          \
  X(IsDestructor, decltype(&CppImpl::IsDestructor))                            \
  X(GetDatamembers, decltype(&CppImpl::GetDatamembers))                        \
  X(GetStaticDatamembers, decltype(&CppImpl::GetStaticDatamembers))            \
  X(GetEnumConstantDatamembers,                                                \
    decltype(&CppImpl::GetEnumConstantDatamembers))                            \
  X(LookupDatamember, decltype(&CppImpl::LookupDatamember))                    \
  X(IsLambdaClass, decltype(&CppImpl::IsLambdaClass))                          \
  X(GetQualifiedName, decltype(&CppImpl::GetQualifiedName))                    \
  X(GetVariableOffset, decltype(&CppImpl::GetVariableOffset))                  \
  X(IsPublicVariable, decltype(&CppImpl::IsPublicVariable))                    \
  X(IsProtectedVariable, decltype(&CppImpl::IsProtectedVariable))              \
  X(IsPrivateVariable, decltype(&CppImpl::IsPrivateVariable))                  \
  X(IsStaticVariable, decltype(&CppImpl::IsStaticVariable))                    \
  X(IsConstVariable, decltype(&CppImpl::IsConstVariable))                      \
  X(GetDimensions, decltype(&CppImpl::GetDimensions))                          \
  X(GetEnumConstants, decltype(&CppImpl::GetEnumConstants))                    \
  X(GetEnumConstantType, decltype(&CppImpl::GetEnumConstantType))              \
  X(GetEnumConstantValue, decltype(&CppImpl::GetEnumConstantValue))            \
  X(DumpScope, decltype(&CppImpl::DumpScope))                                  \
  X(AddSearchPath, decltype(&CppImpl::AddSearchPath))                          \
  X(Evaluate, decltype(&CppImpl::Evaluate))                                    \
  X(IsDebugOutputEnabled, decltype(&CppImpl::IsDebugOutputEnabled))            \
  X(EnableDebugOutput, decltype(&CppImpl::EnableDebugOutput))                  \
  X(MakeFunctionCallable, CppImpl::JitCall (*)(CppImpl::TCppConstFunction_t))  \
  X(GetFunctionAddress, CppImpl::TCppFuncAddr_t (*)(CppImpl::TCppFunction_t))  \
  /*X(API_name, fnptr_ty)*/

// TODO: implement overload that takes an existing opened DL handle
inline void* dlGetProcAddress(const char* name,
                              const char* customLibPath = nullptr) {
  if (!name)
    return nullptr;

  static std::once_flag loaded;
  static void* handle = nullptr;
  static void* (*getCppProcAddress)(const char*) = nullptr;

  std::call_once(loaded, [customLibPath]() {
    // priority order: 1) custom path argument, or CPPINTEROP_LIBRARY_PATH via
    // 2) cmake configured path 3) env vars
    const char* libPath = customLibPath;
    if (!libPath) {
      libPath = std::getenv("CPPINTEROP_LIBRARY_PATH");
    }

    handle = dlopen(libPath, RTLD_LOCAL | RTLD_NOW);
    if (!handle) {
      std::cerr << "[CppInterOp] Failed to load library from " << libPath
                << ": " << dlerror() << '\n';
      return;
    }

    getCppProcAddress = reinterpret_cast<void* (*)(const char*)>(
        dlsym(handle, "CppGetProcAddress"));
    if (!getCppProcAddress) {
      std::cerr << "[CppInterOp] Failed to find CppGetProcAddress: "
                << dlerror() << '\n';
      dlclose(handle);
      handle = nullptr;
    }
  });

  return getCppProcAddress ? getCppProcAddress(name) : nullptr;
}

// Used for the extern clauses below
// FIXME: drop the using clauses
namespace CppAPIType {
#define X(name, type) using name = type;
CPPINTEROP_API_MAP
#undef X
} // end namespace CppAPIType

namespace CppInternal {
namespace Dispatch {

// FIXME: This is required for the types, but we should move the types
// into a separate namespace and only use that scope (CppImpl::Types)
using namespace CppImpl;

#define X(name, type) extern CppAPIType::name name;
CPPINTEROP_API_MAP
#undef X

/// Initialize all CppInterOp API from the dynamically loaded library
/// (RTLD_LOCAL) \param[in] customLibPath Optional custom path to
/// libclangCppInterOp.so \returns true if initialization succeeded, false
/// otherwise
inline bool init_functions(const char* customLibPath = nullptr) {
  // trigger library loading if custom path provided
  std::cout << "[CppInterOp] Initializing CppInterOp API functions from "
            << (customLibPath ? customLibPath : "default library path") << '\n';
  if (customLibPath) {
    void* test = dlGetProcAddress("GetInterpreter", customLibPath);
    if (!test) {
      std::cerr << "[CppInterOp] Failed to initialize with custom path: "
                << customLibPath << '\n';
      return false;
    }
  }

#define X(name, type) name = reinterpret_cast<type>(dlGetProcAddress(#name));
  CPPINTEROP_API_MAP
#undef X

  // test to verify that critical (and consequently all) functions loaded
  if (!GetInterpreter || !CreateInterpreter) {
    std::cerr << "[CppInterOp] Failed to load critical functions" << std::endl;
    return false;
  }

  return true;
}
} // namespace Dispatch
} // namespace CppInternal

namespace Cpp = CppInternal::Dispatch;
#endif // CPPINTEROP_DISPATCH_H
