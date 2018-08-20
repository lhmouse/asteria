// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_INSERTABLE_IOSTREAM_HPP_
#define ROCKET_INSERTABLE_IOSTREAM_HPP_

#include "insertable_streambuf.hpp"
#include <istream> // std::basic_iostream<>

namespace rocket
{

using ::std::basic_iostream;

template<typename charT, typename traitsT = char_traits<charT>, typename allocatorT = allocator<charT>>
  class basic_insertable_iostream;

template<typename charT, typename traitsT, typename allocatorT>
  class basic_insertable_iostream
    : public basic_iostream<charT, traitsT>
    {
    public:
      using string_type  = typename basic_insertable_streambuf<charT, traitsT>::string_type;
      using size_type    = typename basic_insertable_streambuf<charT, traitsT>::size_type;
      using traits_type  = typename basic_iostream<charT, traitsT>::traits_type;
      using char_type    = typename basic_iostream<charT, traitsT>::char_type;
      using int_type     = typename basic_iostream<charT, traitsT>::int_type;

    private:
      basic_insertable_streambuf<charT, traitsT> m_sb;

    public:
      explicit basic_insertable_iostream(ios_base::openmode which = ios_base::in | ios_base::out)
        : basic_iostream<charT, traitsT>(&(this->m_sb))
        , m_sb(which)
        {
        }
      explicit basic_insertable_iostream(string_type str, ios_base::openmode which = ios_base::in | ios_base::out)
        : basic_iostream<charT, traitsT>(&(this->m_sb))
        , m_sb(::std::move(str), which)
        {
        }
      ~basic_insertable_iostream() override;

    public:
      basic_insertable_streambuf<charT, traitsT> * rdbuf() const noexcept
        {
          return const_cast<basic_insertable_streambuf<charT, traitsT> *>(&(this->m_sb));
        }

      const string_type & get_string() const noexcept
        {
          return this->rdbuf()->get_string();
        }
      size_type get_caret() const noexcept
        {
          return this->rdbuf()->get_caret();
        }
      void set_string(string_type str, size_type caret = string_type::npos) noexcept
        {
          return this->rdbuf()->set_string(::std::move(str), caret);
        }
      void set_caret(size_type caret) noexcept
        {
          return this->rdbuf()->set_caret(caret);
        }
      string_type extract_string() noexcept
        {
          return this->rdbuf()->extract_string();
        }
    };

template<typename charT, typename traitsT, typename allocatorT>
  basic_insertable_iostream<charT, traitsT, allocatorT>::~basic_insertable_iostream()
    = default;

extern template class basic_insertable_iostream<char>;
extern template class basic_insertable_iostream<wchar_t>;

using insertable_iostream  = basic_insertable_iostream<char>;
using insertable_wiostream = basic_insertable_iostream<wchar_t>;

}

#endif
