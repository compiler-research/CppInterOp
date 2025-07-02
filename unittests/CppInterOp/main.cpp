#include "Utils.h"
#include <gtest/gtest.h>

void parseCustomArguments(int argc, char** argv) {
  for (int i = 1; i < argc; ++i) {
    std::string arg(argv[i]);
    if (arg == "--use-oop-jit") {
      TestUtils::g_use_oop_jit = true;
      std::cout << "OOP-JIT mode enabled\n";
    }
  }
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  std::cout << "IN GTESTS" << std::endl;
  parseCustomArguments(argc, argv);
  return RUN_ALL_TESTS();
}