// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_TINYFMT_LN_
#define ROCKET_TINYFMT_LN_

#include "tinyfmt.hpp"
#include "tinybuf_ln.hpp"
namespace rocket {

template<typename charT, typename allocT = allocator<charT>>
class basic_tinyfmt_ln
  :
    public basic_tinyfmt<charT>
  {
  public:
    using char_type     = charT;
    using seek_dir      = tinybuf_base::seek_dir;
    using open_mode     = tinybuf_base::open_mode;
    using tinyfmt_type  = basic_tinyfmt<charT>;
    using tinybuf_type  = basic_tinybuf_ln<charT>;

    using buffer_type     = typename tinybuf_type::buffer_type;
    using allocator_type  = typename tinybuf_type::allocator_type;

  private:
    tinybuf_type m_buf;

  public:
    // Constructs the buffer object. The fmt part is stateless and is always
    // default-constructed.
    basic_tinyfmt_ln()
      noexcept(is_nothrow_default_constructible<tinybuf_type>::value)
      { }

    explicit basic_tinyfmt_ln(const buffer_type& ln)
      noexcept(is_nothrow_constructible<tinybuf_type, open_mode, const buffer_type&>::value)
      :
        m_buf(ln, tinybuf_base::open_read_write)
      { }

    explicit basic_tinyfmt_ln(buffer_type&& ln)
      noexcept(is_nothrow_constructible<tinybuf_type, open_mode, buffer_type&&>::value)
      :
        m_buf(move(ln), tinybuf_base::open_read_write)
      { }

    basic_tinyfmt_ln&
    swap(basic_tinyfmt_ln& other)
      noexcept(is_nothrow_swappable<tinybuf_type>::value)
      {
        this->m_buf.swap(other.m_buf);
        return *this;
      }

  protected:
    // Gets the associated buffer.
    ROCKET_PURE virtual
    tinybuf_type&
    do_get_tinybuf_nonconst()
      const override
      {
        return const_cast<tinybuf_type&>(this->m_buf);
      }

  public:
    virtual ~basic_tinyfmt_ln() override;

    // Gets the internal buffer.
    const buffer_type&
    get_buffer()
      const noexcept
      { return this->m_buf.get_buffer();  }

    const char_type*
    data()
      const noexcept
      { return this->m_buf.data();  }

    size_t
    size()
      const noexcept
      { return this->m_buf.size();  }

    ptrdiff_t
    ssize()
      const noexcept
      { return this->m_buf.ssize();  }

    const char_type*
    begin()
      const noexcept
      { return this->m_buf.begin();  }

    const char_type*
    end()
      const noexcept
      { return this->m_buf.end();  }

    // Replaces the internal buffer.
    template<typename xlnT>
    basic_tinyfmt_ln&
    set_buffer(xlnT&& xln, open_mode mode)
      {
        this->m_buf.set_buffer(forward<xlnT>(xln), mode);
        return *this;
      }

    template<typename xlnT>
    basic_tinyfmt_ln&
    set_buffer(xlnT&& xln)
      {
        this->m_buf.set_buffer(forward<xlnT>(xln));
        return *this;
      }

    basic_tinyfmt_ln&
    clear_buffer()
      {
        this->m_buf.clear_buffer();
        return *this;
      }

    buffer_type
    extract_buffer()
      {
        return this->m_buf.extract_buffer();
      }
  };

template<typename charT, typename allocT>
basic_tinyfmt_ln<charT, allocT>::
~basic_tinyfmt_ln()
  {
  }

template<typename charT, typename allocT>
inline
void
swap(basic_tinyfmt_ln<charT, allocT>& lhs, basic_tinyfmt_ln<charT, allocT>& rhs)
  noexcept(noexcept(lhs.swap(rhs)))
  { lhs.swap(rhs);  }

using tinyfmt_ln     = basic_tinyfmt_ln<char>;
using wtinyfmt_ln    = basic_tinyfmt_ln<wchar_t>;
using u16tinyfmt_ln  = basic_tinyfmt_ln<char16_t>;
using u32tinyfmt_ln  = basic_tinyfmt_ln<char32_t>;

extern template class basic_tinyfmt_ln<char>;
extern template class basic_tinyfmt_ln<wchar_t>;
extern template class basic_tinyfmt_ln<char16_t>;
extern template class basic_tinyfmt_ln<char32_t>;

}  // namespace rocket
#endif
