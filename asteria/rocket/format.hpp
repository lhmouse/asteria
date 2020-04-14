// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_FORMAT_HPP_
#define ROCKET_FORMAT_HPP_

#include "tinyfmt.hpp"
#include "throw.hpp"

namespace rocket {

template<typename charT, typename traitsT = char_traits<charT>>
struct basic_formatter;

template<typename charT, typename traitsT>
basic_tinyfmt<charT, traitsT>&
vformat(basic_tinyfmt<charT, traitsT>& fmt, const charT* stempl, size_t ntempl,
        const basic_formatter<charT, traitsT>* pinsts, size_t ninsts);

template<typename charT, typename traitsT>
basic_tinyfmt<charT, traitsT>&
vformat(basic_tinyfmt<charT, traitsT>& fmt, const charT* stempl,
        const basic_formatter<charT, traitsT>* pinsts, size_t ninsts);

template<typename charT, typename traitsT, typename... paramsT>
basic_tinyfmt<charT, traitsT>&
format(basic_tinyfmt<charT, traitsT>& fmt, const charT* stempl, const paramsT&... params);

/* Examples:
 *   rocket::format(fmt, "hello $1 $2", "world", '!');   // outputs "hello world!"
 *   rocket::format(fmt, "${1} + $1 = ${2}", 1, 2);      // outputs "1 + 1 = 2"
 *   rocket::format(fmt, "literal $$");                  // outputs "literal $"
 *   rocket::format(fmt, "funny $0 string");             // outputs "funny funny $0 string string"
 *   rocket::format(fmt, "$123", 'x');                   // outputs "x23"
 *   rocket::format(fmt, "${12}3", 'x');                 // throws an exception
 */

template<typename charT, typename traitsT>
struct basic_formatter
  {
    using tinyfmt_type   = basic_tinyfmt<charT, traitsT>;
    using callback_type  = tinyfmt_type& (tinyfmt_type&, const void*);

    callback_type* ifunc;
    const void* param;

    tinyfmt_type&
    submit(tinyfmt_type& fmt)
    const
      { return this->ifunc(fmt, this->param);  }
  };

template<typename charT, typename traitsT = char_traits<charT>, typename paramT>
constexpr
basic_formatter<charT, traitsT>
make_default_formatter(paramT& param)
noexcept
  {
    return { [](basic_tinyfmt<charT, traitsT>& fmt, const void* ptr) -> basic_tinyfmt<charT, traitsT>&
                 { return fmt << *(static_cast<const paramT*>(ptr));  },
             ::std::addressof(param) };
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
      const charT* pp = traitsT::find(bp, static_cast<size_t>(ep - bp), traitsT::to_char_type('$'));
      if(!pp) {
        // No placeholder has been found. Write all remaining characters verbatim and exit.
        fmt.putn(bp, static_cast<size_t>(ep - bp));
        break;
      }
      // A placeholder has been found. Write all characters preceding it.
      fmt.putn(bp, static_cast<size_t>(pp - bp));

      // Skip the dollar sign and parse the placeholder.
      if(pp == ep) {
        noadl::sprintf_and_throw<invalid_argument>("format: incomplete placeholder (dangling `$`)");
      }
      typename traitsT::int_type ch = traitsT::to_int_type(*++pp);
      bp = ++pp;

      // Replace the placeholder.
      if(ch == '$') {
        // Write a plain dollar sign.
        fmt.putc(traitsT::to_char_type('$'));
        continue;
      }

      // The placeholder shall contain a valid index.
      size_t index = 0;
      switch(ch) {
        case '{': {
          // Look for the terminator.
          bp = traitsT::find(bp, static_cast<size_t>(ep - bp), traitsT::to_char_type('}'));
          if(!bp) {
            noadl::sprintf_and_throw<invalid_argument>("format: incomplete placeholder (no matching `}`)");
          }
          // Parse the argument index.
          ptrdiff_t ndigs = bp++ - pp;
          if(ndigs < 1) {
            noadl::sprintf_and_throw<invalid_argument>("format: missing argument index");
          }
          if(ndigs > 3) {
            noadl::sprintf_and_throw<invalid_argument>("format: too many digits (`%td` > `3`)", ndigs);
          }
          // Collect digits.
          while(--ndigs >= 0) {
            ch = traitsT::to_int_type(*(pp++));
            if((ch < '0') || ('9' < ch)) {
              noadl::sprintf_and_throw<invalid_argument>("format: invalid digit (character `%c`)", (int)ch);
            }
            index = index * 10 + static_cast<size_t>(ch - '0');
          }
          break;
        }

        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
          // Accept a single decimal digit.
          index = static_cast<size_t>(ch - '0');
          break;

        default:
          noadl::sprintf_and_throw<invalid_argument>("format: invalid placeholder (sequence `$%c`)", (int)ch);
      }

      // Replace the placeholder.
      if(index == 0)
        fmt.putn(stempl, ntempl);
      else if(index <= ninsts)
        pinsts[index-1].submit(fmt);
      else
        noadl::sprintf_and_throw<invalid_argument>("format: no enough arguments (`%zu` > `%zu`)", index, ninsts);
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

template<typename charT, typename traitsT, typename... paramsT>
basic_tinyfmt<charT, traitsT>&
format(basic_tinyfmt<charT, traitsT>& fmt, const charT* stempl, const paramsT&... params)
  {
    basic_formatter<charT, traitsT> insts[] = { noadl::make_default_formatter<charT, traitsT>(params)... };
    return noadl::vformat(fmt, stempl, traitsT::length(stempl), insts, sizeof...(params));
  }

using formatter   = basic_formatter<char>;
using wformatter  = basic_formatter<wchar_t>;

extern template
tinyfmt&
vformat(tinyfmt&, const char*, size_t, const formatter*, size_t);

extern template
wtinyfmt&
vformat(wtinyfmt&, const wchar_t*, size_t, const wformatter*, size_t);

}  // namespace rocket

#endif
