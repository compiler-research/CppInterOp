#ifndef UNITTESTS_CPPINTEROP_TESTSHAREDLIB_TESTSHAREDLIB3_H
#define UNITTESTS_CPPINTEROP_TESTSHAREDLIB_TESTSHAREDLIB3_H

// Avoid having to mangle/demangle the symbol name in tests
#ifdef _WIN32
extern "C" __declspec(dllexport) int ret_val();
#else
extern "C" int ret_val();
#endif

#endif // UNITTESTS_CPPINTEROP_TESTSHAREDLIB_TESTSHAREDLIB3_H
