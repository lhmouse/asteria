// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_UTILITIES_HPP_
#  error Please include <asteria/utilities.hpp> instead.
#endif

namespace asteria {
namespace details_utilities {

extern const uint8_t cctype_table[128];

struct Quote_Wrapper
  {
    const char* str;
    size_t len;
  };

tinyfmt&
operator<<(tinyfmt& fmt, const Quote_Wrapper& q);

struct Paragraph_Wrapper
  {
    size_t indent;
    size_t hanging;
  };

tinyfmt&
operator<<(tinyfmt& fmt, const Paragraph_Wrapper& q);

struct Formatted_errno
  {
    int err;
  };

tinyfmt&
operator<<(tinyfmt& fmt, const Formatted_errno& e);

}  // namespace details_utilities
}  // namespace asteria
