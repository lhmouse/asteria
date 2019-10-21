// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_TINYBUF_STR_HPP_
#define ROCKET_TINYBUF_STR_HPP_

#include "tinybuf.hpp"
#include "cow_string.hpp"

namespace rocket {

template<typename charT, typename traitsT = char_traits<charT>,
         typename allocT = allocator<charT>> class basic_tinybuf_str;

template<typename charT, typename traitsT, typename allocT>
    class basic_tinybuf_str : public basic_tinybuf<charT, traitsT>
  {
  public:
    using char_type       = charT;
    using traits_type     = traitsT;
    using allocator_type  = allocT;

    using buffer_type  = basic_tinybuf<charT, traitsT>;
    using string_type  = basic_cow_string<charT, traitsT, allocT>;

    using seek_dir   = typename buffer_type::seek_dir;
    using open_mode  = typename buffer_type::open_mode;
    using int_type   = typename buffer_type::int_type;
    using off_type   = typename buffer_type::off_type;
    using size_type  = typename buffer_type::size_type;

  private:
    string_type m_str;
    size_type m_off;
    open_mode m_mode;

  public:
    basic_tinybuf_str() noexcept
      :
        m_str(), m_off(),
        m_mode()
      {
      }
    explicit basic_tinybuf_str(open_mode mode) noexcept
      :
        m_str(), m_off(),
        m_mode(mode)
      {
      }
    template<typename xstrT> explicit basic_tinybuf_str(xstrT&& xstr, open_mode mode)
      :
        m_str(noadl::forward<xstrT>(xstr)), m_off(),
        m_mode(mode)
      {
      }
    ~basic_tinybuf_str() override;

    basic_tinybuf_str(basic_tinybuf_str&&)
      = default;
    basic_tinybuf_str& operator=(basic_tinybuf_str&&)
      = default;

  protected:
    off_type do_fortell() const override
      {
        if(!(this->m_mode & tinybuf_base::open_read)) {
          // Read access is not enabled.
          return -1;
        }
        // Calculate the number of characters after the get area.
        auto navail = this->m_str.size() - this->m_off;
        if(navail == 0) {
          // If no more characters are available, return -1.
          // Don't return 0 in this case, as it indicates the number of characters is unknown.
          return -1;
        }
        // Return the precise number of characters available.
        return static_cast<off_type>(navail);
      }
    basic_tinybuf_str& do_flush(const char_type*& gcur, const char_type*& gend,
                                char_type*& /*pcur*/, char_type*& /*pend*/) override
      {
        if(gcur) {
          // If the get area exists, update the offset and clear it.
          this->m_off = static_cast<size_type>(gcur - this->m_str.data());
          gcur = nullptr;
          gend = nullptr;
        }
        // Notice that we don't use the put area.
        return *this;
      }
    off_type do_seek(off_type off, seek_dir dir) override
      {
        // Invalidate the get area before doing anything else.
        this->flush();
        // Get the seek reference offset.
        size_type ref;
        if(dir == buffer_type::seek_set)
          ref = 0;
        else if(dir == buffer_type::seek_end)
          ref = this->m_str.size();
        else
          ref = this->m_off;
        // Perform range checks.
        if(off < static_cast<off_type>(-ref)) {
          noadl::sprintf_and_throw<out_of_range>(
            "basic_tinybuf_str: An attempt was made to seek backwards past the beginning of "
            "this string (the offset was `%lld` and the reference offset was `%lld`).",
            static_cast<long long>(off), static_cast<long long>(ref));
        }
        if(off > static_cast<off_type>(this->m_str.size() - ref)) {
          noadl::sprintf_and_throw<out_of_range>(
            "basic_tinybuf_str: An attempt was made to seek past the end of "
            "this string (the offset was `%lld` and the reference offset was `%lld`).",
            static_cast<long long>(off), static_cast<long long>(ref));
        }
        // Convert the relative offset to an absolute one and set it.
        off_type abs = static_cast<off_type>(ref) + off;
        this->m_off = static_cast<size_type>(abs);
        // Return the absolute offset.
        return abs;
      }

    int_type do_underflow(const char_type*& gcur, const char_type*& gend, bool peek) override
      {
        if(!(this->m_mode & tinybuf_base::open_read)) {
          // Read access is not enabled.
          noadl::sprintf_and_throw<invalid_argument>(
            "basic_tinybuf_str: This stream was not opened for reading.");
        }
        // If the get area exists, update the offset and clear it.
        this->flush();
        // Calculate the number of characters available.
        auto navail = this->m_str.size() - this->m_off;
        if(navail == 0) {
          // If no more characters are available, return EOF.
          return traits_type::eof();
        }
        // Get the range of remaining characters.
        auto gbase = this->m_str.data() + this->m_off;
        // Set the new get area. Exclude the first character if `peek` is not set.
        gcur = gbase + !peek;
        gend = gbase + navail;
        return traits_type::to_int_type(gbase[0]);
      }
    basic_tinybuf_str& do_overflow(char_type*& /*pcur*/, char_type*& /*pend*/,
                                   const char_type* sadd, size_type nadd) override
      {
        if(!(this->m_mode & tinybuf_base::open_write)) {
          // Write access is not enabled.
          noadl::sprintf_and_throw<invalid_argument>(
            "basic_tinybuf_str: This stream was not opened for writing.");
        }
        // Be warned if the get area exists, it must be invalidated before modifying the string.
        this->flush();
        // Notice that we don't use the put area.
        if(this->m_mode & tinybuf_base::open_append) {
          // If `open_append` is in effect, always append to the end.
          this->m_str.append(sadd, nadd);
          this->m_off = this->m_str.size();
        }
        else {
          // Replace the substring from `m_off`.
          this->m_str.replace(this->m_off, nadd, sadd, nadd);
          this->m_off += nadd;
        }
        return *this;
      }

  public:
    const string_type& get_string() const noexcept
      {
        return this->m_str;
      }
    const char_type* get_c_string() const noexcept
      {
        return this->m_str.c_str();
      }
    void clear_string(open_mode mode)
      {
        // Invalidate the get area.
        this->flush();
        // Clear the string and set the new mode.
        this->m_str.clear();
        this->m_off = 0;
        this->m_mode = mode;
      }
    template<typename xstrT> void set_string(xstrT&& xstr, open_mode mode)
      {
        // Invalidate the get area.
        this->flush();
        // Set the new string and mode.
        this->m_str = noadl::forward<xstrT>(xstr);
        this->m_off = 0;
        this->m_mode = mode;
      }
    string_type extract_string(open_mode mode)
      {
        // Invalidate the get area.
        this->flush();
        // Swap the string with an empty one and set the new mode.
        string_type str;
        this->m_str.swap(str);
        this->m_off = 0;
        this->m_mode = mode;
        return str;
      }

    basic_tinybuf_str& swap(basic_tinybuf_str& other) noexcept(conjunction<is_nothrow_swappable<buffer_type>,
                                                                           is_nothrow_swappable<string_type>>::value)
      {
        xswap(this->m_str, other.m_str);
        xswap(this->m_off, other.m_off);
        xswap(this->m_mode, other.m_mode);
        return *this;
      }
  };

template<typename charT, typename traitsT, typename allocT>
    basic_tinybuf_str<charT, traitsT, allocT>::~basic_tinybuf_str()
  = default;

template<typename charT, typename traitsT, typename allocT>
    inline void swap(basic_tinybuf_str<charT, traitsT, allocT>& lhs,
                     basic_tinybuf_str<charT, traitsT, allocT>& rhs) noexcept(noexcept(lhs.swap(rhs)))
  {
    lhs.swap(rhs);
  }

extern template class basic_tinybuf_str<char>;
extern template class basic_tinybuf_str<wchar_t>;

using tinybuf_str   = basic_tinybuf_str<char>;
using wtinybuf_str  = basic_tinybuf_str<wchar_t>;

}  // namespace rocket

#endif
