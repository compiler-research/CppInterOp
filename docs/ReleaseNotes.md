Introduction
============

This document contains the release notes for the language interoperability
library CppInterOp, release 1.0. CppInterOp is built on top of
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


What's New in CppInterOp 1.0?
=============================

Some of the major new features and improvements to Clad are listed here. Generic
improvements to Clad as a whole or to its underlying infrastructure are
described first.

External Dependencies
---------------------

* CppInterOp now works with cling (based on patched clang13) and clang-repl
  (based on patched clang16)


Introspection
-------------

* Facilities enabling the is-a operation such as `IsAggregate`, `IsNamespace`,
  `IsClass`, `IsComplete`, `IsBuiltin`, `IsTemplate`,
  `IsTemplateSpecialization`, `IsTypedefed`, `IsAbstract`, `IsEnumScope`,
  `IsEnumConstant`, `IsEnumType`, `IsSmartPtrType`, `IsVariable`,
  `IsFunctionDeleted`, `IsTemplatedFunction`, `IsMethod`, `IsPublicMethod`,
  `IsProtectedMethod`, `IsPrivateMethod`, `IsConstructor`, `IsDestructor`,
  `IsStaticMethod`, `IsVirtualMethod`, `IsPublicVariable`,
  `IsProtectedVariable`, `IsPrivateVariable`, `IsStaticVariable`,
  `IsConstVariable`, `IsRecordType`, `IsPODType`, `IsTypeDerivedFrom`,
  `IsConstMethod`, and `HasDefaultConstructor`.

* Facilities for obtaining the introspection representations of entities such as
  `GetIntegerTypeFromEnumScope`, `GetIntegerTypeFromEnumType`,
  `GetEnumConstants`, `GetEnumConstantType`, `GetEnumConstantValue`,
  `GetSizeOfType`, `SizeOf`, `GetUsingNamespaces`, `GetGlobalScope`,
  `GetUnderlyingScope`, `GetScope`, `GetScopeFromCompleteName`, `GetNamed`,
  `GetParentScope`, `GetScopeFromType`, `GetBaseClassOffset`, `GetClassMethods`,
  `GetDefaultConstructor`, `GetDestructor`, `GetFunctionsUsingName`,
  `GetFunctionReturnType`, `GetFunctionNumArgs`, `GetFunctionRequiredArgs`,
  `GetFunctionArgType`, `GetDatamembers`, `LookupDatamember`, `GetVariableType`,
  `GetVariableOffset`, `GetUnderlyingType`, `GetCanonicalType`, `GetType`,
  `GetComplexType`, `GetTypeFromScope`, `InstantiateClassTemplate`, and
  `InstantiateTemplateFunctionFromString`.


Just-in-Time Compilation
------------------------

* Facilities getting the compiled representation of the entities such as
  `MakeFunctionCallable`, `InsertOrReplaceJitSymbol`, `Allocate`, `Deallocate`,
  `Construct`, and `Destruct`.


Incremental C++
---------------

* Facilities for setting up the intrastructure such as `CreateInterpreter`,
  `GetInterpreter`, `AddSearchPath`, and `AddIncludePath`.

* Facilities for consuming incremental input such as `Declare`, `Process` and
  `Evaluate`.


Debugging
---------

* Facilities for debugging such as `EnableDebugOutput`, `IsDebugOutputEnabled`,
  and `DumpScope`.


Misc
----

* Facilities for getting entities' string representation such as `GetName`,
  `GetCompleteName`, `GetQualifiedName`, `GetQualifiedCompleteName`,
  `GetFunctionSignature`, `GetTypeAsString`, `ObjToString`, and
  `GetAllCppNames`.


Special Kudos
=============

This release wouldn't have happened without the efforts of our contributors,
listed in the form of Firstname Lastname (#contributions):

FirstName LastName (#commits)

A B (N)

Baidyanath Kundu (141)
Vassil Vassilev (68)
Alexander Penev (16)
Smit1603 (8)
Krishna-13-cyber (4)
Vaibhav Thakkar (2)
QuillPusher (2)
