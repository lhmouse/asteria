// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_TINYBUF_HPP_
#define ROCKET_TINYBUF_HPP_

#include "assert.hpp"
#include "throw.hpp"
#include "utilities.hpp"

/* Differences from `std::basic_streambuf`:
 * 1. Locales are not supported.
 * 2. Putting back is not supported.
 * 3. The design has been simplified.
 * 4. `off_type` is always `int64_t` regardless of the traits in effect.
 */

namespace rocket {

template<typename charT, typename traitsT = char_traits<charT>> class basic_tinybuf;

struct tinybuf_base
  {
    enum seek_dir
      {
        seek_set  = 0,  // SEEK_SET
        seek_cur  = 1,  // SEEK_CUR
        seek_end  = 2,  // SEEK_END
      };
    enum open_mode
      {
        open_read    = 0x0001,  // O_RDONLY
        open_write   = 0x0002,  // O_WRONLY
        open_rdwr    = 0x0003,  // O_RDWR
        open_trunc   = 0x0010,  // O_TRUNC
        open_create  = 0x0020,  // O_CREAT
        open_append  = 0x0040,  // O_APPEND
        open_binary  = 0x0080,  // _O_BINARY  (Windows only)
        open_excl    = 0x0100,  // O_EXCL | O_CREAT
      };
  };

template<typename charT, typename traitsT> class basic_tinybuf : public tinybuf_base
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
    basic_tinybuf() noexcept
      = default;
    basic_tinybuf(const basic_tinybuf&) noexcept
      = default;
    basic_tinybuf& operator=(const basic_tinybuf&) noexcept
      = default;

  public:
    virtual ~basic_tinybuf();

  private:
    int_type do_call_underflow(bool peek)
      {
        return this->do_underflow(this->m_gcur, this->m_gend, peek);
      }
    void do_call_overflow(const char_type* sadd, size_type nadd)
      {
        return this->do_overflow(this->m_pcur, this->m_pend, sadd, nadd);
      }

  protected:
    // * Synchronizes the get and put areas with the external device.
    // * Throws an exception on failure.
    // The default implementation does nothing.
    virtual void do_flush(const char_type*& /*gcur*/, const char_type*& /*gend*/, char_type*& /*pcur*/, char_type*& /*pend*/)
      {
      }
    // * Sets the stream position.
    // * Returns its absolute value.
    // * Throws an exception on failure.
    // The default implementation fails.
    virtual off_type do_seek(off_type /*off*/, seek_dir /*dir*/)
      {
        noadl::sprintf_and_throw<invalid_argument>("basic_tinybuf: This stream is not seekable.");
      }
    // * Estimates how many characters are available for non-blocking reads.
    // * Returns `0` if the number is unknown.
    //   Returns `-1` if no character is available.
    // The default implementation indicates an unknown number.
    virtual off_type do_predict() const
      {
        return 0;
      }

    // * Reads data from the external device into the get area and discards it unless `peek` is set.
    // * Returns the first character that has been read, or `traits::eof()` if there are no more characters.
    // * Throws an exception in case of failure.
    // This function may reallocate the get area as needed.
    // The default implementation fails.
    virtual int_type do_underflow(const char_type*& /*gcur*/, const char_type*& /*gend*/, bool /*peek*/)
      {
        noadl::sprintf_and_throw<invalid_argument>("basic_tinybuf: This stream is not readable.");
      }
    // * Writes the contents of the put area, followed by the sequence denoted by `sadd` unless `nadd` is zero, to the external device.
    // * Throws an exception on failure.
    // This function may reallocate the put area as needed.
    // The default implementation fails.
    virtual void do_overflow(char_type*& /*pcur*/, char_type*& /*pend*/, const char_type* /*sadd*/, size_type /*nadd*/)
      {
        noadl::sprintf_and_throw<invalid_argument>("basic_tinybuf: This stream is not writable.");
      }

  public:
    void flush()
      {
        if(ROCKET_EXPECT(!this->m_gcur && !this->m_pcur))
          // Don't bother do anything if neither the get area nor the put area exists.
          return;
        else
          // Synchronize the get and put areas.
          return this->do_flush(this->m_gcur, this->m_gend, this->m_pcur, this->m_pend);
      }
    off_type seek(off_type off, seek_dir dir)
      {
        // Reposition the stream, which might invalidate the get and put areas.
        return this->do_seek(off, dir);
      }
    off_type predict() const
      {
        if(ROCKET_EXPECT(this->m_gcur != this->m_gend))
          // Return the number of characters in the get area.
          return this->m_gend - this->m_gcur;
        else
          // Return the number of characters after the get area.
          return this->do_predict();
      }

    int_type peekc()
      {
        if(ROCKET_EXPECT(this->m_gcur != this->m_gend))
          // Return the first character in the get area.
          return traits_type::to_int_type(this->m_gcur[0]);
        else
          // Try populating the get area.
          return this->do_call_underflow(true);
      }
    int_type getc()
      {
        if(ROCKET_EXPECT(this->m_gcur != this->m_gend))
          // Return and discard the first character in the get area.
          return traits_type::to_int_type(this->m_gcur++[0]);
        else
          // Try populating the get area and discard the first character.
          return this->do_call_underflow(false);
      }
    size_type getn(char_type* s, size_type n)
      {
        auto m = static_cast<size_type>(this->m_gend - this->m_gcur);
        if(ROCKET_UNEXPECT(m == 0)) {
          // If the get area is empty, try populating it.
          if(traits_type::eq_int_type(this->do_call_underflow(true), traits_type::eof())) {
            // Report EOF.
            return 0;
          }
          m = static_cast<size_type>(this->m_gend - this->m_gcur);
          ROCKET_ASSERT(m != 0);
        }
        // Consume some characters from the get area.
        m = noadl::min(m, n);
        traits_type::copy(s, this->m_gcur, m);
        this->m_gcur += m;
        return m;
      }
    void putc(char_type c)
      {
        if(ROCKET_EXPECT(this->m_pcur != this->m_pend))
          // Append a character to the put area.
          return traits_type::assign(this->m_pcur++[0], c), void(0);
        else
          // Evict data from the put area followed by the character specified.
          return this->do_call_overflow(::std::addressof(c), 1);
      }
    void putn(const char_type* s, size_type n)
      {
        auto m = static_cast<size_type>(this->m_pend - this->m_pcur);
        if(ROCKET_UNEXPECT(n >= m)) {
          // If there is no enough room in the put area, evict its contents, followed by the string specified.
          this->do_call_overflow(s, n);
          return;
        }
        // Append the string to the put area.
        traits_type::copy(this->m_pcur, s, m);
        this->m_pcur += m;
      }

    void swap(basic_tinybuf& other) noexcept
      {
        ::std::swap(this->m_gcur, other.m_gcur);
        ::std::swap(this->m_gend, other.m_gend);
        ::std::swap(this->m_pcur, other.m_pcur);
        ::std::swap(this->m_pend, other.m_pend);
      }
  };

template<typename charT, typename traitsT> basic_tinybuf<charT, traitsT>::~basic_tinybuf()
  = default;

template<typename charT, typename traitsT> void swap(basic_tinybuf<charT, traitsT>& lhs, basic_tinybuf<charT, traitsT>& rhs) noexcept
  {
    return lhs.swap(rhs);
  }

extern template class basic_tinybuf<char>;
extern template class basic_tinybuf<wchar_t>;

using tinybuf   = basic_tinybuf<char>;
using wtinybuf  = basic_tinybuf<wchar_t>;

}  // namespace rocket

#endif
