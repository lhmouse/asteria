// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_TINYBUF_
#define ROCKET_TINYBUF_

#include "fwd.hpp"
#include "tinybuf_base.hpp"
namespace rocket {

template<typename charT>
class basic_tinybuf
  :
    public tinybuf_base
  {
  public:
    using char_type  = charT;
    using seek_dir   = tinybuf_base::seek_dir;
    using open_mode  = tinybuf_base::open_mode;

  protected:
    basic_tinybuf() noexcept = default;

    basic_tinybuf(const basic_tinybuf&) noexcept = delete;
    basic_tinybuf& operator=(const basic_tinybuf&) & noexcept = delete;

  public:
    virtual ~basic_tinybuf();

    // Flushes a stream, like `fflush()`.
    virtual
    basic_tinybuf&
    flush() = 0;

    // Gets the current stream pointer.
    // If the stream is non-seekable, `-1` shall be returned.
    virtual
    int64_t
    tell() const = 0;

    // Adjusts the current stream pointer.
    // If an exception is thrown, the stream is left in an unspecified state.
    virtual
    basic_tinybuf&
    seek(int64_t off, seek_dir dir) = 0;

    // Reads some characters from the stream.
    // Upon success, the number of characters that have been read successfully
    // shall be returned. If the end of stream has been reached, zero shall be
    // returned. If an exception is thrown, the stream is left in an unspecified
    // state.
    virtual
    size_t
    getn(char_type* s, size_t n) = 0;

    // Reads a single character from the stream.
    // Upon success, the character that has been read shall be zero-extended to
    // an integer and returned. If the end of stream has been reached, `-1` shall
    // be returned. If an exception is thrown, the stream is left in an unspecified
    // state.
    virtual
    int
    getc() = 0;

    // Puts some characters into the stream.
    // This function shall return only after all characters have been written
    // successfully. If an exception is thrown, the stream is left in an
    // unspecified state.
    virtual
    basic_tinybuf&
    putn(const char_type* s, size_t n) = 0;

    // Puts a single character into the stream.
    // This function shall return only after the character has been written
    // successfully. If an exception is thrown, the stream is left in an
    // unspecified state.
    virtual
    basic_tinybuf&
    putc(char_type c) = 0;
  };

template<typename charT>
basic_tinybuf<charT>::
~basic_tinybuf()
  {
  }

using tinybuf     = basic_tinybuf<char>;
using wtinybuf    = basic_tinybuf<wchar_t>;
using u16tinybuf  = basic_tinybuf<char16_t>;
using u32tinybuf  = basic_tinybuf<char32_t>;

extern template class basic_tinybuf<char>;
extern template class basic_tinybuf<wchar_t>;
extern template class basic_tinybuf<uint16_t>;
extern template class basic_tinybuf<uint32_t>;

}  // namespace rocket
#endif
