#include <CppInterOp/Dispatch.h>

#include <string>
#include <unordered_map>

static const std::unordered_map<std::string_view, CppFnPtrTy>
    DispatchMap = {
#define DISPATCH_API(name, type) {#name, (CppFnPtrTy) static_cast<type>(&CppImpl::name)},
        CPPINTEROP_API_TABLE
#undef DISPATCH_API
};

static inline CppFnPtrTy _cppinterop_get_proc_address(const char* funcName) {
  auto it = DispatchMap.find(funcName);
  return (it != DispatchMap.end()) ? it->second : nullptr;
}

void (*CppGetProcAddress(const unsigned char* procName))(void) {
  return _cppinterop_get_proc_address(reinterpret_cast<const char*>(procName));
}
