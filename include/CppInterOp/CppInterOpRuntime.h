//===--- CppInterOpRuntime.h - Runtime codes for language interoperability ---*- C++ -*-===//
//
// Part of the compiler-research project, under the Apache License v2.0 with
// LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef CPPINTEROP_CPPINTEROPRUNTIME_H
#define CPPINTEROP_CPPINTEROPRUNTIME_H

namespace __internal_CppInterOp {
template <typename Signature>
struct function;

template <typename Res, typename... ArgTypes>
struct function<Res(ArgTypes...)> {
  using result_type = Res;
};
}  // namespace __internal_CppInterOp

#endif // CPPINTEROP_CPPINTEROPRUNTIME_H