// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_TINYBUF_LN_
#define ROCKET_TINYBUF_LN_

#include "tinybuf.hpp"
#include "linear_buffer.hpp"
namespace rocket {

template<typename charT, typename allocT = allocator<charT>>
class basic_tinybuf_ln
  :
    public basic_tinybuf<charT>
  {
  public:
    using char_type     = charT;
    using seek_dir      = tinybuf_base::seek_dir;
    using open_mode     = tinybuf_base::open_mode;
    using tinybuf_type  = basic_tinybuf<charT>;

    using buffer_type     = basic_linear_buffer<charT, allocT>;
    using allocator_type  = allocT;

  private:
    buffer_type m_ln;
    open_mode m_mode = tinybuf_base::open_read_write;

  public:
    constexpr
    basic_tinybuf_ln()
      noexcept(is_nothrow_default_constructible<buffer_type>::value)
      :
        m_ln()
      { }

    explicit constexpr
    basic_tinybuf_ln(const allocator_type& alloc)
      noexcept
      :
        m_ln(alloc)
      { }

    explicit
    basic_tinybuf_ln(open_mode mode, const allocator_type& alloc = allocator_type())
      noexcept
      :
        m_ln(alloc), m_mode(mode)
      { }

    template<typename xlnT,
    ROCKET_ENABLE_IF(is_constructible<buffer_type, xlnT&&, const allocator_type&>::value)>
    constexpr
    basic_tinybuf_ln(xlnT&& xln, open_mode mode = tinybuf_base::open_read_write,
                     const allocator_type& alloc = allocator_type())
      noexcept(is_nothrow_constructible<buffer_type, xlnT&&, const allocator_type>::value)
      :
        m_ln(forward<xlnT>(xln), alloc), m_mode(mode)
      { }

    // The copy and move constructors are necessary because `basic_tinybuf`
    // is not move-constructible.
    basic_tinybuf_ln(const basic_tinybuf_ln& other)
      noexcept(is_nothrow_copy_constructible<buffer_type>::value)
      :
        tinybuf_type(), m_ln(other.m_ln), m_mode(other.m_mode)
      { }

    basic_tinybuf_ln(const basic_tinybuf_ln& other, const allocator_type& alloc)
      noexcept
      :
        m_ln(other.m_ln, alloc), m_mode(other.m_mode)
      { }

    basic_tinybuf_ln&
    operator=(const basic_tinybuf_ln& other)
      & noexcept(is_nothrow_copy_assignable<buffer_type>::value)
      {
        this->m_ln = other.m_ln;
        this->m_mode = other.m_mode;
        return *this;
      }

    basic_tinybuf_ln(basic_tinybuf_ln&& other)
      noexcept(is_nothrow_move_constructible<buffer_type>::value)
      :
        tinybuf_type(), m_ln(move(other.m_ln)), m_mode(noadl::exchange(other.m_mode))
      { }

    basic_tinybuf_ln(basic_tinybuf_ln&& other, const allocator_type& alloc)
      noexcept
      :
        m_ln(move(other.m_ln), alloc), m_mode(noadl::exchange(other.m_mode))
      { }

    basic_tinybuf_ln&
    operator=(basic_tinybuf_ln& other) &&
      noexcept(is_nothrow_move_assignable<buffer_type>::value)
      {
        this->m_ln = move(other.m_ln);
        this->m_mode = noadl::exchange(other.m_mode);
        return *this;
      }

    basic_tinybuf_ln&
    swap(basic_tinybuf_ln& other)
      noexcept(is_nothrow_swappable<buffer_type>::value)
      {
        this->m_ln.swap(other.m_ln);
        ::std::swap(this->m_mode, other.m_mode);
        return *this;
      }

  public:
    virtual ~basic_tinybuf_ln() override;

    // Gets the internal buffer.
    const buffer_type&
    get_buffer()
      const noexcept
      { return this->m_ln;  }

    const char_type*
    data()
      const noexcept
      { return this->m_ln.data();  }

    size_t
    size()
      const noexcept
      { return this->m_ln.size();  }

    ptrdiff_t
    ssize()
      const noexcept
      { return this->m_ln.ssize();  }

    const char_type*
    begin()
      const noexcept
      { return this->m_ln.begin();  }

    const char_type*
    end()
      const noexcept
      { return this->m_ln.end();  }

    // Replaces the internal buffer.
    template<typename xlnT>
    basic_tinybuf_ln&
    set_buffer(xlnT&& xln, open_mode mode)
      {
        this->m_ln = forward<xlnT>(xln);
        this->m_mode = mode;
        return *this;
      }

    template<typename xlnT>
    basic_tinybuf_ln&
    set_buffer(xlnT&& xln)
      {
        this->m_ln = forward<xlnT>(xln);
        return *this;
      }

    basic_tinybuf_ln&
    clear_buffer()
      {
        this->m_ln.clear();
        return *this;
      }

    buffer_type
    extract_buffer()
      {
        buffer_type r;
        this->m_ln.swap(r);
        return r;
      }

    // Does nothing. Buffers need not be flushed.
    virtual
    basic_tinybuf_ln&
    flush()
      override
      {
        return *this;
      }

    // Gets the current stream pointer.
    // This function always fails.
    virtual
    int64_t
    tell()
      const override
      {
        return -1;
      }

    // Adjusts the current stream pointer.
    // This function always fails.
    virtual
    basic_tinybuf_ln&
    seek(int64_t /*off*/, seek_dir /*dir*/)
      override
      {
        noadl::sprintf_and_throw<invalid_argument>(
            "basic_tinybuf_ln: linear buffer not seekable");
      }

    // Reads some characters from the stream. If the end of stream has been
    // reached, zero is returned.
    // In case of an error, an exception is thrown, and the stream is left in an
    // unspecified state.
    virtual
    size_t
    getn(char_type* s, size_t n)
      override
      {
        if(!(this->m_mode & tinybuf_base::open_read))
          noadl::sprintf_and_throw<invalid_argument>(
             "tinybuf_ln: stream not readable");

        return this->m_ln.getn(s, n);
      }

    // Reads a single character from the stream. If the end of stream has been
    // reached, `-1` is returned.
    // In case of an error, an exception is thrown, and the stream is left in an
    // unspecified state.
    virtual
    int
    getc()
      override
      {
        if(!(this->m_mode & tinybuf_base::open_read))
          noadl::sprintf_and_throw<invalid_argument>(
              "tinybuf_ln: stream not readable");

        return this->m_ln.getc();
      }

    // Puts some characters into the stream.
    // In case of an error, an exception is thrown, and the stream is left in an
    // unspecified state.
    virtual
    basic_tinybuf_ln&
    putn(const char_type* s, size_t n)
      override
      {
        if(!(this->m_mode & tinybuf_base::open_write))
          noadl::sprintf_and_throw<invalid_argument>(
              "tinybuf_ln: stream not writable");

        this->m_ln.putn(s, n);
        return *this;
      }

    // Puts a single character into the stream.
    // In case of an error, an exception is thrown, and the stream is left in an
    // unspecified state.
    virtual
    basic_tinybuf_ln&
    putc(char_type c)
      override
      {
        if(!(this->m_mode & tinybuf_base::open_write))
          noadl::sprintf_and_throw<invalid_argument>(
                   "tinybuf_ln: stream not writable");

        this->m_ln.putc(c);
        return *this;
      }
  };

template<typename charT, typename allocT>
basic_tinybuf_ln<charT, allocT>::
~basic_tinybuf_ln()
  {
  }

template<typename charT, typename allocT>
inline
void
swap(basic_tinybuf_ln<charT, allocT>& lhs, basic_tinybuf_ln<charT, allocT>& rhs)
  noexcept(noexcept(lhs.swap(rhs)))
  { lhs.swap(rhs);  }

using tinybuf_ln     = basic_tinybuf_ln<char>;
using wtinybuf_ln    = basic_tinybuf_ln<wchar_t>;
using u16tinybuf_ln  = basic_tinybuf_ln<char16_t>;
using u32tinybuf_ln  = basic_tinybuf_ln<char32_t>;

extern template class basic_tinybuf_ln<char>;
extern template class basic_tinybuf_ln<wchar_t>;
extern template class basic_tinybuf_ln<char16_t>;
extern template class basic_tinybuf_ln<char32_t>;

}  // namespace rocket
#endif
