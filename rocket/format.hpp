// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_FORMAT_
#define ROCKET_FORMAT_

#include "tinyfmt.hpp"
#include "xthrow.hpp"
namespace rocket {

#include "details/format.ipp"

template<typename charT>
struct basic_formatter
  {
    using tinyfmt_type   = basic_tinyfmt<charT>;
    using callback_type  = void (tinyfmt_type&, const void*);

    callback_type* ifunc;
    const void* param;

    constexpr
    basic_formatter() noexcept
      :
        ifunc(nullptr), param(nullptr)
      { }

    constexpr
    basic_formatter(callback_type* xifunc, const void* xparam) noexcept
      :
        ifunc(xifunc), param(xparam)
      { }

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
make_default_formatter(const basic_tinyfmt<charT>&, const valueT& value) noexcept
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

    for(;;) {
      // Look for the next placeholder.
      // XXX: For non-UTF multi-byte strings this is faulty, as trailing
      //      bytes may match the dollar sign coincidentally.
      while((*next != charT()) && (*next != charT('$')))
        next ++;

      if(base != next) {
        fmt.putn(base, static_cast<size_t>(next - base));
        base = next;
      }

      // If the end of the template string has been reached, stop.
      if(*next == charT())
        break;

      // Parse this plafceholder.
      ROCKET_ASSERT(*next == charT('$'));
      next ++;
      switch(*next) {
        case charT('$'): {
          // literal `$`
          fmt.putc(charT('$'));
          break;
        }

        case charT('0'):
        case charT('1'):
        case charT('2'):
        case charT('3'):
        case charT('4'):
        case charT('5'):
        case charT('6'):
        case charT('7'):
        case charT('8'):
        case charT('9'): {
          // simple placeholder
          uint32_t iarg = static_cast<unsigned char>(*next - charT('0'));
          if(iarg == 0) {
            // `0` denotes the template string.
            fmt.putn(stempl, noadl::xstrlen(stempl));
          }
          else if(iarg <= ninsts) {
            // `1`...`ninsts` denote ordinal arguments.
            const basic_formatter<charT>* inst = pinsts + iarg - 1;
            inst->ifunc(fmt, inst->param);
          }
          else {
            static constexpr charT bad_index[] = { '(','b','a','d','-','i','n','d','e','x',')' };
            fmt.putn(bad_index, 11);
          }
          break;
        }

        case charT('{'): {
          // composite placeholder
          next ++;
          base = next;

          while((*next != charT()) && (*next != charT('}')))
            next ++;

          if(*next == charT())
            noadl::sprintf_and_throw<invalid_argument>(
                "vformat: no matching `}` in template string");

          // Check for built-in placeholders.
          static constexpr charT errno_str[] = { 'e','r','r','n','o',':','s','t','r' };
          static constexpr charT errno_full[] = { 'e','r','r','n','o',':','f','u','l','l' };
          static constexpr charT time_utc[] = { 't','i','m','e',':','u','t','c' };

          if(noadl::xmemeq(base, (size_t) (next - base), errno_str, 5)) {
            // `errno`: value of `errno`
            details_format::do_format_errno(fmt);
            break;
          }

          if(noadl::xmemeq(base, (size_t) (next - base), errno_str, 9)) {
            // `errno:str`: description of `errno`
            details_format::do_format_strerror_errno(fmt);
            break;
          }

          if(noadl::xmemeq(base, (size_t) (next - base), errno_full, 10)) {
            // `errno:full`: value of `errno` followed by its description
            details_format::do_format_strerror_errno(fmt);
            fmt.putn_latin1(" (errno ", 8);
            details_format::do_format_errno(fmt);
            fmt.putn_latin1(")", 1);
            break;
          }

          if(noadl::xmemeq(base, (size_t) (next - base), time_utc, 4)) {
            // `time`: current date and time in local time zone
            ::timespec tv;
            ::clock_gettime(CLOCK_REALTIME, &tv);
            ::tm tm;
            ::localtime_r(&(tv.tv_sec), &tm);
            details_format::do_format_time_iso(fmt, tm, tv.tv_nsec);
            break;
          }

          if(noadl::xmemeq(base, (size_t) (next - base), time_utc, 8)) {
            // `time:utc`: current date and time in UTC
            ::timespec tv;
            ::clock_gettime(CLOCK_REALTIME, &tv);
            ::tm tm;
            ::gmtime_r(&(tv.tv_sec), &tm);
            details_format::do_format_time_iso(fmt, tm, tv.tv_nsec);
            break;
          }

          if(next == base) {
            // ``: valid but no output
            break;
          }

          // The placeholder shall be a non-negative integer.
          uint32_t iarg = 0;

          while(base != next) {
            if((*base < charT('0')) || (*base > charT('9'))) {
              iarg = UINT32_MAX;
              break;
            }
            else if(iarg >= 0xFFFFU) {
              iarg = UINT32_MAX;
              break;
            }
            iarg *= 10U;
            iarg += static_cast<unsigned char>(*base - charT('0'));
            base ++;
          }

          if(iarg == 0) {
            // `0` denotes the template string.
            fmt.putn(stempl, noadl::xstrlen(stempl));
          }
          else if(iarg <= ninsts) {
            // `1`...`ninsts` denote ordinal arguments.
            const basic_formatter<charT>* inst = pinsts + iarg - 1;
            inst->ifunc(fmt, inst->param);
          }
          else {
            static constexpr charT bad_index[] = { '(','b','a','d','-','i','n','d','e','x',')' };
            fmt.putn(bad_index, 11);
          }
          break;
        }

        case charT():
          noadl::sprintf_and_throw<invalid_argument>(
              "vformat: dangling `$` at end of template string");

        default:
          noadl::sprintf_and_throw<invalid_argument>(
              "vformat: unknown placeholder `$%c` in template string",
              static_cast<int>(*next));
      }

      next ++;
      base = next;
    }
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
