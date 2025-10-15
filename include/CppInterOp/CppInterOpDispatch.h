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

#define DECLARE_CPP_NULL(func_name) CppAPIType::func_name func_name = nullptr;

#define EXTERN_CPP_FUNC(func_name) extern CppAPIType::func_name func_name;

#define LOAD_CPP_FUNCTION(func_name)                                           \
  func_name =                                                                  \
      reinterpret_cast<CppAPIType::func_name>(dlGetProcAddress(#func_name));

// macro that allows declaration and loading of all CppInterOp API functions in
// a consistent way. This is used as our dispatched API list, along with the
// name-address pair table
#define FOR_EACH_CPP_FUNCTION(DO)                                              \
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
  DO(MakeFunctionCallable)                                                     \
  DO(GetFunctionAddress)                                                       \
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

using GetVersion = std::string (*)();

using Demangle = std::string (*)(const std::string& mangled_name);

using EnableDebugOutput = void (*)(bool value);

using IsDebugOutputEnabled = bool (*)();

using IsAggregate = bool (*)(Cpp::TCppScope_t scope);

using IsNamespace = bool (*)(Cpp::TCppScope_t scope);

using IsClass = bool (*)(Cpp::TCppScope_t scope);

using IsFunction = bool (*)(Cpp::TCppScope_t scope);

using IsFunctionPointerType = bool (*)(Cpp::TCppType_t type);

using IsClassPolymorphic = bool (*)(Cpp::TCppScope_t klass);

using IsComplete = bool (*)(Cpp::TCppScope_t scope);

using SizeOf = size_t (*)(Cpp::TCppScope_t scope);

using IsBuiltin = bool (*)(Cpp::TCppType_t type);

using IsTemplate = bool (*)(Cpp::TCppScope_t handle);

using IsTemplateSpecialization = bool (*)(Cpp::TCppScope_t handle);

using IsTypedefed = bool (*)(Cpp::TCppScope_t handle);

using IsAbstract = bool (*)(Cpp::TCppType_t klass);

using IsEnumScope = bool (*)(Cpp::TCppScope_t handle);

using IsEnumConstant = bool (*)(Cpp::TCppScope_t handle);

using IsEnumType = bool (*)(Cpp::TCppType_t type);

using HasTypeQualifier = bool (*)(Cpp::TCppType_t type, Cpp::QualKind qual);

using RemoveTypeQualifier = Cpp::TCppType_t (*)(Cpp::TCppType_t type,
                                                Cpp::QualKind qual);

using AddTypeQualifier = Cpp::TCppType_t (*)(Cpp::TCppType_t type,
                                             Cpp::QualKind qual);

using GetEnums = void (*)(Cpp::TCppScope_t scope,
                          std::vector<Cpp::TCppScope_t>& Result);

using IsSmartPtrType = bool (*)(Cpp::TCppType_t type);

using GetIntegerTypeFromEnumScope =
    Cpp::TCppType_t (*)(Cpp::TCppScope_t handle);

using GetIntegerTypeFromEnumType = Cpp::TCppType_t (*)(Cpp::TCppType_t handle);

using GetEnumConstants =
    std::vector<Cpp::TCppScope_t> (*)(Cpp::TCppScope_t scope);

using GetEnumConstantType = Cpp::TCppType_t (*)(Cpp::TCppScope_t scope);

using GetEnumConstantValue = Cpp::TCppIndex_t (*)(Cpp::TCppScope_t scope);

using GetSizeOfType = size_t (*)(Cpp::TCppType_t type);

using IsVariable = bool (*)(Cpp::TCppScope_t scope);

using GetName = std::string (*)(Cpp::TCppScope_t klass);

using GetCompleteName = std::string (*)(Cpp::TCppScope_t klass);

using GetQualifiedName = std::string (*)(Cpp::TCppScope_t klass);

using GetQualifiedCompleteName = std::string (*)(Cpp::TCppScope_t klass);

using GetUsingNamespaces =
    std::vector<Cpp::TCppScope_t> (*)(Cpp::TCppScope_t scope);

using GetGlobalScope = Cpp::TCppScope_t (*)();

using GetUnderlyingScope = Cpp::TCppScope_t (*)(Cpp::TCppScope_t scope);

using GetScope = Cpp::TCppScope_t (*)(const std::string& name,
                                      Cpp::TCppScope_t parent);

using GetScopeFromCompleteName = Cpp::TCppScope_t (*)(const std::string& name);

using GetNamed = Cpp::TCppScope_t (*)(const std::string& name,
                                      Cpp::TCppScope_t parent);

using GetParentScope = Cpp::TCppScope_t (*)(Cpp::TCppScope_t scope);

using GetScopeFromType = Cpp::TCppScope_t (*)(Cpp::TCppType_t type);

using GetNumBases = Cpp::TCppIndex_t (*)(Cpp::TCppScope_t klass);

using GetBaseClass = Cpp::TCppScope_t (*)(Cpp::TCppScope_t klass,
                                          Cpp::TCppIndex_t ibase);

using IsSubclass = bool (*)(Cpp::TCppScope_t derived, Cpp::TCppScope_t base);

using GetBaseClassOffset = int64_t (*)(Cpp::TCppScope_t derived,
                                       Cpp::TCppScope_t base);

using GetClassMethods = void (*)(Cpp::TCppScope_t klass,
                                 std::vector<Cpp::TCppScope_t>& methods);

using GetFunctionTemplatedDecls =
    void (*)(Cpp::TCppScope_t klass, std::vector<Cpp::TCppScope_t>& methods);

using HasDefaultConstructor = bool (*)(Cpp::TCppScope_t scope);

using GetDefaultConstructor = Cpp::TCppFunction_t (*)(Cpp::TCppScope_t scope);

using GetDestructor = Cpp::TCppFunction_t (*)(Cpp::TCppScope_t scope);

using GetFunctionsUsingName = std::vector<Cpp::TCppFunction_t> (*)(
    Cpp::TCppScope_t scope, const std::string& name);

using GetFunctionReturnType = Cpp::TCppType_t (*)(Cpp::TCppFunction_t func);

using GetFunctionNumArgs = Cpp::TCppIndex_t (*)(Cpp::TCppFunction_t func);

using GetFunctionRequiredArgs =
    Cpp::TCppIndex_t (*)(Cpp::TCppConstFunction_t func);

using GetFunctionArgType = Cpp::TCppType_t (*)(Cpp::TCppFunction_t func,
                                               Cpp::TCppIndex_t iarg);

using GetFunctionSignature = std::string (*)(Cpp::TCppFunction_t func);

using IsFunctionDeleted = bool (*)(Cpp::TCppConstFunction_t function);

using IsTemplatedFunction = bool (*)(Cpp::TCppFunction_t func);

using ExistsFunctionTemplate = bool (*)(const std::string& name,
                                        Cpp::TCppScope_t parent);

using GetClassTemplatedMethods =
    bool (*)(const std::string& name, Cpp::TCppScope_t parent,
             std::vector<Cpp::TCppFunction_t>& funcs);

using IsMethod = bool (*)(Cpp::TCppConstFunction_t method);

using IsPublicMethod = bool (*)(Cpp::TCppFunction_t method);

using IsProtectedMethod = bool (*)(Cpp::TCppFunction_t method);

using IsPrivateMethod = bool (*)(Cpp::TCppFunction_t method);

using IsConstructor = bool (*)(Cpp::TCppConstFunction_t method);

using IsDestructor = bool (*)(Cpp::TCppConstFunction_t method);

using IsStaticMethod = bool (*)(Cpp::TCppConstFunction_t method);

using GetFunctionAddressFromName =
    Cpp::TCppFuncAddr_t (*)(const char* mangled_name);

using GetFunctionAddressFromMethod =
    Cpp::TCppFuncAddr_t (*)(Cpp::TCppFunction_t method);

using IsVirtualMethod = bool (*)(Cpp::TCppFunction_t method);

using GetDatamembers = void (*)(Cpp::TCppScope_t scope,
                                std::vector<Cpp::TCppScope_t>& datamembers);

using GetStaticDatamembers = void (*)(
    Cpp::TCppScope_t scope, std::vector<Cpp::TCppScope_t>& datamembers);

using GetEnumConstantDatamembers =
    void (*)(Cpp::TCppScope_t scope, std::vector<Cpp::TCppScope_t>& datamembers,
             bool include_enum_class);

using LookupDatamember = Cpp::TCppScope_t (*)(const std::string& name,
                                              Cpp::TCppScope_t parent);

using IsLambdaClass = bool (*)(Cpp::TCppType_t type);

using GetVariableType = Cpp::TCppType_t (*)(Cpp::TCppScope_t var);

using GetVariableOffset = intptr_t (*)(Cpp::TCppScope_t var,
                                       Cpp::TCppScope_t parent);

using IsPublicVariable = bool (*)(Cpp::TCppScope_t var);

using IsProtectedVariable = bool (*)(Cpp::TCppScope_t var);

using IsPrivateVariable = bool (*)(Cpp::TCppScope_t var);

using IsStaticVariable = bool (*)(Cpp::TCppScope_t var);

using IsConstVariable = bool (*)(Cpp::TCppScope_t var);

using IsRecordType = bool (*)(Cpp::TCppType_t type);

using IsPODType = bool (*)(Cpp::TCppType_t type);

using IsPointerType = bool (*)(Cpp::TCppType_t type);

using GetPointeeType = Cpp::TCppType_t (*)(Cpp::TCppType_t type);

using IsReferenceType = bool (*)(Cpp::TCppType_t type);

using GetValueKind = Cpp::ValueKind (*)(Cpp::TCppType_t type);

using IsRValueReferenceType = bool (*)(Cpp::TCppType_t type);

using GetPointerType = Cpp::TCppType_t (*)(Cpp::TCppType_t type);

using GetReferencedType = Cpp::TCppType_t (*)(Cpp::TCppType_t type,
                                              bool rvalue);

using GetNonReferenceType = Cpp::TCppType_t (*)(Cpp::TCppType_t type);

using GetTypeAsString = std::string (*)(Cpp::TCppType_t type);

using GetCanonicalType = Cpp::TCppType_t (*)(Cpp::TCppType_t type);

using JitCallMakeFunctionCallable =
    Cpp::JitCall (*)(Cpp::TCppConstFunction_t func);

using IsConstMethod = bool (*)(Cpp::TCppFunction_t method);

using GetFunctionArgDefault = std::string (*)(Cpp::TCppFunction_t func,
                                              Cpp::TCppIndex_t param_index);

using GetFunctionArgName = std::string (*)(Cpp::TCppFunction_t func,
                                           Cpp::TCppIndex_t param_index);

using GetSpellingFromOperator = std::string (*)(Cpp::Operator op);

using GetOperatorFromSpelling = Cpp::Operator (*)(const std::string& op);

using GetOperatorArity = Cpp::OperatorArity (*)(Cpp::TCppFunction_t op);

using GetOperator = void (*)(Cpp::TCppScope_t scope, Cpp::Operator op,
                             std::vector<Cpp::TCppFunction_t>& operators,
                             Cpp::OperatorArity kind);

using CreateInterpreter =
    Cpp::TInterp_t (*)(const std::vector<const char*>& Args,
                       const std::vector<const char*>& GpuArgs);

using DeleteInterpreter = bool (*)(Cpp::TInterp_t interp);

using ActivateInterpreter = bool (*)(Cpp::TInterp_t interp);

using GetInterpreter = Cpp::TInterp_t (*)();

using UseExternalInterpreter = void (*)(Cpp::TInterp_t interp);

using AddSearchPath = void (*)(const char* dir, bool isUser, bool prepend);

using GetResourceDir = const char* (*)();

using DetectResourceDir = std::string (*)(const char* ClangBinaryName);

using DetectSystemCompilerIncludePaths =
    void (*)(std::vector<std::string>& Paths, const char* CompilerName);

using AddIncludePath = void (*)(const char* dir);

using GetIncludePaths = void (*)(std::vector<std::string>& IncludePaths,
                                 bool withSystem, bool withFlags);

using Declare = int (*)(const char* code, bool silent);

using Process = int (*)(const char* code);

using Evaluate = intptr_t (*)(const char* code, bool* HadError);

using LookupLibrary = std::string (*)(const char* lib_name);

using LoadLibrary = bool (*)(const char* lib_stem, bool lookup);

using UnloadLibrary = void (*)(const char* lib_stem);

using SearchLibrariesForSymbol = std::string (*)(const char* mangled_name,
                                                 bool search_system);

using InsertOrReplaceJitSymbol = bool (*)(const char* linker_mangled_name,
                                          uint64_t address);

using ObjToString = std::string (*)(const char* type, void* obj);

using GetUnderlyingType = Cpp::TCppType_t (*)(Cpp::TCppType_t type);

using BestOverloadFunctionMatch = Cpp::TCppFunction_t (*)(
    const std::vector<Cpp::TCppFunction_t>& candidates,
    const std::vector<Cpp::TemplateArgInfo>& explicit_types,
    const std::vector<Cpp::TemplateArgInfo>& arg_types);

using GetDimensions = std::vector<long int> (*)(Cpp::TCppType_t var);

using DumpScope = void (*)(Cpp::TCppScope_t scope);

using GetClassTemplateInstantiationArgs =
    void (*)(Cpp::TCppScope_t klass, std::vector<Cpp::TemplateArgInfo>& args);

using GetAllCppNames = void (*)(Cpp::TCppScope_t scope,
                                std::set<std::string>& names);

using Deallocate = void (*)(Cpp::TCppScope_t scope, Cpp::TCppObject_t address,
                            Cpp::TCppIndex_t count);

using Allocate = Cpp::TCppObject_t (*)(Cpp::TCppScope_t scope,
                                       Cpp::TCppIndex_t count);

using InstantiateTemplate = Cpp::TCppScope_t (*)(
    Cpp::TCppScope_t tmpl, const Cpp::TemplateArgInfo* template_args,
    size_t template_args_size, bool instantiate_body);

using GetComplexType = Cpp::TCppType_t (*)(Cpp::TCppType_t type);

using GetTypeFromScope = Cpp::TCppType_t (*)(Cpp::TCppScope_t klass);

using GetType = Cpp::TCppType_t (*)(const std::string& type);

using Construct = Cpp::TCppObject_t (*)(Cpp::TCppScope_t scope,
                                        Cpp::TCppObject_t arena,
                                        Cpp::TCppIndex_t count);

using Destruct = bool (*)(Cpp::TCppObject_t This, Cpp::TCppScope_t scope,
                          bool withFree, Cpp::TCppIndex_t count);

using MakeFunctionCallable = Cpp::JitCall (*)(Cpp::TCppConstFunction_t func);
using GetFunctionAddress =
    Cpp::TCppFuncAddr_t (*)(Cpp::TCppConstFunction_t func);
} // end namespace CppAPIType

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
FOR_EACH_CPP_FUNCTION(EXTERN_CPP_FUNC);
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

  FOR_EACH_CPP_FUNCTION(LOAD_CPP_FUNCTION);

  // test to verify that critical (and consequently all) functions loaded
  if (!GetInterpreter || !CreateInterpreter) {
    std::cerr << "[CppInterOp] Failed to load critical functions" << std::endl;
    return false;
  }

  return true;
}
} // namespace CppDispatch

#endif // CPPINTEROP_CPPINTEROPDISPATCH_H
