// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_UTILS_
#  error Please include <asteria/utils.hpp> instead.
#endif
namespace asteria {
namespace details_utils {


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

}  // namespace details_utils
}  // namespace asteria
