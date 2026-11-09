# Introduction

This document contains the release notes for the language interoperability
library CppInterOp, release 2.0.0. CppInterOp is built on top of
[Clang](http://clang.llvm.org) and [LLVM](http://llvm.org%3E) compiler
infrastructure. Here we describe the status of CppInterOp in some detail,
including major improvements from the previous release and new feature work.
Note that if you are reading this file from a git checkout, this document
applies to the *next* release, not the current one.

CppInterOp exposes API from Clang and LLVM in a backward compatibe way. The API
support downstream tools that utilize interactive C++ by using the compiler as
a service. That is, embed Clang and LLVM as a libraries in their codebases. The
API are designed to be minimalistic and aid non-trivial tasks such as language
interoperability on the fly. In such scenarios CppInterOp can be used to provide
the necessary introspection information to the other side helping the language
cross talk.

## What's New in CppInterOp 2.0.0?

Some of the major new features and improvements to CppInterOp are listed here.
Generic improvements to CppInterOp as a whole or to its underlying
infrastructure are described first.

CppInterOp 2.0.0 is a major release. The public API is now defined once in
TableGen (`lib/CppInterOp/CppInterOp.td`), which generates the C++
declarations, the dispatch table and the C API from the same definitions. The
untyped `void*` handles are replaced by distinct handle types, which is a
source-incompatible change for every client. The release also adds
crash reproducers, a vtable overlay for cross-language overrides, memory
ownership analysis and support for LLVM 22 and 23.

## Breaking Changes

- The opaque handle aliases are replaced by distinct struct types and the old
  names are removed: `TCppScope_t` becomes `Cpp::DeclRef`, `TCppType_t`
  `Cpp::TypeRef`, `TCppFunction_t` `Cpp::FuncRef`, `TCppObject_t`
  `Cpp::ObjectRef` and `TInterp_t` `Cpp::InterpRef`. `DeclRef`, `TypeRef` and
  `FuncRef` also have `Const` variants for read-only parameters. Passing a type where a declaration is
  expected no longer compiles (#1014).
- `TCppIndex_t` parameters and results are now `size_t`, and
  `GetFunctionAddress` returns `void*` instead of `TCppFuncAddr_t`.
- `IsAbstract` takes a declaration (`DeclRef`) instead of a type.
- `Evaluate` returns a `Cpp::Box` that carries the typed result, replacing the
  `intptr_t` return value and the `bool*` out-parameter (#990, #1023).
- `BestOverloadFunctionMatch` has a new required out-parameter that receives
  the candidates when the call is ambiguous, and an `is_operator` flag (#1147).
- `GetEnumConstantValue` returns the value as an `int64_t` bit pattern instead
  of `size_t`, so large and negative values survive on every platform (#1096).
- The C API header moved from `clang-c/CXCppInterOp.h` to
  `CppInterOp/CXCppInterOp.h`, and its wrappers are now generated from
  TableGen (#921, #1025).
- `GetExecutorPID` is removed: upstream LLVM has no equivalent for its
  out-of-process executor (#924).
- The `InstantiateTemplate` overload that takes a pointer and a size is
  deprecated in favour of a new overload that takes a
  `std::vector<TemplateArgInfo>` (#980).

`GetType`, `LoadLibrary` and `GetBaseClassOffset` gained parameters with
defaults, so existing calls still compile.

## External Dependencies

- CppInterOp v2.0.0 supports llvm 21-23.
- Adds support for llvm 22 and 23, and drops llvm 19 and 20.

## Introspection

- Adds type queries for cppyy and numba: `IsIntegerType`, `IsFloatingType`,
  `IsVoidPointerType`, `IsSameType`, `IsTemplateParmType` and
  `IsPackExpansionType` (#913, #1021, #1125).
- Adds `IsOperator`, `IsConversionOperator`, `GetTemplatedDecl`,
  `GetClassTemplateArgs`, `IsFunctionProtoType`, `GetFnTypeSignature` and
  `HasAttribute` (#1002, #1004, #1125, #1126, #1163).
- Adds `IsVisibleName`, a cheap check whether a name is visible from a scope
  (#1130).
- Adds `GetOrForceDefinition` to force the definition of an entity into the
  AST (#810).
- Adds memory and ownership analysis: `IsAllocator`, `IsDeallocator`,
  `GetDeallocType`, `GetAllocationSizeParamIndex`, `GetDeallocationIndexes`
  and `GetOwnershipBehaviour` (#1020, #1041, #1052, #1072, #1099, #1121).
- `GetType` accepts an optional parent scope, `GetNamed` honours
  using-directives and looks into base classes, and function lookup handles
  using-declarations (#976, #977, #1026, #1093).
- Supports C++23 explicit object parameters (#1032).
- `GetBaseClassOffset` takes an optional object address to compute virtual
  base offsets at run time (#1145).

## Just-in-Time Compilation

- Adds `MakeVTableOverlay` and `DestroyVTableOverlay`, which redirect selected
  virtual methods of a live object to a foreign implementation, for example a
  Python override, on both the Itanium and Microsoft ABIs (#1015, #1019).
- `JitCall` handles move-only arguments, function references and moves of
  function pointers (#1022, #1024, #1105).
- The in-process JIT binds weak globals the process already defines, resolves
  glibc `libc_nonshared.a` symbols and works around an ORC emulated-TLS crash
  (#1045, #1073, #1077).
- Out-of-process JIT moves from a patched clang 20 to the upstream clang 22
  interface (#924).
- Supports Clad as an interpreter plugin and Cling's modules mode on Linux
  (#830, #1148).
- CUDA: propagates include paths to the device compiler and adds queries for
  CUDA attributes (#1163, #1164).

## Incremental C++

- `Declare` and `Process` report parse and execution failures through their
  return code (#983, #1129).
- Wrapper and dynamic library state is kept per interpreter instead of in
  function-local statics (#900, b8ae9f6).
- Adds `DiagnosticsEngineRAII`, and opens a transaction or a code-synthesis
  scope around queries that can deserialize declarations (#849, #853, #1128,
  #1131).

## Misc

- Crash reproducers: on a crash CppInterOp prints the active interpreters, and
  with `CPPINTEROP_LOG` set it writes the traced API calls to a compilable
  `cppinterop-reproducer-*.cpp` (#890, #967, #989).
- Adds `GetBuildInfo`, which returns the configuration CppInterOp was built
  with (#987).
- Adds `Cpp::Box` for typed results and `Cpp::Result<T>` with structured
  errors that carry the Clang diagnostic; no existing API returns
  `Result<T>` yet (#1023, #1030, #1034).
- Signal handlers and `llvm_shutdown` are left alone when the client owns the
  interpreter (#891, #905).
- `LoadLibrary` can return the reason a library failed to load (#1101).
- Adds an experimental Bazel build and lean embedder installs (#1039, #1097).
- CI adds sanitizers, ROOT and CppJIT integration builds, Emscripten on
  llvm 23 and macOS coverage.

## Fixed Bugs

- [cppyy#219](https://github.com/compiler-research/cppyy/issues/219):
  `operator==` could not be obtained as an attribute (#996).
- [cppyy#220](https://github.com/compiler-research/cppyy/issues/220):
  protected base class members made public with a using-declaration were not
  callable from the derived class (#995).
- Fixes crashes in `IsAbstract` and `GetClassTemplateInstantiationArgs` for
  classes without a definition (#1087, #1088).
- Fixes `IsStaticMethod` and other method queries for templated methods
  (#981, #1021).
- Fixes errors swallowed by `Declare` when diagnostics were silenced (87012b4).
- Fixes the type of unbounded arrays, lookups of ambiguous function templates
  and wrappers that return template aliases (#997, #1085, #1086).

## Special Kudos

This release wouldn't have happened without the efforts of our contributors,
listed in the form of Firstname Lastname (#contributions):

Vassil Vassilev (97; typed handles, TableGen API and C API, tracing and crash reproducers, Box and Result, VTableOverlay, CI)
Aaron Jomy (52; CUDA, type reflection APIs, Clad plugin, Windows and macOS fixes, CI)
Matthew Barton (20; LLVM 22 support, Emscripten and CI)
Jonas Rembser (15; IsVisibleName, resource-dir and build fixes, wrapper and lookup fixes)
Vipul Cariappa (14; LLVM 23 support, overload resolution, template and JitCall fixes)
Emery Conrad (14; Bazel build, deducing this, in-process JIT symbol resolution)
Kerem Şahin (11; memory and ownership analysis, Cling modules on Linux)
Juan Miguel Carceller (3; virtual base offsets, enum static members)
Janak Shah (1; DiagnosticsEngineRAII)
