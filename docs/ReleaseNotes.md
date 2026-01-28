# Introduction

This document contains the release notes for the language interoperability
library CppInterOp, release 1.8.0. CppInterOp is built on top of
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

## What's New in CppInterOp 1.8.0?

Some of the major new features and improvements to CppInterOp are listed here.
Generic improvements to CppInterOp as a whole or to its underlying
infrastructure are described first.

This release focuses on improving runtime introspection, extending the JIT and
incremental compilation capabilities, introducing a new API dispatch mechanism,
and delivering numerous robustness and correctness fixes across the interpreter
and code generation layers.

## External Dependencies

- CppInterOp now works with:
  - llvm21


## Introspection

- Adds `GetDoxygenComment()` to extract Doxygen documentation comments from
  C++ declarations.
- Adds `GetSpellingFromOperator` and `GetOperatorFromSpelling` helpers.
- Adds `GetPointerType` and improved `GetReferencedType` with rvalue support.
- Adds `GetReferenceValueKind`, replacing `IsLValueReferenceType` and
  `IsRValueReferenceType`.
- Adds type qualifier utilities: `HasTypeQualifier`, `RemoveTypeQualifier`, and
  `AddTypeQualifier`.
- Improved type printing and reflection:
  - `GetCompleteName` now includes template parameters.
  - Fixed handling of default arguments and return types in templated functions.
- Adds `IsLambdaClass` and multiple fixes for templated and forward-declared
  entities.
- Improves correctness of variable offsets, base class introspection, and method
  lookup.


## Just-in-Time Compilation

- Adds full support for --Out-of-Process JIT--, including documentation and test
  infrastructure.
- Improves clang-repl integration and symbol handling across LLVM 18–21.
- Adds `DefineAbsoluteSymbol` to define injected symbols in the JIT.
- Major improvements to `JitCall` and runtime code generation:
  - Constructor call support and improved handling of arrays in
    construct/destruct APIs.
  - Support for move semantics (e.g. `std::unique_ptr`).
  - Fixes for lambdas as return types, duplicate symbols, enums vs. functions,
    and namespace conflicts.
  - Improves diagnostics in `JitCall::AreArgumentsValid`.


## Incremental C++

- Adds Cling transaction guards to prevent crashes in `GetNamed` and
  `GetAllCppNames`.
- Improves interpreter lifetime management and ownership semantics.
- Stabilizes successive `Declare` calls and template instantiation workflows.
- Improves integration with LLJIT and incremental execution.


## Misc

- Introduces a new __API Dispatch mechanism__:
  - Allows using CppInterOp without direct linking via runtime symbol dispatch
    (`CppGetProcAddress`).
  - Prevents LLVM/Clang symbol leakage into client applications.
  - Enables safe dynamic loading of CppInterOp alongside other LLVM-based
    libraries.
  - Current limitations:
    - Overloaded public APIs are not supported.
    - API must be initialized via `LoadDispatchAPI` before use.
    - Default arguments are not available in dispatched calls.
- Improves Emscripten support, CI pipelines, and deployment examples.
- Reorganizes public headers (`CppInterOp.h`) into a dedicated include path.
- Numerous improvements to CMake configuration, CI workflows, and documentation.

## Special Kudos

This release wouldn't have happened without the efforts of our contributors,
listed in the form of Firstname Lastname (#contributions):

FirstName LastName (#commits)

A B (N)

Matthew Barton (61; CI, Emscripten & deployment work, build and workflow maintenance)
Aaron Jomy (36; API dispatch, JIT / JitCall improvements, extensive tests and docs)
Vipul Cariappa (34; template & codegen fixes, type-reflection and interpreter robustness)
Vassil Vassilev (12; interpreter lifecycle, test coverage, refactorings and cleanup)
Abhinav Kumar (2; out-of-process JIT implementation and related integration)
Aditya Pandey (2; GetDoxygenComment API and README/build corrections)
Vincenzo Eduardo Padulano (1; target-existence checks and robustness fixes)
Stephan Hageboeck (1; test/build adjustments and gtest compatibility)
Mattias Ellert (1; platform-correctness tests for struct sizing)
Jonas Hahnfeld (1; conditional LLVM option handling / build integration)
Anutosh Bhat (1; compatibility fix for CLANG_VERSION_MAJOR > 21)

