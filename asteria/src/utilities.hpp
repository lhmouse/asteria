// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_UTILITIES_HPP_
#define ASTERIA_UTILITIES_HPP_

#include "fwd.hpp"
#include <exception>

namespace Asteria {

class Formatter
  {
  private:
    uptr<tinyfmt_str> m_fmt;

  public:
    Formatter() noexcept
      {
      }
    ~Formatter();

  private:
    template<typename ValT> void do_put(const ValT& val)
      {
        this->do_open_stream() << val;
      }
    void do_put(bool val)
      {
        this->do_open_stream() << val;
      }
    void do_put(char val)
      {
        this->do_open_stream() << val;
      }
    void do_put(signed char val)
      {
        this->do_open_stream() << static_cast<int>(val);
      }
    void do_put(unsigned char val)
      {
        this->do_open_stream() << static_cast<unsigned>(val);
      }
    void do_put(short val)
      {
        this->do_open_stream() << static_cast<int>(val);
      }
    void do_put(unsigned short val)
      {
        this->do_open_stream() << static_cast<unsigned>(val);
      }
    void do_put(int val)
      {
        this->do_open_stream() << val;
      }
    void do_put(unsigned val)
      {
        this->do_open_stream() << val;
      }
    void do_put(long val)
      {
        this->do_open_stream() << val;
      }
    void do_put(unsigned long val)
      {
        this->do_open_stream() << val;
      }
    void do_put(long long val)
      {
        this->do_open_stream() << val;
      }
    void do_put(unsigned long long val)
      {
        this->do_open_stream() << val;
      }
    void do_put(const char* val)
      {
        this->do_open_stream() << val;
      }
    void do_put(const signed char* val)
      {
        this->do_open_stream() << static_cast<const void*>(val);
      }
    void do_put(const unsigned char* val)
      {
        this->do_open_stream() << static_cast<const void*>(val);
      }
    void do_put(const void* val)
      {
        this->do_open_stream() << val;
      }
    tinyfmt& do_open_stream();

  public:
    template<typename ValT> Formatter& operator,(const ValT& val)
      {
        return this->do_put(val), *this;
      }
    cow_string extract_string() noexcept
      {
        const auto& fmt = this->m_fmt;
        if(!fmt) {
          return rocket::sref("");
        }
        return fmt->extract_string();
      }
  };

#define ASTERIA_XFORMAT_(...)      ((::Asteria::Formatter(), __VA_ARGS__).extract_string())

extern bool write_log_to_stderr(const char* file, long line, cow_string&& msg) noexcept;

// N.B. You can define this as something non-constant.
#ifndef ASTERIA_ENABLE_DEBUG_LOGS
#  define ASTERIA_ENABLE_DEBUG_LOGS  0
#endif
// Write arguments to the standard error stream, if `ASTERIA_ENABLE_DEBUG_LOGS` is defined to be a non-zero value.
#define ASTERIA_DEBUG_LOG(...)     ASTERIA_AND_(ROCKET_UNEXPECT(ASTERIA_ENABLE_DEBUG_LOGS),  \
                                                ::Asteria::write_log_to_stderr(__FILE__, __LINE__, ASTERIA_XFORMAT_(__VA_ARGS__)))
// Write arguments to the standard error stream, then call `std::terminate()`.
#define ASTERIA_TERMINATE(...)     ASTERIA_COMMA_(::Asteria::write_log_to_stderr(__FILE__, __LINE__, ASTERIA_XFORMAT_(__VA_ARGS__)),  \
                                                  ::std::terminate())

class Runtime_Error : public virtual std::exception
  {
  private:
    cow_string m_msg;

  public:
    explicit Runtime_Error(cow_string&& msg)
      {
        this->m_msg.swap(msg);
        this->m_msg.insert(0, "asteria runtime error: ");
      }
    ~Runtime_Error() override;

  public:
    const char* what() const noexcept override
      {
        return this->m_msg.c_str();
      }
  };

[[noreturn]] extern bool throw_runtime_error(const char* func, cow_string&& msg);

// Evaluate arguments to create a string, then throw an exception containing this string.
#define ASTERIA_THROW_RUNTIME_ERROR(...)     ASTERIA_COMMA_(::Asteria::throw_runtime_error(__func__, ASTERIA_XFORMAT_(__VA_ARGS__)),  \
                                                            ::std::terminate())

extern bool utf8_encode(char*& pos, char32_t cp);
extern bool utf8_encode(cow_string& text, char32_t cp);
extern bool utf8_decode(char32_t& cp, const char*& pos, size_t avail);
extern bool utf8_decode(char32_t& cp, const cow_string& text, size_t& offset);

extern bool utf16_encode(char16_t*& pos, char32_t cp);
extern bool utf16_encode(cow_u16string& text, char32_t cp);
extern bool utf16_decode(char32_t& cp, const char16_t*& pos, size_t avail);
extern bool utf16_decode(char32_t& cp, const cow_u16string& text, size_t& offset);

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
