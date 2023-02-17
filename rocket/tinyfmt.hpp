// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_TINYFMT_
#define ROCKET_TINYFMT_

#include "fwd.hpp"
#include "throw.hpp"
#include "tinybuf.hpp"
#include "ascii_numput.hpp"
#include <chrono>
namespace rocket {

template<typename charT, typename traitsT = char_traits<charT>>
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
    basic_tinyfmt() noexcept = default;

    basic_tinyfmt(const basic_tinyfmt&) noexcept = default;

    basic_tinyfmt&
    operator=(const basic_tinyfmt&) & noexcept = default;

  public:
    virtual
    ~basic_tinyfmt();

  public:
    // buffer interfaces
    virtual
    tinybuf_type&
    get_tinybuf() const = 0;

    // unformatted output functions
    basic_tinyfmt&
    flush()
      {
        this->get_tinybuf().flush();
        return *this;
      }

    off_type
    tell() const
      {
        return this->get_tinybuf().tell();
      }

    basic_tinyfmt&
    seek(off_type off, seek_dir dir = tinybuf_base::seek_set)
      {
        this->get_tinybuf().seek(off, dir);
        return *this;
      }

    basic_tinyfmt&
    putc(char_type c)
      {
        this->get_tinybuf().putc(c);
        return *this;
      }

    basic_tinyfmt&
    putn(const char_type* s, size_type n)
      {
        this->get_tinybuf().putn(s, n);
        return *this;
      }

    basic_tinyfmt&
    puts(const char_type* s)
      {
        this->get_tinybuf().puts(s);
        return *this;
      }
  };

template<typename charT, typename traitsT>
basic_tinyfmt<charT, traitsT>::
~basic_tinyfmt()
  {
  }

using tinyfmt     = basic_tinyfmt<char>;
using wtinyfmt    = basic_tinyfmt<wchar_t>;
using u16tinyfmt  = basic_tinyfmt<char16_t>;
using u32tinyfmt  = basic_tinyfmt<char32_t>;

extern template class basic_tinyfmt<char>;

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

inline
basic_tinyfmt<char>&
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
  {
    ascii_numput nump;
    nump.put(value);
    return fmt << nump;
  }

template<typename charT, typename traitsT, typename valueT,
ROCKET_ENABLE_IF(is_enum<valueT>::value)>
basic_tinyfmt<charT, traitsT>&
operator<<(basic_tinyfmt<charT, traitsT>& fmt, valueT value)
  {
    ascii_numput nump;
    nump.put(static_cast<typename underlying_type<valueT>::type>(value));
    return fmt << nump;
  }

template<typename charT, typename traitsT, typename valueT,
ROCKET_DISABLE_IF(is_convertible<valueT*, const charT*>::value)>
basic_tinyfmt<charT, traitsT>&
operator<<(basic_tinyfmt<charT, traitsT>& fmt, valueT* value)
  {
    ascii_numput nump;
    nump.put(reinterpret_cast<const void*>(value));
    return fmt << nump;
  }

template<typename charT, typename traitsT>
basic_tinyfmt<charT, traitsT>&
operator<<(basic_tinyfmt<charT, traitsT>& fmt, const type_info& value)
  {
    return fmt << value.name();
  }

template<typename charT, typename traitsT, typename valueT,
ROCKET_ENABLE_IF(is_base_of<exception, typename remove_cv<valueT>::type>::value)>
basic_tinyfmt<charT, traitsT>&
operator<<(basic_tinyfmt<charT, traitsT>& fmt, const valueT& value)
  {
    return fmt << value.what() << "\n[exception class `" << typeid(value) << "`]";
  }

// std support
template<typename charT, typename traitsT, typename ytraitsT, typename yallocT>
basic_tinyfmt<charT, traitsT>&
operator<<(basic_tinyfmt<charT, traitsT>& fmt, const ::std::basic_string<charT, ytraitsT, yallocT>& str)
  {
    return fmt.putn(str.data(), str.size());
  }

template<typename charT, typename traitsT, typename elementT, typename deleteT>
basic_tinyfmt<charT, traitsT>&
operator<<(basic_tinyfmt<charT, traitsT>& fmt, const ::std::unique_ptr<elementT, deleteT>& ptr)
  {
    return fmt << ptr.get();
  }

template<typename charT, typename traitsT, typename elementT>
basic_tinyfmt<charT, traitsT>&
operator<<(basic_tinyfmt<charT, traitsT>& fmt, const ::std::shared_ptr<elementT>& ptr)
  {
    return fmt << ptr.get();
  }

// rvalue inserter
template<typename charT, typename traitsT, typename xvalueT>
basic_tinyfmt<charT, traitsT>&
operator<<(basic_tinyfmt<charT, traitsT>&& fmt, xvalueT&& xvalue)
  {
    return fmt << ::std::forward<xvalueT>(xvalue);
  }

// ... stupid English, stupid inflection.
#define ROCKET_TINYFMT_NOUN_IRREGULAR(num, sing, pl)   ((((num) > 0) && ((num) <= 1)) ? (sing) : (pl))
#define ROCKET_TINYFMT_NOUN_REGULAR(num, sing)         ROCKET_TINYFMT_NOUN_IRREGULAR(num, sing, sing "s")
#define ROCKET_TINYFMT_NOUN_SAME_PLURAL(num, sing)     ROCKET_TINYFMT_NOUN_IRREGULAR(num, sing, sing)

template<typename charT, typename traitsT, typename repT>
basic_tinyfmt<charT, traitsT>&
operator<<(basic_tinyfmt<charT, traitsT>& fmt, const ::std::chrono::duration<repT, ::std::ratio<1, 1000000000>>& dur)
  {
    return fmt << dur.count() << ROCKET_TINYFMT_NOUN_REGULAR(dur.count(), " nanosecond");
  }

template<typename charT, typename traitsT, typename repT>
basic_tinyfmt<charT, traitsT>&
operator<<(basic_tinyfmt<charT, traitsT>& fmt, const ::std::chrono::duration<repT, ::std::ratio<1, 1000000>>& dur)
  {
    return fmt << dur.count() << ROCKET_TINYFMT_NOUN_REGULAR(dur.count(), " microsecond");
  }

template<typename charT, typename traitsT, typename repT>
basic_tinyfmt<charT, traitsT>&
operator<<(basic_tinyfmt<charT, traitsT>& fmt, const ::std::chrono::duration<repT, ::std::ratio<1, 1000>>& dur)
  {
    return fmt << dur.count() << ROCKET_TINYFMT_NOUN_REGULAR(dur.count(), " millisecond");
  }

template<typename charT, typename traitsT, typename repT>
basic_tinyfmt<charT, traitsT>&
operator<<(basic_tinyfmt<charT, traitsT>& fmt, const ::std::chrono::duration<repT, ::std::ratio<1>>& dur)
  {
    return fmt << dur.count() << ROCKET_TINYFMT_NOUN_REGULAR(dur.count(), " second");
  }

template<typename charT, typename traitsT, typename repT>
basic_tinyfmt<charT, traitsT>&
operator<<(basic_tinyfmt<charT, traitsT>& fmt, const ::std::chrono::duration<repT, ::std::ratio<60>>& dur)
  {
    return fmt << dur.count() << ROCKET_TINYFMT_NOUN_REGULAR(dur.count(), " minute");
  }

template<typename charT, typename traitsT, typename repT>
basic_tinyfmt<charT, traitsT>&
operator<<(basic_tinyfmt<charT, traitsT>& fmt, const ::std::chrono::duration<repT, ::std::ratio<3600>>& dur)
  {
    return fmt << dur.count() << ROCKET_TINYFMT_NOUN_REGULAR(dur.count(), " hour");
  }

template<typename charT, typename traitsT, typename repT>
basic_tinyfmt<charT, traitsT>&
operator<<(basic_tinyfmt<charT, traitsT>& fmt, const ::std::chrono::duration<repT, ::std::ratio<86400>>& dur)
  {
    return fmt << dur.count() << ROCKET_TINYFMT_NOUN_REGULAR(dur.count(), " day");
  }

template<typename charT, typename traitsT, typename repT>
basic_tinyfmt<charT, traitsT>&
operator<<(basic_tinyfmt<charT, traitsT>& fmt, const ::std::chrono::duration<repT, ::std::ratio<604800>>& dur)
  {
    return fmt << dur.count() << ROCKET_TINYFMT_NOUN_REGULAR(dur.count(), " week");
  }

extern template tinyfmt& operator<<(tinyfmt&, char);
extern template tinyfmt& operator<<(tinyfmt&, const char*);
extern template tinyfmt& operator<<(tinyfmt&, const ascii_numput&);
extern template tinyfmt& operator<<(tinyfmt&, signed char);
extern template tinyfmt& operator<<(tinyfmt&, signed short);
extern template tinyfmt& operator<<(tinyfmt&, signed);
extern template tinyfmt& operator<<(tinyfmt&, signed long);
extern template tinyfmt& operator<<(tinyfmt&, signed long long);
extern template tinyfmt& operator<<(tinyfmt&, unsigned char);
extern template tinyfmt& operator<<(tinyfmt&, unsigned short);
extern template tinyfmt& operator<<(tinyfmt&, unsigned);
extern template tinyfmt& operator<<(tinyfmt&, unsigned long);
extern template tinyfmt& operator<<(tinyfmt&, unsigned long long);
extern template tinyfmt& operator<<(tinyfmt&, float);
extern template tinyfmt& operator<<(tinyfmt&, double);
extern template tinyfmt& operator<<(tinyfmt&, const void*);
extern template tinyfmt& operator<<(tinyfmt&, void*);
extern template tinyfmt& operator<<(tinyfmt&, const type_info&);
extern template tinyfmt& operator<<(tinyfmt&, const exception&);
extern template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<1, 1000000000>>&);
extern template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<1, 1000000>>&);
extern template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<1, 1000>>&);
extern template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<1>>&);
extern template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<60>>&);
extern template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<3600>>&);
extern template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<86400>>&);
extern template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<604800>>&);
extern template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<1, 1000000000>>&);
extern template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<1, 1000000>>&);
extern template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<1, 1000>>&);
extern template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<1>>&);
extern template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<60>>&);
extern template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<3600>>&);
extern template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<86400>>&);
extern template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<604800>>&);

}  // namespace rocket
#endif
