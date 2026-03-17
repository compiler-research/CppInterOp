#include "gtest/gtest.h"
#include "CppInterOp/CppInterOp.h"

#include <algorithm>
#include <string>
#include <vector>

namespace {

TEST(PathsTest, GetIncludePathsEdgeCases) {
    std::vector<const char*> InterpArgs = {
        "-nostdinc",
        "-nostdinc++",
        "-stdlib=libc++",
        "-fmodule-cache-path=/tmp/fake_cache",
        "-v"
    };
    
    Cpp::CreateInterpreter(InterpArgs);

    std::vector<std::string> IncPaths;
    Cpp::GetIncludePaths(IncPaths, true, true); 

    auto Contains = [&](const std::string& flag) {
        return std::find(IncPaths.begin(), IncPaths.end(), flag) != IncPaths.end();
    };

    EXPECT_TRUE(Contains("-fmodule-cache-path=/tmp/fake_cache") || Contains("-fmodule-cache-path"));
    EXPECT_TRUE(Contains("-nostdinc"));
    EXPECT_TRUE(Contains("-nostdinc++"));
    EXPECT_TRUE(Contains("-stdlib=libc++"));
    EXPECT_TRUE(Contains("-v"));
}

}