// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_TINYFMT_STR_HPP_
#define ROCKET_TINYFMT_STR_HPP_

#include "tinyfmt.hpp"
#include "tinybuf_str.hpp"

namespace rocket {

template<typename charT, typename traitsT = char_traits<charT>, typename allocT = allocator<charT>>
class basic_tinyfmt_str;

template<typename charT, typename traitsT, typename allocT>
class basic_tinyfmt_str
  : public basic_tinyfmt<charT, traitsT>
  {
  public:
    using char_type       = charT;
    using traits_type     = traitsT;
    using allocator_type  = allocT;

    using tinyfmt_type  = basic_tinyfmt<charT, traitsT>;
    using tinybuf_type  = basic_tinybuf_str<charT, traitsT>;
    using string_type   = typename tinybuf_type::string_type;

    using seek_dir   = typename tinybuf_type::seek_dir;
    using open_mode  = typename tinybuf_type::open_mode;
    using int_type   = typename tinybuf_type::int_type;
    using off_type   = typename tinybuf_type::off_type;
    using size_type  = typename tinybuf_type::size_type;

  private:
    mutable tinybuf_type m_buf;

  public:
    basic_tinyfmt_str() noexcept
      : m_buf(tinybuf_base::open_write)
      { }

    explicit
    basic_tinyfmt_str(open_mode mode) noexcept
      : m_buf(mode)
      { }

    template<typename xstrT,
    ROCKET_ENABLE_IF(is_constructible<string_type, xstrT&&>::value)>
    explicit
    basic_tinyfmt_str(xstrT&& xstr, open_mode mode = tinybuf_base::open_write)
      : m_buf(::std::forward<xstrT>(xstr), mode)
      { }

    basic_tinyfmt_str&
    swap(basic_tinyfmt_str& other) noexcept(is_nothrow_swappable<tinybuf_type>::value)
      { noadl::xswap(this->m_buf, other.m_buf);
        return *this;  }

  public:
    ~basic_tinyfmt_str() override;

    tinybuf_type&
    get_tinybuf() const override
      { return this->m_buf;  }

    const string_type&
    get_string() const noexcept
      { return this->m_buf.get_string();  }

    const char_type*
    c_str() const noexcept
      { return this->m_buf.c_str();  }

    size_type
    length() const noexcept
      { return this->m_buf.length();  }

    basic_tinyfmt_str&
    clear_string(open_mode mode = tinybuf_base::open_write)
      {
        this->m_buf.clear_string(mode);
        return *this;
      }

    template<typename xstrT>
    basic_tinyfmt_str&
    set_string(xstrT&& xstr, open_mode mode = tinybuf_base::open_write)
      {
        this->m_buf.set_string(::std::forward<xstrT>(xstr), mode);
        return *this;
      }

    string_type
    extract_string(open_mode mode = tinybuf_base::open_write)
      {
        return this->m_buf.extract_string(mode);
      }
  };

template<typename charT, typename traitsT, typename allocT>
basic_tinyfmt_str<charT, traitsT, allocT>::
~basic_tinyfmt_str()
  { }

template<typename charT, typename traitsT, typename allocT>
inline void
swap(basic_tinyfmt_str<charT, traitsT, allocT>& lhs, basic_tinyfmt_str<charT, traitsT, allocT>& rhs)
  noexcept(noexcept(lhs.swap(rhs)))
  { lhs.swap(rhs);  }

extern template
class basic_tinyfmt_str<char>;

extern template
class basic_tinyfmt_str<wchar_t>;

using tinyfmt_str   = basic_tinyfmt_str<char>;
using wtinyfmt_str  = basic_tinyfmt_str<wchar_t>;

}  // namespace rocket

#endif
