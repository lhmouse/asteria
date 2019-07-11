// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "backtrace_frame.hpp"
#include "../utilities.hpp"

namespace Asteria {

const char* Backtrace_Frame::stringify_ftype(Backtrace_Frame::Ftype xftype) noexcept
  {
    switch(xftype) {
    case ftype_native:
      {
        return "native";
      }
    case ftype_throw:
      {
        return "throw";
      }
    case ftype_catch:
      {
        return "catch";
      }
    case ftype_func:
      {
        return "func";
      }
    default:
      return "<unknown frame type>";
    }
  }

Backtrace_Frame::~Backtrace_Frame()
  {
  }

}  // namespace Asteria
