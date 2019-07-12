// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "utilities.hpp"
#include <iostream>
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
      strm = rocket::make_unique<rocket::cow_ostringstream>();
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

    constexpr char s_cntrl_reps[][16] =
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

    template<typename ParamT> inline void do_append_str(rocket::cow_string& str, ParamT&& param)
      {
        str.append(rocket::forward<ParamT>(param));
      }
    inline void do_append_str(rocket::cow_string& str, char c)
      {
        str.push_back(c);
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
    do_ltoa_fixed(str, static_cast<long>(ts.tv_nsec / 1000000), 3);
#endif
    // Append the file name and line number.
    do_append_str(str, " @@ ");
    do_append_str(str, file);
    do_append_str(str, ':');
    do_ltoa_fixed(str, line, 1);
    // Start a new line for the user-defined message.
    do_append_str(str, "\n\t");
    // Neutralize control characters and indent paragraphs.
    for(char c : msg) {
      // Control characters are ['\x00','\x1F'] and '\x7F'.
      std::size_t uch = c & 0xFF;
      if(uch == 0x7F) {
        do_append_str(str, "[DEL\\x7F]");
        continue;
      }
      if(uch <= 0x1F) {
        do_append_str(str, s_cntrl_reps[uch]);
        continue;
      }
      do_append_str(str, c);
    };
    // Terminate the message with a line feed.
    do_append_str(str, '\n');
    // Write the string now.
    std::cerr << str << std::flush;
    return !!std::cerr;
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

bool utf8_encode(char*& pos, char32_t cp)
  {
    if(cp < 0x80) {
      // This character takes only one byte.
      *(pos++) = static_cast<char>(cp);
      return true;
    }
    if((0xD800 <= cp) && (cp < 0xE000)) {
      // Surrogates are reserved for UTF-16.
      return false;
    }
    if(cp >= 0x110000) {
      // Code point is too large.
      return false;
    }
    // Encode bits into a byte.
    auto encode_one = [&](int sh, char32_t m)
      {
        *(pos++) = static_cast<char>((~m << 1) | ((cp >> sh) & m));
      };
    // Encode the code point now. The result may be 2, 3 or 4 bytes.
    if(cp < 0x800) {
      encode_one( 6, 0x1F);
      encode_one( 0, 0x3F);
      return true;
    }
    if(cp < 0x10000) {
      encode_one(12, 0x0F);
      encode_one( 6, 0x3F);
      encode_one( 0, 0x3F);
      return true;
    }
    encode_one(18, 0x07);
    encode_one(12, 0x3F);
    encode_one( 6, 0x3F);
    encode_one( 0, 0x3F);
    return true;
  }

bool utf8_encode(rocket::cow_string& text, char32_t cp)
  {
    char str[4];
    char* pos = str;
    // Encode the code point into this temporary buffer.
    if(!utf8_encode(pos, cp)) {
      return false;
    }
    // Append all bytes encoded.
    text.append(str, pos);
    return true;
  }

bool utf8_decode(char32_t& cp, const char*& pos, std::size_t avail)
  {
    if(avail == 0) {
      return false;
    }
    // Read the first byte.
    cp = *(pos++) & 0xFF;
    if(cp < 0x80) {
      // This sequence contains only one byte.
      return true;
    }
    if((cp < 0xC0) || (0xF8 <= cp)) {
      // This is not a leading character.
      return false;
    }
    // Calculate the number of bytes in this code point.
    auto u8len = static_cast<std::size_t>(2 + (cp >= 0xE0) + (cp >= 0xF0));
    ROCKET_ASSERT(u8len >= 2);
    ROCKET_ASSERT(u8len <= 4);
    if(u8len > avail) {
      // No enough characters have been provided.
      return false;
    }
    // Unset bits that are not part of the payload.
    cp &= UINT32_C(0xFF) >> u8len;
    // Accumulate trailing code units.
    for(std::size_t i = 1; i < u8len; ++i) {
      char32_t cu = *(pos++) & 0xFF;
      if((cu < 0x80) || (0xC0 <= cu)) {
        // This trailing character is not valid.
        return false;
      }
      cp = (cp << 6) | (cu & 0x3F);
    }
    if((0xD800 <= cp) && (cp < 0xE000)) {
      // Surrogates are reserved for UTF-16.
      return false;
    }
    if(cp >= 0x110000) {
      // Code point is too large.
      return false;
    }
    // Re-encode it and check for overlong sequences.
    auto milen = static_cast<std::size_t>(1 + (cp >= 0x80) + (cp >= 0x800) + (cp >= 0x10000));
    if(milen != u8len) {
      // Overlong sequences are not allowed.
      return false;
    }
    return true;
  }

bool utf8_decode(char32_t& cp, const rocket::cow_string& text, std::size_t& offset)
  {
    if(offset >= text.size()) {
      return false;
    }
    const char* pos = text.data() + offset;
    // Decode bytes.
    if(!utf8_decode(cp, pos, text.size() - offset)) {
      return false;
    }
    // Update the offset.
    offset = static_cast<std::size_t>(pos - text.data());
    return true;
  }

bool utf16_encode(char16_t*& pos, char32_t cp)
  {
    if((0xD800 <= cp) && (cp < 0xE000)) {
      // Surrogates are reserved for UTF-16.
      return false;
    }
    if(cp < 0x10000) {
      // This character takes only one code unit.
      *(pos++) = static_cast<char16_t>(cp);
      return true;
    }
    if(cp >= 0x110000) {
      // Code point is too large.
      return false;
    }
    // Write surrogates.
    *(pos++) = static_cast<char16_t>(0xD800 + ((cp - 0x10000) >> 10));
    *(pos++) = static_cast<char16_t>(0xDC00 + (cp & 0x3FF));
    return true;
  }

bool utf16_encode(rocket::cow_u16string& text, char32_t cp)
  {
    char16_t str[2];
    char16_t* pos = str;
    // Encode the code point into this temporary buffer.
    if(!utf16_encode(pos, cp)) {
      return false;
    }
    // Append all bytes encoded.
    text.append(str, pos);
    return true;
  }

bool utf16_decode(char32_t& cp, const char16_t*& pos, std::size_t avail)
  {
    if(avail == 0) {
      return false;
    }
    // Read the first code unit.
    cp = *(pos++) & 0xFFFF;
    if((cp < 0xD800) || (0xE000 <= cp)) {
      // This sequence contains only one code unit.
      return true;
    }
    if(cp >= 0xDC00) {
      // A trailing surrogate is not allowed unless following a leading surrogate.
      return false;
    }
    if(avail < 2) {
      // No enough code units have been provided.
      return false;
    }
    // Read the trailing surrogate.
    char16_t cu = *(pos++);
    if((cu < 0xDC00) || (0xE000 <= cu)) {
      // Only a trailing surrogate is allowed to follow a leading surrogate.
      return false;
    }
    cp = 0x10000 + ((cp & 0x3FF) << 10) + (cu & 0x3FF);
    return true;
  }

bool utf16_decode(char32_t& cp, const rocket::cow_u16string& text, std::size_t& offset)
  {
    if(offset >= text.size()) {
      return false;
    }
    const char16_t* pos = text.data() + offset;
    // Decode bytes.
    if(!utf16_decode(cp, pos, text.size() - offset)) {
      return false;
    }
    // Update the offset.
    offset = static_cast<std::size_t>(pos - text.data());
    return true;
  }

    namespace {

    constexpr char s_quote_table[][5] =
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

    }

rocket::cow_string& quote(rocket::cow_string& sbuf, const char* str, std::size_t len)
  {
    sbuf.clear();
    // Enclose the string with double quotes, escaping characters as needed.
    sbuf += '\"';
    std::for_each(str, str + len, [&](char c) { sbuf += s_quote_table[(c & 0xFF)];  });
    sbuf += '\"';
    return sbuf;
  }

rocket::cow_string quote(const char* str, std::size_t len)
  {
    rocket::cow_string sbuf;
    quote(sbuf, str, len);
    return sbuf;
  }

rocket::cow_string& quote(rocket::cow_string& sbuf, const char* str)
  {
    return quote(sbuf, str, std::strlen(str));
  }

rocket::cow_string quote(const char* str)
  {
    return quote(str, std::strlen(str));
  }

rocket::cow_string& quote(rocket::cow_string& sbuf, const rocket::cow_string& str)
  {
    return quote(sbuf, str.data(), str.size());
  }

rocket::cow_string quote(const rocket::cow_string& str)
  {
    return quote(str.data(), str.size());
  }

rocket::cow_string& pwrapln(rocket::cow_string& sbuf, std::size_t indent, std::size_t hanging)
  {
    sbuf.clear();
    if(indent != 0) {
      // Terminate the current line.
      sbuf = rocket::sref("\n");
      if(hanging != 0) {
        // Indent the next line accordingly.
        sbuf.append(hanging, ' ');
      }
    }
    else {
      // Write everything in a single line, separated by spaces.
      sbuf = rocket::sref(" ");
    }
    return sbuf;
  }

rocket::cow_string pwrapln(std::size_t indent, std::size_t hanging)
  {
    rocket::cow_string sbuf;
    pwrapln(sbuf, indent, hanging);
    return sbuf;
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
    }
    else {
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
