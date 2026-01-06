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
#ifndef CPPINTEROP_CPPINTEROPDISPATCH_H
#define CPPINTEROP_CPPINTEROPDISPATCH_H

#include <cstdlib>
#include <dlfcn.h>
#include <iostream>
#include <string>
#include <mutex>

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
  X(CreateInterpreter, decltype(&CppStatic::CreateInterpreter))                \
  X(GetInterpreter, decltype(&CppStatic::GetInterpreter))                      \
  X(Process, decltype(&CppStatic::Process))                                    \
  X(GetResourceDir, decltype(&CppStatic::GetResourceDir))                      \
  X(AddIncludePath, decltype(&CppStatic::AddIncludePath))                      \
  X(LoadLibrary, decltype(&CppStatic::LoadLibrary))                            \
  X(Declare, decltype(&CppStatic::Declare))                                    \
  X(DeleteInterpreter, decltype(&CppStatic::DeleteInterpreter))                \
  X(IsNamespace, decltype(&CppStatic::IsNamespace))                            \
  X(ObjToString, decltype(&CppStatic::ObjToString))                            \
  X(GetQualifiedCompleteName, decltype(&CppStatic::GetQualifiedCompleteName))  \
  X(GetValueKind, decltype(&CppStatic::GetValueKind))                          \
  X(GetNonReferenceType, decltype(&CppStatic::GetNonReferenceType))            \
  X(IsEnumType, decltype(&CppStatic::IsEnumType))                              \
  X(GetIntegerTypeFromEnumType,                                                \
    decltype(&CppStatic::GetIntegerTypeFromEnumType))                          \
  X(GetReferencedType, decltype(&CppStatic::GetReferencedType))                \
  X(IsPointerType, decltype(&CppStatic::IsPointerType))                        \
  X(GetPointeeType, decltype(&CppStatic::GetPointeeType))                      \
  X(GetPointerType, decltype(&CppStatic::GetPointerType))                      \
  X(IsReferenceType, decltype(&CppStatic::IsReferenceType))                    \
  X(GetTypeAsString, decltype(&CppStatic::GetTypeAsString))                    \
  X(GetCanonicalType, decltype(&CppStatic::GetCanonicalType))                  \
  X(HasTypeQualifier, decltype(&CppStatic::HasTypeQualifier))                  \
  X(RemoveTypeQualifier, decltype(&CppStatic::RemoveTypeQualifier))            \
  X(GetUnderlyingType, decltype(&CppStatic::GetUnderlyingType))                \
  X(IsRecordType, decltype(&CppStatic::IsRecordType))                          \
  X(IsFunctionPointerType, decltype(&CppStatic::IsFunctionPointerType))        \
  X(GetVariableType, decltype(&CppStatic::GetVariableType))                    \
  X(GetNamed, decltype(&CppStatic::GetNamed))                                  \
  X(GetScopeFromType, decltype(&CppStatic::GetScopeFromType))                  \
  X(GetClassTemplateInstantiationArgs,                                         \
    decltype(&CppStatic::GetClassTemplateInstantiationArgs))                   \
  X(IsClass, decltype(&CppStatic::IsClass))                                    \
  X(GetType, decltype(&CppStatic::GetType))                                    \
  X(GetTypeFromScope, decltype(&CppStatic::GetTypeFromScope))                  \
  X(GetComplexType, decltype(&CppStatic::GetComplexType))                      \
  X(GetIntegerTypeFromEnumScope,                                               \
    decltype(&CppStatic::GetIntegerTypeFromEnumScope))                         \
  X(GetUnderlyingScope, decltype(&CppStatic::GetUnderlyingScope))              \
  X(GetScope, decltype(&CppStatic::GetScope))                                  \
  X(GetGlobalScope, decltype(&CppStatic::GetGlobalScope))                      \
  X(GetScopeFromCompleteName, decltype(&CppStatic::GetScopeFromCompleteName))  \
  X(InstantiateTemplate, decltype(&CppStatic::InstantiateTemplate))            \
  X(GetParentScope, decltype(&CppStatic::GetParentScope))                      \
  X(IsTemplate, decltype(&CppStatic::IsTemplate))                              \
  X(IsTemplateSpecialization, decltype(&CppStatic::IsTemplateSpecialization))  \
  X(IsTypedefed, decltype(&CppStatic::IsTypedefed))                            \
  X(IsClassPolymorphic, decltype(&CppStatic::IsClassPolymorphic))              \
  X(Demangle, decltype(&CppStatic::Demangle))                                  \
  X(SizeOf, decltype(&CppStatic::SizeOf))                                      \
  X(GetSizeOfType, decltype(&CppStatic::GetSizeOfType))                        \
  X(IsBuiltin, decltype(&CppStatic::IsBuiltin))                                \
  X(IsComplete, decltype(&CppStatic::IsComplete))                              \
  X(Allocate, decltype(&CppStatic::Allocate))                                  \
  X(Deallocate, decltype(&CppStatic::Deallocate))                              \
  X(Construct, decltype(&CppStatic::Construct))                                \
  X(Destruct, decltype(&CppStatic::Destruct))                                  \
  X(IsAbstract, decltype(&CppStatic::IsAbstract))                              \
  X(IsEnumScope, decltype(&CppStatic::IsEnumScope))                            \
  X(IsEnumConstant, decltype(&CppStatic::IsEnumConstant))                      \
  X(IsAggregate, decltype(&CppStatic::IsAggregate))                            \
  X(HasDefaultConstructor, decltype(&CppStatic::HasDefaultConstructor))        \
  X(IsVariable, decltype(&CppStatic::IsVariable))                              \
  X(GetAllCppNames, decltype(&CppStatic::GetAllCppNames))                      \
  X(GetUsingNamespaces, decltype(&CppStatic::GetUsingNamespaces))              \
  X(GetCompleteName, decltype(&CppStatic::GetCompleteName))                    \
  X(GetDestructor, decltype(&CppStatic::GetDestructor))                        \
  X(IsVirtualMethod, decltype(&CppStatic::IsVirtualMethod))                    \
  X(GetNumBases, decltype(&CppStatic::GetNumBases))                            \
  X(GetName, decltype(&CppStatic::GetName))                                    \
  X(GetBaseClass, decltype(&CppStatic::GetBaseClass))                          \
  X(IsSubclass, decltype(&CppStatic::IsSubclass))                              \
  X(GetOperator, decltype(&CppStatic::GetOperator))                            \
  X(GetFunctionReturnType, decltype(&CppStatic::GetFunctionReturnType))        \
  X(GetBaseClassOffset, decltype(&CppStatic::GetBaseClassOffset))              \
  X(GetClassMethods, decltype(&CppStatic::GetClassMethods))                    \
  X(GetFunctionsUsingName, decltype(&CppStatic::GetFunctionsUsingName))        \
  X(GetFunctionNumArgs, decltype(&CppStatic::GetFunctionNumArgs))              \
  X(GetFunctionRequiredArgs, decltype(&CppStatic::GetFunctionRequiredArgs))    \
  X(GetFunctionArgName, decltype(&CppStatic::GetFunctionArgName))              \
  X(GetFunctionArgType, decltype(&CppStatic::GetFunctionArgType))              \
  X(GetFunctionArgDefault, decltype(&CppStatic::GetFunctionArgDefault))        \
  X(IsConstMethod, decltype(&CppStatic::IsConstMethod))                        \
  X(GetFunctionTemplatedDecls,                                                 \
    decltype(&CppStatic::GetFunctionTemplatedDecls))                           \
  X(ExistsFunctionTemplate, decltype(&CppStatic::ExistsFunctionTemplate))      \
  X(IsTemplatedFunction, decltype(&CppStatic::IsTemplatedFunction))            \
  X(IsStaticMethod, decltype(&CppStatic::IsStaticMethod))                      \
  X(GetClassTemplatedMethods, decltype(&CppStatic::GetClassTemplatedMethods))  \
  X(BestOverloadFunctionMatch,                                                 \
    decltype(&CppStatic::BestOverloadFunctionMatch))                           \
  X(GetOperatorFromSpelling, decltype(&CppStatic::GetOperatorFromSpelling))    \
  X(IsFunctionDeleted, decltype(&CppStatic::IsFunctionDeleted))                \
  X(IsPublicMethod, decltype(&CppStatic::IsPublicMethod))                      \
  X(IsProtectedMethod, decltype(&CppStatic::IsProtectedMethod))                \
  X(IsPrivateMethod, decltype(&CppStatic::IsPrivateMethod))                    \
  X(IsConstructor, decltype(&CppStatic::IsConstructor))                        \
  X(IsDestructor, decltype(&CppStatic::IsDestructor))                          \
  X(GetDatamembers, decltype(&CppStatic::GetDatamembers))                      \
  X(GetStaticDatamembers, decltype(&CppStatic::GetStaticDatamembers))          \
  X(GetEnumConstantDatamembers,                                                \
    decltype(&CppStatic::GetEnumConstantDatamembers))                          \
  X(LookupDatamember, decltype(&CppStatic::LookupDatamember))                  \
  X(IsLambdaClass, decltype(&CppStatic::IsLambdaClass))                        \
  X(GetQualifiedName, decltype(&CppStatic::GetQualifiedName))                  \
  X(GetVariableOffset, decltype(&CppStatic::GetVariableOffset))                \
  X(IsPublicVariable, decltype(&CppStatic::IsPublicVariable))                  \
  X(IsProtectedVariable, decltype(&CppStatic::IsProtectedVariable))            \
  X(IsPrivateVariable, decltype(&CppStatic::IsPrivateVariable))                \
  X(IsStaticVariable, decltype(&CppStatic::IsStaticVariable))                  \
  X(IsConstVariable, decltype(&CppStatic::IsConstVariable))                    \
  X(GetDimensions, decltype(&CppStatic::GetDimensions))                        \
  X(GetEnumConstants, decltype(&CppStatic::GetEnumConstants))                  \
  X(GetEnumConstantType, decltype(&CppStatic::GetEnumConstantType))            \
  X(GetEnumConstantValue, decltype(&CppStatic::GetEnumConstantValue))          \
  X(DumpScope, decltype(&CppStatic::DumpScope))                                \
  X(AddSearchPath, decltype(&CppStatic::AddSearchPath))                        \
  X(Evaluate, decltype(&CppStatic::Evaluate))                                  \
  X(IsDebugOutputEnabled, decltype(&CppStatic::IsDebugOutputEnabled))          \
  X(EnableDebugOutput, decltype(&CppStatic::EnableDebugOutput))                \
  X(MakeFunctionCallable,                                                      \
    CppStatic::JitCall (*)(CppStatic::TCppConstFunction_t))                    \
  X(GetFunctionAddress,                                                        \
    CppStatic::TCppFuncAddr_t (*)(CppStatic::TCppFunction_t))                  \
  /*X(API_name, fnptr_ty)*/
namespace CppDispatch {
// forward all type aliases
using TCppIndex_t = ::CppStatic::TCppIndex_t;
using TCppScope_t = ::CppStatic::TCppScope_t;
using TCppConstScope_t = ::CppStatic::TCppConstScope_t;
using TCppType_t = ::CppStatic::TCppType_t;
using TCppFunction_t = ::CppStatic::TCppFunction_t;
using TCppConstFunction_t = ::CppStatic::TCppConstFunction_t;
using TCppFuncAddr_t = ::CppStatic::TCppFuncAddr_t;
using TInterp_t = ::CppStatic::TInterp_t;
using TCppObject_t = ::CppStatic::TCppObject_t;

using Operator = ::CppStatic::Operator;
using OperatorArity = ::CppStatic::OperatorArity;
using QualKind = ::CppStatic::QualKind;
using TemplateArgInfo = ::CppStatic::TemplateArgInfo;
using ValueKind = ::CppStatic::ValueKind;

using JitCall = ::CppStatic::JitCall;
} // end namespace CppDispatch

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

namespace CppDispatch {

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
            << (customLibPath ? customLibPath
                             : "default library path") << '\n';
  if (customLibPath) {
    void* test = dlGetProcAddress("GetInterpreter", customLibPath);
    if (!test) {
      std::cerr << "[CppInterOp] Failed to initialize with custom path: "
                << customLibPath << '\n';
      return false;
    }
  }

#define X(name, type)                                                          \
  name = reinterpret_cast<type>(dlGetProcAddress(#name));
  CPPINTEROP_API_MAP
#undef X

  // test to verify that critical (and consequently all) functions loaded
  if (!GetInterpreter || !CreateInterpreter) {
    std::cerr << "[CppInterOp] Failed to load critical functions" << std::endl;
    return false;
  }

  return true;
}
} // namespace CppDispatch

namespace Cpp = CppDispatch;
#endif // CPPINTEROP_CPPINTEROPDISPATCH_H
