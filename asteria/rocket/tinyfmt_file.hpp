// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_TINYFMT_FILE_HPP_
#define ROCKET_TINYFMT_FILE_HPP_

#include "tinyfmt.hpp"
#include <fstream>  // std::filebuf

namespace rocket {

template<typename charT, typename traitsT = char_traits<charT>> class basic_tinyfmt_file;

template<typename charT, typename traitsT> class basic_tinyfmt_file : public basic_tinyfmt<charT, traitsT>
  {
  public:
    using char_type    = charT;
    using traits_type  = traitsT;

    using int_type   = typename traits_type::int_type;
    using pos_type   = typename traits_type::pos_type;
    using off_type   = typename traits_type::off_type;

    // N.B. These are non-standard extensions.
    using streambuf_type  = ::std::basic_filebuf<charT, traitsT>;
    using stream_type     = basic_tinyfmt<charT, traitsT>;

  private:
    mutable streambuf_type m_sb;

  public:
    basic_tinyfmt_file()
      : stream_type(&(this->m_sb)),
        m_sb()
      {
      }
    explicit basic_tinyfmt_file(const char* filename, ios_base::openmode which = ios_base::out)
      : stream_type(&(this->m_sb)),
        m_sb()
      {
        this->open(filename, which);
      }
    basic_tinyfmt_file(basic_tinyfmt_file&& other) noexcept
      : stream_type(static_cast<stream_type&&>(other)),
        m_sb(noadl::move(other.m_sb))
      {
        this->stream_type::set_rdbuf(&(this->m_sb));
      }
    basic_tinyfmt_file& operator=(basic_tinyfmt_file&& other) noexcept
      {
        this->stream_type::operator=(static_cast<stream_type&&>(other));
        this->m_sb = noadl::move(other.m_sb);
        return *this;
      }
    ~basic_tinyfmt_file() override;

  public:
    streambuf_type* rdbuf() const noexcept
      {
        return &(this->m_sb);
      }

    bool is_open() const
      {
        return this->rdbuf()->is_open();
      }
    basic_tinyfmt_file& open(const char* filename, ios_base::openmode which = ios_base::out)
      {
        if(!this->rdbuf()->open(filename, which)) {
          noadl::sprintf_and_throw<runtime_error>("basic_tinyfmt_file: An error occurred while opening file '%s' with mode `%ld`.",
                                                  filename, static_cast<long>(which));
        }
        return *this;
      }
    basic_tinyfmt_file& close()
      {
        if(!this->rdbuf()->close()) {
          noadl::sprintf_and_throw<runtime_error>("basic_tinyfmt_file: An error occurred while closing the file.");
        }
        return *this;
      }

    void swap(basic_tinyfmt_file& other)
      {
        this->stream_type::swap(other);
        this->m_sb.swap(other.m_sb);
      }
  };

template<typename charT, typename traitsT> basic_tinyfmt_file<charT, traitsT>::~basic_tinyfmt_file()
  = default;

template<typename charT, typename traitsT> void swap(basic_tinyfmt_file<charT, traitsT>& lhs,
                                                     basic_tinyfmt_file<charT, traitsT>& rhs)
  {
    return lhs.swap(rhs);
  }

extern template class basic_tinyfmt_file<char>;
extern template class basic_tinyfmt_file<wchar_t>;

using tinyfmt_file = basic_tinyfmt_file<char>;
using wtinyfmt_file = basic_tinyfmt_file<wchar_t>;

}  // namespace rocket

#endif
