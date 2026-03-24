# Introduction

This document contains the release notes for the language interoperability
library CppInterOp, release 1.9.0. CppInterOp is built on top of
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

## What's New in CppInterOp 1.9.0?

Some of the major new features and improvements to CppInterOp are listed here.
Generic improvements to CppInterOp as a whole or to its underlying
infrastructure are described first.

This release drops LLVM 18 support, adds CUDA/GPU support with automatic SDK
and architecture detection, improves introspection and type reflection APIs,
hardens the incremental C++ and dispatch infrastructure, and includes ROOT-Cling
integration changes for Cppyy.

## External Dependencies

- CppInterOp v1.9.0 supports llvm 19-21
- Drops llvm18 support.

## Introspection

- Adds `Cpp::IsExplicit` for constructors, conversion operators and deduction
  guides.
- Adds `Cpp::GetLanguage` for language detection support.
- Replaces string comparison with AST check in `IsBuiltin` for `std::complex`.
- Merges `GetCompleteName` and `GetQualifiedCompleteName` into a shared helper.
- Makes `GetTypeAsString()` use the printing policy from ASTContext.

## Just-in-Time Compilation

- Cling specific changes required for Cppyy in ROOT (#787).
- Improved CUDA interpreter creation, uses clang::Driver to detect CUDA SDK and GPU architecture.
- Adds incremental CUB BlockReduce test for CUDA.

## Incremental C++

- Fixes `SynthesizingCodeRAII` for clang-repl (#819).
- Uses a common `compat::Value` to avoid macro guard duplication.

## Misc

- Adds `GetDoxygenComment` to the API dispatch table.
- Disables verbose `LoadAPI` message unless debug mode is enabled.
- Allows reset of dispatch data to test load/unload.
- Prioritizes user-provided resource-dir over auto-detection (#791).
- Uses `llvm::sys::fs` instead of `<filesystem>` to check resource-dir.
- Includes API after undef of windows.h `LoadLibrary`.
- Removes `using namespace std` from CppInterOp.cpp (#801).
- Removes no-soname from cmake (#816).
- Numerous improvements to CMake configuration, CI workflows, and documentation.

## Fixed Bugs

- Fix error-prone failure path in `GetTypeFromScope`.
- Fix `GetLanguage` for CUDA/HIP where CXX standard was returned.
- Fix CUDA runtime error detection in tests.
- Fixes thread-safe initialization of Dispatch API using function-local
  static (#844).
- Sets default build type if not provided.

## Special Kudos

This release wouldn't have happened without the efforts of our contributors,
listed in the form of Firstname Lastname (#contributions):

Matthew Barton (21; CI, Emscripten, build and deployment improvements)
Aaron Jomy (19; CUDA support, Dispatch, IsExplicit API, CI, build and cmake fixes)
Vassil Vassilev (5; coverage, test fixes, release preparation)
Vipul Cariappa (3; ROOT-Cling integration, SynthesizingCodeRAII fix, resource-dir)
Adriteyo Das (3; IsBuiltin AST check, GetCompleteName refactoring, default build type)
Emery Conrad (2; Dispatch reset, path resolution in detection test)
Kerem Şahin (1; language detection support)
Jonas Hahnfeld (1; resource-dir check)
Caevan Lin (1; thread-safe Dispatch API initialization)
Janak Shah (1; printing policy fix)
