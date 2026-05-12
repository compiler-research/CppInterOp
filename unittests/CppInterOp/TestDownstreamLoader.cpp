// dlopens TestDownstreamLib without linking libclangCppInterOp -- the
// same loader posture cppyy_backend.loader uses. Exit status is what
// ctest checks; any unsatisfied UND on the probe trips here.

#include <cstdio>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#ifndef TEST_DOWNSTREAM_LIB_PATH
#error "TEST_DOWNSTREAM_LIB_PATH must be defined"
#endif

int main() {
#ifdef _WIN32
  HMODULE h = LoadLibraryA(TEST_DOWNSTREAM_LIB_PATH);
  if (!h) {
    std::fprintf(stderr, "LoadLibrary(%s) failed: %lu\n",
                 TEST_DOWNSTREAM_LIB_PATH,
                 static_cast<unsigned long>(GetLastError()));
    return 1;
  }
  FreeLibrary(h);
#else
  void* h = dlopen(TEST_DOWNSTREAM_LIB_PATH, RTLD_NOW | RTLD_LOCAL);
  if (!h) {
    std::fprintf(stderr, "dlopen(%s) failed: %s\n", TEST_DOWNSTREAM_LIB_PATH,
                 dlerror());
    return 1;
  }
  dlclose(h);
#endif
  return 0;
}
