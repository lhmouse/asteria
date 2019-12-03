// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_UTILITIES_HPP_
#define ASTERIA_UTILITIES_HPP_

#include "fwd.hpp"
#include <exception>

namespace Asteria {

// Stream-style formatter
class Formatter
  {
  private:
    tinyfmt_str m_fmt;

  public:
    template<typename ValueT> Formatter& operator,(const ValueT& value)
      {
        return (this->m_fmt << value), *this;
      }
    cow_string extract_string() noexcept
      {
        return this->m_fmt.extract_string();
      }
  };

#define ASTERIA_FORMAT(...)        ((::Asteria::Formatter(), __VA_ARGS__).extract_string())

extern bool write_log_to_stderr(const char* file, long line, cow_string&& msg) noexcept;

#define ASTERIA_TERMINATE(...)     (::Asteria::write_log_to_stderr(__FILE__, __LINE__,  \
                                         ASTERIA_FORMAT("ASTERIA_TERMINATE: ", __VA_ARGS__,  \
                                           "\nThis is likely a bug. Please report.")),  \
                                       ::std::terminate())

#define ASTERIA_THROW(...)         (::rocket::sprintf_and_throw<::std::runtime_error>(  \
                                       "%s: %s\n[thrown from native code at '%s:%ld']",  \
                                       __func__, ASTERIA_FORMAT(__VA_ARGS__).c_str(),  \
                                       __FILE__, (long)__LINE__))

// UTF-8 conversion functions
extern bool utf8_encode(char*& pos, char32_t cp);
extern bool utf8_encode(cow_string& text, char32_t cp);
extern bool utf8_decode(char32_t& cp, const char*& pos, size_t avail);
extern bool utf8_decode(char32_t& cp, const cow_string& text, size_t& offset);

// UTF-16 conversion functions
extern bool utf16_encode(char16_t*& pos, char32_t cp);
extern bool utf16_encode(cow_u16string& text, char32_t cp);
extern bool utf16_decode(char32_t& cp, const char16_t*& pos, size_t avail);
extern bool utf16_decode(char32_t& cp, const cow_u16string& text, size_t& offset);

// C-style quoting
struct Quote_Wrapper
  {
    const char* str;
    size_t len;
  };

constexpr Quote_Wrapper quote(const char* str, size_t len) noexcept
  {
    return { str, len };
  }
inline Quote_Wrapper quote(const char* str) noexcept
  {
    return quote(str, std::strlen(str));
  }
inline Quote_Wrapper quote(const cow_string& str) noexcept
  {
    return quote(str.data(), str.size());
  }

extern tinyfmt& operator<<(tinyfmt& fmt, const Quote_Wrapper& q);

// Justifying
struct Paragraph_Wrapper
  {
    size_t indent;
    size_t hanging;
  };

constexpr Paragraph_Wrapper pwrap(size_t indent, size_t hanging) noexcept
  {
    return { indent, hanging };
  }

extern tinyfmt& operator<<(tinyfmt& fmt, const Paragraph_Wrapper& q);

// Negative array index wrapper
struct Wrapped_Index
  {
    uint64_t nprepend;  // number of elements to prepend
    uint64_t nappend;  // number of elements to append
    size_t rindex;  // the wrapped index (valid if both `nprepend` and `nappend` are zeroes)
  };

extern Wrapped_Index wrap_index(int64_t index, size_t size) noexcept;

// Note that the return value may be either positive or negative.
extern uint64_t generate_random_seed() noexcept;

}  // namespace Asteria

#endif
