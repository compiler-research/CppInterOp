#include "CppInterOp/CppInterOp.h"
#include "CppInterOp/CppInterOpTypes.h"

#include "gtest/gtest.h"

#include <cstdint>
#include <vector>

namespace {

// Replacement functions installed into vtable slots. The leading self
// parameter matches the implicit `this` of the virtual they replace.
extern "C" int repl_negate(void* /*self*/, int x) { return -x; }
extern "C" int repl_double(void* /*self*/, int x) { return x * 2; }

// Replacement that reads the object via `this`: layout is { vptr, int value }
// on every ABI we target, so the data member sits at sizeof(void*).
extern "C" int repl_read_value(void* self, int x) {
  int v = *reinterpret_cast<int*>(static_cast<char*>(self) + sizeof(void*));
  return v + x;
}

// Parallel test-TU definitions of A / B used by the OverlayThroughHierarchy
// test. Layout and ABI come from the host compiler here; the same struct
// shape is declared at interpreter level inside the test so reflection
// computes matching vtable slot indices. foo / bar access the object's
// data members via typed static_cast on the self pointer the vtable slot
// hands them -- exactly the pattern a language binding's adapter writes.
// Virtual destructors keep the vtable layout in line with the other tests
// here (Itanium D1/D0 pair before user virtuals; MSVC single deleting-dtor).
struct A {
  int m_x;
  A(int x) : m_x(x) {}
  virtual ~A() {}
  virtual int method() { return m_x; }
};

struct B : A {
  double m_d;
  B(int x, double d) : A(x), m_d(d) {}
  int method() override { return static_cast<int>(m_d); }
};

extern "C" int foo(void* self) {
  return static_cast<A*>(self)->m_x + 10;
}
extern "C" int bar(void* self) {
  return static_cast<int>(static_cast<B*>(self)->m_x +
                          static_cast<B*>(self)->m_d);
}

int call_slot_no_arg(void* inst, int slot) {
  void** vptr = *reinterpret_cast<void***>(inst);
  return reinterpret_cast<int (*)(void*)>(vptr[slot])(inst);
}

// OverlayB declared through the interpreter so the overlay runs against
// a reflected, JIT-constructed object -- the language-binding setting.
// Itanium has a destructor pair (D1/D0) before the user virtuals; MSVC
// has a single deleting-dtor slot, so the user-virtual indices differ.
Cpp::TCppScope_t DeclareBase() {
  // -include new: Construct's wrapper uses placement new, so every
  // interpreter compilation must see <new>.
  Cpp::CreateInterpreter({"-include", "new"});
  Cpp::Declare("struct OverlayB {"
               "  virtual ~OverlayB() {}"
               "  virtual int alpha(int x) { return x + 10; }"
               "  virtual int beta(int x) { return x + 20; }"
               "};");
  return Cpp::GetNamed("OverlayB");
}

Cpp::TCppFunction_t Method(Cpp::TCppScope_t scope, const char* name) {
  std::vector<Cpp::TCppFunction_t> methods;
  Cpp::GetClassMethods(scope, methods);
  for (auto* m : methods)
    if (Cpp::GetName(m) == name)
      return m;
  return nullptr;
}

// Dispatch through the installed vtable slot. Calling beta() directly in
// this TU would let the compiler devirtualize and bypass the overlay;
// reading the slot tests what the overlay actually installs.
int call_slot(void* inst, int slot, int arg) {
  void** vptr = *reinterpret_cast<void***>(inst);
  return reinterpret_cast<int (*)(void*, int)>(vptr[slot])(inst, arg);
}

#ifdef _WIN32
constexpr int kAlpha = 1;
constexpr int kBeta = 2;
#else
constexpr int kAlpha = 2;
constexpr int kBeta = 3;
#endif

} // namespace

TEST(VTableOverlay, ReplacesSlotPreservingOthers) {
  auto* B = DeclareBase();
  void* inst = Cpp::Construct(B);
  ASSERT_NE(inst, nullptr);
  auto ov = Cpp::MakeUniqueVTableOverlay(
      inst, B, {{Method(B, "beta"), reinterpret_cast<void*>(&repl_negate)}});
  ASSERT_TRUE(ov);
  EXPECT_EQ(call_slot(inst, kBeta, 5), -5);  // overlaid
  EXPECT_EQ(call_slot(inst, kAlpha, 5), 15); // preserved: alpha -> x+10
  ov.reset(); // restore the vptr before freeing the object
  Cpp::Destruct(inst, B);
}

TEST(VTableOverlay, PreservesPrefixAndUnrelatedSlots) {
  auto* B = DeclareBase();
  void* inst = Cpp::Construct(B);
  ASSERT_NE(inst, nullptr);
  void** aot = *reinterpret_cast<void***>(inst);
  // Prefix slot(s) immediately before the address point: Itanium has
  // offset-to-top at [-2] and type_info at [-1]; MSVC has just the
  // complete-object-locator at [-1].
  void* prefix_m1 = aot[-1];
#ifndef _WIN32
  void* prefix_m2 = aot[-2];
#endif
  void* alpha_slot = aot[kAlpha];
  auto ov = Cpp::MakeUniqueVTableOverlay(
      inst, B, {{Method(B, "beta"), reinterpret_cast<void*>(&repl_negate)}});
  ASSERT_TRUE(ov);
  void** now = *reinterpret_cast<void***>(inst);
  EXPECT_EQ(now[-1], prefix_m1);
#ifndef _WIN32
  EXPECT_EQ(now[-2], prefix_m2);
#endif
  EXPECT_EQ(now[kAlpha], alpha_slot); // unrelated slot copied verbatim
  ov.reset();
  Cpp::Destruct(inst, B);
}

TEST(VTableOverlay, RestoresOnDestroy) {
  auto* B = DeclareBase();
  void* inst = Cpp::Construct(B);
  ASSERT_NE(inst, nullptr);
  void* aot = *reinterpret_cast<void**>(inst);
  {
    auto ov = Cpp::MakeUniqueVTableOverlay(
        inst, B, {{Method(B, "beta"), reinterpret_cast<void*>(&repl_negate)}});
    ASSERT_TRUE(ov);
    EXPECT_NE(*reinterpret_cast<void**>(inst), aot);
  }
  EXPECT_EQ(*reinterpret_cast<void**>(inst), aot);
  EXPECT_EQ(call_slot(inst, kBeta, 5), 25); // original beta restored: x+20
  Cpp::Destruct(inst, B); // overlay already released by the inner scope
}

TEST(VTableOverlay, ReplacesMultipleSlots) {
  auto* B = DeclareBase();
  void* inst = Cpp::Construct(B);
  ASSERT_NE(inst, nullptr);
  auto ov = Cpp::MakeUniqueVTableOverlay(
      inst, B,
      {{Method(B, "alpha"), reinterpret_cast<void*>(&repl_negate)},
       {Method(B, "beta"), reinterpret_cast<void*>(&repl_double)}});
  ASSERT_TRUE(ov);
  EXPECT_EQ(call_slot(inst, kAlpha, 5), -5);
  EXPECT_EQ(call_slot(inst, kBeta, 5), 10);
  ov.reset();
  Cpp::Destruct(inst, B);
}

TEST(VTableOverlay, RejectsInvalidInput) {
  auto* B = DeclareBase();
  void* inst = Cpp::Construct(B);
  ASSERT_NE(inst, nullptr);
  Cpp::TCppConstFunction_t beta = Method(B, "beta");
  Cpp::TCppConstFunction_t none = nullptr;
  void* fn = reinterpret_cast<void*>(&repl_negate);

  EXPECT_EQ(Cpp::MakeVTableOverlay(nullptr, B, &beta, &fn, 1), nullptr); // inst
  EXPECT_EQ(Cpp::MakeVTableOverlay(inst, nullptr, &beta, &fn, 1),
            nullptr);                                                 // base
  EXPECT_EQ(Cpp::MakeVTableOverlay(inst, B, &none, &fn, 1), nullptr); // method
  Cpp::Destruct(inst, B);
}

// Sibling instance of the same type is unaffected when another instance is
// overlaid -- proves per-instance scope of the vptr swap (not per-class).
TEST(VTableOverlay, OverlayIsPerInstance) {
  auto* B = DeclareBase();
  void* a = Cpp::Construct(B);
  void* b = Cpp::Construct(B);
  ASSERT_NE(a, nullptr);
  ASSERT_NE(b, nullptr);
  void* b_vptr_before = *reinterpret_cast<void**>(b);

  auto ov = Cpp::MakeUniqueVTableOverlay(
      a, B, {{Method(B, "beta"), reinterpret_cast<void*>(&repl_negate)}});
  ASSERT_TRUE(ov);

  EXPECT_NE(*reinterpret_cast<void**>(a), b_vptr_before); // a swapped
  EXPECT_EQ(*reinterpret_cast<void**>(b), b_vptr_before); // b untouched
  EXPECT_EQ(call_slot(a, kBeta, 5), -5);  // overlay on a
  EXPECT_EQ(call_slot(b, kBeta, 5), 25);  // original on b

  ov.reset();
  Cpp::Destruct(a, B);
  Cpp::Destruct(b, B);
}

// Thunks receive the unmodified `this` pointer of the overlaid instance, so
// they can read the object's data members directly. Layout assumed by the
// test: [ vptr ][ int value ] (true on every ABI we build for).
TEST(VTableOverlay, ThunkReadsThisAndDataMember) {
  Cpp::CreateInterpreter({"-include", "new"});
  Cpp::Declare("struct OverlayWithData {"
               "  int value;"
               "  OverlayWithData() : value(100) {}"
               "  virtual ~OverlayWithData() {}"
               "  virtual int frob(int x) { return x; }"
               "};");
  auto* T = Cpp::GetNamed("OverlayWithData");
  ASSERT_NE(T, nullptr);
  void* inst = Cpp::Construct(T);
  ASSERT_NE(inst, nullptr);

  auto ov = Cpp::MakeUniqueVTableOverlay(
      inst, T, {{Method(T, "frob"), reinterpret_cast<void*>(&repl_read_value)}});
  ASSERT_TRUE(ov);

  // First user virtual lives at kAlpha (Itanium D1/D0 prefix; MSVC single
  // deleting-dtor) -- the same layout the existing tests use.
  EXPECT_EQ(call_slot(inst, kAlpha, 5), 105); // value(=100) + x(=5)
  ov.reset();
  Cpp::Destruct(inst, T);
}

// Derived class with an override has the same single-vtable layout as the
// base; overlay still finds and replaces the (overridden) slot.
TEST(VTableOverlay, DerivedClassWithOverride) {
  Cpp::CreateInterpreter({"-include", "new"});
  Cpp::Declare("struct DvBase {"
               "  virtual ~DvBase() {}"
               "  virtual int frob(int x) { return x + 1; }"
               "};"
               "struct DvDerived : DvBase {"
               "  int frob(int x) override { return x + 2; }"
               "};");
  auto* D = Cpp::GetNamed("DvDerived");
  ASSERT_NE(D, nullptr);
  void* inst = Cpp::Construct(D);
  ASSERT_NE(inst, nullptr);

  // Sanity-check the derived's original slot dispatches to DvDerived::frob.
  EXPECT_EQ(call_slot(inst, kAlpha, 5), 7);

  auto ov = Cpp::MakeUniqueVTableOverlay(
      inst, D, {{Method(D, "frob"), reinterpret_cast<void*>(&repl_negate)}});
  ASSERT_TRUE(ov);
  EXPECT_EQ(call_slot(inst, kAlpha, 5), -5);
  ov.reset();
  Cpp::Destruct(inst, D);
}

// Three-level single-inheritance chain (A <- B <- C). Slot layout is still
// flat (one vptr), so overlay on C through C's scope replaces the slot.
TEST(VTableOverlay, MultiLevelInheritance) {
  Cpp::CreateInterpreter({"-include", "new"});
  Cpp::Declare("struct MlA { virtual ~MlA() {} virtual int frob(int x) { return x + 1; } };"
               "struct MlB : MlA { int frob(int x) override { return x + 2; } };"
               "struct MlC : MlB { int frob(int x) override { return x + 3; } };");
  auto* C = Cpp::GetNamed("MlC");
  ASSERT_NE(C, nullptr);
  void* inst = Cpp::Construct(C);
  ASSERT_NE(inst, nullptr);

  EXPECT_EQ(call_slot(inst, kAlpha, 5), 8); // MlC::frob: x+3

  auto ov = Cpp::MakeUniqueVTableOverlay(
      inst, C, {{Method(C, "frob"), reinterpret_cast<void*>(&repl_double)}});
  ASSERT_TRUE(ov);
  EXPECT_EQ(call_slot(inst, kAlpha, 5), 10); // overlay: x*2
  ov.reset();
  Cpp::Destruct(inst, C);
}

// Multiple inheritance with two polymorphic direct bases produces two
// vptrs (one per non-empty base subobject); the primary-vptr-only overlay
// cannot retarget the secondary-base dispatch path. MakeVTableOverlay
// refuses such a layout outright so the caller never gets back a handle
// that would mis-dispatch through the untouched secondary vptr.
TEST(VTableOverlay, RejectsMultipleInheritance) {
  Cpp::CreateInterpreter({"-include", "new"});
  Cpp::Declare("struct MiA { virtual ~MiA() {} virtual int af(int x) { return x + 10; } };"
               "struct MiB { virtual ~MiB() {} virtual int bf(int x) { return x + 20; } };"
               "struct MiC : MiA, MiB {"
               "  int af(int x) override { return x + 11; }"
               "  int bf(int x) override { return x + 22; }"
               "};");
  auto* C = Cpp::GetNamed("MiC");
  ASSERT_NE(C, nullptr);
  void* inst = Cpp::Construct(C);
  ASSERT_NE(inst, nullptr);

  auto ov = Cpp::MakeUniqueVTableOverlay(
      inst, C, {{Method(C, "af"), reinterpret_cast<void*>(&repl_negate)}});
  EXPECT_FALSE(ov); // refuses the layout

  Cpp::Destruct(inst, C);
}

// Virtual inheritance has a longer pre-address-point prefix (vbase-offset
// entries) and a vtable-in-vbase that carries `_ZTv0_n*` virtual thunks for
// dispatch through the virtual-base pointer -- neither is covered by the
// primary-vptr overlay. MakeVTableOverlay refuses, mirroring the multi-
// inheritance case. Reviewer ref: stackoverflow.com/a/39182009.
// Non-polymorphic class has no vtable to overlay; rejected at the
// RD->isPolymorphic() gate inside MakeVTableOverlay.
TEST(VTableOverlay, RejectsNonPolymorphicBase) {
  Cpp::CreateInterpreter({"-include", "new"});
  Cpp::Declare("struct NonPoly { int x; };"
               "struct PolyMethodHolder {"
               "  virtual int dummy(int x) { return x; }"
               "};");
  auto* NP = Cpp::GetNamed("NonPoly");
  auto* PH = Cpp::GetNamed("PolyMethodHolder");
  ASSERT_NE(NP, nullptr);
  ASSERT_NE(PH, nullptr);
  void* inst = Cpp::Construct(NP);
  ASSERT_NE(inst, nullptr);
  Cpp::TCppConstFunction_t dummy = Method(PH, "dummy");
  void* fn = reinterpret_cast<void*>(&repl_negate);
  EXPECT_EQ(Cpp::MakeVTableOverlay(inst, NP, &dummy, &fn, 1), nullptr);
  Cpp::Destruct(inst, NP);
}

// A virtual method from an unrelated, larger class has a slot index that
// exceeds the target base's vtable size; applyVTableOverlay's per-slot
// bounds check rejects rather than writing past the overlay block.
TEST(VTableOverlay, RejectsOutOfRangeMethodSlot) {
  Cpp::CreateInterpreter({"-include", "new"});
  Cpp::Declare("struct SmallBase {"
               "  virtual ~SmallBase() {}"
               "  virtual int sf(int x) { return x; }"
               "};"
               "struct LargeUnrelated {"
               "  virtual ~LargeUnrelated() {}"
               "  virtual int v1(int) { return 0; }"
               "  virtual int v2(int) { return 0; }"
               "  virtual int v3(int) { return 0; }"
               "  virtual int v4(int) { return 0; }"
               "  virtual int v5(int) { return 0; }"
               "  virtual int v6(int) { return 0; }"
               "  virtual int v7(int) { return 0; }"
               "  virtual int v8(int) { return 0; }"
               "  virtual int v9(int) { return 0; }"
               "  virtual int v10(int) { return 0; }"
               "};");
  auto* Small = Cpp::GetNamed("SmallBase");
  auto* Large = Cpp::GetNamed("LargeUnrelated");
  ASSERT_NE(Small, nullptr);
  ASSERT_NE(Large, nullptr);
  void* inst = Cpp::Construct(Small);
  ASSERT_NE(inst, nullptr);
  Cpp::TCppConstFunction_t v10 = Method(Large, "v10");
  void* fn = reinterpret_cast<void*>(&repl_negate);
  EXPECT_EQ(Cpp::MakeVTableOverlay(inst, Small, &v10, &fn, 1), nullptr);
  Cpp::Destruct(inst, Small);
}

// Overlay across a A <- B hierarchy using replacements that read the
// object's data members through typed static_cast on `self`. Objects are
// stack-allocated by the test TU (matching the interpreter-side layout the
// reflection API computes the slot indices for) and initialised with
// non-zero member values; the replacements pull those values back out and
// fold them into the return, exercising the live-`this` path end to end.
TEST(VTableOverlay, OverlayThroughHierarchyAccessesDataMembers) {
  Cpp::CreateInterpreter({});
  Cpp::Declare("struct A {"
               "  int m_x;"
               "  A(int x) : m_x(x) {}"
               "  virtual ~A() {}"
               "  virtual int method() { return m_x; }"
               "};"
               "struct B : A {"
               "  double m_d;"
               "  B(int x, double d) : A(x), m_d(d) {}"
               "  int method() override { return (int)m_d; }"
               "};");
  auto* A_scope = Cpp::GetNamed("A");
  auto* B_scope = Cpp::GetNamed("B");
  ASSERT_NE(A_scope, nullptr);
  ASSERT_NE(B_scope, nullptr);

  A a(5);
  B b(7, 3.5);

  // Baseline: the host-compiler-built objects dispatch to their C++ bodies.
  EXPECT_EQ(call_slot_no_arg(&a, kAlpha), 5);  // A::method returns m_x
  EXPECT_EQ(call_slot_no_arg(&b, kAlpha), 3);  // B::method returns (int)m_d

  auto ov_a = Cpp::MakeUniqueVTableOverlay(
      &a, A_scope,
      {{Method(A_scope, "method"), reinterpret_cast<void*>(&foo)}});
  ASSERT_TRUE(ov_a);
  auto ov_b = Cpp::MakeUniqueVTableOverlay(
      &b, B_scope,
      {{Method(B_scope, "method"), reinterpret_cast<void*>(&bar)}});
  ASSERT_TRUE(ov_b);

  EXPECT_EQ(call_slot_no_arg(&a, kAlpha), 15); // foo: A::m_x(5) + 10
  EXPECT_EQ(call_slot_no_arg(&b, kAlpha), 10); // bar: (int)(7 + 3.5)

  ov_a.reset();
  ov_b.reset();
  // No Destruct: stack-allocated objects, C++ dtors fire on scope exit
  // (vptrs restored above so virtual dispatch lands on the real dtor).
}

TEST(VTableOverlay, RejectsVirtualInheritance) {
  Cpp::CreateInterpreter({"-include", "new"});
  Cpp::Declare("struct ViBase {"
               "  virtual ~ViBase() {}"
               "  virtual int frob(int x) { return x + 1; }"
               "};"
               "struct ViDerived : virtual ViBase {"
               "  int frob(int x) override { return x + 2; }"
               "};");
  auto* D = Cpp::GetNamed("ViDerived");
  ASSERT_NE(D, nullptr);
  void* inst = Cpp::Construct(D);
  ASSERT_NE(inst, nullptr);

  auto ov = Cpp::MakeUniqueVTableOverlay(
      inst, D, {{Method(D, "frob"), reinterpret_cast<void*>(&repl_negate)}});
  EXPECT_FALSE(ov); // refuses the layout

  Cpp::Destruct(inst, D);
}
