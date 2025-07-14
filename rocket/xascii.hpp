// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_XASCII_
#define ROCKET_XASCII_

#include "fwd.hpp"
namespace rocket {

constexpr
bool
ascii_is_upper(char c)
  noexcept
  {
    return (c >= 'A') && (c <= 'Z');
  }

constexpr
bool
ascii_is_lower(char c)
  noexcept
  {
    return (c >= 'a') && (c <= 'z');
  }

constexpr
bool
ascii_is_alpha(char c)
  noexcept
  {
    return (c >= 'A') && (c <= 'z') && (((c - 'A') & 0x1F) <= 25);
  }

constexpr
char
ascii_to_upper(char c)
  noexcept
  {
    return static_cast<char>(c & ((0xDF00 | (c - 'a') | ('z' - c)) >> 8));
  }

constexpr
char
ascii_to_lower(char c)
  noexcept
  {
    return static_cast<char>(c | ((0x2000 & ('A' - c - 1) & (c - 'Z' - 1)) >> 8));
  }

constexpr
bool
ascii_ci_equal(char x, char y)
  noexcept
  {
    return noadl::ascii_to_lower(x) == noadl::ascii_to_lower(y);
  }

constexpr
int
ascii_ci_compare(char x, char y)
  noexcept
  {
    return (noadl::ascii_to_lower(x) & 0xFF) - (noadl::ascii_to_lower(y) & 0xFF);
  }

constexpr
bool
ascii_ci_equal(const char* sx, size_t nx, const char* sy, size_t ny)
  noexcept
  {
    // If the lengths are unequal, the strings cannot be equal.
    if(nx != ny)
      return false;

    // Perform character-wise comparison of two strings of `nx` characters.
    for(size_t k = 0;  k != nx;  ++k)
      if(noadl::ascii_ci_compare(sx[k], sy[k]) != 0)
        return false;

    // All characters are equal.
    return true;
  }

constexpr
int
ascii_ci_compare(const char* sx, size_t nx, const char* sy, size_t ny)
  noexcept
  {
    // Get the length of the common initial substring.
    size_t n = noadl::min(nx, ny);

    // If any characters compare unequal, return immediately.
    for(size_t k = 0;  k != n;  ++k)
      if(int diff = noadl::ascii_ci_compare(sx[k], sy[k]))
        return diff;

    // Compare the lengths.
    if(nx != ny)
      return (nx < ny) ? -1 : +1;

    // All characters are equal.
    return 0;
  }

constexpr
size_t
ascii_ci_hash(const char* sp, size_t n)
  noexcept
  {
    // Implement the FNV-1a hash algorithm.
    uint32_t reg = 0x811C9DC5U;

    for(size_t k = 0;  k != n;  ++k)
      reg ^= static_cast<uint8_t>(noadl::ascii_to_lower(sp[k])),
        reg *= 0x1000193U;

    return reg;
  }

}  // namespace rocket
#endif
