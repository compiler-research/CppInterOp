# Introduction

This document contains the release notes for the language interoperability
library CppInterOp, release 1.6.0. CppInterOp is built on top of
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

## What's New in CppInterOp 1.6.0?

Some of the major new features and improvements to CppInterOp are listed here.
Generic improvements to CppInterOp as a whole or to its underlying
infrastructure are described first.

## External Dependencies

- CppInterOp now works with:
  - llvm19

## Introspection

- Added `IsPointerType` and `GetPointerType` functions.
- Added `IsReferenceType` and `GetNonReferenceType` functions.
- Introduced `IsFunctionPointerType` function.
- Implemented `GetEnumConstantDatamembers` function to resolve all
  EnumConstantDecls declared in a class.
- Added IsClassPolymorphic function.


## Just-in-Time Compilation

- Introduced Demangle function for handling name demangling.


## Incremental C++

- Code generation fixes for `MakeFunctionCallable` for template operators and
  incorrect handling when the return type is a function pointer.

## Misc

- Fixed a bug preventing users from disabling testing in CMake.
- Prefixed interpreter CMake option for better clarity.
- Multiple improvements in the continuous integration infrastructure including
  compilation times, cache sizes, added new jobs for arm.

## Special Kudos

This release wouldn't have happened without the efforts of our contributors,
listed in the form of Firstname Lastname (#contributions):

FirstName LastName (#commits)

A B (N)

mcbarton (57)
Vipul Cariappa (11)
Aaron Jomy (7)
Aaron  Jomy (3)
maximusron (1)
Vassil Vassilev (1)
Gnimuc (1)
