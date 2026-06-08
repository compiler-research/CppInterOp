//===--- ErrorInternal.cpp - Cpp::Result<T> abort helper --------*- C++ -*-===//
//
// Part of the compiler-research project, under the Apache License v2.0 with
// LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Out-of-line abort for Cpp::Result<T>::value() called on an
// error-bearing Result. Keeps the inline Result<T> body small.
//
// Later PRs extend this file with the boundary translator
// (llvm::Error / Expected<T> -> Cpp::Result<T>) and per-call
// diagnostic capture.
//
//===----------------------------------------------------------------------===//

#include "CppInterOp/Error.h"

#include "CppInterOp/CppInterOpTypes.h"

#include <cstdio>
#include <cstdlib>

namespace Cpp {

[[noreturn]] CPPINTEROP_API void
ResultAbort_ValueOnError(const ErrorRef& /*Err*/) {
  std::fputs("Cpp::Result<T>::value() called on an error-bearing "
             "Result. Use value_or(fallback) for lenient semantics, "
             "or branch on .ok() / .error() before calling .value().\n",
             stderr);
  std::abort();
}

#ifndef NDEBUG
[[noreturn]] CPPINTEROP_API void
ResultAbort_UncheckedOnDtor(const ErrorRef& /*Err*/) {
  std::fputs("Cpp::Result destroyed without check (likely a dropped "
             "error). Call .ok() / .error() / .value() to inspect, "
             "or .ignore() to acknowledge.\n",
             stderr);
  std::abort();
}
#endif

} // namespace Cpp
