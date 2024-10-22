#ifndef CPPINTEROP_UNITTESTS_LIBCPPINTEROP_UTILS_H
#define CPPINTEROP_UNITTESTS_LIBCPPINTEROP_UTILS_H

#include "../../lib/Interpreter/Compatibility.h"

#include "llvm/Support/Valgrind.h"
#include <memory>
#include <vector>
#include "clang-c/CXString.h"

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

// libclang's string manipulation APIs
const char* clang_getCString(CXString string);
void clang_disposeString(CXString string);

#endif // CPPINTEROP_UNITTESTS_LIBCPPINTEROP_UTILS_H
