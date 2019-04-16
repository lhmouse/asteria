// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "utilities.hpp"
#ifdef _WIN32
#  include <windows.h>  // ::SYSTEMTIME, ::GetSystemTime()
#else
#  include <time.h>  // ::timespec, ::clock_gettime(), ::localtime()
#endif

namespace Asteria {

///////////////////////////////////////////////////////////////////////////////
// Formatter
///////////////////////////////////////////////////////////////////////////////

Formatter::~Formatter()
  {
  }

std::ostream& Formatter::do_open_stream()
  {
    auto& strm = this->m_strm_opt;
    if(!strm) {
      strm = rocket::make_unique<rocket::insertable_ostream>();
    }
    return *strm;
  }

bool are_debug_logs_enabled() noexcept
  {
#ifdef ASTERIA_ENABLE_DEBUG_LOGS
    return true;
#else
    return false;
#endif
  }

    namespace {

    inline void do_ltoa_fixed(rocket::cow_string& str, long num, unsigned width)
      {
        std::array<char, 64> sbuf;
        auto spos = sbuf.end();
        // Write digits from the right to the left.
        long reg = num;
        for(unsigned i = 0; i < width; ++i) {
          long d = reg % 10;
          reg /= 10;
          *--spos = static_cast<char>('0' + d);
        }
        // Append the formatted string.
        str.append(spos, sbuf.end());
      }

    template<typename ParamT> inline void do_append_str(rocket::cow_string& str, ParamT&& param)
      {
        str.append(rocket::forward<ParamT>(param));
      }
    inline void do_append_str(rocket::cow_string& str, char ch)
      {
        str.push_back(ch);
      }

    }  // namespace

bool write_log_to_stderr(const char* file, long line, rocket::cow_string&& msg) noexcept
  {
    rocket::cow_string str;
    str.reserve(1023);
    // Append the timestamp.
#ifdef _WIN32
    ::SYSTEMTIME st;
    ::GetSystemTime(&st);
    // YYYY-MM-DD hh:mm:ss.sss
    do_ltoa_fixed(str, st.wYear, 4);
    do_append_str(str, '-');
    do_ltoa_fixed(str, st.wMonth, 2);
    do_append_str(str, '-');
    do_ltoa_fixed(str, st.wDay, 2);
    do_append_str(str, ' ');
    do_ltoa_fixed(str, st.wHour, 2);
    do_append_str(str, ':');
    do_ltoa_fixed(str, st.wMinute, 2);
    do_append_str(str, ':');
    do_ltoa_fixed(str, st.wSecond, 2);
    do_append_str(str, '.');
    do_ltoa_fixed(str, st.wMillisecond, 3);
#else
    ::timespec ts;
    ::clock_gettime(CLOCK_REALTIME, &ts);
    ::tm tr;
    ::localtime_r(&(ts.tv_sec), &tr);
    // YYYY-MM-DD hh:mm:ss.sss
    do_ltoa_fixed(str, tr.tm_year + 1900, 4);
    do_append_str(str, '-');
    do_ltoa_fixed(str, tr.tm_mon + 1, 2);
    do_append_str(str, '-');
    do_ltoa_fixed(str, tr.tm_mday, 2);
    do_append_str(str, ' ');
    do_ltoa_fixed(str, tr.tm_hour, 2);
    do_append_str(str, ':');
    do_ltoa_fixed(str, tr.tm_min, 2);
    do_append_str(str, ':');
    do_ltoa_fixed(str, tr.tm_sec, 2);
    do_append_str(str, '.');
    do_ltoa_fixed(str, ts.tv_nsec / 1000000, 3);
#endif
    // Append the file name and line number.
    do_append_str(str, " @@ ");
    do_append_str(str, file);
    do_append_str(str, ':');
    do_ltoa_fixed(str, line, 1);
    // Start a new line for the user-defined message.
    do_append_str(str, "\n\t");
    // Neutralize control characters and indent paragraphs.
    for(char ch : msg) {
      // Control characters are ['\x00','\x20') and '\x7F'.
      static constexpr char s_replacements[][16] =
        {
          "[NUL"   "\\x00]",   "[SOH"   "\\x01]",   "[STX"   "\\x02]",   "[ETX"   "\\x03]",
          "[EOT"   "\\x04]",   "[ENQ"   "\\x05]",   "[ACK"   "\\x06]",   "[BEL"   "\\x07]",
          "[BS"    "\\x08]",   /*TAB*/  "\t",       /*LF*/   "\n\t",     "[VT"    "\\x0B]",
          "[FF"    "\\x0C]",   /*CR*/   "\r",       "[SO"    "\\x0E]",   "[SI"    "\\x0F]",
          "[DLE"   "\\x10]",   "[DC1"   "\\x11]",   "[DC2"   "\\x12]",   "[DC3"   "\\x13]",
          "[DC4"   "\\x14]",   "[NAK"   "\\x15]",   "[SYN"   "\\x16]",   "[ETB"   "\\x17]",
          "[CAN"   "\\x18]",   "[EM"    "\\x19]",   "[SUB"   "\\x1A]",   "[ESC"   "\\x1B]",
          "[FS"    "\\x1C]",   "[GS"    "\\x1D]",   "[RS"    "\\x1E]",   "[US"    "\\x1F]",
        };
      // Check one character.
      std::size_t uch = ch & 0xFF;
      if(uch == 0x7F) {
        do_append_str(str, "[DEL\\x7F]");
        continue;
      }
      if(uch < rocket::countof(s_replacements)) {
        do_append_str(str, s_replacements[uch]);
        continue;
      }
      do_append_str(str, ch);
    };
    // Terminate the message with a line feed.
    do_append_str(str, '\n');
    // Write the string now.
    int res = std::fputs(str.c_str(), stderr);
    return res >= 0;
  }

///////////////////////////////////////////////////////////////////////////////
// Runtime_Error
///////////////////////////////////////////////////////////////////////////////

Runtime_Error::~Runtime_Error()
  {
  }

bool throw_runtime_error(const char* func, rocket::cow_string&& msg)
  {
    // Append the function signature.
    msg << "\n[thrown from native function `" << func << "(...)`]";
    // Throw it.
    throw Runtime_Error(rocket::move(msg));
  }

///////////////////////////////////////////////////////////////////////////////
// Quote
///////////////////////////////////////////////////////////////////////////////

std::ostream& operator<<(std::ostream& os, const Quote& q)
  {
    const std::ostream::sentry sentry(os);
    if(!sentry) {
      return os;
    }
    auto state = std::ios_base::goodbit;
    try {
      auto eof = std::char_traits<char>::eof();
      // Insert the open quote mark.
      if(os.rdbuf()->sputc('\"') == eof) {
        state |= std::ios_base::failbit;
        goto z;
      }
      // Insert the string.
      auto rbase = q.data() + q.size();
      auto rem = q.size();
      while(rem > 0) {
        auto uch = static_cast<unsigned>(rbase[-rem] & 0xFF);
        switch(uch) {
        case '\a':
          {
            if(os.rdbuf()->sputn("\\a", 2) < 2) {
              state |= std::ios_base::failbit;
              goto z;
            }
            break;
          }
        case '\b':
          {
            if(os.rdbuf()->sputn("\\b", 2) < 2) {
              state |= std::ios_base::failbit;
              goto z;
            }
            break;
          }
        case '\f':
          {
            if(os.rdbuf()->sputn("\\f", 2) < 2) {
              state |= std::ios_base::failbit;
              goto z;
            }
            break;
          }
        case '\n':
          {
            if(os.rdbuf()->sputn("\\n", 2) < 2) {
              state |= std::ios_base::failbit;
              goto z;
            }
            break;
          }
        case '\r':
          {
            if(os.rdbuf()->sputn("\\r", 2) < 2) {
              state |= std::ios_base::failbit;
              goto z;
            }
            break;
          }
        case '\t':
          {
            if(os.rdbuf()->sputn("\\t", 2) < 2) {
              state |= std::ios_base::failbit;
              goto z;
            }
            break;
          }
        case '\v':
          {
            if(os.rdbuf()->sputn("\\v", 2) < 2) {
              state |= std::ios_base::failbit;
              goto z;
            }
            break;
          }
        case '\0':
          {
            if(os.rdbuf()->sputn("\\0", 2) < 2) {
              state |= std::ios_base::failbit;
              goto z;
            }
            break;
          }
        case '\x1A':
          {
            if(os.rdbuf()->sputn("\\Z", 2) < 2) {
              state |= std::ios_base::failbit;
              goto z;
            }
            break;
          }
        case '\x1B':
          {
            if(os.rdbuf()->sputn("\\e", 2) < 2) {
              state |= std::ios_base::failbit;
              goto z;
            }
            break;
          }
        case '\"':
        case '\'':
        case '\\':
          {
            if(os.rdbuf()->sputc('\\') == eof) {
              state |= std::ios_base::failbit;
              goto z;
            }
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
        case 0x78:  case 0x79:  case 0x7a:  case 0x7b:  case 0x7c:  case 0x7d:  case 0x7e:
          {
            // Write the character as is.
            if(os.rdbuf()->sputc(static_cast<char>(uch)) == eof) {
              state |= std::ios_base::failbit;
              goto z;
            }
            break;
          }
        default:
          {
            static constexpr char s_digits[] = "0123456789ABCDEF";
            char xseq[4] = { '\\', 'x', s_digits[(uch >> 4) & 0x0F], s_digits[uch & 0x0F] };
            if(os.rdbuf()->sputn(xseq, 4) < 4) {
              state |= std::ios_base::failbit;
              goto z;
            }
            break;
          }
        }
        --rem;
      }
      // Insert the closed quote mark.
      if(os.rdbuf()->sputc('\"') == eof) {
        state |= std::ios_base::failbit;
        goto z;
      }
    } catch(...) {
      rocket::handle_ios_exception(os, state);
    }
z:
    if(state) {
      os.setstate(state);
    }
    return os;
  }

///////////////////////////////////////////////////////////////////////////////
// Wrap Index
///////////////////////////////////////////////////////////////////////////////

Wrapped_Index wrap_index(std::int64_t index, std::size_t size) noexcept
  {
    ROCKET_ASSERT(size <= PTRDIFF_MAX);
    // The range of valid indices is (~size, size).
    Wrapped_Index w;
    auto ssize = static_cast<std::int64_t>(size);
    if(index >= 0) {
      // Append elements as needed.
      auto nappend = rocket::max(index, ssize - 1) - (ssize - 1);
      w.nprepend = 0;
      w.nappend = static_cast<std::uint64_t>(nappend);
      // `index` is truncated if it does not fit in `size_t`, but in this case it shouldn't be used.
      w.rindex = static_cast<std::size_t>(index);
    } else {
      // Prepend elements as needed.
      auto nprepend = rocket::max(index - 1, ~ssize) - (index - 1);
      w.nprepend = static_cast<std::uint64_t>(nprepend);
      w.nappend = 0;
      // `index + ssize` cannot overflow when `index` is negative and `ssize` is not.
      w.rindex = static_cast<std::size_t>(index + ssize) + static_cast<std::size_t>(nprepend);
    }
    return w;
  }

///////////////////////////////////////////////////////////////////////////////
// Random Seed
///////////////////////////////////////////////////////////////////////////////

std::uint64_t generate_random_seed() noexcept
  {
    // Get the system time of very high resolution.
    std::int64_t tp;
#ifdef _WIN32
    ::LARGE_INTEGER li;
    ::QueryPerformanceCounter(&li);
    tp = li.QuadPart;
#else
    ::timespec ts;
    ::clock_gettime(CLOCK_MONOTONIC, &ts);
    tp = static_cast<std::int64_t>(ts.tv_sec) * 1000000000 + ts.tv_nsec;
#endif
    // Hash it using FNV-1a to erase sensitive information.
    //   https://en.wikipedia.org/wiki/Fowler–Noll–Vo_hash_function
    // The timestamp is read in little-endian byte order.
    std::uint64_t seed = 0xCBF29CE484222325;
    for(int i = 0; i < 8; ++i) {
      auto byte = static_cast<unsigned char>(tp >> i * 8);
      seed = (seed ^ byte) * 0x100000001B3;
    }
    return seed;
  }

}  // namespace Asteria
