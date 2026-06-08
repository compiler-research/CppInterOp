//===- ResultTest.cpp - Tests for Cpp::Result<T> ---------------*- C++ -*-===//
//
// Part of the compiler-research project, under the Apache License v2.0 with
// LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Value-semantic tests for Cpp::Result<T>: construction, accessors,
// copy/move (including the Unchecked-flag transfer that catches the
// moved-from-aborts-in-dtor trap), .ignore() opt-out, and death paths
// for dropped errors and value()-on-error.
//
//===----------------------------------------------------------------------===//

#include "CppInterOp/CppInterOp.h"
#include "CppInterOp/CppInterOpTypes.h"
#include "CppInterOp/Error.h"

#include "llvm/Support/Error.h"

#include "gtest/gtest.h"

#include <string>
#include <utility>

// Size invariants. Each row caps Cpp::Result<T> at llvm::Expected<T>'s
// size; a refactor that inflates the layout fails the build.

static_assert(sizeof(Cpp::ErrorRef) == sizeof(void*),
              "ErrorRef must be a single pointer");

// Result<void> in release is just the ErrorRef pointer. Debug adds the
// Unchecked bit; allow up to llvm::Error's debug-fudge ceiling.
static_assert(sizeof(Cpp::Result<void>) <= sizeof(llvm::Error) * 2,
              "Result<void> should not exceed llvm::Error layout");

static_assert(sizeof(Cpp::Result<Cpp::DeclRef>) <=
                  sizeof(llvm::Expected<void*>),
              "Result<DeclRef> must not exceed llvm::Expected<void*>");

static_assert(sizeof(Cpp::Result<int>) <= sizeof(llvm::Expected<int>),
              "Result<int> must not exceed llvm::Expected<int>");

namespace {
// Address of a file-local sentinel byte serves as a non-null but
// never-dereferenced ErrorRef state for the tests. Using a real
// address keeps clang-tidy's int-to-ptr / reinterpret-cast checks
// happy without weakening the test.
const char kErrorSentinelByte = 0;
const void* const kFakeState = &kErrorSentinelByte;
Cpp::ErrorRef MakeErr() { return Cpp::ErrorRef{kFakeState}; }
} // namespace

// === Construction + basic accessors =====================================

TEST(ResultTest, Result_OkFromValue) {
  Cpp::Result<int> R{42};
  EXPECT_TRUE(R.ok());
  EXPECT_TRUE(static_cast<bool>(R));
  EXPECT_EQ(R.status(), Cpp::Status::Ok);
  EXPECT_TRUE(R.error().isOk());
  EXPECT_EQ(R.value(), 42);
  EXPECT_EQ(R.value_or(-1), 42);
}

TEST(ResultTest, Result_DefaultIsOk) {
  // Default-constructed Result<T> holds a default-constructed T; ok().
  Cpp::Result<int> R;
  EXPECT_TRUE(R.ok());
  EXPECT_EQ(R.value(), 0);
}

TEST(ResultTest, Result_ErrorFromErrorRef) {
  Cpp::Result<int> R{MakeErr()};
  EXPECT_FALSE(R.ok());
  EXPECT_FALSE(static_cast<bool>(R));
  EXPECT_EQ(R.status(), Cpp::Status::Failed);
  EXPECT_FALSE(R.error().isOk());
  EXPECT_EQ(R.error().state, kFakeState);
  EXPECT_EQ(R.value_or(-1), -1);
}

// === Result<void> ========================================================

TEST(ResultTest, ResultVoid_DefaultIsOk) {
  Cpp::Result<void> R;
  EXPECT_TRUE(R.ok());
  EXPECT_EQ(R.status(), Cpp::Status::Ok);
  EXPECT_TRUE(R.error().isOk());
}

TEST(ResultTest, ResultVoid_FromErrorRef) {
  Cpp::Result<void> R{MakeErr()};
  EXPECT_FALSE(R.ok());
  EXPECT_EQ(R.status(), Cpp::Status::Failed);
  EXPECT_EQ(R.error().state, kFakeState);
}

// === Copy semantics ======================================================

TEST(ResultTest, Copy_Ok) {
  Cpp::Result<int> A{7};
  // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
  Cpp::Result<int> B{A}; // exercises the copy ctor; do not collapse to a ref
  EXPECT_TRUE(B.ok());
  EXPECT_EQ(B.value(), 7);
  // The source remains usable after copy; we explicitly check it here
  // so its dtor doesn't trip the must-handle guard.
  EXPECT_EQ(A.value(), 7);
}

TEST(ResultTest, Copy_Error) {
  Cpp::Result<int> A{MakeErr()};
  // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
  Cpp::Result<int> B{A}; // exercises the copy ctor; do not collapse to a ref
  EXPECT_FALSE(B.ok());
  EXPECT_FALSE(A.ok()); // mark A checked so its dtor doesn't abort
}

// === Move semantics ======================================================

TEST(ResultTest, Move_Ok) {
  Cpp::Result<int> A{11};
  Cpp::Result<int> B{std::move(A)};
  EXPECT_TRUE(B.ok());
  EXPECT_EQ(B.value(), 11);
  // A was Ok-valued; the move-from state is undefined for the value
  // but still ok-tagged. No abort on A's dtor.
}

// Pins the bug where Result<void>'s default move ctor failed to
// transfer the Unchecked obligation, causing the source's dtor to
// abort even though the value semantically went to the destination
// (most visible when threaded through a tracing record() wrapper).
TEST(ResultTest, MoveCtor_Void_TransfersUncheckedToSource) {
  Cpp::Result<void> A{MakeErr()};
  Cpp::Result<void> B{std::move(A)};
  // Check only B; A must not abort at scope exit despite never being
  // inspected directly.
  EXPECT_FALSE(B.ok());
}

TEST(ResultTest, MoveCtor_NonVoid_TransfersUncheckedToSource) {
  Cpp::Result<int> A{MakeErr()};
  Cpp::Result<int> B{std::move(A)};
  EXPECT_FALSE(B.ok());
}

// Non-trivial T exercises the placement-new + explicit destructor
// paths in ctor/dtor. Result<int> alone wouldn't catch a missing
// T::~T() call -- int has no destructor to miss. ASan / leak checker
// flags any miss here.
TEST(ResultTest, Result_NonTrivialT_PlacementNewAndDtor) {
  {
    Cpp::Result<std::string> R{std::string("hello")};
    EXPECT_TRUE(R.ok());
    EXPECT_EQ(R.value(), "hello");
  }

  Cpp::Result<std::string> A{std::string("world")};
  Cpp::Result<std::string> B{std::move(A)};
  EXPECT_TRUE(B.ok());
  EXPECT_EQ(B.value(), "world");
}

// .ignore() silences the dropped-error abort. Without it, the inner
// blocks would die at scope exit.
TEST(ResultTest, Ignore_SuppressesDroppedErrorAbort) {
  {
    Cpp::Result<int> R{MakeErr()};
    R.ignore();
  }
  {
    Cpp::Result<void> R{MakeErr()};
    R.ignore();
  }
  SUCCEED();
}

#ifndef EMSCRIPTEN

#ifndef NDEBUG
TEST(ResultDeathTest, DroppedError_Aborts) {
  ::testing::FLAGS_gtest_death_test_style = "threadsafe";
  EXPECT_DEATH({ (void)Cpp::Result<int>(MakeErr()); },
               ".*destroyed without check.*");
}
#endif // !NDEBUG

// value() on an error always aborts, debug or release.
TEST(ResultDeathTest, ValueOnError_Aborts) {
  ::testing::FLAGS_gtest_death_test_style = "threadsafe";
  EXPECT_DEATH({ (void)Cpp::Result<int>(MakeErr()).value(); },
               ".*value\\(\\) called on an error-bearing Result.*");
}

#endif // !EMSCRIPTEN
