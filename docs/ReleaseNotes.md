# Introduction

This document contains the release notes for the language interoperability
library CppInterOp, release 1.5.0. CppInterOp is built on top of
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

## What's New in CppInterOp 1.5.0?

Some of the major new features and improvements to CppInterOp are listed here.
Generic improvements to CppInterOp as a whole or to its underlying
infrastructure are described first.

## New Features

* Jupyter-Lite Integration: Added a Jupyter-Lite demo for xeus-cpp showcasing
  CppInterOp's capabilities in a browser-based environment. (#380)
* New C API: Introduced a libclang-style C API for better integration with
  CppInterOp's interpreter and scope manipulations. This includes new types
  CXScope and CXQualType. (#337)

## Enhancements

* Emscripten Build Improvements:
  * Resolved deployment issues and improved CI workflows for Emscripten builds.
    (#384, #414, #415)
  * Removed redundant dependencies like zlib, optimizing Emscripten builds.
    (#373)
  * Switched to shared builds for WebAssembly targets. (#375)

* Documentation Updates:
  * Improved documentation for using CppInterOp with updated references and
    build instructions. (#370, #390)
  * Set REPL mode as the default and updated related documentation. (#360)

* CI/CD Optimization:
  * Split workflows for MacOS, Ubuntu, and Windows for better build management.
    (#404, #408)
  * Enhanced stale PR and issue processing workflows for better project
    maintenance. (#376, #368)

## Bug Fixes

[255](https://github.com/compiler-research/CppInterOp/issues/255)

* Build and Deployment:
  * Fixed shared library deployment paths. (#415)
  * Addressed copying issues with deployment scripts for xeus-cpp-lite. (#411)
  * Resolved build failures in complex inheritance scenarios and LLVM-related
    issues. (#389, #396)

* General Stability:
  * Fixed assertion issues for external interpreters. (#367)
  * Resolved cache size problems to optimize performance. (#386)

* Miscellaneous:
  * Removed duplicate workflows and redundant files to streamline the project.
    (#374, #391)

## Maintenance

* CI Updates:
  * Updated GitHub workflows, including CI naming conventions and support for
    LLVM 19. (#406, #369)
  * Added markdown linting for consistency across documentation. (#363)

* Code Cleanup:
  * Removed unused code and addressed minor issues for better maintainability.
    (#356)


## Special Kudos

This release wouldn't have happened without the efforts of our contributors,
listed in the form of Firstname Lastname (#contributions):

FirstName LastName (#commits)

mcbarton (46)
Vipul Cariappa (6)
Anutosh Bhat (6)
Aaron Jomy (4)
Anurag Bhat (3)
Aaron  Jomy (3)
Vassil Vassilev (2)
Gnimuc (2)
Atharv Dubey (1)
