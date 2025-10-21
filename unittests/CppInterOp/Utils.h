#ifndef CPPINTEROP_UNITTESTS_LIBCPPINTEROP_UTILS_H
#define CPPINTEROP_UNITTESTS_LIBCPPINTEROP_UTILS_H

#include "../../lib/CppInterOp/Compatibility.h"

#include "clang-c/CXCppInterOp.h"
#include "clang-c/CXString.h"
#include "CppInterOp/CppInterOp.h"

#include "llvm/Support/Valgrind.h"

#include <memory>
#include <string>
#include <vector>
#include "gtest/gtest.h"

using namespace clang;
using namespace llvm;

namespace clang {
class Decl;
}
#define Interp (static_cast<compat::Interpreter*>(Cpp::GetInterpreter()))
namespace TestUtils {

struct TestConfig {
    bool use_oop_jit;
    std::string name;

    // Constructor ensures proper initialization
    TestConfig(bool oop_jit, const std::string& n) 
        : use_oop_jit(oop_jit), name(n) {}

    // Default constructor
    TestConfig() : use_oop_jit(false), name("InProcessJIT") {}
};

extern TestConfig current_config;

// Helper to get interpreter args with current config
std::vector<const char*>
GetInterpreterArgs(const std::vector<const char*>& base_args = {});

void GetAllTopLevelDecls(const std::string& code,
                         std::vector<clang::Decl*>& Decls,
                         bool filter_implicitGenerated = false,
                         const std::vector<const char*>& interpreter_args = {});
void GetAllSubDecls(clang::Decl* D, std::vector<clang::Decl*>& SubDecls,
                    bool filter_implicitGenerated = false);
} // end namespace TestUtils

const char* get_c_string(CXString string);

void dispose_string(CXString string);

CXScope make_scope(const clang::Decl* D, const CXInterpreter I);

bool IsTargetX86();

class CppInterOpTest : public ::testing::TestWithParam<TestUtils::TestConfig> {
protected:
  void SetUp() override { TestUtils::current_config = GetParam(); }

public:
  static TInterp_t CreateInterpreter(const std::vector<const char*>& Args = {},
                              const std::vector<const char*>& GpuArgs = {}) {
    auto mergedArgs = TestUtils::GetInterpreterArgs(Args);
    return Cpp::CreateInterpreter(mergedArgs, GpuArgs);
  }
};

#endif // CPPINTEROP_UNITTESTS_LIBCPPINTEROP_UTILS_H
