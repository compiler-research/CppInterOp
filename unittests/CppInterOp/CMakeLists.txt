set(LLVM_LINK_COMPONENTS
  Support
  )

add_cppinterop_unittest(CppInterOpTests
  ScopeReflectionTest.cpp
  FunctionReflectionTest.cpp
  VariableReflectionTest.cpp
  )

target_link_libraries(CppInterOpTests
  PRIVATE
  clingInterpreter
  clangCppInterOp
  clangAST
  clangBasic
  clangLex
  )
