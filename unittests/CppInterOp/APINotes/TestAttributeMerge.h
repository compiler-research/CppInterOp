#ifndef UNITTESTS_CPPINTEROP_TESTATTRIBUTEMERGE_H
#define UNITTESTS_CPPINTEROP_TESTATTRIBUTEMERGE_H

[[clang::annotate("cppMalloc")]] void* mergeFunc();

#pragma clang attribute push([[clang::annotate("cppNew")]], apply_to = function)
int* overloadFunc();
int* overloadFunc(int n);
#pragma clang attribute pop

[[clang::annotate("cppMalloc")]] void* func70_helper();

#endif
