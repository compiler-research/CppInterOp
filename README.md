## CppInterOp Introduction
The CppInterOp library provides a minimalist approachfor other languages to 
identify C++ entities (variables, classes, etc.). This 
enables interoperability with C++ code, bringing the speed and 
efficiency of C++ to simpler, more interactive languages like Python.

### Incremental Adoption
CppInterOp can be adopted incrementally. While the rest of the framework is 
the same, a small part of CppInterOp can be utilized. More components may be 
adopted over time. 

### Minimalist by design
While the library includes some tricky code, it is designed to be simple and 
robust (simple function calls, no inheritance, etc.). The goal is to make it 
as close to the compiler API as possible, and each routine to do just one 
thing that it was designed for. 

### Further Enhancing the Dynamic/Automatic bindings in CPPYY
The main use case for CppInterOp is with the CPPYY service. CPPYY is an 
automatic run-time bindings generator for Python & C++, and supports a wide 
range of C++ features (e.g., template instantiation). It operates on demand 
and generates only what is necessary. It requires a compiler (Cling[^1] 
/Clang-REPL[^2]) that can be available during program runtime. 

Once CppInterOp is integrated with LLVM's[^3] Clang-REPL component (that can 
then be used as a runtime compiler for CPPYY), it will further enhance 
CPPYY's performance in the following ways:

- **Simpler codebase**: Removal of string parsing logic will lead to a 
simpler code base.
- **LLVM Integration**: The CppInterOp interfaces will be a part of the LLVM 
toolchain (as part of Clang-REPL).
- **Better C++ Support**: C++ features such as Partial Template 
Specialization will be available through CppInterOp.
- **Fewer Lines of Code**: A lot of dependencies and workarounds will be 
removed, reducing the lines of code required to execute CPPYY.
- **Well tested interoperability Layer**: The CppInterOp interfaces have full
 unit test coverage.

### 'Roots' in High Energy Physics research
Besides being developed as a general-purpose library, one of the long-term 
goals of CppInterOp is to stay backward compatible and be adopted in the High
 Energy Physics (HEP) field, as it will become an essential part of the Root 
framework. Over time, parts of the Root framework can be swapped by this API,
 adding speed and resilience with it.

### Build Instructions for Linux (Debian)
Build instructions for CppInterOp and its dependencies are as follows.

#### Setup Clang-REPL
Clone and checkout the LLVM project repository.
```
git clone https://github.com/llvm/llvm-project.git
git checkout release/16.x
```
Get the following patches required for development work.
```
compgen -G "../patches/llvm/clang16-*.patch" > /dev/null && find ../patches/llvm/clang16-*.patch -printf "%f\n" && git apply ../patches/llvm/clang16-*.patch
```
##### Build Clang
Clang-REPL is an interpreter that CppInterOp works alongside. Build Clang (and 
Clang-REPL along with it).
```
mkdir build && cd build
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
```
Note the 'llvm-project' directory location.
```
cd ../
export LLVM_DIR=$PWD
cd ../
```
Next, use the following export commands.
```
export CB_PYTHON_DIR="$PWD/cppyy-backend/python"
export INTEROP_DIR="$CB_PYTHON_DIR/cppyy_backend"
```
#### Build 'LLVM-Project' related dependencies
Following steps are required to help build CppInterOp alongside the 
LLVM-Project.
```
mkdir build && cd build
export INTEROP_BUILD_DIR=$PWD
cmake -DCMAKE_BUILD_TYPE=Release  \
                -DUSE_CLING=OFF             \
                -DUSE_REPL=ON               \
                -DLLVM_DIR=$LLVM_BUILD_DIR  \
                -DLLVM_USE_LINKER=lld       \
                -DBUILD_SHARED_LIBS=ON      \
                -DCMAKE_INSTALL_PREFIX=$INTEROP_DIR \
                ../
cmake --build . --target install --parallel $(nproc --all)
```
#### Build Cling related dependencies
Besides the Clang-REPL interpreter, CppInterOp also works alongside the Cling 
interpreter. Cling depends on its own customised version of `llvm-project`, 
hosted under the `root-project` (see the git path below). 
Use the following build instructions.
```
git clone --depth=1 https://github.com/root-project/cling.git
git clone --depth=1 -b cling-llvm13 https://github.com/root-project/llvm-project.git
cd llvm-project
mkdir build && cd build
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
```
Note the 'llvm-project' directory location.
```
cd ../
export LLVM_DIR=$PWD
cd ../
```
Next, export the following directory.
```
export INTEROP_DIR=$PWD/cppyy-backend/python/cppyy_backend
```
#### Build CppInterOp
Finally, clone the CppInterOp repository.
```
git clone https://github.com/compiler-research/CppInterOp.git
cd InterOp
mkdir build && cd build
INTEROP_BUILD_DIR=$(PWD)
Execute the following.
cmake -DBUILD_SHARED_LIBS=ON -DUSE_CLING=ON -DUSE_REPL=Off -DCling_DIR=$LLVM_DIR/build -DCMAKE_INSTALL_PREFIX=$INTEROP_DIR ..
cmake --build . --target install
```

---
Further Reading: [C++ Language Interoperability Layer](https://compiler-research.org/libinterop/)

[^1]: Cling is an interpretive Compiler for C++.
[^2]: Clang-REPL is an interactive C++ interpreter that enables incremental 
compilation. 
[^3]: LLVM is a Compiler Framework. It is a collection of modular compiler 
and toolchain technologies. 