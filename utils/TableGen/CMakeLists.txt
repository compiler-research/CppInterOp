set(LLVM_LINK_COMPONENTS Support TableGen)

add_tablegen(cppinterop-tblgen CPPINTEROP
  CppInterOpEmitter.cpp
  TableGen.cpp
)
if(EMSCRIPTEN)
  set(CPPINTEROP_SOURCE_DIR "${PROJECT_SOURCE_DIR}/lib/CppInterOp")

  configure_file(
      pre.js
      "${CMAKE_CURRENT_BINARY_DIR}/pre.js"
      @ONLY
  )

  target_link_options(cppinterop-tblgen PRIVATE
    "-sFORCE_FILESYSTEM=1"
    "--pre-js" "${CMAKE_CURRENT_BINARY_DIR}/pre.js"
  )
endif()
