// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "utilities.hpp"
#include <iostream> // std::cerr
#include <cstdio> // std::snprintf()

#ifdef _WIN32
#  include <windows.h> // ::SYSTEMTIME, ::GetSystemTime()
#else
#  include <time.h> // ::timespec, ::clock_gettime(), ::localtime()
#endif

namespace Asteria {

///////////////////////////////////////////////////////////////////////////////
// Formatter
///////////////////////////////////////////////////////////////////////////////

Formatter::Formatter()
  : m_stream()
  {
    this->m_stream.setf(std::ios::boolalpha);
  }
Formatter::~Formatter()
  {
  }

void Formatter::do_put(bool value)
  {
    this->m_stream <<value;
  }
void Formatter::do_put(char value)
  {
    this->m_stream <<value;
  }
void Formatter::do_put(signed char value)
  {
    this->m_stream <<static_cast<int>(value);
  }
void Formatter::do_put(unsigned char value)
  {
    this->m_stream <<static_cast<unsigned>(value);
  }
void Formatter::do_put(short value)
  {
    this->m_stream <<static_cast<int>(value);
  }
void Formatter::do_put(unsigned short value)
  {
    this->m_stream <<static_cast<unsigned>(value);
  }
void Formatter::do_put(int value)
  {
    this->m_stream <<value;
  }
void Formatter::do_put(unsigned value)
  {
    this->m_stream <<value;
  }
void Formatter::do_put(long value)
  {
    this->m_stream <<value;
  }
void Formatter::do_put(unsigned long value)
  {
    this->m_stream <<value;
  }
void Formatter::do_put(long long value)
  {
    this->m_stream <<value;
  }
void Formatter::do_put(unsigned long long value)
  {
    this->m_stream <<value;
  }
void Formatter::do_put(const char *value)
  {
    this->m_stream <<value;
  }
void Formatter::do_put(const signed char *value)
  {
    this->m_stream <<static_cast<const void *>(value);
  }
void Formatter::do_put(const unsigned char *value)
  {
    this->m_stream <<static_cast<const void *>(value);
  }
void Formatter::do_put(const void *value)
  {
    this->m_stream <<value;
  }

namespace {

  int do_print_time(char *str, std::size_t cap)
    {
      int len;
#ifdef _WIN32
      ::SYSTEMTIME s;
      ::GetSystemTime(&s);
      len = std::snprintf(str, cap, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
                          s.wYear, s.wMonth, s.wDay, s.wHour, s.wMinute, s.wSecond, s.wMilliseconds);
#else
      ::timespec t;
      int err = ::clock_gettime(CLOCK_REALTIME, &t);
      ROCKET_ASSERT(err == 0);
      ::tm s;
      ::localtime_r(&(t.tv_sec), &s);
      len = std::snprintf(str, cap, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
                          s.tm_year + 1900, s.tm_mon, s.tm_mday, s.tm_hour, s.tm_min, s.tm_sec, static_cast<int>(t.tv_nsec / 1000000));
#endif
      return len;
    }

  void do_replace_all(rocket::cow_string &str, char ch, const char *reps)
    {
      const auto repn = std::char_traits<char>::length(reps);
      auto pos = static_cast<rocket::cow_string::size_type>(0);
      do {
        pos = str.find(ch, pos);
        if(pos == str.npos) {
          break;
        }
        str.replace(pos, 1, reps, repn);
        pos += repn;
      } while(true);
    }

}

bool are_debug_logs_enabled() noexcept
  {
#ifdef ASTERIA_ENABLE_DEBUG_LOGS
    return true;
#else
    return false;
#endif
  }

bool write_log_to_stderr(const char *file, unsigned long line, Formatter &&fmt) noexcept
  try {
    auto &oss = fmt.get_stream();
    oss.set_caret(0);
    char time_str[64];
    do_print_time(time_str, sizeof(time_str));
    oss <<time_str <<" $$ ";
    oss.set_caret(oss.npos);
    oss <<" @@ " <<file <<':' <<line;
    auto str = oss.extract_string();
    do_replace_all(str, '\n', "\n\t");
    do_replace_all(str, '\0', "[NUL]");
    str.push_back('\n');
    std::cerr <<str <<std::flush;
    return true;
  } catch(...) {
    return false;
  }

Runtime_error::~Runtime_error()
  {
  }

const char * Runtime_error::what() const noexcept
  {
    return this->m_msg.c_str();
  }

[[noreturn]] void throw_runtime_error(const char *funcsig, Formatter &&fmt)
  {
    auto &oss = fmt.get_stream();
    oss.set_caret(0);
    oss <<funcsig <<": ";
    auto str = oss.extract_string();
    throw Runtime_error(std::move(str));
  }

///////////////////////////////////////////////////////////////////////////////
// Indent
///////////////////////////////////////////////////////////////////////////////

std::ostream & operator<<(std::ostream &os, const Indent &n)
  {
    const std::ostream::sentry sentry(os);
    if(!sentry) {
      return os;
    }
    auto state = std::ios_base::goodbit;
    try {
      // Optimize stream insertion a little.
      std::array<char, 256> buf;
      const auto fc = os.fill();
      buf.fill(fc);
      buf[0] = n.head();
      std::size_t nb = 1;
      // Write chunks.
      std::size_t rem = n.count();
      for(;;) {
        const auto nq = rocket::min(rem, buf.size() - nb);
        nb += nq;
        rem -= nq;
        if(os.rdbuf()->sputn(buf.data(), static_cast<long>(nb)) != static_cast<long>(nb)) {
          state |= std::ios_base::failbit;
          break;
        }
        if(rem == 0) {
          break;
        }
        nb = 0;
        buf[0] = fc;
      }
    } catch(...) {
      rocket::handle_ios_exception(os, state);
    }
    if(state) {
      os.setstate(state);
    }
    return os;
  }

///////////////////////////////////////////////////////////////////////////////
// Quote
///////////////////////////////////////////////////////////////////////////////

std::ostream & operator<<(std::ostream &os, const Quote &q)
  {
    const std::ostream::sentry sentry(os);
    if(!sentry) {
      return os;
    }
    auto state = std::ios_base::goodbit;
    try {
      // Optimize stream insertion a little.
      std::array<char, 272> buf;
      buf[0] = '\"';
      std::size_t nb = 1;
      // Write chunks.
      std::size_t rem = q.size();
      for(;;) {
        if((rem == 0) || (nb >= buf.size() - 16)) {
          if(rem == 0) {
            buf[nb++] = '\"';
          }
          if(os.rdbuf()->sputn(buf.data(), static_cast<long>(nb)) != static_cast<long>(nb)) {
            state |= std::ios_base::failbit;
            break;
          }
          if(rem == 0) {
            break;
          }
          nb = 0;
        }
        const int ch = *(q.data() + q.size() - rem);
        switch(ch) {
          case '\a': {
            buf[nb++] = '\\';
            buf[nb++] = 'a';
            break;
          }
          case '\b': {
            buf[nb++] = '\\';
            buf[nb++] = 'b';
            break;
          }
          case '\f': {
            buf[nb++] = '\\';
            buf[nb++] = 'f';
            break;
          }
          case '\n': {
            buf[nb++] = '\\';
            buf[nb++] = 'n';
            break;
          }
          case '\r': {
            buf[nb++] = '\\';
            buf[nb++] = 'r';
            break;
          }
          case '\t': {
            buf[nb++] = '\\';
            buf[nb++] = 't';
            break;
          }
          case '\v': {
            buf[nb++] = '\\';
            buf[nb++] = 'v';
            break;
          }
          case '\0': {
            buf[nb++] = '\\';
            buf[nb++] = '0';
            break;
          }
          case '\x1A': {
            buf[nb++] = '\\';
            buf[nb++] = 'Z';
            break;
          }
          case '\x1B': {
            buf[nb++] = '\\';
            buf[nb++] = 'e';
            break;
          }
          case '\"':  case '\'':  case '\\': {
            buf[nb++] = '\\';
          }
          // Fallthrough.
          case 0x20:  case 0x21:  /*  "  */   case 0x23:  case 0x24:  case 0x25:  case 0x26:  /*  '  */
          case 0x28:  case 0x29:  case 0x2a:  case 0x2b:  case 0x2c:  case 0x2d:  case 0x2e:  case 0x2f:
          case 0x30:  case 0x31:  case 0x32:  case 0x33:  case 0x34:  case 0x35:  case 0x36:  case 0x37:
          case 0x38:  case 0x39:  case 0x3a:  case 0x3b:  case 0x3c:  case 0x3d:  case 0x3e:  case 0x3f:
          case 0x40:  case 0x41:  case 0x42:  case 0x43:  case 0x44:  case 0x45:  case 0x46:  case 0x47:
          case 0x48:  case 0x49:  case 0x4a:  case 0x4b:  case 0x4c:  case 0x4d:  case 0x4e:  case 0x4f:
          case 0x50:  case 0x51:  case 0x52:  case 0x53:  case 0x54:  case 0x55:  case 0x56:  case 0x57:
          case 0x58:  case 0x59:  case 0x5a:  case 0x5b:  /*  \  */   case 0x5d:  case 0x5e:  case 0x5f:
          case 0x60:  case 0x61:  case 0x62:  case 0x63:  case 0x64:  case 0x65:  case 0x66:  case 0x67:
          case 0x68:  case 0x69:  case 0x6a:  case 0x6b:  case 0x6c:  case 0x6d:  case 0x6e:  case 0x6f:
          case 0x70:  case 0x71:  case 0x72:  case 0x73:  case 0x74:  case 0x75:  case 0x76:  case 0x77:
          case 0x78:  case 0x79:  case 0x7a:  case 0x7b:  case 0x7c:  case 0x7d:  case 0x7e: {
            buf[nb++] = static_cast<char>(ch);
            break;
          }
          default: {
            static constexpr char s_digits[] = "0123456789ABCDEF";
            buf[nb++] = '\\';
            buf[nb++] = 'x';
            buf[nb++] = s_digits[(ch >> 4) & 0x0F];
            buf[nb++] = s_digits[(ch >> 0) & 0x0F];
            break;
          }
        }
        rem -= 1;
      }
    } catch(...) {
      rocket::handle_ios_exception(os, state);
    }
    if(state) {
      os.setstate(state);
    }
    return os;
  }

}
