//--------------------------------------------------------------------*- C++ -*-
// CppInterOp Dispatch Mechanism
// author:  Aaron Jomy <aaron.jomy@cern.ch>
//===----------------------------------------------------------------------===//
//
// This defines the mechanism which enables dispatching of the CppInterOp API
// without linking to it, preventing any LLVM or Clang symbols from being leaked
// into the client application.
//
//===----------------------------------------------------------------------===//
#ifndef CPPINTEROP_CPPINTEROPDISPATCH_H
#define CPPINTEROP_CPPINTEROPDISPATCH_H

#include <cstddef>
#include <cstdint>
#include <set>
#include <string>
#include <vector>

#include <cstdlib>
#include <dlfcn.h>
#include <iostream>
#include <mutex>

#include <CppInterOp/CppInterOp.h>

// Configured by CMake, can be overridden by defining before including this
// header
#ifndef CPPINTEROP_LIBRARY_PATH
#define CPPINTEROP_LIBRARY_PATH "@CPPINTEROP_LIBRARY_PATH@"
#endif

using __CPP_FUNC = void (*)();

///\param[in] procname - the name of the FunctionEntry in the symbol lookup
/// table.
///
///\returns the function address of the requested API, or nullptr if not found
extern "C" CPPINTEROP_API void (
    *CppGetProcAddress(const unsigned char* procname))(void);

#define EXTERN_CPP_FUNC_SIMPLE(func_name)                                      \
  extern CppAPIType::func_name func_name;

#define EXTERN_CPP_FUNC_OVERLOADED(func_name, signature)                       \
  extern CppAPIType::func_name func_name;

#define LOAD_CPP_FUNCTION_SIMPLE(func_name)                                    \
  func_name =                                                                  \
      reinterpret_cast<CppAPIType::func_name>(dlGetProcAddress(#func_name));

#define LOAD_CPP_FUNCTION_OVERLOADED(func_name, signature)                     \
  func_name =                                                                  \
      reinterpret_cast<CppAPIType::func_name>(dlGetProcAddress(#func_name));

#define DECLARE_CPP_NULL_SIMPLE(func_name)                                     \
  CppAPIType::func_name func_name = nullptr;

#define DECLARE_CPP_NULL_OVERLOADED(func_name, signature)                      \
  CppAPIType::func_name func_name = nullptr;

// macro that allows declaration and loading of all CppInterOp API functions in
// a consistent way. This is used as our dispatched API list, along with the
// name-address pair table
#define FOR_EACH_CPP_FUNCTION_SIMPLE(DO)                                       \
  DO(CreateInterpreter)                                                        \
  DO(GetInterpreter)                                                           \
  DO(Process)                                                                  \
  DO(GetResourceDir)                                                           \
  DO(AddIncludePath)                                                           \
  DO(LoadLibrary)                                                              \
  DO(Declare)                                                                  \
  DO(DeleteInterpreter)                                                        \
  DO(IsNamespace)                                                              \
  DO(ObjToString)                                                              \
  DO(GetQualifiedCompleteName)                                                 \
  DO(GetValueKind)                                                             \
  DO(GetNonReferenceType)                                                      \
  DO(IsEnumType)                                                               \
  DO(GetIntegerTypeFromEnumType)                                               \
  DO(GetReferencedType)                                                        \
  DO(IsPointerType)                                                            \
  DO(GetPointeeType)                                                           \
  DO(GetPointerType)                                                           \
  DO(IsReferenceType)                                                          \
  DO(GetTypeAsString)                                                          \
  DO(GetCanonicalType)                                                         \
  DO(HasTypeQualifier)                                                         \
  DO(RemoveTypeQualifier)                                                      \
  DO(GetUnderlyingType)                                                        \
  DO(IsRecordType)                                                             \
  DO(IsFunctionPointerType)                                                    \
  DO(GetVariableType)                                                          \
  DO(GetNamed)                                                                 \
  DO(GetScopeFromType)                                                         \
  DO(GetClassTemplateInstantiationArgs)                                        \
  DO(IsClass)                                                                  \
  DO(GetType)                                                                  \
  DO(GetTypeFromScope)                                                         \
  DO(GetComplexType)                                                           \
  DO(GetIntegerTypeFromEnumScope)                                              \
  DO(GetUnderlyingScope)                                                       \
  DO(GetScope)                                                                 \
  DO(GetGlobalScope)                                                           \
  DO(GetScopeFromCompleteName)                                                 \
  DO(InstantiateTemplate)                                                      \
  DO(GetParentScope)                                                           \
  DO(IsTemplate)                                                               \
  DO(IsTemplateSpecialization)                                                 \
  DO(IsTypedefed)                                                              \
  DO(IsClassPolymorphic)                                                       \
  DO(Demangle)                                                                 \
  DO(SizeOf)                                                                   \
  DO(GetSizeOfType)                                                            \
  DO(IsBuiltin)                                                                \
  DO(IsComplete)                                                               \
  DO(Allocate)                                                                 \
  DO(Deallocate)                                                               \
  DO(Construct)                                                                \
  DO(Destruct)                                                                 \
  DO(IsAbstract)                                                               \
  DO(IsEnumScope)                                                              \
  DO(IsEnumConstant)                                                           \
  DO(IsAggregate)                                                              \
  DO(HasDefaultConstructor)                                                    \
  DO(IsVariable)                                                               \
  DO(GetAllCppNames)                                                           \
  DO(GetUsingNamespaces)                                                       \
  DO(GetCompleteName)                                                          \
  DO(GetDestructor)                                                            \
  DO(IsVirtualMethod)                                                          \
  DO(GetNumBases)                                                              \
  DO(GetName)                                                                  \
  DO(GetBaseClass)                                                             \
  DO(IsSubclass)                                                               \
  DO(GetOperator)                                                              \
  DO(GetFunctionReturnType)                                                    \
  DO(GetBaseClassOffset)                                                       \
  DO(GetClassMethods)                                                          \
  DO(GetFunctionsUsingName)                                                    \
  DO(GetFunctionNumArgs)                                                       \
  DO(GetFunctionRequiredArgs)                                                  \
  DO(GetFunctionArgName)                                                       \
  DO(GetFunctionArgType)                                                       \
  DO(GetFunctionArgDefault)                                                    \
  DO(IsConstMethod)                                                            \
  DO(GetFunctionTemplatedDecls)                                                \
  DO(ExistsFunctionTemplate)                                                   \
  DO(IsTemplatedFunction)                                                      \
  DO(IsStaticMethod)                                                           \
  DO(GetClassTemplatedMethods)                                                 \
  DO(BestOverloadFunctionMatch)                                                \
  DO(GetOperatorFromSpelling)                                                  \
  DO(IsFunctionDeleted)                                                        \
  DO(IsPublicMethod)                                                           \
  DO(IsProtectedMethod)                                                        \
  DO(IsPrivateMethod)                                                          \
  DO(IsConstructor)                                                            \
  DO(IsDestructor)                                                             \
  DO(GetDatamembers)                                                           \
  DO(GetStaticDatamembers)                                                     \
  DO(GetEnumConstantDatamembers)                                               \
  DO(LookupDatamember)                                                         \
  DO(IsLambdaClass)                                                            \
  DO(GetQualifiedName)                                                         \
  DO(GetVariableOffset)                                                        \
  DO(IsPublicVariable)                                                         \
  DO(IsProtectedVariable)                                                      \
  DO(IsPrivateVariable)                                                        \
  DO(IsStaticVariable)                                                         \
  DO(IsConstVariable)                                                          \
  DO(GetDimensions)                                                            \
  DO(GetEnumConstants)                                                         \
  DO(GetEnumConstantType)                                                      \
  DO(GetEnumConstantValue)                                                     \
  DO(DumpScope)                                                                \
  DO(AddSearchPath)                                                            \
  DO(Evaluate)                                                                 \
  DO(IsDebugOutputEnabled)                                                     \
  DO(EnableDebugOutput)

#define FOR_EACH_CPP_FUNCTION_OVERLOADED(DO)                                   \
  DO(MakeFunctionCallable, Cpp::JitCall (*)(Cpp::TCppConstFunction_t))         \
  DO(GetFunctionAddress, Cpp::TCppFuncAddr_t (*)(Cpp::TCppFunction_t))

#define EXTRACT_NAME_OVERLOADED(name, sig) name

#define FOR_EACH_CPP_FUNCTION(DO)                                              \
  FOR_EACH_CPP_FUNCTION_SIMPLE(DO)                                             \
  FOR_EACH_CPP_FUNCTION_OVERLOADED(EXTRACT_NAME_OVERLOADED)

#define DECLARE_TYPE_SIMPLE(func_name)                                         \
  using func_name = decltype(&Cpp::func_name);

#define DECLARE_TYPE_OVERLOADED(func_name, signature)                          \
  using func_name = signature;
namespace CppDispatch {
// Forward all type aliases
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

namespace CppAPIType {
FOR_EACH_CPP_FUNCTION_SIMPLE(DECLARE_TYPE_SIMPLE)
FOR_EACH_CPP_FUNCTION_OVERLOADED(DECLARE_TYPE_OVERLOADED)
} // end namespace CppAPIType

#undef DECLARE_TYPE_SIMPLE
#undef DECLARE_TYPE_OVERLOADED

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
    if (!libPath || libPath[0] == '\0') {
      libPath = CPPINTEROP_LIBRARY_PATH;
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
namespace CppDispatch {
FOR_EACH_CPP_FUNCTION_SIMPLE(EXTERN_CPP_FUNC_SIMPLE)
FOR_EACH_CPP_FUNCTION_OVERLOADED(EXTERN_CPP_FUNC_OVERLOADED)

/// Initialize all CppInterOp API from the dynamically loaded library
/// (RTLD_LOCAL) \param[in] customLibPath Optional custom path to
/// libclangCppInterOp.so \returns true if initialization succeeded, false
/// otherwise
inline bool init_functions(const char* customLibPath = nullptr) {
  // trigger library loading if custom path provided
  if (customLibPath) {
    void* test = dlGetProcAddress("GetInterpreter", customLibPath);
    if (!test) {
      std::cerr << "[CppInterOp] Failed to initialize with custom path: "
                << customLibPath << '\n';
      return false;
    }
  }

  FOR_EACH_CPP_FUNCTION_SIMPLE(LOAD_CPP_FUNCTION_SIMPLE)
  FOR_EACH_CPP_FUNCTION_OVERLOADED(LOAD_CPP_FUNCTION_OVERLOADED)

  // test to verify that critical (and consequently all) functions loaded
  if (!GetInterpreter || !CreateInterpreter) {
    std::cerr << "[CppInterOp] Failed to load critical functions" << std::endl;
    return false;
  }

  return true;
}
} // namespace CppDispatch

#endif // CPPINTEROP_CPPINTEROPDISPATCH_H
