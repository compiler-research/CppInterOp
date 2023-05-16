//--------------------------------------------------------------------*- C++ -*-
// CLING - the C++ LLVM-based InterpreterG :)
// author:  Vassil Vassilev <vvasilev@cern.ch>
//
// This file is dual-licensed: you can choose to license it under the University
// of Illinois Open Source License or the GNU Lesser General Public License. See
// LICENSE.TXT for details.
//------------------------------------------------------------------------------

#ifndef CPPINTEROP_CPPINTEROP_H
#define CPPINTEROP_CPPINTEROP_H

#include <cassert>
#include <string>
#include <vector>

namespace Cpp {
  using TCppIndex_t = size_t;
  using TCppScope_t = void*;
  using TCppType_t = void*;
  using TCppFunction_t = void*;
  using TCppConstFunction_t = const void*;
  using TCppFuncAddr_t = void*;
  using TCppSema_t = void *;
  using TInterp_t = void*;
  using TCppObject_t = void*;
  /// A class modeling function calls for functions produced by the interpreter
  /// in compiled code. It provides an information if we are calling a standard
  /// function, constructor or destructor.
  class JitCall {
  public:
    friend JitCall MakeFunctionCallable(TInterp_t, TCppConstFunction_t);
    enum Kind : char {
      kUnknown = 0,
      kGenericCall,
      kDestructorCall,
    };
    struct ArgList {
      void** m_Args = nullptr;
      size_t m_ArgSize = 0;
      // Clang struggles with =default...
      ArgList() : m_Args(nullptr), m_ArgSize(0) {}
      ArgList(void** Args, size_t ArgSize)
        : m_Args(Args), m_ArgSize(ArgSize) {}
    };
    // FIXME: Figure out how to unify the wrapper signatures.
    // FIXME: Hide these implementation details by moving wrapper generation in
    // this class.
    using GenericCall = void (*)(void*, int, void**, void*);
    using DestructorCall = void (*)(void*, unsigned long, int);
  private:
    union {
      GenericCall m_GenericCall;
      DestructorCall m_DestructorCall;
    };
    const Kind m_Kind;
    TCppConstFunction_t m_FD;
    JitCall() : m_Kind(kUnknown), m_GenericCall(nullptr), m_FD(nullptr) {}
    JitCall(Kind K, GenericCall C, TCppConstFunction_t FD)
      : m_Kind(K), m_GenericCall(C), m_FD(FD) {}
    JitCall(Kind K, DestructorCall C, TCppConstFunction_t Dtor)
      : m_Kind(K), m_DestructorCall(C), m_FD(Dtor) {}
    bool AreArgumentsValid(void* result, ArgList args, void* self) const;
    void ReportInvokeStart(void* result, ArgList args, void* self) const;
    void ReportInvokeStart(void* object, unsigned long nary,
                           int withFree) const;
    void ReportInvokeEnd() const;
  public:
    Kind getKind() const { return m_Kind; }
    bool isValid() const { return getKind() != kUnknown; }
    bool isInvalid() const { return !isValid(); }
    explicit operator bool() const { return isValid(); }

    // Specialized for calling void functions.
    void Invoke(ArgList args = {}, void* self = nullptr) const {
      Invoke(/*result=*/nullptr, args, self);
    }

    /// Makes a call to a generic function or method.
    ///\param[in] result - the location where the return result will be placed.
    ///\param[in] args - a pointer to a argument list and argument size.
    ///\param[in] self - the this pointer of the object.
    // FIXME: Adjust the arguments and their types: args_size can be unsigned;
    // self can go in the end and be nullptr by default; result can be a nullptr
    // by default. These changes should be syncronized with the wrapper if we
    // decide to directly.
    void Invoke(void* result, ArgList args = {}, void* self = nullptr) const {
      // Forward if we intended to call a dtor with only 1 parameter.
      if (m_Kind == kDestructorCall && result && !args.m_Args)
        return InvokeDestructor(result, /*nary=*/0UL, /*withFree=*/true);

#ifndef NDEBUG
      assert(AreArgumentsValid(result, args, self) && "Invalid args!");
      ReportInvokeStart(result, args, self);
#endif // NDEBUG
      m_GenericCall(self, args.m_ArgSize, args.m_Args, result);
    }
    /// Makes a call to a destructor.
    ///\param[in] object - the pointer of the object whose destructor we call.
    ///\param[in] nary - the count of the objects we destruct if we deal with an
    ///           array of objects.
    ///\param[in] withFree - true if we should call operator delete or false if
    ///           we should call only the destructor.
    //FIXME: Change the type of withFree from int to bool in the wrapper code.
    void InvokeDestructor(void* object, unsigned long nary = 0,
                          int withFree = true) const {
      assert(m_Kind == kDestructorCall && "Wrong overload!");
#ifndef NDEBUG
      ReportInvokeStart(object, nary, withFree);
#endif // NDEBUG
      m_DestructorCall(object, nary, withFree);
    }
  };

  /// Enables or disables the debugging printouts on stderr.
  /// Debugging output can be enabled also by the environment variable
  /// CPPINTEROP_EXTRA_INTERPRETER_ARGS. For example,
  /// CPPINTEROP_EXTRA_INTERPRETER_ARGS="-mllvm -debug-only=jitcall" to produce
  /// only debug output for jitcall events.
  void EnableDebugOutput(bool value = true);

  ///\returns true if the debugging printouts on stderr are enabled.
  bool IsDebugOutputEnabled();

  ///\returns true if the scope supports aggregate initialization.
  bool IsAggregate(TCppScope_t scope);

  bool IsNamespace(TCppScope_t scope);

  bool IsClass(TCppScope_t scope);
  // See TClingClassInfo::IsLoaded
  bool IsComplete(TCppScope_t scope);

  size_t SizeOf(TCppScope_t scope);
  bool IsBuiltin(TCppType_t type);

  bool IsTemplate(TCppScope_t handle);

  bool IsTemplateSpecialization(TCppScope_t handle);

  bool IsAbstract(TCppType_t klass);

  bool IsEnumScope(TCppScope_t handle);

  bool IsEnumConstant(TCppScope_t handle);

  bool IsEnumType(TCppType_t type);

  /// We assume that smart pointer types define both operator* and
  /// operator->.
  bool IsSmartPtrType(TCppType_t type);

  TCppType_t GetIntegerTypeFromEnumScope(TCppScope_t handle);

  TCppType_t GetIntegerTypeFromEnumType(TCppType_t handle);

  std::vector<TCppScope_t> GetEnumConstants(TCppScope_t scope);

  TCppType_t GetEnumConstantType(TCppScope_t scope);

  TCppIndex_t GetEnumConstantValue(TCppScope_t scope);

  size_t GetSizeOfType(TCppSema_t sema, TCppType_t type);

  bool IsVariable(TCppScope_t scope);

  std::string GetName(TCppType_t klass);

  std::string GetCompleteName(TCppSema_t sema, TCppType_t klass);

  std::string GetQualifiedName(TCppType_t klass);

  std::string GetQualifiedCompleteName(TCppSema_t sema, TCppType_t klass);

  std::vector<TCppScope_t> GetUsingNamespaces(TCppScope_t scope);

  TCppScope_t GetGlobalScope(TCppSema_t sema);

  TCppScope_t GetScope(TCppSema_t sema, const std::string &name,
                       TCppScope_t parent = 0);

  TCppScope_t GetScopeFromCompleteName(TCppSema_t sema,
                                       const std::string &name);

  TCppScope_t GetNamed(TCppSema_t sema, const std::string &name,
                       TCppScope_t parent = nullptr);

  TCppScope_t GetParentScope(TCppScope_t scope);

  TCppScope_t GetScopeFromType(TCppType_t type);

  TCppIndex_t GetNumBases(TCppType_t klass);

  TCppScope_t GetBaseClass(TCppType_t klass, TCppIndex_t ibase);

  bool IsSubclass(TInterp_t interp, TCppScope_t derived, TCppScope_t base);

  int64_t GetBaseClassOffset(TCppSema_t sema, TCppScope_t derived,
                             TCppScope_t base);

  std::vector<TCppFunction_t> GetClassMethods(TCppSema_t sema,
                                              TCppScope_t klass);

  ///\returns if a class has a default constructor.
  bool HasDefaultConstructor(TCppScope_t scope);

  ///\returns the default constructor of a class if any.
  TCppFunction_t GetDefaultConstructor(TCppSema_t sema, TCppScope_t scope);

  ///\returns the class destructor.
  TCppFunction_t GetDestructor(TCppScope_t scope);

  std::vector<TCppFunction_t> GetFunctionsUsingName(TCppSema_t sema,
                                                    TCppScope_t scope,
                                                    const std::string &name);

  TCppType_t GetFunctionReturnType(TCppFunction_t func);

  TCppIndex_t GetFunctionNumArgs(TCppFunction_t func);

  TCppIndex_t GetFunctionRequiredArgs(TCppConstFunction_t func);

  TCppType_t GetFunctionArgType(TCppFunction_t func, TCppIndex_t iarg);

  /// Returns a stringified version of a given function signature in the form:
  /// void N::f(int i, double d, long l = 0, char ch = 'a').
  std::string GetFunctionSignature(TCppFunction_t func);

  /// Returns if a function was marked as \c =delete.
  bool IsFunctionDeleted(TCppConstFunction_t function);

  bool IsTemplatedFunction(TCppFunction_t func);

  bool ExistsFunctionTemplate(TCppSema_t sema, const std::string &name,
                              TCppScope_t parent = 0);

  bool IsMethod(TCppConstFunction_t method);

  bool IsPublicMethod(TCppFunction_t method);

  bool IsProtectedMethod(TCppFunction_t method);

  bool IsPrivateMethod(TCppFunction_t method);

  bool IsConstructor(TCppConstFunction_t method);

  bool IsDestructor(TCppConstFunction_t method);

  bool IsStaticMethod(TCppFunction_t method);

  TCppFuncAddr_t GetFunctionAddress(TInterp_t interp, TCppFunction_t method);

  bool IsVirtualMethod(TCppFunction_t method);

  std::vector<TCppScope_t> GetDatamembers(TCppScope_t scope);

  TCppScope_t LookupDatamember(TCppSema_t sema, const std::string &name,
                               TCppScope_t parent);

  TCppType_t GetVariableType(TCppScope_t var);

  intptr_t GetVariableOffset(TInterp_t interp, TCppScope_t var);

  bool IsPublicVariable(TCppScope_t var);

  bool IsProtectedVariable(TCppScope_t var);

  bool IsPrivateVariable(TCppScope_t var);

  bool IsStaticVariable(TCppScope_t var);

  bool IsConstVariable(TCppScope_t var);

  bool IsRecordType(TCppType_t type);

  bool IsPODType(TCppSema_t sema, TCppType_t type);

  TCppType_t GetUnderlyingType(TCppType_t type);

  std::string GetTypeAsString(TCppType_t type);

  TCppType_t GetCanonicalType(TCppType_t type);

  TCppType_t GetType(TCppSema_t sema, const std::string &type);

  TCppType_t GetComplexType(TCppSema_t sema, TCppType_t element_type);

  TCppType_t GetTypeFromScope(TCppScope_t klass);

  /// Check if a C++ type derives from another.
  bool IsTypeDerivedFrom(TInterp_t interp, TCppType_t derived, TCppType_t base);

  /// Creates a trampoline function by using the interpreter and returns a
  /// uniform interface to call it from compiled code.
  JitCall MakeFunctionCallable(TInterp_t interp, TCppConstFunction_t func);

  /// Checks if a function declared is of const type or not
  bool IsConstMethod(TCppFunction_t method);

  /// Returns the default argument value as string.
  std::string GetFunctionArgDefault(TCppFunction_t func, TCppIndex_t param_index);

  /// Returns the argument name of function as string.
  std::string GetFunctionArgName(TCppFunction_t func, TCppIndex_t param_index);

  /// Creates an instance of the interpreter we need for the various interop
  /// services.
  ///\param[in] Args - the list of arguments for interpreter constructor.
  ///\param[in] CPPINTEROP_EXTRA_INTERPRETER_ARGS - an env variable, if defined,
  ///           adds additional arguments to the interpreter.
  TInterp_t CreateInterpreter(const std::vector<const char *> &Args = {});

  TCppSema_t GetSema(TInterp_t interp);

  void AddSearchPath(TInterp_t interp, const char *dir, bool isUser = true,
                     bool prepend = false);

  /// Returns the resource-dir path.
  const char *GetResourceDir(TInterp_t interp);

  void AddIncludePath(TInterp_t interp, const char *dir);

  TCppIndex_t Declare(TInterp_t interp, const char *code, bool silent = false);

  /// Declares and runs a code snippet in \c code.
  ///\returns 0 on success
  int Process(TInterp_t interp, const char *code);

  /// Declares, runs and returns the execution result as a intptr_t.
  ///\returns the expression results as a intptr_t.
  intptr_t Evaluate(TInterp_t interp, const char *code,
                    bool *HadError = nullptr);

  const std::string LookupLibrary(TInterp_t interp, const char *lib_name);

  bool LoadLibrary(TInterp_t interp, const char *lib_path, bool lookup = true);

  std::string ObjToString(TInterp_t interp, const char *type, void *obj);

  struct TemplateArgInfo {
    TCppScope_t m_Type;
    const char* m_IntegralValue;
    TemplateArgInfo(TCppScope_t type, const char* integral_value = nullptr)
      : m_Type(type), m_IntegralValue(integral_value) {}
  };
  TCppScope_t InstantiateClassTemplate(TInterp_t interp, TCppScope_t tmpl,
                                       TemplateArgInfo *template_args,
                                       size_t template_args_size);

  std::vector<std::string> GetAllCppNames(TCppScope_t scope);

  void DumpScope(TCppScope_t scope);

  namespace DimensionValue {
    enum : long int {
      UNKNOWN_SIZE = -1,
    };
  }

  std::vector<long int> GetDimensions(TCppType_t type);

  /// Allocates memory for a given class.
  TCppObject_t Allocate(TCppScope_t scope);

  /// Deallocates memory for a given class.
  void Deallocate(TCppScope_t scope, TCppObject_t address);

  /// Creates an object of class \c scope and calls its default constructor. If
  /// \c arena is set it uses placement new.
  TCppObject_t Construct(TInterp_t interp, TCppScope_t scope,
                         void *arena = nullptr);

  /// Calls the destructor of object of type \c type. When withFree is true it
  /// calls operator delete/free.
  void Destruct(TInterp_t interp, TCppObject_t This, TCppScope_t type,
                bool withFree = true);
} // end namespace Cpp

#endif // CPPINTEROP_CPPINTEROP_H
