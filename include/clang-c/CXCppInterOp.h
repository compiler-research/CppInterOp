// NOLINTBEGIN()
#ifndef LLVM_CLANG_C_CXCPPINTEROP_H
#define LLVM_CLANG_C_CXCPPINTEROP_H

#include "clang-c/CXErrorCode.h"
#include "clang-c/CXString.h"
#include "clang-c/ExternC.h"
#include "clang-c/Index.h"
#include "clang-c/Platform.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

LLVM_CLANG_C_EXTERN_C_BEGIN

/**
 * \defgroup CPPINTEROP_INTERPRETER_MANIP Interpreter manipulations
 *
 * @{
 */

/**
 * An opaque pointer representing an interpreter context.
 */
typedef struct CXInterpreterImpl* CXInterpreter;

/**
 * Create a Clang interpreter instance from the given arguments.
 *
 * \param argv The arguments that would be passed to the interpreter.
 *
 * \param argc The number of arguments in \c argv.
 *
 * \returns a \c CXInterpreter.
 */
CXInterpreter clang_createInterpreter(const char* const* argv, int argc);

typedef void* TInterp_t;

/**
 * Bridge between C API and C++ API.
 *
 * \returns a \c CXInterpreter.
 */
CXInterpreter clang_createInterpreterFromPtr(TInterp_t I);

/**
 * Returns a \c TInterp_t.
 */
TInterp_t clang_interpreter_getInterpreterAsPtr(CXInterpreter I);

/**
 * Similar to \c clang_interpreter_getInterpreterAsPtr() but it takes the
 * ownership.
 */
TInterp_t clang_interpreter_takeInterpreterAsPtr(CXInterpreter I);

/**
 * Dispose of the given interpreter context.
 */
void clang_interpreter_dispose(CXInterpreter I);

/**
 * Describes the return result of the different routines that do the incremental
 * compilation.
 */
typedef enum {
  /**
   * The compilation was successful.
   */
  CXInterpreter_Success = 0,
  /**
   * The compilation failed.
   */
  CXInterpreter_Failure = 1,
  /**
   * More more input is expected.
   */
  CXInterpreter_MoreInputExpected = 2,
} CXInterpreter_CompilationResult;

/**
 * Add a search path to the interpreter.
 *
 * \param I The interpreter.
 *
 * \param dir The directory to add.
 *
 * \param isUser Whether the directory is a user directory.
 *
 * \param prepend Whether to prepend the directory to the search path.
 */
void clang_interpreter_addSearchPath(CXInterpreter I, const char* dir,
                                     bool isUser, bool prepend);

/**
 * Get the resource directory.
 */
const char* clang_interpreter_getResourceDir(CXInterpreter I);

/**
 * Add an include path.
 *
 * \param I The interpreter.
 *
 * \param dir The directory to add.
 */
void clang_interpreter_addIncludePath(CXInterpreter I, const char* dir);

/**
 * Declares a code snippet in \c code and does not execute it.
 *
 * \param I The interpreter.
 *
 * \param code The code snippet to declare.
 *
 * \param silent Whether to suppress the diagnostics or not
 *
 * \returns a \c CXErrorCode.
 */
enum CXErrorCode clang_interpreter_declare(CXInterpreter I, const char* code,
                                           bool silent);

/**
 * Declares and executes a code snippet in \c code.
 *
 * \param I The interpreter.
 *
 * \param code The code snippet to execute.
 *
 * \returns a \c CXErrorCode.
 */
enum CXErrorCode clang_interpreter_process(CXInterpreter I, const char* code);

/**
 * An opaque pointer representing a lightweight struct that is used for carrying
 * execution results.
 */
typedef void* CXValue;

/**
 * Create a CXValue.
 *
 * \returns a \c CXValue.
 */
CXValue clang_createValue();

/**
 * Dispose of the given CXValue.
 *
 * \param V The CXValue to dispose.
 */
void clang_value_dispose(CXValue V);

/**
 * Declares, executes and stores the execution result to \c V.
 *
 * \param[in] I The interpreter.
 *
 * \param[in] code The code snippet to evaluate.
 *
 * \param[out] V The value to store the execution result.
 *
 * \returns a \c CXErrorCode.
 */
enum CXErrorCode clang_interpreter_evaluate(CXInterpreter I, const char* code,
                                            CXValue V);

/**
 * Looks up the library if access is enabled.
 *
 * \param I The interpreter.
 *
 * \param lib_name The name of the library to lookup.
 *
 * \returns the path to the library.
 */
CXString clang_interpreter_lookupLibrary(CXInterpreter I, const char* lib_name);

/**
 * Finds \c lib_stem considering the list of search paths and loads it by
 * calling dlopen.
 *
 * \param I The interpreter.
 *
 * \param lib_stem The stem of the library to load.
 *
 * \param lookup Whether to lookup the library or not.
 *
 * \returns a \c CXInterpreter_CompilationResult.
 */
CXInterpreter_CompilationResult
clang_interpreter_loadLibrary(CXInterpreter I, const char* lib_stem,
                              bool lookup);

/**
 * Finds \c lib_stem considering the list of search paths and unloads it by
 * calling dlclose.
 *
 * \param I The interpreter.
 *
 * \param lib_stem The stem of the library to unload.
 */
void clang_interpreter_unloadLibrary(CXInterpreter I, const char* lib_stem);

/**
 * Scans all libraries on the library search path for a given potentially
 * mangled symbol name.
 *
 * \param I The interpreter.
 *
 * \param mangled_name The mangled name of the symbol to search for.
 *
 * \param search_system Whether to search the system paths or not.
 *
 * \returns the path to the first library that contains the symbol definition.
 */
CXString clang_interpreter_searchLibrariesForSymbol(CXInterpreter I,
                                                    const char* mangled_name,
                                                    bool search_system);

/**
 * Inserts or replaces a symbol in the JIT with the one provided. This is useful
 * for providing our own implementations of facilities such as printf.
 *
 * \param I The interpreter.
 *
 * \param linker_mangled_name The name of the symbol to be inserted or replaced.
 *
 * \param address The new address of the symbol.
 *
 * \returns true on failure.
 */
bool clang_interpreter_insertOrReplaceJitSymbol(CXInterpreter I,
                                                const char* linker_mangled_name,
                                                uint64_t address);

typedef void* CXFuncAddr;

/**
 * Get the function address from the given mangled name.
 *
 * \param I The interpreter.
 *
 * \param mangled_name The mangled name of the function.
 *
 * \returns the address of the function given its potentially mangled name.
 */
CXFuncAddr
clang_interpreter_getFunctionAddressFromMangledName(CXInterpreter I,
                                                    const char* mangled_name);

/**
 * @}
 */

/**
 * \defgroup CPPINTEROP_TYPE_MANIP Type manipulations
 *
 * @{
 */

/**
 * An opaque pointer representing a type.
 */
typedef struct {
  enum CXTypeKind kind;
  void* data;
  const void* meta;
} CXQualType;

/**
 * Checks if it is a "built-in" or a "complex" type.
 */
bool clang_qualtype_isBuiltin(CXQualType type);

/**
 * Checks if the passed value is an enum type or not.
 */
bool clang_qualtype_isEnumType(CXQualType type);

/**
 * We assume that smart pointer types define both operator* and operator->.
 */
bool clang_qualtype_isSmartPtrType(CXQualType type);

/**
 * Checks if the provided parameter is a Record (struct).
 */
bool clang_scope_isRecordType(CXQualType type);

/**
 * Checks if the provided parameter is a Plain Old Data Type (POD).
 */
bool clang_scope_isPODType(CXQualType type);

/**
 * For the given "type", this function gets the integer type that the enum
 * represents, so that you can store it properly in your specific language.
 */
CXQualType clang_qualtype_getIntegerTypeFromEnumType(CXQualType type);

/**
 * Gets the pure, Underlying Type (as opposed to the Using Type).
 */
CXQualType clang_qualtype_getUnderlyingType(CXQualType type);

/**
 * Gets the Type (passed as a parameter) as a String value.
 */
CXString clang_qualtype_getTypeAsString(CXQualType type);

/**
 * Gets the Canonical Type string from the std string. A canonical type
 * is the type with any typedef names, syntactic sugars or modifiers stripped
 * out of it.
 */
CXQualType clang_qualtype_getCanonicalType(CXQualType type);

/**
 * Used to either get the built-in type of the provided string, or
 * use the name to lookup the actual type.
 */
CXQualType clang_qualtype_getType(CXInterpreter I, const char* name);

/**
 * Returns the complex of the provided type.
 */
CXQualType clang_qualtype_getComplexType(CXQualType eltype);

/**
 * Gets the size of the "type" that is passed in to this function.
 */
size_t clang_qualtype_getSizeOfType(CXQualType type);

/**
 * Checks if a C++ type derives from another.
 */
bool clang_qualtype_isTypeDerivedFrom(CXQualType derived, CXQualType base);

/**
 * @}
 */

/**
 * \defgroup CPPINTEROP_SCOPE_MANIP Scope manipulations
 *
 * @{
 */

/**
 * Describes the kind of entity that a scope refers to.
 */
enum CXScopeKind {
  CXScope_Unexposed = 0,
  CXScope_Invalid = 1,
  /** The global scope. */
  CXScope_Global = 2,
  /** Namespaces. */
  CXScope_Namespace = 3,
  /** Function, methods, constructor etc. */
  CXScope_Function = 4,
  /** Variables. */
  CXScope_Variable = 5,
  /** Enum Constants. */
  CXScope_EnumConstant = 6,
  /** Fields. */
  CXScope_Field = 7,
  // reserved for future use
};

/**
 * An opaque pointer representing a variable, typedef, function, struct, etc.
 */
typedef struct {
  enum CXScopeKind kind;
  void* data;
  const void* meta;
} CXScope;

// for debugging purposes
void clang_scope_dump(CXScope S);

/**
 * This will convert a class into its type, so for example, you can use it to
 * declare variables in it.
 */
CXQualType clang_scope_getTypeFromScope(CXScope S);

/**
 * Checks if the given class represents an aggregate type.
 *
 * \returns true if the \c scope supports aggregate initialization.
 */
bool clang_scope_isAggregate(CXScope S);

/**
 * Checks if the scope is a namespace or not.
 */
bool clang_scope_isNamespace(CXScope S);

/**
 * Checks if the scope is a class or not.
 */
bool clang_scope_isClass(CXScope S);

/**
 * Checks if the class definition is present, or not. Performs a template
 * instantiation if necessary.
 */
bool clang_scope_isComplete(CXScope S);

/**
 * Get the size of the given type.
 */
size_t clang_scope_sizeOf(CXScope S);

/**
 * Checks if it is a templated class.
 */
bool clang_scope_isTemplate(CXScope S);

/**
 * Checks if it is a class template specialization class.
 */
bool clang_scope_isTemplateSpecialization(CXScope S);

/**
 * Checks if \c handle introduces a typedef name via \c typedef or \c using.
 */
bool clang_scope_isTypedefed(CXScope S);

bool clang_scope_isAbstract(CXScope S);

/**
 * Checks if it is an enum name (EnumDecl represents an enum name).
 */
bool clang_scope_isEnumScope(CXScope S);

/**
 * Checks if it is an enum's value (EnumConstantDecl represents each enum
 * constant that is defined).
 */
bool clang_scope_isEnumConstant(CXScope S);

/**
 * Extracts enum declarations from a specified scope and stores them in vector
 */
CXStringSet* clang_scope_getEnums(CXScope S);

/**
 * For the given "class", get the integer type that the enum represents, so that
 * you can store it properly in your specific language.
 */
CXQualType clang_scope_getIntegerTypeFromEnumScope(CXScope S);

typedef struct {
  CXScope* Scopes;
  size_t Count;
} CXScopeSet;

/**
 * Free the given scope set.
 */
void clang_disposeScopeSet(CXScopeSet* set);

/**
 * Gets a list of all the enum constants for an enum.
 */
CXScopeSet* clang_scope_getEnumConstants(CXScope S);

/**
 * Gets the enum name when an enum constant is passed.
 */
CXQualType clang_scope_getEnumConstantType(CXScope S);

/**
 * Gets the index value (0,1,2, etcetera) of the enum constant that was passed
 * into this function.
 */
size_t clang_scope_getEnumConstantValue(CXScope S);

/**
 * Checks if the passed value is a variable.
 */
bool clang_scope_isVariable(CXScope S);

/**
 * Gets the name of any named decl (a class, namespace, variable, or a
 * function).
 */
CXString clang_scope_getName(CXScope S);

/**
 * This is similar to clang_scope_getName() function, but besides the name, it
 * also gets the template arguments.
 */
CXString clang_scope_getCompleteName(CXScope S);

/**
 * Gets the "qualified" name (including the namespace) of any
 * named decl (a class, namespace, variable, or a function).
 */
CXString clang_scope_getQualifiedName(CXScope S);

/**
 * This is similar to clang_scope_getQualifiedName() function, but besides
 * the "qualified" name (including the namespace), it also gets the template
 * arguments.
 */
CXString clang_scope_getQualifiedCompleteName(CXScope S);

/**
 * Gets the list of namespaces utilized in the supplied scope.
 */
CXScopeSet* clang_scope_getUsingNamespaces(CXScope S);

/**
 * Gets the global scope of the whole interpreter instance.
 */
CXScope clang_scope_getGlobalScope(CXInterpreter I);

/**
 * Strips the typedef and returns the underlying class, and if the underlying
 * decl is not a class it returns the input unchanged.
 */
CXScope clang_scope_getUnderlyingScope(CXScope S);

/**
 * Gets the namespace or class (by stripping typedefs) for the name passed as a
 * parameter. \c parent is mandatory, the global scope should be used as
 * the default value.
 */
CXScope clang_scope_getScope(const char* name, CXScope parent);

/**
 *  This function performs a lookup within the specified parent,
 * a specific named entity (functions, enums, etcetera).
 */
CXScope clang_scope_getNamed(const char* name, CXScope parent);

/**
 * Gets the parent of the scope that is passed as a parameter.
 */
CXScope clang_scope_getParentScope(CXScope parent);

/**
 * Gets the scope of the type that is passed as a parameter.
 */
CXScope clang_scope_getScopeFromType(CXQualType type);

/**
 * Gets the number of Base Classes for the Derived Class that
 * is passed as a parameter.
 */
size_t clang_scope_getNumBases(CXScope S);

/**
 * Gets a specific Base Class using its index. Typically
 * clang_scope_getNumBases() is used to get the number of Base Classes, and then
 * that number can be used to iterate through the index value to get each
 * specific base class.
 */
CXScope clang_scope_getBaseClass(CXScope S, size_t ibase);

/**
 * Checks if the supplied Derived Class is a sub-class of the provided Base
 * Class.
 */
bool clang_scope_isSubclass(CXScope derived, CXScope base);

/**
 * Each base has its own offset in a Derived Class. This offset can be
 * used to get to the Base Class fields.
 */
int64_t clang_scope_getBaseClassOffset(CXScope derived, CXScope base);

/**
 * Gets a list of all the Methods that are in the Class that is supplied as a
 * parameter.
 */
CXScopeSet* clang_scope_getClassMethods(CXScope S);

/**
 * Template function pointer list to add proxies for
 * un-instantiated/non-overloaded templated methods.
 */
CXScopeSet* clang_scope_getFunctionTemplatedDecls(CXScope S);

/**
 * Checks if a class has a default constructor.
 */
bool clang_scope_hasDefaultConstructor(CXScope S);

/**
 * Returns the default constructor of a class, if any.
 */
CXScope clang_scope_getDefaultConstructor(CXScope S);

/**
 * Returns the class destructor, if any.
 */
CXScope clang_scope_getDestructor(CXScope S);

/**
 * Looks up all the functions that have the name that is passed as a parameter
 * in this function.
 */
CXScopeSet* clang_scope_getFunctionsUsingName(CXScope S, const char* name);

/**
 * Gets the return type of the provided function.
 */
CXQualType clang_scope_getFunctionReturnType(CXScope func);

/**
 * Gets the number of Arguments for the provided function.
 */
size_t clang_scope_getFunctionNumArgs(CXScope func);

/**
 * Gets the number of Required Arguments for the provided function.
 */
size_t clang_scope_getFunctionRequiredArgs(CXScope func);

/**
 * For each Argument of a function, you can get the Argument Type by providing
 * the Argument Index, based on the number of arguments from the
 * clang_scope_getFunctionNumArgs() function.
 */
CXQualType clang_scope_getFunctionArgType(CXScope func, size_t iarg);

/**
 * Returns a stringified version of a given function signature in the form:
 * void N::f(int i, double d, long l = 0, char ch = 'a').
 */
CXString clang_scope_getFunctionSignature(CXScope func);

/**
 * Checks if a function was marked as \c =delete.
 */
bool clang_scope_isFunctionDeleted(CXScope func);

/**
 * Checks if a function is a templated function.
 */
bool clang_scope_isTemplatedFunction(CXScope func);

/**
 * This function performs a lookup to check if there is a templated function of
 * that type. \c parent is mandatory, the global scope should be used as the
 * default value.
 */
bool clang_scope_existsFunctionTemplate(const char* name, CXScope parent);

/**
 * Gets a list of all the Templated Methods that are in the Class that is
 * supplied as a parameter.
 */
CXScopeSet* clang_scope_getClassTemplatedMethods(const char* name,
                                                 CXScope parent);

/**
 * Checks if the provided parameter is a method.
 */
bool clang_scope_isMethod(CXScope method);

/**
 * Checks if the provided parameter is a 'Public' method.
 */
bool clang_scope_isPublicMethod(CXScope method);

/**
 * Checks if the provided parameter is a 'Protected' method.
 */
bool clang_scope_isProtectedMethod(CXScope method);

/**
 * Checks if the provided parameter is a 'Private' method.
 */
bool clang_scope_isPrivateMethod(CXScope method);

/**
 * Checks if the provided parameter is a Constructor.
 */
bool clang_scope_isConstructor(CXScope method);

/**
 * Checks if the provided parameter is a Destructor.
 */
bool clang_scope_isDestructor(CXScope method);

/**
 * Checks if the provided parameter is a 'Static' method.
 */
bool clang_scope_isStaticMethod(CXScope method);

/**
 * Returns the address of the function given its function declaration.
 */
CXFuncAddr clang_scope_getFunctionAddress(CXScope method);

/**
 * Checks if the provided parameter is a 'Virtual' method.
 */
bool clang_scope_isVirtualMethod(CXScope method);

/**
 * Checks if a function declared is of const type or not.
 */
bool clang_scope_isConstMethod(CXScope method);

/**
 * Returns the default argument value as string.
 */
CXString clang_scope_getFunctionArgDefault(CXScope func, size_t param_index);

/**
 * Returns the argument name of function as string.
 */
CXString clang_scope_getFunctionArgName(CXScope func, size_t param_index);

typedef struct {
  void* Type;
  const char* IntegralValue;
} CXTemplateArgInfo;

/**
 * Builds a template instantiation for a given templated declaration.
 * Offers a single interface for instantiation of class, function and variable
 * templates.
 *
 * \param[in] tmpl The uninstantiated template class/function.
 *
 * \param[in] template_args The pointer to vector of template arguments stored
 * in the \c TemplateArgInfo struct
 *
 * \param[in] template_args_size The size of the vector of template arguments
 * passed as \c template_args
 *
 * \returns a \c CXScope representing the instantiated templated
 * class/function/variable.
 */
CXScope clang_scope_instantiateTemplate(CXScope tmpl,
                                        CXTemplateArgInfo* template_args,
                                        size_t template_args_size);

/**
 * Gets all the Fields/Data Members of a Class. For now, it only gets non-static
 * data members but in a future update, it may support getting static data
 * members as well.
 */
CXScopeSet* clang_scope_getDatamembers(CXScope S);

/**
 * This is a Lookup function to be used specifically for data members.
 * \c parent is mandatory, the global scope should be used as the default value.
 */
CXScope clang_scope_lookupDatamember(const char* name, CXScope parent);

/**
 * Gets the type of the variable that is passed as a parameter.
 */
CXQualType clang_scope_getVariableType(CXScope var);

/**
 * Gets the address of the variable, you can use it to get the value stored in
 * the variable.
 */
intptr_t clang_scope_getVariableOffset(CXScope var);

/**
 * Checks if the provided variable is a 'Public' variable.
 */
bool clang_scope_isPublicVariable(CXScope var);

/**
 * Checks if the provided variable is a 'Protected' variable.
 */
bool clang_scope_isProtectedVariable(CXScope var);

/**
 * Checks if the provided variable is a 'Private' variable.
 */
bool clang_scope_isPrivateVariable(CXScope var);

/**
 * Checks if the provided variable is a 'Static' variable.
 */
bool clang_scope_isStaticVariable(CXScope var);

/**
 * Checks if the provided variable is a 'Const' variable.
 */
bool clang_scope_isConstVariable(CXScope var);

/**
 * @}
 */

/**
 * \defgroup CPPINTEROP_OBJECT_MANIP Object manipulations
 *
 * @{
 */

/**
 * An opaque pointer representing the object of a given type (\c CXScope).
 */
typedef void* CXObject;

/**
 * Allocates memory for the given type.
 */
CXObject clang_allocate(CXScope S);

/**
 * Deallocates memory for a given class.
 */
void clang_deallocate(CXObject address);

/**
 * Creates an object of class \c scope and calls its default constructor. If \c
 * arena is set it uses placement new.
 */
CXObject clang_construct(CXScope scope, void* arena);

/**
 * Calls the destructor of object of type \c type. When withFree is true it
 * calls operator delete/free.
 *
 * \param I The interpreter.
 *
 * \param This The object to destruct.
 *
 * \param type The type of the object.
 *
 * \param withFree Whether to call operator delete/free or not.
 */
void clang_destruct(CXObject This, CXScope S, bool withFree);

/**
 * @}
 */

/**
 * \defgroup CPPINTEROP_JITCALL_MANIP JitCall manipulations
 *
 * @{
 */

/**
 * An opaque pointer representing CppInterOp's JitCall, a class modeling
 * function calls for functions produced by the interpreter in compiled code.
 */
typedef struct CXJitCallImpl* CXJitCall;

/**
 * Dispose of the given CXJitCall.
 *
 * \param J The CXJitCall to dispose.
 */
void clang_jitcall_dispose(CXJitCall J);

/**
 * The kind of the JitCall.
 */
typedef enum {
  CXJitCall_Unknown = 0,
  CXJitCall_GenericCall = 1,
  CXJitCall_DestructorCall = 2,
} CXJitCallKind;

/**
 * Get the kind of the given CXJitCall.
 *
 * \param J The CXJitCall.
 *
 * \returns the kind of the given CXJitCall.
 */
CXJitCallKind clang_jitcall_getKind(CXJitCall J);

/**
 * Check if the given CXJitCall is valid.
 *
 * \param J The CXJitCall.
 *
 * \returns true if the given CXJitCall is valid.
 */
bool clang_jitcall_isValid(CXJitCall J);

typedef struct {
  void** data;
  size_t numArgs;
} CXJitCallArgList;

/**
 * Makes a call to a generic function or method.
 *
 * \param[in] J The CXJitCall.
 *
 * \param[in] result The location where the return result will be placed.
 *
 * \param[in] args The arguments to pass to the invocation.
 *
 * \param[in] self The 'this pointer' of the object.
 */
void clang_jitcall_invoke(CXJitCall J, void* result, CXJitCallArgList args,
                          void* self);

/**
 * Creates a trampoline function by using the interpreter and returns a uniform
 * interface to call it from compiled code.
 *
 * \returns a \c CXJitCall.
 */
CXJitCall clang_jitcall_makeFunctionCallable(CXScope func);

/**
 * @}
 */

LLVM_CLANG_C_EXTERN_C_END

#endif // LLVM_CLANG_C_CXCPPINTEROP_H
       // NOLINTEND()