set(DLM
  DynamicLibraryManager.cpp
  DynamicLibraryManagerSymbol.cpp
  Paths.cpp
)
if (USE_CLING)
  set(LLVM_OPTIONAL_SOURCES ${LLVM_OPTIONAL_SOURCES} ${DLM})
  set(DLM)
endif(USE_CLING)
if (USE_REPL)
  #Use DML optional sources
endif(USE_REPL)

add_llvm_library(clangCppInterOp OBJECT
  CppInterOp.cpp
  ${DLM}
 )

set_source_files_properties(CppInterOp.cpp
  PROPERTIES COMPILE_DEFINITIONS "LLVM_BINARY_DIR=\"${LLVM_BINARY_DIR}\"")

