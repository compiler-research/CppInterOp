set(LLVM_LINK_COMPONENTS Support)

add_tablegen(cppinterop-tblgen CPPINTEROP
  CppInterOpEmitter.cpp
  TableGen.cpp
)
