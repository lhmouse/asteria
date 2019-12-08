// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_FORMAT_HPP_
#define ROCKET_FORMAT_HPP_

#include "tinyfmt.hpp"
#include "throw.hpp"

namespace rocket {

template<typename charT, typename traitsT = char_traits<charT>> struct basic_formatter;

template<typename charT, typename traitsT>
    basic_tinyfmt<charT, traitsT>& vformat(basic_tinyfmt<charT, traitsT>& fmt, const charT* templ,
                                           const basic_formatter<charT, traitsT>* pinsts, size_t ninsts);
template<typename charT, typename traitsT, typename... paramsT>
    basic_tinyfmt<charT, traitsT>& format(basic_tinyfmt<charT, traitsT>& fmt, const charT* templ,
                                          const paramsT&... params);

/* Examples:
 *   rocket::format(fmt, "hello $1 $2", "world", '!');   // outputs "hello world!"
 *   rocket::format(fmt, "${1} + $1 = ${2}", 1, 2);      // outputs "1 + 1 = 2"
 *   rocket::format(fmt, "literal $$");                  // outputs "literal $"
 *   rocket::format(fmt, "funny $0 string");             // outputs "funny funny $0 string string"
 *   rocket::format(fmt, "$123", 'x');                   // outputs "x23"
 *   rocket::format(fmt, "${12}3", 'x');                 // throws an exception
 */

template<typename charT, typename traitsT> struct basic_formatter
  {
    using tinyfmt_type   = basic_tinyfmt<charT, traitsT>;
    using callback_type  = tinyfmt_type& (tinyfmt_type&, const void*);

    callback_type* ifunc;
    const void* param;

    tinyfmt_type& submit(tinyfmt_type& fmt) const
      {
        return this->ifunc(fmt, this->param);
      }
  };

template<typename charT, typename traitsT = char_traits<charT>, typename paramT>
    constexpr basic_formatter<charT, traitsT> make_default_formatter(paramT& param)
  {
    return { [](basic_tinyfmt<charT, traitsT>& fmt, const void* ptr) -> basic_tinyfmt<charT, traitsT>&
                 { return fmt << *(static_cast<const paramT*>(ptr));  },
             ::std::addressof(param) };
  }

template<typename charT, typename traitsT>
    basic_tinyfmt<charT, traitsT>& vformat(basic_tinyfmt<charT, traitsT>& fmt, const charT* templ,
                                           const basic_formatter<charT, traitsT>* pinsts, size_t ninsts)
  {
    // Get the range of `format`. The end pointer points to the null terminator.
    const charT* bp = templ;
    const charT* ep = bp + traitsT::length(templ);
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
      // This will not result in overflows as `ep` points to the null terminator.
      unsigned long ch = static_cast<unsigned long>(traitsT::to_int_type(*++pp));
      if(ch == 0) {
        noadl::sprintf_and_throw<invalid_argument>("format: placeholder incomplete (dangling `$`)");
      }
      bp = ++pp;
      // Replace the placeholder.
      if(ch == '$') {
        // Write a plain dollar sign.
        fmt.putc(traitsT::to_char_type('$'));
        continue;
      }
      // The placeholder shall contain a valid index.
      size_t index;
      if(ch - '0' < 10) {
        // Accept a single decimal digit.
        index = ch - '0';
      }
      else if(ch == '{') {
        // At least one digit is required.
        ch = static_cast<unsigned long>(traitsT::to_int_type(*pp));
        if(ch - '0' >= 10)
          noadl::sprintf_and_throw<invalid_argument>("format: argument index invalid (offset `%td`)", pp - templ);
        index = ch - '0';
        bp = pp;
        // Look for the terminator.
        for(;;) {
          if(*++pp == 0)
            noadl::sprintf_and_throw<invalid_argument>("format: placeholder incomplete (no matching `}`)");
          if(*pp == '}')
            break;
        }
        // Get more digits.
        for(;;) {
          ch = static_cast<uint8_t>(*++bp);
          if(ch - '0' >= 10)
            break;
          if(index >= 999)
            noadl::sprintf_and_throw<invalid_argument>("format: argument index too large");
          index = index * 10 + ch - '0';
        }
        // TODO: Support the ':FORMAT' syntax.
        if(bp != pp)
          noadl::sprintf_and_throw<invalid_argument>("format: excess characters in placeholder (offset `%td`)", pp - templ);
        bp = ++pp;
      }
      else {
        noadl::sprintf_and_throw<invalid_argument>("format: placeholder invalid (offset `%td`)", pp - templ);
      }
      // Replace the placeholder.
      if(index == 0) {
        fmt.putn(templ, static_cast<size_t>(ep - templ));
      }
      else if(index <= ninsts) {
        pinsts[index-1].submit(fmt);
      }
      else
        noadl::sprintf_and_throw<invalid_argument>("format: no enough arguments (`%zu` > `%zu`)", index, ninsts);
    }
    return fmt;
  }

template<typename charT, typename traitsT, typename... paramsT>
    basic_tinyfmt<charT, traitsT>& format(basic_tinyfmt<charT, traitsT>& fmt, const charT* templ,
                                          const paramsT&... params)
  {
    basic_formatter<charT, traitsT> insts[] = { make_default_formatter<charT, traitsT>(params)... };
    return noadl::vformat(fmt, templ, insts, sizeof...(params));
  }

using formatter   = basic_formatter<char>;
using wformatter  = basic_formatter<wchar_t>;

extern template tinyfmt&  vformat(tinyfmt&  fmt, const char*    templ, const formatter*  pinsts, size_t ninsts);
extern template wtinyfmt& vformat(wtinyfmt& fmt, const wchar_t* templ, const wformatter* pinsts, size_t ninsts);

}  // namespace rocket

#endif
