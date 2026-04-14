//===--- Tracing.cpp - Tracing implementation -------------------*- C++ -*-===//
//
// Part of the compiler-research project, under the Apache License v2.0 with
// LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "Tracing.h"

#include "CppInterOp/CppInterOp.h"

#include "llvm/Support/FileSystem.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"

#include <cassert>
#include <system_error>

namespace CppInterOp {
namespace Tracing {

TraceInfo* TraceInfo::TheTraceInfo = nullptr;

void InitTracing() {
  assert(!TraceInfo::TheTraceInfo);
  static llvm::ManagedStatic<TraceInfo> TI;
  TraceInfo::TheTraceInfo = &*TI;
}

/// Helper: emit version info as comment lines.
static void WriteVersionComment(llvm::raw_ostream& OS,
                                const std::string& Version) {
  llvm::StringRef Ver(Version);
  while (!Ver.empty()) {
    auto [Line, Rest] = Ver.split('\n');
    if (!Line.empty())
      OS << "// " << Line << "\n";
    Ver = Rest;
  }
}

std::string TraceInfo::writeToFile(const std::string& Version) {
  llvm::SmallString<128> TmpDir;
  llvm::sys::path::system_temp_directory(/*ErasedOnReboot=*/true, TmpDir);
  llvm::SmallString<128> Path;
  llvm::sys::path::append(Path, TmpDir, "cppinterop-reproducer-%%%%%%.cpp");

  int FD;
  std::error_code EC = llvm::sys::fs::createUniqueFile(Path, FD, Path);
  if (EC)
    return "";

  llvm::raw_fd_ostream OS(FD, /*shouldClose=*/true);

  OS << "// CppInterOp crash reproducer\n";
  std::string Ver = Version.empty() ? CppImpl::GetVersion() : Version;
  WriteVersionComment(OS, Ver);
  OS << "// Generated automatically — re-run to reproduce the crash.\n";
  OS << "#include <CppInterOp/CppInterOp.h>\n\n";
  OS << "void reproducer() {\n";
  for (const auto& Line : m_Log)
    OS << Line << "\n";
  OS << "}\n\n";
  OS << "int main() { reproducer(); return 0; }\n";
  OS.flush();
  return std::string(Path);
}

} // namespace Tracing
} // namespace CppInterOp
