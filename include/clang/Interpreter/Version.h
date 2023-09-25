//--------------------------------------------------------------------*- C++ -*-
// CppInterOp version
// author:  Vassil Vassilev <vvasilev-at-cern.ch>
//------------------------------------------------------------------------------

#ifndef CPPINTEROP_VERSION
#define CPPINTEROP_VERSION

#include <string>

#include "VCSVersion.inc"

namespace InterOp {
  inline std::string getInterOpRevision() {
#ifdef CPPINTEROP_REVISION
    return CPPINTEROP_REVISION;
#else
    return "";
#endif
  } ;
} // end namespace InterOp

#endif //CPPINTEROP_VERSION
