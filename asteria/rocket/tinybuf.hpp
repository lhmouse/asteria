// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_TINYBUF_HPP_
#define ROCKET_TINYBUF_HPP_

#include "fwd.hpp"
#include "assert.hpp"
#include "throw.hpp"
#include "char_traits.hpp"

namespace rocket {

template<typename charT, typename traitsT = char_traits<charT>>
class basic_tinybuf;

/* Differences from `std::basic_streambuf`:
 * 1. Locales are not supported.
 * 2. Putting back is not supported.
 * 3. The design has been simplified.
 * 4. `off_type` is always `int64_t` regardless of the traits in effect.
**/

struct tinybuf_base
  {
    enum seek_dir : uint8_t
      {
        seek_set  = SEEK_SET,
        seek_cur  = SEEK_CUR,
        seek_end  = SEEK_END,
      };

    enum open_mode : uint16_t
      {
        open_read        = 0b000000000001,  // O_RDONLY
        open_write       = 0b000000000010,  // O_WRONLY
        open_read_write  = 0b000000000011,  // O_RDWR
        open_append      = 0b000000000110,  // O_APPEND
        open_binary      = 0b000000001000,  // _O_BINARY (Windows only)
        open_create      = 0b000000010010,  // O_CREAT
        open_truncate    = 0b000000100010,  // O_TRUNC
        open_exclusive   = 0b000001000010,  // O_EXCL
      };
  };

constexpr tinybuf_base::open_mode
operator~(tinybuf_base::open_mode rhs) noexcept
  { return (tinybuf_base::open_mode)~(uint32_t)rhs;  }

constexpr tinybuf_base::open_mode
operator&(tinybuf_base::open_mode lhs, tinybuf_base::open_mode rhs) noexcept
  { return (tinybuf_base::open_mode)((uint32_t)lhs & (uint32_t)rhs);  }

constexpr tinybuf_base::open_mode
operator|(tinybuf_base::open_mode lhs, tinybuf_base::open_mode rhs) noexcept
  { return (tinybuf_base::open_mode)((uint32_t)lhs | (uint32_t)rhs);  }

constexpr tinybuf_base::open_mode
operator^(tinybuf_base::open_mode lhs, tinybuf_base::open_mode rhs) noexcept
  { return (tinybuf_base::open_mode)((uint32_t)lhs ^ (uint32_t)rhs);  }

template<typename charT, typename traitsT>
class basic_tinybuf
  : public tinybuf_base
  {
  public:
    using char_type       = charT;
    using traits_type     = traitsT;

    using int_type   = typename traits_type::int_type;
    using off_type   = int64_t;
    using size_type  = size_t;

  protected:
    basic_tinybuf() noexcept
      = default;

    basic_tinybuf(const basic_tinybuf&) noexcept
      = default;

    basic_tinybuf&
    operator=(const basic_tinybuf&) noexcept
      = default;

  protected:
    // These can be overridden by derived buffer classes.
    virtual void
    do_flush()
      { }

    virtual off_type
    do_tell() const
      {
        noadl::sprintf_and_throw<invalid_argument>("tinybuf: stream not seekable");
      }

    virtual void
    do_seek(off_type /*off*/, seek_dir /*dir*/)
      {
        noadl::sprintf_and_throw<invalid_argument>("tinybuf: stream not seekable");
      }

    virtual size_type
    do_getn(char_type* /*s*/, size_type /*n*/)
      {
        noadl::sprintf_and_throw<invalid_argument>("tinybuf: stream not readable");
      }

    virtual int_type
    do_getc()
      {
        char_type c;
        if(this->do_getn(::std::addressof(c), 1) == 0)
          return traits_type::eof();
        return traits_type::to_int_type(c);
      }

    virtual void
    do_putn(const char_type* /*s*/, size_type /*n*/)
      {
        noadl::sprintf_and_throw<invalid_argument>("tinybuf: stream not writable");
      }

    virtual void
    do_putc(char_type c)
      {
        this->do_putn(::std::addressof(c), 1);
      }

  public:
    virtual
    ~basic_tinybuf();

    basic_tinybuf&
    flush()
      { this->do_flush();  return *this;  }

    off_type
    tell() const
      { return this->do_tell();  }

    basic_tinybuf&
    seek(off_type off, seek_dir dir = seek_set)
      { this->do_seek(off, dir);  return *this;  }

    size_type
    getn(char_type* s, size_type n)
      { return this->do_getn(s, n);  }

    int_type
    getc()
      { return this->do_getc();  }

    basic_tinybuf&
    putn(const char_type* s, size_type n)
      { this->do_putn(s, n);  return *this;  }

    basic_tinybuf&
    putc(char_type c)
      { this->do_putc(c);  return *this;  }

    basic_tinybuf&
    puts(const char_type* s)
      { this->do_putn(s, traits_type::length(s));  return *this;  }
  };

template<typename charT, typename traitsT>
basic_tinybuf<charT, traitsT>::
~basic_tinybuf()
  { }

template<typename charT, typename traitsT>
inline void
swap(basic_tinybuf<charT, traitsT>& lhs, basic_tinybuf<charT, traitsT>& rhs)
  noexcept(noexcept(lhs.swap(rhs)))
  { lhs.swap(rhs);  }

extern template
class basic_tinybuf<char>;

extern template
class basic_tinybuf<wchar_t>;

using tinybuf   = basic_tinybuf<char>;
using wtinybuf  = basic_tinybuf<wchar_t>;

}  // namespace rocket

#endif
