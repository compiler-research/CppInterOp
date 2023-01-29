set(LLVM_LINK_COMPONENTS
  Support
  )

add_interop_unittest(InterOpTests
  ScopeReflectionTest.cpp
  )

clang_target_link_libraries(InterOpTests
  PRIVATE
  clingInterpreter
  clangAST
  clangBasic
  clangLex
  )
