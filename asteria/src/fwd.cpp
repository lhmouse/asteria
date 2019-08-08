// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "fwd.hpp"
#include "runtime/value.hpp"

namespace Asteria {

const char* describe_frame_type(Frame_Type type) noexcept
  {
    switch(type) {
    case frame_type_native:
      {
        return "native";
      }
    case frame_type_throw:
      {
        return "throw";
      }
    case frame_type_catch:
      {
        return "catch";
      }
    case frame_type_func:
      {
        return "func";
      }
    default:
      return "<unknown frame type>";
    }
  }

}  // namespace Asteria
