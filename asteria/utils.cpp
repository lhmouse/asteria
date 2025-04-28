// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#include "xprecompiled.hpp"
#include "utils.hpp"
#include <time.h>  // ::timespec, ::clock_gettime(), ::localtime()
#include <unistd.h>  // ::write
#include <openssl/rand.h>
namespace asteria {
namespace {

const char s_char_escapes[][5] =
  {
    "\\0",   "\\x01", "\\x02", "\\x03", "\\x04", "\\x05", "\\x06", "\\a",
    "\\b",   "\\t",   "\\n",   "\\v",   "\\f",   "\\r",   "\\x0E", "\\x0F",
    "\\x10", "\\x11", "\\x12", "\\x13", "\\x14", "\\x15", "\\x16", "\\x17",
    "\\x18", "\\x19", "\\Z",   "\\e",   "\\x1C", "\\x1D", "\\x1E", "\\x1F",
    " ",     "!",     "\\\"",  "#",     "$",     "%",     "&",     "\\'",
    "(",     ")",     "*",     "+",     ",",     "-",     ".",     "/",
    "0",     "1",     "2",     "3",     "4",     "5",     "6",     "7",
    "8",     "9",     ":",     ";",     "<",     "=",     ">",     "?",
    "@",     "A",     "B",     "C",     "D",     "E",     "F",     "G",
    "H",     "I",     "J",     "K",     "L",     "M",     "N",     "O",
    "P",     "Q",     "R",     "S",     "T",     "U",     "V",     "W",
    "X",     "Y",     "Z",     "[",     "\\\\",  "]",     "^",     "_",
    "`",     "a",     "b",     "c",     "d",     "e",     "f",     "g",
    "h",     "i",     "j",     "k",     "l",     "m",     "n",     "o",
    "p",     "q",     "r",     "s",     "t",     "u",     "v",     "w",
    "x",     "y",     "z",     "{",     "|",     "}",     "~",     "\\x7F",
    "\\x80", "\\x81", "\\x82", "\\x83", "\\x84", "\\x85", "\\x86", "\\x87",
    "\\x88", "\\x89", "\\x8A", "\\x8B", "\\x8C", "\\x8D", "\\x8E", "\\x8F",
    "\\x90", "\\x91", "\\x92", "\\x93", "\\x94", "\\x95", "\\x96", "\\x97",
    "\\x98", "\\x99", "\\x9A", "\\x9B", "\\x9C", "\\x9D", "\\x9E", "\\x9F",
    "\\xA0", "\\xA1", "\\xA2", "\\xA3", "\\xA4", "\\xA5", "\\xA6", "\\xA7",
    "\\xA8", "\\xA9", "\\xAA", "\\xAB", "\\xAC", "\\xAD", "\\xAE", "\\xAF",
    "\\xB0", "\\xB1", "\\xB2", "\\xB3", "\\xB4", "\\xB5", "\\xB6", "\\xB7",
    "\\xB8", "\\xB9", "\\xBA", "\\xBB", "\\xBC", "\\xBD", "\\xBE", "\\xBF",
    "\\xC0", "\\xC1", "\\xC2", "\\xC3", "\\xC4", "\\xC5", "\\xC6", "\\xC7",
    "\\xC8", "\\xC9", "\\xCA", "\\xCB", "\\xCC", "\\xCD", "\\xCE", "\\xCF",
    "\\xD0", "\\xD1", "\\xD2", "\\xD3", "\\xD4", "\\xD5", "\\xD6", "\\xD7",
    "\\xD8", "\\xD9", "\\xDA", "\\xDB", "\\xDC", "\\xDD", "\\xDE", "\\xDF",
    "\\xE0", "\\xE1", "\\xE2", "\\xE3", "\\xE4", "\\xE5", "\\xE6", "\\xE7",
    "\\xE8", "\\xE9", "\\xEA", "\\xEB", "\\xEC", "\\xED", "\\xEE", "\\xEF",
    "\\xF0", "\\xF1", "\\xF2", "\\xF3", "\\xF4", "\\xF5", "\\xF6", "\\xF7",
    "\\xF8", "\\xF9", "\\xFA", "\\xFB", "\\xFC", "\\xFD", "\\xFE", "\\xFF",
  };

}  // namespace

ptrdiff_t
write_log_to_stderr(const char* file, long line, const char* func, cow_string&& msg)
  {
    // Compose the string to write.
    cow_string data;
    data.reserve(2047);

    // Write the timestamp and tag for sorting.
    ::timespec ts;
    ::clock_gettime(CLOCK_REALTIME, &ts);
    ::tm tr;
    ::localtime_r(&(ts.tv_sec), &tr);

    ::rocket::ascii_numput nump;
    uint64_t datetime = (uint32_t) tr.tm_year + 1900;
    datetime *= 100;
    datetime += (uint32_t) tr.tm_mon + 1;
    datetime *= 100;
    datetime += (uint32_t) tr.tm_mday;
    datetime *= 100;
    datetime += (uint32_t) tr.tm_hour;
    datetime *= 100;
    datetime += (uint32_t) tr.tm_min;
    datetime *= 100;
    datetime += (uint32_t) tr.tm_sec;
    nump.put_DU(datetime);

    data.append(nump.data() +  0, 4);
    data.push_back('-');
    data.append(nump.data() +  4, 2);
    data.push_back('-');
    data.append(nump.data() +  6, 2);
    data.push_back(' ');
    data.append(nump.data() +  8, 2);
    data.push_back(':');
    data.append(nump.data() + 10, 2);
    data.push_back(':');
    data.append(nump.data() + 12, 2);

    nump.put_DU((uint32_t) ts.tv_nsec, 9);
    data.push_back('.');
    data.append(nump.data(), 9);
    data.push_back(' ');

    // Append the function name and source location, followed by a line feed.
    data += "(";
    data += func;
    data += ") @ ";
    data += file;
    data += ':';
    nump.put_DU((unsigned long) line);
    data.append(nump.data(), nump.size());
    data += "\x1B\x45\t";

    // Neutralize control characters: ['\x00','\x1F'] and '\x7F'.
    for(char c : msg)
      switch(c)
        {
        case '\x00':  data += "[NUL]";       break;
        case '\x01':  data += "[SOH]";       break;
        case '\x02':  data += "[STX]";       break;
        case '\x03':  data += "[ETX]";       break;
        case '\x04':  data += "[EOT]";       break;
        case '\x05':  data += "[ENQ]";       break;
        case '\x06':  data += "[ACK]";       break;
        case '\x07':  data += "[BEL]";       break;
        case '\x08':  data += "[BS]";        break;
        case '\x09':  data += "\t";          break;
        case '\x0A':  data += "\x1B\x45\t";  break;
        case '\x0B':  data += "[VT]";        break;
        case '\x0C':  data += "[FF]";        break;
        case '\x0D':                         break;
        case '\x0E':  data += "[SO]";        break;
        case '\x0F':  data += "[SI]";        break;
        case '\x10':  data += "[DLE]";       break;
        case '\x11':  data += "[DC1]";       break;
        case '\x12':  data += "[DC2]";       break;
        case '\x13':  data += "[DC3]";       break;
        case '\x14':  data += "[DC4]";       break;
        case '\x15':  data += "[NAK]";       break;
        case '\x16':  data += "[SYN]";       break;
        case '\x17':  data += "[ETB]";       break;
        case '\x18':  data += "[CAN]";       break;
        case '\x19':  data += "[EM]";        break;
        case '\x1A':  data += "[SUB]";       break;
        case '\x1B':  data += "[ESC]";       break;
        case '\x1C':  data += "[FS]";        break;
        case '\x1D':  data += "[GS]";        break;
        case '\x1E':  data += "[RS]";        break;
        case '\x1F':  data += "[US]";        break;
        case '\x7F':  data += "[DEL]";       break;
        default:      data += c;             break;
        }

    // Remove trailing space characters.
    data.erase(data.rfind_not_of(" \f\n\r\t\v") + 1);

    // Finalize the message with a 'true' line terminator.
    data += "\x1B\x45\n";

    // Write the string now. Errors are ignored.
    return ::write(STDERR_FILENO, data.data(), data.size());
  }

void
throw_runtime_error(const char* file, long line, const char* func, cow_string&& msg)
  {
    // Compose the string to throw.
    cow_string data;
    data.reserve(2047);

    // Append the user-provided exception message.
    data.append(msg.begin(), msg.end());

    // Remove trailing space characters.
    data.erase(data.rfind_not_of(" \f\n\r\t\v") + 1);

    // Append the source location.
    data += "\n[thrown from `";
    data += func;
    data += "(...)` at '";
    data += file;
    data += ':';
    ::rocket::ascii_numput nump;
    nump.put_DU((unsigned long) line);
    data.append(nump.data(), nump.size());
    data += "']";

    // Throw it.
    throw ::std::runtime_error(data.c_str());
  }

int64_t
safe_double_to_int64(double val)
  {
    ::feclearexcept(FE_ALL_EXCEPT);
    int64_t ival = ::llrint(val);
    int fex = ::fetestexcept(FE_ALL_EXCEPT);

    if(fex & FE_INVALID)
      ::rocket::sprintf_and_throw<::std::invalid_argument>(
            "safe_double_to_int64: `%.17g` is not representable", val);

    if(fex != 0)
      ::rocket::sprintf_and_throw<::std::invalid_argument>(
            "safe_double_to_int64: `%.17g` is not exact", val);

    return ival;
  }

bool
utf8_encode(char*& pos, char32_t cp) noexcept
  {
    if(ROCKET_EXPECT(cp < 0x80U)) {
      // This character takes only one byte.
      *(pos++) = (char) cp;
      return true;
    }

    if(((cp >= 0xD800U) && (cp < 0xE000U)) || (cp >= 0x110000U))
      return false;

#define do_utf8_encode_one_(sh, m)  \
      (*(pos++) = (char) ((~m << 1) | (cp >> sh & m)))  // no semicolon

    // Encode the code point now. The result may be 2, 3 or 4 bytes.
    if(cp < 0x800U) {
      do_utf8_encode_one_( 6, 0x1FU);
      do_utf8_encode_one_( 0, 0x3FU);
    }
    else if(cp < 0x10000U) {
      do_utf8_encode_one_(12, 0x0FU);
      do_utf8_encode_one_( 6, 0x3FU);
      do_utf8_encode_one_( 0, 0x3FU);
    }
    else {
      do_utf8_encode_one_(18, 0x07U);
      do_utf8_encode_one_(12, 0x3FU);
      do_utf8_encode_one_( 6, 0x3FU);
      do_utf8_encode_one_( 0, 0x3FU);
    }
    return true;
  }

bool
utf8_encode(cow_string& text, char32_t cp)
  {
    // Encode the code point into this temporary buffer.
    char str[4];
    char* pos = str;
    if(!utf8_encode(pos, cp))
      return false;

    // Append all bytes encoded.
    text.append(str, pos);
    return true;
  }

bool
utf8_decode(char32_t& cp, const char*& pos, size_t avail) noexcept
  {
    if(avail == 0)
      return false;

    // Read the first byte.
    cp = (uint8_t) *(pos++);
    if(cp < 0x80U)
      return true;
    else if((cp < 0xC0U) || (cp >= 0xF8U))
      return false;

    // Calculate the number of bytes in this code point.
    uint32_t u8len = 2U + (cp >= 0xE0U) + (cp >= 0xF0U);
    if(u8len > avail)
      return false;

    // Unset bits that are not part of the payload.
    cp &= 0xFFU >> u8len;

    // Accumulate trailing code units.
    for(size_t i = 1;  i < u8len;  ++i)
      if(((uint8_t) *pos >= 0x80U) || ((uint8_t) *pos < 0xC0U))
        cp = (cp << 6) | ((uint8_t) *(pos++) & 0x3FU);
      else
        return false;

    if(((cp >= 0xD800U) && (cp < 0xE000U)) || (cp >= 0x110000U))
      return false;

    if(1U + (cp >= 0x80U) + (cp >= 0x800U) + (cp >= 0x10000U) != u8len)
      return false;

    return true;
  }

bool
utf8_decode(char32_t& cp, cow_stringR text, size_t& offset)
  {
    if(offset >= text.size())
      return false;

    // Decode bytes.
    const char* pos = text.data() + offset;
    if(!utf8_decode(cp, pos, text.size() - offset))
      return false;

    // Update the offset.
    offset = (size_t) (pos - text.data());
    return true;
  }

bool
utf16_encode(char16_t*& pos, char32_t cp) noexcept
  {
    if(((cp >= 0xD800U) && (cp < 0xE000U)) || (cp >= 0x110000U))
      return false;

    if(cp < 0x10000U) {
      *(pos++) = (char16_t) cp;
      return true;
    }

    // Write surrogates.
    *(pos++) = (char16_t) (0xD800U + ((cp - 0x10000U) >> 10));
    *(pos++) = (char16_t) (0xDC00U + (cp & 0x3FFU));
    return true;
  }

bool
utf16_encode(cow_u16string& text, char32_t cp)
  {
    // Encode the code point into this temporary buffer.
    char16_t str[2];
    char16_t* pos = str;
    if(!utf16_encode(pos, cp))
      return false;

    // Append all bytes encoded.
    text.append(str, pos);
    return true;
  }

bool
utf16_decode(char32_t& cp, const char16_t*& pos, size_t avail) noexcept
  {
    if(avail == 0)
      return false;

    // Read the first code unit.
    cp = *(pos++);
    if(ROCKET_EXPECT((cp < 0xD800U) || (cp >= 0xE000U)))
      return true;

    if(cp >= 0xDC00U)
      return false;

    if(avail < 2)
      return false;

    // Read the trailing surrogate.
    char16_t cu = *(pos++);
    if((cu < 0xDC00U) || (cu >= 0xE000U))
      return false;

    cp = 0x10000U + ((cp & 0x3FFU) << 10) + (cu & 0x3FFU);
    return true;
  }

bool
utf16_decode(char32_t& cp, const cow_u16string& text, size_t& offset)
  {
    if(offset >= text.size())
      return false;

    // Decode bytes.
    const char16_t* pos = text.data() + offset;
    if(!utf16_decode(cp, pos, text.size() - offset))
      return false;

    // Update the offset.
    offset = (size_t) (pos - text.data());
    return true;
  }

tinyfmt&
c_quote(tinyfmt& fmt, const char* data, size_t size)
  {
    for(size_t k = 0;  k != size;  ++k) {
      uint32_t ch = (uint8_t) data[k];
      const char* seq = s_char_escapes[ch];

      // Optimize the operation if it comprises only one character.
      if(ROCKET_EXPECT(seq[1] == 0))
        fmt << seq[0];
      else
        fmt << seq;
    }
    return fmt;
  }

tinyfmt&
c_quote(tinyfmt& fmt, cow_stringR data)
  {
    return c_quote(fmt, data.data(), data.size());
  }

cow_string&
c_quote(cow_string& str, const char* data, size_t size)
  {
    for(size_t k = 0;  k != size;  ++k) {
      uint32_t ch = (uint8_t) data[k];
      const char* seq = s_char_escapes[ch];

      // Optimize the operation if it comprises only one character.
      if(ROCKET_EXPECT(seq[1] == 0))
        str += seq[0];
      else
        str += seq;
    }
    return str;
  }

cow_string&
c_quote(cow_string& str, cow_stringR data)
  {
    return c_quote(str, data.data(), data.size());
  }

cow_string
get_real_path(cow_stringR path)
  {
    char* str = ::realpath(path.safe_c_str(), nullptr);
    if(!str)
      ::rocket::sprintf_and_throw<::std::runtime_error>(
          "get_real_path: could not resolve `%s`: %m", path.c_str());

    const auto guard = ::rocket::make_unique_handle(str, ::free);
    return cow_string(str);
  }

}  // namespace asteria
