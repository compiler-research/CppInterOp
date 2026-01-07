//------------------------------------------------------------------------------
// CppInterOp Dispatch Implementation
// author:  Aaron Jomy <aaron.jomy@cern.ch>
//------------------------------------------------------------------------------

#include <CppInterOp/Dispatch.h>

#include <unordered_map>

static const std::unordered_map<std::string_view, __CPP_FUNC>
    INTEROP_FUNCTIONS = {
#define X(name, type) {#name, (__CPP_FUNC) static_cast<type>(&CppImpl::name)},
        CPPINTEROP_API_MAP
#undef X
};

#undef MAP_ENTRY_SIMPLE
#undef MAP_ENTRY_OVERLOADED

static inline __CPP_FUNC _cppinterop_get_proc_address(const char* funcName) {
  auto it = INTEROP_FUNCTIONS.find(funcName);
  return (it != INTEROP_FUNCTIONS.end()) ? it->second : nullptr;
}

void (*CppGetProcAddress(const unsigned char* procName))(void) {
  return _cppinterop_get_proc_address(reinterpret_cast<const char*>(procName));
}
