//===- CppInterOpAPI.td - Base classes for CppInterOp API -------*- td -*-===//
//
// Part of the compiler-research project, under the Apache License v2.0 with
// LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Base TableGen classes for describing CppInterOp's public API. Each API
// function is a record that inherits from CppInterOpAPI.
//
// cppinterop-tblgen emits a single CppInterOpAPI.inc file containing one
// CPPINTEROP_API_FUNC(...) invocation per function. Each use site defines
// the macro to expand differently (raw pointers, wrappers, map entries, etc).
//
//===----------------------------------------------------------------------===//

/// Semantic kind for an API type — used for future strong-type enforcement.
/// "None" means the argument is a plain C/C++ type with no semantic wrapper.
class APITypeKind<string name> {
  string Name = name;
}
def NoKind     : APITypeKind<"None">;
def ScopeKind  : APITypeKind<"Scope">;
def TypeKind   : APITypeKind<"Type">;
def FuncKind   : APITypeKind<"Function">;
def ObjectKind : APITypeKind<"Object">;
def InterpKind : APITypeKind<"Interp">;

// An argument to an API function.
class Arg<string type, string name, string default_ = ""> {
  string Type = type;
  string Name = name;
  string Default = default_;
  APITypeKind Kind = NoKind;
}

// A typed argument — carries semantic kind for future strong-type generation.
class TypedArg<APITypeKind kind, string type, string name,
               string default_ = ""> : Arg<type, name, default_> {
  let Kind = kind;
}

// Base class for all CppInterOp API functions.
class CppInterOpAPI {
  // The C++ function name as it appears in CppImpl::.
  string CppName = NAME;
  // Return type.
  string ReturnType;
  // Ordered list of arguments.
  list<Arg> Args = [];
  // The name used in the dispatch table. Defaults to CppName. Override for
  // overloaded functions (e.g. GetFunctionAddress_fn).
  string DispatchName = NAME;
  // Semantic kind of the return type.
  APITypeKind ReturnKind = NoKind;
  // Doxygen documentation string (may contain newlines).
  string Doc = "";
}
