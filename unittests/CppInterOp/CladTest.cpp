#include "Utils.h"

#include "CppInterOp/CppInterOp.h"

#include "gtest/gtest.h"

#include <string>

using namespace TestUtils;

TYPED_TEST(CPPINTEROP_TEST_MODE, CladTest_Sanity) {
  Cpp::CreateInterpreter({"-std=c++17"}, {});

  std::string code = R"(
    #include "clad/Differentiator/Differentiator.h"
    static double pow2(double x) { return x * x; }
  )";
  EXPECT_EQ(Cpp::Declare(code.c_str()), 0);

  EXPECT_EQ(Cpp::Evaluate("clad::differentiate(pow2, 0).execute(3)")
                .unbox<double>(),
            6.);
}
