#include "Utils.h"
#include "CppInterOp/CppInterOp.h"

#include "clang/Basic/Version.h"

#include "llvm/ADT/SmallString.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"

#include "gtest/gtest.h"

// This function isn't referenced outside its translation unit, but it
// can't use the "static" keyword because its address is used for
// GetMainExecutable (since some platforms don't support taking the
// address of main, and some platforms can't implement GetMainExecutable
// without being given the address of a function in the main executable).
std::string GetExecutablePath(const char* Argv0) {
  // This just needs to be some symbol in the binary; C++ doesn't
  // allow taking the address of ::main however.
  void* MainAddr = (void*)intptr_t(GetExecutablePath);
  return llvm::sys::fs::getMainExecutable(Argv0, MainAddr);
}

TYPED_TEST(CPPINTEROP_TEST_MODE, DynamicLibraryManager_Sanity) {
#ifdef EMSCRIPTEN
  GTEST_SKIP() << "Test fails for Emscipten builds";
#endif

#if CLANG_VERSION_MAJOR == 20 && defined(CPPINTEROP_USE_CLING) &&              \
    defined(_WIN32)
  GTEST_SKIP() << "Test fails with Cling on Windows";
#endif
  if (TypeParam::isOutOfProcess)
    GTEST_SKIP() << "Test fails for OOP JIT builds";

  EXPECT_TRUE(TestFixture::CreateInterpreter());
  EXPECT_FALSE(Cpp::GetFunctionAddress("ret_zero"));

  std::string BinaryPath = GetExecutablePath(/*Argv0=*/nullptr);
  llvm::StringRef Dir = llvm::sys::path::parent_path(BinaryPath);
  Cpp::AddSearchPath(Dir.str().c_str());

  // FIXME: dlsym on mach-o takes the C-level name, however, the macho-o format
  // adds an additional underscore (_) prefix to the lowered names. Figure out
  // how to harmonize that API.
#ifdef __APPLE__
  std::string PathToTestSharedLib =
      Cpp::SearchLibrariesForSymbol("_ret_zero", /*system_search=*/false);
#else
  std::string PathToTestSharedLib =
      Cpp::SearchLibrariesForSymbol("ret_zero", /*system_search=*/false);
#endif // __APPLE__

  EXPECT_STRNE("", PathToTestSharedLib.c_str())
      << "Cannot find: '" << PathToTestSharedLib << "' in '" << Dir.str()
      << "'";

  EXPECT_TRUE(Cpp::LoadLibrary(PathToTestSharedLib.c_str()));
  // Force ExecutionEngine to be created.
  Cpp::Process("");
  // FIXME: Conda returns false to run this code on osx.
#ifndef __APPLE__
  EXPECT_TRUE(Cpp::GetFunctionAddress("ret_zero"));
#endif //__APPLE__

  Cpp::UnloadLibrary("TestSharedLib");
  // We have no reliable way to check if it was unloaded because posix does not
  // require the library to be actually unloaded but just the handle to be
  // invalidated...
  // EXPECT_FALSE(Cpp::GetFunctionAddress("ret_zero"));
}

TYPED_TEST(CPPINTEROP_TEST_MODE, DynamicLibraryManager_BasicSymbolLookup) {
#ifndef EMSCRIPTEN
  GTEST_SKIP() << "This test is only intended for Emscripten builds.";
#endif
  if (TypeParam::isOutOfProcess)
    GTEST_SKIP() << "Test fails for OOP JIT builds";

  ASSERT_TRUE(TestFixture::CreateInterpreter());
  EXPECT_FALSE(Cpp::GetFunctionAddress("ret_zero"));

  // Load the library manually. Use known preload path (MEMFS path)
  const char* wasmLibPath = "libTestSharedLib.so";  // Preloaded path in MEMFS
  EXPECT_TRUE(Cpp::LoadLibrary(wasmLibPath, false));

  Cpp::Process("");

  void* Addr = Cpp::GetFunctionAddress("ret_zero");
  EXPECT_NE(Addr, nullptr) << "Symbol 'ret_zero' not found after dlopen.";

  using RetZeroFn = int (*)();
  auto Fn = reinterpret_cast<RetZeroFn>(Addr);
  EXPECT_EQ(Fn(), 0);
}

// Weak globals a loaded library already defines -- header-defined singletons:
// a function-local static in an inline function and a C++17 inline static
// data member -- must be bound by jitted code rather than materialized as
// duplicates (compat::bindProcessWeakGlobals). Otherwise the library and the
// JIT each hold their own instance of what the program means to be one
// singleton, and teardown destroys it twice.
TYPED_TEST(CPPINTEROP_TEST_MODE, DynamicLibraryManager_SharedWeakGlobals) {
#ifdef EMSCRIPTEN
  GTEST_SKIP() << "Test not intended for Emscripten builds";
#endif
#ifdef CPPINTEROP_USE_CLING
  GTEST_SKIP() << "bindProcessWeakGlobals is wired into the clang-repl "
                  "CppInternal::Interpreter path, not cling's interpreter";
#endif
#if defined(_WIN32) || defined(__APPLE__)
  GTEST_SKIP() << "the dlsym-based weak-global binding targets ELF";
#endif
  if (TypeParam::isOutOfProcess)
    GTEST_SKIP() << "the dlsym-based weak-global binding is in-process only";

  ASSERT_TRUE(TestFixture::CreateInterpreter());

  // Resolve the library by path (SearchLibrariesForSymbol cannot run twice in
  // one process, and DynamicLibraryManager_Sanity already used it): beside
  // the test binary (cmake), or in the working directory (bazel).
  std::string BinaryPath = GetExecutablePath(/*Argv0=*/nullptr);
  llvm::StringRef Dir = llvm::sys::path::parent_path(BinaryPath);
  std::string PathToTestSharedLib;
  for (const char* Candidate : {"libTestSharedLib.so", "TestSharedLib.so"}) {
    llvm::SmallString<256> P(Dir);
    llvm::sys::path::append(P, Candidate);
    if (llvm::sys::fs::exists(P)) {
      PathToTestSharedLib = P.str().str();
      break;
    }
    if (llvm::sys::fs::exists(Candidate)) {
      PathToTestSharedLib = Candidate;
      break;
    }
  }
  ASSERT_STRNE("", PathToTestSharedLib.c_str());
  ASSERT_TRUE(Cpp::LoadLibrary(PathToTestSharedLib.c_str()));
  Cpp::Process(""); // force the execution engine into existence

  // Repeat TestSharedLib.h's singleton definitions in the JIT (the mangled
  // names must match the library's) and take each side's addresses.
  ASSERT_FALSE(Cpp::Process(R"(
    struct SingletonFixture {
      static SingletonFixture& get() {
        static SingletonFixture instance;
        return instance;
      }
      static inline int s_inline_member = 0;
      int value = 0;
    };
    extern "C" void* singleton_probe_jit_meyers_addr() {
      return &SingletonFixture::get();
    }
    extern "C" void* singleton_probe_jit_member_addr() {
      return &SingletonFixture::s_inline_member;
    }
    extern "C" void singleton_probe_jit_set(int v) {
      SingletonFixture::get().value = v;
      SingletonFixture::s_inline_member = v + 1;
    }
  )"));

  auto GetFn = [](const char* Name) {
    void* A = Cpp::GetFunctionAddress(Name);
    EXPECT_NE(A, nullptr) << Name;
    return A;
  };
  // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
  auto* JitMeyersAddr =
      reinterpret_cast<void* (*)()>(GetFn("singleton_probe_jit_meyers_addr"));
  auto* JitMemberAddr =
      reinterpret_cast<void* (*)()>(GetFn("singleton_probe_jit_member_addr"));
  auto* JitSet =
      reinterpret_cast<void (*)(int)>(GetFn("singleton_probe_jit_set"));
  auto* AotMeyersAddr =
      reinterpret_cast<void* (*)()>(GetFn("singleton_fixture_meyers_addr"));
  auto* AotMeyersValue =
      reinterpret_cast<int (*)()>(GetFn("singleton_fixture_meyers_value"));
  auto* AotMemberAddr =
      reinterpret_cast<void* (*)()>(GetFn("singleton_fixture_member_addr"));
  auto* AotMemberValue =
      reinterpret_cast<int (*)()>(GetFn("singleton_fixture_member_value"));
  // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)

  // One instance, not two: both flavors' addresses agree across the boundary.
  EXPECT_EQ(JitMeyersAddr(), AotMeyersAddr());
  EXPECT_EQ(JitMemberAddr(), AotMemberAddr());

  // And mutations through the jitted side are visible to the library.
  JitSet(41);
  EXPECT_EQ(AotMeyersValue(), 41);
  EXPECT_EQ(AotMemberValue(), 42);
}
