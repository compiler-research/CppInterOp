set(LLVM_LINK_COMPONENTS
  Support
  )

add_interop_unittest(InterOpTests
  EnumReflectionTest.cpp
  FunctionReflectionTest.cpp
  InterpreterTest.cpp
  ScopeReflectionTest.cpp
  TypeReflectionTest.cpp
  Utils.cpp
  VariableReflectionTest.cpp
  )
target_link_libraries(InterOpTests
  PRIVATE
  clangInterOp
  )

export_executable_symbols(InterOpTests)
