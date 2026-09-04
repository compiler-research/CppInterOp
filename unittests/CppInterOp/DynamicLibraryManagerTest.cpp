#include "Utils.h"
#include "CppInterOp/CppInterOp.h"

#include "clang/Basic/Version.h"

#include "llvm/ADT/SmallString.h"

#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"

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

// A failed dlopen must report the dlerror() text on stderr.
TYPED_TEST(CPPINTEROP_TEST_MODE, DynamicLibraryManager_LoadFailureReason) {
#if defined(EMSCRIPTEN) || defined(_WIN32)
  GTEST_SKIP() << "Test checks dlerror-style messages";
#endif
  if (TypeParam::isOutOfProcess)
    GTEST_SKIP() << "Test fails for OOP JIT builds";

  EXPECT_TRUE(TestFixture::CreateInterpreter());

  // A file that exists but is not a shared library makes dlopen fail.
  int FD = -1;
  llvm::SmallString<256> BadLib;
  ASSERT_FALSE(llvm::sys::fs::createTemporaryFile("bad-lib", "so", FD, BadLib));
  {
    llvm::raw_fd_ostream OS(FD, /*shouldClose=*/true);
    OS << "not a shared library";
  }

  testing::internal::CaptureStderr();
  EXPECT_FALSE(Cpp::LoadLibrary(BadLib.c_str(), /*lookup=*/false));
  std::string Err = testing::internal::GetCapturedStderr();
  // dlerror() names the failing file.
  EXPECT_NE(Err.find(llvm::sys::path::filename(BadLib).str()),
            std::string::npos)
      << "stderr was: '" << Err << "'";

  // With an out-parameter the reason goes there and not to stderr.
  std::string Reason = "stale";
  testing::internal::CaptureStderr();
  EXPECT_FALSE(Cpp::LoadLibrary(BadLib.c_str(), /*lookup=*/false, &Reason));
#ifdef CPPINTEROP_USE_CLING
  testing::internal::GetCapturedStderr(); // cling still prints its own text
#else
  EXPECT_EQ(testing::internal::GetCapturedStderr(), "");
#endif
  EXPECT_NE(Reason.find(llvm::sys::path::filename(BadLib).str()),
            std::string::npos)
      << "reason was: '" << Reason << "'";

  // A failed lookup reports that too.
  EXPECT_FALSE(
      Cpp::LoadLibrary("no-such-cppinterop-lib", /*lookup=*/true, &Reason));
  EXPECT_NE(Reason.find("no-such-cppinterop-lib"), std::string::npos)
      << "reason was: '" << Reason << "'";

  // Success clears a stale reason.
  std::string BinaryPath = GetExecutablePath(/*Argv0=*/nullptr);
  Cpp::AddSearchPath(llvm::sys::path::parent_path(BinaryPath).str().c_str());
  Reason = "stale";
  EXPECT_TRUE(Cpp::LoadLibrary("TestSharedLib", /*lookup=*/true, &Reason))
      << "reason was: '" << Reason << "'";
  EXPECT_EQ(Reason, "");

  EXPECT_FALSE(llvm::sys::fs::remove(BadLib));
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
