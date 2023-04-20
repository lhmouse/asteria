// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_FORMAT_
#define ROCKET_FORMAT_

#include "tinyfmt.hpp"
#include "throw.hpp"
namespace rocket {

template<typename charT>
struct basic_formatter
  {
    using tinyfmt_type   = basic_tinyfmt<charT>;
    using callback_type  = void (tinyfmt_type&, const void*);

    callback_type* ifunc;
    const void* param;

    constexpr
    basic_formatter() noexcept
      : ifunc(nullptr), param(nullptr)
      { }

    constexpr
    basic_formatter(callback_type* xifunc, const void* xparam) noexcept
      : ifunc(xifunc), param(xparam)
      { }

    void
    do_format_output(tinyfmt_type& fmt) const
      { this->ifunc(fmt, this->param);  }

    // Format the given value with `operator<<` via argument-dependent lookup.
    template<typename valueT>
    static
    void
    default_callback(tinyfmt_type& fmt, const void* ptr)
      { fmt << *static_cast<const valueT*>(ptr);  }
  };

template<typename charT, typename valueT>
constexpr
basic_formatter<charT>
make_default_formatter(basic_tinyfmt<charT>& /*fmt*/, const valueT& value) noexcept
  {
    using formatter = basic_formatter<charT>;
    constexpr auto callback = formatter::template default_callback<valueT>;
    return formatter(callback, ::std::addressof(value));
  }

template<typename charT>
basic_tinyfmt<charT>&
vformat(basic_tinyfmt<charT>& fmt, const charT* stempl, const basic_formatter<charT>* pinsts, size_t ninsts)
  {
    const charT* base = stempl;
    const charT* next = stempl;

    while(*next != charT()) {
      // Look for the next placeholder.
      // XXX: For multi-byte strings this is faulty, as a non-leading byte
      //      may coincidentally match the dollar sign.
      if(*next != charT('$')) {
        next ++;
        continue;
      }

      if(next != base)
        fmt.putn(base, static_cast<size_t>(next - base));

      next ++;
      if(*next == charT()) {
        // end of string
        noadl::sprintf_and_throw<invalid_argument>(
            "vformat: dangling `$` in template string");
      }
      else if(*next == charT('$')) {
        // literal `$`
        fmt.putc(charT('$'));
      }
      else if((*next >= charT('0')) && (*next <= charT('9'))) {
        // simple specifier
        unsigned ai = static_cast<unsigned>(*next - charT('0'));
        if(ai > ninsts)
          noadl::sprintf_and_throw<out_of_range>(
              "vformat: no enough arguments (`%lld > `%lld)",
              static_cast<long long>(ai), static_cast<long long>(ninsts));

        // Zero denotes the format string and non-zero denotes an argument.
        // No padding or alignment is added for this specifier.
        if(ai == 0)
          fmt.puts(stempl);
        else
          (pinsts + ai - 1)->do_format_output(fmt);
      }
      else if(*next == charT('{')) {
        // This is the long, generic form, such as `${42}`.
        base = next;
        next = noadl::xstrchr(base + 1, charT('}'));
        if(!next)
          noadl::sprintf_and_throw<invalid_argument>(
              "vformat: unmatched `${` in template string");

        // At this moment, only the index of an argument is accepted.
        // TODO: Support more comprehensive placeholders.
        unsigned ai = 0;

        while(++ base != next) {
          // XXX: this is sloppy and needs polishing.
          // XXX: I'm too lazy to check for overflows.
          if((*base >= charT('0')) && (*base <= charT('9')))
            ai = ai * 10 + static_cast<unsigned>(*base - charT('0'));
        }

        if(ai > ninsts)
          noadl::sprintf_and_throw<out_of_range>(
              "vformat: no enough arguments (`%lld > `%lld)",
              static_cast<long long>(ai), static_cast<long long>(ninsts));

        // Zero denotes the format string and non-zero denotes an argument.
        // TODO: Support more comprehensive placeholders.
        if(ai == 0)
          fmt.puts(stempl);
        else
          (pinsts + ai - 1)->do_format_output(fmt);
      }
      else
        noadl::sprintf_and_throw<invalid_argument>(
            "vformat: invalid placeholder `$%c` in template string",
            static_cast<int>(*next));

      // Move past this placeholder.
      next ++;
      base = next;
    }

    if(next != base)
      fmt.putn(base, static_cast<size_t>(next - base));

    return fmt;
  }

template<typename charT, typename... paramsT>
inline
basic_tinyfmt<charT>&
format(basic_tinyfmt<charT>& fmt, const charT* stempl, const paramsT&... params)
  {
    using formatter = basic_formatter<charT>;
    formatter insts[] = { noadl::make_default_formatter(fmt, params)..., formatter() };
    return noadl::vformat(fmt, stempl, insts, sizeof...(params));
  }

using formatter     = basic_formatter<char>;
using wformatter    = basic_formatter<wchar_t>;
using u16formatter  = basic_formatter<char16_t>;
using u32formatter  = basic_formatter<char32_t>;

extern template tinyfmt& vformat(tinyfmt&, const char*, const formatter*, size_t);
extern template wtinyfmt& vformat(wtinyfmt&, const wchar_t*, const wformatter*, size_t);
extern template u16tinyfmt& vformat(u16tinyfmt&, const char16_t*, const u16formatter*, size_t);
extern template u32tinyfmt& vformat(u32tinyfmt&, const char32_t*, const u32formatter*, size_t);

}  // namespace rocket
#endif
