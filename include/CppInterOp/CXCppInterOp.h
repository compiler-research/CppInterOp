//===- CXCppInterOp.h - C API for the CppInterOp library --------*- C -*-===//
//
// Part of the compiler-research project, under the Apache License v2.0 with
// LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Public C API for CppInterOp. Wraps the tablegen-generated declarations
// in CXCppInterOpDecl.inc and adds the hand-written extras that cannot
// be emitted by the generator (NoCWrapper return types, mixed return +
// out-param shapes).
//
//===----------------------------------------------------------------------===//

#ifndef CPPINTEROP_CXCPPINTEROP_H
#define CPPINTEROP_CXCPPINTEROP_H

#include "CppInterOp/CppInterOpTypes.h"

#ifdef __cplusplus
// The generated .inc references the public namespace alias `Cpp` to
// pull C-struct types into scope; mirror that alias here so this
// header is self-sufficient (CppInterOp.h is C++-only, so we can't
// rely on the consumer having included it).
// NOLINTNEXTLINE(misc-unused-alias-decls)
namespace Cpp = CppImpl;

extern "C" {
#endif

#include "CppInterOp/CXCppInterOpDecl.inc"

// --- Hand-written wrappers (see lib/CppInterOp/CXCppInterOp.cpp) ---

/// Returns the templated method scopes inside \c parent matching
/// \c name. The bool-return-with-vector-outparam shape is not
/// expressible by the tablegen wrapper emitter; callers check
/// \c arr.size > 0 to detect "no matches".
CPPINTEROP_API CppInterOpArray
cppinterop_GetClassTemplatedMethods(const char* name, void* parent);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // CPPINTEROP_CXCPPINTEROP_H
