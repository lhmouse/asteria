// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_UTIL_HPP_
#define ASTERIA_UTIL_HPP_

#include "fwd.hpp"
#include "../rocket/tinyfmt_str.hpp"
#include "../rocket/format.hpp"
#include "details/util.ipp"

namespace asteria {

// Error handling
ptrdiff_t
write_log_to_stderr(const char* file, long line, cow_string&& msg)
  noexcept;

template<typename... ParamsT>
ROCKET_NOINLINE ROCKET_FLATTEN_FUNCTION
cow_string
format_string(const char* templ, const ParamsT&... params)
  {
    ::rocket::tinyfmt_str fmt;
    format(fmt, templ, params...);  // ADL intended
    return fmt.extract_string();
  }

// Note the format string must be a string literal.
#define ASTERIA_TERMINATE(...)  \
    (::asteria::write_log_to_stderr(__FILE__, __LINE__,  \
       __func__ + ::asteria::format_string(": " __VA_ARGS__)  \
                + "\nThis is likely a bug. Please report."),  \
     ::std::terminate())

// Note the format string must be a string literal.
#define ASTERIA_THROW(...)  \
    (::rocket::sprintf_and_throw<::std::runtime_error>(  \
       "%s: %s\n[thrown from native code at '%s:%d']", __func__,  \
         ::asteria::format_string("" __VA_ARGS__).c_str(), __FILE__, (int)__LINE__))

// UTF-8 conversion functions
bool
utf8_encode(char*& pos, char32_t cp)
  noexcept;

bool
utf8_encode(cow_string& text, char32_t cp);

bool
utf8_decode(char32_t& cp, const char*& pos, size_t avail)
  noexcept;

bool
utf8_decode(char32_t& cp, const cow_string& text, size_t& offset);

// UTF-16 conversion functions
bool
utf16_encode(char16_t*& pos, char32_t cp)
  noexcept;

bool
utf16_encode(cow_u16string& text, char32_t cp);

bool
utf16_decode(char32_t& cp, const char16_t*& pos, size_t avail)
  noexcept;

bool
utf16_decode(char32_t& cp, const cow_u16string& text, size_t& offset);

// Type conversion
template<typename enumT>
ROCKET_CONST_FUNCTION constexpr
typename ::std::underlying_type<enumT>::type
weaken_enum(enumT value)
  noexcept
  { return static_cast<typename ::std::underlying_type<enumT>::type>(value);  }

// Bytewise comparison
inline
bool
mem_equal(const void* s, const void* r, size_t n)
noexcept
  { return ::std::memcmp(s, r, n) == 0;  }

// C character types
enum : uint8_t
  {
    cctype_space   = 0x01,  // [ \t\v\f\r\n]
    cctype_alpha   = 0x02,  // [A-Za-z]
    cctype_digit   = 0x04,  // [0-9]
    cctype_xdigit  = 0x08,  // [0-9A-Fa-f]
    cctype_namei   = 0x10,  // [A-Za-z_]
    cctype_blank   = 0x20,  // [ \t]
  };

ROCKET_CONST_FUNCTION inline
uint8_t
get_cctype(char c)
  noexcept
  {
    size_t m = static_cast<uint8_t>(c);
    if(m >= 128)
      return 0;
    return details_util::cctype_table[m];
  }

ROCKET_CONST_FUNCTION inline
bool
is_cctype(char c, uint8_t mask)
  noexcept
  { return noadl::get_cctype(c) & mask;  }

// Numeric conversion
ROCKET_CONST_FUNCTION inline
bool
is_convertible_to_integer(double val)
  noexcept
  {
    return ::std::islessequal(-0x1p63, val) && ::std::isless(val, 0x1p63);
  }

ROCKET_CONST_FUNCTION inline
bool
is_safe_integer(double val)
  noexcept
  {
    double absv = ::std::abs(val);
    return ::std::islessequal(1 - 0x1p53, absv) &&
           ::std::islessequal(absv, 0x1p53 - 1) &&
           static_cast<double>(static_cast<int64_t>(val)) == val;
  }

// C-style quoting
constexpr
details_util::Quote_Wrapper
quote(const char* str, size_t len)
  noexcept
  { return { str, len };  }

inline
details_util::Quote_Wrapper
quote(const char* str)
  noexcept
  { return noadl::quote(str, ::std::strlen(str));  }

inline
details_util::Quote_Wrapper
quote(const cow_string& str)
  noexcept
  { return noadl::quote(str.data(), str.size());  }

// Justifying
constexpr
details_util::Paragraph_Wrapper
pwrap(size_t indent, size_t hanging)
  noexcept
  { return { indent, hanging };  }

// Error numbers
constexpr
details_util::Formatted_errno
format_errno(int err)
  noexcept
  { return { err };  }

// Negative array index wrapper
struct Wrapped_Index
  {
    uint64_t nprepend;  // number of elements to prepend
    uint64_t nappend;   // number of elements to append
    size_t rindex;      // the wrapped index (valid if both
                        // `nprepend` and `nappend` are zeroes)
  };

ROCKET_CONST_FUNCTION
Wrapped_Index
wrap_index(int64_t index, size_t size)
  noexcept;

// Note that all bits in the result are filled.
uint64_t
generate_random_seed()
  noexcept;

}  // namespace asteria

#endif
