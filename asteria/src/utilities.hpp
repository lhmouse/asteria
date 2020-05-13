// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_UTILITIES_HPP_
#define ASTERIA_UTILITIES_HPP_

#include "fwd.hpp"
#include "../rocket/tinyfmt_str.hpp"
#include "../rocket/format.hpp"

namespace asteria {

// Error handling
ptrdiff_t
write_log_to_stderr(const char* file, long line, cow_string&& msg)
noexcept;

template<typename... ParamsT>
ROCKET_NOINLINE
cow_string
format_string(const ParamsT&... params)
  {
    ::rocket::tinyfmt_str fmt;
    format(fmt, params...);  // ADL intended
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
       "%s: %s\n[thrown from native code at '%s:%ld']", __func__,  \
         ::asteria::format_string("" __VA_ARGS__).c_str(), __FILE__,  \
         static_cast<long>(__LINE__)))

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
constexpr
typename ::std::underlying_type<enumT>::type
weaken_enum(enumT value)
noexcept
  { return static_cast<typename ::std::underlying_type<enumT>::type>(value);  }

// C character types
enum : uint8_t
  {
    cctype_space   = 0x01,  // [ \t\v\f\r\n]
    cctype_alpha   = 0x02,  // [A-Za-z]
    cctype_digit   = 0x04,  // [0-9]
    cctype_xdigit  = 0x08,  // [0-9A-Fa-f]
    cctype_namei   = 0x10,  // [A-Za-z_]
  };

extern const uint8_t cctype_table[128];

inline
uint8_t
get_cctype(char c)
noexcept
  { return (uint8_t(c) < 128) ? cctype_table[uint8_t(c)] : 0;  }

inline
bool
is_cctype(char c, uint8_t mask)
noexcept
  { return get_cctype(c) & mask;  }

// C-style quoting
struct Quote_Wrapper
  {
    const char* str;
    size_t len;
  };

constexpr
Quote_Wrapper
quote(const char* str, size_t len)
noexcept
  { return { str, len };  }

inline
Quote_Wrapper
quote(const char* str)
noexcept
  { return quote(str, ::std::strlen(str));  }

inline
Quote_Wrapper
quote(const cow_string& str)
noexcept
  { return quote(str.data(), str.size());  }

tinyfmt&
operator<<(tinyfmt& fmt, const Quote_Wrapper& q);

// Justifying
struct Paragraph_Wrapper
  {
    size_t indent;
    size_t hanging;
  };

constexpr
Paragraph_Wrapper
pwrap(size_t indent, size_t hanging)
noexcept
  { return { indent, hanging };  }

tinyfmt&
operator<<(tinyfmt& fmt, const Paragraph_Wrapper& q);

// Error numbers
struct Formatted_errno
  {
    int err;
  };

constexpr
Formatted_errno
format_errno(int err)
noexcept
  { return { err };  }

tinyfmt&
operator<<(tinyfmt& fmt, const Formatted_errno& e);

// Negative array index wrapper
struct Wrapped_Index
  {
    uint64_t nprepend;  // number of elements to prepend
    uint64_t nappend;  // number of elements to append
    size_t rindex;  // the wrapped index (valid if both `nprepend` and `nappend` are zeroes)
  };

Wrapped_Index
wrap_index(int64_t index, size_t size)
noexcept;

// Note that all bits in the result are filled.
uint64_t
generate_random_seed()
noexcept;

}  // namespace asteria

#endif
