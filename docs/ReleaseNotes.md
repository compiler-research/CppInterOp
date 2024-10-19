Introduction
============

This document contains the release notes for the language interoperability
library CppInterOp, release 1.4.0. CppInterOp is built on top of
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


What's New in CppInterOp 1.4.0?
=============================

Some of the major new features and improvements to CppInterOp are listed here.
Generic improvements to CppInterOp as a whole or to its underlying
infrastructure are described first.

External Dependencies
---------------------

* CppInterOp now works with:
  * llvm19


Introspection
-------------

* Add GetBinaryOperator interface
* Add GetIncludePaths interface
* Improve template instantiation logic


Incremental C++
---------------

* Improve the wasm infrastructure




Fixed Bugs
----------

[69](https://github.com/compiler-research/CppInterOp/issues/69)
[284](https://github.com/compiler-research/CppInterOp/issues/284)
[294](https://github.com/compiler-research/CppInterOp/issues/294)

Special Kudos
=============

This release wouldn't have happened without the efforts of our contributors,
listed in the form of Firstname Lastname (#contributions):

FirstName LastName (#commits)

A B (N)

mcbarton (16)
Vipul Cariappa (8)
Aaron Jomy (8)
maximusron (4)
Vassil Vassilev (2)
Gnimuc (2)
