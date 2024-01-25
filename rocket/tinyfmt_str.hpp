// This str is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_TINYFMT_STR_
#define ROCKET_TINYFMT_STR_

#include "tinyfmt.hpp"
#include "tinybuf_str.hpp"
namespace rocket {

template<typename charT, typename allocT = allocator<charT>>
class basic_tinyfmt_str
  :
    public basic_tinyfmt<charT>
  {
  public:
    using char_type     = charT;
    using seek_dir      = tinybuf_base::seek_dir;
    using open_mode     = tinybuf_base::open_mode;
    using tinyfmt_type  = basic_tinyfmt<charT>;
    using tinybuf_type  = basic_tinybuf_str<charT>;

    using string_type     = typename tinybuf_type::string_type;
    using allocator_type  = typename tinybuf_type::allocator_type;

  private:
    tinybuf_type m_buf;

  public:
    // Constructs the buffer object. The fmt part is stateless and is always
    // default-constructed.
    basic_tinyfmt_str()
      noexcept(is_nothrow_default_constructible<tinybuf_type>::value)
      { }

    explicit basic_tinyfmt_str(const string_type& str)
      noexcept(is_nothrow_constructible<tinybuf_type, open_mode, const string_type&>::value)
      :
        m_buf(str, tinybuf_base::open_read_write)
      { }

    explicit basic_tinyfmt_str(string_type&& str)
      noexcept(is_nothrow_constructible<tinybuf_type, open_mode, string_type&&>::value)
      :
        m_buf(move(str), tinybuf_base::open_read_write)
      { }

    basic_tinyfmt_str&
    swap(basic_tinyfmt_str& other)
      noexcept(is_nothrow_swappable<tinybuf_type>::value)
      {
        this->m_buf.swap(other.m_buf);
        return *this;
      }

  protected:
    // Gets the associated buffer.
    ROCKET_PURE virtual
    tinybuf_type&
    do_get_tinybuf_nonconst() const override
      {
        return const_cast<tinybuf_type&>(this->m_buf);
      }

  public:
    virtual
    ~basic_tinyfmt_str() override;

    // Gets the internal string.
    const string_type&
    get_string() const noexcept
      { return this->m_buf.get_string();  }

    const char_type*
    c_str() const noexcept
      { return this->m_buf.c_str();  }

    size_t
    length() const noexcept
      { return this->m_buf.length();  }

    const char_type*
    data() const noexcept
      { return this->m_buf.data();  }

    size_t
    size() const noexcept
      { return this->m_buf.size();  }

    ptrdiff_t
    ssize() const noexcept
      { return this->m_buf.ssize();  }

    size_t
    capacity() const noexcept
      { return this->m_buf.capacity();  }

    basic_tinyfmt_str&
    reserve(size_t res_arg)
      {
        this->m_buf.reserve(res_arg);
        return *this;
      }

    // Replaces the internal string.
    template<typename xstrT>
    basic_tinyfmt_str&
    set_string(xstrT&& xstr, open_mode mode)
      {
        this->m_buf.set_string(forward<xstrT>(xstr), mode);
        return *this;
      }

    template<typename xstrT>
    basic_tinyfmt_str&
    set_string(xstrT&& xstr)
      {
        this->m_buf.set_string(forward<xstrT>(xstr));
        return *this;
      }

    basic_tinyfmt_str&
    clear_string()
      {
        this->m_buf.clear_string();
        return *this;
      }

    string_type&&
    extract_string()
      {
        return this->m_buf.extract_string();
      }
  };

template<typename charT, typename allocT>
basic_tinyfmt_str<charT, allocT>::
~basic_tinyfmt_str()
  {
  }

template<typename charT, typename allocT>
inline
void
swap(basic_tinyfmt_str<charT, allocT>& lhs, basic_tinyfmt_str<charT, allocT>& rhs)
  noexcept(noexcept(lhs.swap(rhs)))
  { lhs.swap(rhs);  }

using tinyfmt_str     = basic_tinyfmt_str<char>;
using wtinyfmt_str    = basic_tinyfmt_str<wchar_t>;
using u16tinyfmt_str  = basic_tinyfmt_str<char16_t>;
using u32tinyfmt_str  = basic_tinyfmt_str<char32_t>;

extern template class basic_tinyfmt_str<char>;
extern template class basic_tinyfmt_str<wchar_t>;
extern template class basic_tinyfmt_str<char16_t>;
extern template class basic_tinyfmt_str<char32_t>;

}  // namespace rocket
#endif
