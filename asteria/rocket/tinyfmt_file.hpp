// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_TINYFMT_FILE_HPP_
#define ROCKET_TINYFMT_FILE_HPP_

#include "tinyfmt.hpp"
#include "tinybuf_file.hpp"

namespace rocket {

template<typename charT, typename traitsT = char_traits<charT>,
         typename allocT = allocator<charT>> class basic_tinyfmt_file;

template<typename charT, typename traitsT, typename allocT>
    class basic_tinyfmt_file : public basic_tinyfmt<charT, traitsT>
  {
  public:
    using char_type       = charT;
    using traits_type     = traitsT;
    using allocator_type  = allocT;

    using tinyfmt_type     = basic_tinyfmt<charT, traitsT>;
    using tinybuf_type     = basic_tinybuf_file<charT, traitsT, allocT>;
    using file_handle      = typename tinybuf_type::file_handle;
    using closer_function  = typename tinybuf_type::closer_function;

    using seek_dir   = typename tinybuf_type::seek_dir;
    using open_mode  = typename tinybuf_type::open_mode;
    using int_type   = typename tinybuf_type::int_type;
    using off_type   = typename tinybuf_type::off_type;
    using size_type  = typename tinybuf_type::size_type;

  private:
    mutable tinybuf_type m_buf;

  public:
    basic_tinyfmt_file() noexcept
      :
        m_buf()
      {
      }
    explicit basic_tinyfmt_file(const char* path, open_mode mode = tinybuf_base::open_write) noexcept
      :
        m_buf(path, mode)
      {
      }
    basic_tinyfmt_file(file_handle fp, closer_function* cl) noexcept
      :
        m_buf(fp, cl)
      {
      }
    ~basic_tinyfmt_file() override;

    basic_tinyfmt_file(basic_tinyfmt_file&&)
      = default;
    basic_tinyfmt_file& operator=(basic_tinyfmt_file&&)
      = default;

  public:
    tinybuf_type& get_tinybuf() const override
      {
        return this->m_buf;
      }

    file_handle get_handle() const noexcept
      {
        return this->m_buf.get_handle();
      }
    closer_function* get_closer() const noexcept
      {
        return this->m_buf.get_closer();
      }
    basic_tinyfmt_file& reset(file_handle fp, closer_function* cl) noexcept
      {
        return this->m_buf.reset(fp, cl), *this;
      }
    basic_tinyfmt_file& open(const char* path, open_mode mode = tinybuf_base::open_write)
      {
        return this->m_buf.open(path, mode), *this;
      }
    basic_tinyfmt_file& close() noexcept
      {
        return this->m_buf.close(), *this;
      }

    basic_tinyfmt_file& swap(basic_tinyfmt_file& other) noexcept(is_nothrow_swappable<tinybuf_type>::value)
      {
        xswap(this->m_buf, other.m_buf);
        return *this;
      }
  };

template<typename charT, typename traitsT, typename allocT>
    basic_tinyfmt_file<charT, traitsT, allocT>::~basic_tinyfmt_file()
  = default;

template<typename charT, typename traitsT, typename allocT>
    inline void swap(basic_tinyfmt_file<charT, traitsT, allocT>& lhs,
                     basic_tinyfmt_file<charT, traitsT, allocT>& rhs) noexcept(noexcept(lhs.swap(rhs)))
  {
    lhs.swap(rhs);
  }

extern template class basic_tinyfmt_file<char>;
extern template class basic_tinyfmt_file<wchar_t>;

using tinyfmt_file   = basic_tinyfmt_file<char>;
using wtinyfmt_file  = basic_tinyfmt_file<wchar_t>;

}  // namespace rocket

#endif
