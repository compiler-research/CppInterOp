Undo and failed inputs
======================

An interactive session sends one input at a time. When an input fails, or when
the user withdraws an input with ``Cpp::Undo``, the interpreter must return to
the state it had before that input. Otherwise a single mistake can make every
later input fail, and the user must restart the session.

clang-repl removes the declarations of a withdrawn input. It does not yet roll
back all the state that the input changed outside its own declarations. The
upstream work on this is in progress (see `Upstream work`_). Until it lands,
CppInterOp does the part that it can do through the clang API. The code is in
``lib/CppInterOp/InputRollback.cpp`` and ``WithdrawInputs`` in
``lib/CppInterOp/CppInterOp.cpp``. Remove each workaround when the matching
upstream change is in all supported LLVM versions.

API
---

``Cpp::GetInputCount()``
  Returns the number of inputs that the interpreter holds. Record it as a
  checkpoint.

``Cpp::Undo(N)``
  Withdraws the last ``N`` inputs. To return to a checkpoint, pass the
  difference between the current count and the checkpoint.

A failed ``Cpp::Declare``, ``Cpp::Process`` or ``Cpp::Evaluate`` withdraws its
own inputs. The caller does not call ``Cpp::Undo`` for it.

A language binding usually adds inputs of its own, for example JIT call
wrappers. Count from a checkpoint, not from the binding's own calls. Do not
call ``Cpp::Undo`` through a JIT call wrapper: the wrapper can be one of the
inputs that it withdraws.

What CppInterOp rolls back
--------------------------

.. list-table::
   :header-rows: 1
   :widths: 30 35 35

   * - State
     - When
     - How
   * - Inputs that fail after parse (link, run time)
     - Failed ``Declare``, ``Process``, ``Evaluate``
     - ``Cpp::Undo`` of the inputs that the call added
   * - Module that cannot link but has static initializers
     - Before ``Execute``
     - Look up its external symbols first and fail cleanly. LLJIT keeps the
       init function of a module that fails to link queued, and every later
       input then fails.
   * - Implicit instantiations that a failed input requested
     - Failed parse
     - Record them through ``ASTConsumer`` callbacks; reset the body, the point
       of instantiation and the pending queue
   * - Macros (``#define``, ``#undef``)
     - Failed parse, ``Undo``
     - Record them through ``PPCallbacks``; restore the previous definition
   * - ``#pragma once`` and ``#import`` state, include guards
     - Failed parse, ``Undo``
     - Clear the included-file state of files that the input entered first;
       an include guard is a macro
   * - Names in the TU scope
     - ``Undo``
     - Remove them from ``TUScope`` and ``IdResolver``
   * - Members added to a namespace that an earlier input opened
     - ``Undo``
     - Remove them from the namespace lookup
   * - Enumerators of an unscoped enum
     - ``Undo``
     - Remove them from the enclosing lookup. Since llvm#218149, clang
       removes the enum but not its enumerators.
   * - A declaration that a withdrawn redeclaration replaced
     - ``Undo``
     - Put it back in the scope and the lookup; make it the latest of its
       redeclaration chain when it is the first declaration
   * - Cached JIT call wrappers
     - ``Undo``
     - Drop the wrappers that live in a withdrawn input

The instantiation, macro and include rollback needs clang-repl from LLVM 22 or
later. The link check is for in-process JIT only.

What is not rolled back, and why
--------------------------------

.. list-table::
   :header-rows: 1
   :widths: 35 35 30

   * - Case
     - Why CppInterOp cannot do it
     - Upstream work
   * - CodeGen error in an input (for example a duplicate mangled name);
       the session can crash
     - The state is in ``CodeGenModule``, which is private to clang CodeGen
     - llvm#226285, #234493, #204152, #223305
   * - Function that fails to link on its first call, then the missing
       symbol is defined
     - ORC keeps the error state of the failed lookup, also after the module
       is removed. Undo the input and define the function again.
     - None yet (ORC)
   * - Class definition after a forward declaration in an earlier input, then
       undo of the definition
     - The redeclarations of a class share private definition data
     - Fixed by llvm#218149 in LLVM 23.1.3
   * - A redeclaration chain with more than one kept declaration
     - The public API can only cut a chain back to its first declaration
     - Fixed by llvm#218149 in LLVM 23.1.3
   * - A header that changed on disk after undo
     - The file manager caches the contents for the session
     - llvm#182750
   * - Run-time side effects (memory, files, output)
     - Not interpreter state
     - --
   * - Objects whose class was withdrawn
     - Their code is withdrawn; calls fail
     - --
   * - cling
     - cling has its own transactions; ``GetInputCount`` returns 0
     - --

Tests
-----

``unittests/CppInterOp/InterpreterTest.cpp`` (``Interpreter_FailedLink*``,
``Interpreter_FailedParse*``, ``Interpreter_Undo*``). Bindings have more
end-to-end cases; for example cppjit's ``test/test_failed_input.py`` marks
each upstream gap as an expected failure.

Upstream work
-------------

Merged in clang-repl:

- llvm#85378, #127467: error recovery in ``CleanUpPTU``.
- llvm#149396: error message on a failed undo.
- llvm#178648: ``CleanUpPTU`` and implicit C function declarations.
- llvm#213235 (issue #213230): unlink the withdrawn translation unit on undo.
- llvm#218129: do not remove ``extern "C"`` declarations twice.
- llvm#218149 (issue #201844): keep earlier declarations alive when an input
  fails.

Open:

- llvm#223305: error recovery for clang-repl, general rollback of
  declaration state across inputs.
- llvm#231363: mark and rewind for ``BumpPtrAllocator``, for #223305.
- llvm#234493: ``CodeGenModule::forgetDecl``.
- llvm#204152: module contamination after a failed template instantiation.
- llvm#226285 (issue #226252): drop the IR of a failed input.
- llvm#192588, #226253, #228330: roll back or redo the implicit instantiations
  of a failed input (issue #146770).
- llvm#182750: undo of file inclusion and macro definitions.
- llvm#228370: undo the enumerators of an unscoped enumeration.
- llvm#75629: segfault in ``CleanUpPTU``.
