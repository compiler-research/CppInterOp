#include <CppInterOp/Dispatch.h>

#include <string_view>
#include <unordered_map>

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
static const std::unordered_map<std::string_view, CppFnPtrTy>
    DispatchMap = {
#define DISPATCH_API(name, type) {#name, (CppFnPtrTy) static_cast<type>(&CppImpl::name)},
        CPPINTEROP_API_TABLE
#undef DISPATCH_API
};

CppFnPtrTy CppGetProcAddress(const char* funcName) {
  auto it = DispatchMap.find(funcName);
  if (it == DispatchMap.end())
    return nullptr;
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  return reinterpret_cast<CppFnPtrTy>(it->second);
}