// Implementation of the dispatch symbol table. Maps function names to
// Cpp:: addresses for dlopen consumers via CppGetProcAddress.
// This is a library internal — it includes CppInterOp.h (not Dispatch.h)
// because it needs the Cpp:: function declarations.

#include "CppInterOp/CppInterOp.h"

#include <CppInterOp/Error.h>
#include <iostream>
#include <string_view>
#include <tuple>

using namespace Cpp;
using CppFnPtrTy = void (*)();

// NOLINTBEGIN(cppcoreguidelines-pro-type-cstyle-cast)
// Suppress deprecation: the dispatch table intentionally exposes all
// overloads, including those marked [[deprecated]] in the public API.
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#elif defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4996)
#endif
static constexpr auto DispatchMap = std::tuple{
#define CPPINTEROP_API_FUNC(DN, CN, Ret, DeclArgs, CallArgs, RawTypes)         \
  std::pair<std::string_view, Ret(*) RawTypes>{                                \
      #DN, static_cast<Ret(*) RawTypes>(&Cpp::CN)},
#include "CppInterOp/CppInterOpAPI.inc"
};
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif
// NOLINTEND(cppcoreguidelines-pro-type-cstyle-cast)

template <std::size_t I = 0>
static CppFnPtrTy find_function(std::string_view name) {
  if constexpr (I < std::tuple_size_v<decltype(DispatchMap)>) {
    const auto& entry = std::get<I>(DispatchMap);
    if (entry.first == name)
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      return reinterpret_cast<CppFnPtrTy>(entry.second);
    return find_function<I + 1>(name);
  }
  return nullptr;
}

extern "C" CPPINTEROP_API CppFnPtrTy CppGetProcAddress(const char* funcName) {
  auto fn = find_function(funcName);
  if (!fn)
    std::cerr << "[CppInterOp Dispatch] Failed to find API: " << funcName
              << " May need to be ported to CppInterOp.td\n";
  return fn;
}
