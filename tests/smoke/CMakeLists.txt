cmake_minimum_required(VERSION 3.20)
project(CppInterOpSmokeTest CXX)

add_executable(smoke_test smoke_test.cpp)
target_link_libraries(smoke_test PRIVATE CppInterOp)

# For Emscripten builds, generate HTML and set memory
if(EMSCRIPTEN)
    set_target_properties(smoke_test PROPERTIES
        LINK_FLAGS "-s WASM=1 -s TOTAL_MEMORY=67108864"
        SUFFIX ".html"
    )
endif()