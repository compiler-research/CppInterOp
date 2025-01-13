if(EMSCRIPTEN)
# Without this it tries compiling to static library and get a warning
set_property(GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS TRUE)
set(CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS "-s SIDE_MODULE=1")
set(CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS "-s SIDE_MODULE=1")
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
# Without this you get an error that -gc-sections and -r cannot be used together. 
# Not sure I have picked the correct one to keep
target_link_options(clangCppInterOp PRIVATE
    PUBLIC "SHELL: -Wl,--no-gc-sections -s LINKABLE=1 -s WASM_BIGINT"
  )
endif()
set_target_properties(TestSharedLib PROPERTIES FOLDER "Tests")
