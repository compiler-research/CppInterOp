#ifndef CPPINTEROP_UNITTESTS_LIBCPPINTEROP_UTILS_H
#define CPPINTEROP_UNITTESTS_LIBCPPINTEROP_UTILS_H

#include "clang/Interpreter/Compatibility.h"

#include <memory>
#include <vector>

using namespace clang;
using namespace llvm;

namespace clang {
  class Decl;
}
#define Interp (static_cast<compat::Interpreter*>(Cpp::GetInterpreter()))
namespace TestUtils {
  void GetAllTopLevelDecls(const std::string& code, std::vector<clang::Decl*>& Decls,
                           bool filter_implicitGenerated = false);
  void GetAllSubDecls(clang::Decl *D, std::vector<clang::Decl*>& SubDecls,
                      bool filter_implicitGenerated = false);
} // end namespace TestUtils

#endif // CPPINTEROP_UNITTESTS_LIBCPPINTEROP_UTILS_H
