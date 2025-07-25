//------------------------------------------------------------------------------
// CLING - the C++ LLVM-based InterpreterG :)
// author:  Vassil Vassilev <vasil.georgiev.vasilev@cern.ch>
//
// This file is dual-licensed: you can choose to license it under the University
// of Illinois Open Source License or the GNU Lesser General Public License. See
// LICENSE.TXT for details.
//------------------------------------------------------------------------------

#include "DynamicLibraryManager.h"
#include "Compatibility.h"
#include "Paths.h"

#include "llvm/ADT/StringSet.h"
#include "llvm/BinaryFormat/Magic.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/DynamicLibrary.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"

#if defined(_WIN32)
#include "llvm/BinaryFormat/COFF.h"
#include "llvm/Support/Endian.h"
#endif

#include <fstream>
#include <sys/stat.h>
#include <system_error>

namespace Cpp {

using namespace Cpp::utils::platform;
using namespace Cpp::utils;
using namespace llvm;

DynamicLibraryManager::DynamicLibraryManager() {
  const SmallVector<const char*, 10> kSysLibraryEnv = {
      "LD_LIBRARY_PATH",
#if __APPLE__
      "DYLD_LIBRARY_PATH",
      "DYLD_FALLBACK_LIBRARY_PATH",
  /*
  "DYLD_VERSIONED_LIBRARY_PATH",
  "DYLD_FRAMEWORK_PATH",
  "DYLD_FALLBACK_FRAMEWORK_PATH",
  "DYLD_VERSIONED_FRAMEWORK_PATH",
  */
#elif defined(_WIN32)
      "PATH",
#endif
  };

  // Behaviour is to not add paths that don't exist...In an interpreted env
  // does this make sense? Path could pop into existence at any time.
  for (const char* Var : kSysLibraryEnv) {
    if (const char* Env = GetEnv(Var)) {
      SmallVector<StringRef, 10> CurPaths;
      SplitPaths(Env, CurPaths, utils::kPruneNonExistent,
                 Cpp::utils::platform::kEnvDelim);
      for (const auto& Path : CurPaths)
        addSearchPath(Path);
    }
  }

  // $CWD is the last user path searched.
  addSearchPath(".");

  SmallVector<std::string, 64> SysPaths;
  Cpp::utils::platform::GetSystemLibraryPaths(SysPaths);

  for (const std::string& P : SysPaths)
    addSearchPath(P, /*IsUser*/ false);
}
///\returns substitution of pattern in the front of original with replacement
/// Example: substFront("@rpath/abc", "@rpath/", "/tmp") -> "/tmp/abc"
static std::string substFront(StringRef original, StringRef pattern,
                              StringRef replacement) {
  if (!original.starts_with_insensitive(pattern))
    return original.str();
  SmallString<512> result(replacement);
  result.append(original.drop_front(pattern.size()));
  return result.str().str();
}

///\returns substitution of all known linker variables in \c original
static std::string substAll(StringRef original, StringRef libLoader) {

  // Handle substitutions (MacOS):
  // @rpath - This function does not substitute @rpath, because
  //          this variable is already handled by lookupLibrary where
  //          @rpath is replaced with all paths from RPATH one by one.
  // @executable_path - Main program path.
  // @loader_path - Loader library (or main program) path.
  //
  // Handle substitutions (Linux):
  // https://man7.org/linux/man-pages/man8/ld.so.8.html
  // $origin - Loader library (or main program) path.
  // $lib - lib lib64
  // $platform - x86_64 AT_PLATFORM

  std::string result;
#ifdef __APPLE__
  SmallString<512> mainExecutablePath(
      llvm::sys::fs::getMainExecutable(nullptr, nullptr));
  llvm::sys::path::remove_filename(mainExecutablePath);
  SmallString<512> loaderPath;
  if (libLoader.empty()) {
    loaderPath = mainExecutablePath;
  } else {
    loaderPath = libLoader.str();
    llvm::sys::path::remove_filename(loaderPath);
  }

  result = substFront(original, "@executable_path", mainExecutablePath);
  result = substFront(result, "@loader_path", loaderPath);
  return result;
#else
  SmallString<512> loaderPath;
  if (libLoader.empty()) {
    loaderPath = llvm::sys::fs::getMainExecutable(nullptr, nullptr);
  } else {
    loaderPath = libLoader.str();
  }
  llvm::sys::path::remove_filename(loaderPath);

  result = substFront(original, "$origin", loaderPath);
  // result = substFront(result, "$lib", true?"lib":"lib64");
  // result = substFront(result, "$platform", "x86_64");
  return result;
#endif
}

std::string DynamicLibraryManager::lookupLibInPaths(
    StringRef libStem, SmallVector<llvm::StringRef, 2> RPath /*={}*/,
    SmallVector<llvm::StringRef, 2> RunPath /*={}*/,
    StringRef libLoader /*=""*/) const {
#define DEBUG_TYPE "Dyld::lookupLibInPaths"

  LLVM_DEBUG(dbgs() << "Dyld::lookupLibInPaths" << libStem.str()
                    << ", ..., libLoader=" << libLoader << "\n");

  // Lookup priority is: RPATH, LD_LIBRARY_PATH/m_SearchPaths, RUNPATH
  // See: https://en.wikipedia.org/wiki/Rpath
  // See: https://amir.rachum.com/blog/2016/09/17/shared-libraries/

  LLVM_DEBUG(dbgs() << "Dyld::lookupLibInPaths: \n");
  LLVM_DEBUG(dbgs() << ":: RPATH\n");
#ifndef NDEBUG
  for (auto Info : RPath) {
    LLVM_DEBUG(dbgs() << ":::: " << Info.str() << "\n");
  }
#endif
  LLVM_DEBUG(dbgs() << ":: SearchPaths (LD_LIBRARY_PATH, etc...)\n");
  for (auto Info : getSearchPaths()) {
    LLVM_DEBUG(dbgs() << ":::: " << Info.Path
                      << ", user=" << (Info.IsUser ? "true" : "false") << "\n");
  }
  LLVM_DEBUG(dbgs() << ":: RUNPATH\n");
#ifndef NDEBUG
  for (auto Info : RunPath) {
    LLVM_DEBUG(dbgs() << ":::: " << Info.str() << "\n");
  }
#endif
  SmallString<512> ThisPath;
  // RPATH
  for (auto Info : RPath) {
    ThisPath = substAll(Info, libLoader);
    llvm::sys::path::append(ThisPath, libStem);
    // to absolute path?
    LLVM_DEBUG(dbgs() << "## Try: " << ThisPath);
    if (isSharedLibrary(ThisPath.str())) {
      LLVM_DEBUG(dbgs() << " ... Found (in RPATH)!\n");
      return ThisPath.str().str();
    }
  }
  // m_SearchPaths
  for (const SearchPathInfo& Info : m_SearchPaths) {
    ThisPath = Info.Path;
    llvm::sys::path::append(ThisPath, libStem);
    // to absolute path?
    LLVM_DEBUG(dbgs() << "## Try: " << ThisPath);
    if (isSharedLibrary(ThisPath.str())) {
      LLVM_DEBUG(dbgs() << " ... Found (in SearchPaths)!\n");
      return ThisPath.str().str();
    }
  }
  // RUNPATH
  for (auto Info : RunPath) {
    ThisPath = substAll(Info, libLoader);
    llvm::sys::path::append(ThisPath, libStem);
    // to absolute path?
    LLVM_DEBUG(dbgs() << "## Try: " << ThisPath);
    if (isSharedLibrary(ThisPath.str())) {
      LLVM_DEBUG(dbgs() << " ... Found (in RUNPATH)!\n");
      return ThisPath.str().str();
    }
  }

  LLVM_DEBUG(dbgs() << "## NotFound!!!\n");

  return "";

#undef DEBUG_TYPE
}

std::string DynamicLibraryManager::lookupLibMaybeAddExt(
    StringRef libStem, SmallVector<llvm::StringRef, 2> RPath /*={}*/,
    SmallVector<llvm::StringRef, 2> RunPath /*={}*/,
    StringRef libLoader /*=""*/) const {
#define DEBUG_TYPE "Dyld::lookupLibMaybeAddExt:"

  using namespace llvm::sys;

  LLVM_DEBUG(dbgs() << "Dyld::lookupLibMaybeAddExt: " << libStem.str()
                    << ", ..., libLoader=" << libLoader << "\n");

  std::string foundDyLib = lookupLibInPaths(libStem, RPath, RunPath, libLoader);

  if (foundDyLib.empty()) {
    // Add DyLib extension:
    SmallString<512> filenameWithExt(libStem);
#if defined(LLVM_ON_UNIX)
#ifdef __APPLE__
    SmallString<512>::iterator IStemEnd = filenameWithExt.end() - 1;
#endif
    static const char* DyLibExt = ".so";
#elif defined(_WIN32)
    static const char* DyLibExt = ".dll";
#else
#error "Unsupported platform."
#endif
    filenameWithExt += DyLibExt;
    foundDyLib = lookupLibInPaths(filenameWithExt, RPath, RunPath, libLoader);
#ifdef __APPLE__
    if (foundDyLib.empty()) {
      filenameWithExt.erase(IStemEnd + 1, filenameWithExt.end());
      filenameWithExt += ".dylib";
      foundDyLib = lookupLibInPaths(filenameWithExt, RPath, RunPath, libLoader);
    }
#endif
  }

  if (foundDyLib.empty())
    return std::string();

  // get canonical path name and check if already loaded
  const std::string Path = platform::NormalizePath(foundDyLib);
  if (Path.empty()) {
    LLVM_DEBUG(
        dbgs() << "cling::DynamicLibraryManager::lookupLibMaybeAddExt(): "
               << "error getting real (canonical) path of library "
               << foundDyLib << '\n');
    return foundDyLib;
  }
  return Path;

#undef DEBUG_TYPE
}

std::string DynamicLibraryManager::normalizePath(StringRef path) {
#define DEBUG_TYPE "Dyld::normalizePath:"
  // Make the path canonical if the file exists.
  const std::string Path = path.str();
  struct stat buffer;
  if (::stat(Path.c_str(), &buffer) != 0)
    return std::string();

  const std::string NPath = platform::NormalizePath(Path);
  if (NPath.empty())
    LLVM_DEBUG(dbgs() << "Could not normalize: '" << Path << "'");
  return NPath;
#undef DEBUG_TYPE
}

std::string RPathToStr2(SmallVector<StringRef, 2> V) {
  std::string result;
  for (auto item : V)
    result += item.str() + ",";
  if (!result.empty())
    result.pop_back();
  return result;
}

std::string DynamicLibraryManager::lookupLibrary(
    StringRef libStem, SmallVector<llvm::StringRef, 2> RPath /*={}*/,
    SmallVector<llvm::StringRef, 2> RunPath /*={}*/,
    StringRef libLoader /*=""*/, bool variateLibStem /*=true*/) const {
#define DEBUG_TYPE "Dyld::lookupLibrary:"
  LLVM_DEBUG(dbgs() << "Dyld::lookupLibrary: " << libStem.str() << ", "
                    << RPathToStr2(RPath) << ", " << RPathToStr2(RunPath)
                    << ", " << libLoader.str() << "\n");

  // If it is an absolute path, don't try iterate over the paths.
  if (llvm::sys::path::is_absolute(libStem)) {
    if (isSharedLibrary(libStem))
      return normalizePath(libStem);

    LLVM_DEBUG(dbgs() << "Dyld::lookupLibrary: '" << libStem.str() << "'"
                      << "is not a shared library\n");
    return std::string();
  }

  // Subst all known linker variables ($origin, @rpath, etc.)
#ifdef __APPLE__
  // On MacOS @rpath is preplaced by all paths in RPATH one by one.
  if (libStem.starts_with_insensitive("@rpath")) {
    for (auto& P : RPath) {
      std::string result = substFront(libStem, "@rpath", P);
      if (isSharedLibrary(result))
        return normalizePath(result);
    }
  } else {
#endif
    std::string result = substAll(libStem, libLoader);
    if (isSharedLibrary(result))
      return normalizePath(result);
#ifdef __APPLE__
  }
#endif

  // Expand libStem with paths, extensions, etc.
  std::string foundName;
  if (variateLibStem) {
    foundName = lookupLibMaybeAddExt(libStem, RPath, RunPath, libLoader);
    if (foundName.empty()) {
      StringRef libStemName = llvm::sys::path::filename(libStem);
      if (!libStemName.starts_with("lib")) {
        // try with "lib" prefix:
        foundName = lookupLibMaybeAddExt(
            libStem.str().insert(libStem.size() - libStemName.size(), "lib"),
            RPath, RunPath, libLoader);
      }
    }
  } else {
    foundName = lookupLibInPaths(libStem, RPath, RunPath, libLoader);
  }

  if (!foundName.empty())
    return platform::NormalizePath(foundName);

  return std::string();
#undef DEBUG_TYPE
}

DynamicLibraryManager::LoadLibResult
DynamicLibraryManager::loadLibrary(StringRef libStem, bool permanent,
                                   bool resolved) {
#define DEBUG_TYPE "Dyld::loadLibrary:"
  LLVM_DEBUG(dbgs() << "Dyld::loadLibrary: " << libStem.str() << ", "
                    << (permanent ? "permanent" : "not-permanent") << ", "
                    << (resolved ? "resolved" : "not-resolved") << "\n");

  std::string canonicalLoadedLib;
  if (resolved) {
    canonicalLoadedLib = libStem.str();
  } else {
    canonicalLoadedLib = lookupLibrary(libStem);
    if (canonicalLoadedLib.empty())
      return kLoadLibNotFound;
  }

  if (m_LoadedLibraries.find(canonicalLoadedLib) != m_LoadedLibraries.end())
    return kLoadLibAlreadyLoaded;

  // TODO: !permanent case

  std::string errMsg;
  DyLibHandle dyLibHandle = platform::DLOpen(canonicalLoadedLib, &errMsg);
  if (!dyLibHandle) {
    // We emit callback to LibraryLoadingFailed when we get error with error
    // message.
    // TODO: Implement callbacks

    LLVM_DEBUG(dbgs() << "DynamicLibraryManager::loadLibrary(): " << errMsg);

    return kLoadLibLoadError;
  }

  std::pair<DyLibs::iterator, bool> insRes = m_DyLibs.insert(
      std::pair<DyLibHandle, std::string>(dyLibHandle, canonicalLoadedLib));
  if (!insRes.second)
    return kLoadLibAlreadyLoaded;
  m_LoadedLibraries.insert(canonicalLoadedLib);
  return kLoadLibSuccess;
#undef DEBUG_TYPE
}

void DynamicLibraryManager::unloadLibrary(StringRef libStem) {
#define DEBUG_TYPE "Dyld::unloadLibrary:"
  std::string canonicalLoadedLib = lookupLibrary(libStem);
  if (!isLibraryLoaded(canonicalLoadedLib))
    return;

  DyLibHandle dyLibHandle = nullptr;
  for (DyLibs::const_iterator I = m_DyLibs.begin(), E = m_DyLibs.end(); I != E;
       ++I) {
    if (I->second == canonicalLoadedLib) {
      dyLibHandle = I->first;
      break;
    }
  }

  // TODO: !permanent case

  std::string errMsg;
  platform::DLClose(dyLibHandle, &errMsg);
  if (!errMsg.empty()) {
    LLVM_DEBUG(dbgs() << "cling::DynamicLibraryManager::unloadLibrary(): "
                      << errMsg << '\n');
  }

  // TODO: Implement callbacks

  m_DyLibs.erase(dyLibHandle);
  m_LoadedLibraries.erase(canonicalLoadedLib);
#undef DEBUG_TYPE
}

bool DynamicLibraryManager::isLibraryLoaded(StringRef fullPath) const {
  std::string canonPath = normalizePath(fullPath);
  if (m_LoadedLibraries.find(canonPath) != m_LoadedLibraries.end())
    return true;
  return false;
}

void DynamicLibraryManager::dump(llvm::raw_ostream* S /*= nullptr*/) const {
  llvm::raw_ostream& OS = S ? *S : llvm::outs();

  // FIXME: print in a stable order the contents of m_SearchPaths
  for (const auto& Info : getSearchPaths()) {
    if (!Info.IsUser)
      OS << "[system] ";
    OS << Info.Path.c_str() << "\n";
  }
}

#if defined(_WIN32)
static bool IsDLL(llvm::StringRef headers) {
  using namespace llvm::support::endian;

  uint32_t headeroffset = read32le(headers.data() + 0x3c);
  auto peheader = headers.substr(headeroffset, 24);
  if (peheader.size() != 24) {
    return false;
  }
  // Read Characteristics from the coff header
  uint32_t characteristics = read16le(peheader.data() + 22);
  return (characteristics & llvm::COFF::IMAGE_FILE_DLL) != 0;
}
#endif

bool DynamicLibraryManager::isSharedLibrary(StringRef libFullPath,
                                            bool* exists /*=0*/) {
  using namespace llvm;

  auto filetype = sys::fs::get_file_type(libFullPath, /*Follow*/ true);
  if (filetype != sys::fs::file_type::regular_file) {
    if (exists) {
      // get_file_type returns status_error also in case of file_not_found.
      *exists = filetype != sys::fs::file_type::status_error;
    }
    return false;
  }

  // Do not use the identify_magic overload taking a path: It will open the
  // file and then mmap its contents, possibly causing bus errors when another
  // process truncates the file while we are trying to read it. Instead just
  // read the first 1024 bytes, which should be enough for identify_magic to
  // do its work.
  // TODO: Fix the code upstream and consider going back to calling the
  // convenience function after a future LLVM upgrade.
  std::string path = libFullPath.str();
  std::ifstream in(path, std::ios::binary);
  char header[1024] = {0};
  in.read(header, sizeof(header));
  if (in.fail()) {
    if (exists)
      *exists = false;
    return false;
  }

  StringRef headerStr(header, in.gcount());
  file_magic Magic = identify_magic(headerStr);

  bool result =
#ifdef __APPLE__
      (Magic == file_magic::macho_fixed_virtual_memory_shared_lib ||
       Magic == file_magic::macho_dynamically_linked_shared_lib ||
       Magic == file_magic::macho_dynamically_linked_shared_lib_stub ||
       Magic == file_magic::macho_universal_binary)
#elif defined(LLVM_ON_UNIX)
#ifdef __CYGWIN__
      (Magic == file_magic::pecoff_executable)
#else
      (Magic == file_magic::elf_shared_object)
#endif
#elif defined(_WIN32)
      // We should only include dll libraries without including executables,
      // object code and others...
      (Magic == file_magic::pecoff_executable && IsDLL(headerStr))
#else
#error "Unsupported platform."
#endif
      ;

  return result;
}

} // end namespace Cpp
