add_llvm_library(clangInterOp OBJECT
  InterOp.cpp
 )

set_source_files_properties(InterOp.cpp
  PROPERTIES COMPILE_DEFINITIONS "LLVM_BINARY_DIR=\"${LLVM_BINARY_DIR}\"")

