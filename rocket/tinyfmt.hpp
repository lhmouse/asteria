// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_TINYFMT_
#define ROCKET_TINYFMT_

#include "fwd.hpp"
#include "xthrow.hpp"
#include "tinybuf.hpp"
#include "ascii_numput.hpp"
#include <chrono>
namespace rocket {

// Differences from `std::basic_ostream`:
// 1. Locales are not supported.
// 2. Formatting is not supported.
// 3. The stream is stateless. Exceptions are preferred when reporting errors.
template<typename charT>
class basic_tinyfmt
  {
  public:
    using char_type     = charT;
    using seek_dir      = tinybuf_base::seek_dir;
    using open_mode     = tinybuf_base::open_mode;
    using tinybuf_type  = basic_tinybuf<charT>;

  protected:
    basic_tinyfmt() noexcept = default;

    basic_tinyfmt(const basic_tinyfmt&) noexcept = default;
    basic_tinyfmt& operator=(const basic_tinyfmt&) & noexcept = default;

  protected:
    // Gets the associated buffer.
    // This function is provided to unify `const` and non-`const`
    // implementations. Successive calls to this function on the same
    // `basic_tinyfmt` object shall return references to the same buffer.
    ROCKET_PURE virtual
    tinybuf_type&
    do_get_tinybuf_nonconst() const = 0;

  public:
    virtual
    ~basic_tinyfmt();

    // These are wrappers for the buffer.
    const tinybuf_type&
    buf() const
      {
        return this->do_get_tinybuf_nonconst();
      }

    tinybuf_type&
    mut_buf()
      {
        return this->do_get_tinybuf_nonconst();
      }

    // These are wrappers for buffer operations.
    basic_tinyfmt&
    flush()
      {
        this->mut_buf().flush();
        return *this;
      }

    int64_t
    tell() const
      {
        int64_t off = this->buf().tell();
        return off;
      }

    basic_tinyfmt&
    seek(int64_t off, seek_dir dir)
      {
        this->mut_buf().seek(off, dir);
        return *this;
      }

    size_t
    getn(char_type* s, size_t n)
      {
        size_t ngot = this->mut_buf().getn(s, n);
        return ngot;
      }

    int
    getc()
      {
        int ch = this->mut_buf().getc();
        return ch;
      }

    basic_tinyfmt&
    putn(const char_type* s, size_t n)
      {
        this->mut_buf().putn(s, n);
        return *this;
      }

    basic_tinyfmt&
    putc(char_type c)
      {
        this->mut_buf().putc(c);
        return *this;
      }

    basic_tinyfmt&
    putn_latin1(const char* s, size_t n)
      {
        constexpr size_t M = 32;
        char_type x[M];
        size_t ns = 0, nx = 0;

        while(ns != n) {
          // Zero-extend one character to `char_type`.
          ROCKET_ASSERT(nx != M);
          x[nx++] = static_cast<char_type>(s[ns++] & 0xFF);

          if(ROCKET_EXPECT(ns != n) && ROCKET_EXPECT(nx != M))
            continue;

          // Flush all pending characters.
          this->mut_buf().putn(x, nx);
          nx = 0;
        }
        return *this;
      }
  };

template<typename charT>
basic_tinyfmt<charT>::
~basic_tinyfmt()
  {
  }

// Inserts a null-terminated multi-byte string. The string shall begin and
// end in the initial shift state, otherwise the result is unspecified.
basic_tinyfmt<wchar_t>&
operator<<(basic_tinyfmt<wchar_t>& fmt, const char* s);

basic_tinyfmt<char16_t>&
operator<<(basic_tinyfmt<char16_t>& fmt, const char* s);

basic_tinyfmt<char32_t>&
operator<<(basic_tinyfmt<char32_t>& fmt, const char* s);

// specialized stream inserters
template<typename charT>
inline
basic_tinyfmt<charT>&
operator<<(basic_tinyfmt<charT>& fmt, charT c)
  {
    return fmt.putc(c);
  }

template<typename charT>
inline
basic_tinyfmt<charT>&
operator<<(basic_tinyfmt<charT>& fmt, const charT* s)
  {
    if(!s) {
      static constexpr charT null[] = { '(','n','u','l','l',')' };
      return fmt.putn(null, 6);
    }
    return fmt.putn(s, noadl::xstrlen(s));
  }

template<typename charT, typename allocT>
basic_tinyfmt<charT>&
operator<<(basic_tinyfmt<charT>& fmt, const ::std::basic_string<charT, allocT>& str)
  {
    return fmt.putn(str.data(), str.size());
  }

#ifdef __cpp_lib_string_view
template<typename charT>
basic_tinyfmt<charT>&
operator<<(basic_tinyfmt<charT>& fmt, const ::std::basic_string_view<charT>& str)
  {
    return fmt.putn(str.data(), str.size());
  }
#endif  // __cpp_lib_string_view

template<typename charT>
inline
basic_tinyfmt<charT>&
operator<<(basic_tinyfmt<charT>& fmt, const ascii_numput& nump)
  {
    return fmt << nump.c_str();
  }

template<typename charT, typename valueT>
inline
basic_tinyfmt<charT>&&
operator<<(basic_tinyfmt<charT>&& fmt, const valueT& value)
  {
    return ::std::move(fmt << value);
  }

// delegating inserters
template<typename charT, typename valueT,
ROCKET_ENABLE_IF(is_arithmetic<valueT>::value && !is_same<valueT, charT>::value)>
inline
basic_tinyfmt<charT>&
operator<<(basic_tinyfmt<charT>& fmt, valueT value)
  {
    ascii_numput nump;
    nump.put(value);
    return fmt << nump;
  }

template<typename charT, typename valueT,
ROCKET_ENABLE_IF(is_enum<valueT>::value)>
basic_tinyfmt<charT>&
operator<<(basic_tinyfmt<charT>& fmt, valueT value)
  {
    ascii_numput nump;
    nump.put(static_cast<typename underlying_type<valueT>::type>(value));
    return fmt << nump;
  }

template<typename charT, typename valueT,
ROCKET_DISABLE_IF(is_convertible<valueT*, const charT*>::value || is_convertible<valueT*, const char*>::value)>
basic_tinyfmt<charT>&
operator<<(basic_tinyfmt<charT>& fmt, valueT* value)
  {
    ascii_numput nump;
    nump.put(reinterpret_cast<const void*>(value));
    return fmt << nump;
  }

template<typename charT, typename elementT, typename deleteT>
basic_tinyfmt<charT>&
operator<<(basic_tinyfmt<charT>& fmt, const ::std::unique_ptr<elementT, deleteT>& ptr)
  {
    return fmt << ptr.get();
  }

template<typename charT, typename elementT>
basic_tinyfmt<charT>&
operator<<(basic_tinyfmt<charT>& fmt, const ::std::shared_ptr<elementT>& ptr)
  {
    return fmt << ptr.get();
  }

template<typename charT>
basic_tinyfmt<charT>&
operator<<(basic_tinyfmt<charT>& fmt, const type_info& value)
  {
    return fmt << value.name();
  }

template<typename charT, typename valueT,
ROCKET_ENABLE_IF(is_base_of<exception, typename remove_reference<valueT>::type>::value)>
basic_tinyfmt<charT>&
operator<<(basic_tinyfmt<charT>& fmt, const valueT& value)
  {
    return fmt << value.what() << "\n[exception class `" << typeid(value) << "`]";
  }

// ... stupid English, stupid inflection.
#define ROCKET_TINYFMT_NOUN_IRREGULAR(num, sing, pl)   ((((num) > 0) && ((num) <= 1)) ? (sing) : (pl))
#define ROCKET_TINYFMT_NOUN_REGULAR(num, sing)         ROCKET_TINYFMT_NOUN_IRREGULAR(num, sing, sing "s")
#define ROCKET_TINYFMT_NOUN_SAME_PLURAL(num, sing)     ROCKET_TINYFMT_NOUN_IRREGULAR(num, sing, sing)

template<typename charT, typename repT>
basic_tinyfmt<charT>&
operator<<(basic_tinyfmt<charT>& fmt, const ::std::chrono::duration<repT, ::std::ratio<1, 1000000000>>& dur)
  {
    return fmt << dur.count() << ROCKET_TINYFMT_NOUN_REGULAR(dur.count(), " nanosecond");
  }

template<typename charT, typename repT>
basic_tinyfmt<charT>&
operator<<(basic_tinyfmt<charT>& fmt, const ::std::chrono::duration<repT, ::std::ratio<1, 1000000>>& dur)
  {
    return fmt << dur.count() << ROCKET_TINYFMT_NOUN_REGULAR(dur.count(), " microsecond");
  }

template<typename charT, typename repT>
basic_tinyfmt<charT>&
operator<<(basic_tinyfmt<charT>& fmt, const ::std::chrono::duration<repT, ::std::ratio<1, 1000>>& dur)
  {
    return fmt << dur.count() << ROCKET_TINYFMT_NOUN_REGULAR(dur.count(), " millisecond");
  }

template<typename charT, typename repT>
basic_tinyfmt<charT>&
operator<<(basic_tinyfmt<charT>& fmt, const ::std::chrono::duration<repT, ::std::ratio<1>>& dur)
  {
    return fmt << dur.count() << ROCKET_TINYFMT_NOUN_REGULAR(dur.count(), " second");
  }

template<typename charT, typename repT>
basic_tinyfmt<charT>&
operator<<(basic_tinyfmt<charT>& fmt, const ::std::chrono::duration<repT, ::std::ratio<60>>& dur)
  {
    return fmt << dur.count() << ROCKET_TINYFMT_NOUN_REGULAR(dur.count(), " minute");
  }

template<typename charT, typename repT>
basic_tinyfmt<charT>&
operator<<(basic_tinyfmt<charT>& fmt, const ::std::chrono::duration<repT, ::std::ratio<3600>>& dur)
  {
    return fmt << dur.count() << ROCKET_TINYFMT_NOUN_REGULAR(dur.count(), " hour");
  }

template<typename charT, typename repT>
basic_tinyfmt<charT>&
operator<<(basic_tinyfmt<charT>& fmt, const ::std::chrono::duration<repT, ::std::ratio<86400>>& dur)
  {
    return fmt << dur.count() << ROCKET_TINYFMT_NOUN_REGULAR(dur.count(), " day");
  }

template<typename charT, typename repT>
basic_tinyfmt<charT>&
operator<<(basic_tinyfmt<charT>& fmt, const ::std::chrono::duration<repT, ::std::ratio<604800>>& dur)
  {
    return fmt << dur.count() << ROCKET_TINYFMT_NOUN_REGULAR(dur.count(), " week");
  }

using tinyfmt     = basic_tinyfmt<char>;
using wtinyfmt    = basic_tinyfmt<wchar_t>;
using u16tinyfmt  = basic_tinyfmt<char16_t>;
using u32tinyfmt  = basic_tinyfmt<char32_t>;

extern template class basic_tinyfmt<char>;
extern template class basic_tinyfmt<wchar_t>;
extern template class basic_tinyfmt<char16_t>;
extern template class basic_tinyfmt<char32_t>;

extern template tinyfmt& operator<<(tinyfmt&, char);
extern template wtinyfmt& operator<<(wtinyfmt&, wchar_t);
extern template u16tinyfmt& operator<<(u16tinyfmt&, char16_t);
extern template u32tinyfmt& operator<<(u32tinyfmt&, char32_t);

extern template tinyfmt& operator<<(tinyfmt&, const char*);
extern template wtinyfmt& operator<<(wtinyfmt&, const wchar_t*);
extern template u16tinyfmt& operator<<(u16tinyfmt&, const char16_t*);
extern template u32tinyfmt& operator<<(u32tinyfmt&, const char32_t*);

extern template tinyfmt& operator<<(tinyfmt&, const ascii_numput&);
extern template wtinyfmt& operator<<(wtinyfmt&, const ascii_numput&);
extern template u16tinyfmt& operator<<(u16tinyfmt&, const ascii_numput&);
extern template u32tinyfmt& operator<<(u32tinyfmt&, const ascii_numput&);

extern template tinyfmt& operator<<(tinyfmt&, signed char);
extern template wtinyfmt& operator<<(wtinyfmt&, signed char);
extern template u16tinyfmt& operator<<(u16tinyfmt&, signed char);
extern template u32tinyfmt& operator<<(u32tinyfmt&, signed char);

extern template tinyfmt& operator<<(tinyfmt&, signed short);
extern template wtinyfmt& operator<<(wtinyfmt&, signed short);
extern template u16tinyfmt& operator<<(u16tinyfmt&, signed short);
extern template u32tinyfmt& operator<<(u32tinyfmt&, signed short);

extern template tinyfmt& operator<<(tinyfmt&, signed);
extern template wtinyfmt& operator<<(wtinyfmt&, signed);
extern template u16tinyfmt& operator<<(u16tinyfmt&, signed);
extern template u32tinyfmt& operator<<(u32tinyfmt&, signed);

extern template tinyfmt& operator<<(tinyfmt&, signed long);
extern template wtinyfmt& operator<<(wtinyfmt&, signed long);
extern template u16tinyfmt& operator<<(u16tinyfmt&, signed long);
extern template u32tinyfmt& operator<<(u32tinyfmt&, signed long);

extern template tinyfmt& operator<<(tinyfmt&, signed long long);
extern template wtinyfmt& operator<<(wtinyfmt&, signed long long);
extern template u16tinyfmt& operator<<(u16tinyfmt&, signed long long);
extern template u32tinyfmt& operator<<(u32tinyfmt&, signed long long);

extern template tinyfmt& operator<<(tinyfmt&, unsigned char);
extern template wtinyfmt& operator<<(wtinyfmt&, unsigned char);
extern template u16tinyfmt& operator<<(u16tinyfmt&, unsigned char);
extern template u32tinyfmt& operator<<(u32tinyfmt&, unsigned char);

extern template tinyfmt& operator<<(tinyfmt&, unsigned short);
extern template wtinyfmt& operator<<(wtinyfmt&, unsigned short);
extern template u16tinyfmt& operator<<(u16tinyfmt&, unsigned short);
extern template u32tinyfmt& operator<<(u32tinyfmt&, unsigned short);

extern template tinyfmt& operator<<(tinyfmt&, unsigned);
extern template wtinyfmt& operator<<(wtinyfmt&, unsigned);
extern template u16tinyfmt& operator<<(u16tinyfmt&, unsigned);
extern template u32tinyfmt& operator<<(u32tinyfmt&, unsigned);

extern template tinyfmt& operator<<(tinyfmt&, unsigned long);
extern template wtinyfmt& operator<<(wtinyfmt&, unsigned long);
extern template u16tinyfmt& operator<<(u16tinyfmt&, unsigned long);
extern template u32tinyfmt& operator<<(u32tinyfmt&, unsigned long);

extern template tinyfmt& operator<<(tinyfmt&, unsigned long long);
extern template wtinyfmt& operator<<(wtinyfmt&, unsigned long long);
extern template u16tinyfmt& operator<<(u16tinyfmt&, unsigned long long);
extern template u32tinyfmt& operator<<(u32tinyfmt&, unsigned long long);

extern template tinyfmt& operator<<(tinyfmt&, float);
extern template wtinyfmt& operator<<(wtinyfmt&, float);
extern template u16tinyfmt& operator<<(u16tinyfmt&, float);
extern template u32tinyfmt& operator<<(u32tinyfmt&, float);

extern template tinyfmt& operator<<(tinyfmt&, double);
extern template wtinyfmt& operator<<(wtinyfmt&, double);
extern template u16tinyfmt& operator<<(u16tinyfmt&, double);
extern template u32tinyfmt& operator<<(u32tinyfmt&, double);

extern template tinyfmt& operator<<(tinyfmt&, const void*);
extern template wtinyfmt& operator<<(wtinyfmt&, const void*);
extern template u16tinyfmt& operator<<(u16tinyfmt&, const void*);
extern template u32tinyfmt& operator<<(u32tinyfmt&, const void*);

extern template tinyfmt& operator<<(tinyfmt&, void*);
extern template wtinyfmt& operator<<(wtinyfmt&, void*);
extern template u16tinyfmt& operator<<(u16tinyfmt&, void*);
extern template u32tinyfmt& operator<<(u32tinyfmt&, void*);

extern template tinyfmt& operator<<(tinyfmt&, const type_info&);
extern template wtinyfmt& operator<<(wtinyfmt&, const type_info&);
extern template u16tinyfmt& operator<<(u16tinyfmt&, const type_info&);
extern template u32tinyfmt& operator<<(u32tinyfmt&, const type_info&);

extern template tinyfmt& operator<<(tinyfmt&, const exception&);
extern template wtinyfmt& operator<<(wtinyfmt&, const exception&);
extern template u16tinyfmt& operator<<(u16tinyfmt&, const exception&);
extern template u32tinyfmt& operator<<(u32tinyfmt&, const exception&);

extern template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<1, 1000000000>>&);
extern template wtinyfmt& operator<<(wtinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<1, 1000000000>>&);
extern template u16tinyfmt& operator<<(u16tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<1, 1000000000>>&);
extern template u32tinyfmt& operator<<(u32tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<1, 1000000000>>&);

extern template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<1, 1000000>>&);
extern template wtinyfmt& operator<<(wtinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<1, 1000000>>&);
extern template u16tinyfmt& operator<<(u16tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<1, 1000000>>&);
extern template u32tinyfmt& operator<<(u32tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<1, 1000000>>&);

extern template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<1, 1000>>&);
extern template wtinyfmt& operator<<(wtinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<1, 1000>>&);
extern template u16tinyfmt& operator<<(u16tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<1, 1000>>&);
extern template u32tinyfmt& operator<<(u32tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<1, 1000>>&);

extern template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<1>>&);
extern template wtinyfmt& operator<<(wtinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<1>>&);
extern template u16tinyfmt& operator<<(u16tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<1>>&);
extern template u32tinyfmt& operator<<(u32tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<1>>&);

extern template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<60>>&);
extern template wtinyfmt& operator<<(wtinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<60>>&);
extern template u16tinyfmt& operator<<(u16tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<60>>&);
extern template u32tinyfmt& operator<<(u32tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<60>>&);

extern template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<3600>>&);
extern template wtinyfmt& operator<<(wtinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<3600>>&);
extern template u16tinyfmt& operator<<(u16tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<3600>>&);
extern template u32tinyfmt& operator<<(u32tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<3600>>&);

extern template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<86400>>&);
extern template wtinyfmt& operator<<(wtinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<86400>>&);
extern template u16tinyfmt& operator<<(u16tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<86400>>&);
extern template u32tinyfmt& operator<<(u32tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<86400>>&);

extern template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<604800>>&);
extern template wtinyfmt& operator<<(wtinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<604800>>&);
extern template u16tinyfmt& operator<<(u16tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<604800>>&);
extern template u32tinyfmt& operator<<(u32tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<604800>>&);

extern template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<1, 1000000000>>&);
extern template wtinyfmt& operator<<(wtinyfmt&, const ::std::chrono::duration<double, ::std::ratio<1, 1000000000>>&);
extern template u16tinyfmt& operator<<(u16tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<1, 1000000000>>&);
extern template u32tinyfmt& operator<<(u32tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<1, 1000000000>>&);

extern template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<1, 1000000>>&);
extern template wtinyfmt& operator<<(wtinyfmt&, const ::std::chrono::duration<double, ::std::ratio<1, 1000000>>&);
extern template u16tinyfmt& operator<<(u16tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<1, 1000000>>&);
extern template u32tinyfmt& operator<<(u32tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<1, 1000000>>&);

extern template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<1, 1000>>&);
extern template wtinyfmt& operator<<(wtinyfmt&, const ::std::chrono::duration<double, ::std::ratio<1, 1000>>&);
extern template u16tinyfmt& operator<<(u16tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<1, 1000>>&);
extern template u32tinyfmt& operator<<(u32tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<1, 1000>>&);

extern template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<1>>&);
extern template wtinyfmt& operator<<(wtinyfmt&, const ::std::chrono::duration<double, ::std::ratio<1>>&);
extern template u16tinyfmt& operator<<(u16tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<1>>&);
extern template u32tinyfmt& operator<<(u32tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<1>>&);

extern template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<60>>&);
extern template wtinyfmt& operator<<(wtinyfmt&, const ::std::chrono::duration<double, ::std::ratio<60>>&);
extern template u16tinyfmt& operator<<(u16tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<60>>&);
extern template u32tinyfmt& operator<<(u32tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<60>>&);

extern template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<3600>>&);
extern template wtinyfmt& operator<<(wtinyfmt&, const ::std::chrono::duration<double, ::std::ratio<3600>>&);
extern template u16tinyfmt& operator<<(u16tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<3600>>&);
extern template u32tinyfmt& operator<<(u32tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<3600>>&);

extern template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<86400>>&);
extern template wtinyfmt& operator<<(wtinyfmt&, const ::std::chrono::duration<double, ::std::ratio<86400>>&);
extern template u16tinyfmt& operator<<(u16tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<86400>>&);
extern template u32tinyfmt& operator<<(u32tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<86400>>&);

extern template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<604800>>&);
extern template wtinyfmt& operator<<(wtinyfmt&, const ::std::chrono::duration<double, ::std::ratio<604800>>&);
extern template u16tinyfmt& operator<<(u16tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<604800>>&);
extern template u32tinyfmt& operator<<(u32tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<604800>>&);

}  // namespace rocket
#endif
