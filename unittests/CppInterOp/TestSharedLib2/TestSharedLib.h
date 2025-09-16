#ifndef UNITTESTS_CPPINTEROP_TESTSHAREDLIB_TESTSHAREDLIB2_H
#define UNITTESTS_CPPINTEROP_TESTSHAREDLIB_TESTSHAREDLIB2_H

// Avoid having to mangle/demangle the symbol name in tests
#ifdef _WIN32
extern "C" __declspec(dllexport) int ret_one();
#else
extern "C" int __attribute__((visibility("default"))) ret_one();
#endif

#endif // UNITTESTS_CPPINTEROP_TESTSHAREDLIB_TESTSHAREDLIB2_H
