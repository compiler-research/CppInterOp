#include "gtest/gtest.h"

#include "clang/Interpreter/CppInterOp.h"


#include "llvm/ExecutionEngine/Orc/Core.h"
#include "llvm/ExecutionEngine/Orc/DebugUtils.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"

// Helper functions

// This function isn't referenced outside its translation unit, but it
// can't use the "static" keyword because its address is used for
// GetMainExecutable (since some platforms don't support taking the
// address of main, and some platforms can't implement GetMainExecutable
// without being given the address of a function in the main executable).
std::string GetExecutablePath(const char* Argv0, void* mainAddr = nullptr) {
  // This just needs to be some symbol in the binary; C++ doesn't
  // allow taking the address of ::main however.
  void* MainAddr = mainAddr ? mainAddr : (void*)intptr_t(GetExecutablePath);
  return llvm::sys::fs::getMainExecutable(Argv0, MainAddr);
}

// Mangle symbol name
static inline std::string MangleNameForDlsym(const std::string& name) {
  std::string nameForDlsym = name;
#if defined(R__MACOSX) || defined(R__WIN32)
  if (nameForDlsym[0] != '_')
    nameForDlsym.insert (0, 1, '_');
#endif //R__MACOSX
  return nameForDlsym;
}

#include "../../lib/Interpreter/CppInterOp.cpp"


// Tests

TEST(DynamicLibraryManagerTest, Sanity) {
  EXPECT_TRUE(Cpp::CreateInterpreter());
  EXPECT_FALSE(Cpp::GetFunctionAddress(MangleNameForDlsym("ret_zero").c_str()));

  std::string BinaryPath = GetExecutablePath(nullptr, nullptr);
  llvm::StringRef Dir = llvm::sys::path::parent_path(BinaryPath);
  Cpp::AddSearchPath(Dir.str().c_str());

  // Find and load library with "ret_zero" symbol defined and exported
  //
  // FIXME: dlsym on mach-o takes the C-level name, however, the macho-o format
  // adds an additional underscore (_) prefix to the lowered names. Figure out
  // how to harmonize that API.
  std::string PathToTestSharedLib =
      Cpp::SearchLibrariesForSymbol(MangleNameForDlsym("ret_zero").c_str(), /*system_search=*/false);
  EXPECT_STRNE("", PathToTestSharedLib.c_str())
      << "Cannot find: '" << PathToTestSharedLib << "' in '" << Dir.str() << "'";
  EXPECT_TRUE(Cpp::LoadLibrary(PathToTestSharedLib.c_str()));

  // Force ExecutionEngine to be created.
  Cpp::Process("");

  // FIXME: Conda returns false to run this code on osx.
  EXPECT_TRUE(Cpp::GetFunctionAddress(MangleNameForDlsym("ret_zero").c_str()));

  Cpp::UnloadLibrary("TestSharedLib");
  // We have no reliable way to check if it was unloaded because posix does not
  // require the library to be actually unloaded but just the handle to be
  // invalidated...
  // EXPECT_FALSE(Cpp::GetFunctionAddress(MangleNameForDlsym("ret_zero").c_str()));
}

TEST(DynamicLibraryManagerTest, LibrariesAutoload) {
  EXPECT_TRUE(Cpp::CreateInterpreter());

  // Autoload by default is OFF. Symbol search must fail.
  EXPECT_FALSE(Cpp::GetFunctionAddress(MangleNameForDlsym("ret_one").c_str()));

  // Libraries Search path by default is set to main executable directory.
  std::string BinaryPath = GetExecutablePath(nullptr, nullptr);
  llvm::StringRef Dir = llvm::sys::path::parent_path(BinaryPath);
  Cpp::AddSearchPath(Dir.str().c_str());

  // Find library with "ret_one" symbol defined and exported
  //
  // FIXME: dlsym on mach-o takes the C-level name, however, the macho-o format
  // adds an additional underscore (_) prefix to the lowered names. Figure out
  // how to harmonize that API. For now we use out minimal implementation of
  // helper function.
  std::string PathToTestSharedLib1 =
      Cpp::SearchLibrariesForSymbol(MangleNameForDlsym("ret_one").c_str(), /*system_search=*/false);
  // If result is "" then we cannot find this library.
  EXPECT_STRNE("", PathToTestSharedLib1.c_str())
      << "Cannot find: '" << PathToTestSharedLib1 << "' in '" << Dir.str() << "'";

  // Force ExecutionEngine to be created.
  Cpp::Process("");

  // Check default Autoload is OFF
  EXPECT_FALSE(Cpp::GetLibrariesAutoload());
  // Find symbol must fail (when auotload=off)
  EXPECT_FALSE(Cpp::GetFunctionAddress(MangleNameForDlsym("ret_one").c_str()));

  // Autoload turn ON
  Cpp::SetLibrariesAutoload(true);
  // Check autorum status (must be turned ON)
  EXPECT_TRUE(Cpp::GetLibrariesAutoload());

  // FIXME: Conda returns false to run this code on osx.
  EXPECT_TRUE(Cpp::GetFunctionAddress(MangleNameForDlsym("ret_one").c_str()));
  // Check for some symbols (exists and not exists)
  EXPECT_FALSE(Cpp::GetFunctionAddress(MangleNameForDlsym("ret_not_exist").c_str()));
  EXPECT_FALSE(Cpp::GetFunctionAddress(MangleNameForDlsym("ret_not_exist1").c_str()));
  EXPECT_TRUE(Cpp::GetFunctionAddress(MangleNameForDlsym("ret_two").c_str()));

  //Cpp::UnloadLibrary("TestSharedLib1");
  //// We have no reliable way to check if it was unloaded because posix does not
  //// require the library to be actually unloaded but just the handle to be
  //// invalidated...
  //EXPECT_FALSE(Cpp::GetFunctionAddress(MangleNameForDlsym("ret_one").c_str()));

  // Autoload turn OFF
  Cpp::SetLibrariesAutoload(false);
  // Check autorum status (must be turned OFF)
  EXPECT_FALSE(Cpp::GetLibrariesAutoload());
  EXPECT_TRUE(Cpp::GetFunctionAddress(MangleNameForDlsym("ret_one").c_str()));  // false if unload works

  // Autoload turn ON again
  Cpp::SetLibrariesAutoload(true);
  // Check autorum status (must be turned ON)
  EXPECT_TRUE(Cpp::GetLibrariesAutoload());
  EXPECT_TRUE(Cpp::GetFunctionAddress(MangleNameForDlsym("ret_one").c_str()));

  // Find library with "ret_1" symbol defined and exported
  std::string PathToTestSharedLib2 =
      Cpp::SearchLibrariesForSymbol(MangleNameForDlsym("ret_1").c_str(), /*system_search=*/false);
  // If result is "" then we cannot find this library.
  EXPECT_STRNE("", PathToTestSharedLib2.c_str())
      << "Cannot find: '" << PathToTestSharedLib2 << "' in '" << Dir.str() << "'";
  // Find symbol must success (when auotload=on)
  EXPECT_TRUE(Cpp::GetFunctionAddress(MangleNameForDlsym("ret_1").c_str()));
  // Find symbol must success (when auotload=on)
  EXPECT_TRUE(Cpp::GetFunctionAddress(MangleNameForDlsym("ret_2").c_str()));
}

TEST(DynamicLibraryManagerTest, LibrariesAutoloadExtraCoverage) {
  EXPECT_TRUE(Cpp::CreateInterpreter());

  // Libraries Search path by default is set to main executable directory.
  std::string BinaryPath = GetExecutablePath(nullptr, nullptr);
  llvm::StringRef Dir = llvm::sys::path::parent_path(BinaryPath);
  Cpp::AddSearchPath(Dir.str().c_str());

  // Force ExecutionEngine to be created.
  Cpp::Process("");

  // Autoload turn ON
  Cpp::SetLibrariesAutoload(true);
  // Check autorum status (must be turned ON)
  EXPECT_TRUE(Cpp::GetLibrariesAutoload());

  // Cover: MU getName()
  std::string res;
  llvm::raw_string_ostream rss(res);
  const llvm::orc::SymbolNameVector syms;
  Cpp::AutoLoadLibrarySearchGenerator::AutoloadLibraryMU MU("test", syms);
  rss << MU;
  EXPECT_STRNE("", rss.str().c_str()) << "MU problem!";

  // Cover: LoadLibrary error
  // if (DLM->loadLibrary(lib, false) != DynamicLibraryManager::LoadLibResult::kLoadLibSuccess) {
  //   LLVM_DEBUG(dbgs() << "MU: Failed to load library " << lib);
  //   string err = "MU: Failed to load library! " + lib;
  //   perror(err.c_str());
  // } else {
  // Find library with "ret_value" symbol defined and exported
  std::string PathToTestSharedLib3 =
      Cpp::SearchLibrariesForSymbol(MangleNameForDlsym("ret_val").c_str(), /*system_search=*/false);
  // If result is "" then we cannot find this library.
  EXPECT_STRNE("", PathToTestSharedLib3.c_str())
      << "Cannot find: '" << PathToTestSharedLib3 << "' in '" << Dir.str() << "'";
  // Remove library for simulate load error
  llvm::sys::fs::remove(PathToTestSharedLib3, true);
  EXPECT_TRUE(Cpp::GetLibrariesAutoload());
  // FIXME: Conda returns false to run this code on osx.
  EXPECT_FALSE(Cpp::GetFunctionAddress(MangleNameForDlsym("ret_val").c_str()));

  // Cover
  // } else {
  //   // Collect all failing symbols, delegate their responsibility and then
  //   // fail their materialization. R->defineNonExistent() sounds like it
  //   // should do that, but it's not implemented?!
  //   failedSymbols.insert(symbol);
  // TODO: implement test this case

  // Cover
  // if (!failedSymbols.empty()) {
  //   auto failingMR = R->delegate(failedSymbols);
  //   if (failingMR) {
  //     (*failingMR)->failMaterialization();
  // TODO: implement test this case

  // Cover
  // void discard(const llvm::orc::JITDylib &JD, const llvm::orc::SymbolStringPtr &Name) override {}
  // TODO: implement test this case

  // Cover
  // if (Path.empty()) {
  //   LLVM_DEBUG(dbgs() << "DynamicLibraryManager::lookupLibMaybeAddExt(): "
  // TODO: implement test this case

  // Cover
  // platform::DLClose(dyLibHandle, &errMsg);
  // if (!errMsg.empty()) {
  //   LLVM_DEBUG(dbgs() << "DynamicLibraryManager::unloadLibrary(): "
  // TODO: implement test this case
}
