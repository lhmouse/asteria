// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_COW_ISTRINGSTREAM_HPP_
#define ROCKET_COW_ISTRINGSTREAM_HPP_

#include "cow_stringbuf.hpp"
#include <istream>  // std::basic_istream<>

namespace rocket {

template<typename charT, typename traitsT = char_traits<charT>, typename allocatorT = allocator<charT>> class basic_cow_istringstream;

template<typename charT, typename traitsT, typename allocatorT> class basic_cow_istringstream : public basic_istream<charT, traitsT>
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
    using stream_type      = basic_istream<charT, traitsT>;

    using size_type        = typename string_type::size_type;
    using difference_type  = typename string_type::difference_type;

    static constexpr size_type npos = string_type::npos;

  private:
    mutable streambuf_type m_sb;

  public:
    basic_cow_istringstream()
      : stream_type(&(this->m_sb)),
        m_sb(ios_base::in)
      {
      }
    explicit basic_cow_istringstream(ios_base::openmode which)
      : stream_type(&(this->m_sb)),
        m_sb(which | ios_base::in)
      {
      }
    explicit basic_cow_istringstream(string_type str, ios_base::openmode which = ios_base::in)
      : stream_type(&(this->m_sb)),
        m_sb(noadl::move(str), which | ios_base::in)
      {
      }
    basic_cow_istringstream(basic_cow_istringstream&& other) noexcept
      : stream_type(static_cast<stream_type&&>(other)),
        m_sb(noadl::move(other.m_sb))
      {
        this->stream_type::set_rdbuf(&(this->m_sb));
      }
    basic_cow_istringstream& operator=(basic_cow_istringstream&& other) noexcept
      {
        this->stream_type::operator=(static_cast<stream_type&&>(other));
        this->m_sb = noadl::move(other.m_sb);
        return *this;
      }
    ~basic_cow_istringstream() override;

  public:
    streambuf_type* rdbuf() const noexcept
      {
        return &(this->m_sb);
      }

    const string_type& get_string() const
      {
        return this->rdbuf()->get_string();
      }
    void set_string(string_type str)
      {
        return this->rdbuf()->set_string(noadl::move(str));
      }
    string_type extract_string()
      {
        return this->rdbuf()->extract_string();
      }

    void swap(basic_cow_istringstream& other) noexcept
      {
        this->stream_type::swap(other);
        this->m_sb.swap(other.m_sb);
      }
  };

#if !(defined(__cpp_inline_variables) && (__cpp_inline_variables >= 201606))
template<typename charT, typename traitsT, typename allocatorT> constexpr typename basic_cow_istringstream<charT, traitsT, allocatorT>::size_type basic_cow_istringstream<charT, traitsT, allocatorT>::npos;
#endif

template<typename charT, typename traitsT, typename allocatorT> basic_cow_istringstream<charT, traitsT, allocatorT>::~basic_cow_istringstream()
  = default;

template<typename charT, typename traitsT, typename allocatorT> void swap(basic_cow_istringstream<charT, traitsT, allocatorT>& lhs,
                                                                          basic_cow_istringstream<charT, traitsT, allocatorT>& rhs) noexcept
  {
    lhs.swap(rhs);
  }

extern template class basic_cow_istringstream<char>;
extern template class basic_cow_istringstream<wchar_t>;

using cow_istringstream = basic_cow_istringstream<char>;
using cow_wistringstream = basic_cow_istringstream<wchar_t>;

}  // namespace rocket

#endif
