// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_COW_STRINGSTREAM_HPP_
#define ROCKET_COW_STRINGSTREAM_HPP_

#include "cow_stringbuf.hpp"
#include <istream>  // std::basic_iostream<>

namespace rocket {

template<typename charT, typename traitsT = char_traits<charT>, typename allocatorT = allocator<charT>> class basic_cow_stringstream;

template<typename charT, typename traitsT, typename allocatorT> class basic_cow_stringstream : public basic_iostream<charT, traitsT>
  {
  public:
    using char_type       = charT;
    using traits_type     = traitsT;
    using allocator_type  = allocatorT;

    using int_type   = typename traits_type::int_type;
    using pos_type   = typename traits_type::pos_type;
    using off_type   = typename traits_type::off_type;

    // N.B. These are non-standard extensions.
    using streambuf_type   = basic_cow_stringbuf<char_type, traits_type, allocator_type>;
    using string_type      = basic_cow_string<char_type, traits_type, allocator_type>;
    using size_type        = typename string_type::size_type;
    using difference_type  = typename string_type::difference_type;

    static constexpr size_type npos = string_type::npos;

  private:
    mutable streambuf_type m_sb;

  public:
    explicit basic_cow_stringstream(string_type str, size_type caret = npos, ios_base::openmode which = ios_base::in | ios_base::out)
      : basic_iostream<char_type, traits_type>(&(this->m_sb)),
        m_sb(noadl::move(str), caret, which)
      {
      }
    basic_cow_stringstream()
      : basic_cow_stringstream(string_type())
      {
      }
    basic_cow_stringstream(ios_base::openmode which)
      : basic_cow_stringstream(string_type(), npos, which)
      {
      }
    ~basic_cow_stringstream() override;

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
  };

#if !(defined(__cpp_inline_variables) && (__cpp_inline_variables >= 201606))
template<typename charT, typename traitsT, typename allocatorT> constexpr typename basic_cow_stringstream<charT, traitsT, allocatorT>::size_type basic_cow_stringstream<charT, traitsT, allocatorT>::npos;
#endif

template<typename charT, typename traitsT, typename allocatorT> basic_cow_stringstream<charT, traitsT, allocatorT>::~basic_cow_stringstream()
  = default;

extern template class basic_cow_stringstream<char>;
extern template class basic_cow_stringstream<wchar_t>;

using cow_stringstream = basic_cow_stringstream<char>;
using cow_wstringstream = basic_cow_stringstream<wchar_t>;

}  // namespace rocket

#endif
