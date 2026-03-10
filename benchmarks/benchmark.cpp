#include <emscripten.h>
#include <cstdio>
#include <vector>
#include <string>
#include <functional>
#include "CppInterOp/CppInterOp.h"

constexpr int iterations = 1000;

struct Benchmark {
   std::string name;
   std::function<void()> func;
};

void run_benchmarks(const std::vector<Benchmark>& benchmarks) {
   for (const auto& bench : benchmarks) {
       bench.func(); // warmup

       double start = emscripten_get_now();
       for (int i = 0; i < iterations; ++i) {
           bench.func();
       }
       double end = emscripten_get_now();
       double elapsed_ms = end - start;
       double avg_time = elapsed_ms / iterations;

       printf("Benchmark: %-20s | Avg time: %.6f ms (over %d runs)\n",
              bench.name.c_str(), avg_time, iterations);
   }
}

void cpp_getversion() {
  Cpp::GetVersion();
}

void cpp_process() {
  Cpp::CreateInterpreter();
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
