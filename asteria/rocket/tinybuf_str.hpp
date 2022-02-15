// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_TINYBUF_STR_HPP_
#define ROCKET_TINYBUF_STR_HPP_

#include "tinybuf.hpp"
#include "cow_string.hpp"

namespace rocket {

template<typename charT, typename traitsT = char_traits<charT>, typename allocT = allocator<charT>>
class basic_tinybuf_str;

template<typename charT, typename traitsT, typename allocT>
class basic_tinybuf_str
  : public basic_tinybuf<charT, traitsT>
  {
  public:
    using char_type       = charT;
    using traits_type     = traitsT;
    using allocator_type  = allocT;

    using tinybuf_type  = basic_tinybuf<charT, traitsT>;
    using string_type   = basic_cow_string<charT, traitsT, allocT>;

    using seek_dir   = typename tinybuf_type::seek_dir;
    using open_mode  = typename tinybuf_type::open_mode;
    using int_type   = typename tinybuf_type::int_type;
    using off_type   = typename tinybuf_type::off_type;
    using size_type  = typename tinybuf_type::size_type;

  private:
    string_type m_str;
    off_type m_off;
    open_mode m_mode;

  public:
    basic_tinybuf_str() noexcept(is_nothrow_constructible<string_type>::value)
      : m_str(), m_off(), m_mode()
      { }

    explicit
    basic_tinybuf_str(const allocator_type& alloc) noexcept
      : m_str(alloc), m_off(), m_mode()
      { }

    explicit
    basic_tinybuf_str(open_mode mode, const allocator_type& alloc = allocator_type()) noexcept
      : m_str(alloc), m_off(), m_mode(mode)
      { }

    template<typename xstrT,
    ROCKET_ENABLE_IF(is_constructible<string_type, xstrT&&>::value)>
    explicit
    basic_tinybuf_str(xstrT&& xstr, open_mode mode, const allocator_type& alloc = allocator_type())
      : m_str(::std::forward<xstrT>(xstr)), m_off(), m_mode(mode)
      { }

    basic_tinybuf_str&
    swap(basic_tinybuf_str& other) noexcept(is_nothrow_swappable<string_type>::value)
      { noadl::xswap(this->m_str, other.m_str);
        noadl::xswap(this->m_off, other.m_off);
        noadl::xswap(this->m_mode, other.m_mode);
        return *this;  }

  protected:
    void
    do_flush() override
      { }

    off_type
    do_tell() const override
      { return this->m_off;  }

    void
    do_seek(off_type off, seek_dir dir) override
      {
        // Get the seek origin.
        off_type orig = 0;
        if(dir == tinybuf_type::seek_cur)
          orig = this->m_off;
        else if(dir == tinybuf_type::seek_end)
          orig = static_cast<off_type>(this->m_str.size());

        // Calculate the target offset.
        off_type targ;
        if(ROCKET_ADD_OVERFLOW(orig, off, ::std::addressof(targ)))
          noadl::sprintf_and_throw<out_of_range>(
                "tinybuf_str: stream offset overflow (operands were `%lld` and `%lld`)",
                static_cast<long long>(orig), static_cast<long long>(off));

        if(targ < 0)
          noadl::sprintf_and_throw<out_of_range>("tinybuf_str: negative stream offset");

        // Set the new stream offset. Note it is valid to write past the end.
        this->m_off = targ;
      }

    size_type
    do_getn(char_type* s, size_type n) override
      {
        if(!(this->m_mode & tinybuf_base::open_read))
          noadl::sprintf_and_throw<out_of_range>("tinybuf_str: stream not readable");

        // Fix stream offset.
        size_type roff = 0;
        if(this->m_off > this->m_str.ssize()) {
          // End of stream has been reached.
          return 0;
        }
        else if(this->m_off > 0)
          roff = static_cast<size_type>(this->m_off);

        // Copy the substring and update stream offset accordingly.
        size_type r = this->m_str.copy(roff, s, n);
        this->m_off = static_cast<off_type>(roff + r);
        return r;
      }

    int_type
    do_getc() override
      {
        char_type c;
        if(this->basic_tinybuf_str::do_getn(::std::addressof(c), 1) == 0)
          return traits_type::eof();
        return traits_type::to_int_type(c);
      }

    void
    do_putn(const char_type* s, size_type n) override
      {
        if(!(this->m_mode & tinybuf_base::open_write))
          noadl::sprintf_and_throw<out_of_range>("tinybuf_str: stream not writable");

        // Fix stream offset.
        size_type roff = 0;
        if(this->m_mode & tinybuf_base::open_append) {
          // Override any existent offset.
          roff = this->m_str.size();
        }
        else if(this->m_off > this->m_str.ssize()) {
          // Append null characters.
          off_type nadd;
          if(ROCKET_SUB_OVERFLOW(this->m_off, this->m_str.ssize(), ::std::addressof(nadd)))
            noadl::sprintf_and_throw<invalid_argument>("tinybuf_str: string length overflow");

          if(nadd > static_cast<off_type>(this->m_str.max_size()))
            noadl::sprintf_and_throw<invalid_argument>("tinybuf_str: string too long");

          this->m_str.append(static_cast<size_t>(nadd), char_type());
          ROCKET_ASSERT(this->m_off == this->m_str.ssize());
        }
        else if(this->m_off > 0)
          roff = static_cast<size_type>(this->m_off);

        // Insert the string and update stream offset accordingly.
        this->m_str.replace(roff, n, s, n);
        this->m_off = static_cast<off_type>(roff + n);
      }

    void
    do_putc(char_type c) override
      {
        this->basic_tinybuf_str::do_putn(::std::addressof(c), 1);
      }

  public:
    ~basic_tinybuf_str() override;

    const string_type&
    str() const noexcept
      { return this->m_str;  }

    const char_type*
    c_str() const noexcept
      { return this->m_str.c_str();  }

    size_type
    length() const noexcept
      { return this->m_str.length();  }

    const string_type&
    get_string() const noexcept
      { return this->m_str;  }

    template<typename xstrT>
    void
    set_string(xstrT&& xstr, open_mode mode)
      {
        this->m_str = ::std::forward<xstrT>(xstr);
        this->m_off = 0;
        this->m_mode = mode;
      }

    void
    clear_string(open_mode mode)
      {
        this->m_str.clear();
        this->m_off = 0;
        this->m_mode = mode;
      }

    string_type
    extract_string(open_mode mode)
      {
        string_type r = ::std::move(this->m_str);
        this->clear_string(mode);
        return r;
      }
  };

template<typename charT, typename traitsT, typename allocT>
basic_tinybuf_str<charT, traitsT, allocT>::
~basic_tinybuf_str()
  { }

template<typename charT, typename traitsT, typename allocT>
inline void
swap(basic_tinybuf_str<charT, traitsT, allocT>& lhs, basic_tinybuf_str<charT, traitsT, allocT>& rhs)
  noexcept(noexcept(lhs.swap(rhs)))
  { lhs.swap(rhs);  }

extern template
class basic_tinybuf_str<char>;

extern template
class basic_tinybuf_str<wchar_t>;

using tinybuf_str   = basic_tinybuf_str<char>;
using wtinybuf_str  = basic_tinybuf_str<wchar_t>;

}  // namespace rocket

#endif
