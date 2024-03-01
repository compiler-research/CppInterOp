Introduction
============

This document contains the release notes for the language interoperability
library CppInterOp, release 1.2.0. CppInterOp is built on top of
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


What's New in CppInterOp 1.2.0?
=============================

Some of the major new features and improvements to Clad are listed here. Generic
improvements to Clad as a whole or to its underlying infrastructure are
described first.

External Dependencies
---------------------

* CppInterOp now works with:
  * cling (based on patched clang13)
  * clang-repl
    * based on patched clang16
    * clang17


Misc
----

* Add support for Windows
* Add support for Wasm


Fixed Bugs
----------

Special Kudos
=============

This release wouldn't have happened without the efforts of our contributors,
listed in the form of Firstname Lastname (#contributions):

FirstName LastName (#commits)

A B (N)

fsfod (30)
mcbarton (10)
Vassil Vassilev (6)
Shreyas Atre (4)
Alexander Penev (2)
Saqib (1)
Aaron  Jomy (1)

