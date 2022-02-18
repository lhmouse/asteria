// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_TINYFMT_HPP_
#define ROCKET_TINYFMT_HPP_

#include "fwd.hpp"
#include "throw.hpp"
#include "tinybuf.hpp"
#include "ascii_numput.hpp"

namespace rocket {

template<typename charT,
         typename traitsT = char_traits<charT>>
class basic_tinyfmt;

/* Differences from `std::basic_ostream`:
 * 1. Locales are not supported.
 * 2. Formatting is not supported.
 * 3. The stream is stateless. Exceptions are preferred when reporting errors.
**/

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

    basic_tinyfmt&
    operator=(const basic_tinyfmt&) noexcept
      = default;

  public:
    virtual
    ~basic_tinyfmt();

  public:
    // buffer interfaces
    virtual tinybuf_type&
    get_tinybuf() const
      = 0;

    // unformatted output functions
    basic_tinyfmt&
    flush()
      { this->get_tinybuf().flush();  return *this;  }

    off_type
    tell() const
      { return this->get_tinybuf().tell();  }

    basic_tinyfmt&
    seek(off_type off, seek_dir dir = tinybuf_base::seek_set)
      { this->get_tinybuf().seek(off, dir);  return *this;  }

    basic_tinyfmt&
    putc(char_type c)
      { this->get_tinybuf().putc(c);  return *this;  }

    basic_tinyfmt&
    putn(const char_type* s, size_type n)
      { this->get_tinybuf().putn(s, n);  return *this;  }

    basic_tinyfmt&
    puts(const char_type* s)
      { this->get_tinybuf().puts(s);  return *this;  }
  };

template<typename charT, typename traitsT>
basic_tinyfmt<charT, traitsT>::
~basic_tinyfmt()
  { }

extern template
class basic_tinyfmt<char>;

extern template
class basic_tinyfmt<wchar_t>;

using tinyfmt   = basic_tinyfmt<char>;
using wtinyfmt  = basic_tinyfmt<wchar_t>;

// zero-conversion inserters
template<typename charT, typename traitsT>
basic_tinyfmt<charT, traitsT>&
operator<<(basic_tinyfmt<charT, traitsT>& fmt, charT c)
  {
    // Insert the character as is.
    auto& buf = fmt.get_tinybuf();
    buf.putc(c);
    return fmt;
  }

template<typename charT, typename traitsT>
basic_tinyfmt<charT, traitsT>&
operator<<(basic_tinyfmt<charT, traitsT>& fmt, const charT* s)
  {
    // Insert the sequence as is.
    auto& buf = fmt.get_tinybuf();
    buf.puts(s);
    return fmt;
  }

// conversion inserters
template<typename charT, typename traitsT>
basic_tinyfmt<charT, traitsT>&
operator<<(basic_tinyfmt<charT, traitsT>& fmt, const ascii_numput& nump)
  {
    // Widen all characters and insert them.
    auto& buf = fmt.get_tinybuf();
    noadl::for_each(nump, [&](char c) { buf.putc(traitsT::to_char_type(c & 0xFF));  });
    return fmt;
  }

inline basic_tinyfmt<char>&
operator<<(basic_tinyfmt<char>& fmt, const ascii_numput& nump)
  {
    // Optimize it a bit if no conversion is required.
    auto& buf = fmt.get_tinybuf();
    buf.putn(nump.data(), nump.size());
    return fmt;
  }

// delegating inserters
template<typename charT, typename traitsT, typename valueT,
ROCKET_ENABLE_IF(is_arithmetic<valueT>::value),
ROCKET_DISABLE_IF(is_same<const volatile valueT, const volatile charT>::value)>
basic_tinyfmt<charT, traitsT>&
operator<<(basic_tinyfmt<charT, traitsT>& fmt, valueT value)
  { return fmt << ascii_numput(value);  }

template<typename charT, typename traitsT, typename valueT,
ROCKET_ENABLE_IF(is_enum<valueT>::value)>
basic_tinyfmt<charT, traitsT>&
operator<<(basic_tinyfmt<charT, traitsT>& fmt, valueT value)
  { return fmt << ascii_numput(static_cast<typename underlying_type<valueT>::type>(value));  }

template<typename charT, typename traitsT, typename valueT,
ROCKET_DISABLE_IF(is_convertible<valueT*, const charT*>::value)>
basic_tinyfmt<charT, traitsT>&
operator<<(basic_tinyfmt<charT, traitsT>& fmt, valueT* value)
  { return fmt << ascii_numput(reinterpret_cast<const void*>(value));  }

template<typename charT, typename traitsT>
basic_tinyfmt<charT, traitsT>&
operator<<(basic_tinyfmt<charT, traitsT>& fmt, const type_info& value)
  { return fmt << value.name();  }

template<typename charT, typename traitsT, typename valueT,
ROCKET_ENABLE_IF(is_base_of<exception, typename remove_cv<valueT>::type>::value)>
basic_tinyfmt<charT, traitsT>&
operator<<(basic_tinyfmt<charT, traitsT>& fmt, const valueT& value)
  { return fmt << value.what() << "\n[exception class `" << typeid(value) << "`]";  }

// rvalue inserter
template<typename charT, typename traitsT, typename xvalueT>
basic_tinyfmt<charT, traitsT>&
operator<<(basic_tinyfmt<charT, traitsT>&& fmt, xvalueT&& xvalue)
  { return fmt << ::std::forward<xvalueT>(xvalue);  }

extern template
tinyfmt&
operator<<(tinyfmt&, char);

extern template
tinyfmt&
operator<<(tinyfmt&, const char*);

extern template
tinyfmt&
operator<<(tinyfmt&, const ascii_numput&);

extern template
tinyfmt&
operator<<(tinyfmt&, signed char);

extern template
tinyfmt&
operator<<(tinyfmt&, signed short);

extern template
tinyfmt&
operator<<(tinyfmt&, signed);

extern template
tinyfmt&
operator<<(tinyfmt&, signed long);

extern template
tinyfmt&
operator<<(tinyfmt&, signed long long);

extern template
tinyfmt&
operator<<(tinyfmt&, unsigned char);

extern template
tinyfmt&
operator<<(tinyfmt&, unsigned short);

extern template
tinyfmt&
operator<<(tinyfmt&, unsigned);

extern template
tinyfmt&
operator<<(tinyfmt&, unsigned long);

extern template
tinyfmt&
operator<<(tinyfmt&, unsigned long long);

extern template
tinyfmt&
operator<<(tinyfmt&, float);

extern template
tinyfmt&
operator<<(tinyfmt&, double);

extern template
tinyfmt&
operator<<(tinyfmt&, const void*);

extern template
tinyfmt&
operator<<(tinyfmt&, void*);

extern template
tinyfmt&
operator<<(tinyfmt&, const type_info&);

extern template
tinyfmt&
operator<<(tinyfmt&, const exception&);

extern template
wtinyfmt&
operator<<(wtinyfmt&, wchar_t);

extern template
wtinyfmt&
operator<<(wtinyfmt&, const wchar_t*);

extern template
wtinyfmt&
operator<<(wtinyfmt&, const ascii_numput&);

extern template
wtinyfmt&
operator<<(wtinyfmt&, signed char);

extern template
wtinyfmt&
operator<<(wtinyfmt&, signed short);

extern template
wtinyfmt&
operator<<(wtinyfmt&, signed);

extern template
wtinyfmt&
operator<<(wtinyfmt&, signed long);

extern template
wtinyfmt&
operator<<(wtinyfmt&, signed long long);

extern template
wtinyfmt&
operator<<(wtinyfmt&, unsigned char);

extern template
wtinyfmt&
operator<<(wtinyfmt&, unsigned short);

extern template
wtinyfmt&
operator<<(wtinyfmt&, unsigned);

extern template
wtinyfmt&
operator<<(wtinyfmt&, unsigned long);

extern template
wtinyfmt&
operator<<(wtinyfmt&, unsigned long long);

extern template
wtinyfmt&
operator<<(wtinyfmt&, float);

extern template
wtinyfmt&
operator<<(wtinyfmt&, double);

extern template
wtinyfmt&
operator<<(wtinyfmt&, const void*);

extern template
wtinyfmt&
operator<<(wtinyfmt&, void*);

extern template
wtinyfmt&
operator<<(wtinyfmt&, const type_info&);

extern template
wtinyfmt&
operator<<(wtinyfmt&, const exception&);

}  // namespace rocket

#endif
