//===--- InputRollback.h - Undo state an input leaves outside its decls ---===//
//
// Part of the compiler-research project, under the Apache License v2.0 with
// LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LIB_CPPINTEROP_INPUTROLLBACK_H
#define LIB_CPPINTEROP_INPUTROLLBACK_H

#include "CppInterOp/CppInterOpTypes.h"

#include <cstddef>
#include <memory>

namespace clang {
class CompilerInstance;
class Interpreter;
class Value;
} // namespace clang

namespace CppInternal {

/// The number of partial translation units \p I holds.
CPPINTEROP_API size_t InputCountOf(clang::Interpreter& I);

/// Moves the value of the last input into \p V, as
/// clang::Interpreter::ParseAndExecute does.
CPPINTEROP_API void TakeLastValue(clang::Interpreter& I, clang::Value& V);

/// Undoes what clang-repl keeps of a withdrawn input: the instantiations that
/// a failed input requested, macros, and include state.
class InputRollback {
public:
  struct State;

  /// Hooks into the consumer and preprocessor of \p CI. Returns null when
  /// this clang version lays out the consumer differently.
  static std::unique_ptr<InputRollback> install(clang::CompilerInstance& CI);

  /// Call before parsing an input; \p Index is the input count then.
  CPPINTEROP_API void beginInput(size_t Index);
  /// Call when the input started by beginInput fails to parse.
  CPPINTEROP_API void failedParse();
  /// Call after the inputs from \p Keep on are undone.
  void withdrawFrom(size_t Keep);

  explicit InputRollback(std::shared_ptr<State> S);
  InputRollback(const InputRollback&) = delete;
  InputRollback& operator=(const InputRollback&) = delete;
  InputRollback(InputRollback&&) = delete;
  InputRollback& operator=(InputRollback&&) = delete;
  ~InputRollback();

private:
  // Shared with the hooks, which clang owns and may outlive this object.
  std::shared_ptr<State> S;
};

} // namespace CppInternal

#endif // LIB_CPPINTEROP_INPUTROLLBACK_H
