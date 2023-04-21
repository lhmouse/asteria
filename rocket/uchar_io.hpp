// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_UCHAR_IO_
#define ROCKET_UCHAR_IO_

#include "fwd.hpp"
#include <wchar.h>
namespace rocket {

// Gets a sequence of characters from `fp`. The operation stops if `n`
// characters have been stored, or if the end of file has been reached, or
// after a new line character is encountered (which is also stored). Upon
// success, the number of characters that have been stored is returned. If
// the operation fails, an exception is thrown, and `fp` and `mbst` are
// left in unspecified states.
size_t
xfgetn(::FILE* fp, ::mbstate_t& mbst, char* s, size_t n);

size_t
xfgetn(::FILE* fp, ::mbstate_t& mbst, wchar_t* s, size_t n);

size_t
xfgetn(::FILE* fp, ::mbstate_t& mbst, char16_t* s, size_t n);

size_t
xfgetn(::FILE* fp, ::mbstate_t& mbst, char32_t* s, size_t n);

// Gets a single character from `fp`. If the end of file has been reached,
// `-1` is returned. Upon success, the character is stored into `c`. If
// the operation fails, an exception is thrown, and `fp` and `mbst` are
// left in unspecified states.
int
xfgetc(::FILE* fp, ::mbstate_t& mbst, char& c);

int
xfgetc(::FILE* fp, ::mbstate_t& mbst, wchar_t& c);

int
xfgetc(::FILE* fp, ::mbstate_t& mbst, char16_t& c);

int
xfgetc(::FILE* fp, ::mbstate_t& mbst, char32_t& c);

// Puts a sequence of characters into `fp`. Upon success, the number of
// bytes that have been transferred is returned. If the operation fails, an
// exception is thrown, and `fp` and `mbst` are left in unspecified states.
size_t
xfputn(::FILE* fp, ::mbstate_t& mbst, const char* s, size_t n);

size_t
xfputn(::FILE* fp, ::mbstate_t& mbst, const wchar_t* s, size_t n);

size_t
xfputn(::FILE* fp, ::mbstate_t& mbst, const char16_t* s, size_t n);

size_t
xfputn(::FILE* fp, ::mbstate_t& mbst, const char32_t* s, size_t n);

// Puts a single character into `fp`. Upon success, a non-negative value is
// returned. If the operation fails, an exception is thrown, and `fp`
// and `mbst` are left in unspecified states.
int
xfputc(::FILE* fp, ::mbstate_t& mbst, char c);

int
xfputc(::FILE* fp, ::mbstate_t& mbst, wchar_t c);

int
xfputc(::FILE* fp, ::mbstate_t& mbst, char16_t c);

int
xfputc(::FILE* fp, ::mbstate_t& mbst, char32_t c);

}  // namespace rocket
#endif
