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
    do_ltoa_fixed(str, st.wMilliseconds, 3);
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
          "[NUL\\x00]",  "[SOH\\x01]",  "[STX\\x02]",  "[ETX\\x03]",
          "[EOT\\x04]",  "[ENQ\\x05]",  "[ACK\\x06]",  "[BEL\\x07]",
          "[BS\\x08]",   /*TAB*/"\t",   /*LF*/"\n\t",  "[VT\\x0B]",
          "[FF\\x0C]",   /*CR*/"\r",    "[SO\\x0E]",   "[SI\\x0F]",
          "[DLE\\x10]",  "[DC1\\x11]",  "[DC2\\x12]",  "[DC3\\x13]",
          "[DC4\\x14]",  "[NAK\\x15]",  "[SYN\\x16]",  "[ETB\\x17]",
          "[CAN\\x18]",  "[EM\\x19]",   "[SUB\\x1A]",  "[ESC\\x1B]",
          "[FS\\x1C]",   "[GS\\x1D]",   "[RS\\x1E]",   "[US\\x1F]",
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

void quote(rocket::cow_string& output, const char* data, std::size_t size)
  {
    do_append_str(output, '\"');
    // Append characters in `[data,data+size)`
    for(std::size_t i = 0; i != size; ++i) {
      // Use a lookup table?
      static constexpr char s_replacements[][256] =
        {
          "\\0",    "\\x01",  "\\x02",  "\\x03",  "\\x04",  "\\x05",  "\\x06",  "\\a",
          "\\b",    "\\t",    "\\n",    "\\v",    "\\f",    "\\r",    "\\x0E",  "\\x0F",
          "\\x10",  "\\x11",  "\\x12",  "\\x13",  "\\x14",  "\\x15",  "\\x16",  "\\x17",
          "\\x18",  "\\x19",  "\\Z",    "\\e",    "\\x1C",  "\\x1D",  "\\x1E",  "\\x1F",
          " ",      "!",      "\\\"",   "#",      "$",      "%",      "&",      "\\\'",
          "(",      ")",      "*",      "+",      ",",      "-",      ".",      "/",
          "0",      "1",      "2",      "3",      "4",      "5",      "6",      "7",
          "8",      "9",      ":",      ";",      "<",      "=",      ">",      "?",
          "@",      "A" ,     "B",      "C" ,     "D",      "E",      "F" ,     "G",
          "H",      "I",      "J",      "K",      "L",      "M",      "N",      "O",
          "P",      "Q",      "R",      "S",      "T",      "U",      "V",      "W",
          "X",      "Y",      "Z",      "[",      "\\\\",   "]",      "^",      "_",
          "`",      "a",      "b",      "c",      "d",      "e",      "f",      "g",
          "h",      "i" ,     "j",      "k" ,     "l",      "m",      "n" ,     "o",
          "p",      "q",      "r",      "s",      "t",      "u",      "v",      "w",
          "x",      "y",      "z",      "{",      "|",      "}",      "~",      "\\x7F",
          "\\x80",  "\\x81",  "\\x82",  "\\x83",  "\\x84",  "\\x85",  "\\x86",  "\\x87",
          "\\x88",  "\\x89",  "\\x8A",  "\\x8B",  "\\x8C",  "\\x8D",  "\\x8E",  "\\x8F",
          "\\x90",  "\\x91",  "\\x92",  "\\x93",  "\\x94",  "\\x95",  "\\x96",  "\\x97",
          "\\x98",  "\\x99",  "\\x9A",  "\\x9B",  "\\x9C",  "\\x9D",  "\\x9E",  "\\x9F",
          "\\xA0",  "\\xA1",  "\\xA2",  "\\xA3",  "\\xA4",  "\\xA5",  "\\xA6",  "\\xA7",
          "\\xA8",  "\\xA9",  "\\xAA",  "\\xAB",  "\\xAC",  "\\xAD",  "\\xAE",  "\\xAF",
          "\\xB0",  "\\xB1",  "\\xB2",  "\\xB3",  "\\xB4",  "\\xB5",  "\\xB6",  "\\xB7",
          "\\xB8",  "\\xB9",  "\\xBA",  "\\xBB",  "\\xBC",  "\\xBD",  "\\xBE",  "\\xBF",
          "\\xC0",  "\\xC1",  "\\xC2",  "\\xC3",  "\\xC4",  "\\xC5",  "\\xC6",  "\\xC7",
          "\\xC8",  "\\xC9",  "\\xCA",  "\\xCB",  "\\xCC",  "\\xCD",  "\\xCE",  "\\xCF",
          "\\xD0",  "\\xD1",  "\\xD2",  "\\xD3",  "\\xD4",  "\\xD5",  "\\xD6",  "\\xD7",
          "\\xD8",  "\\xD9",  "\\xDA",  "\\xDB",  "\\xDC",  "\\xDD",  "\\xDE",  "\\xDF",
          "\\xE0",  "\\xE1",  "\\xE2",  "\\xE3",  "\\xE4",  "\\xE5",  "\\xE6",  "\\xE7",
          "\\xE8",  "\\xE9",  "\\xEA",  "\\xEB",  "\\xEC",  "\\xED",  "\\xEE",  "\\xEF",
          "\\xF0",  "\\xF1",  "\\xF2",  "\\xF3",  "\\xF4",  "\\xF5",  "\\xF6",  "\\xF7",
          "\\xF8",  "\\xF9",  "\\xFA",  "\\xFB",  "\\xFC",  "\\xFD",  "\\xFE",  "\\xFF",
        };
      std::size_t uch = data[i] & 0xFF;
      do_append_str(output, s_replacements[uch]);
    }
    do_append_str(output, '\"');
  }

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
