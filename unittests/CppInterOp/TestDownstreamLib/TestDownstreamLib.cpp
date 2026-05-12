#include "TestDownstreamLib.h"

// Dispatch.h is the cppyy-backend consumer entry point; including it
// selects per-DSO inline DispatchRaw slots.
#include "CppInterOp/Dispatch.h"

// ODR-uses the inline JitCall fast path. JC is an opaque param so the
// optimizer can't DCE the calls at any -O level. The body never runs;
// only the .o's UND-symbol surface matters.
void downstream_link_probe(CppImpl::JitCall* JC) {
  JC->Invoke();
  JC->InvokeConstructor(nullptr);
  JC->InvokeDestructor(nullptr);
}
