// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_UTILITIES_HPP_
#define ASTERIA_UTILITIES_HPP_

#include "rocket/compatibility.h"
#include "rocket/insertable_ostream.hpp"
#include <iomanip>
#include <exception>
#include <cstdint>

namespace Asteria {

///////////////////////////////////////////////////////////////////////////////
// Formatter
///////////////////////////////////////////////////////////////////////////////

class Formatter
  {
  private:
    rocket::insertable_ostream m_stream;

  public:
    Formatter()
      {
        this->m_stream.setf(std::ios::boolalpha);
      }
    ~Formatter();

    Formatter(const Formatter &)
      = delete;
    Formatter & operator=(const Formatter &)
      = delete;

  private:
    template<typename ValueT>
      void do_put(const ValueT &value)
      {
        this->m_stream <<value;
      }
    void do_put(bool value);
    void do_put(char value);
    void do_put(signed char value);
    void do_put(unsigned char value);
    void do_put(short value);
    void do_put(unsigned short value);
    void do_put(int value);
    void do_put(unsigned value);
    void do_put(long value);
    void do_put(unsigned long value);
    void do_put(long long value);
    void do_put(unsigned long long value);
    void do_put(const char *value);
    void do_put(const signed char *value);
    void do_put(const unsigned char *value);
    void do_put(const void *value);

  public:
    const rocket::insertable_ostream & get_stream() const noexcept
      {
        return this->m_stream;
      }
    rocket::insertable_ostream & get_stream() noexcept
      {
        return this->m_stream;
      }
    template<typename ValueT>
      Formatter & operator,(const ValueT &value) noexcept
      try {
        this->do_put(value);
        return *this;
      } catch(...) {
        return *this;
      }
  };

#define ASTERIA_FORMAT(fmt_, ...)             (static_cast<::Asteria::Formatter &&>(fmt_), __VA_ARGS__)
#define ASTERIA_CREATE_FORMATTER(...)         (::std::move(ASTERIA_FORMAT(::Asteria::Formatter(), __VA_ARGS__)))
#define ASTERIA_FORMAT_STRING(...)            (ASTERIA_CREATE_FORMATTER(__VA_ARGS__).get_stream().extract_string())

extern bool are_debug_logs_enabled() noexcept;
extern bool write_log_to_stderr(const char *file, unsigned long line, Formatter &&fmt) noexcept;

#define ASTERIA_DEBUG_LOG(...)                (::Asteria::are_debug_logs_enabled() && ::Asteria::write_log_to_stderr(__FILE__, __LINE__, ASTERIA_CREATE_FORMATTER(__VA_ARGS__)))
#define ASTERIA_TERMINATE(...)                (::Asteria::write_log_to_stderr(__FILE__, __LINE__, ASTERIA_CREATE_FORMATTER("FATAL ERROR: ", __VA_ARGS__)), ::std::terminate())

class Runtime_error : public virtual std::exception
  {
  private:
    rocket::cow_string m_msg;

  public:
    explicit Runtime_error(rocket::cow_string msg) noexcept
      : m_msg(std::move(msg))
      {
      }
    ~Runtime_error();

  public:
    const char * what() const noexcept override;
  };

[[noreturn]] extern void throw_runtime_error(const char *funcsig, Formatter &&fmt);

#define ASTERIA_THROW_RUNTIME_ERROR(...)      (::Asteria::throw_runtime_error(ROCKET_FUNCSIG, ASTERIA_CREATE_FORMATTER(__VA_ARGS__)))

///////////////////////////////////////////////////////////////////////////////
// Indent
///////////////////////////////////////////////////////////////////////////////

class Indent
  {
  private:
    char m_head;
    std::size_t m_count;

  public:
    constexpr Indent(char xhead, std::size_t xcount) noexcept
      : m_head(xhead), m_count(xcount)
      {
      }

  public:
    char head() const noexcept
      {
        return this->m_head;
      }
    std::size_t count() const noexcept
      {
        return this->m_count;
      }
  };

constexpr Indent indent(char head, std::size_t count) noexcept
  {
    return Indent(head, count);
  }

extern std::ostream & operator<<(std::ostream &os, const Indent &n);

///////////////////////////////////////////////////////////////////////////////
// Quote
///////////////////////////////////////////////////////////////////////////////

class Quote
  {
  private:
    const char *m_data;
    std::size_t m_size;

  public:
    constexpr Quote(const char *xdata, std::size_t xsize) noexcept
      : m_data(xdata), m_size(xsize)
      {
      }

  public:
    const char * data() const noexcept
      {
        return this->m_data;
      }
    std::size_t size() const noexcept
      {
        return this->m_size;
      }
  };

constexpr Quote quote(const char *data, std::size_t size) noexcept
  {
    return Quote(data, size);
  }
inline Quote quote(const char *str) noexcept
  {
    return Quote(str, std::char_traits<char>::length(str));
  }
inline Quote quote(const rocket::cow_string &str) noexcept
  {
    return Quote(str.data(), str.size());
  }

extern std::ostream & operator<<(std::ostream &os, const Quote &q);

///////////////////////////////////////////////////////////////////////////////
// Miscellaneous
///////////////////////////////////////////////////////////////////////////////

inline std::uint64_t wrap_index(std::uint64_t &bfill_out, std::uint64_t &efill_out, std::int64_t index, std::size_t size)
  {
    const auto rsize = static_cast<std::int64_t>(size);
    auto rindex = index;
    if(rindex < 0) {
      rindex += rsize;
    }
    bfill_out = 0;
    efill_out = 0;
    if(rindex < 0) {
      bfill_out = 0 - static_cast<std::uint64_t>(rindex);
    } else if(rindex >= rsize) {
      efill_out = static_cast<std::uint64_t>(rindex) - size + 1;
    }
    return static_cast<std::uint64_t>(rindex);
  }

}

#endif
