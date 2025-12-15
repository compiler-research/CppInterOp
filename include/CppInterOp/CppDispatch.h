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
  X(CreateInterpreter, decltype(&Cpp::CreateInterpreter))                      \
  X(GetInterpreter, decltype(&Cpp::GetInterpreter))                            \
  X(Process, decltype(&Cpp::Process))                                          \
  X(GetResourceDir, decltype(&Cpp::GetResourceDir))                            \
  X(AddIncludePath, decltype(&Cpp::AddIncludePath))                            \
  X(LoadLibrary, decltype(&Cpp::LoadLibrary))                                  \
  X(Declare, decltype(&Cpp::Declare))                                          \
  X(DeleteInterpreter, decltype(&Cpp::DeleteInterpreter))                      \
  X(IsNamespace, decltype(&Cpp::IsNamespace))                                  \
  X(ObjToString, decltype(&Cpp::ObjToString))                                  \
  X(GetQualifiedCompleteName, decltype(&Cpp::GetQualifiedCompleteName))        \
  X(GetValueKind, decltype(&Cpp::GetValueKind))                                \
  X(GetNonReferenceType, decltype(&Cpp::GetNonReferenceType))                  \
  X(IsEnumType, decltype(&Cpp::IsEnumType))                                    \
  X(GetIntegerTypeFromEnumType, decltype(&Cpp::GetIntegerTypeFromEnumType))    \
  X(GetReferencedType, decltype(&Cpp::GetReferencedType))                      \
  X(IsPointerType, decltype(&Cpp::IsPointerType))                              \
  X(GetPointeeType, decltype(&Cpp::GetPointeeType))                            \
  X(GetPointerType, decltype(&Cpp::GetPointerType))                            \
  X(IsReferenceType, decltype(&Cpp::IsReferenceType))                          \
  X(GetTypeAsString, decltype(&Cpp::GetTypeAsString))                          \
  X(GetCanonicalType, decltype(&Cpp::GetCanonicalType))                        \
  X(HasTypeQualifier, decltype(&Cpp::HasTypeQualifier))                        \
  X(RemoveTypeQualifier, decltype(&Cpp::RemoveTypeQualifier))                  \
  X(GetUnderlyingType, decltype(&Cpp::GetUnderlyingType))                      \
  X(IsRecordType, decltype(&Cpp::IsRecordType))                                \
  X(IsFunctionPointerType, decltype(&Cpp::IsFunctionPointerType))              \
  X(GetVariableType, decltype(&Cpp::GetVariableType))                          \
  X(GetNamed, decltype(&Cpp::GetNamed))                                        \
  X(GetScopeFromType, decltype(&Cpp::GetScopeFromType))                        \
  X(GetClassTemplateInstantiationArgs,                                         \
    decltype(&Cpp::GetClassTemplateInstantiationArgs))                         \
  X(IsClass, decltype(&Cpp::IsClass))                                          \
  X(GetType, decltype(&Cpp::GetType))                                          \
  X(GetTypeFromScope, decltype(&Cpp::GetTypeFromScope))                        \
  X(GetComplexType, decltype(&Cpp::GetComplexType))                            \
  X(GetIntegerTypeFromEnumScope, decltype(&Cpp::GetIntegerTypeFromEnumScope))  \
  X(GetUnderlyingScope, decltype(&Cpp::GetUnderlyingScope))                    \
  X(GetScope, decltype(&Cpp::GetScope))                                        \
  X(GetGlobalScope, decltype(&Cpp::GetGlobalScope))                            \
  X(GetScopeFromCompleteName, decltype(&Cpp::GetScopeFromCompleteName))        \
  X(InstantiateTemplate, decltype(&Cpp::InstantiateTemplate))                  \
  X(GetParentScope, decltype(&Cpp::GetParentScope))                            \
  X(IsTemplate, decltype(&Cpp::IsTemplate))                                    \
  X(IsTemplateSpecialization, decltype(&Cpp::IsTemplateSpecialization))        \
  X(IsTypedefed, decltype(&Cpp::IsTypedefed))                                  \
  X(IsClassPolymorphic, decltype(&Cpp::IsClassPolymorphic))                    \
  X(Demangle, decltype(&Cpp::Demangle))                                        \
  X(SizeOf, decltype(&Cpp::SizeOf))                                            \
  X(GetSizeOfType, decltype(&Cpp::GetSizeOfType))                              \
  X(IsBuiltin, decltype(&Cpp::IsBuiltin))                                      \
  X(IsComplete, decltype(&Cpp::IsComplete))                                    \
  X(Allocate, decltype(&Cpp::Allocate))                                        \
  X(Deallocate, decltype(&Cpp::Deallocate))                                    \
  X(Construct, decltype(&Cpp::Construct))                                      \
  X(Destruct, decltype(&Cpp::Destruct))                                        \
  X(IsAbstract, decltype(&Cpp::IsAbstract))                                    \
  X(IsEnumScope, decltype(&Cpp::IsEnumScope))                                  \
  X(IsEnumConstant, decltype(&Cpp::IsEnumConstant))                            \
  X(IsAggregate, decltype(&Cpp::IsAggregate))                                  \
  X(HasDefaultConstructor, decltype(&Cpp::HasDefaultConstructor))              \
  X(IsVariable, decltype(&Cpp::IsVariable))                                    \
  X(GetAllCppNames, decltype(&Cpp::GetAllCppNames))                            \
  X(GetUsingNamespaces, decltype(&Cpp::GetUsingNamespaces))                    \
  X(GetCompleteName, decltype(&Cpp::GetCompleteName))                          \
  X(GetDestructor, decltype(&Cpp::GetDestructor))                              \
  X(IsVirtualMethod, decltype(&Cpp::IsVirtualMethod))                          \
  X(GetNumBases, decltype(&Cpp::GetNumBases))                                  \
  X(GetName, decltype(&Cpp::GetName))                                          \
  X(GetBaseClass, decltype(&Cpp::GetBaseClass))                                \
  X(IsSubclass, decltype(&Cpp::IsSubclass))                                    \
  X(GetOperator, decltype(&Cpp::GetOperator))                                  \
  X(GetFunctionReturnType, decltype(&Cpp::GetFunctionReturnType))              \
  X(GetBaseClassOffset, decltype(&Cpp::GetBaseClassOffset))                    \
  X(GetClassMethods, decltype(&Cpp::GetClassMethods))                          \
  X(GetFunctionsUsingName, decltype(&Cpp::GetFunctionsUsingName))              \
  X(GetFunctionNumArgs, decltype(&Cpp::GetFunctionNumArgs))                    \
  X(GetFunctionRequiredArgs, decltype(&Cpp::GetFunctionRequiredArgs))          \
  X(GetFunctionArgName, decltype(&Cpp::GetFunctionArgName))                    \
  X(GetFunctionArgType, decltype(&Cpp::GetFunctionArgType))                    \
  X(GetFunctionArgDefault, decltype(&Cpp::GetFunctionArgDefault))              \
  X(IsConstMethod, decltype(&Cpp::IsConstMethod))                              \
  X(GetFunctionTemplatedDecls, decltype(&Cpp::GetFunctionTemplatedDecls))      \
  X(ExistsFunctionTemplate, decltype(&Cpp::ExistsFunctionTemplate))            \
  X(IsTemplatedFunction, decltype(&Cpp::IsTemplatedFunction))                  \
  X(IsStaticMethod, decltype(&Cpp::IsStaticMethod))                            \
  X(GetClassTemplatedMethods, decltype(&Cpp::GetClassTemplatedMethods))        \
  X(BestOverloadFunctionMatch, decltype(&Cpp::BestOverloadFunctionMatch))      \
  X(GetOperatorFromSpelling, decltype(&Cpp::GetOperatorFromSpelling))          \
  X(IsFunctionDeleted, decltype(&Cpp::IsFunctionDeleted))                      \
  X(IsPublicMethod, decltype(&Cpp::IsPublicMethod))                            \
  X(IsProtectedMethod, decltype(&Cpp::IsProtectedMethod))                      \
  X(IsPrivateMethod, decltype(&Cpp::IsPrivateMethod))                          \
  X(IsConstructor, decltype(&Cpp::IsConstructor))                              \
  X(IsDestructor, decltype(&Cpp::IsDestructor))                                \
  X(GetDatamembers, decltype(&Cpp::GetDatamembers))                            \
  X(GetStaticDatamembers, decltype(&Cpp::GetStaticDatamembers))                \
  X(GetEnumConstantDatamembers, decltype(&Cpp::GetEnumConstantDatamembers))    \
  X(LookupDatamember, decltype(&Cpp::LookupDatamember))                        \
  X(IsLambdaClass, decltype(&Cpp::IsLambdaClass))                              \
  X(GetQualifiedName, decltype(&Cpp::GetQualifiedName))                        \
  X(GetVariableOffset, decltype(&Cpp::GetVariableOffset))                      \
  X(IsPublicVariable, decltype(&Cpp::IsPublicVariable))                        \
  X(IsProtectedVariable, decltype(&Cpp::IsProtectedVariable))                  \
  X(IsPrivateVariable, decltype(&Cpp::IsPrivateVariable))                      \
  X(IsStaticVariable, decltype(&Cpp::IsStaticVariable))                        \
  X(IsConstVariable, decltype(&Cpp::IsConstVariable))                          \
  X(GetDimensions, decltype(&Cpp::GetDimensions))                              \
  X(GetEnumConstants, decltype(&Cpp::GetEnumConstants))                        \
  X(GetEnumConstantType, decltype(&Cpp::GetEnumConstantType))                  \
  X(GetEnumConstantValue, decltype(&Cpp::GetEnumConstantValue))                \
  X(DumpScope, decltype(&Cpp::DumpScope))                                      \
  X(AddSearchPath, decltype(&Cpp::AddSearchPath))                              \
  X(Evaluate, decltype(&Cpp::Evaluate))                                        \
  X(IsDebugOutputEnabled, decltype(&Cpp::IsDebugOutputEnabled))                \
  X(EnableDebugOutput, decltype(&Cpp::EnableDebugOutput))                      \
  X(MakeFunctionCallable, Cpp::JitCall (*)(Cpp::TCppConstFunction_t))          \
  X(GetFunctionAddress, Cpp::TCppFuncAddr_t (*)(Cpp::TCppFunction_t))          \
  /*X(API_name, fnptr_ty)*/
namespace CppDispatch {
// forward all type aliases
using TCppIndex_t = ::Cpp::TCppIndex_t;
using TCppScope_t = ::Cpp::TCppScope_t;
using TCppConstScope_t = ::Cpp::TCppConstScope_t;
using TCppType_t = ::Cpp::TCppType_t;
using TCppFunction_t = ::Cpp::TCppFunction_t;
using TCppConstFunction_t = ::Cpp::TCppConstFunction_t;
using TCppFuncAddr_t = ::Cpp::TCppFuncAddr_t;
using TInterp_t = ::Cpp::TInterp_t;
using TCppObject_t = ::Cpp::TCppObject_t;

using Operator = ::Cpp::Operator;
using OperatorArity = ::Cpp::OperatorArity;
using QualKind = ::Cpp::QualKind;
using TemplateArgInfo = ::Cpp::TemplateArgInfo;
using ValueKind = ::Cpp::ValueKind;

using JitCall = ::Cpp::JitCall;
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

#endif // CPPINTEROP_CPPINTEROPDISPATCH_H
