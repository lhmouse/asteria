// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_FORMAT_HPP_
#define ROCKET_FORMAT_HPP_

#include "tinyfmt.hpp"
#include "throw.hpp"

namespace rocket {

template<typename charT, typename traitsT, typename... paramsT>
    basic_tinyfmt<charT, traitsT>& format(basic_tinyfmt<charT, traitsT>& out, const charT* fmt, const paramsT&... params);

/* Examples:
 *   rocket::format(out, "hello $1 $2", "world", '!');   // outputs "hello world!"
 *   rocket::format(out, "${1} + $1 = ${2}", 1, 2);      // outputs "1 + 1 = 2"
 *   rocket::format(out, "literal $$");                  // outputs "literal $"
 *   rocket::format(out, "funny $0 string");             // outputs "funny funny $0 string string"
 *   rocket::format(out, "$123", 'x');                   // outputs "x23"
 *   rocket::format(out, "${12}3", 'x');                 // throws an exception
 */

    namespace details_format {

    template<typename charT, typename traitsT> struct inserter
      {
        typename add_lvalue_reference<void (basic_tinyfmt<charT, traitsT>&, const void*)>::type ifunc;
        const void* param;

        void do_insert(basic_tinyfmt<charT, traitsT>& out) const
          {
            this->ifunc(out, this->param);
          }
      };

    template<typename charT, typename traitsT, typename paramT>
        constexpr inserter<charT, traitsT> make_inserter(paramT& param) noexcept
      {
        return { *[](basic_tinyfmt<charT, traitsT>& out, const void* p) { out << *(static_cast<const paramT*>(p));  },
                 ::std::addressof(param) };
      }

    }

template<typename charT, typename traitsT, typename... paramsT>
    basic_tinyfmt<charT, traitsT>& format(basic_tinyfmt<charT, traitsT>& out, const charT* fmt, const paramsT&... params)
  {
    // Construct an array of type-erased inserters.
    details_format::inserter<charT, traitsT> inserters[] = { details_format::make_inserter<charT, traitsT>(params)... };
    // Get the range of `format`. The end pointer points to the null terminator.
    const charT* bp = fmt;
    const charT* ep = bp + traitsT::length(bp);
    for(;;) {
      // Look for a placeholder sequence.
      const charT* pp = traitsT::find(bp, static_cast<size_t>(ep - bp), traitsT::to_char_type('$'));
      if(!pp) {
        // No placeholder has been found. Write all remaining characters verbatim and exit.
        out.putn(bp, static_cast<size_t>(ep - bp));
        break;
      }
      // A placeholder has been found. Write all characters preceding it.
      out.putn(bp, static_cast<size_t>(pp - bp));
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
        out.putc(traitsT::to_char_type('$'));
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
          noadl::sprintf_and_throw<invalid_argument>("format: argument index invalid (offset `%td`)", pp - fmt);
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
          noadl::sprintf_and_throw<invalid_argument>("format: excess characters in placeholder (offset `%td`)", pp - fmt);
        bp = ++pp;
      }
      else {
        noadl::sprintf_and_throw<invalid_argument>("format: placeholder invalid (offset `%td`)", pp - fmt);
      }
      // Replace the placeholder.
      if(index == 0) {
        out.putn(fmt, static_cast<size_t>(ep - fmt));
      }
      else if(index <= sizeof...(params)) {
        inserters[index-1].do_insert(out);
      }
      else
        noadl::sprintf_and_throw<invalid_argument>("format: no enough arguments (`%zu` > `%zu`)", index, sizeof...(params));
    }
    return out;
  }

}  // namespace rocket

#endif
