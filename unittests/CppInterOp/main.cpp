#include "Utils.h"
#include <gtest/gtest.h>

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  int result = RUN_ALL_TESTS();
  if (result != 0) {
    return result;
  }
#ifdef LLVM_BUILT_WITH_OOP_JIT
  TestUtils::use_oop_jit = true;
  result = RUN_ALL_TESTS();
#endif
  return result;
}