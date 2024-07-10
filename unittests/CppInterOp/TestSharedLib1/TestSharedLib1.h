#ifndef UNITTESTS_CPPINTEROP_TESTSHAREDLIB_TESTSHAREDLIB1_H
#define UNITTESTS_CPPINTEROP_TESTSHAREDLIB_TESTSHAREDLIB1_H

// Avoid having to mangle/demangle the symbol name in tests
#ifdef _WIN32
extern "C" __declspec(dllexport) int ret_one();
extern "C" __declspec(dllexport) int ret_two();
#else
extern "C" int ret_one();
extern "C" int ret_two();
#endif

#endif // UNITTESTS_CPPINTEROP_TESTSHAREDLIB_TESTSHAREDLIB1_H
