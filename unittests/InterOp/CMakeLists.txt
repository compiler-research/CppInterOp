set(LLVM_LINK_COMPONENTS
  Support
  )

add_interop_unittest(InterOpTests
  Utils.cpp
  ScopeReflectionTest.cpp
  FunctionReflectionTest.cpp
  VariableReflectionTest.cpp
  TypeReflectionTest.cpp
  EnumReflectionTest.cpp
  )

set_source_files_properties(Utils.cpp
  PROPERTIES COMPILE_DEFINITIONS "LLVM_BINARY_DIR=\"${LLVM_BINARY_DIR}\"")

target_link_libraries(InterOpTests
  PRIVATE
  clingInterpreter
  clangInterOp
  clangAST
  clangBasic
  clangLex
  )
