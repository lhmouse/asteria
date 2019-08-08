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

const char* describe_gtype(Gtype gtype) noexcept
  {
    switch(gtype) {
    case gtype_null:
      {
        return "null";
      }
    case gtype_boolean:
      {
        return "boolean";
      }
    case gtype_integer:
      {
        return "integer";
      }
    case gtype_real:
      {
        return "real";
      }
    case gtype_string:
      {
        return "string";
      }
    case gtype_opaque:
      {
        return "opaque";
      }
    case gtype_function:
      {
        return "function";
      }
    case gtype_array:
      {
        return "array";
      }
    case gtype_object:
      {
        return "object";
      }
    default:
      return "<unknown data type>";
    }
  }

// We assume that a all-bit-zero struct represents the `null` value.
// This is effectively undefined behavior. Don't play with this at home!
alignas(Value) const unsigned char null_value_storage[sizeof(Value)] = { };

}  // namespace Asteria
