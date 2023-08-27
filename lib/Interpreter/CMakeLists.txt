set(LLVM_LINK_COMPONENTS
  ${LLVM_TARGETS_TO_BUILD}
  BinaryFormat
  Core
  Object
  OrcJIT
  Support
)

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

if (USE_CLING)
  set(cling_clang_interp clingInterpreter)
endif()
if (USE_REPL)
  set(cling_clang_interp clangInterpreter)
endif()

target_link_libraries(clangCppInterOp PRIVATE
  ${cling_clang_interp}
  clangAST
  clangBasic
  clangLex
  clangSema
  dl
)

set_source_files_properties(CppInterOp.cpp
  PROPERTIES COMPILE_DEFINITIONS "LLVM_BINARY_DIR=\"${LLVM_BINARY_DIR}\"")

