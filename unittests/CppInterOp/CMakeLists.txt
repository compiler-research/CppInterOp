set(LLVM_LINK_COMPONENTS
  Support
  )

add_cppinterop_unittest(CppInterOpTests
  EnumReflectionTest.cpp
  FunctionReflectionTest.cpp
  InterpreterTest.cpp
  ScopeReflectionTest.cpp
  TypeReflectionTest.cpp
  Utils.cpp
  VariableReflectionTest.cpp
  )

if (USE_CLING)
  set(cling_clang_interp "clingInterpreter")
endif()
if (USE_REPL)
  set(cling_clang_interp "clangInterpreter")
endif()


target_link_libraries(CppInterOpTests
  PRIVATE
  ${cling_clang_interp}
  clangCppInterOp
  clangAST
  clangBasic
  clangLex
  )
