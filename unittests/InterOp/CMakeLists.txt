set(LLVM_LINK_COMPONENTS
  Support
  )

add_interop_unittest(InterOpTests
  ScopeReflectionTest.cpp
  FunctionReflectionTest.cpp
  VariableReflectionTest.cpp
  TypeReflectionTest.cpp
  )

target_link_libraries(InterOpTests
  PRIVATE
  clingInterpreter
  clangInterOp
  clangAST
  clangBasic
  clangLex
  )
