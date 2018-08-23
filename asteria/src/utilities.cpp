// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "utilities.hpp"
#include "runtime_error.hpp"
#include <iostream> // std::cerr
#include <time.h> // ::time_t, ::time(), ::asctime_r()

namespace Asteria {

Formatter::Formatter(const char *file, unsigned long line, const char *func) noexcept
  : m_file(file), m_line(line), m_func(func)
  {
    m_stream <<std::boolalpha;
  }
Formatter::~Formatter()
  = default;

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

bool are_debug_logs_enabled() noexcept
  {
#ifdef ENABLE_DEBUG_LOGS
    return true;
#else
    return false;
#endif
  }
bool write_log_to_stderr(Formatter &&fmt) noexcept
  try {
    auto &oss = fmt.get_stream();
    ::time_t now;
    ::time(&now);
    char time_str[26];
#ifdef _WIN32
    ::ctime_s(time_str, sizeof(time_str), &now);
#else
    ::ctime_r(&now, time_str);
#endif
    time_str[24] = 0;
    oss.set_caret(0);
    oss <<"[" <<time_str <<"] " <<fmt.get_file() <<':' <<fmt.get_line() <<" ## ";
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
