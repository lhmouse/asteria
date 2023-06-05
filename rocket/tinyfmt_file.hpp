// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_TINYFMT_FILE_
#define ROCKET_TINYFMT_FILE_

#include "tinyfmt.hpp"
#include "tinybuf_file.hpp"
namespace rocket {

template<typename charT>
class basic_tinyfmt_file
  : public basic_tinyfmt<charT>
  {
  public:
    using char_type     = charT;
    using seek_dir      = tinybuf_base::seek_dir;
    using open_mode     = tinybuf_base::open_mode;
    using tinyfmt_type  = basic_tinyfmt<charT>;
    using tinybuf_type  = basic_tinybuf_file<charT>;

    using file_type     = typename tinybuf_type::file_type;
    using handle_type   = typename tinybuf_type::handle_type;
    using closer_type   = typename tinybuf_type::closer_type;

  private:
    tinybuf_type m_buf;

  public:
    // Constructs the buffer object. The fmt part is stateless and is always
    // default-constructed.
    basic_tinyfmt_file()
      noexcept(is_nothrow_default_constructible<tinybuf_type>::value)
      {
      }

    template<typename... paramsT,
    ROCKET_ENABLE_IF(is_constructible<tinybuf_type, paramsT&&...>::value)>
    explicit
    basic_tinyfmt_file(paramsT&&... params)
      noexcept(is_nothrow_constructible<tinybuf_type, paramsT&&...>::value)
      : m_buf(::std::forward<paramsT>(params)...)
      {
      }

    basic_tinyfmt_file&
    swap(basic_tinyfmt_file& other)
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
    ~basic_tinyfmt_file() override;

    // Gets the file pointer.
    handle_type
    get_handle() const noexcept
      { return this->m_buf.get_handle();  }

    // Gets the file closer function.
    const closer_type&
    get_closer() const noexcept
      { return this->m_buf.get_closer();  }

    closer_type&
    get_closer() noexcept
      { return this->m_buf.get_closer();  }

    // Takes ownership of an existent file.
    basic_tinyfmt_file&
    reset(file_type&& file) noexcept
      {
        this->m_buf.reset(::std::move(file));
        return *this;
      }

    basic_tinyfmt_file&
    reset(handle_type fp, closer_type cl) noexcept
      {
        this->m_buf.reset(fp, cl);
        return *this;
      }

    // Opens a new file.
    basic_tinyfmt_file&
    open(const char* path, open_mode mode)
      {
        this->m_buf.open(path, mode);
        return *this;
      }

    // Closes the current file, if any.
    basic_tinyfmt_file&
    close() noexcept
      {
        this->m_buf.close();
        return *this;
      }
  };

template<typename charT>
basic_tinyfmt_file<charT>::
~basic_tinyfmt_file()
  {
  }

template<typename charT>
inline
void
swap(basic_tinyfmt_file<charT>& lhs, basic_tinyfmt_file<charT>& rhs)
  noexcept(noexcept(lhs.swap(rhs)))
  {
    lhs.swap(rhs);
  }

using tinyfmt_file     = basic_tinyfmt_file<char>;
using wtinyfmt_file    = basic_tinyfmt_file<wchar_t>;
using u16tinyfmt_file  = basic_tinyfmt_file<char16_t>;
using u32tinyfmt_file  = basic_tinyfmt_file<char32_t>;

extern template class basic_tinyfmt_file<char>;
extern template class basic_tinyfmt_file<wchar_t>;
extern template class basic_tinyfmt_file<char16_t>;
extern template class basic_tinyfmt_file<char32_t>;

}  // namespace rocket
#endif
