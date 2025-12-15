//------------------------------------------------------------------------------
// CppInterOp Dispatch Implementation
// author:  Aaron Jomy <aaron.jomy@cern.ch>
//------------------------------------------------------------------------------

#include <CppInterOp/CppInterOp.h>
#include <CppInterOp/CppInterOpDispatch.h>

#include <unordered_map>

// Macro for simple functions (direct cast)
#define MAP_ENTRY_SIMPLE(func_name) {#func_name, (__CPP_FUNC)Cpp::func_name},

// Macro for overloaded functions (needs static_cast with signature)
#define MAP_ENTRY_OVERLOADED(func_name, signature)                             \
  {#func_name, (__CPP_FUNC) static_cast<signature>(&Cpp::func_name)},

static const std::unordered_map<std::string_view, __CPP_FUNC>
    INTEROP_FUNCTIONS = {
        FOR_EACH_CPP_FUNCTION_SIMPLE(MAP_ENTRY_SIMPLE)
            FOR_EACH_CPP_FUNCTION_OVERLOADED(MAP_ENTRY_OVERLOADED)};

#undef MAP_ENTRY_SIMPLE
#undef MAP_ENTRY_OVERLOADED

static inline __CPP_FUNC _cppinterop_get_proc_address(const char* funcName) {
    auto it = INTEROP_FUNCTIONS.find(funcName);
    return (it != INTEROP_FUNCTIONS.end()) ? it->second : nullptr;
}

void (*CppGetProcAddress(const unsigned char* procName))(void) {
  return _cppinterop_get_proc_address(reinterpret_cast<const char*>(procName));
}
