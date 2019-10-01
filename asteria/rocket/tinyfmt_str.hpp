// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_TINYFMT_STR_HPP_
#define ROCKET_TINYFMT_STR_HPP_

#include "cow_stringbuf.hpp"
#include "tinyfmt.hpp"

namespace rocket {

template<typename charT, typename traitsT = char_traits<charT>,
         typename allocatorT = allocator<charT>> class basic_tinyfmt_str;

template<typename charT, typename traitsT,
         typename allocatorT> class basic_tinyfmt_str : public basic_tinyfmt<charT, traitsT>
  {
  public:
    using char_type       = charT;
    using traits_type     = traitsT;
    using allocator_type  = allocatorT;

    using int_type   = typename traits_type::int_type;
    using pos_type   = typename traits_type::pos_type;
    using off_type   = typename traits_type::off_type;

    // N.B. These are non-standard extensions.
    using streambuf_type   = basic_cow_stringbuf<charT, traitsT, allocatorT>;
    using string_type      = basic_cow_string<charT, traitsT, allocatorT>;
    using stream_type      = basic_tinyfmt<charT, traitsT>;

    using size_type        = typename string_type::size_type;
    using difference_type  = typename string_type::difference_type;

    static constexpr size_type npos = string_type::npos;

  private:
    mutable streambuf_type m_sb;

  public:
    basic_tinyfmt_str()
      : stream_type(&(this->m_sb)),
        m_sb(ios_base::out)
      {
      }
    explicit basic_tinyfmt_str(ios_base::openmode which)
      : stream_type(&(this->m_sb)),
        m_sb(which | ios_base::out)
      {
      }
    explicit basic_tinyfmt_str(string_type str, ios_base::openmode which = ios_base::out)
      : stream_type(&(this->m_sb)),
        m_sb(noadl::move(str), which | ios_base::out)
      {
      }
    basic_tinyfmt_str(basic_tinyfmt_str&& other) noexcept
      : stream_type(static_cast<stream_type&&>(other)),
        m_sb(noadl::move(other.m_sb))
      {
        this->stream_type::set_rdbuf(&(this->m_sb));
      }
    basic_tinyfmt_str& operator=(basic_tinyfmt_str&& other) noexcept
      {
        this->stream_type::operator=(static_cast<stream_type&&>(other));
        this->m_sb = noadl::move(other.m_sb);
        return *this;
      }
    ~basic_tinyfmt_str() override;

  public:
    streambuf_type* rdbuf() const noexcept
      {
        return &(this->m_sb);
      }

    const string_type& get_string() const noexcept
      {
        return this->m_sb.get_string();
      }
    const char_type* get_cstring() const noexcept
      {
        return this->m_sb.get_cstring();
      }
    basic_tinyfmt_str& set_string(string_type str)
      {
        return this->m_sb.set_string(noadl::move(str)), *this;
      }
    basic_tinyfmt_str& clear_string()
      {
        return this->m_sb.clear_string(), *this;
      }
    string_type extract_string()
      {
        return this->m_sb.extract_string();
      }

    void swap(basic_tinyfmt_str& other)
      {
        this->stream_type::swap(other);
        this->m_sb.swap(other.m_sb);
      }
  };

#if !(defined(__cpp_inline_variables) && (__cpp_inline_variables >= 201606))
template<typename charT, typename traitsT,
         typename allocatorT> constexpr typename basic_tinyfmt_str<charT, traitsT,
                                                                   allocatorT>::size_type basic_tinyfmt_str<charT, traitsT,
                                                                                                            allocatorT>::npos;
#endif

template<typename charT, typename traitsT,
         typename allocatorT> basic_tinyfmt_str<charT, traitsT, allocatorT>::~basic_tinyfmt_str()
  = default;

template<typename charT, typename traitsT,
         typename allocatorT> void swap(basic_tinyfmt_str<charT, traitsT, allocatorT>& lhs,
                                        basic_tinyfmt_str<charT, traitsT, allocatorT>& rhs)
  {
    return lhs.swap(rhs);
  }

extern template class basic_tinyfmt_str<char>;
extern template class basic_tinyfmt_str<wchar_t>;

using tinyfmt_str = basic_tinyfmt_str<char>;
using wtinyfmt_str = basic_tinyfmt_str<wchar_t>;

}  // namespace rocket

#endif
