#ifndef INTEROP_UNITTESTS_LIBINTEROP_UTILS_H
#define INTEROP_UNITTESTS_LIBINTEROP_UTILS_H

#include "clang/Interpreter/Compatibility.h"

#include <memory>
#include <vector>

using namespace clang;
using namespace llvm;

namespace clang {
  class Decl;
}
#define Interp (static_cast<compat::Interpreter*>(InterOp::GetInterpreter()))
namespace TestUtils {
  void GetAllTopLevelDecls(const std::string& code, std::vector<clang::Decl*>& Decls,
                           bool filter_implicitGenerated = false);
  void GetAllSubDecls(clang::Decl *D, std::vector<clang::Decl*>& SubDecls,
                      bool filter_implicitGenerated = false);
} // end namespace TestUtils

#endif // INTEROP_UNITTESTS_LIBINTEROP_UTILS_H
