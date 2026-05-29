// Cross-TU performance check for VTableOverlay.
//
// The dispatch loop (OverlayDispatchOnce) lives in TestSharedLib, so the
// call site cannot devirtualize or inline against the derived type known
// here. That makes the measurement honest: it reflects a real AOT caller
// in another library dispatching through the (possibly overlaid) vtable.
//
// VTableOverlay does NOT make the call faster -- it is not devirtualization
// or inlining; it only swaps the function pointer installed at a vtable
// slot. The claim under test is that the swap is *free at the call site*:
// the overlaid path runs the same vtable indirection as a plain virtual
// call, so it costs the same. That zero-overhead property is what enables
// language bindings to redirect dispatch into Python (or another runtime)
// without paying any per-call cost over the C++ virtual baseline.

#include "CppInterOp/CppInterOp.h"
#include "CppInterOp/CppInterOpTypes.h"
#include "TestSharedLib/TestSharedLib.h"

#include "PerfCompare.h"
#include "gtest/gtest.h"

#include <benchmark/benchmark.h>

#include <vector>

namespace {

struct Impl : OverlayBase {
  [[gnu::noinline]] int frob(int x) override { return x + 1; }
};
extern "C" int xtu_replacement(void* /*self*/, int x) { return x + 100; }

// Reflect OverlayBase (definition mirrors TestSharedLib) so the overlay is
// driven through the public reflected API -- the language-binding setting.
Cpp::TCppScope_t ReflectOverlayBase() {
  Cpp::CreateInterpreter({});
  Cpp::Declare("struct OverlayBase {"
               "  OverlayBase();"
               "  virtual ~OverlayBase();"
               "  virtual int frob(int x);"
               "};");
  return Cpp::GetNamed("OverlayBase");
}

Cpp::TCppFunction_t Frob(Cpp::TCppScope_t scope) {
  std::vector<Cpp::TCppFunction_t> methods;
  Cpp::GetClassMethods(scope, methods);
  for (auto* m : methods)
    if (Cpp::GetName(m) == "frob")
      return m;
  return nullptr;
}

Cpp::UniqueVTableOverlay OverlayFrob(void* inst, Cpp::TCppScope_t base) {
  return Cpp::MakeUniqueVTableOverlay(
      inst, base, {{Frob(base), reinterpret_cast<void*>(&xtu_replacement)}});
}

} // namespace

TEST(VTableOverlayCrossTU, OverlayDispatchesAcrossDSO) {
  Impl impl;
  auto* base = ReflectOverlayBase();
  ASSERT_NE(base, nullptr);
  auto ov = OverlayFrob(&impl, base);
  ASSERT_TRUE(ov);
  // The call site is in TestSharedLib; the overlay still takes effect.
  EXPECT_EQ(OverlayDispatchOnce(&impl, 7), 107);
}

static void BM_XTU_BareVirtual(benchmark::State& state) {
  Impl impl;
  for (auto _ : state)
    benchmark::DoNotOptimize(OverlayDispatchOnce(&impl, 7));
}
BENCHMARK(BM_XTU_BareVirtual);

static void BM_XTU_OverlayDirectFn(benchmark::State& state) {
  Impl impl;
  auto* base = ReflectOverlayBase();
  auto ov = OverlayFrob(&impl, base);
  for (auto _ : state)
    benchmark::DoNotOptimize(OverlayDispatchOnce(&impl, 7));
}
BENCHMARK(BM_XTU_OverlayDirectFn);

TEST(VTableOverlayCrossTU, OverlayAddsNoPerCallCost) {
#if !defined(NDEBUG) || defined(__SANITIZE_ADDRESS__)
  GTEST_SKIP() << "Perf assertions need a Release, non-sanitizer build.";
#endif
  EXPECT_NOT_SLOWER_THAN(BM_XTU_OverlayDirectFn, BM_XTU_BareVirtual);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  benchmark::Initialize(&argc, argv);
  return RUN_ALL_TESTS();
}
