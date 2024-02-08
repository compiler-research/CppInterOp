<div align="center">

# CppInterOp

[![Build Status](https://github.com/compiler-research/CppInterOp/actions/workflows/ci.yml/badge.svg)](https://github.com/compiler-research/CppInterOp/actions/workflows/ci.yml) 
[![codecov]( https://codecov.io/gh/compiler-research/CppInterOp/branch/main/graph/badge.svg)](https://codecov.io/gh/compiler-research/CppInterOp)


CppInterOp exposes API from [Clang](http://clang.llvm.org/) and [LLVM](https://llvm.org) in a backward compatibe way. The API support downstream tools that utilize interactive C++ by using the compiler as a service. That is, embed Clang and LLVM as a libraries in their codebases. The API are designed to be minimalistic and aid non-trivial tasks such as language interoperability on the fly. In such scenarios CppInterOp can be used to provide the necessary introspection information to the other side helping the language cross talk.


#### [Installation](#build-instructions-for-linux) | [Documentation](https://cppinterop.readthedocs.io/en/latest/index.html) | [CppInterOp API Documentation](https://cppinterop.readthedocs.io/en/latest/build/html/index.html)
</div>

## CppInterOp Introduction
The CppInterOp library provides a minimalist approach for other languages to 
bridge C++ entities (variables, classes, etc.). This 
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

### Build Instructions (Includes instructions both Unix systems and Windows)
Build instructions for CppInterOp and its dependencies are as follows. CppInterOP can be built with either Cling and Clang-REPL, so instructions will differ slightly depending on which option you would like to build, but should be clear from the section title which instructions to follow.

#### Clone CppInterOp and cppyy-backend
First clone the CppInterOp repository, as this contains patches that need to be applied to the subsequently cloned llvm-project repo (these patches are only applied if building CppInterOp with Clang-REPL)
```
git clone --depth=1 https://github.com/compiler-research/CppInterOp.git
```
and clone cppyy-backend repository where we will be installing the CppInterOp library
```
git clone --depth=1 https://github.com/compiler-research/cppyy-backend.git
```

#### Setup Clang-REPL 
Clone the 17.x release of the LLVM project repository.
```
git clone --depth=1 --branch release/17.x https://github.com/llvm/llvm-project.git
cd llvm-project
```
Get the following patches required for development work. To apply these patches on Linux and MacOS execute the following command
```
git apply -v ../CppInterOp/patches/llvm/clang17-*.patch
```
and
```
cp -r ..\CppInterOp\patches\llvm\clang17* .
git apply -v clang17-1-NewOperator.patch
```
on Windows.

##### Build Clang-REPL
Clang-REPL is an interpreter that CppInterOp works alongside. Build Clang (and 
Clang-REPL along with it). On Linux and MaxOS you do this by executing the following
command
```
mkdir build 
cd build 
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
On Windows you would do this by executing the following
```
$env:ncpus = %NUMBER_OF_PROCESSORS%
mkdir build 
cd build 
cmake   -DLLVM_ENABLE_PROJECTS=clang                  `
        -DLLVM_TARGETS_TO_BUILD="host;NVPTX"          `
        -DCMAKE_BUILD_TYPE=Release                    `
        -DLLVM_ENABLE_ASSERTIONS=ON                   `
        -DCLANG_ENABLE_STATIC_ANALYZER=OFF            `
        -DCLANG_ENABLE_ARCMT=OFF                      `
        -DCLANG_ENABLE_FORMAT=OFF                     `
        -DCLANG_ENABLE_BOOTSTRAP=OFF                  `
        ..\llvm
        cmake --build . --target clang clang-repl --parallel $env:ncpus
```
Note the 'llvm-project' directory location. On linux and MacOS you execute the following
```
cd ../
export LLVM_DIR=$PWD
cd ../
```
On Windows you execute the following
```
cd ..\
$env:LLVM_DIR= $PWD.Path
cd ..\
```

#### Build Cling and related dependencies
Besides the Clang-REPL interpreter, CppInterOp also works alongside the Cling 
interpreter. Cling depends on its own customised version of `llvm-project`, 
hosted under the `root-project` (see the git path below). 
Use the following build instructions to build on Linux and MacOS
```
git clone https://github.com/root-project/cling.git
cd ./cling/
git checkout tags/v1.0
cd ..
git clone --depth=1 -b cling-llvm13 https://github.com/root-project/llvm-project.git
mkdir llvm-project/build
cd llvm-project/build
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
Use the following build instructions to build on Windows
```
git clone https://github.com/root-project/cling.git
cd .\cling\
git checkout tags/v1.0
cd ..
git clone --depth=1 -b cling-llvm13 https://github.com/root-project/llvm-project.git
$env:ncpus = %NUMBER_OF_PROCESSORS%
$env:PWD_DIR= $PWD.Path
$env:CLING_DIR="$env:PWD_DIR\cling"
mkdir llvm-project\build
cd llvm-project\build
cmake   -DLLVM_ENABLE_PROJECTS=clang                  `
        -DLLVM_EXTERNAL_PROJECTS=cling                `
        -DLLVM_EXTERNAL_CLING_SOURCE_DIR="$env:CLING_DIR"   `
        -DLLVM_TARGETS_TO_BUILD="host;NVPTX"          `
        -DCMAKE_BUILD_TYPE=Release                    `
        -DLLVM_ENABLE_ASSERTIONS=ON                   `
        -DCLANG_ENABLE_STATIC_ANALYZER=OFF            `
        -DCLANG_ENABLE_ARCMT=OFF                      `
        -DCLANG_ENABLE_FORMAT=OFF                     `
        -DCLANG_ENABLE_BOOTSTRAP=OFF                  `
        ../llvm
cmake --build . --target clang --parallel $env:ncpus
cmake --build . --target cling --parallel $env:ncpus
cmake --build . --target gtest_main --parallel $env:ncpus
```
Note the 'llvm-project' directory location. On linux and MacOS you execute the following
```
cd ../
export LLVM_DIR=$PWD
cd ../
```
On Windows you execute the following
```
cd ..\
$env:LLVM_DIR= $PWD.Path
cd ..\
```

#### Environment variables 
Regardless of whether you are building CppInterOP with Cling or Clang-REPL you will need to define the following Envirnoment variables (as they clear for a new session, it is recommended that you also add these to your .bashrc in linux, .bash_profile if on MacOS, or profile.ps1 on Windows). On Linux and MacOS you define as follows
```
export CB_PYTHON_DIR="$PWD/cppyy-backend/python"
export CPPINTEROP_DIR="$CB_PYTHON_DIR/cppyy_backend"
export CPLUS_INCLUDE_PATH="${CPLUS_INCLUDE_PATH}:${LLVM_DIR}/llvm/include:${LLVM_DIR}/clang/include:${LLVM_DIR}/build/include:${LLVM_DIR}/build/tools/clang/include"
export PYTHONPATH=$PYTHONPATH:$CPYCPPYY_DIR:$CB_PYTHON_DIR
```
If on MacOS you will also need the following envirnoment variable defined
```
export SDKROOT=`xcrun --show-sdk-path`
```
On Windows you define as follows (assumes you have defined $env:PWD_DIR= $PWD.Path )
```
$env:CB_PYTHON_DIR="$env:PWD_DIR\cppyy-backend\python"
$env:CPPINTEROP_DIR="$env:CB_PYTHON_DIR\cppyy_backend"
$env:CPLUS_INCLUDE_PATH="$env:CPLUS_INCLUDE_PATH;$env:LLVM_DIR\llvm\include;$env:LLVM_DIR\clang\include;$env:LLVM_DIR\build\include;$env:LLVM_DIR\build\tools\clang\include"
$env:PYTHONPATH="$env:PYTHONPATH;$env:CPYCPPYY_DIR;$env:CB_PYTHON_DIR"
```

#### Build CppInterOp
Now CppInterOp can be installed. On Linux and MacOS execute
```
mkdir CppInterOp/build/
cd CppInterOp/build/
```
On Windows execute
```
mkdir CppInterOp\build\
cd CppInterOp\build\
```

Now if you want to build CppInterOp with Clang-REPL then execute the following commands on Linux and MacOS
```
cmake -DBUILD_SHARED_LIBS=ON -DUSE_CLING=Off -DUSE_REPL=ON -DLLVM_DIR=$LLVM_DIR/build/lib/cmake/llvm -DClang_DIR=$LLVM_DIR/build/lib/cmake/clang -DCMAKE_INSTALL_PREFIX=$CPPINTEROP_DIR ..
cmake --build . --target install --parallel $(nproc --all)
```
and
```
cmake -DUSE_CLING=Off -DUSE_REPL=ON -DLLVM_DIR=$env:LLVM_DIR\build\lib\cmake\llvm -DClang_DIR=$env:LLVM_DIR\build\lib\cmake\clang -DCMAKE_INSTALL_PREFIX=$env:CPPINTEROP_DIR ..
cmake --build . --target install --parallel $env:ncpus
```
on Windows. If alternatively you would like to install CppInterOp with Cling then execute the following commands on Linux and MacOS
```
cmake -DBUILD_SHARED_LIBS=ON -DUSE_CLING=ON -DUSE_REPL=Off -DCling_DIR=$LLVM_DIR/build/tools/cling -DLLVM_DIR=$LLVM_DIR/build/lib/cmake/llvm -DClang_DIR=$LLVM_DIR/build/lib/cmake/clang -DCMAKE_INSTALL_PREFIX=$CPPINTEROP_DIR ..
cmake --build . --target install --parallel $(nproc --all)
```
and
```
cmake -DUSE_CLING=ON -DUSE_REPL=Off -DCling_DIR=$env:LLVM_DIR\build\tools\cling -DLLVM_DIR=$env:LLVM_DIR\build\lib\cmake\llvm -DClang_DIR=$env:LLVM_DIR\build\lib\cmake\clang -DCMAKE_INSTALL_PREFIX=$env:CPPINTEROP_DIR ..
cmake --build . --target install --parallel $env:ncpus
```

#### Testing CppInterOp 
To test the built CppInterOp execute the following command in the CppInterOP build folder on Linux and MacOS
```
cmake --build . --target check-cppinterop --parallel $(nproc --all)
```
and
```
cmake --build . --target check-cppinterop --parallel $env:ncpus
```
on Windows. Now go back to the top level directory in which your building CppInterOP. On Linux and MacOS you do this by executing
```
cd ../..
```
and
```
cd ..\..
```
on Windows. Now you are in a position to install cppyy following the instructions below.

#### Building and Install cppyy-backend

Cd into the cppyy-backend directory, build it and copy library files into `python/cppyy-backend` directory:

```
cd cppyy-backend
mkdir -p python/cppyy_backend/lib build 
cd build
cmake -DCppInterOp_DIR=$CPPINTEROP_DIR ..
cmake --build .
```
If on a linux system now execute the following command
```
cp libcppyy-backend.so ../python/cppyy_backend/lib/
```
and if on MacOS execute the following command
```
cp libcppyy-backend.dylib ../python/cppyy_backend/lib/
```

Note go back to the top level build directory
```
cd ../..
```

#### Install CPyCppyy

Create virtual environment and activate it:
```
python3 -m venv .venv
source .venv/bin/activate
```

```
git clone --depth=1 https://github.com/compiler-research/CPyCppyy.git
mkdir CPyCppyy/build
cd CPyCppyy/build
cmake ..
cmake --build .
```

Note down the path to the `build` directory as `CPYCPPYY_DIR`:
```
export CPYCPPYY_DIR=$PWD
cd ../..
```

#### Install cppyy

```
git clone --depth=1 https://github.com/compiler-research/cppyy.git
cd cppyy
python -m pip install --upgrade . --no-deps
cd ..
```

#### Run cppyy

Each time you want to run cppyy you need to:
Activate the virtual environment
```
source .venv/bin/activate
```

Now you can `import cppyy` in `python`
```
python -c "import cppyy"
```

#### Run cppyy tests

**Follow the steps in Run cppyy.** Change to the test directory, make the library files and run pytest:
```
cd cppyy/test
make all
python -m pip install pytest
python -m pytest -sv
```

---
Further Reading: [C++ Language Interoperability Layer](https://compiler-research.org/libinterop/)

[^1]: Cling is an interpretive Compiler for C++.
[^2]: Clang-REPL is an interactive C++ interpreter that enables incremental 
compilation. 
[^3]: LLVM is a Compiler Framework. It is a collection of modular compiler 
and toolchain technologies. 
