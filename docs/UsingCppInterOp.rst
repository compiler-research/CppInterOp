Using CppInterop
----------------

C++ Language Interoperability Layer
===================================

The CppInterop comes with using it is a dynamic shared library, 
**libclangCppInterOp.so** which resides in the CppInterOp/build/lib/.

.. code-block:: bash

    libInterop = ctypes.CDLL("./libclangCppInterOp.so")
    
The above method of usage is for Python; for C, we can include the headers of 
the library. Including this library in our programme enables the user to use 
the abilities of CppInterOp. CppInterOp helps programmers with multiple 
verifications such as isClass, isBoolean, isStruct, and many more in different 
languages. With the interop layer, we can access the scopes, namespaces of 
classes and members that are being used. The interoperability layer helps us 
with the instantiation of templates, diagnostic interaction, creation of 
objects, and many more things.

This section briefly describes all the key **features** offered by 
CppInterop. If you are just getting started with CppInterop, then this is the 
best place to start.

Incremental Adoption
====================
CppInterOp can be adopted incrementally. While the rest of the framework is the 
same, a small part of CppInterOp can be utilized. More components may be 
adopted over time.

Minimalist by design
====================
While the library includes some tricky code, it is designed to be simple and
robust (simple function calls, no inheritance, etc.). The goal is to make it as
close to the compiler API as possible, and each routine should do just one thing.
that it was designed for.

Further Enhancing the Dynamic/Automatic bindings in CPPYY
=========================================================
The main use case for CppInterOp is the CPPYY service. CPPYY is an
automatic run-time bindings generator for Python and C++, and supports a wide
range of C++ features (e.g., template instantiation). It operates on demand and
generates only what is necessary. It requires a compiler (Cling or Clang-REPL).
that can be available during programme runtime.

Once CppInterOp is integrated with LLVM's Clang-REPL component (that can then
be used as a runtime compiler for CPPYY), it will further enhance CPPYY's
performance in the following ways:


**Simpler codebase:** The removal of string parsing logic will lead to a simpler
code base.

**LLVM Integration:** The CppInterOp interfaces will be part of the LLVM 
toolchain (as part of Clang-REPL).

**Better C++ Support:** C++ features such as Partial Template Specialisation will
be available through CppInterOp.

**Fewer Lines of Code:** A lot of dependencies and workarounds will be removed,
reducing the lines of code required to execute CPPYY.

**Well tested interoperability Layer:** The CppInterOp interfaces have full
unit test coverage.


In case you haven't already installed CppInterop, please do so before proceeding
with the Installation And Usage Guide.
:doc:`Installation and usage <InstallationAndUsage>`