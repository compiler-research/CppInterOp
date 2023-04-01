//--------------------------------------------------------------------*- C++ -*-
// CLING - the C++ LLVM-based InterpreterG :)
// author:
//
// This file is dual-licensed: you can choose to license it under the University
// of Illinois Open Source License or the GNU Lesser General Public License. See
// LICENSE.TXT for details.
//------------------------------------------------------------------------------

#include "clang/Basic/FileManager.h"
#include "clang/Interpreter/Paths.h"
#include "clang/Lex/HeaderSearchOptions.h"

#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"

#include <dlfcn.h>

namespace InterOp {
namespace utils {

namespace platform {
#if defined(LLVM_ON_UNIX)
  const char* const kEnvDelim = ":";
#elif defined(_WIN32)
  const char* const kEnvDelim = ";";
#else
  #error "Unknown platform (environmental delimiter)"
#endif

bool Popen(const std::string& Cmd, llvm::SmallVectorImpl<char>& Buf, bool RdE) {
  if (FILE *PF = ::popen(RdE ? (Cmd + " 2>&1").c_str() : Cmd.c_str(), "r")) {
    Buf.resize(0);
    const size_t Chunk = Buf.capacity_in_bytes();
    while (true) {
      const size_t Len = Buf.size();
      Buf.resize(Len + Chunk);
      const size_t R = ::fread(&Buf[Len], sizeof(char), Chunk, PF);
      if (R < Chunk) {
        Buf.resize(Len + R);
        break;
      }
    }
    ::pclose(PF);
    return !Buf.empty();
  }
  return false;
}

bool GetSystemLibraryPaths(llvm::SmallVectorImpl<std::string>& Paths) {
#if defined(__APPLE__) || defined(__CYGWIN__)
  Paths.push_back("/usr/local/lib/");
  Paths.push_back("/usr/X11R6/lib/");
  Paths.push_back("/usr/lib/");
  Paths.push_back("/lib/");

 #ifndef __APPLE__
  Paths.push_back("/lib/x86_64-linux-gnu/");
  Paths.push_back("/usr/local/lib64/");
  Paths.push_back("/usr/lib64/");
  Paths.push_back("/lib64/");
 #endif
#else
  llvm::SmallString<1024> Buf;
  platform::Popen("LD_DEBUG=libs LD_PRELOAD=DOESNOTEXIST ls", Buf, true);
  const llvm::StringRef Result = Buf.str();

  const std::size_t NPos = std::string::npos;
  const std::size_t LD = Result.find("(LD_LIBRARY_PATH)");
  std::size_t From = Result.find("search path=", LD == NPos ? 0 : LD);
  if (From != NPos) {
    std::size_t To = Result.find("(system search path)", From);
    if (To != NPos) {
      From += 12;
      while (To > From && isspace(Result[To - 1]))
        --To;
      std::string SysPath = Result.substr(From, To-From).str();
      SysPath.erase(std::remove_if(SysPath.begin(), SysPath.end(), ::isspace),
                    SysPath.end());

      llvm::SmallVector<llvm::StringRef, 10> CurPaths;
      SplitPaths(SysPath, CurPaths);
      for (const auto& Path : CurPaths)
        Paths.push_back(Path.str());
    }
  }
#endif
  return true;
}

#define PATH_MAXC (PATH_MAX+1)

std::string NormalizePath(const std::string& Path) {
 char Buf[PATH_MAXC];
  if (const char* Result = ::realpath(Path.c_str(), Buf))
    return std::string(Result);

  ::perror("realpath");
  return std::string();
}

static void DLErr(std::string* Err) {
  if (!Err)
    return;
  if (const char* DyLibError = ::dlerror())
    *Err = DyLibError;
}

const void* DLOpen(const std::string& Path, std::string* Err /* = nullptr */) {
  void* Lib = ::dlopen(Path.c_str(), RTLD_LAZY|RTLD_GLOBAL);
  DLErr(Err);
  return Lib;
}

const void* DLSym(const std::string& Name, std::string* Err /* = nullptr*/) {
  if (const void* Self = ::dlopen(nullptr, RTLD_GLOBAL)) {
    // get dlopen error if there is one
    DLErr(Err);
    const void* Sym = ::dlsym(const_cast<void*>(Self), Name.c_str());
    // overwrite error if dlsym caused one
    DLErr(Err);
    // only get dlclose error if dlopen & dlsym haven't emited one
    DLClose(Self, Err && Err->empty() ? Err : nullptr);
    return Sym;
  }
  DLErr(Err);
  return nullptr;
}

void DLClose(const void* Lib, std::string* Err /* = nullptr*/) {
  ::dlclose(const_cast<void*>(Lib));
  DLErr(Err);
}

} // namespace platform

bool ExpandEnvVars(std::string& Str, bool Path) {
  std::size_t DPos = Str.find("$");
  while (DPos != std::string::npos) {
    std::size_t SPos = Str.find("/", DPos + 1);
    std::size_t Length = Str.length();

    if (SPos != std::string::npos) // if we found a "/"
      Length = SPos - DPos;

    std::string EnvVar = Str.substr(DPos + 1, Length -1); //"HOME"
    std::string FullPath;
    if (const char* Tok = ::getenv(EnvVar.c_str()))
      FullPath = Tok;

    Str.replace(DPos, Length, FullPath);
    DPos = Str.find("$", DPos + 1); //search for next env variable
  }
  if (!Path)
    return true;
  return llvm::sys::fs::exists(Str.c_str());
}

using namespace llvm;
using namespace clang;

// Adapted from clang/lib/Frontend/CompilerInvocation.cpp

void CopyIncludePaths(const clang::HeaderSearchOptions& Opts,
                      llvm::SmallVectorImpl<std::string>& incpaths,
                      bool withSystem, bool withFlags) {
  if (withFlags && Opts.Sysroot != "/") {
    incpaths.push_back("-isysroot");
    incpaths.push_back(Opts.Sysroot);
  }

  /// User specified include entries.
  for (unsigned i = 0, e = Opts.UserEntries.size(); i != e; ++i) {
    const HeaderSearchOptions::Entry &E = Opts.UserEntries[i];
    if (E.IsFramework && E.Group != frontend::Angled)
      llvm::report_fatal_error("Invalid option set!");
    switch (E.Group) {
    case frontend::After:
      if (withFlags) incpaths.push_back("-idirafter");
      break;

    case frontend::Quoted:
      if (withFlags) incpaths.push_back("-iquote");
      break;

    case frontend::System:
      if (!withSystem) continue;
      if (withFlags) incpaths.push_back("-isystem");
      break;

    case frontend::IndexHeaderMap:
      if (!withSystem) continue;
      if (withFlags) incpaths.push_back("-index-header-map");
      if (withFlags) incpaths.push_back(E.IsFramework? "-F" : "-I");
      break;

    case frontend::CSystem:
      if (!withSystem) continue;
      if (withFlags) incpaths.push_back("-c-isystem");
      break;

    case frontend::ExternCSystem:
      if (!withSystem) continue;
      if (withFlags) incpaths.push_back("-extern-c-isystem");
      break;

    case frontend::CXXSystem:
      if (!withSystem) continue;
      if (withFlags) incpaths.push_back("-cxx-isystem");
      break;

    case frontend::ObjCSystem:
      if (!withSystem) continue;
      if (withFlags) incpaths.push_back("-objc-isystem");
      break;

    case frontend::ObjCXXSystem:
      if (!withSystem) continue;
      if (withFlags) incpaths.push_back("-objcxx-isystem");
      break;

    case frontend::Angled:
      if (withFlags) incpaths.push_back(E.IsFramework ? "-F" : "-I");
      break;
    }
    incpaths.push_back(E.Path);
  }

  if (withSystem && !Opts.ResourceDir.empty()) {
    if (withFlags) incpaths.push_back("-resource-dir");
    incpaths.push_back(Opts.ResourceDir);
  }
  if (withSystem && withFlags && !Opts.ModuleCachePath.empty()) {
    incpaths.push_back("-fmodule-cache-path");
    incpaths.push_back(Opts.ModuleCachePath);
  }
  if (withSystem && withFlags && !Opts.UseStandardSystemIncludes)
    incpaths.push_back("-nostdinc");
  if (withSystem && withFlags && !Opts.UseStandardCXXIncludes)
    incpaths.push_back("-nostdinc++");
  if (withSystem && withFlags && Opts.UseLibcxx)
    incpaths.push_back("-stdlib=libc++");
  if (withSystem && withFlags && Opts.Verbose)
    incpaths.push_back("-v");
}

void DumpIncludePaths(const clang::HeaderSearchOptions& Opts,
                      llvm::raw_ostream& Out,
                      bool WithSystem, bool WithFlags) {
  llvm::SmallVector<std::string, 100> IncPaths;
  CopyIncludePaths(Opts, IncPaths, WithSystem, WithFlags);
  // print'em all
  for (unsigned i = 0; i < IncPaths.size(); ++i) {
    Out << IncPaths[i] <<"\n";
  }
}

void LogNonExistantDirectory(llvm::StringRef Path) {
#define DEBUG_TYPE "LogNonExistantDirectory"
  LLVM_DEBUG(dbgs() << "  ignoring nonexistent directory \"" << Path << "\"\n");
#undef  DEBUG_TYPE
}

static void LogFileStatus(const char* Prefix, const char* FileType,
                          llvm::StringRef Path) {
#define DEBUG_TYPE "LogFileStatus"
  LLVM_DEBUG(dbgs() << Prefix << " " << FileType << " '" << Path << "'\n";);
#undef  DEBUG_TYPE
}

bool LookForFile(const std::vector<const char*>& Args, std::string& Path,
                 const clang::FileManager* FM, const char* FileType) {
  if (llvm::sys::fs::is_regular_file(Path)) {
    if (FileType)
      LogFileStatus("Using", FileType, Path);
    return true;
  }
  if (FileType)
    LogFileStatus("Ignoring", FileType, Path);

  SmallString<1024> FilePath;
  if (FM) {
    FilePath.assign(Path);
    if (FM->FixupRelativePath(FilePath) &&
        llvm::sys::fs::is_regular_file(FilePath)) {
      if (FileType)
        LogFileStatus("Using", FileType, FilePath.str());
      Path = FilePath.str().str();
      return true;
    }
    // Don't write same same log entry twice when FilePath == Path
    if (FileType && !FilePath.str().equals(Path))
      LogFileStatus("Ignoring", FileType, FilePath);
  }
  else if (llvm::sys::path::is_absolute(Path))
    return false;

  for (std::vector<const char*>::const_iterator It = Args.begin(),
       End = Args.end(); It < End; ++It) {
    const char* Arg = *It;
    // TODO: Suppport '-iquote' and MSVC equivalent
    if (!::strncmp("-I", Arg, 2) || !::strncmp("/I", Arg, 2)) {
      if (!Arg[2]) {
        if (++It >= End)
          break;
        FilePath.assign(*It);
      }
      else
        FilePath.assign(Arg + 2);

      llvm::sys::path::append(FilePath, Path.c_str());
      if (llvm::sys::fs::is_regular_file(FilePath)) {
        if (FileType)
          LogFileStatus("Using", FileType, FilePath.str());
        Path = FilePath.str().str();
        return true;
      }
      if (FileType)
        LogFileStatus("Ignoring", FileType, FilePath);
    }
  }
  return false;
}

bool SplitPaths(llvm::StringRef PathStr,
                llvm::SmallVectorImpl<llvm::StringRef>& Paths,
                SplitMode Mode, llvm::StringRef Delim, bool Verbose) {
#define DEBUG_TYPE "SplitPths"

  assert(Delim.size() && "Splitting without a delimiter");

#if defined(_WIN32)
  // Support using a ':' delimiter on Windows.
  const bool WindowsColon = Delim.equals(":");
#endif

  bool AllExisted = true;
  for (std::pair<llvm::StringRef, llvm::StringRef> Split = PathStr.split(Delim);
       !Split.second.empty(); Split = PathStr.split(Delim)) {

    if (!Split.first.empty()) {
      bool Exists = llvm::sys::fs::is_directory(Split.first);

#if defined(_WIN32)
    // Because drive letters will have a colon we have to make sure the split
    // occurs at a colon not followed by a path separator.
    if (!Exists && WindowsColon && Split.first.size()==1) {
      // Both clang and cl.exe support '\' and '/' path separators.
      if (Split.second.front() == '\\' || Split.second.front() == '/') {
          const std::pair<llvm::StringRef, llvm::StringRef> Tmp =
              Split.second.split(Delim);
          // Split.first = 'C', but we want 'C:', so Tmp.first.size()+2
          Split.first =
              llvm::StringRef(Split.first.data(), Tmp.first.size() + 2);
          Split.second = Tmp.second;
          Exists = llvm::sys::fs::is_directory(Split.first);
      }
    }
#endif

      AllExisted = AllExisted && Exists;

      if (!Exists) {
        if (Mode == kFailNonExistant) {
          if (Verbose) {
            // Exiting early, but still log all non-existant paths that we have
            LogNonExistantDirectory(Split.first);
            while (!Split.second.empty()) {
              Split = PathStr.split(Delim);
              if (llvm::sys::fs::is_directory(Split.first)) {
                LLVM_DEBUG(dbgs() << "  ignoring directory that exists \""
                                  << Split.first << "\"\n");
              } else
                LogNonExistantDirectory(Split.first);
              Split = Split.second.split(Delim);
            }
            if (!llvm::sys::fs::is_directory(Split.first))
              LogNonExistantDirectory(Split.first);
          }
          return false;
        } else if (Mode == kAllowNonExistant)
          Paths.push_back(Split.first);
        else if (Verbose)
          LogNonExistantDirectory(Split.first);
      } else
        Paths.push_back(Split.first);
    }

    PathStr = Split.second;
  }

  // Trim trailing sep in case of A:B:C:D:
  if (!PathStr.empty() && PathStr.endswith(Delim))
    PathStr = PathStr.substr(0, PathStr.size()-Delim.size());

  if (!PathStr.empty()) {
    if (!llvm::sys::fs::is_directory(PathStr)) {
      AllExisted = false;
      if (Mode == kAllowNonExistant)
        Paths.push_back(PathStr);
      else if (Verbose)
        LogNonExistantDirectory(PathStr);
    } else
      Paths.push_back(PathStr);
  }

  return AllExisted;

#undef  DEBUG_TYPE
}

void AddIncludePaths(llvm::StringRef PathStr,
                         clang::HeaderSearchOptions& HOpts,
                         const char* Delim /* = InterOp::utils::platform::kEnvDelim */) {
#define DEBUG_TYPE "AddIncludePaths"

  llvm::SmallVector<llvm::StringRef, 10> Paths;
  if (Delim && *Delim)
    SplitPaths(PathStr, Paths, kAllowNonExistant, Delim, HOpts.Verbose);
  else
    Paths.push_back(PathStr);

  // Avoid duplicates
  llvm::SmallVector<llvm::StringRef, 10> PathsChecked;
  for (llvm::StringRef Path : Paths) {
    bool Exists = false;
    for (const clang::HeaderSearchOptions::Entry& E : HOpts.UserEntries) {
      if ((Exists = E.Path == Path))
        break;
    }
    if (!Exists)
      PathsChecked.push_back(Path);
  }

  const bool IsFramework = false;
  const bool IsSysRootRelative = true;
  for (llvm::StringRef Path : PathsChecked)
    HOpts.AddPath(Path, clang::frontend::Angled, IsFramework, IsSysRootRelative);

  if (HOpts.Verbose) {
    LLVM_DEBUG(dbgs() << "Added include paths:\n");
    for (llvm::StringRef Path : PathsChecked)
      LLVM_DEBUG(dbgs() << "  " << Path << "\n");
  }

#undef  DEBUG_TYPE
}

} // namespace utils
} // namespace InterOp
