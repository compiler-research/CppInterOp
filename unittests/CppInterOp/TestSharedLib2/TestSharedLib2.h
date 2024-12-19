#ifndef UNITTESTS_CPPINTEROP_TESTSHAREDLIB_TESTSHAREDLIB2_H
#define UNITTESTS_CPPINTEROP_TESTSHAREDLIB_TESTSHAREDLIB2_H

// Avoid having to mangle/demangle the symbol name in tests
#ifdef _WIN32
extern "C" __declspec(dllexport) int ret_1();
extern "C" __declspec(dllexport) int ret_2();
#else
extern "C" int ret_1();
extern "C" int ret_2();
#endif

#endif // UNITTESTS_CPPINTEROP_TESTSHAREDLIB_TESTSHAREDLIB2_H
