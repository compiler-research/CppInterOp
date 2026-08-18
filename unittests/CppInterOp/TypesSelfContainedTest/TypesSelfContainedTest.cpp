// Regression guard: CppInterOpTypes.h must stay a fully self-contained
// header — every helper's implementation lives either inline in the
// header or in a plain-libc-shaped body. If a future contributor adds
// a helper backed by an out-of-line .cpp definition in
// libclangCppInterOp, this translation unit will fail to LINK
// (undefined symbol at ld time) because the executable deliberately
// does not link against libclangCppInterOp.
//
// Consumers reaching the CppInterOp API through Dispatch.h
// (cppyy-backend, xeus-cpp) do not link against libclangCppInterOp —
// they dlopen it lazily via LoadDispatchAPI. A CppInterOpTypes.h that
// referenced an out-of-line symbol from libclangCppInterOp would
// either force those consumers into a DT_NEEDED against the full
// library (the exact class of hazard we hit with
// Cpp::ResultAbort_UncheckedOnDtor) or require routing the helper
// through the dispatch table.
//
// This binary is not run — it's a compile-and-link probe. Success ==
// build succeeded. Failure looks like:
//   undefined reference to `some_new_helper_that_snuck_in'
//
// If you land such a symbol here, either (a) make it a static inline
// in the header, (b) route it through the dispatch table, or (c) split
// the ABI-helper prelude into a truly no-op runtime archive that
// downstream Dispatch consumers can link cheaply.
//
// Follow-up commits extend the probe body as new helpers land in
// CppInterOpTypes.h. Today only the pre-existing types are exercised:
// the opaque handles, TemplateArgInfo, and the CppInterOp{Array,
// StringArray} PODs.

#include "CppInterOp/CppInterOpTypes.h"

#include <cstddef>

// Force ODR-use of every currently-provided CppInterOpTypes.h helper
// so the linker resolves each one. If any helper picks up an
// out-of-line dependency, this function fails to link.
extern "C" int cppinterop_types_self_contained_probe(int seed) {
  // Handle types (Cpp::DeclRef etc.) — POD trivially copyable; a
  // default-constructed handle compares equal to nullptr and yields
  // false in a bool context. All inline, no external symbols.
  Cpp::DeclRef d;
  if (d)
    return -1;

  // TemplateArgInfo — standard-layout POD with a #ifdef __cplusplus
  // constructor. Instantiation touches the ctor without pulling any
  // implementation symbol.
  Cpp::TemplateArgInfo tai(nullptr);
  (void)tai;

  // Array/StringArray helpers — pure PODs. No methods, no ctors that
  // reach out-of-line code.
  Cpp::CppInterOpArray arr = {nullptr, 0};
  Cpp::CppInterOpStringArray sarr = {nullptr, 0};
  return static_cast<int>(seed + arr.size + sarr.size);
}

int main(int argc, char** /*argv*/) {
  // Volatile / argc mixing keeps the compiler from optimizing the probe
  // call away in a Release build. The returned value is uninteresting;
  // the load-bearing signal is that the link succeeded.
  volatile int r = cppinterop_types_self_contained_probe(argc);
  (void)r;
  return 0;
}
