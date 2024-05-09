Introduction
============

This document contains the release notes for the language interoperability
library CppInterOp, release 1.3.0. CppInterOp is built on top of
[Clang](http://clang.llvm.org) and [LLVM](http://llvm.org>) compiler
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


What's New in CppInterOp 1.3.0?
=============================

Some of the major new features and improvements to CppInterOp are listed here.
Generic improvements to CladCppInterOp as a whole or to its underlying
infrastructure are described first.

External Dependencies
---------------------

* CppInterOp now works with:
  * cling (based on patched clang16)
  * clang-repl
    * based on patched clang16
    * clang17
    * clang18


Introspection
-------------

*
* `GetFunctionArgDefault` and `GetFunctionArgName` now consider template
  functions
* Consolidate the template instantiation logic.


Incremental C++
---------------

* Add Code Completion API
* Improved Windows support
* Add API for detection of resource and include directories from the system
  compiler


Misc
----

* Improved installation of the CppInterOp folder.


Fixed Bugs
----------

[257](https://github.com/compiler-research/CppInterOp/issues/257)

Special Kudos
=============

This release wouldn't have happened without the efforts of our contributors,
listed in the form of Firstname Lastname (#contributions):

FirstName LastName (#commits)

A B (N)

Aaron  Jomy (19)
mcbarton (17)
Vassil Vassilev (10)
Alexander Penev (4)
