// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "utilities.hpp"
#include <iostream>  // std::cerr

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

std::ostream & Formatter::do_open_stream()
  {
    auto &strm = this->m_strm_opt;
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

bool write_log_to_stderr(const char *file, long line, rocket::cow_string &&msg) noexcept
  {
    // Behaves like an UnformattedOutputFunction.
    auto &os = std::cerr;
    const std::ostream::sentry sentry(os);
    if(!sentry) {
      return false;
    }
    auto state = std::ios_base::goodbit;
    try {
      const auto eof = std::char_traits<char>::eof();
      char tstr[64];
      long tlen = 0;
      // This will be faster than `printf()`.
      const auto print_0ld = [&](long width, long num)
        {
          static constexpr char s_digits[] = "0123456789";
          // Write digits from the right to the left.
          auto reg = num;
          for(long i = 0; i < width; ++i) {
            const auto d = reg % 10;
            reg /= 10;
            tstr[tlen + width + ~i] = s_digits[d];
          }
          // Don't forget to update `tlen`.
          tlen += width;
        };
      // Prepend the timestamp.
#ifdef _WIN32
      ::SYSTEMTIME st;
      ::GetSystemTime(&st);
      // YYYY-MM-DD hh:mm:ss.sss
      print_0ld(4, st.wYear);
      tstr[tlen++] = '-';
      print_0ld(2, st.wMonth);
      tstr[tlen++] = '-';
      print_0ld(2, st.wDay);
      tstr[tlen++] = ' ';
      print_0ld(2, st.wHour);
      tstr[tlen++] = ':';
      print_0ld(2, st.wMinute);
      tstr[tlen++] = ':';
      print_0ld(2, st.wSecond);
      tstr[tlen++] = '.';
      print_0ld(3, st.wMilliseconds);
#else
      ::timespec ts;
      ::clock_gettime(CLOCK_REALTIME, &ts);
      ::tm tr;
      ::localtime_r(&(ts.tv_sec), &tr);
      // YYYY-MM-DD hh:mm:ss.sss
      print_0ld(4, tr.tm_year + 1900);
      tstr[tlen++] = '-';
      print_0ld(2, tr.tm_mon + 1);
      tstr[tlen++] = '-';
      print_0ld(2, tr.tm_mday);
      tstr[tlen++] = ' ';
      print_0ld(2, tr.tm_hour);
      tstr[tlen++] = ':';
      print_0ld(2, tr.tm_min);
      tstr[tlen++] = ':';
      print_0ld(2, tr.tm_sec);
      tstr[tlen++] = '.';
      print_0ld(3, ts.tv_nsec / 1000000);
#endif
      if(os.rdbuf()->sputn(tstr, tlen) < tlen) {
        state |= std::ios_base::failbit;
        goto z;
      }
      // Insert the file name and line number.
      if(os.rdbuf()->sputn(" @@ ", 4) < 4) {
        state |= std::ios_base::failbit;
        goto z;
      }
      tlen = static_cast<long>(std::strlen(file));
      if(os.rdbuf()->sputn(file, tlen) < tlen) {
        state |= std::ios_base::failbit;
        goto z;
      }
      if(os.rdbuf()->sputc(':') == eof) {
        state |= std::ios_base::failbit;
        goto z;
      }
      tlen = 0;
      print_0ld(1, line);
      if(os.rdbuf()->sputn(tstr, tlen) < tlen) {
        state |= std::ios_base::failbit;
        goto z;
      }
      // Start a new line for the user-defined message.
      if(os.rdbuf()->sputn("\n\t", 2) < 2) {
        state |= std::ios_base::failbit;
        goto z;
      }
      // Neutralize control characters and indent paragraphs.
      for(auto it = msg.begin(); it != msg.end(); ++it) {
        // Control characters are ['\x00','\x20') and '\x7F'.
        static constexpr char s_replacements[][16] =
          {
            "[NUL"   "\\x00]",
            "[SOH"   "\\x01]",
            "[STX"   "\\x02]",
            "[ETX"   "\\x03]",
            "[EOT"   "\\x04]",
            "[ENQ"   "\\x05]",
            "[ACK"   "\\x06]",
            "[BEL"   "\\x07]",
            "[BS"    "\\x08]",
            /*TAB*/  "\t",
            /*LF*/   "\n\t",
            "[VT"    "\\x0B]",
            "[FF"    "\\x0C]",
            /*CR*/   "\r",
            "[SO"    "\\x0E]",
            "[SI"    "\\x0F]",
            "[DLE"   "\\x10]",
            "[DC1"   "\\x11]",
            "[DC2"   "\\x12]",
            "[DC3"   "\\x13]",
            "[DC4"   "\\x14]",
            "[NAK"   "\\x15]",
            "[SYN"   "\\x16]",
            "[ETB"   "\\x17]",
            "[CAN"   "\\x18]",
            "[EM"    "\\x19]",
            "[SUB"   "\\x1A]",
            "[ESC"   "\\x1B]",
            "[FS"    "\\x1C]",
            "[GS"    "\\x1D]",
            "[RS"    "\\x1E]",
            "[US"    "\\x1F]",
          };
        // Check one character.
        const auto uch = static_cast<unsigned>(*it & 0xFF);
        if(uch == 0x7F) {
          const auto srep = "[DEL\\x7F]";
          tlen = static_cast<long>(std::strlen(srep));
          if(os.rdbuf()->sputn(srep, tlen) < tlen) {
            state |= std::ios_base::failbit;
            goto z;
          }
          continue;
        }
        if(uch < rocket::countof(s_replacements)) {
          const auto srep = s_replacements[uch];
          tlen = static_cast<long>(std::strlen(srep));
          if(os.rdbuf()->sputn(srep, tlen) < tlen) {
            state |= std::ios_base::failbit;
            goto z;
          }
          continue;
        }
        if(os.rdbuf()->sputc(static_cast<char>(uch)) == eof) {
          state |= std::ios_base::failbit;
          goto z;
        }
      }
      // Terminate the message with a line feed.
      if(os.rdbuf()->sputc('\n') == eof) {
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
    return !!os;
  }

///////////////////////////////////////////////////////////////////////////////
// Runtime_Error
///////////////////////////////////////////////////////////////////////////////

Runtime_Error::~Runtime_Error()
  {
  }

bool throw_runtime_error(const char *funcsig, rocket::cow_string &&msg)
  {
    // Append the function signature.
    msg << "\n[thrown from `" << funcsig << "`]";
    // Throw it.
    throw Runtime_Error(std::move(msg));
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
      const auto eof = std::char_traits<char>::eof();
      // Insert the head character.
      const auto head = n.head();
      if(os.rdbuf()->sputc(head) == eof) {
        state |= std::ios_base::failbit;
        goto z;
      }
      // Insert filling characters.
      const auto fill = os.fill();
      auto rem = n.count();
      while(rem > 0) {
        if(os.rdbuf()->sputc(fill) == eof) {
          state |= std::ios_base::failbit;
          goto z;
        }
        --rem;
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
      const auto eof = std::char_traits<char>::eof();
      // Insert the open quote mark.
      if(os.rdbuf()->sputc('\"') == eof) {
        state |= std::ios_base::failbit;
        goto z;
      }
      // Insert the string.
      const auto rbase = q.data() + q.size();
      auto rem = q.size();
      while(rem > 0) {
        const auto uch = static_cast<unsigned>(rbase[-rem] & 0xFF);
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
            const char xseq[4] = { '\\', 'x', s_digits[(uch >> 4) & 0x0F], s_digits[uch & 0x0F] };
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
// Wrappable Index
///////////////////////////////////////////////////////////////////////////////

Wrapped_Subscript wrap_subscript(std::int64_t index, std::size_t size)
  {
    const auto rsize = static_cast<std::int64_t>(size);
    // Wrap `index` if it is negative.
    // N.B. The result may still be negative.
    auto rindex = index;
    if(rindex < 0) {
      rindex += rsize;
    }
    Wrapped_Subscript wrap = { static_cast<std::size_t>(rindex), 0, 0 };
    // If `rindex` is still negative, we will have to insert elements in the front.
    if(rindex < 0) {
      // Calculate the number of elements to fill.
      const auto front_fill = 0 - static_cast<std::uint64_t>(rindex);
      if(front_fill > PTRDIFF_MAX) {
        rocket::throw_length_error("wrap_subscript: The subscript `%lld` is invalid.", static_cast<long long>(index));
      }
      wrap.front_fill = static_cast<std::size_t>(front_fill);
      return wrap;
    }
    // If `rindex` is greater than or equal to the size, we will have to insert elements in the back.
    if(rindex >= rsize) {
      // Calculate the number of elements to fill.
      const auto back_fill = static_cast<std::uint64_t>(rindex - rsize) + 1;
      if(back_fill > PTRDIFF_MAX) {
        rocket::throw_length_error("wrap_subscript: The subscript `%lld` is invalid.", static_cast<long long>(index));
      }
      wrap.back_fill = static_cast<std::size_t>(back_fill);
      return wrap;
    }
    // `rindex` fits in range.
    return wrap;
  }

}
