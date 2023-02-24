add_llvm_library(clangCppInterOp OBJECT
  CppInterOp.cpp
 )

set_source_files_properties(CppInterOp.cpp
  PROPERTIES COMPILE_DEFINITIONS "LLVM_BINARY_DIR=\"${LLVM_BINARY_DIR}\"")

