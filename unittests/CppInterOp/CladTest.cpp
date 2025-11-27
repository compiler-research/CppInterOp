#include "Utils.h"

#include "CppInterOp/CppInterOp.h"
#include "gtest/gtest.h"

using namespace TestUtils;
using namespace llvm;
using namespace clang;

// This should work
// TYPED_TEST(CppInterOpTest, CladTestSanity) {
//     std::string code = R"(
//         #include "CppInterOp/CppInterOp.h"
//     )";

//     auto I = Cpp::CreateInterpreter();
//     Cpp::Declare(code.c_str());
// }

TYPED_TEST(CppInterOpTest, CladTestSanity) {
    std::vector<Decl *> Decls;
    std::string code = R"(
    #include "clad/Differentiator/Differentiator.h"
    static double pow2(double x) { return x * x; }
    )";

    auto I = Cpp::CreateInterpreter();
    Cpp::Declare(code.c_str());

    std::string code_diff = "clad::differentiate(pow2, 0).execute(3)";
    ASSERT_EQ(Cpp::Evaluate(code_diff.c_str()), 6);
}
