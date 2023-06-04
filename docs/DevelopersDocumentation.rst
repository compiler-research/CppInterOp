Developers Documentation
-------------------------

Building from source
---------------------

Clang-Repl
===========

.. code-block:: bash

    export INTEROP_DIR=$PWD/cppyy-backend/python/cppyy_backend

    git clone https://github.com/compiler-research/CppInterOp.git

    cd CppInterOp

    mkdir build && cd build

    INTEROP_BUILD_DIR=$(PWD)

    cmake -DBUILD_SHARED_LIBS=ON -DUSE_CLING=ON -DUSE_REPL=Off -DCling_DIR=$LLVM_DIR/build -DCMAKE_INSTALL_PREFIX=$INTEROP_DIR ..

    cmake --build . --target install


CppInterOp is a C++ Language Interoperability Layer

CppInterOp Internal Documentation
=================================

CppInterOp maintains an internal Doxygen documentation of its components. Internal
documentation aims to capture intrinsic details and overall usage of code 
components. The goal of internal documentation is to make the codebase easier 
to understand for the new developers. 