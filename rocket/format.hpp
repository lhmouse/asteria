// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_FORMAT_
#define ROCKET_FORMAT_

#include "tinyfmt.hpp"
#include "throw.hpp"

namespace rocket {

template<typename charT, typename traitsT = char_traits<charT>>
struct basic_formatter;

template<typename charT, typename traitsT, typename valueT>
constexpr
basic_formatter<charT, traitsT>
make_default_formatter(const basic_tinyfmt<charT, traitsT>& fmt, valueT& value) noexcept;

template<typename charT, typename traitsT, typename allocT>
class basic_cow_string;

template<typename charT, typename traitsT>
basic_tinyfmt<charT, traitsT>&
vformat(basic_tinyfmt<charT, traitsT>& fmt, const charT* stempl, size_t ntempl,
        const basic_formatter<charT, traitsT>* pinsts, size_t ninsts);

template<typename charT, typename traitsT>
basic_tinyfmt<charT, traitsT>&
vformat(basic_tinyfmt<charT, traitsT>& fmt, const charT* stempl,
        const basic_formatter<charT, traitsT>* pinsts, size_t ninsts);

template<typename charT, typename traitsT, typename allocT>
basic_tinyfmt<charT, traitsT>&
vformat(basic_tinyfmt<charT, traitsT>& fmt, const basic_cow_string<charT, traitsT, allocT>& stempl,
        const basic_formatter<charT, traitsT>* pinsts, size_t ninsts);

template<typename charT, typename traitsT, typename... paramsT>
basic_tinyfmt<charT, traitsT>&
format(basic_tinyfmt<charT, traitsT>& fmt, const charT* stempl,
       const paramsT&... params);

template<typename charT, typename traitsT, typename allocT, typename... paramsT>
basic_tinyfmt<charT, traitsT>&
format(basic_tinyfmt<charT, traitsT>& fmt, const basic_cow_string<charT, traitsT, allocT>& stempl,
       const paramsT&... params);

/* Examples:
 *   rocket::format(fmt, "hello $1 $2", "world", '!');   // "hello world!"
 *   rocket::format(fmt, "${1} + $1 = ${2}", 1, 2);      // "1 + 1 = 2"
 *   rocket::format(fmt, "literal $$");                  // "literal $"
 *   rocket::format(fmt, "funny $0 string");             // "funny funny $0 string string"
 *   rocket::format(fmt, "$123", 'x');                   // "x23"
 *   rocket::format(fmt, "${12}3", 'x');                 // throws an exception
**/

template<typename charT, typename traitsT>
struct basic_formatter
  {
    using tinyfmt_type   = basic_tinyfmt<charT, traitsT>;
    using callback_type  = void (tinyfmt_type&, const void*);

    callback_type* ifunc;
    const void* param;

    void
    operator()(tinyfmt_type& fmt) const
      { this->ifunc(fmt, this->param);  }
  };

template<typename charT, typename traitsT, typename valueT>
constexpr
basic_formatter<charT, traitsT>
make_default_formatter(basic_tinyfmt<charT, traitsT>& /*fmt*/, valueT& value) noexcept
  {
    basic_formatter<charT, traitsT> r;
    r.ifunc = [](auto& fmt, auto* ptr) { fmt << *static_cast<valueT*>(ptr);  };
    r.param = ::std::addressof(value);
    return r;
  }

template<typename charT, typename traitsT>
basic_tinyfmt<charT, traitsT>&
vformat(basic_tinyfmt<charT, traitsT>& fmt, const charT* stempl, size_t ntempl,
        const basic_formatter<charT, traitsT>* pinsts, size_t ninsts)
  {
    // Get the range of `format`. The end pointer points to the null terminator.
    const charT* bp = stempl;
    const charT* ep = bp + ntempl;

    for(;;) {
      // Look for a placeholder sequence.
      const charT* pp = traitsT::find(bp, (size_t) (ep - bp), traitsT::to_char_type('$'));
      if(!pp) {
        // No placeholder has been found.
        // Write all remaining characters verbatim and exit.
        fmt.putn(bp, (size_t) (ep - bp));
        break;
      }

      // A placeholder has been found. Write all characters preceding it.
      fmt.putn(bp, (size_t) (pp - bp));

      // Skip the dollar sign and parse the placeholder.
      if(pp == ep)
        noadl::sprintf_and_throw<invalid_argument>(
              "format: incomplete placeholder (dangling `$`)");

      typename traitsT::int_type ch = traitsT::to_int_type(*++pp);
      size_t index = 0;
      bp = ++pp;

      if(ch == '$') {
        // Write a plain dollar sign.
        fmt.putc(traitsT::to_char_type('$'));
        continue;
      }

      switch(ch) {
        case '{': {
          // Look for the terminator.
          bp = traitsT::find(bp, (size_t) (ep - bp), traitsT::to_char_type('}'));
          if(!bp)
            noadl::sprintf_and_throw<invalid_argument>(
                  "format: incomplete placeholder (missing `}`)");

          // Parse the argument index.
          ptrdiff_t ndigs = bp++ - pp;
          if(ndigs < 1)
            noadl::sprintf_and_throw<invalid_argument>(
                  "format: missing argument index");

          if(ndigs > 3)
            noadl::sprintf_and_throw<invalid_argument>(
                  "format: too many digits (`%td` > `3`)", ndigs);

          // Collect digits.
          while(--ndigs >= 0) {
            ch = traitsT::to_int_type(*(pp++));
            char32_t dval = (unsigned char) (ch - '0');
            if(dval > 9)
              noadl::sprintf_and_throw<invalid_argument>(
                    "format: invalid digit `%c`", (int) ch);

            // Accumulate a digit.
            index = index * 10 + dval;
          }
          break;
        }

        case '0' ... '9':
          // Accept a single decimal digit.
          index = (unsigned char) (ch - '0');
          break;

        default:
          noadl::sprintf_and_throw<invalid_argument>(
                "format: invalid placeholder `$%c`", (int) ch);
      }

      // Replace the placeholder.
      if(index == 0)
        fmt.putn(stempl, ntempl);
      else if(index <= ninsts)
        pinsts[index-1](fmt);
      else
        noadl::sprintf_and_throw<invalid_argument>(
              "format: no enough arguments (expecting at least `%zu`, got `%zu`)",
              index, ninsts);
    }
    return fmt;
  }

template<typename charT, typename traitsT>
basic_tinyfmt<charT, traitsT>&
vformat(basic_tinyfmt<charT, traitsT>& fmt, const charT* stempl,
        const basic_formatter<charT, traitsT>* pinsts, size_t ninsts)
  {
    return noadl::vformat(fmt, stempl, traitsT::length(stempl), pinsts, ninsts);
  }

template<typename charT, typename traitsT, typename allocT>
basic_tinyfmt<charT, traitsT>&
vformat(basic_tinyfmt<charT, traitsT>& fmt, const basic_cow_string<charT, traitsT, allocT>& stempl,
        const basic_formatter<charT, traitsT>* pinsts, size_t ninsts)
  {
    return noadl::vformat(fmt, stempl.c_str(), stempl.length(), pinsts, ninsts);
  }

template<typename charT, typename traitsT, typename... paramsT>
basic_tinyfmt<charT, traitsT>&
format(basic_tinyfmt<charT, traitsT>& fmt, const charT* stempl,
       const paramsT&... params)
  {
    using vformatter = basic_formatter<charT, traitsT>;
    vformatter insts[] = { noadl::make_default_formatter(fmt, params)..., { } };
    return noadl::vformat(fmt, stempl, insts, sizeof...(params));
  }

template<typename charT, typename traitsT, typename allocT, typename... paramsT>
basic_tinyfmt<charT, traitsT>&
format(basic_tinyfmt<charT, traitsT>& fmt, const basic_cow_string<charT, traitsT, allocT>& stempl,
       const paramsT&... params)
  {
    using vformatter = basic_formatter<charT, traitsT>;
    vformatter insts[] = { noadl::make_default_formatter(fmt, params)..., { } };
    return noadl::vformat(fmt, stempl, insts, sizeof...(params));
  }

using formatter   = basic_formatter<char>;
using wformatter  = basic_formatter<wchar_t>;

extern
template
tinyfmt&
vformat(tinyfmt&, const char*, size_t, const formatter*, size_t);

extern
template
wtinyfmt&
vformat(wtinyfmt&, const wchar_t*, size_t, const wformatter*, size_t);

}  // namespace rocket

#endif
