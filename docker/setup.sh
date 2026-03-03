# Build LLVM
cd llvm-project
mkdir build 
cd build 
cmake -DLLVM_ENABLE_PROJECTS=clang                                  \
                -DLLVM_TARGETS_TO_BUILD="host;NVPTX"                \
                -DCMAKE_BUILD_TYPE=Release                          \
                -DLLVM_ENABLE_ASSERTIONS=ON                         \
                -DCLANG_ENABLE_STATIC_ANALYZER=OFF                  \
                -DCLANG_ENABLE_ARCMT=OFF                            \
                -DCLANG_ENABLE_FORMAT=OFF                           \
                -DCLANG_ENABLE_BOOTSTRAP=OFF                        \
                ../llvm
cmake --build . --target clang clang-repl --parallel $(nproc --all)
cmake --build . --target clang clang-repl llvm-jitlink-executor orc_rt-x86_64 --parallel $(nproc --all)

cd ..
export LLVM_DIR=$PWD

cd ..

export CB_PYTHON_DIR="$PWD/cppyy-backend/python"
export CPPINTEROP_DIR="$CB_PYTHON_DIR/cppyy_backend"
export CPLUS_INCLUDE_PATH="${CPLUS_INCLUDE_PATH}:${LLVM_DIR}/llvm/include:${LLVM_DIR}/clang/include:${LLVM_DIR}/build/include:${LLVM_DIR}/build/tools/clang/include"

# Build CppInterOp
mkdir CppInterOp/build/
cd CppInterOp/build/
cmake -DBUILD_SHARED_LIBS=ON -DLLVM_DIR=$LLVM_DIR/build/lib/cmake/llvm -DClang_DIR=$LLVM_DIR/build/lib/cmake/clang -DCMAKE_INSTALL_PREFIX=$CPPINTEROP_DIR -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --target install --parallel $(nproc --all)

cd ../..

# Build cppyy-backend
cd cppyy-backend
mkdir -p python/cppyy_backend/lib build 
cd build
cmake -DCppInterOp_DIR=$CPPINTEROP_DIR ..
cmake --build .
cp libcppyy-backend.so ../python/cppyy_backend/lib/

# Build CPyCppyy
virtualenv .venv
source .venv/bin/activate
git clone --depth=1 https://github.com/compiler-research/CPyCppyy.git
mkdir CPyCppyy/build
cd CPyCppyy/build
cmake ..
cmake --build .
export CPYCPPYY_DIR=$PWD

cd ../..

# Install Cppyy
export PYTHONPATH=$PYTHONPATH:$CPYCPPYY_DIR:$CB_PYTHON_DIR
git clone --depth=1 https://github.com/compiler-research/cppyy.git
cd cppyy
pip install setuptools
python3 -m pip install --upgrade . --no-deps --no-build-isolation
cd ..