#include "gtest/gtest.h"
#include "CppInterOp/CppInterOp.h"

#include <algorithm>
#include <string>
#include <vector>

namespace {

TEST(PathsTest, GetIncludePathsEdgeCases) {
    std::vector<const char*> InterpArgs = {
        "-fmodule-cache-path=/tmp/fake_cache",
    };
    
    Cpp::CreateInterpreter(InterpArgs);

    std::vector<std::string> IncPaths;
    Cpp::GetIncludePaths(IncPaths, true, true); 

    auto Contains = [&](const std::string& flag) {
        return std::find(IncPaths.begin(), IncPaths.end(), flag) != IncPaths.end();
    };

    bool hasModuleCache = Contains("-fmodule-cache-path=/tmp/fake_cache") || 
                          (Contains("-fmodule-cache-path") && Contains("/tmp/fake_cache"));
                          
    EXPECT_TRUE(hasModuleCache);
}

}