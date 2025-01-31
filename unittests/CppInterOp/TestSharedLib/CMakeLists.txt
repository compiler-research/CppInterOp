if(EMSCRIPTEN)
# Without this it tries compiling to static library and get a warning
set_property(GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS TRUE)
set(CMAKE_STRIP FALSE)
endif()

add_llvm_library(TestSharedLib
  SHARED
  DISABLE_LLVM_LINK_LLVM_DYLIB
  BUILDTREE_ONLY
  TestSharedLib.cpp)
# Put TestSharedLib next to the unit test executable.
set_output_directory(TestSharedLib
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/unittests/bin/$<CONFIG>/
  LIBRARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/unittests/bin/$<CONFIG>/
  )


if(EMSCRIPTEN)
  set_target_properties(TestSharedLib PROPERTIES
    NO_SONAME 1
    COMPILE_FLAGS "-s SIDE_MODULE=1"
    LINK_FLAGS "-s WASM_BIGINT  -s SIDE_MODULE=1 -Wl,--export-all"
    SUFFIX ".wasm"
  )
endif()

set_target_properties(TestSharedLib PROPERTIES FOLDER "Tests")
