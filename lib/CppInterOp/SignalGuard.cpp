//===--- SignalGuard.cpp - Signal-protected calls ---------------*- C++ -*-===//
//
// Part of the compiler-research project, under the Apache License v2.0 with
// LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements EnableSignalProtection, IsSignalProtectionEnabled and
// InvokeProtected. InvokeProtected runs a callback under a sigsetjmp frame. A
// signal handler turns a fatal signal raised inside the callback into the
// return value, so an interactive host can report the fault instead of
// terminating with the process.
//
// The guard owns the dispositions of the handled signals while enabled and
// saves the ones it displaces. llvm::sys::AddSignalHandler is not used
// because LLVM's handler restores the pre-LLVM dispositions and consumes its
// callbacks on its first run, which does not suit a guard that must fire
// repeatedly. When a signal arrives on a thread with no protected frame, the
// handler reinstates the displaced disposition and returns. The faulting
// instruction then re-executes and the signal reaches that disposition with
// its original information.
//
//===----------------------------------------------------------------------===//

#include "CppInterOp/CppInterOp.h"

#include "Tracing.h"

#include <atomic>
#include <cassert>
#include <mutex>

#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
#define CPPINTEROP_HAVE_SIGNAL_GUARD 1
#include <setjmp.h>
#include <signal.h>
#include <string.h>
#endif

namespace Cpp {

#ifdef CPPINTEROP_HAVE_SIGNAL_GUARD

/// The signals the guard converts. SIGINT and SIGTERM are requests to the
/// host, not faults in the callee, and stay with the host.
static constexpr int CoveredSignals[] = {SIGSEGV, SIGBUS, SIGILL, SIGFPE,
                                         SIGABRT};
static constexpr size_t NumCovered =
    sizeof(CoveredSignals) / sizeof(CoveredSignals[0]);

/// One InvokeProtected activation. Frames form a per-thread stack through
/// Prev, so protected calls nest.
struct ProtectedFrame {
  sigjmp_buf Buf;
  ProtectedFrame* Prev;
};

/// Innermost protected frame of the calling thread.
static thread_local ProtectedFrame* CurrentFrame = nullptr;

/// Protected frames active on any thread. The handler reads thread-local
/// storage only when this is nonzero. A first access to the dynamic TLS of a
/// dlopen'ed library can allocate, which a signal handler must not do.
static std::atomic<int> ActiveFrames{0};

/// Whether EnableSignalProtection(true) is in effect.
static std::atomic<bool> Enabled{false};

/// Serializes install and uninstall. The handler never takes it.
static std::mutex InstallMutex;

/// The disposition each covered signal had when the guard displaced it.
/// Valid while the guard's handler is installed for that signal.
static struct sigaction SavedActions[NumCovered];

static int indexOf(int Sig) {
  for (size_t I = 0; I < NumCovered; ++I)
    if (CoveredSignals[I] == Sig)
      return static_cast<int>(I);
  return -1;
}

static void handleSignal(int Sig, siginfo_t* Info, void* Ctx);

static bool isGuardHandler(const struct sigaction& SA) {
  return (SA.sa_flags & SA_SIGINFO) && SA.sa_sigaction == &handleSignal;
}

/// Jumps into the innermost protected frame of the faulting thread. With no
/// frame active, reinstates the displaced disposition and returns, so the
/// faulting instruction re-executes and the signal reaches that disposition.
static void handleSignal(int Sig, siginfo_t* /*Info*/, void* /*Ctx*/) {
  if (ActiveFrames.load(std::memory_order_acquire) > 0)
    if (ProtectedFrame* Frame = CurrentFrame)
      siglongjmp(Frame->Buf, Sig);
  int Idx = indexOf(Sig);
  if (Idx >= 0)
    sigaction(Sig, &SavedActions[Idx], nullptr);
}

/// Installs the guard for every covered signal that is not guarded already
/// and saves the disposition it displaces. Returns false on the first
/// failure and leaves the rollback to the caller, which holds InstallMutex.
static bool installHandlers() {
  struct sigaction New;
  memset(&New, 0, sizeof(New));
  New.sa_sigaction = &handleSignal;
  // Without SA_NODEFER the signal stays blocked while the handler runs, and
  // sigsetjmp(Buf, 1) restores the caller's mask on the jump back. SA_ONSTACK
  // uses the thread's alternate stack where one exists.
  New.sa_flags = SA_SIGINFO | SA_ONSTACK;
  sigemptyset(&New.sa_mask);
  for (size_t I = 0; I < NumCovered; ++I) {
    struct sigaction Cur;
    if (sigaction(CoveredSignals[I], nullptr, &Cur) != 0)
      return false;
    if (isGuardHandler(Cur))
      continue;
    if (sigaction(CoveredSignals[I], &New, &SavedActions[I]) != 0)
      return false;
  }
  return true;
}

/// Restores the saved disposition of every covered signal the guard still
/// handles. A signal a third party took over in the meantime is left alone.
/// The caller holds InstallMutex.
static void uninstallHandlers() {
  for (size_t I = 0; I < NumCovered; ++I) {
    struct sigaction Cur;
    if (sigaction(CoveredSignals[I], nullptr, &Cur) == 0 && isGuardHandler(Cur))
      sigaction(CoveredSignals[I], &SavedActions[I], nullptr);
  }
}

/// Publishes a frame as the thread's innermost one for its lifetime. Lives in
/// InvokeProtected's frame, which is intact after the siglongjmp lands there,
/// so the destructor runs on every exit path.
struct FrameScope {
  ProtectedFrame* Prev;
  explicit FrameScope(ProtectedFrame* Frame) : Prev(CurrentFrame) {
    Frame->Prev = Prev;
    CurrentFrame = Frame;
    ActiveFrames.fetch_add(1, std::memory_order_acq_rel);
  }
  ~FrameScope() {
    CurrentFrame = Prev;
    ActiveFrames.fetch_sub(1, std::memory_order_acq_rel);
  }
  FrameScope(const FrameScope&) = delete;
  FrameScope& operator=(const FrameScope&) = delete;
};

#endif // CPPINTEROP_HAVE_SIGNAL_GUARD

bool EnableSignalProtection(bool enable /*=true*/) {
  INTEROP_TRACE(enable);
#ifdef CPPINTEROP_HAVE_SIGNAL_GUARD
  std::lock_guard<std::mutex> Lock(InstallMutex);
  if (enable && installHandlers()) {
    Enabled.store(true, std::memory_order_release);
    return INTEROP_RETURN(true);
  }
  Enabled.store(false, std::memory_order_release);
  uninstallHandlers();
  return INTEROP_RETURN(false);
#else
  (void)enable;
  return INTEROP_RETURN(false);
#endif
}

bool IsSignalProtectionEnabled() {
  INTEROP_TRACE();
#ifdef CPPINTEROP_HAVE_SIGNAL_GUARD
  return INTEROP_RETURN(Enabled.load(std::memory_order_acquire));
#else
  return INTEROP_RETURN(false);
#endif
}

int InvokeProtected(ProtectedCallFn fn, void* arg /*=nullptr*/) {
  assert(fn && "InvokeProtected needs a callback");
  if (!fn)
    return 0;
#ifdef CPPINTEROP_HAVE_SIGNAL_GUARD
  if (!Enabled.load(std::memory_order_acquire)) {
    fn(arg);
    return 0;
  }
  ProtectedFrame Frame;
  FrameScope Scope(&Frame);
  // Save the signal mask too. The faulting signal is blocked while the
  // handler runs and the jump back must unblock it.
  int Sig = sigsetjmp(Frame.Buf, 1);
  if (Sig == 0)
    fn(arg);
  return Sig;
#else
  fn(arg);
  return 0;
#endif
}

} // namespace Cpp
