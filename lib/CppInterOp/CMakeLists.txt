if(EMSCRIPTEN)
  set_property(GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS TRUE)
  set(LLVM_LINK_COMPONENTS "")
else()
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
endif()
  set(DLM
    DynamicLibraryManager.cpp
    DynamicLibraryManagerSymbol.cpp
    Paths.cpp
  )
  if (CPPINTEROP_USE_CLING)
    set(LLVM_OPTIONAL_SOURCES ${LLVM_OPTIONAL_SOURCES} ${DLM})
    set(DLM)
  endif(CPPINTEROP_USE_CLING)
  if (CPPINTEROP_USE_REPL)
    #Use DML optional sources
  endif(CPPINTEROP_USE_REPL)

  if (CPPINTEROP_USE_CLING)
    set(cling_clang_interp clingInterpreter)
  endif()
  if (CPPINTEROP_USE_REPL)
    set(cling_clang_interp clangInterpreter)
  endif()

if(EMSCRIPTEN)
  set(link_libs
    ${cling_clang_interp}
    )
else()
  set(link_libs
    ${cling_clang_interp}
    clangAST
    clangBasic
    clangFrontend
    clangLex
    clangSema
    )
endif()

  if(NOT WIN32 AND NOT EMSCRIPTEN)
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


if(EMSCRIPTEN)
  # FIXME: When dynamically linking the Emscripten shared library to the
  # unit tests main_module you get errors due to undefined symbols. The reading of the file
  # below into a SYMBOLS_LIST variable is a temporary workaround that exports the undefined
  # symbols from the shared library, until it can be determined why they are not being exported already.
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/exports.ld" SYMBOLS_LIST)

  # Replace newlines with spaces
  string(REPLACE "\n" " " SYMBOLS_LIST "${SYMBOLS_LIST}")

  set_target_properties(clangCppInterOp
    PROPERTIES NO_SONAME 1
  )
  target_link_options(clangCppInterOp
    PRIVATE "SHELL: -s WASM_BIGINT"
    PRIVATE "SHELL: -s SIDE_MODULE=1"
    PRIVATE "SHELL: ${SYMBOLS_LIST}"
  )
  if (CPPINTEROP_ENABLE_TESTING)
    # When compiling Emscripten tests the shared library it links to is expected to be in the same folder as the compiled Javascript
    add_custom_command(TARGET clangCppInterOp POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:clangCppInterOp> ${CMAKE_BINARY_DIR}/unittests/CppInterOp/
    )
  endif(CPPINTEROP_ENABLE_TESTING)

endif()

target_compile_definitions(clangCppInterOp PUBLIC "_CINDEX_LIB_") # workaround for the use of `CINDEX_LINKAGE`

string(REPLACE ";" "\;" _VER CPPINTEROP_VERSION)
set_source_files_properties(CppInterOp.cpp PROPERTIES COMPILE_DEFINITIONS
  "LLVM_BINARY_DIR=\"${LLVM_BINARY_DIR}\";CPPINTEROP_VERSION=\"${_VAR}\""
)

if(MSVC)
  target_link_options(clangCppInterOp PRIVATE
    "/EXPORT:?write@raw_ostream@llvm@@QEAAAEAV12@PEBD_K@Z"
    "/EXPORT:?errs@llvm@@YAAEAVraw_fd_ostream@1@XZ"
    "/EXPORT:?DestroyDataAndMakeUninit@APValue@clang@@AEAAXXZ"
    "/EXPORT:?dbgs@llvm@@YAAEAVraw_ostream@1@XZ"
    "/EXPORT:?fatalUncheckedError@Error@llvm@@AEBAXXZ"
    "/EXPORT:?logAllUnhandledErrors@llvm@@YAXVError@1@AEAVraw_ostream@1@VTwine@1@@Z"
    "/EXPORT:?isFloatingType@Type@clang@@QEBA_NXZ"
    "/EXPORT:?getDecl@TagType@clang@@QEBAPEAVTagDecl@2@XZ"
    "/EXPORT:?hasDefiningAttr@Decl@clang@@QEBA_NXZ"
    "/EXPORT:?getNumParams@FunctionDecl@clang@@QEBAIXZ"
    "/EXPORT:?getTemplateInstantiationPattern@FunctionDecl@clang@@QEBAPEAV12@_N@Z"
    "/EXPORT:?getTemplateSpecializationArgs@FunctionDecl@clang@@QEBAPEBVTemplateArgumentList@2@XZ"
    "/EXPORT:?initSlowCase@APInt@llvm@@AEAAX_K_N@Z"
    "/EXPORT:?compare@APInt@llvm@@AEBAHAEBV12@@Z"
    "/EXPORT:?compareSigned@APInt@llvm@@AEBAHAEBV12@@Z"
    "/EXPORT:?compareValues@APSInt@llvm@@SAHAEBV12@0@Z"
    "/EXPORT:??1raw_ostream@llvm@@UEAA@XZ"
    "/EXPORT:??1raw_string_ostream@llvm@@UEAA@XZ"
    "/EXPORT:?write_impl@raw_string_ostream@llvm@@EEAAXPEBD_K@Z"
    "/EXPORT:?changeColor@raw_ostream@llvm@@UEAAAEAV12@W4Colors@12@_N1@Z"
    "/EXPORT:?resetColor@raw_ostream@llvm@@UEAAAEAV12@XZ"
    "/EXPORT:?reverseColor@raw_ostream@llvm@@UEAAAEAV12@XZ"
    "/EXPORT:?preferred_buffer_size@raw_ostream@llvm@@MEBA_KXZ"
    "/EXPORT:?SetBufferAndMode@raw_ostream@llvm@@AEAAXPEAD_KW4BufferKind@12@@Z"
    "/EXPORT:?flush_nonempty@raw_ostream@llvm@@AEAAXXZ"
    "/EXPORT:?anchor@raw_ostream@llvm@@EEAAXXZ"
    "/EXPORT:?maybeMangleDeclName@compat@@YAXAEBVGlobalDecl@clang@@AEAV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z"
"/EXPORT:?grow_pod@?$SmallVectorBase@I@llvm@@IEAAXPEAX_K1@Z"
"/EXPORT:??0APInt@llvm@@QEAA@IV?$ArrayRef@_K@1@@Z"
"/EXPORT:?sext@APInt@llvm@@QEBA?AV12@I@Z"
"/EXPORT:?zext@APInt@llvm@@QEBA?AV12@I@Z"
"/EXPORT:?allocate_buffer@llvm@@YAPEAX_K0@Z"
"/EXPORT:?RehashTable@StringMapImpl@llvm@@IEAAII@Z"
"/EXPORT:?LookupBucketFor@StringMapImpl@llvm@@IEAAIVStringRef@2@I@Z"
"/EXPORT:?hash@StringMapImpl@llvm@@SAIVStringRef@2@@Z"
"/EXPORT:?getUnqualifiedDesugaredType@Type@clang@@QEBAPEBV12@XZ"
"/EXPORT:?getASTContext@Decl@clang@@QEBAAEAVASTContext@2@XZ"
"/EXPORT:?getAttrs@Decl@clang@@QEBAAEBV?$SmallVector@PEAVAttr@clang@@$03@llvm@@XZ"
"/EXPORT:?lookup@DeclContext@clang@@QEBA?AVDeclContextLookupResult@2@VDeclarationName@2@@Z"
"/EXPORT:?isThisDeclarationADefinition@VarDecl@clang@@QEBA?AW4DefinitionKind@12@AEAVASTContext@2@@Z"
"/EXPORT:?isInjectedClassName@RecordDecl@clang@@QEBA_NXZ"
"/EXPORT:?getTypeDeclTypeSlow@ASTContext@clang@@AEBA?AVQualType@2@PEBVTypeDecl@2@@Z"
"/EXPORT:?getComplexType@ASTContext@clang@@QEBA?AVQualType@2@V32@@Z"
"/EXPORT:?createMangleContext@ASTContext@clang@@QEAAPEAVMangleContext@2@PEBVTargetInfo@2@@Z"
"/EXPORT:?getSpecializations@ClassTemplateDecl@clang@@IEBAAEAV?$FoldingSetVector@VClassTemplateSpecializationDecl@clang@@V?$SmallVector@PEAVClassTemplateSpecializationDecl@clang@@$07@llvm@@@llvm@@XZ"
"/EXPORT:?getSpecializedTemplate@VarTemplateSpecializationDecl@clang@@QEBAPEAVVarTemplateDecl@2@XZ"
"/EXPORT:?shouldMangleDeclName@MangleContext@clang@@QEAA_NPEBVNamedDecl@2@@Z"
"/EXPORT:?mangleName@MangleContext@clang@@QEAAXVGlobalDecl@2@AEAVraw_ostream@llvm@@@Z"
"/EXPORT:?getStdNamespace@Sema@clang@@QEBAPEAVNamespaceDecl@2@XZ"
"/EXPORT:?castToDeclContext@Decl@clang@@SAPEAVDeclContext@2@PEBV12@@Z"
"/EXPORT:?decls_begin@DeclContext@clang@@QEBA?AVdecl_iterator@12@XZ"
"/EXPORT:?classof@DeclContext@clang@@SA_NPEBVDecl@2@@Z"
"/EXPORT:?Parse@Interpreter@clang@@QEAA?AV?$Expected@AEAUPartialTranslationUnit@clang@@@llvm@@VStringRef@4@@Z"
"/EXPORT:?Execute@Interpreter@clang@@QEAA?AVError@llvm@@AEAUPartialTranslationUnit@2@@Z"
"/EXPORT:?getLValueReferenceType@ASTContext@clang@@QEBA?AVQualType@2@V32@_N@Z"
"/EXPORT:?getCompilerInstance@Interpreter@clang@@QEAAPEAVCompilerInstance@2@XZ"
"/EXPORT:?RunningOnValgrind@sys@llvm@@YA_NXZ"
"/EXPORT:?grow_pod@?$SmallVectorBase@_K@llvm@@IEAAXPEAX_K1@Z"
"/EXPORT:?access@fs@sys@llvm@@YA?AVerror_code@std@@AEBVTwine@3@W4AccessMode@123@@Z"
"/EXPORT:?CreateCpp@IncrementalCompilerBuilder@clang@@QEAA?AV?$Expected@V?$unique_ptr@VCompilerInstance@clang@@U?$default_delete@VCompilerInstance@clang@@@std@@@std@@@llvm@@XZ"
"/EXPORT:?create@Interpreter@clang@@SA?AV?$Expected@V?$unique_ptr@VInterpreter@clang@@U?$default_delete@VInterpreter@clang@@@std@@@std@@@llvm@@V?$unique_ptr@VCompilerInstance@clang@@U?$default_delete@VCompilerInstance@clang@@@std@@@std@@@Z"
"/EXPORT:__imp_?classof@CUDAGlobalAttr@clang@@SA_NPEBVAttr@2@@Z"
  )
endif()
