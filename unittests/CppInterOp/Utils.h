#ifndef CPPINTEROP_UNITTESTS_LIBCPPINTEROP_UTILS_H
#define CPPINTEROP_UNITTESTS_LIBCPPINTEROP_UTILS_H

#include "../../lib/CppInterOp/Compatibility.h"

#include "CppInterOp/CppInterOp.h"
#include "clang-c/CXCppInterOp.h"
#include "clang-c/CXString.h"

#include "llvm/Support/Valgrind.h"

#include <string>
#include <thread>
#include <utility>
#include <vector>

using namespace clang;
using namespace llvm;

namespace clang {
  class Decl;
}

#define TU_getSema(I) (static_cast<compat::Interpreter*>(I))->getSema()
#define TU_getASTContext(I)                                                    \
  (static_cast<compat::Interpreter*>(I))->getSema().getASTContext()

namespace TestUtils {
TInterp_t
GetAllTopLevelDecls(const std::string& code, std::vector<clang::Decl*>& Decls,
                    bool filter_implicitGenerated = false,
                    const std::vector<const char*>& interpreter_args = {});
void GetAllSubDecls(clang::Decl* D, std::vector<clang::Decl*>& SubDecls,
                    bool filter_implicitGenerated = false);
} // end namespace TestUtils

const char* get_c_string(CXString string);

void dispose_string(CXString string);

CXScope make_scope(const clang::Decl* D, const CXInterpreter I);

bool IsTargetX86(Cpp::TInterp_t I);

struct ThreadPoolExecutor {
  static void run(std::vector<std::pair<const char*, void (*)()>>& fns,
                  unsigned runners = std::thread::hardware_concurrency());
};

#endif // CPPINTEROP_UNITTESTS_LIBCPPINTEROP_UTILS_H
