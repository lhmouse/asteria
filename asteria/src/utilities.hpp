// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_UTILITIES_HPP_
#define ASTERIA_UTILITIES_HPP_

#include "rocket/compatibility.h"
#include "rocket/insertable_ostream.hpp"
#include <iomanip>
#include <exception>

namespace Asteria {

class Formatter
  {
  private:
    rocket::insertable_ostream m_stream;

  public:
    Formatter();
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

constexpr bool are_debug_logs_enabled() noexcept
  {
#ifdef ASTERIA_ENABLE_DEBUG_LOGS
    return true;
#else
    return false;
#endif
  }

extern bool write_log_to_stderr(const char *file, unsigned long line, Formatter &&fmt) noexcept;
[[noreturn]] extern void throw_runtime_error(const char *funcsig, Formatter &&fmt);

}

#define ASTERIA_CREATE_FORMATTER(...)         (::std::move((::Asteria::Formatter(), __VA_ARGS__)))

#define ASTERIA_FORMAT_STRING(...)            (ASTERIA_CREATE_FORMATTER(__VA_ARGS__).get_stream().extract_string())
#define ASTERIA_DEBUG_LOG(...)                (::Asteria::are_debug_logs_enabled() && ::Asteria::write_log_to_stderr(__FILE__, __LINE__, ASTERIA_CREATE_FORMATTER(__VA_ARGS__)))
#define ASTERIA_TERMINATE(...)                (::Asteria::write_log_to_stderr(__FILE__, __LINE__, ASTERIA_CREATE_FORMATTER("FATAL ERROR: ", __VA_ARGS__)), ::std::terminate())
#define ASTERIA_THROW_RUNTIME_ERROR(...)      (::Asteria::throw_runtime_error(ROCKET_FUNCSIG, ASTERIA_CREATE_FORMATTER(__VA_ARGS__)))

#endif
