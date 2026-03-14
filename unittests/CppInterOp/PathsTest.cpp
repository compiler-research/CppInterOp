#include "gtest/gtest.h"
#include "clang/Lex/HeaderSearchOptions.h"
#include "llvm/ADT/SmallVector.h"
#include <string>
#include <algorithm>
#include <string>
#include <string>


#include "../../lib/CppInterOp/Paths.h" 

namespace {

TEST(PathsTest, CopyIncludePathsEdgeCases) {
    clang::HeaderSearchOptions Opts;
    
    // Set up edge-case flags to trigger uncovered branches
    Opts.ModuleCachePath = "/tmp/fake_cache"; // Triggers -fmodule-cache-path
    Opts.UseStandardSystemIncludes = false;   // Triggers -nostdinc
    Opts.UseStandardCXXIncludes = false;      // Triggers -nostdinc++
    Opts.UseLibcxx = true;                    // Triggers -stdlib=libc++
    Opts.Verbose = true;                      // Triggers -v

    llvm::SmallVector<std::string, 16> IncPaths;

    CppInternal::utils::CopyIncludePaths(Opts, IncPaths, true, true);

    auto Contains = [&](const std::string& flag) {
        return std::find(IncPaths.begin(), IncPaths.end(), flag) != IncPaths.end();
    };

    // Verify the results
    EXPECT_TRUE(Contains("-fmodule-cache-path"));
    EXPECT_TRUE(Contains("/tmp/fake_cache"));
    EXPECT_TRUE(Contains("-nostdinc"));
    EXPECT_TRUE(Contains("-nostdinc++"));
    EXPECT_TRUE(Contains("-stdlib=libc++"));
    EXPECT_TRUE(Contains("-v"));
}

} // end anonymous namespace