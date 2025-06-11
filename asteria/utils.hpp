// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_UTILS_
#define ASTERIA_UTILS_

#include "fwd.hpp"
#include "../rocket/tinyfmt_str.hpp"
namespace asteria {

// Formatting
template<typename... xParams>
constexpr
array<const char*, sizeof...(xParams)>
make_string_template(const xParams&... params)
  {
    array<const char*, sizeof...(xParams)> templs = { params... };
    return templs;
  }

template<size_t N, typename... xParams>
tinyfmt&
format(tinyfmt& fmt, const array<const char*, N>& templs, const xParams&... params)
  {
    for(size_t k = 0;  k != N;  ++k) {
      if(k != 0)
        fmt.putc('\n');
      format(fmt, templs[k], params...);
    }
    return fmt;
  }

template<size_t N, typename... xParams>
cow_string&
format(cow_string& str, const array<const char*, N>& templs, const xParams&... params)
  {
    ::rocket::tinyfmt_str fmt;
    fmt.set_string(move(str), ::rocket::tinybuf::open_append);
    format(fmt, templs, params...);
    str = fmt.extract_string();
    return str;
  }

template<typename xTempls, typename... xParams>
cow_string
sformat(const xTempls& templs, const xParams&... params)
  {
    ::rocket::tinyfmt_str fmt;
    format(fmt, templs, params...);
    return fmt.extract_string();
  }

// Error handling
// Note string templates must be parenthesized.
ptrdiff_t
write_log_to_stderr(const char* file, long line, const char* func, cow_string&& msg);

#define ASTERIA_WRITE_STDERR(str)  \
    (::asteria::write_log_to_stderr(__FILE__, __LINE__, __FUNCTION__, str))

#define ASTERIA_TERMINATE(TEMPLATE, ...)  \
    ((void) ::asteria::write_log_to_stderr(__FILE__, __LINE__, __FUNCTION__,  \
       ::asteria::sformat((::asteria::make_string_template TEMPLATE), ##__VA_ARGS__)),  \
     ::std::terminate())

[[noreturn]]
void
throw_runtime_error(const char* file, long line, const char* func, cow_string&& msg);

#define ASTERIA_THROW(TEMPLATE, ...)  \
    ((void) ::asteria::throw_runtime_error(__FILE__, __LINE__, __FUNCTION__,  \
       ::asteria::sformat((::asteria::make_string_template TEMPLATE), ##__VA_ARGS__)),  \
     ROCKET_UNREACHABLE())

// Raw memory management
template<typename objectT, typename sourceT>
ROCKET_ALWAYS_INLINE
void
bcopy(objectT& __restrict dst, const sourceT& __restrict src) noexcept
  {
    static_assert(sizeof(objectT) == sizeof(sourceT), "size mismatch");
    using bytes = char [sizeof(objectT)];
    ::memcpy(reinterpret_cast<bytes&>(dst), reinterpret_cast<const bytes&>(src),
             sizeof(objectT));
  }

template<typename objectT>
ROCKET_ALWAYS_INLINE
void
bfill(objectT& dst, unsigned char value) noexcept
  {
    using bytes = char [sizeof(objectT)];
    ::memset(reinterpret_cast<bytes&>(dst), value, sizeof(objectT));
  }

// C character types
enum : uint8_t
  {
    cmask_space   = 0x01,  // [ \t\v\f\r\n]
    cmask_alpha   = 0x02,  // [A-Za-z]
    cmask_digit   = 0x04,  // [0-9]
    cmask_xdigit  = 0x08,  // [0-9A-Fa-f]
    cmask_namei   = 0x10,  // [A-Za-z_]
    cmask_name    = 0x14,  // [A-Za-z0-9_]
    cmask_blank   = 0x20,  // [ \t]
    cmask_cntrl   = 0x40,  // [[:cntrl:]]
  };

constexpr uint8_t cmask_table[128] =
  {
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x40, 0x21, 0x61, 0x41, 0x41, 0x41, 0x40, 0x40,
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C,
    0x0C, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x12,
    0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12,
    0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12,
    0x12, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00, 0x10,
    0x00, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x12,
    0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12,
    0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12,
    0x12, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00, 0x40,
  };

constexpr
uint8_t
get_cmask(char ch) noexcept
  {
    return ((ch & 0x7F) == ch) ? cmask_table[(uint8_t) ch] : 0;
  }

constexpr
bool
is_cmask(char ch, uint8_t mask) noexcept
  {
    return noadl::get_cmask(ch) & mask;
  }

// Negative array index wrapper
struct Wrapped_Index
  {
    uint64_t nappend;   // number of elements to append
    uint64_t nprepend;  // number of elements to prepend

    size_t rindex;  // wrapped index, within range if and only if
                    // both `nappend` and `nprepend` are zeroes

    constexpr Wrapped_Index(ptrdiff_t ssize, int64_t sindex) noexcept
      :
        nappend(0), nprepend(0),
        rindex(0)
      {
        ROCKET_ASSERT(ssize >= 0);

        if(sindex >= 0) {
          ptrdiff_t last_sindex = ssize - 1;
          this->nappend = (uint64_t) (::rocket::max(last_sindex, sindex) - last_sindex);
          this->rindex = (size_t) sindex;
        }
        else {
          ptrdiff_t first_sindex = 0 - ssize;
          this->nprepend = (uint64_t) (first_sindex - ::rocket::min(sindex, first_sindex));
          this->rindex = (size_t) (sindex + ssize) + (size_t) this->nprepend;
        }
      }
  };

constexpr
Wrapped_Index
wrap_array_index(ptrdiff_t ssize, int64_t sindex) noexcept
  {
    return Wrapped_Index(ssize, sindex);
  }

// Numeric conversion
int64_t
safe_double_to_int64(double val);

// UTF-8 conversion functions
bool
utf8_encode(char*& pos, char32_t cp) noexcept;

bool
utf8_encode(cow_string& text, char32_t cp);

bool
utf8_decode(char32_t& cp, const char*& pos, size_t avail) noexcept;

bool
utf8_decode(char32_t& cp, const cow_string& text, size_t& offset);

// UTF-16 conversion functions
bool
utf16_encode(char16_t*& pos, char32_t cp) noexcept;

bool
utf16_encode(cow_u16string& text, char32_t cp);

bool
utf16_decode(char32_t& cp, const char16_t*& pos, size_t avail) noexcept;

bool
utf16_decode(char32_t& cp, const cow_u16string& text, size_t& offset);

// C-style quoting
tinyfmt&
c_quote(tinyfmt& fmt, const char* data, size_t size);

tinyfmt&
c_quote(tinyfmt& fmt, const cow_string& data);

cow_string&
c_quote(cow_string& str, const char* data, size_t size);

cow_string&
c_quote(cow_string& str, const cow_string& data);

// Path resolution
cow_string
get_real_path(const cow_string& path);

}  // namespace asteria
#endif
