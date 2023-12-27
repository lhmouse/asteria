// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_TINYBUF_STR_
#define ROCKET_TINYBUF_STR_

#include "tinybuf.hpp"
#include "cow_string.hpp"
namespace rocket {

template<typename charT, typename allocT = allocator<charT>>
class basic_tinybuf_str
  :
    public basic_tinybuf<charT>
  {
  public:
    using char_type     = charT;
    using seek_dir      = tinybuf_base::seek_dir;
    using open_mode     = tinybuf_base::open_mode;
    using tinybuf_type  = basic_tinybuf<charT>;

    using string_type     = basic_cow_string<charT, allocT>;
    using allocator_type  = allocT;

  private:
    string_type m_str;
    int64_t m_off = 0;
    open_mode m_mode = tinybuf_base::open_read_write;

  public:
    explicit constexpr
    basic_tinybuf_str(const allocator_type& alloc) noexcept
      :
        m_str(alloc)
      { }

    constexpr
    basic_tinybuf_str() noexcept(is_nothrow_default_constructible<string_type>::value)
      { }

    explicit
    basic_tinybuf_str(open_mode mode, const allocator_type& alloc = allocator_type()) noexcept
      :
        m_str(alloc), m_mode(mode)
      { }

    template<typename xstrT,
    ROCKET_ENABLE_IF(is_constructible<string_type, xstrT&&, const allocator_type&>::value)>
    explicit constexpr
    basic_tinybuf_str(xstrT&& xstr, open_mode mode, const allocator_type& alloc = allocator_type())
      noexcept(is_nothrow_constructible<string_type, xstrT&&, const allocator_type>::value)
      :
        m_str(forward<xstrT>(xstr), alloc), m_mode(mode)
      { }

    // The copy and move constructors are necessary because `basic_tinybuf`
    // is not move-constructible.
    basic_tinybuf_str(const basic_tinybuf_str& other)
      noexcept(is_nothrow_copy_constructible<string_type>::value)
      :
        tinybuf_type(), m_str(other.m_str), m_off(other.m_off), m_mode(other.m_mode)
      { }

    basic_tinybuf_str(const basic_tinybuf_str& other, const allocator_type& alloc) noexcept
      :
        m_str(other.m_str, alloc), m_off(other.m_off), m_mode(other.m_mode)
      { }

    basic_tinybuf_str&
    operator=(const basic_tinybuf_str& other) &
      noexcept(is_nothrow_copy_assignable<string_type>::value)
      {
        this->m_str = other.m_str;
        this->m_off = other.m_off;
        this->m_mode = other.m_mode;
        return *this;
      }

    basic_tinybuf_str(basic_tinybuf_str&& other)
      noexcept(is_nothrow_move_constructible<string_type>::value)
      :
        tinybuf_type(), m_str(move(other.m_str)), m_off(noadl::exchange(other.m_off)),
        m_mode(noadl::exchange(other.m_mode))
      { }

    basic_tinybuf_str(basic_tinybuf_str&& other, const allocator_type& alloc) noexcept
      :
        m_str(move(other.m_str), alloc), m_off(noadl::exchange(other.m_off)),
        m_mode(noadl::exchange(other.m_mode))
      { }

    basic_tinybuf_str&
    operator=(basic_tinybuf_str& other) &&
      noexcept(is_nothrow_move_assignable<string_type>::value)
      {
        this->m_str = move(other.m_str);
        this->m_off = noadl::exchange(other.m_off);
        this->m_mode = noadl::exchange(other.m_mode);
        return *this;
      }

    basic_tinybuf_str&
    swap(basic_tinybuf_str& other)
      noexcept(is_nothrow_swappable<string_type>::value)
      {
        this->m_str.swap(other.m_str);
        ::std::swap(this->m_off, other.m_off);
        ::std::swap(this->m_mode, other.m_mode);
        return *this;
      }

  public:
    virtual
    ~basic_tinybuf_str() override;

    // Gets the internal string.
    const string_type&
    get_string() const noexcept
      { return this->m_str;  }

    const char_type*
    c_str() const noexcept
      { return this->m_str.c_str();  }

    size_t
    length() const noexcept
      { return this->m_str.length();  }

    const char_type*
    data() const noexcept
      { return this->m_str.data();  }

    size_t
    size() const noexcept
      { return this->m_str.size();  }

    ptrdiff_t
    ssize() const noexcept
      { return this->m_str.ssize();  }

    size_t
    capacity() const noexcept
      { return this->m_str.capacity();  }

    basic_tinybuf_str&
    reserve(size_t res_arg)
      {
        this->m_str.reserve(res_arg);
        return *this;
      }

    // Replaces the internal string.
    template<typename xstrT>
    basic_tinybuf_str&
    set_string(xstrT&& xstr, open_mode mode)
      {
        this->m_str = forward<xstrT>(xstr);
        this->m_off = 0;
        this->m_mode = mode;
        return *this;
      }

    template<typename xstrT>
    basic_tinybuf_str&
    set_string(xstrT&& xstr)
      {
        this->m_str = forward<xstrT>(xstr);
        this->m_off = 0;
        return *this;
      }

    basic_tinybuf_str&
    clear_string()
      {
        this->m_str.clear();
        this->m_off = 0;
        return *this;
      }

    string_type&&
    extract_string()
      {
        this->m_off = 0;
        return move(this->m_str);
      }

    // Does nothing. Strings need not be flushed.
    virtual
    basic_tinybuf_str&
    flush() override
      {
        return *this;
      }

    // Gets the current stream pointer.
    // This cannot fail for strings.
    virtual
    int64_t
    tell() const override
      {
        return this->m_off;
      }

    // Adjusts the current stream pointer. It is not valid to seek before the
    // beginning. However it is valid to seek past the end; a consequent write
    // operation will extend the string, filling zeroes as necessary.
    // In case of an error, an exception is thrown, and the stream is left in an
    // unspecified state.
    virtual
    basic_tinybuf_str&
    seek(int64_t off, seek_dir dir) override
      {
        int64_t orig, targ;

        // Get the origin offset.
        switch(dir) {
          case tinybuf_base::seek_set:
            orig = 0;
            break;

          case tinybuf_base::seek_cur:
            orig = this->m_off;
            break;

          case tinybuf_base::seek_end:
            orig = static_cast<int64_t>(this->m_str.size());
            break;

          default:
            noadl::sprintf_and_throw<invalid_argument>(
                "tinybuf_str: seek direction `%d` not valid",
                static_cast<int>(dir));
        }

        // Calculate the target offset.
        if(ROCKET_ADD_OVERFLOW(orig, off, ::std::addressof(targ)))
          noadl::sprintf_and_throw<out_of_range>(
              "tinybuf_str: stream offset overflow (operands were `%lld` and `%lld`)",
              static_cast<long long>(orig), static_cast<long long>(off));

        if(targ < 0)
          noadl::sprintf_and_throw<out_of_range>(
              "tinybuf_str: seeking to negative offsets not allowed");

        // Set the new stream offset. It is valid to write past the end.
        this->m_off = targ;
        return *this;
      }

    // Reads some characters from the stream. If the end of stream has been
    // reached, zero is returned.
    // In case of an error, an exception is thrown, and the stream is left in an
    // unspecified state.
    virtual
    size_t
    getn(char_type* s, size_t n) override
      {
        if(!(this->m_mode & tinybuf_base::open_read))
          noadl::sprintf_and_throw<invalid_argument>(
                   "tinybuf_str: stream not readable");

        // If the stream offset is beyond the end, report end of stream.
        if(this->m_off >= static_cast<int64_t>(this->m_str.size()))
          return 0;

        const char_type* bptr = this->m_str.data() + static_cast<size_t>(this->m_off);
        size_t r = noadl::min(n, this->m_str.size() - static_cast<size_t>(this->m_off));
        this->m_off += static_cast<int64_t>(r);
        noadl::xmempcpy(s, bptr, r);
        return r;
      }

    // Reads a single character from the stream. If the end of stream has been
    // reached, `-1` is returned.
    // In case of an error, an exception is thrown, and the stream is left in an
    // unspecified state.
    virtual
    int
    getc() override
      {
        if(!(this->m_mode & tinybuf_base::open_read))
          noadl::sprintf_and_throw<invalid_argument>(
                   "tinybuf_str: stream not readable");

        // If the stream offset is beyond the end, report end of stream.
        if(this->m_off >= static_cast<int64_t>(this->m_str.size()))
          return -1;

        const char_type* bptr = this->m_str.data() + static_cast<size_t>(this->m_off);
        this->m_off ++;
        return noadl::int_from(*bptr);
      }

    // Puts some characters into the stream.
    // In case of an error, an exception is thrown, and the stream is left in an
    // unspecified state.
    virtual
    basic_tinybuf_str&
    putn(const char_type* s, size_t n) override
      {
        if(!(this->m_mode & tinybuf_base::open_write))
          noadl::sprintf_and_throw<invalid_argument>(
                   "tinybuf_str: stream not writable");

        if((this->m_mode & tinybuf_base::open_append)
             || (this->m_off == static_cast<int64_t>(this->m_str.size()))) {
          // This can be optimized a little.
          this->m_str.append(s, n);
          this->m_off = static_cast<int64_t>(this->m_str.size());
          return *this;
        }

        // Resize the string as necessary.
        if((this->m_off > static_cast<int64_t>(this->m_str.max_size()))
             || (n > this->m_str.max_size() - static_cast<size_t>(this->m_off)))
          noadl::sprintf_and_throw<overflow_error>(
                   "tinybuf_str: string too long (offset `%lld` + `%lld` > `%lld`)",
                   static_cast<long long>(this->m_off), static_cast<long long>(n),
                   static_cast<long long>(this->m_str.max_size()));

        this->m_str.resize(static_cast<size_t>(this->m_off) + n);
        char_type* bptr = this->m_str.mut_data() + static_cast<size_t>(this->m_off);
        this->m_off += static_cast<int64_t>(n);
        noadl::xmempcpy(bptr, s, n);
        return *this;
      }

    // Puts a single character into the stream.
    // In case of an error, an exception is thrown, and the stream is left in an
    // unspecified state.
    virtual
    basic_tinybuf_str&
    putc(char_type c) override
      {
        if(!(this->m_mode & tinybuf_base::open_write))
          noadl::sprintf_and_throw<invalid_argument>(
                   "tinybuf_str: stream not writable");

        if((this->m_mode & tinybuf_base::open_append)
             || (this->m_off == static_cast<int64_t>(this->m_str.size()))) {
          // This can be optimized a little.
          this->m_str.push_back(c);
          this->m_off = static_cast<int64_t>(this->m_str.size());
          return *this;
        }

        // Resize the string as necessary.
        if(this->m_off >= static_cast<int64_t>(this->m_str.max_size()))
          noadl::sprintf_and_throw<overflow_error>(
                   "tinybuf_str: string too long (offset `%lld` > `%lld`)",
                   static_cast<long long>(this->m_off),
                   static_cast<long long>(this->m_str.max_size()));

        this->m_str.resize(static_cast<size_t>(this->m_off) + 1);
        char_type* bptr = this->m_str.mut_data() + static_cast<size_t>(this->m_off);
        this->m_off ++;
        *bptr = c;
        return *this;
      }
  };

template<typename charT, typename allocT>
basic_tinybuf_str<charT, allocT>::
~basic_tinybuf_str()
  {
  }

template<typename charT, typename allocT>
inline
void
swap(basic_tinybuf_str<charT, allocT>& lhs, basic_tinybuf_str<charT, allocT>& rhs)
  noexcept(noexcept(lhs.swap(rhs)))
  {
    lhs.swap(rhs);
  }

using tinybuf_str     = basic_tinybuf_str<char>;
using wtinybuf_str    = basic_tinybuf_str<wchar_t>;
using u16tinybuf_str  = basic_tinybuf_str<char16_t>;
using u32tinybuf_str  = basic_tinybuf_str<char32_t>;

extern template class basic_tinybuf_str<char>;
extern template class basic_tinybuf_str<wchar_t>;
extern template class basic_tinybuf_str<char16_t>;
extern template class basic_tinybuf_str<char32_t>;

}  // namespace rocket
#endif
