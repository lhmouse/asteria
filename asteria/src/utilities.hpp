// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_UTILITIES_HPP_
#define ASTERIA_UTILITIES_HPP_

#include "fwd.hpp"
#include "rocket/compiler.h"
#include "rocket/utilities.hpp"
#include "rocket/cow_ostringstream.hpp"
#include "rocket/unique_ptr.hpp"
#include <iomanip>
#include <exception>
#include <cstdint>

namespace Asteria {

class Formatter
  {
  private:
    rocket::unique_ptr<rocket::cow_ostringstream> m_strm_opt;

  public:
    Formatter() noexcept
      {
      }
    ~Formatter();

  private:
    template<typename ValueT> void do_put(const ValueT& value)
      {
        this->do_open_stream() << value;
      }
    void do_put(bool value)
      {
        this->do_open_stream() << value;
      }
    void do_put(char value)
      {
        this->do_open_stream() << value;
      }
    void do_put(signed char value)
      {
        this->do_open_stream() << static_cast<int>(value);
      }
    void do_put(unsigned char value)
      {
        this->do_open_stream() << static_cast<unsigned>(value);
      }
    void do_put(short value)
      {
        this->do_open_stream() << static_cast<int>(value);
      }
    void do_put(unsigned short value)
      {
        this->do_open_stream() << static_cast<unsigned>(value);
      }
    void do_put(int value)
      {
        this->do_open_stream() << value;
      }
    void do_put(unsigned value)
      {
        this->do_open_stream() << value;
      }
    void do_put(long value)
      {
        this->do_open_stream() << value;
      }
    void do_put(unsigned long value)
      {
        this->do_open_stream() << value;
      }
    void do_put(long long value)
      {
        this->do_open_stream() << value;
      }
    void do_put(unsigned long long value)
      {
        this->do_open_stream() << value;
      }
    void do_put(const char* value)
      {
        this->do_open_stream() << value;
      }
    void do_put(const signed char* value)
      {
        this->do_open_stream() << static_cast<const void*>(value);
      }
    void do_put(const unsigned char* value)
      {
        this->do_open_stream() << static_cast<const void*>(value);
      }
    void do_put(const void* value)
      {
        this->do_open_stream() << value;
      }
    std::ostream& do_open_stream();

  public:
    template<typename ValueT> Formatter& operator,(const ValueT& value) noexcept
      try {
        this->do_put(value);
        return *this;
      }
      catch(...) {
        return *this;
      }
    rocket::cow_string extract_string() noexcept
      {
        return this->m_strm_opt ? this->m_strm_opt->extract_string() : rocket::clear;
      }
  };

#define ASTERIA_XFORMAT_(...)      ((::Asteria::Formatter(), __VA_ARGS__).extract_string())

extern bool are_debug_logs_enabled() noexcept;
extern bool write_log_to_stderr(const char* file, long line, rocket::cow_string&& msg) noexcept;

// If `are_debug_logs_enabled()` returns `true`, evaluate arguments and write the result to `std::cerr`; otherwise, do nothing.
#define ASTERIA_DEBUG_LOG(...)     ASTERIA_AND_(ROCKET_UNEXPECT(::Asteria::are_debug_logs_enabled()),  \
                                                ::Asteria::write_log_to_stderr(__FILE__, __LINE__, ASTERIA_XFORMAT_(__VA_ARGS__)))

// Evaluate arguments and write the result to `std::cerr`, then call `std::terminate()`.
#define ASTERIA_TERMINATE(...)     ASTERIA_COMMA_(::Asteria::write_log_to_stderr(__FILE__, __LINE__, ASTERIA_XFORMAT_(__VA_ARGS__)),  \
                                                  ::std::terminate())

class Runtime_Error : public virtual std::exception
  {
  private:
    rocket::cow_string m_msg;

  public:
    explicit Runtime_Error(const rocket::cow_string& msg) noexcept
      : m_msg(msg)
      {
      }
    explicit Runtime_Error(rocket::cow_string&& msg) noexcept
      : m_msg(rocket::move(msg))
      {
      }
    ~Runtime_Error() override;

  public:
    const char* what() const noexcept override
      {
        return this->m_msg.c_str();
      }
  };

[[noreturn]] extern bool throw_runtime_error(const char* func, rocket::cow_string&& msg);

// Evaluate arguments to create a string, then throw an exception containing this string.
#define ASTERIA_THROW_RUNTIME_ERROR(...)     ASTERIA_COMMA_(::Asteria::throw_runtime_error(__func__, ASTERIA_XFORMAT_(__VA_ARGS__)),  \
                                                            ::std::terminate())

extern bool utf8_encode(char*& pos, char32_t cp);
extern bool utf8_encode(rocket::cow_string& text, char32_t cp);
extern bool utf8_decode(char32_t& cp, const char*& pos, std::size_t avail);
extern bool utf8_decode(char32_t& cp, const rocket::cow_string& text, std::size_t& offset);

extern bool utf16_encode(char16_t*& pos, char32_t cp);
extern bool utf16_encode(rocket::cow_u16string& text, char32_t cp);
extern bool utf16_decode(char32_t& cp, const char16_t*& pos, std::size_t avail);
extern bool utf16_decode(char32_t& cp, const rocket::cow_u16string& text, std::size_t& offset);

extern rocket::cow_string& quote(rocket::cow_string& sbuf, const char* str, std::size_t len);
extern rocket::cow_string quote(const char* str, std::size_t len);
extern rocket::cow_string& quote(rocket::cow_string& sbuf, const char* str);
extern rocket::cow_string quote(const char* str);
extern rocket::cow_string& quote(rocket::cow_string& sbuf, const rocket::cow_string& str);
extern rocket::cow_string quote(const rocket::cow_string& str);

extern rocket::cow_string& pwrapln(rocket::cow_string& sbuf, std::size_t indent, std::size_t hanging);
extern rocket::cow_string pwrapln(std::size_t indent, std::size_t hanging);

struct Wrapped_Index
  {
    std::uint64_t nprepend;  // number of elements to prepend
    std::uint64_t nappend;  // number of elements to append
    std::size_t rindex;  // the wrapped index (valid if both `nprepend` and `nappend` are zeroes)
  };

extern Wrapped_Index wrap_index(std::int64_t index, std::size_t size) noexcept;

// Note that the return value may be either positive or negative.
extern std::uint64_t generate_random_seed() noexcept;

}  // namespace Asteria

#endif
