// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "precompiled.ipp"
#include "utils.hpp"
#include <time.h>  // ::timespec, ::clock_gettime(), ::localtime()
#include <unistd.h>  // ::write
namespace asteria {

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
    data += " (";
    data += func;
    data += ") @ ";
    data += file;
    data += ':';
    nump.put_DU((unsigned long) line);
    data.append(nump.data(), nump.size());
    data += "\n\t";

    // Neutralize control characters: ['\x00','\x1F'] and '\x7F'.
    for(char c : msg) {
      uint32_t ch = (unsigned char) c;
      if(ch <= 0x1F)
        data += details_utils::ctrl_char_names[ch];
      else if(ch == 0x7F)
        data += "[DEL]";
      else
        data += (char) ch;
    }

    // Remove trailing space characters.
    size_t pos = data.find_last_not_of(" \f\n\r\t\v");
    data.erase(pos + 1);
    data += "\n\n";

    // Write the string now. Errors are ignored.
    return ::write(STDERR_FILENO, data.data(), data.size());
  }

void
throw_runtime_error(const char* file, long line, const char* func, cow_string&& msg)
  {
    // Compose the string to throw.
    cow_string data;
    data.reserve(2047);

    // Append the function name.
    data += func;
    data += ": ";

    // Append the user-provided exception message.
    data.append(msg.begin(), msg.end());

    // Remove trailing space characters.
    size_t pos = data.find_last_not_of(" \f\n\r\t\v");
    data.erase(pos + 1);
    data += "\n";

    // Append the source location.
    data += "[thrown from '";
    data += file;
    data += ':';
    ::rocket::ascii_numput nump;
    nump.put_DU((unsigned long) line);
    data.append(nump.data(), nump.size());
    data += "']";

    // Throw it.
    throw ::std::runtime_error(data.c_str());
  }

bool
utf8_encode(char*& pos, char32_t cp) noexcept
  {
    if(cp < 0x80) {
      // This character takes only one byte.
      *(pos++) = (char) cp;
      return true;
    }

    if((0xD800 <= cp) && (cp < 0xE000))
      // Surrogates are reserved for UTF-16.
      return false;

    if(cp >= 0x110000)
      // Code point is too large.
      return false;

    // Encode bits into a byte.
    auto encode_one = [&](int sh, char32_t m)
      { *(pos++) = (char) ((~m << 1) | (cp >> sh & m));  };

    // Encode the code point now. The result may be 2, 3 or 4 bytes.
    if(cp < 0x800) {
      encode_one( 6, 0x1F);
      encode_one( 0, 0x3F);
    }
    else if(cp < 0x10000) {
      encode_one(12, 0x0F);
      encode_one( 6, 0x3F);
      encode_one( 0, 0x3F);
    }
    else {
      encode_one(18, 0x07);
      encode_one(12, 0x3F);
      encode_one( 6, 0x3F);
      encode_one( 0, 0x3F);
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
    cp = *(pos++) & 0xFF;
    if(cp < 0x80)
      // This sequence contains only one byte.
      return true;

    if((cp < 0xC0) || (0xF8 <= cp))
      // This is not a leading character.
      return false;

    // Calculate the number of bytes in this code point.
    uint32_t u8len = 2U + (cp >= 0xE0) + (cp >= 0xF0);
    ROCKET_ASSERT(u8len >= 2);
    ROCKET_ASSERT(u8len <= 4);
    if(u8len > avail)
      // No enough characters have been provided.
      return false;

    // Unset bits that are not part of the payload.
    cp &= UINT32_C(0xFF) >> u8len;

    // Accumulate trailing code units.
    for(size_t i = 1;  i < u8len;  ++i) {
      char32_t cu = *(pos++) & 0xFF;
      if((cu < 0x80) || (0xC0 <= cu))
        // This trailing character is not valid.
        return false;
      cp = (cp << 6) | (cu & 0x3F);
    }

    if((0xD800 <= cp) && (cp < 0xE000))
      // Surrogates are reserved for UTF-16.
      return false;

    if(cp >= 0x110000)
      // Code point is too large.
      return false;

    // Re-encode it and check for overlong sequences.
    uint32_t milen = 1U + (cp >= 0x80) + (cp >= 0x800) + (cp >= 0x10000);
    if(milen != u8len)
      // Overlong sequences are not allowed.
      return false;

    return true;
  }

bool
utf8_decode(char32_t& cp, stringR text, size_t& offset)
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
    if((0xD800 <= cp) && (cp < 0xE000))
      // Surrogates are reserved for UTF-16.
      return false;

    if(cp < 0x10000) {
      // This character takes only one code unit.
      *(pos++) = (char16_t) cp;
      return true;
    }

    if(cp >= 0x110000)
      // Code point is too large.
      return false;

    // Write surrogates.
    *(pos++) = (char16_t) (0xD800 + ((cp - 0x10000) >> 10));
    *(pos++) = (char16_t) (0xDC00 + (cp & 0x3FF));
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
    cp = *(pos++) & 0xFFFF;
    if((cp < 0xD800) || (0xE000 <= cp))
      // This sequence contains only one code unit.
      return true;

    if(cp >= 0xDC00)
      // A trailing surrogate is not allowed unless following a leading surrogate.
      return false;

    if(avail < 2)
      // No enough code units have been provided.
      return false;

    // Read the trailing surrogate.
    char16_t cu = *(pos++);
    if((cu < 0xDC00) || (0xE000 <= cu))
      // Only a trailing surrogate is allowed to follow a leading surrogate.
      return false;

    cp = 0x10000 + ((cp & 0x3FF) << 10) + (cu & 0x3FF);
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

Wrapped_Index
wrap_index(int64_t index, size_t size) noexcept
  {
    ROCKET_ASSERT(size <= PTRDIFF_MAX);

    // The range of valid indices is (~size, size).
    Wrapped_Index w;
    auto ssize = (int64_t) size;
    if(index >= 0) {
      // Append elements as needed.
      auto nappend = ::rocket::max(index, ssize - 1) - (ssize - 1);
      w.nprepend = 0;
      w.nappend = (uint64_t) nappend;

      // `index` is truncated if it does not fit in `size_t`, but in this case it
      // shouldn't be used.
      w.rindex = (size_t) index;
    }
    else {
      // Prepend elements as needed.
      auto nprepend = ::rocket::max(index - 1, ~ssize) - (index - 1);
      w.nprepend = (uint64_t) nprepend;
      w.nappend = 0;

      // `index + ssize` cannot overflow when `index` is negative and `ssize` is not.
      w.rindex = (size_t) (index + ssize) + (size_t) nprepend;
    }
    return w;
  }

uint64_t
generate_random_seed() noexcept
  {
    // Get the system time of very high resolution.
    ::timespec ts;
    ::clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t ireg = (uint64_t) ts.tv_sec;
    ireg <<= 30;
    ireg |= (uint32_t) ts.tv_nsec;

    // Hash it using FNV-1a to erase sensitive information.
    //   https://en.wikipedia.org/wiki/Fowler–Noll–Vo_hash_function
    // The source value is read in little-endian byte order.
    uint64_t seed = 0xCBF29CE484222325U;
    for(size_t i = 0;  i < 8;  ++i) {
      seed = (seed ^ (ireg & 0xFF)) * 0x100000001B3U;
      ireg >>= 8;
    }
    return seed;
  }

}  // namespace asteria
