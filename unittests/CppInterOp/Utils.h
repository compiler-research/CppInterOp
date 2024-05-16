#ifndef CPPINTEROP_UNITTESTS_LIBCPPINTEROP_UTILS_H
#define CPPINTEROP_UNITTESTS_LIBCPPINTEROP_UTILS_H

#include "../../lib/Interpreter/Compatibility.h"

#include <memory>
#include <vector>

// Include this to access the RUNNING_ON_VALGRIND directive to conditionally skip tests reporting memory issues

#ifndef __APPLE__
  #include <valgrind/valgrind.h>
#endif //__APPLE__
#ifdef __APPLE__
  #define RUNNING_ON_VALGRIND 0
#endif //__APPLE__

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
