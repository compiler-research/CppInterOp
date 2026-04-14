#include "../../lib/CppInterOp/Tracing.h"

#include "CppInterOp/CppInterOp.h"

#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"

#include <fstream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace CppInterOp::Tracing;
using ::testing::HasSubstr;
using ::testing::Not;

// Helper: join the full trace log into a single string for matching.
static std::string getFullLog() {
  std::string result;
  for (const auto& entry : TraceInfo::TheTraceInfo->getLog())
    result += entry + "\n";
  return result;
}

// ---------------------------------------------------------------------------
// Fixtures
// ---------------------------------------------------------------------------

class TracingTest : public ::testing::Test {
protected:
  void SetUp() override {
    if (TraceInfo::TheTraceInfo)
      TraceInfo::TheTraceInfo->clear();
    TraceInfo::TheTraceInfo = nullptr;
    InitTracing();
  }
};

class NoTracingTest : public ::testing::Test {
protected:
  void SetUp() override { TraceInfo::TheTraceInfo = nullptr; }
};

// ---------------------------------------------------------------------------
// Helpers — small functions that use the macros
// ---------------------------------------------------------------------------

int UnannotatedFunction(int x) {
  INTEROP_TRACE(x);
  return x * 2; // Missing INTEROP_RETURN!
}

int AnnotatedFunction(int x) {
  INTEROP_TRACE(x);
  return INTEROP_RETURN(x * 2);
}

void VoidFunc() {
  INTEROP_TRACE();
  return INTEROP_VOID_RETURN();
}

void FillHandles(std::vector<void*>& out) {
  INTEROP_TRACE(INTEROP_OUT(out));
  out.push_back((void*)0xA);
  out.push_back((void*)0xB);
  out.push_back((void*)0xC);
  return INTEROP_VOID_RETURN();
}

// ---------------------------------------------------------------------------
// Tests: disabled tracing
// ---------------------------------------------------------------------------

TEST_F(NoTracingTest, ZeroCostWhenDisabled) {
  EXPECT_EQ(UnannotatedFunction(5), 10);
}

TEST_F(NoTracingTest, RespectsEnvVar) {
  TraceInfo TI;
  EXPECT_FALSE(TI.isEnabled());
}

// ---------------------------------------------------------------------------
// Tests: basic tracing
// ---------------------------------------------------------------------------

TEST_F(TracingTest, DetectUnannotatedBranch) {
#ifndef NDEBUG
  EXPECT_DEATH({ UnannotatedFunction(5); }, "Unannotated exit branch detected");
#endif
}

// A function where INTEROP_TRACE is missing an argument — simulates a
// developer adding a parameter but forgetting to update the trace call.
void MissingTraceArg(int x, int y) {
  INTEROP_TRACE(x); // only passes x, not y
  return INTEROP_VOID_RETURN();
}

TEST_F(TracingTest, DetectMissingTraceArgument) {
#ifndef NDEBUG
  EXPECT_DEATH({ MissingTraceArg(1, 2); }, "argument count mismatch");
#endif
}

// A function with an out-param where INTEROP_OUT is forgotten.
void MissingOutParam(int x, std::vector<void*>& out) {
  INTEROP_TRACE(x); // missing INTEROP_OUT(out)
  out.push_back((void*)0x1);
  return INTEROP_VOID_RETURN();
}

TEST_F(TracingTest, DetectMissingOutParam) {
#ifndef NDEBUG
  std::vector<void*> v;
  EXPECT_DEATH({ MissingOutParam(1, v); }, "argument count mismatch");
#endif
}

TEST_F(TracingTest, CountParamsHandlesVoidParamList) {
  // MSVC's __FUNCSIG__ uses "(void)" for zero-parameter functions.
  // Verify countParams handles both styles correctly.
  using TR = CppInterOp::Tracing::TraceRegion;
  EXPECT_EQ(TR::countParams("void foo()"), 0U);
  EXPECT_EQ(TR::countParams("void foo(void)"), 0U);
  EXPECT_EQ(TR::countParams("void *__cdecl CppImpl::GetInterpreter(void)"), 0U);
  EXPECT_EQ(TR::countParams("void foo(int)"), 1U);
  EXPECT_EQ(TR::countParams("void foo(int, double)"), 2U);
  EXPECT_EQ(TR::countParams("void foo(std::vector<int>&)"), 1U);
  EXPECT_EQ(TR::countParams("void foo(std::map<int, int>&)"), 1U);
  EXPECT_EQ(TR::countParams("void foo(int, std::vector<int>&)"), 2U);
}

TEST_F(TracingTest, ValidAnnotatedBranch) {
  EXPECT_EQ(AnnotatedFunction(5), 10);
}

TEST_F(TracingTest, VoidFunctionTracing) {
  VoidFunc();
  auto output = TraceInfo::TheTraceInfo->getLastLogEntry();
  EXPECT_THAT(output, HasSubstr("CppInterOp::VoidFunc();"));
}

// ---------------------------------------------------------------------------
// Tests: INTEROP_TRACE with and without arguments (__VA_OPT__ coverage)
// ---------------------------------------------------------------------------

void NoArgTrace() {
  INTEROP_TRACE();
  return INTEROP_VOID_RETURN();
}

void WithOutParamTrace(std::vector<void*>& out) {
  INTEROP_TRACE(INTEROP_OUT(out));
  out.push_back((void*)0xDE);
  return INTEROP_VOID_RETURN();
}

TEST_F(TracingTest, TraceWithNoArgs) {
  NoArgTrace();
  auto output = TraceInfo::TheTraceInfo->getLastLogEntry();
  EXPECT_THAT(output, HasSubstr("CppInterOp::NoArgTrace();"));
}

TEST_F(TracingTest, TraceWithOutParamArg) {
  std::vector<void*> v;
  WithOutParamTrace(v);
  auto output = TraceInfo::TheTraceInfo->getLastLogEntry();
  // OutParam should not appear in the arg list.
  EXPECT_THAT(output, HasSubstr("CppInterOp::WithOutParamTrace();"));
  EXPECT_EQ(v.size(), 1u);
}

// ---------------------------------------------------------------------------
// Tests: argument formatting in reproducer output
// ---------------------------------------------------------------------------

void* FuncReturningHandle() {
  INTEROP_TRACE();
  return INTEROP_RETURN((void*)0xBEEF);
}

void FuncTakingHandle(void* h) {
  INTEROP_TRACE(h);
  return INTEROP_VOID_RETURN();
}

void FuncTakingString(const char* s) {
  INTEROP_TRACE(s);
  return INTEROP_VOID_RETURN();
}

void FuncTakingInt(int x) {
  INTEROP_TRACE(x);
  return INTEROP_VOID_RETURN();
}

void FuncTakingMixed(void* h, const char* s, int x) {
  INTEROP_TRACE(h, s, x);
  return INTEROP_VOID_RETURN();
}

void FuncWithHandleAndOut(void* h, std::vector<void*>& out) {
  INTEROP_TRACE(h, INTEROP_OUT(out));
  out.push_back((void*)0xF00);
  return INTEROP_VOID_RETURN();
}

TEST_F(TracingTest, ArgFormattingHandle) {
  void* h = FuncReturningHandle();
  FuncTakingHandle(h);
  auto output = getFullLog();
  // The handle should be registered as v1, then reused.
  EXPECT_THAT(output, HasSubstr("auto v1 = CppInterOp::FuncReturningHandle()"));
  EXPECT_THAT(output, HasSubstr("CppInterOp::FuncTakingHandle(v1)"));
}

TEST_F(TracingTest, ArgFormattingString) {
  FuncTakingString("hello");
  auto output = TraceInfo::TheTraceInfo->getLastLogEntry();
  EXPECT_THAT(output, HasSubstr("CppInterOp::FuncTakingString(\"hello\")"));
}

TEST_F(TracingTest, ArgFormattingInt) {
  FuncTakingInt(42);
  auto output = TraceInfo::TheTraceInfo->getLastLogEntry();
  EXPECT_THAT(output, HasSubstr("CppInterOp::FuncTakingInt(42)"));
}

TEST_F(TracingTest, ArgFormattingMixed) {
  void* h = FuncReturningHandle();
  FuncTakingMixed(h, "world", 99);
  auto output = getFullLog();
  EXPECT_THAT(output,
              HasSubstr("CppInterOp::FuncTakingMixed(v1, \"world\", 99)"));
}

TEST_F(TracingTest, ArgFormattingOutParamSkipped) {
  // OutParam should not appear in the formatted args,
  // but regular args before it should.
  void* h = FuncReturningHandle();
  std::vector<void*> out;
  FuncWithHandleAndOut(h, out);
  auto output = getFullLog();
  // Should show only the handle arg, not the OutParam.
  EXPECT_THAT(output, HasSubstr("CppInterOp::FuncWithHandleAndOut(v1)"));
}

// Test: when two functions return the same pointer, the second should not
// redeclare the handle variable — it should emit a comment instead.
void* ReturnSameHandle() {
  INTEROP_TRACE();
  return INTEROP_RETURN((void*)0xDA1E);
}

TEST_F(TracingTest, DuplicateHandleNotRedeclared) {
  void* h1 = ReturnSameHandle(); // first call → "auto v1 = ..."
  void* h2 = ReturnSameHandle(); // same pointer → "/*v1*/ ..."
  (void)h1;
  (void)h2;
  auto output = getFullLog();

  // Count occurrences of "auto v1 ="
  size_t firstDecl = output.find("auto v1 =");
  ASSERT_NE(firstDecl, std::string::npos) << "First call should declare v1";

  // The second occurrence should NOT be "auto v1 =" but "/*v1*/"
  size_t secondCall =
      output.find("CppInterOp::ReturnSameHandle()", firstDecl + 1);
  ASSERT_NE(secondCall, std::string::npos) << "Should have a second call";

  // There should be exactly one "auto v1 =" in the output.
  size_t secondDecl = output.find("auto v1 =", firstDecl + 1);
  EXPECT_EQ(secondDecl, std::string::npos)
      << "Handle v1 should not be redeclared. Output:\n"
      << output;

  // The second call should have the comment form.
  EXPECT_THAT(output, HasSubstr("/*v1*/"));
}

// Test: when a pointer-returning function returns nullptr, the output
// should annotate it as /*nullptr*/ so the reader knows.
void* ReturnNull() {
  INTEROP_TRACE();
  return INTEROP_RETURN(nullptr);
}

TEST_F(TracingTest, NullptrReturnAnnotated) {
  ReturnNull();
  auto output = TraceInfo::TheTraceInfo->getLastLogEntry();
  EXPECT_THAT(output, HasSubstr("/*nullptr*/"));
  EXPECT_THAT(output, HasSubstr("CppInterOp::ReturnNull()"));
}

// Test: non-void, non-pointer returns (int, bool, size_t) should NOT
// get any handle annotation.
int ReturnInt() {
  INTEROP_TRACE();
  return INTEROP_RETURN(42);
}

TEST_F(TracingTest, NonPointerReturnNoAnnotation) {
  ReturnInt();
  auto output = TraceInfo::TheTraceInfo->getLastLogEntry();
  EXPECT_THAT(output, Not(HasSubstr("auto")));
  EXPECT_THAT(output, Not(HasSubstr("nullptr")));
  EXPECT_THAT(output, HasSubstr("CppInterOp::ReturnInt()"));
}

// Test: non-streamable types (e.g. std::vector) are formatted as placeholders.
void FuncTakingVector(const std::vector<const char*>& args) {
  INTEROP_TRACE(args);
  return INTEROP_VOID_RETURN();
}

TEST_F(TracingTest, ArgFormattingNonStreamable) {
  std::vector<const char*> v = {"a", "b"};
  FuncTakingVector(v);
  auto output = TraceInfo::TheTraceInfo->getLastLogEntry();
  // Non-streamable types should get a {...} placeholder.
  EXPECT_THAT(output, HasSubstr("CppInterOp::FuncTakingVector({...})"));
}

// ---------------------------------------------------------------------------
// Tests: handle registry
// ---------------------------------------------------------------------------

TEST_F(TracingTest, HandleRegistry) {
  TraceInfo& TI = *TraceInfo::TheTraceInfo;
  void* ptr1 = (void*)0x1234;
  void* ptr2 = (void*)0x5678;

  std::string v1 = TI.getOrRegisterHandle(ptr1);
  EXPECT_FALSE(v1.empty());
  EXPECT_EQ(v1[0], 'v');

  EXPECT_EQ(TI.getOrRegisterHandle(ptr1), v1);
  EXPECT_EQ(TI.lookupHandle(ptr1), v1);

  std::string v2 = TI.getOrRegisterHandle(ptr2);
  EXPECT_NE(v1, v2);
  EXPECT_EQ(TI.lookupHandle(ptr2), v2);

  EXPECT_EQ(TI.lookupHandle(nullptr), "nullptr");

  // getOrRegisterHandle(nullptr) should return empty, not register.
  EXPECT_EQ(TI.getOrRegisterHandle(nullptr), "");
}

// ---------------------------------------------------------------------------
// Tests: timer stacking
// ---------------------------------------------------------------------------

TEST_F(TracingTest, TimerStackPushPop) {
  TraceInfo& TI = *TraceInfo::TheTraceInfo;

  llvm::Timer& T1 = TI.getTimer("FuncA");
  llvm::Timer& T2 = TI.getTimer("FuncB");

  // Push T1 — it should be running.
  TI.pushTimer(&T1);
  EXPECT_TRUE(T1.isRunning());

  // Push T2 — T1 paused, T2 running (covers line 75: nested stop).
  TI.pushTimer(&T2);
  EXPECT_FALSE(T1.isRunning());
  EXPECT_TRUE(T2.isRunning());

  // Pop T2 — T1 resumes (covers line 86: restart parent).
  TI.popTimer();
  EXPECT_TRUE(T1.isRunning());
  EXPECT_FALSE(T2.isRunning());

  // Pop T1 — nothing running.
  TI.popTimer();
  EXPECT_FALSE(T1.isRunning());

  // Pop on empty stack — should not crash (covers line 82).
  TI.popTimer();
}

TEST_F(TracingTest, ClearWithRunningTimers) {
  TraceInfo& TI = *TraceInfo::TheTraceInfo;

  llvm::Timer& T1 = TI.getTimer("RunningTimer");
  TI.pushTimer(&T1);
  EXPECT_TRUE(T1.isRunning());

  // clear() should stop running timers (covers lines 129-130).
  TI.clear();
  EXPECT_FALSE(T1.isRunning());
}

// ---------------------------------------------------------------------------
// Tests: StartTracing activates tracing if not yet active
// ---------------------------------------------------------------------------

TEST(StartTracingActivation, ActivatesIfNotActive) {
  // Save and clear TheTraceInfo to simulate tracing not being active.
  auto* saved = TraceInfo::TheTraceInfo;
  TraceInfo::TheTraceInfo = nullptr;

  // StartTracing should call InitTracing (covers line 151).
  std::string Path = CppInterOp::Tracing::StartTracing();
  ASSERT_NE(TraceInfo::TheTraceInfo, nullptr);
  ASSERT_FALSE(Path.empty());

  CppInterOp::Tracing::StopTracing();
  llvm::sys::fs::remove(Path);

  // Restore so other tests aren't affected.
  TraceInfo::TheTraceInfo = saved;
}

// ---------------------------------------------------------------------------
// Tests: countParams edge cases
// ---------------------------------------------------------------------------

TEST_F(TracingTest, CountParamsEdgeCases) {
  using TR = CppInterOp::Tracing::TraceRegion;

  // No parenthesis at all (covers line 333).
  EXPECT_EQ(TR::countParams("nosig"), 0U);

  // Spaces after opening paren (covers line 338).
  EXPECT_EQ(TR::countParams("void foo( int x )"), 1U);
  EXPECT_EQ(TR::countParams("void foo(  )"), 0U);
}

// ---------------------------------------------------------------------------
// Tests: handle chaining across calls
// ---------------------------------------------------------------------------

TEST_F(TracingTest, ChainedReproducerLogic) {
  {
    void* h;
    {
      INTEROP_TRACE();
      h = (void*)0x111;
      INTEROP_RETURN(h);
    }
    {
      INTEROP_TRACE();
      INTEROP_RETURN(0);
    }
  }
  auto output = getFullLog();
  EXPECT_THAT(output, HasSubstr("auto v1 ="));
}

// ---------------------------------------------------------------------------
// Tests: out-parameters
// ---------------------------------------------------------------------------

TEST_F(TracingTest, OutParamRegistersHandles) {
  TraceInfo& TI = *TraceInfo::TheTraceInfo;

  std::vector<void*> results;
  FillHandles(results);

  // The three handles pushed inside FillHandles should now be registered.
  EXPECT_NE(TI.lookupHandle((void*)0xA), "nullptr");
  EXPECT_NE(TI.lookupHandle((void*)0xB), "nullptr");
  EXPECT_NE(TI.lookupHandle((void*)0xC), "nullptr");
}

TEST_F(TracingTest, OutParamHandlesUsableInLaterCalls) {
  TraceInfo& TI = *TraceInfo::TheTraceInfo;

  std::vector<void*> results;
  FillHandles(results);

  // The handle should have been registered by the OutParam mechanism.
  std::string handleName = TI.lookupHandle(results[0]);
  EXPECT_EQ(handleName[0], 'v');
}

TEST_F(NoTracingTest, OutParamNoOpWhenDisabled) {
  std::vector<void*> results;
  // Should not crash when tracing is disabled.
  FillHandles(results);
  EXPECT_EQ(results.size(), 3u);
}

// Out-param with non-pointer elements (e.g., std::vector<std::string>)
// should compile and work — handles are only registered for pointer types.
void FillStrings(std::vector<std::string>& out) {
  INTEROP_TRACE(INTEROP_OUT(out));
  out.push_back("hello");
  out.push_back("world");
  return INTEROP_VOID_RETURN();
}

TEST_F(TracingTest, OutParamNonPointerElementType) {
  std::vector<std::string> results;
  FillStrings(results);
  auto output = TraceInfo::TheTraceInfo->getLastLogEntry();

  EXPECT_EQ(results.size(), 2u);
  EXPECT_EQ(results[0], "hello");
  EXPECT_EQ(results[1], "world");
  EXPECT_THAT(output, HasSubstr("CppInterOp::FillStrings();"));
}

TEST_F(NoTracingTest, OutParamNonPointerDisabled) {
  std::vector<std::string> results;
  FillStrings(results);
  EXPECT_EQ(results.size(), 2u);
}

// ---------------------------------------------------------------------------
// Tests: writeToFile
// ---------------------------------------------------------------------------

TEST_F(TracingTest, WriteToFileProducesValidReproducer) {
  // Generate some trace entries.
  {
    INTEROP_TRACE();
    INTEROP_RETURN((void*)0xF00);
  }
  {
    INTEROP_TRACE();
    return INTEROP_VOID_RETURN();
  }

  TraceInfo& TI = *TraceInfo::TheTraceInfo;
  std::string Path = TI.writeToFile();
  ASSERT_FALSE(Path.empty());

  // Read the file back.
  std::ifstream ifs(Path);
  ASSERT_TRUE(ifs.good());
  std::string content((std::istreambuf_iterator<char>(ifs)),
                      std::istreambuf_iterator<char>());

  EXPECT_THAT(content, HasSubstr("#include <CppInterOp/CppInterOp.h>"));
  EXPECT_THAT(content, HasSubstr("int main()"));
  EXPECT_THAT(content, HasSubstr("CppInterOp::"));

  // Clean up.
  llvm::sys::fs::remove(Path);
}

TEST_F(TracingTest, WriteToFileContainsOutParamCalls) {
  std::vector<void*> results;
  FillHandles(results);

  TraceInfo& TI = *TraceInfo::TheTraceInfo;
  std::string Path = TI.writeToFile();
  ASSERT_FALSE(Path.empty());

  std::ifstream ifs(Path);
  std::string content((std::istreambuf_iterator<char>(ifs)),
                      std::istreambuf_iterator<char>());

  EXPECT_THAT(content, HasSubstr("CppInterOp::FillHandles();"));

  llvm::sys::fs::remove(Path);
}

// ---------------------------------------------------------------------------
// Tests: JitCall wrapper source and Invoke are logged
// ---------------------------------------------------------------------------

TEST_F(TracingTest, JitCallWrapperSourceLogged) {
  Cpp::CreateInterpreter({});
  ASSERT_NE(TraceInfo::TheTraceInfo, nullptr);

  // Placement new requires <new>.
  Cpp::Declare("#include <new>");
  Cpp::Declare("namespace WrapNS { int add(int a, int b) { return a + b; } }");
  auto* Func =
      static_cast<void*>(Cpp::GetNamed("add", Cpp::GetScope("WrapNS")));
  ASSERT_NE(Func, nullptr);

  TraceInfo::TheTraceInfo->clear();

  // MakeFunctionCallable triggers make_wrapper which logs the wrapper source.
  auto JC = Cpp::MakeFunctionCallable(Func);
  ASSERT_TRUE(JC.isValid());

  auto log = getFullLog();
  EXPECT_THAT(log, HasSubstr("=== Wrapper for"));
  EXPECT_THAT(log, HasSubstr("=== End wrapper ==="));
  // The wrapper source should contain the function name.
  EXPECT_THAT(log, HasSubstr("add"));
}

#ifndef NDEBUG
TEST_F(TracingTest, JitCallInvokeLogged) {
  Cpp::CreateInterpreter({});
  ASSERT_NE(TraceInfo::TheTraceInfo, nullptr);

  Cpp::Declare("#include <new>");
  Cpp::Declare("namespace InvNS { int square(int x) { return x * x; } }");
  auto* Func =
      static_cast<void*>(Cpp::GetNamed("square", Cpp::GetScope("InvNS")));
  ASSERT_NE(Func, nullptr);

  auto JC = Cpp::MakeFunctionCallable(Func);
  ASSERT_TRUE(JC.isValid());

  // Clear so only the Invoke shows up.
  TraceInfo::TheTraceInfo->clear();

  int arg = 7;
  int result = 0;
  void* args[] = {&arg};
  JC.Invoke(&result, {args, 1});

  auto log = getFullLog();
  // ReportInvokeStart should have logged the invoke.
  EXPECT_THAT(log, HasSubstr("JitCall::Invoke"));
  EXPECT_THAT(log, HasSubstr("square"));
  EXPECT_THAT(log, HasSubstr("nargs=1"));

  // And the function should have actually executed.
  EXPECT_EQ(result, 49);
}
#endif

// Verify that log entries include timing annotations.
TEST_F(TracingTest, LogEntriesIncludeTimingAnnotation) {
  VoidFunc();
  auto output = TraceInfo::TheTraceInfo->getLastLogEntry();
  EXPECT_THAT(output, HasSubstr("// ["));
  EXPECT_THAT(output, HasSubstr("ns]"));
}
