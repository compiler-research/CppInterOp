#ifndef UNITTESTS_CPPINTEROP_TESTDOWNSTREAMLIB_TESTDOWNSTREAMLIB_H
#define UNITTESTS_CPPINTEROP_TESTDOWNSTREAMLIB_TESTDOWNSTREAMLIB_H

// Probe entry that ODR-uses the inline JitCall fast path. JC is opaque
// so the optimizer can't DCE the inlined references at any -O level.
namespace CppImpl {
class JitCall;
}
#ifdef _WIN32
extern "C" __declspec(dllexport) void downstream_link_probe(
    CppImpl::JitCall* JC);
#else
extern "C" __attribute__((visibility("default"))) void
downstream_link_probe(CppImpl::JitCall* JC);
#endif

#endif // UNITTESTS_CPPINTEROP_TESTDOWNSTREAMLIB_TESTDOWNSTREAMLIB_H
