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
