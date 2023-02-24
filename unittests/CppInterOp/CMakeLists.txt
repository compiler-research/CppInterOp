set(LLVM_LINK_COMPONENTS
  Support
  )

add_cppinterop_unittest(CppInterOpTests
  Utils.cpp
  ScopeReflectionTest.cpp
  FunctionReflectionTest.cpp
  VariableReflectionTest.cpp
  TypeReflectionTest.cpp
  EnumReflectionTest.cpp
  )

target_link_libraries(CppInterOpTests
  PRIVATE
  clingInterpreter
  clangCppInterOp
  clangAST
  clangBasic
  clangLex
  )
