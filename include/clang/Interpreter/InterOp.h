//--------------------------------------------------------------------*- C++ -*-
// CLING - the C++ LLVM-based InterpreterG :)
// author:  Vassil Vassilev <vvasilev@cern.ch>
//
// This file is dual-licensed: you can choose to license it under the University
// of Illinois Open Source License or the GNU Lesser General Public License. See
// LICENSE.TXT for details.
//------------------------------------------------------------------------------

#ifndef INTEROP_INTEROP_H
#define INTEROP_INTEROP_H

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

namespace InterOp {
  using TCppIndex_t = size_t;
  using TCppScope_t = void*;
  using TCppType_t = void*;
  using TCppFunction_t = void*;
  using TCppConstFunction_t = const void*;
  using TCppFuncAddr_t = void*;
  using TInterp_t = void*;
  using TCppObject_t = void*;
  /// A class modeling function calls for functions produced by the interpreter
  /// in compiled code. It provides an information if we are calling a standard
  /// function, constructor or destructor.
  class JitCall {
  public:
    friend JitCall MakeFunctionCallable(TCppConstFunction_t);
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

    /// Checks if the passed arguments are valid for the given function.
    bool AreArgumentsValid(void* result, ArgList args, void* self) const;

    /// This function is used for debugging, it reports when the function was
    /// called.
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
    ///\param[in] self - the 'this pointer' of the object.
    // FIXME: Adjust the arguments and their types: args_size can be unsigned;
    // self can go in the end and be nullptr by default; result can be a nullptr
    // by default. These changes should be synchronized with the wrapper if we
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
  /// INTEROP_EXTRA_INTERPRETER_ARGS. For example,
  /// INTEROP_EXTRA_INTERPRETER_ARGS="-mllvm -debug-only=jitcall" to produce
  /// only debug output for jitcall events.
  void EnableDebugOutput(bool value = true);

  ///\returns true if the debugging printouts on stderr are enabled.
  bool IsDebugOutputEnabled();

  /// Checks if the given class represents an aggregate type).
  ///\returns true if \c scope is an array or a C++ tag (as per C++
  ///[dcl.init.aggr]) \returns true if the scope supports aggregate
  /// initialization.
  bool IsAggregate(TCppScope_t scope);

  /// Checks if the scope is a namespace or not.
  bool IsNamespace(TCppScope_t scope);

  /// Checks if the scope is a class or not.
  bool IsClass(TCppScope_t scope);

  // See TClingClassInfo::IsLoaded
  /// Checks if the class definition is present, or not. Performs a
  /// template instantiation if necessary.
  bool IsComplete(TCppScope_t scope);

  size_t SizeOf(TCppScope_t scope);

  /// Checks if it is a "built-in" or a "complex" type.
  bool IsBuiltin(TCppType_t type);

  /// Checks if it is a templated class.
  bool IsTemplate(TCppScope_t handle);

  /// Checks if it is a class template specialization class.
  bool IsTemplateSpecialization(TCppScope_t handle);

  bool IsAbstract(TCppType_t klass);

  /// Checks if it is an enum name (EnumDecl represents an enum name).
  bool IsEnumScope(TCppScope_t handle);

  /// Checks if it is an enum's value (EnumConstantDecl represents
  /// each enum constant that is defined).
  bool IsEnumConstant(TCppScope_t handle);

  /// Checks if the passed value is an enum type or not.
  bool IsEnumType(TCppType_t type);

  /// We assume that smart pointer types define both operator* and
  /// operator->.
  bool IsSmartPtrType(TCppType_t type);

  /// For the given "class", get the integer type that the enum
  /// represents, so that you can store it properly in your specific
  /// language.
  TCppType_t GetIntegerTypeFromEnumScope(TCppScope_t handle);

  /// For the given "type", this function gets the integer type that the enum
  /// represents, so that you can store it properly in your specific
  /// language.
  TCppType_t GetIntegerTypeFromEnumType(TCppType_t handle);

  /// Gets a list of all the enum constants for an enum.
  std::vector<TCppScope_t> GetEnumConstants(TCppScope_t scope);

  /// Gets the enum name when an enum constant is passed.
  TCppType_t GetEnumConstantType(TCppScope_t scope);

  /// Gets the index value (0,1,2, etcetera) of the enum constant
  /// that was passed into this function.
  TCppIndex_t GetEnumConstantValue(TCppScope_t scope);

  /// Gets the size of the "type" that is passed in to this function.
  size_t GetSizeOfType(TCppType_t type);

  /// Checks if the passed value is a variable.
  bool IsVariable(TCppScope_t scope);

  /// Gets the name of any named decl (a class,
  /// namespace, variable, or a function).
  std::string GetName(TCppType_t klass);

  /// This is similar to GetName() function, but besides
  /// the name, it also gets the template arguments.
  std::string GetCompleteName(TCppType_t klass);

  /// Gets the "qualified" name (including the namespace) of any
  /// named decl (a class, namespace, variable, or a function).
  std::string GetQualifiedName(TCppType_t klass);

  /// This is similar to GetQualifiedName() function, but besides
  /// the "qualified" name (including the namespace), it also
  /// gets the template arguments.
  std::string GetQualifiedCompleteName(TCppType_t klass);

  /// Gets the list of namespaces utilized in the supplied scope.
  std::vector<TCppScope_t> GetUsingNamespaces(TCppScope_t scope);

  /// Gets the global scope of the whole C++  instance.
  TCppScope_t GetGlobalScope();

  /// Strips the typedef and returns the underlying class, and if the
  /// underlying decl is not a class it returns the input unchanged.
  TCppScope_t GetUnderlyingScope(TCppScope_t scope);

  /// Gets the namespace or class for the name passed as a parameter,
  /// and if the parent is not passed, then global scope will be assumed.
  TCppScope_t GetScope(const std::string &name, TCppScope_t parent = 0);

  /// When the namespace is known, then the parent doesn't need
  /// to be specified. This will probably be phased-out in
  /// future versions of the interop library.
  TCppScope_t GetScopeFromCompleteName(const std::string &name);

  /// This function performs a lookup within the specified parent,
  /// a specific named entity (functions, enums, etcetera).
  TCppScope_t GetNamed(const std::string &name,
                       TCppScope_t parent = nullptr);

  /// Gets the parent of the scope that is passed as a parameter.
  TCppScope_t GetParentScope(TCppScope_t scope);

  /// Gets the scope of the type that is passed as a parameter.
  TCppScope_t GetScopeFromType(TCppType_t type);

  /// Gets the number of Base Classes for the Derived Class that
  /// is passed as a parameter.
  TCppIndex_t GetNumBases(TCppType_t klass);

  /// Gets a specific Base Class using its index. Typically GetNumBases()
  /// is used to get the number of Base Classes, and then that number
  /// can be used to iterate through the index value to get each specific
  /// base class.
  TCppScope_t GetBaseClass(TCppType_t klass, TCppIndex_t ibase);

  /// Checks if the supplied Derived Class is a sub-class of the
  /// provided Base Class.
  bool IsSubclass(TCppScope_t derived, TCppScope_t base);

  /// Each base has its own offset in a Derived Class. This offset can be
  /// used to get to the Base Class fields.
  int64_t GetBaseClassOffset(TCppScope_t derived, TCppScope_t base);

  /// Gets a list of all the Methods that are in the Class that is
  /// supplied as a parameter.
  std::vector<TCppFunction_t> GetClassMethods(TCppScope_t klass);

  ///\returns if a class has a default constructor.
  bool HasDefaultConstructor(TCppScope_t scope);

  ///\returns the default constructor of a class, if any.
  TCppFunction_t GetDefaultConstructor(TCppScope_t scope);

  ///\returns the class destructor, if any.
  TCppFunction_t GetDestructor(TCppScope_t scope);

  /// Looks up all the functions that have the name that is
  /// passed as a parameter in this function.
  std::vector<TCppFunction_t> GetFunctionsUsingName(
        TCppScope_t scope, const std::string& name);

  /// Gets the return type of the provided function.
  TCppType_t GetFunctionReturnType(TCppFunction_t func);

  /// Gets the number of Arguments for the provided function.
  TCppIndex_t GetFunctionNumArgs(TCppFunction_t func);

  /// Gets the number of Required Arguments for the provided function.
  TCppIndex_t GetFunctionRequiredArgs(TCppConstFunction_t func);

  /// For each Argument of a function, you can get the Argument Type
  /// by providing the Argument Index, based on the number of arguments
  /// from the GetFunctionNumArgs() function.
  TCppType_t GetFunctionArgType(TCppFunction_t func, TCppIndex_t iarg);

  ///\returns a stringified version of a given function signature in the form:
  /// void N::f(int i, double d, long l = 0, char ch = 'a').
  std::string GetFunctionSignature(TCppFunction_t func);

  ///\returns if a function was marked as \c =delete.
  bool IsFunctionDeleted(TCppConstFunction_t function);

  bool IsTemplatedFunction(TCppFunction_t func);

  /// This function performs a lookup to check if there is a
  /// templated function of that type.
  bool ExistsFunctionTemplate(const std::string& name,
          TCppScope_t parent = 0);

  /// Checks if the provided parameter is a method.
  bool IsMethod(TCppConstFunction_t method);

  /// Checks if the provided parameter is a 'Public' method.
  bool IsPublicMethod(TCppFunction_t method);

  /// Checks if the provided parameter is a 'Protected' method.
  bool IsProtectedMethod(TCppFunction_t method);

  /// Checks if the provided parameter is a 'Private' method.
  bool IsPrivateMethod(TCppFunction_t method);

  /// Checks if the provided parameter is a Constructor.
  bool IsConstructor(TCppConstFunction_t method);

  /// Checks if the provided parameter is a Destructor.
  bool IsDestructor(TCppConstFunction_t method);

  /// Checks if the provided parameter is a 'Static' method.
  bool IsStaticMethod(TCppFunction_t method);

  /// Gets the address of the function to be able to call it.
  TCppFuncAddr_t GetFunctionAddress(TCppFunction_t method);

  /// Checks if the provided parameter is a 'Virtual' method.
  bool IsVirtualMethod(TCppFunction_t method);

  /// Gets all the Fields/Data Members of a Class. For now, it
  /// only gets non-static data members but in a future update,
  /// it may support getting static data members as well.
  std::vector<TCppScope_t> GetDatamembers(TCppScope_t scope);

  /// This is a Lookup function to be used specifically for data members.
  TCppScope_t LookupDatamember(const std::string& name, TCppScope_t parent);

  /// Gets the type of the variable that is passed as a parameter.
  TCppType_t GetVariableType(TCppScope_t var);

  /// Gets the address of the variable, you can use it to get the
  /// value stored in the variable.
  intptr_t GetVariableOffset(TCppScope_t var);

  /// Checks if the provided variable is a 'Public' variable.
  bool IsPublicVariable(TCppScope_t var);

  /// Checks if the provided variable is a 'Protected' variable.
  bool IsProtectedVariable(TCppScope_t var);

  /// Checks if the provided variable is a 'Private' variable.
  bool IsPrivateVariable(TCppScope_t var);

  /// Checks if the provided variable is a 'Static' variable.
  bool IsStaticVariable(TCppScope_t var);

  /// Checks if the provided variable is a 'Constant' variable.
  bool IsConstVariable(TCppScope_t var);

  /// Checks if the provided parameter is a Record (struct).
  bool IsRecordType(TCppType_t type);

  /// Checks if the provided parameter is a Plain Old Data Type (POD).
  bool IsPODType(TCppType_t type);

  /// Gets the pure, Underlying Type (as opposed to the Using Type).
  TCppType_t GetUnderlyingType(TCppType_t type);

  /// Gets the Type (passed as a parameter) as a String value.
  std::string GetTypeAsString(TCppType_t type);

  /// Gets the Canonical Type string from the std string. A canonical type
  /// is the type with any typedef names, syntactic sugars or modifiers stripped
  /// out of it.
  TCppType_t GetCanonicalType(TCppType_t type);

  /// Used to either get the built-in type of the provided string, or
  /// use the name to lookup the actual type.
  TCppType_t GetType(const std::string &type);

  ///\returns the complex of the provided type.
  TCppType_t GetComplexType(TCppType_t element_type);

  /// This will convert a class into its type, so for example, you can
  /// use it to declare variables in it.
  TCppType_t GetTypeFromScope(TCppScope_t klass);

  /// Checks if a C++ type derives from another.
  bool IsTypeDerivedFrom(TCppType_t derived, TCppType_t base);

  /// Creates a trampoline function by using the interpreter and returns a
  /// uniform interface to call it from compiled code.
  JitCall MakeFunctionCallable(TCppConstFunction_t func);

  /// Checks if a function declared is of const type or not.
  bool IsConstMethod(TCppFunction_t method);

  ///\returns the default argument value as string.
  std::string GetFunctionArgDefault(TCppFunction_t func, TCppIndex_t param_index);

  ///\returns the argument name of function as string.
  std::string GetFunctionArgName(TCppFunction_t func, TCppIndex_t param_index);

  /// Creates an instance of the interpreter we need for the various interop
  /// services.
  ///\param[in] Args - the list of arguments for interpreter constructor.
  ///\param[in] INTEROP_EXTRA_INTERPRETER_ARGS - an env variable, if defined,
  ///           adds additional arguments to the interpreter.
  extern "C" TInterp_t CreateInterpreter(const std::vector<const char*> &Args = {});

  /// Checks which Interpreter backend was InterOp library built with (Cling,
  /// Clang-REPL, etcetera). In practice, the selected interpreter should not
  /// matter, since the library will function in the same way.
  ///\returns the current interpreter instance, if any.
  TInterp_t GetInterpreter();

  /// Adds a Search Path for the Interpreter to get the libraries.
  void AddSearchPath(const char *dir, bool isUser = true, bool prepend = false);

  /// Returns the resource-dir path (for headers).
  const char* GetResourceDir();

  /// Secondary search path for headers, if not found using the
  /// GetResourceDir() function.
  void AddIncludePath(const char *dir);

  /// Only Declares a code snippet in \c code and does not execute it.
  TCppIndex_t Declare(const char *code, bool silent = false);

  /// Declares and executes a code snippet in \c code.
  ///\returns 0 on success
  int Process(const char *code);

  /// Declares, executes and returns the execution result as a intptr_t.
  ///\returns the expression results as a intptr_t.
  intptr_t Evaluate(const char *code, bool *HadError = nullptr);

  /// Looks up the library if access is enabled.
  ///\returns the path to the library.
  const std::string LookupLibrary(const char *lib_name);

  /// Loads the library based on the path returned by the LookupLibrary()
  /// function.
  bool LoadLibrary(const char *lib_path, bool lookup = true);

  /// Tries to load provided objects in a string format (prettyprint).
  std::string ObjToString(const char *type, void *obj);

  struct TemplateArgInfo {
    TCppScope_t m_Type;
    const char* m_IntegralValue;
    TemplateArgInfo(TCppScope_t type, const char* integral_value = nullptr)
      : m_Type(type), m_IntegralValue(integral_value) {}
  };
  TCppScope_t InstantiateClassTemplate(TCppScope_t tmpl,
                                       TemplateArgInfo* template_args,
                                       size_t template_args_size);

  std::vector<std::string> GetAllCppNames(TCppScope_t scope);

  void DumpScope(TCppScope_t scope);

  namespace DimensionValue {
    enum : long int {
      UNKNOWN_SIZE = -1,
    };
  }

  /// Gets the size/dimensions of a multi-dimension array.
  std::vector<long int> GetDimensions(TCppType_t type);

  /// Allocates memory for a given class.
  TCppObject_t Allocate(TCppScope_t scope);

  /// Deallocates memory for a given class.
  void Deallocate(TCppScope_t scope, TCppObject_t address);

  /// Creates an object of class \c scope and calls its default constructor. If
  /// \c arena is set it uses placement new.
  TCppObject_t Construct(TCppScope_t scope,
                         void* arena = nullptr);

  /// Calls the destructor of object of type \c type. When withFree is true it
  /// calls operator delete/free.
  void Destruct(TCppObject_t This, TCppScope_t type,
                bool withFree = true);
} // end namespace InterOp

#endif // INTEROP_INTEROP_H
