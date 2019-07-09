// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_COW_STRINGBUF_HPP_
#define ROCKET_COW_STRINGBUF_HPP_

#include "cow_string.hpp"
#include <streambuf>  // std::ios_base, std::basic_streambuf<>, std::streamsize

namespace rocket {

template<typename charT, typename traitsT = char_traits<charT>,
         typename allocatorT = allocator<charT>> class basic_cow_stringbuf;

    namespace details_cow_stringbuf {

    template<typename stringT> struct storage
      {
        using string_type  = stringT;
        using size_type    = typename string_type::size_type;

        string_type str;
        size_type goff;  // the offset past the end of the get area
        size_type poff;  // the offset of the beginning of the put area

        constexpr storage() noexcept(is_nothrow_constructible<string_type>::value)
          : str(), goff(0), poff(0)
          {
          }
        explicit storage(string_type xstr) noexcept(is_nothrow_move_constructible<string_type>::value)
          : str(noadl::move(xstr)), goff(0), poff(0)
          {
          }

        ::std::ios_base::openmode rewind(::std::ios_base::openmode which) noexcept
          {
            // Reset get and put positions.
            this->goff = 0;
            this->poff = 0;
            // Handle versatile stream modes.
            if(which & ios_base::trunc) {
              this->str.clear();
            }
            if(which & ios_base::ate) {
              this->poff = this->str.size();
            }
            return which;
          }

        void swap(storage& other) noexcept(is_nothrow_swappable<string_type>::value)
          {
            noadl::adl_swap(this->str, other.str);
            ::std::swap(this->goff, other.goff);
            ::std::swap(this->poff, other.poff);
          }
      };

    }

template<typename charT, typename traitsT,
         typename allocatorT> class basic_cow_stringbuf : public basic_streambuf<charT, traitsT>
  {
  public:
    using char_type       = charT;
    using traits_type     = traitsT;
    using allocator_type  = allocatorT;

    using int_type   = typename traits_type::int_type;
    using pos_type   = typename traits_type::pos_type;
    using off_type   = typename traits_type::off_type;

    // N.B. These are non-standard extensions.
    using streambuf_type   = basic_streambuf<charT, traitsT>;
    using string_type      = basic_cow_string<charT, traitsT, allocatorT>;

    using size_type        = typename string_type::size_type;
    using difference_type  = typename string_type::difference_type;

    static constexpr size_type npos = string_type::npos;

  private:
    ios_base::openmode m_which;
    details_cow_stringbuf::storage<string_type> m_stor;

  public:
    basic_cow_stringbuf()
      : m_which(ios_base::in | ios_base::out), m_stor()
      {
      }
    explicit basic_cow_stringbuf(ios_base::openmode which)
      : m_which(which), m_stor()
      {
        this->m_stor.rewind(which);
      }
    explicit basic_cow_stringbuf(string_type str, ios_base::openmode which = ios_base::in | ios_base::out)
      : m_which(which), m_stor(noadl::move(str))
      {
        this->m_stor.rewind(which);
      }
    basic_cow_stringbuf(basic_cow_stringbuf&& other)
      : m_which(other.m_which), m_stor(noadl::move(other.m_stor))
      {
        other.do_abandon();
        this->imbue(other.getloc());
      }
    basic_cow_stringbuf& operator=(basic_cow_stringbuf&& other)
      {
        this->m_which = other.m_which;
        this->m_stor = noadl::move(other.m_stor);
        other.do_abandon();
        this->imbue(other.getloc());
        return *this;
      }
    ~basic_cow_stringbuf() override;

  private:
    void do_abandon() noexcept
      {
        // Abandon the get and put areas.
        this->setg(nullptr, nullptr, nullptr);
        this->setp(nullptr, nullptr);
      }
    void do_syncg() noexcept
      {
        ROCKET_ASSERT(this->m_which & ios_base::in);
        // `goff` designates the past-the-end position of the get area.
        auto goff = this->m_stor.goff - static_cast<size_type>(this->egptr() - this->gptr());
        // This cannot invalidate the put area, because if a put area exists it will have made the string unique.
        auto qbase = const_cast<char_type*>(this->m_stor.str.data());
        auto ntotal = this->m_stor.str.size();
        // Reset the get area.
        this->setg(qbase, qbase + goff, qbase + ntotal);
        this->m_stor.goff = ntotal;
      }
    void do_freeg() noexcept
      {
        ROCKET_ASSERT(this->m_which & ios_base::in);
        // `goff` designates the past-the-end position of the get area.
        auto goff = this->m_stor.goff - static_cast<size_type>(this->egptr() - this->gptr());
        // Invalidate the get area.
        this->setg(nullptr, nullptr, nullptr);
        this->m_stor.goff = goff;
      }
    void do_syncp() noexcept
      {
        ROCKET_ASSERT(this->m_which & ios_base::out);
        // `poff` designates the beginning of the put area.
        auto poff = this->m_stor.poff + static_cast<size_type>(this->pptr() - this->pbase());
        // `goff` designates the past-the-end position of the get area.
        auto goff = this->m_stor.goff - static_cast<size_type>(this->egptr() - this->gptr());
        // This may invalidate the get area, since the string uses reference counting.
        auto qbase = this->m_stor.str.mut_data();
        auto ntotal = this->m_stor.str.size();
        // Reset the put area.
        this->setp(qbase + poff, qbase + ntotal);
        this->m_stor.poff = poff;
        // Invalidate the get area.
        this->setg(nullptr, nullptr, nullptr);
        this->m_stor.goff = goff;
      }
    void do_freep() noexcept
      {
        ROCKET_ASSERT(this->m_which & ios_base::out);
        // `poff` designates the beginning of the put area.
        auto poff = this->m_stor.poff + static_cast<size_type>(this->pptr() - this->pbase());
        // `goff` designates the past-the-end position of the get area.
        auto goff = this->m_stor.goff - static_cast<size_type>(this->egptr() - this->gptr());
        // Invalidate the put area.
        this->setp(nullptr, nullptr);
        this->m_stor.poff = poff;
        // Invalidate the get area.
        this->setg(nullptr, nullptr, nullptr);
        this->m_stor.goff = goff;
      }
    void do_syncpg() noexcept
      {
        ROCKET_ASSERT(this->m_which & ios_base::in);
        ROCKET_ASSERT(this->m_which & ios_base::out);
        // `poff` designates the beginning of the put area.
        auto poff = this->m_stor.poff + static_cast<size_type>(this->pptr() - this->pbase());
        // `goff` designates the past-the-end position of the get area.
        auto goff = this->m_stor.goff - static_cast<size_type>(this->egptr() - this->gptr());
        // This may invalidate the get area, since the string uses reference counting.
        auto qbase = this->m_stor.str.mut_data();
        auto ntotal = this->m_stor.str.size();
        // Reset the put area.
        this->setp(qbase + poff, qbase + ntotal);
        this->m_stor.poff = poff;
        // Reset the get area.
        this->setg(qbase, qbase + goff, qbase + ntotal);
        this->m_stor.goff = ntotal;
      }

  protected:
    pos_type seekoff(off_type off, ios_base::seekdir dir, ios_base::openmode which) override
      {
        pos_type res = off_type(-1);
        // Validate arguments.
        if(which & ~(ios_base::in | ios_base::out)) {
          // Seek mode is invalid.
          return res;
        }
        this->do_freep();
        // Get the seek reference offset.
        bool seekg = this->m_which & which & ios_base::in;
        bool seekp = this->m_which & which & ios_base::out;
        auto absoff = static_cast<streamsize>(off);
        auto refoff = static_cast<streamsize>(-1);
        auto ntotal = static_cast<streamsize>(this->m_stor.str.size());
        // XXX: This code looks terrible.
        if(dir == ios_base::beg) {
          // The offset is from the beginning of the string.
          refoff = 0;
        } else if(dir == ios_base::end) {
          // The offset is from the end of the string.
          refoff = ntotal;
        } else if(!seekp) {
          // The offset is from the get position.
          refoff = static_cast<streamsize>(this->m_stor.goff);
        } else if(!seekg) {
          // The offset is from the put position.
          refoff = static_cast<streamsize>(this->m_stor.poff);
        } else {
          // XXX: If you move both get and put positions, the target must be absolute.
          return res;
        }
        if((absoff < -refoff) || (absoff > ntotal - refoff)) {
          // The offset is out of range.
          return res;
        }
        if(this->m_which & which & ios_base::in) {
          // Abandon the get area, then set the new get offset.
          this->m_stor.goff = static_cast<size_type>(absoff);
          res = absoff;
        }
        if(this->m_which & which & ios_base::out) {
          // Abandon the put area, then set the new put offset.
          this->m_stor.poff = static_cast<size_type>(absoff);
          res = absoff;
        }
        return res;
      }
    pos_type seekpos(pos_type pos, ios_base::openmode which) override
      {
        // The state in `pos` is ignored.
        return this->seekoff(static_cast<streamsize>(pos), ios_base::beg, which);
      }

    streamsize showmanyc() override
      {
        if(!(this->m_which & ios_base::in)) {
          // Nothing can be read.
          return -1;
        }
        // Get the number of bytes available in and after the get area.
        return static_cast<streamsize>(this->egptr() - this->gptr())
               + static_cast<streamsize>(this->m_stor.str.size() - this->m_stor.goff);
      }
    int_type underflow() override
      {
        if(!(this->m_which & ios_base::in)) {
          // Nothing can be read.
          return traits_type::eof();
        }
        this->do_syncg();
        if(this->gptr() == this->egptr()) {
          // No more data are available.
          return traits_type::eof();
        }
        // Return the character at the get pointer.
        return traits_type::to_int_type(this->gptr()[0]);
      }
    int_type uflow() override
      {
        // What was the origin of this function?
        int_type c = this->basic_cow_stringbuf::underflow();
        if(traits_type::eq_int_type(c, traits_type::eof())) {
          // No more data are available.
          return traits_type::eof();
        }
        // Increment the get pointer.
        this->gbump(1);
        // Return the character preceding the get pointer.
        return c;
      }
    streamsize xsgetn(char_type* s, streamsize n) override
      {
        if(!(this->m_which & ios_base::in)) {
          // Nothing can be read.
          return 0;
        }
        if(n <= 0) {
          return 0;
        }
        this->do_freeg();
        // Copy some characters and advance the get offset.
        auto nlimit = static_cast<size_type>(noadl::min(n, static_cast<streamsize>(this->m_stor.str.max_size())));
        auto nread = this->m_stor.str.copy(s, this->m_stor.goff, nlimit);
        this->m_stor.goff += nread;
        // Return the number of characters that have been copied.
        return static_cast<streamsize>(nread);
      }
    int_type pbackfail(int_type c) override
      {
        if(!(this->m_which & ios_base::in)) {
          // Nothing can be read.
          return traits_type::eof();
        }
        if(traits_type::eq_int_type(c, traits_type::eof())) {
          // A put-back position is requested.
          this->do_syncg();
          if(this->gptr() == this->eback()) {
            // If the get pointer is at the beginning of the string, no character exists before it, so fail.
            return traits_type::eof();
          }
        } else {
          // A different character is to be put back.
          if(!(this->m_which & ios_base::out)) {
            // Nothing can be written.
            return traits_type::eof();
          }
          this->do_syncpg();
          if(this->gptr() == this->eback()) {
            // If the get pointer is at the beginning of the string, no character can be written, so fail.
            return traits_type::eof();
          }
          // Modify the get area to reflect the change.
          traits_type::assign(this->gptr()[-1], traits_type::to_char_type(c));
        }
        // Decrement the get pointer.
        this->gbump(-1);
        // Return the character at the get pointer.
        return traits_type::to_int_type(this->gptr()[0]);
      }
    int_type overflow(int_type c) override
      {
        if(!(this->m_which & ios_base::out)) {
          // Nothing can be written.
          return traits_type::eof();
        }
        this->do_freep();
        if(!traits_type::eq_int_type(c, traits_type::eof())) {
          // Add this character to the string.
          if(this->m_stor.poff < this->m_stor.str.size()) {
            this->m_stor.str.mut(this->m_stor.poff) = traits_type::to_char_type(c);
          } else {
            this->m_stor.str.push_back(traits_type::to_char_type(c));
          }
          this->m_stor.poff += 1;
        }
        if(this->m_stor.poff < this->m_stor.str.size()) {
          // Set up the put area for efficiency.
          // N.B. This would be unnecessary if we were using unbuffered I/O.
          this->do_syncp();
        }
        // Return the character that has just been written.
        return traits_type::not_eof(c);
      }
    streamsize xsputn(const char_type* s, streamsize n) override
      {
        if(!(this->m_which & ios_base::out)) {
          // Nothing can be written.
          return 0;
        }
        if(n <= 0) {
          return 0;
        }
        this->do_freep();
        // Copy some characters and advance the put offset.
        auto nlimit = static_cast<size_type>(noadl::min(n, static_cast<streamsize>(this->m_stor.str.max_size())));
        this->m_stor.str.replace(this->m_stor.poff, nlimit, s, nlimit);
        this->m_stor.poff += nlimit;
        // Return the number of characters that have been copied.
        return static_cast<streamsize>(nlimit);
      }

  public:
    const string_type& get_string() const noexcept
      {
        return this->m_stor.str;
      }
    void set_string(string_type str)
      {
        this->do_abandon();
        this->m_stor.str = noadl::move(str);
        this->m_stor.rewind(this->m_which);
      }
    string_type extract_string()
      {
        this->do_abandon();
        auto str = noadl::move(this->m_stor.str);
        this->m_stor.rewind(this->m_which);
        return str;
      }

    void swap(basic_cow_stringbuf& other)
      {
        ::std::swap(this->m_which, other.m_which);
        this->m_stor.swap(other.m_stor);
        this->imbue(other.pubimbue(this->getloc()));
      }
  };

#if !(defined(__cpp_inline_variables) && (__cpp_inline_variables >= 201606))
template<typename charT, typename traitsT,
         typename allocatorT> constexpr typename basic_cow_stringbuf<charT, traitsT,
                                                                     allocatorT>::size_type basic_cow_stringbuf<charT, traitsT,
                                                                                                                allocatorT>::npos;
#endif

template<typename charT, typename traitsT,
         typename allocatorT> basic_cow_stringbuf<charT, traitsT, allocatorT>::~basic_cow_stringbuf()
  = default;

template<typename charT, typename traitsT,
         typename allocatorT> void swap(basic_cow_stringbuf<charT, traitsT, allocatorT>& lhs,
                                        basic_cow_stringbuf<charT, traitsT, allocatorT>& rhs)
  {
    lhs.swap(rhs);
  }

extern template class basic_cow_stringbuf<char>;
extern template class basic_cow_stringbuf<wchar_t>;

using cow_stringbuf = basic_cow_stringbuf<char>;
using cow_wstringbuf = basic_cow_stringbuf<wchar_t>;

}  // namespace rocket

#endif
