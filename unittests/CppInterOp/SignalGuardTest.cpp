// Tests for EnableSignalProtection, IsSignalProtectionEnabled and
// InvokeProtected. No interpreter is required. Native only: the mechanism is
// a stub on Windows and Emscripten.

#include "CppInterOp/CppInterOp.h"
#include "gtest/gtest.h"

#include <csignal>
#include <cstdlib>
#include <cstring>
#include <thread>

// Loaded at run time so the compiler must emit a real store. A store through
// a literal null pointer is undefined behavior that clang folds into a trap
// instruction, which raises SIGILL, not SIGSEGV.
static volatile int* volatile NullInt = nullptr;

static void writeThroughNull(void*) { *NullInt = 42; }
static void setFlag(void* P) { *static_cast<int*>(P) = 1; }
static void callAbort(void*) { std::abort(); }
static void raiseStored(void* P) { std::raise(*static_cast<int*>(P)); }
static void ignoreSignal(int) {}

// The inner call faults and the outer callback records the code and continues.
static void faultInsideThenContinue(void* P) {
  *static_cast<int*>(P) = Cpp::InvokeProtected(writeThroughNull, nullptr);
}

// The inner call completes, then the outer callback faults.
static void completeInsideThenFault(void* P) {
  Cpp::InvokeProtected(setFlag, P);
  *NullInt = 1;
}

// Address of the handler installed for Sig, whichever union member holds it.
static void* handlerOf(int Sig) {
  struct sigaction SA;
  memset(&SA, 0, sizeof(SA));
  if (sigaction(Sig, nullptr, &SA) != 0)
    return nullptr;
  void* Addr = nullptr;
  if (SA.sa_flags & SA_SIGINFO)
    memcpy(&Addr, &SA.sa_sigaction, sizeof(Addr));
  else
    memcpy(&Addr, &SA.sa_handler, sizeof(Addr));
  return Addr;
}

static bool isBlocked(int Sig) {
  sigset_t Set;
  sigemptyset(&Set);
  pthread_sigmask(SIG_BLOCK, nullptr, &Set);
  return sigismember(&Set, Sig) == 1;
}

static const int Covered[] = {SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGABRT};
static const int NumCovered = sizeof(Covered) / sizeof(Covered[0]);

namespace {
class SignalGuardTest : public ::testing::Test {
protected:
  void TearDown() override { Cpp::EnableSignalProtection(false); }
};
} // namespace

TEST_F(SignalGuardTest, DisabledLeavesDispositionsAlone) {
  Cpp::EnableSignalProtection(false);
  EXPECT_FALSE(Cpp::IsSignalProtectionEnabled());

  void* Before[NumCovered];
  for (int I = 0; I < NumCovered; ++I)
    Before[I] = handlerOf(Covered[I]);

  int Flag = 0;
  EXPECT_EQ(Cpp::InvokeProtected(setFlag, &Flag), 0);
  EXPECT_EQ(Flag, 1);

  for (int I = 0; I < NumCovered; ++I)
    EXPECT_EQ(handlerOf(Covered[I]), Before[I]) << "signal " << Covered[I];
}

TEST_F(SignalGuardTest, EnableInstallsAndDisableRestores) {
  Cpp::EnableSignalProtection(false);
  void* Before[NumCovered];
  for (int I = 0; I < NumCovered; ++I)
    Before[I] = handlerOf(Covered[I]);

  EXPECT_TRUE(Cpp::EnableSignalProtection(true));
  EXPECT_TRUE(Cpp::IsSignalProtectionEnabled());
  void* Guard = handlerOf(SIGSEGV);
  EXPECT_NE(Guard, Before[0]);
  for (int I = 1; I < NumCovered; ++I)
    EXPECT_EQ(handlerOf(Covered[I]), Guard) << "signal " << Covered[I];

  // Enabling twice is idempotent.
  EXPECT_TRUE(Cpp::EnableSignalProtection(true));
  EXPECT_EQ(handlerOf(SIGSEGV), Guard);

  EXPECT_FALSE(Cpp::EnableSignalProtection(false));
  EXPECT_FALSE(Cpp::IsSignalProtectionEnabled());
  for (int I = 0; I < NumCovered; ++I)
    EXPECT_EQ(handlerOf(Covered[I]), Before[I]) << "signal " << Covered[I];
}

TEST_F(SignalGuardTest, NullWriteReturnsSIGSEGVRepeatedly) {
  ASSERT_TRUE(Cpp::EnableSignalProtection(true));
  int Flag = 0;
  EXPECT_EQ(Cpp::InvokeProtected(setFlag, &Flag), 0);
  EXPECT_EQ(Flag, 1);
  EXPECT_EQ(Cpp::InvokeProtected(writeThroughNull, nullptr), SIGSEGV);
  EXPECT_EQ(Cpp::InvokeProtected(writeThroughNull, nullptr), SIGSEGV);
  // The guard is still armed and a clean call still completes.
  Flag = 0;
  EXPECT_EQ(Cpp::InvokeProtected(setFlag, &Flag), 0);
  EXPECT_EQ(Flag, 1);
}

TEST_F(SignalGuardTest, SignalMaskIsRestoredAfterRecovery) {
  ASSERT_TRUE(Cpp::EnableSignalProtection(true));
  ASSERT_FALSE(isBlocked(SIGSEGV));
  EXPECT_EQ(Cpp::InvokeProtected(writeThroughNull, nullptr), SIGSEGV);
  // The handler ran with SIGSEGV blocked. sigsetjmp(..., 1) must have
  // restored the caller's mask.
  EXPECT_FALSE(isBlocked(SIGSEGV));
}

TEST_F(SignalGuardTest, AbortReturnsSIGABRTRepeatedly) {
  ASSERT_TRUE(Cpp::EnableSignalProtection(true));
  // glibc's abort() releases its lock and resets its stage before raising,
  // so a second abort after a recovered one still reaches the handler.
  EXPECT_EQ(Cpp::InvokeProtected(callAbort, nullptr), SIGABRT);
  EXPECT_EQ(Cpp::InvokeProtected(callAbort, nullptr), SIGABRT);
}

TEST_F(SignalGuardTest, RaisedSignalsReportTheirNumber) {
  ASSERT_TRUE(Cpp::EnableSignalProtection(true));
  for (int Sig : Covered) {
    int Which = Sig;
    EXPECT_EQ(Cpp::InvokeProtected(raiseStored, &Which), Sig)
        << "signal " << Sig;
  }
}

TEST_F(SignalGuardTest, NestedFramesAreIndependent) {
  ASSERT_TRUE(Cpp::EnableSignalProtection(true));

  int Inner = -1;
  EXPECT_EQ(Cpp::InvokeProtected(faultInsideThenContinue, &Inner), 0);
  EXPECT_EQ(Inner, SIGSEGV);

  int Flag = 0;
  EXPECT_EQ(Cpp::InvokeProtected(completeInsideThenFault, &Flag), SIGSEGV);
  EXPECT_EQ(Flag, 1);

  // Both frames were popped: a bare fault would now be unprotected, so a
  // further protected call must still catch its own fault.
  EXPECT_EQ(Cpp::InvokeProtected(writeThroughNull, nullptr), SIGSEGV);
}

TEST_F(SignalGuardTest, ProtectedCallOnAnotherThread) {
  ASSERT_TRUE(Cpp::EnableSignalProtection(true));
  int Result = -1;
  int Flag = 0;
  std::thread T([&] {
    Result = Cpp::InvokeProtected(writeThroughNull, nullptr);
    Cpp::InvokeProtected(setFlag, &Flag);
  });
  T.join();
  EXPECT_EQ(Result, SIGSEGV);
  EXPECT_EQ(Flag, 1);
}

TEST_F(SignalGuardTest, UnprotectedRaiseRestoresDisposition) {
  // A covered signal outside a protected frame hands the signal back to the
  // displaced disposition. A raised signal is consumed by that first
  // delivery, so the displaced handler is installed again but does not run.
  struct sigaction Original;
  ASSERT_EQ(sigaction(SIGFPE, nullptr, &Original), 0);
  struct sigaction Ignore;
  memset(&Ignore, 0, sizeof(Ignore));
  Ignore.sa_handler = &ignoreSignal;
  sigemptyset(&Ignore.sa_mask);
  ASSERT_EQ(sigaction(SIGFPE, &Ignore, nullptr), 0);

  ASSERT_TRUE(Cpp::EnableSignalProtection(true));
  ASSERT_NE(handlerOf(SIGFPE), reinterpret_cast<void*>(&ignoreSignal));
  std::raise(SIGFPE);
  EXPECT_EQ(handlerOf(SIGFPE), reinterpret_cast<void*>(&ignoreSignal));
  // The other signals are still guarded.
  EXPECT_EQ(handlerOf(SIGSEGV), handlerOf(SIGILL));
  EXPECT_NE(handlerOf(SIGSEGV), reinterpret_cast<void*>(&ignoreSignal));

  Cpp::EnableSignalProtection(false);
  ASSERT_EQ(sigaction(SIGFPE, &Original, nullptr), 0);
}

#if GTEST_HAS_DEATH_TEST
TEST_F(SignalGuardTest, UnprotectedFaultStillTerminates) {
  // Outside a protected frame the guard reinstates the displaced disposition
  // and returns, the fault re-executes, and the process dies. Run in a
  // forked child.
  EXPECT_EXIT(
      {
        Cpp::EnableSignalProtection(true);
        *NullInt = 1;
        std::_Exit(0);
      },
      ::testing::KilledBySignal(SIGSEGV), "");
}
#endif // GTEST_HAS_DEATH_TEST
