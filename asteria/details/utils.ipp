// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_UTILS_
#  error Please include <asteria/utils.hpp> instead.
#endif
namespace asteria {
namespace details_utils {

extern const uint8_t cmask_table[];
extern const char ctrl_char_names[][8];
extern const char char_escapes[][8];

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

}  // namespace details_utils
}  // namespace asteria
