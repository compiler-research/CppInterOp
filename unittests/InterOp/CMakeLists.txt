set(LLVM_LINK_COMPONENTS
  Support
  )

add_interop_unittest(InterOpTests
  ScopeReflectionTest.cpp
  FunctionReflectionTest.cpp
  )

target_link_libraries(InterOpTests
  PRIVATE
  clingInterpreter
  clangInterOp
  clangAST
  clangBasic
  clangLex
  )
