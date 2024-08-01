set(LLVM_LINK_COMPONENTS
  ${LLVM_TARGETS_TO_BUILD}
  BinaryFormat
  Core
  Object
  OrcJit
  Support
)
# FIXME: Investigate why this needs to be conditionally included.
if ("LLVMFrontendDriver" IN_LIST LLVM_AVAILABLE_LIBS)
  list(APPEND LLVM_LINK_COMPONENTS  FrontendDriver)
endif()
if ("LLVMOrcDebugging" IN_LIST LLVM_AVAILABLE_LIBS)
  list(APPEND LLVM_LINK_COMPONENTS OrcDebugging)
endif()

set(DLM
  DynamicLibraryManager.cpp
  DynamicLibraryManagerSymbol.cpp
  Paths.cpp
)
if (USE_CLING)
  set(LLVM_OPTIONAL_SOURCES ${LLVM_OPTIONAL_SOURCES} ${DLM})
  set(DLM)
endif(USE_CLING)
if (USE_REPL)
  #Use DML optional sources
endif(USE_REPL)

if (USE_CLING)
  set(cling_clang_interp clingInterpreter)
endif()
if (USE_REPL)
  set(cling_clang_interp clangInterpreter)
endif()

set(link_libs
  ${cling_clang_interp}
  clangAST
  clangBasic
  clangFrontend
  clangLex
  clangSema
  )

 if(NOT WIN32)
  list(APPEND link_libs dl)
 endif()

# Get rid of libLLVM-X.so which is appended to the list of static libraries.
if (LLVM_LINK_LLVM_DYLIB)
  set(new_libs ${link_libs})
  set(libs ${new_libs})
  while(NOT "${new_libs}" STREQUAL "")
    foreach(lib ${new_libs})
      if(TARGET ${lib})
        get_target_property(transitive_libs ${lib} INTERFACE_LINK_LIBRARIES)
        if (NOT transitive_libs)
          continue()
        endif()
        foreach(transitive_lib ${transitive_libs})
          get_target_property(lib_type ${transitive_lib} TYPE)
          if("${lib_type}" STREQUAL "STATIC_LIBRARY")
            list(APPEND static_transitive_libs ${transitive_lib})
          else()
            # Filter our libLLVM.so and friends.
            continue()
          endif()
          if(NOT ${transitive_lib} IN_LIST libs)
            list(APPEND newer_libs ${transitive_lib})
            list(APPEND libs ${transitive_lib})
          endif()
        endforeach(transitive_lib)
        # Update the target properties with the list of only static libraries.
        set_target_properties(${lib} PROPERTIES INTERFACE_LINK_LIBRARIES "${static_transitive_libs}")
        set(static_transitive_libs "")
      endif()
    endforeach(lib)
    set(new_libs ${newer_libs})
    set(newer_libs "")
  endwhile()
  # We just got rid of the libLLVM.so and other components shipped as shared
  # libraries, we need to make up for the missing dependency.
  list(APPEND LLVM_LINK_COMPONENTS
    Coverage
    FrontendHLSL
    LTO
    )
  # We will need to append the missing dependencies to pull in the right
  # LLVM library dependencies. 
  list(APPEND link_libs
    clangCodeGen
    clangStaticAnalyzerCore
    )
endif(LLVM_LINK_LLVM_DYLIB)

add_llvm_library(clangCppInterOp
  DISABLE_LLVM_LINK_LLVM_DYLIB
  CppInterOp.cpp
  CXCppInterOp.cpp
  ${DLM}
  LINK_LIBS
  ${link_libs}
 )

string(REPLACE ";" "\;" _VER CPPINTEROP_VERSION)
set_source_files_properties(CppInterOp.cpp PROPERTIES COMPILE_DEFINITIONS
  "LLVM_BINARY_DIR=\"${LLVM_BINARY_DIR}\";CPPINTEROP_VERSION=\"${_VAR}\""
)
