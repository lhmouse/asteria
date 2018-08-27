// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "utilities.hpp"
#include "runtime_error.hpp"
#include <iostream> // std::cerr
#include <time.h> // ::time_t, ::clock_gettime(), ::localtime()
#include <stdio.h> // ::sprintf()

#ifdef _WIN32
#  include <windows.h> // ::SYSTEMTIME, ::GetSystemTime()
#endif

namespace Asteria {

Formatter::Formatter(const char *file, unsigned long line, const char *func) noexcept
  : m_file(file), m_line(line), m_func(func)
  {
    m_stream <<std::boolalpha;
  }
Formatter::~Formatter()
  {
  }

void Formatter::do_put(bool value)
  {
    m_stream <<value;
  }
void Formatter::do_put(char value)
  {
    m_stream <<value;
  }
void Formatter::do_put(signed char value)
  {
    m_stream <<static_cast<int>(value);
  }
void Formatter::do_put(unsigned char value)
  {
    m_stream <<static_cast<unsigned>(value);
  }
void Formatter::do_put(short value)
  {
    m_stream <<static_cast<int>(value);
  }
void Formatter::do_put(unsigned short value)
  {
    m_stream <<static_cast<unsigned>(value);
  }
void Formatter::do_put(int value)
  {
    m_stream <<value;
  }
void Formatter::do_put(unsigned value)
  {
    m_stream <<value;
  }
void Formatter::do_put(long value)
  {
    m_stream <<value;
  }
void Formatter::do_put(unsigned long value)
  {
    m_stream <<value;
  }
void Formatter::do_put(long long value)
  {
    m_stream <<value;
  }
void Formatter::do_put(unsigned long long value)
  {
    m_stream <<value;
  }
void Formatter::do_put(const char *value)
  {
    m_stream <<value;
  }
void Formatter::do_put(const signed char *value)
  {
    m_stream <<static_cast<const void *>(value);
  }
void Formatter::do_put(const unsigned char *value)
  {
    m_stream <<static_cast<const void *>(value);
  }
void Formatter::do_put(const void *value)
  {
    m_stream <<value;
  }

namespace {

  int do_print_time(char *str)
    {
      int len;
#ifdef _WIN32
      ::SYSTEMTIME s;
      ::GetSystemTime(&s);
      len = ::sprintf(str, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
                      s.wYear, s.wMonth, s.wDay, s.wHour, s.wMinute, s.wSecond, s.wMilliseconds);
#else
      ::timespec t;
      int err = ::clock_gettime(CLOCK_REALTIME, &t);
      ROCKET_ASSERT(err == 0);
      ::tm s;
      ::localtime_r(&(t.tv_sec), &s);
      len = ::sprintf(str, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
                      s.tm_year + 1900, s.tm_mon, s.tm_mday, s.tm_hour, s.tm_min, s.tm_sec, static_cast<int>(t.tv_nsec / 1000000));
#endif
      return len;
    }

}

bool write_log_to_stderr(Formatter &&fmt) noexcept
  try {
    auto &oss = fmt.get_stream();
    oss.set_caret(0);
    char time_str[64];
    do_print_time(time_str);
    oss <<time_str <<" $$ ";
    oss.set_caret(String::npos);
    oss <<" @@ " <<fmt.get_file() <<':' <<fmt.get_line()
    auto str = oss.extract_string();
    for(auto i = str.find('\n'); i != str.npos; i = str.find('\n', i + 2)) {
      str.insert(i + 1, 1, '\t');
    }
    std::cerr <<str <<std::endl;
    return true;
  } catch(...) {
    return false;
  }

void throw_runtime_error(Formatter &&fmt)
  {
    auto &oss = fmt.get_stream();
    oss.set_caret(0);
    oss <<fmt.get_func() <<": ";
    auto str = oss.extract_string();
    throw Runtime_error(std::move(str));
  }

}
