#include "CppInterOp/CppInterOp.h"
#include <chrono>
#include <cstdio>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include <functional>
#include <string>
#include <vector>

constexpr int iterations = 1000;

struct Benchmark {
   std::string name;
   std::function<void()> func;
};

static double now_ms() {
#ifdef __EMSCRIPTEN__
    return emscripten_get_now();
#else
    using namespace std::chrono;
    return duration<double, std::milli>(
        high_resolution_clock::now().time_since_epoch()
    ).count();
#endif
}

static void run_benchmarks(const std::vector<Benchmark>& benchmarks) {
   for (const auto& bench : benchmarks) {
       bench.func();

       double start = now_ms();
       for (int i = 0; i < iterations; ++i) {
           bench.func();
       }
       double end = now_ms();
       double elapsed_ms = end - start;
       double avg_time = elapsed_ms / iterations;

       printf("Benchmark: %-20s | Avg time: %.6f ms (over %d runs)\n",
              bench.name.c_str(), avg_time, iterations);
   }
}

static void cpp_getversion() {
  Cpp::GetVersion();
}

static void cpp_process() {
  static bool initialized = false;
  if (!initialized) {
    Cpp::CreateInterpreter();
    initialized = true;
  }
  Cpp::Process("int a = 12;");
}


int main() {
   std::vector<Benchmark> benchmarks = {
       {"getversion", cpp_getversion},
       {"process", cpp_process},
   };

   run_benchmarks(benchmarks);
   return 0;
}
