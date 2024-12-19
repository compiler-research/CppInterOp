# Wasm Build Instructions

It should be noted that the wasm build of CppInterOp is still experimental and subject to change to change.  

## CppInterOp Wasm Build Instructions

This document first starts with the instructions on how to build a wasm build of CppInterOp. Before we start it should be noted that  
unlike the non wasm version of CppInterOp we currently only support the Clang-REPL backend using llvm>19 for osx and Linux.  
We will first make folder to build our wasm build of CppInterOp. This can be done by executing the following command

```bash
mkdir CppInterOp-wasm
```

Now move into this directory using the following command

```bash
cd ./CppInterOp-wasm
```

To create a wasm build of CppInterOp we make use of the emsdk toolchain. This can be installed by executing (we only currently  
support version 3.1.45)
```bash
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install  3.1.45
```

Go back upto the top level build directory using

```bash
cd ../..
```

and activate the emsdk environment

```bash
./emsdk/emsdk activate 3.1.45
source ./emsdk/emsdk_env.sh
```

Now clone the 19.x release of the LLVM project repository and CppInterOp (the building of the emscripten version of llvm can be
avoided by executing micromamba install llvm -c <https://repo.mamba.pm/emscripten-forge> and setting the LLVM_BUILD_DIR appropriately)


```bash
git clone --depth=1 --branch release/19.x https://github.com/llvm/llvm-project.git
git clone --depth=1 https://github.com/compiler-research/CppInterOp.git
```

Now move into the cloned llvm-project folder and apply a required patch

```bash
cd ./llvm-project/
git apply -v ../CppInterOp/patches/llvm/emscripten-clang${{ matrix.clang-runtime }}-*.patch
```

We are now in a position to build an emscripten build of llvm by executing the following
```bash
mkdir build
cd build
emcmake cmake -DCMAKE_BUILD_TYPE=Release \
                        -DLLVM_HOST_TRIPLE=wasm32-unknown-emscripten \
                        -DLLVM_ENABLE_ASSERTIONS=ON                        \
                        -DLLVM_TARGETS_TO_BUILD="WebAssembly" \
                        -DLLVM_ENABLE_LIBEDIT=OFF \
                        -DLLVM_ENABLE_PROJECTS="clang;lld" \
                        -DLLVM_ENABLE_ZSTD=OFF \
                        -DLLVM_ENABLE_LIBXML2=OFF \
                        -DCLANG_ENABLE_STATIC_ANALYZER=OFF \
                        -DCLANG_ENABLE_ARCMT=OFF \
                        -DCLANG_ENABLE_BOOTSTRAP=OFF \
                        -DCMAKE_CXX_FLAGS="-Dwait4=__syscall_wait4" \
                        ../llvm
emmake make clang -j $(nproc --all)
emmake make lld -j $(nproc --all)
```

Once this finishes building we need to take note of where we built our llvm build. This can be done by executing the following

```bash
export LLVM_BUILD_DIR=$PWD
```

We can move onto building the wasm version of CppInterOp. To do this execute the following  

```bash
cd ../../CppInterOp/
mkdir build
cd ./build/
emcmake cmake -DCMAKE_BUILD_TYPE=Release    \
                -DUSE_CLING=OFF                             \
                -DUSE_REPL=ON                               \
                -DLLVM_DIR=$LLVM_BUILD_DIR/lib/cmake/llvm      \
                -DLLD_DIR=$LLVM_BUILD_DIR/lib/cmake/lld     \
                -DClang_DIR=$LLVM_BUILD_DIR/lib/cmake/clang     \
                -DBUILD_SHARED_LIBS=ON                      \
                -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ON            \
                -DCMAKE_INSTALL_PREFIX=$PREFIX         \
                ../
emmake make -j $(nproc --all)
```

Once this finishes building we need to take note of where we built CppInterOp. This can be done by executing the following

```bash
export CPPINTEROP_BUILD_DIR=$PWD
```

## Xeus-cpp-lite Wasm Build Instructions

A project which makes use of the wasm build of CppInterOp is xeus-cpp. xeus-cpp is a C++ Jupyter kernel. To build xeus-cpp we first need to  
install several other dependencies. To create an Conda environment with this dependencies installed execute the following from the  
CppInterOp build folder (these instructions will assume you have already installed micromamba)  

```bash
cd ..
micromamba create -f environment-wasm.yml --platform=emscripten-wasm32
micromamba activate CppInterOp-wasm
export PREFIX=$MAMBA_ROOT_PREFIX/envs/CppInterOp-wasm
```
Now we can build an wasm build of xeus-cpp by executing the following  

```bash
cd ..
git clone --depth=1 https://github.com/compiler-research/xeus-cpp.git
cd ./xeus-cpp
mkdir build
cd build
export CMAKE_PREFIX_PATH=$PREFIX
export CMAKE_SYSTEM_PREFIX_PATH=$PREFIX 
emcmake cmake \
          -DCMAKE_BUILD_TYPE=Release                                     \
          -DCMAKE_PREFIX_PATH=$PREFIX                                    \
          -DCMAKE_INSTALL_PREFIX=$PREFIX                                 \
          -DXEUS_CPP_EMSCRIPTEN_WASM_BUILD=ON                            \
          -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ON                         \
          -DCppInterOp_DIR="$CPPINTEROP_BUILD_DIR/lib/cmake/CppInterOp"  \
          ..
 emmake make -j $(nproc --all) install
```

To build Jupyter Lite with this kernel without creating a website you can execute the following

```bash
cd ../..
micromamba create -n xeus-lite-host jupyterlite-core
micromamba activate xeus-lite-host
python -m pip install jupyterlite-xeus
jupyter lite build --XeusAddon.prefix=$PREFIX
```

Once the Jupyter Lite site has built you can test the website locally  by executing

```bash
jupyter lite serve --XeusAddon.prefix=$PREFIX
```
