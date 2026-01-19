#ifndef CPPINTEROP_RUNTIME_H
#define CPPINTEROP_RUNTIME_H

// This header contains runtime utilities previously injected into the
// interpreter using I->declare(). It is now provided as a real header so that
// the interpreter can load it on startup.

namespace __internal_CppInterOp {

template <typename Signature>
struct function;
template <typename Res, typename... ArgTypes>
struct function<Res(ArgTypes...)> {
    using result_type = Res;
};

} // namespace __internal_CppInterOp

#endif // CPPINTEROP_RUNTIME_H
