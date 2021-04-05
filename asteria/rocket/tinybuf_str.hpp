// This file is part of Asteria.
// Copyleft 2018 - 2021, LH_Mouse. All wrongs reserved.

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
    string_type m_stor;
    size_type m_goff = 0;  // offset of the beginning of the get area
    bool m_appm = false;  // append mode

  public:
    basic_tinybuf_str() noexcept(is_nothrow_constructible<string_type>::value)
      : m_stor()
      { }

    explicit
    basic_tinybuf_str(const allocator_type& alloc) noexcept
      : m_stor(alloc)
      { }

    explicit
    basic_tinybuf_str(open_mode mode, const allocator_type& alloc = allocator_type()) noexcept
      : basic_tinybuf_str(alloc)
      { this->clear_string(mode);  }

    template<typename xstrT>
    explicit
    basic_tinybuf_str(xstrT&& xstr, open_mode mode, const allocator_type& alloc = allocator_type())
      : basic_tinybuf_str(alloc)
      { this->set_string(::std::forward<xstrT>(xstr), mode);  }

    ~basic_tinybuf_str() override;

    basic_tinybuf_str(basic_tinybuf_str&&)
      = default;

    basic_tinybuf_str&
    operator=(basic_tinybuf_str&&)
      = default;

  protected:
    off_type
    do_fortell() const override
      {
        // Calculate the number of characters after the get area.
        auto navail = this->m_stor.size() - this->m_goff;
        if(navail == 0) {
          // If no more characters are available, return -1.
          // Don't return 0 in this case, as it indicates the number of characters is unknown.
          return -1;
        }
        // Return the precise number of characters available.
        return static_cast<off_type>(navail);
      }

    basic_tinybuf_str&
    do_flush(const char_type*& gcur, const char_type*& gend, char_type*& /*pcur*/, char_type*& /*pend*/) override
      {
        if(gcur) {
          // If the get area exists, update the offset and clear it.
          this->m_goff = static_cast<size_type>(gcur - this->m_stor.data());
          gcur = nullptr;
          gend = nullptr;
        }
        // Notice that we don't use put areas.
        return *this;
      }

    off_type
    do_seek(off_type off, seek_dir dir) override
      {
        // Invalidate the get area before doing anything else.
        this->do_sync_areas();

        // Get the seek reference offset.
        size_type ref;
        if(dir == tinybuf_type::seek_set)
          ref = 0;
        else if(dir == tinybuf_type::seek_end)
          ref = this->m_stor.size();
        else
          ref = this->m_goff;

        // Perform range checks.
        if(off < static_cast<off_type>(-ref))
          noadl::sprintf_and_throw<out_of_range>("tinybuf_str: Attempt to seek to a negative offset");
        if(off > static_cast<off_type>(this->m_stor.size() - ref))
          noadl::sprintf_and_throw<out_of_range>("tinybuf_str: Attempt to seek past the end");

        // Convert the relative offset to an absolute one and set it.
        off_type abs = static_cast<off_type>(ref) + off;
        this->m_goff = static_cast<size_type>(abs);
        return abs;
      }

    int_type
    do_underflow(const char_type*& gcur, const char_type*& gend, bool peek) override
      {
        // If the get area exists, update the offset and clear it.
        this->do_sync_areas();

        // Calculate the number of characters available.
        auto navail = this->m_stor.size() - this->m_goff;
        if(navail == 0)
          // If no more characters are available, return EOF.
          return traits_type::eof();

        // Get the range of remaining characters.
        auto gbase = this->m_stor.data() + this->m_goff;
        // Set the new get area. Exclude the first character if `peek` is not set.
        gcur = gbase + !peek;
        gend = gbase + navail;
        return traits_type::to_int_type(gbase[0]);
      }

    basic_tinybuf_str&
    do_overflow(char_type*& /*pcur*/, char_type*& /*pend*/, const char_type* sadd, size_type nadd) override
      {
        // Be warned if the get area exists, it must be invalidated before modifying the string.
        this->do_sync_areas();

        // Notice that we don't use put areas.
        // If `open_append` is in effect, always append to the end.
        if(ROCKET_EXPECT((this->m_goff == this->m_stor.size()) || this->m_appm)) {
          // Append the string to the end.
          this->m_stor.append(sadd, nadd);
          this->m_goff = this->m_stor.size();
        }
        else {
          // Replace the substring from `m_goff`.
          this->m_stor.replace(this->m_goff, nadd, sadd, nadd);
          this->m_goff += nadd;
        }
        return *this;
      }

  public:
    const string_type&
    get_string() const noexcept
      { return this->m_stor;  }

    const char_type*
    c_str() const noexcept
      { return this->m_stor.c_str();  }

    size_type
    length() const noexcept
      { return this->m_stor.length();  }

    void
    clear_string(open_mode mode)
      {
        this->do_purge_areas();

        // Clear the string and set the new mode.
        this->m_stor.clear();
        this->m_goff = 0;
        this->m_appm = tinybuf_base::has_mode(mode, tinybuf_base::open_append);
      }

    template<typename xstrT>
    void
    set_string(xstrT&& xstr, open_mode mode)
      {
        this->do_purge_areas();

        // Set the new string and mode.
        this->m_stor = ::std::forward<xstrT>(xstr);
        this->m_goff = 0;
        this->m_appm = tinybuf_base::has_mode(mode, tinybuf_base::open_append);
      }

    string_type
    extract_string(open_mode mode)
      {
        this->do_purge_areas();

        // Swap the string with an empty one and set the new mode.
        string_type str;
        this->m_stor.swap(str);
        this->m_goff = 0;
        this->m_appm = tinybuf_base::has_mode(mode, tinybuf_base::open_append);
        return str;
      }

    basic_tinybuf_str&
    swap(basic_tinybuf_str& other) noexcept(is_nothrow_swappable<string_type>::value)
      {
        this->tinybuf_type::swap(other);

        noadl::xswap(this->m_stor, other.m_stor);
        noadl::xswap(this->m_goff, other.m_goff);
        noadl::xswap(this->m_appm, other.m_appm);
        return *this;
      }
  };

template<typename charT, typename traitsT, typename allocT>
basic_tinybuf_str<charT, traitsT, allocT>::
~basic_tinybuf_str()
  { }

template<typename charT, typename traitsT, typename allocT>
inline
void
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
