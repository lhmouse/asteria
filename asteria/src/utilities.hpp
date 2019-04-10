// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_UTILITIES_HPP_
#define ASTERIA_UTILITIES_HPP_

#include "rocket/compiler.h"
#include "rocket/utilities.hpp"
#include "rocket/insertable_ostream.hpp"
#include "rocket/unique_ptr.hpp"
#include <iomanip>
#include <exception>
#include <cstdint>

namespace Asteria {

///////////////////////////////////////////////////////////////////////////////
// Internal Macros
///////////////////////////////////////////////////////////////////////////////

#define ASTERIA_AND_(x_, y_)             (bool(x_) && bool(y_))
#define ASTERIA_OR_(x_, y_)              (bool(x_) || bool(y_))
#define ASTERIA_COMMA_(x_, y_)           (void(x_) ,      (y_))

///////////////////////////////////////////////////////////////////////////////
// Formatter
///////////////////////////////////////////////////////////////////////////////

class Formatter
  {
  private:
    rocket::unique_ptr<rocket::insertable_ostream> m_strm_opt;

  public:
    Formatter() noexcept
      {
      }
    ~Formatter();

  private:
    std::ostream& do_open_stream();

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

  public:
    template<typename ValueT> Formatter& operator,(const ValueT& value) noexcept
      try {
        this->do_put(value);
        return *this;
      } catch(...) {
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

///////////////////////////////////////////////////////////////////////////////
// Runtime_Error
///////////////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////////////
// Quote
///////////////////////////////////////////////////////////////////////////////

class Quote
  {
  private:
    const char* m_data;
    std::size_t m_size;

  public:
    constexpr Quote(const char* xdata, std::size_t xsize) noexcept
      : m_data(xdata), m_size(xsize)
      {
      }

  public:
    const char* data() const noexcept
      {
        return this->m_data;
      }
    std::size_t size() const noexcept
      {
        return this->m_size;
      }
  };

constexpr Quote quote(const char* data, std::size_t size) noexcept
  {
    return Quote(data, size);
  }
inline Quote quote(const char* str) noexcept
  {
    return Quote(str, std::char_traits<char>::length(str));
  }
inline Quote quote(const rocket::cow_string& str) noexcept
  {
    return Quote(str.data(), str.size());
  }

extern std::ostream& operator<<(std::ostream& os, const Quote& q);

///////////////////////////////////////////////////////////////////////////////
// Wrap Index
///////////////////////////////////////////////////////////////////////////////

struct Wrapped_Index
  {
    std::uint64_t nprepend;  // number of elements to prepend
    std::uint64_t nappend;  // number of elements to append
    std::size_t rindex;  // the wrapped index (valid if both `nprepend` and `nappend` are zeroes)
  };

extern Wrapped_Index wrap_index(std::int64_t index, std::size_t size) noexcept;

///////////////////////////////////////////////////////////////////////////////
// Random Seed
///////////////////////////////////////////////////////////////////////////////

// Note that the return value may be either positive or negative.
extern std::uint64_t generate_random_seed() noexcept;

}  // namespace Asteria

#endif
