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
        size_type gpos;
        size_type ppos;

        constexpr storage()
          : str(), gpos(0), ppos(0)
          {
          }
        explicit storage(string_type xstr)
          : str(noadl::move(xstr)), gpos(0), ppos(0)
          {
          }

        void init(::std::ios_base::openmode which)
          {
            if(which & ios_base::ate) {
              // Seek to end upon open.
              this->ppos = this->str.size();
            }
            if(which & ios_base::trunc) {
              // Clear the string. Why do you want to use it?
              this->str.clear();
              this->ppos = 0;
            }
          }

        void swap(storage& other)
          {
            noadl::adl_swap(this->str, other.str);
            ::std::swap(this->gpos, other.gpos);
            ::std::swap(this->ppos, other.ppos);
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
        this->m_stor.init(which);
      }
    explicit basic_cow_stringbuf(string_type str, ios_base::openmode which = ios_base::in | ios_base::out)
      : m_which(which), m_stor(noadl::move(str))
      {
        this->m_stor.init(which);
      }
    basic_cow_stringbuf(basic_cow_stringbuf&& other)
      : m_which(other.m_which), m_stor(noadl::move(other.m_stor))
      {
        other.do_invalidate();
        this->imbue(other.getloc());
      }
    basic_cow_stringbuf& operator=(basic_cow_stringbuf&& other)
      {
        this->m_which = other.m_which;
        this->m_stor = noadl::move(other.m_stor);
        other.do_invalidate();
        this->imbue(other.getloc());
        return *this;
      }
    ~basic_cow_stringbuf() override;

  private:
    void do_invalidate()
      {
        // Invalidate the get and put areas.
        this->setg(nullptr, nullptr, nullptr);
        this->setp(nullptr, nullptr);
      }

  protected:
    

  public:
    const string_type& get_string() const noexcept
      {
        return this->m_stor.str;
      }
    void set_string(string_type str)
      {
        this->m_stor.str = noadl::move(str);
        this->m_stor.init(this->m_which);
        this->do_invalidate();
      }
    string_type extract_string()
      {
        auto str = rocket::move(this->m_stor.str);
        this->m_stor.init(this->m_which);
        this->do_invalidate();
        return str;
      }

    void swap(basic_cow_stringbuf& other)
      {
        ::std::swap(this->m_which, other.m_which);
        this->m_stor.swap(other.m_stor);
        this->do_invalidate();
        other.do_invalidate();
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
