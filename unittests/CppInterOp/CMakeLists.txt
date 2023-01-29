set(LLVM_LINK_COMPONENTS
  Support
  )

add_cppinterop_unittest(CppInterOpTests
  ScopeReflectionTest.cpp
  )

clang_target_link_libraries(CppInterOpTests
  PRIVATE
  clingInterpreter
  clangAST
  clangBasic
  clangLex
  )
