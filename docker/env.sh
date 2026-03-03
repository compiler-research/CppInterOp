export PARENT_DIR="/compiler-research"
export LLVM_DIR="$PARENT_DIR/llvm-project"
export CB_PYTHON_DIR="$PARENT_DIR/cppyy-backend/python"
export CPPINTEROP_DIR="$CB_PYTHON_DIR/cppyy_backend"
export CPLUS_INCLUDE_PATH="${CPLUS_INCLUDE_PATH}:${LLVM_DIR}/llvm/include:${LLVM_DIR}/clang/include:${LLVM_DIR}/build/include:${LLVM_DIR}/build/tools/clang/include"
export CPYCPPYY_DIR="$PARENT_DIR/CPyCppyy"
export PYTHONPATH=$PYTHONPATH:$CPYCPPYY_DIR:$CB_PYTHON_DIR