#include <CppInterOp/Dispatch.h>

#include <iostream>
#include <string_view>
#include <unordered_map>

using namespace CppImpl;

// NOLINTBEGIN(cppcoreguidelines-pro-type-cstyle-cast)
static const std::unordered_map<std::string_view, CppFnPtrTy> DispatchMap = {
#define CPPINTEROP_API_FUNC(DN, CN, Ret, DeclArgs, CallArgs, RawTypes)         \
  {#DN, (CppFnPtrTy) static_cast<Ret(*) RawTypes>(&CppImpl::CN)},
#include "CppInterOp/CppInterOpAPI.inc"
};
// NOLINTEND(cppcoreguidelines-pro-type-cstyle-cast)

CppFnPtrTy CppGetProcAddress(const char* funcName) {
  auto it = DispatchMap.find(funcName);
  if (it == DispatchMap.end()) {
    std::cerr
        << "[CppInterOp Dispatch] Failed to find API: " << funcName
        << " May need to be ported to the symbol-address table in Dispatch.h\n";
    return nullptr;
  }
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  return reinterpret_cast<CppFnPtrTy>(it->second);
}
