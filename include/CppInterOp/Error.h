//===--- Error.h - Cpp::Result<T> -------------------------------*- C++ -*-===//
//
// Part of the compiler-research project, under the Apache License v2.0 with
// LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Cpp::Result<T> -- the [[nodiscard]] return type fallible APIs use.
// A Result holds either a value of T or an ErrorRef. The discipline
// matches llvm::Expected: call .ok() (or coerce to bool), then .value()
// on success or .error() on failure. value() on an error aborts;
// value_or(fallback) is the lenient form.
//
// In debug builds, dropping an error-bearing Result without inspecting
// it aborts. .ignore() opts out of that check.
//
// This PR is just the type. Later PRs add the boundary translator,
// per-call diagnostics, and structured failure payloads.
//
//===----------------------------------------------------------------------===//

#ifndef CPPINTEROP_ERROR_H
#define CPPINTEROP_ERROR_H

#include "CppInterOp/CppInterOpTypes.h"

#ifdef __cplusplus

#include <cstddef>
#include <cstdint>
#include <new>
#include <type_traits>
#include <utility>

namespace Cpp {

/// Coarse outcome code. Status::Failed is a placeholder -- later PRs
/// refine the taxonomy (NotFound, ParseError, ...) as APIs migrate.
enum class Status : uint8_t {
  Ok = 0,
  Failed,
};

/// 8-byte opaque error handle. nullptr state means "no error". A
/// non-null state is an opaque pointer interpreted by later PRs.
struct ErrorRef {
  const void* state = nullptr;

  [[nodiscard]] bool isOk() const { return state == nullptr; }
};

/// Out-of-line abort for value()-on-error. Keeps Result<T>'s inline
/// body small.
[[noreturn]] CPPINTEROP_API void ResultAbort_ValueOnError(const ErrorRef& Err);

#ifndef NDEBUG
/// Out-of-line abort for the debug must-handle check. Shared between
/// Result<T> and Result<void> so the message and abort path live in
/// one place.
[[noreturn]] CPPINTEROP_API void
ResultAbort_UncheckedOnDtor(const ErrorRef& Err);
#endif

// Result<T>: union of T storage and the ErrorRef pointer, plus a
// HasError discriminator. Sized comparably to llvm::Expected<T>
// (pinned by static_assert in ResultBench). Debug adds an Unchecked
// bit; release shrinks to bare union + discriminator.

template <typename T> class [[nodiscard]] Result {
  static_assert(!std::is_same<T, void>::value,
                "Result<void> is provided via specialization below");

  static constexpr size_t kStorageSize = sizeof(T) > sizeof(void*)
                                             ? sizeof(T)
                                             : sizeof(void*);
  static constexpr size_t kStorageAlign = alignof(T) > alignof(void*)
                                              ? alignof(T)
                                              : alignof(void*);

  union {
    alignas(kStorageAlign) unsigned char m_ValueBytes[kStorageSize];
    const void* m_ErrState; // when m_HasError == true
  };
  bool m_HasError = false;

#ifndef NDEBUG
  mutable bool m_Unchecked = true;
  void markChecked() const { m_Unchecked = false; }
#else
  void markChecked() const {}
#endif

  // reinterpret_cast across the union member -- std::launder is C++17
  // and the header has to parse in C++14 consumer contexts.
  T* valuePtr() { return reinterpret_cast<T*>(m_ValueBytes); }
  const T* valuePtr() const { return reinterpret_cast<const T*>(m_ValueBytes); }

public:
  Result() { new (m_ValueBytes) T(); }

  Result(T V) { new (m_ValueBytes) T(std::move(V)); }

  Result(ErrorRef E) : m_ErrState(E.state), m_HasError(true) {}

  Result(const Result& Other) : m_HasError(Other.m_HasError) {
    if (m_HasError)
      m_ErrState = Other.m_ErrState;
    else
      new (m_ValueBytes) T(*Other.valuePtr());
#ifndef NDEBUG
    m_Unchecked = Other.m_Unchecked;
    Other.m_Unchecked = false;
#endif
  }

  Result(Result&& Other) noexcept : m_HasError(Other.m_HasError) {
    if (m_HasError)
      m_ErrState = Other.m_ErrState;
    else
      new (m_ValueBytes) T(std::move(*Other.valuePtr()));
#ifndef NDEBUG
    m_Unchecked = Other.m_Unchecked;
    Other.m_Unchecked = false;
#endif
  }

  ~Result() {
#ifndef NDEBUG
    // Must-handle enforcement: an error-bearing Result that wasn't
    // inspected aborts. Use .ignore() to drop on purpose.
    if (m_Unchecked && m_HasError)
      ResultAbort_UncheckedOnDtor(ErrorRef{m_ErrState});
#endif
    if (!m_HasError)
      valuePtr()->~T();
  }

  Result& operator=(const Result&) = delete;
  Result& operator=(Result&&) = delete;

  /// Mark this Result checked without reading it -- for when you
  /// genuinely want to drop the outcome.
  void ignore() const { markChecked(); }

  [[nodiscard]] bool ok() const {
    markChecked();
    return !m_HasError;
  }
  explicit operator bool() const { return ok(); }

  [[nodiscard]] Status status() const {
    markChecked();
    return m_HasError ? Status::Failed : Status::Ok;
  }

  [[nodiscard]] ErrorRef error() const {
    markChecked();
    return m_HasError ? ErrorRef{m_ErrState} : ErrorRef{};
  }

  /// Value on Ok; aborts on Error. Use value_or for lenient semantics.
  T value() const {
    markChecked();
    if (m_HasError)
      ResultAbort_ValueOnError(error());
    return *valuePtr();
  }

  T value_or(T fallback) const {
    markChecked();
    return m_HasError ? std::move(fallback) : *valuePtr();
  }
};

// Void specialization: no T storage, ErrorRef stored directly.

template <> class [[nodiscard]] Result<void> {
  const void* m_ErrState = nullptr;
#ifndef NDEBUG
  mutable bool m_Unchecked = true;
  void markChecked() const { m_Unchecked = false; }
#else
  void markChecked() const {}
#endif

public:
  Result() = default;
  Result(ErrorRef E) : m_ErrState(E.state) {}

  Result& operator=(const Result&) = delete;
  Result& operator=(Result&&) = delete;

  Result(const Result& O) : m_ErrState(O.m_ErrState) {
#ifndef NDEBUG
    m_Unchecked = O.m_Unchecked;
    O.m_Unchecked = false;
#endif
  }

  Result(Result&& O) noexcept : m_ErrState(O.m_ErrState) {
#ifndef NDEBUG
    // Transfer the must-handle obligation to the destination -- the
    // default move leaves m_Unchecked=true on the source and the
    // source's dtor would abort even though the value went to the
    // caller (the bug seen when threading Results through tracing
    // record() wrappers).
    m_Unchecked = O.m_Unchecked;
    O.m_Unchecked = false;
#endif
    O.m_ErrState = nullptr;
  }

  ~Result() {
#ifndef NDEBUG
    if (m_Unchecked && m_ErrState)
      ResultAbort_UncheckedOnDtor(ErrorRef{m_ErrState});
#endif
  }

  void ignore() const { markChecked(); }

  [[nodiscard]] bool ok() const {
    markChecked();
    return m_ErrState == nullptr;
  }
  explicit operator bool() const { return ok(); }

  [[nodiscard]] Status status() const {
    markChecked();
    return m_ErrState ? Status::Failed : Status::Ok;
  }

  [[nodiscard]] ErrorRef error() const {
    markChecked();
    return ErrorRef{m_ErrState};
  }
};

} // namespace Cpp

#endif // __cplusplus

#endif // CPPINTEROP_ERROR_H
