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

    using tinyfmt_type  = basic_tinyfmt<charT, traitsT>;
    using buffer_type   = basic_tinybuf_file<charT, traitsT, allocT>;
    using file_handle   = typename buffer_type::file_handle;
    using closer_type   = typename buffer_type::closer_type;

    using seek_dir   = typename buffer_type::seek_dir;
    using open_mode  = typename buffer_type::open_mode;
    using int_type   = typename buffer_type::int_type;
    using off_type   = typename buffer_type::off_type;
    using size_type  = typename buffer_type::size_type;

  private:
    mutable buffer_type m_buf;

  public:
    basic_tinyfmt_file() noexcept
      :
        m_buf()
      { }
    explicit basic_tinyfmt_file(const char* path, open_mode mode) noexcept
      :
        m_buf(path, mode)
      { }
    basic_tinyfmt_file(file_handle fp, const closer_type& cl) noexcept
      :
        m_buf(fp, cl)
      { }
    ~basic_tinyfmt_file() override;

    basic_tinyfmt_file(basic_tinyfmt_file&&)
      = default;
    basic_tinyfmt_file& operator=(basic_tinyfmt_file&&)
      = default;

  public:
    buffer_type& get_buffer() const override
      {
        return this->m_buf;
      }

    file_handle get_handle() const noexcept
      {
        return this->m_buf.get_handle();
      }
    basic_tinyfmt_file& open(const char* path, open_mode mode = tinybuf_base::open_write)
      {
        return this->m_buf.open(path, mode), *this;
      }
    basic_tinyfmt_file& reset(file_handle fp, const closer_type& cl)
      {
        return this->m_buf.reset(fp, cl), *this;
      }
    basic_tinyfmt_file& close()
      {
        return this->m_buf.close(), *this;
      }

    void swap(basic_tinyfmt_file& other) noexcept(is_nothrow_swappable<buffer_type>::value)
      {
        noadl::adl_swap(this->m_buf, other.m_buf);
      }
  };

template<typename charT, typename traitsT, typename allocT>
    basic_tinyfmt_file<charT, traitsT, allocT>::~basic_tinyfmt_file()
  = default;

template<typename charT, typename traitsT, typename allocT>
    inline void swap(basic_tinyfmt_file<charT, traitsT, allocT>& lhs,
                     basic_tinyfmt_file<charT, traitsT, allocT>& rhs) noexcept(noexcept(lhs.swap(rhs)))
  {
    return lhs.swap(rhs);
  }

extern template class basic_tinyfmt_file<char>;
extern template class basic_tinyfmt_file<wchar_t>;

using tinyfmt_file   = basic_tinyfmt_file<char>;
using wtinyfmt_file  = basic_tinyfmt_file<wchar_t>;

}  // namespace rocket

#endif
