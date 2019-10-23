// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_TINYFMT_HPP_
#define ROCKET_TINYFMT_HPP_

#include "utilities.hpp"
#include "throw.hpp"
#include "tinybuf.hpp"
#include "tinynumput.hpp"

namespace rocket {

template<typename charT, typename traitsT = char_traits<charT>> class basic_tinyfmt;

/* Differences from `std::basic_ostream`:
 * 1. Locales are not supported.
 * 2. Formatting is not supported.
 * 3. The stream is stateless. Exceptions are preferred when reporting errors.
 */

template<typename charT, typename traitsT>
    class basic_tinyfmt
  {
  public:
    using char_type    = charT;
    using traits_type  = traitsT;

    using tinyfmt_type  = basic_tinyfmt;
    using tinybuf_type  = basic_tinybuf<charT, traitsT>;

    using seek_dir   = typename tinybuf_type::seek_dir;
    using open_mode  = typename tinybuf_type::open_mode;
    using int_type   = typename tinybuf_type::int_type;
    using off_type   = typename tinybuf_type::off_type;
    using size_type  = typename tinybuf_type::size_type;

  protected:
    // This interface class is stateless.
    basic_tinyfmt() noexcept
      = default;
    basic_tinyfmt(const basic_tinyfmt&) noexcept
      = default;
    basic_tinyfmt& operator=(const basic_tinyfmt&) noexcept
      = default;

  public:
    virtual ~basic_tinyfmt();

  public:
    // buffer interfaces
    virtual tinybuf_type& get_buffer() const = 0;

    // unformatted output functions
    basic_tinyfmt& flush()
      {
        return this->get_buffer().flush(), *this;
      }
    off_type seek(off_type off, seek_dir dir = tinybuf_base::seek_set)
      {
        return this->get_buffer().seek(off, dir);
      }
    basic_tinyfmt& put(char_type c)
      {
        return this->get_buffer().put(c), *this;
      }
    basic_tinyfmt& put(const char_type* s, size_type n)
      {
        return this->get_buffer().put(s, n), *this;
      }
  };

template<typename charT, typename traitsT>
    basic_tinyfmt<charT, traitsT>::~basic_tinyfmt()
  = default;

// zero-conversion inserters
template<typename charT, typename traitsT>
    basic_tinyfmt<charT, traitsT>& operator<<(basic_tinyfmt<charT, traitsT>& fmt, const charT& c)
  {
    // Insert the character as is.
    auto& buf = fmt.get_buffer();
    buf.put(c);
    return fmt;
  }
template<typename charT, typename traitsT>
    basic_tinyfmt<charT, traitsT>& operator<<(basic_tinyfmt<charT, traitsT>& fmt, const charT* s)
  {
    // Insert the sequence as is.
    auto& buf = fmt.get_buffer();
    buf.put(s, traitsT::length(s));
    return fmt;
  }

// conversion inserters
template<typename charT, typename traitsT>
    basic_tinyfmt<charT, traitsT>& operator<<(basic_tinyfmt<charT, traitsT>& fmt, const tinynumput& nump)
  {
    // Widen all characters and insert them.
    auto& buf = fmt.get_buffer();
    noadl::for_each(nump, [&](char c) { buf.put(traitsT::to_char_type(static_cast<unsigned char>(c)));  });
    return fmt;
  }
inline basic_tinyfmt<char>& operator<<(basic_tinyfmt<char>& fmt, const tinynumput& nump)
  {
    // Optimize it a bit if no conversion is required.
    auto& buf = fmt.get_buffer();
    buf.put(nump.data(), nump.size());
    return fmt;
  }

// delegating inserters
template<typename charT, typename traitsT, typename valueT,
         ROCKET_ENABLE_IF(is_arithmetic<valueT>::value)>
    basic_tinyfmt<charT, traitsT>& operator<<(basic_tinyfmt<charT, traitsT>& fmt, const valueT& value)
  {
    return fmt << tinynumput(value);
  }
template<typename charT, typename traitsT, typename valueT,
         ROCKET_ENABLE_IF(is_enum<valueT>::value)>
    basic_tinyfmt<charT, traitsT>& operator<<(basic_tinyfmt<charT, traitsT>& fmt, const valueT& value)
  {
    return fmt << tinynumput(static_cast<typename underlying_type<valueT>::type>(value));
  }
template<typename charT, typename traitsT, typename valueT>
    basic_tinyfmt<charT, traitsT>& operator<<(basic_tinyfmt<charT, traitsT>& fmt, valueT* value)
  {
    return fmt << tinynumput(static_cast<const void*>(value));
  }

// rvalue inserter
template<typename charT, typename traitsT, typename xvalueT>
    basic_tinyfmt<charT, traitsT>& operator<<(basic_tinyfmt<charT, traitsT>&& fmt, xvalueT&& xvalue)
  {
    return fmt << noadl::forward<xvalueT>(xvalue);
  }

extern template class basic_tinyfmt<char>;
extern template class basic_tinyfmt<wchar_t>;

using tinyfmt   = basic_tinyfmt<char>;
using wtinyfmt  = basic_tinyfmt<wchar_t>;

}  // namespace rocket

#endif
