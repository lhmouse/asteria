// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_TINYBUF_HPP_
#define ROCKET_TINYBUF_HPP_

#include "assert.hpp"
#include "throw.hpp"
#include "utilities.hpp"
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
        seek_set  = 0,  // SEEK_SET
        seek_cur  = 1,  // SEEK_CUR
        seek_end  = 2,  // SEEK_END
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

    static constexpr
    bool
    has_mode(open_mode mode, open_mode opt)
    noexcept
      { return (mode & opt) == opt;  }
  };

constexpr
tinybuf_base::open_mode
operator~(tinybuf_base::open_mode rhs)
noexcept
  { return tinybuf_base::open_mode(~(uint32_t)rhs);  }

constexpr
tinybuf_base::open_mode
operator&(tinybuf_base::open_mode lhs, tinybuf_base::open_mode rhs)
noexcept
  { return tinybuf_base::open_mode((uint32_t)lhs & (uint32_t)rhs);  }

constexpr
tinybuf_base::open_mode
operator|(tinybuf_base::open_mode lhs, tinybuf_base::open_mode rhs)
noexcept
  { return tinybuf_base::open_mode((uint32_t)lhs | (uint32_t)rhs);  }

constexpr
tinybuf_base::open_mode
operator^(tinybuf_base::open_mode lhs, tinybuf_base::open_mode rhs)
noexcept
  { return tinybuf_base::open_mode((uint32_t)lhs ^ (uint32_t)rhs);  }

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

  private:
    // get area
    const char_type* m_gcur = nullptr;
    const char_type* m_gend = nullptr;

    // put area
    char_type* m_pcur = nullptr;
    char_type* m_pend = nullptr;

  protected:
    basic_tinybuf()
    noexcept
      = default;

    basic_tinybuf(const basic_tinybuf&)
    noexcept
      = default;

    basic_tinybuf&
    operator=(const basic_tinybuf&)
    noexcept
      = default;

  public:
    virtual
    ~basic_tinybuf();

  private:
    int_type
    do_call_underflow(bool peek)
      { return this->do_underflow(this->m_gcur, this->m_gend, peek);  }

    basic_tinybuf&
    do_call_overflow(const char_type* sadd, size_type nadd)
      { return this->do_overflow(this->m_pcur, this->m_pend, sadd, nadd);  }

  protected:
    // * Estimates how many characters are available for non-blocking reads.
    // * Returns `0` if the number is unknown.
    //   Returns `-1` if no character is available.
    virtual
    off_type
    do_fortell()
    const
      {
        // The default implementation indicates an unknown number.
        return 0;
      }

    // * Synchronizes the get and put areas with the external device.
    // * Throws an exception on failure.
    virtual
    basic_tinybuf&
    do_flush(const char_type*& /*gcur*/, const char_type*& /*gend*/, char_type*& /*pcur*/, char_type*& /*pend*/)
      {
        // The default implementation does nothing.
        return *this;
      }

    // * Sets the stream position.
    // * Returns its absolute value.
    // * Throws an exception on failure.
    virtual
    off_type
    do_seek(off_type /*off*/, seek_dir /*dir*/)
      {
        // The default implementation always fails.
        noadl::sprintf_and_throw<invalid_argument>("tinybuf: stream not seekable");
      }

    // * Reads data from the external device into the get area and discards it unless `peek` is set.
    // * Returns the first character that has been read, or `traits::eof()` if there are no more
    //   characters.
    // * Throws an exception in case of failure.
    // This function may reallocate the get area as needed.
    virtual
    int_type
    do_underflow(const char_type*& /*gcur*/, const char_type*& /*gend*/, bool /*peek*/)
      {
        // The default implementation always fails.
        noadl::sprintf_and_throw<invalid_argument>("tinybuf: stream not readable");
      }

    // * Writes the contents of the put area, followed by the sequence denoted by `sadd`
    //   unless `nadd` is zero, to the external device.
    // * Throws an exception on failure.
    // This function may reallocate the put area as needed.
    virtual
    basic_tinybuf&
    do_overflow(char_type*& /*pcur*/, char_type*& /*pend*/, const char_type* /*sadd*/, size_type /*nadd*/)
      {
       // The default implementation always fails.
        noadl::sprintf_and_throw<invalid_argument>("tinybuf: stream not writable");
      }

    // * Calls `do_flush()` only when either the get or put area is active.
    // This function may be useful when handling interleaving reads and writes.
    basic_tinybuf&
    do_sync_areas()
      {
        if(ROCKET_EXPECT(!this->m_gcur && !this->m_pcur))
          // Don't bother doing anything if neither the get area nor the put area exists.
          return *this;
        else
          // Synchronize the get and put areas.
          return this->do_flush(this->m_gcur, this->m_gend, this->m_pcur, this->m_pend);
      }

    // * Destroys the get and put areas.
    // This function may be useful when clearing this buffer.
    basic_tinybuf&
    do_purge_areas()
    noexcept
      {
        // Purge the get area.
        this->m_gcur = nullptr;
        this->m_gend = nullptr;

        // Purge the put area.
        this->m_pcur = nullptr;
        this->m_pend = nullptr;
        return *this;
      }

  public:
    off_type
    fortell()
    const
      {
        if(ROCKET_EXPECT(this->m_gcur != this->m_gend))
          // Return the number of characters in the get area.
          return this->m_gend - this->m_gcur;
        else
          // Return the number of characters after the get area.
          return this->do_fortell();
      }

    basic_tinybuf&
    flush()
      {
        // Synchronize the get and put areas.
        return this->do_flush(this->m_gcur, this->m_gend, this->m_pcur, this->m_pend);
      }

    off_type
    seek(off_type off, seek_dir dir)
      {
        // Reposition the stream, which might invalidate the get and put areas.
        return this->do_seek(off, dir);
      }

    int_type
    peekc()
      {
        if(ROCKET_EXPECT(this->m_gcur != this->m_gend))
          // Return the first character in the get area.
          return traits_type::to_int_type(this->m_gcur[0]);
        else
          // Try populating the get area.
          return this->do_call_underflow(true);
      }

    int_type
    getc()
      {
        if(ROCKET_EXPECT(this->m_gcur != this->m_gend))
          // Return and discard the first character in the get area.
          return traits_type::to_int_type(this->m_gcur++[0]);
        else
          // Try populating the get area and discard the first character.
          return this->do_call_underflow(false);
      }

    basic_tinybuf&
    putc(char_type c)
      {
        if(ROCKET_EXPECT(this->m_pcur != this->m_pend))
          // Append a character to the put area.
          return traits_type::assign(this->m_pcur++[0], c), *this;
        else
          // Evict data from the put area followed by the character specified.
          return this->do_call_overflow(::std::addressof(c), 1);
      }

    size_type
    getn(char_type* s, size_type n)
      {
        if(n == 0) {
          // There is nothing to do.
          return 0;
        }
        auto k = static_cast<size_type>(this->m_gend - this->m_gcur);
        if(ROCKET_UNEXPECT(k == 0)) {
          // If the get area is empty, try populating it.
          if(traits_type::is_eof(this->do_call_underflow(true)))
            return 0;
          k = static_cast<size_type>(this->m_gend - this->m_gcur);
          ROCKET_ASSERT(k != 0);
        }
        // Don't read past the end of the get area.
        k = noadl::min(k, n);
        // Consume some characters from the get area.
        traits_type::copy(s, this->m_gcur, k);
        this->m_gcur += k;
        return k;
      }

    basic_tinybuf&
    putn(const char_type* s, size_type n)
      {
        if(n == 0) {
          // There is nothing to do.
          return *this;
        }
        auto k = static_cast<size_type>(this->m_pend - this->m_pcur);
        if(ROCKET_UNEXPECT(n >= k)) {
          // If there is no enough room in the put area, evict its contents, followed by
          // the string specified.
          return this->do_call_overflow(s, n);
        }
        // Append the string to the put area.
        traits_type::copy(this->m_pcur, s, k);
        this->m_pcur += k;
        return *this;
      }

    basic_tinybuf&
    puts(const char_type* s)
      {
        static constexpr char_type s_null[] = { '(', 'n', 'u', 'l', 'l', ')', 0 };
        if(!s) {
          // Don't result in a segfault at least.
          return this->putn(s_null, traits_type::length(s_null));
        }
        return this->putn(s, traits_type::length(s));
      }

    basic_tinybuf&
    swap(basic_tinybuf& other)
    noexcept
      {
        noadl::xswap(this->m_gcur, other.m_gcur);
        noadl::xswap(this->m_gend, other.m_gend);
        noadl::xswap(this->m_pcur, other.m_pcur);
        noadl::xswap(this->m_pend, other.m_pend);
        return *this;
      }
  };

template<typename charT, typename traitsT>
basic_tinybuf<charT, traitsT>::
~basic_tinybuf()
  = default;

template<typename charT, typename traitsT>
inline
void
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
