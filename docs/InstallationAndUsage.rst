InstallationAndUsage
---------------------


Build cling with LLVM and clang:
===================================


.. code-block:: bash

   git clone --depth=1 https://github.com/root-project/cling.git
   git clone --depth=1 -b cling-llvm13 https://github.com/root-project/llvm-project.git
   cd llvm-project
   mkdir build
   cd build
   cmake -DLLVM_ENABLE_PROJECTS=clang                \
      -DLLVM_EXTERNAL_PROJECTS=cling                \
      -DLLVM_EXTERNAL_CLING_SOURCE_DIR=../../cling  \
      -DLLVM_TARGETS_TO_BUILD="host;NVPTX"          \
      -DCMAKE_BUILD_TYPE=Release                    \
      -DLLVM_ENABLE_ASSERTIONS=ON                   \
      -DLLVM_USE_LINKER=lld                         \
      -DCLANG_ENABLE_STATIC_ANALYZER=OFF            \
      -DCLANG_ENABLE_ARCMT=OFF                      \
      -DCLANG_ENABLE_FORMAT=OFF                     \
      -DCLANG_ENABLE_BOOTSTRAP=OFF                  \
      ../llvm
   cmake --build . --target clang --parallel $(nproc --all)
   cmake --build . --target cling --parallel $(nproc --all)
   cmake --build . --target gtest_main --parallel $(nproc --all)

Note down the llvm-project directory location as we will need it later:

   cd ../
   export LLVM_DIR=$PWD
   cd ../

Clone the CppInterOp repo. Build it using cling and install. Note down the path to CppInterOp install directory. This will be referred to as CPPINTEROP_DIR:

   export CPPINTEROP_DIR=$PWD/cppyy-backend/python/cppyy_backend
   git clone https://github.com/compiler-research/CppInterOp.git
   cd CppInterOp
   mkdir build && cd build
   CPPINTEROP_BUILD_DIR=$(PWD)
   cmake -DBUILD_SHARED_LIBS=ON -DUSE_CLING=ON -DUSE_REPL=Off -DCling_DIR=$LLVM_DIR/build -DCMAKE_INSTALL_PREFIX=$CPPINTEROP_DIR ..
   cmake --build . --target install

Build Clang-Repl:
=================

.. code-block:: text


   Get the llvm/release/16.x version:
   git clone https://github.com/llvm/llvm-project.git
   git checkout release/16.x

   Patches for development
   compgen -G "../patches/llvm/clang16-*.patch" > /dev/null && find ../patches/llvm/clang16-*.patch -printf "%f\n" 
   && git apply ../patches/llvm/clang16-*.patch


   mkdir build
   cd build

   Build Clang-16 :

      cmake -DLLVM_ENABLE_PROJECTS=clang                  \
             -DLLVM_TARGETS_TO_BUILD="host;NVPTX"          \
             -DCMAKE_BUILD_TYPE=Release                    \
             -DLLVM_ENABLE_ASSERTIONS=ON                   \
             -DLLVM_USE_LINKER=lld                         \
             -DCLANG_ENABLE_STATIC_ANALYZER=OFF            \
             -DCLANG_ENABLE_ARCMT=OFF                      \
             -DCLANG_ENABLE_FORMAT=OFF                     \
             -DCLANG_ENABLE_BOOTSTRAP=OFF                  \
             ../llvm


      cmake --build . --target clang clang-repl --parallel $(nproc --all)

   Note down the llvm-project directory location as we will need it later:
   cd ../
   export LLVM_DIR=$PWD
   cd ../

   Export Commands:
   export CB_PYTHON_DIR="$PWD/cppyy-backend/python"
   export CPPINTEROP_DIR="$CB_PYTHON_DIR/cppyy_backend"


          cmake -DCMAKE_BUILD_TYPE=Release  \
                -DUSE_CLING=OFF             \
                -DUSE_REPL=ON               \
                -DLLVM_DIR=$LLVM_BUILD_DIR  \
                -DLLVM_USE_LINKER=lld       \
                -DBUILD_SHARED_LIBS=ON      \
                -DCMAKE_INSTALL_PREFIX=$CPPINTEROP_DIR \
                ../

   cmake --build . --target install --parallel $(nproc --all)

