// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#include "tinyfmt.hpp"
#include "static_vector.hpp"
#include <limits.h>  // MB_LEN_MAX
#include <wchar.h>
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#ifdef HAVE_UCHAR_H
#  include <uchar.h>
#endif
namespace rocket {
namespace {

template<typename xwcharT, typename xmbrtowcT>
void
do_putmbn_common(basic_tinybuf<xwcharT>& buf, xmbrtowcT&& xmbrtowc, const char* s)
  {
    if(!s) {
      static constexpr xwcharT null[] = { '(','n','u','l','l',')' };
      buf.putn(null, 6);
      return;
    }

    ::mbstate_t mbst = { };
    static_vector<xwcharT, 128> wtemp;
    const char* sptr = s;
    while(*sptr != 0) {
      // Decode one multi-byte character.
      xwcharT wc;
      int mblen = static_cast<int>(xmbrtowc(&wc, sptr, MB_LEN_MAX, &mbst));

      switch(mblen) {
        case -3:
          // trailing surrogate written; no input consumed
          wtemp.push_back(wc);
          break;

        case -2:
        case -1:
          // input invalid
          mbst = { };
          wtemp.push_back(static_cast<unsigned char>(*sptr));
          sptr ++;
          break;

        case 0:
          // zero byte written
          wtemp.push_back(0);
          sptr ++;
          break;

        default:
          // multi-byte character written; `mblen` bytes consumed
          wtemp.push_back(wc);
          sptr += static_cast<unsigned>(mblen);
          break;
      }

      if(wtemp.size() != wtemp.capacity())
        continue;

      // If the buffer is full now, flush pending characters.
      buf.putn(wtemp.data(), wtemp.size());
      wtemp.clear();
    }

    if(wtemp.size() != 0)
      buf.putn(wtemp.data(), wtemp.size());
  }

}  // namespace

basic_tinyfmt<wchar_t>&
operator<<(basic_tinyfmt<wchar_t>& fmt, const char* s)
  {
    do_putmbn_common(fmt.mut_buf(), ::mbrtowc, s);
    return fmt;
  }

basic_tinyfmt<char16_t>&
operator<<(basic_tinyfmt<char16_t>& fmt, const char* s)
  {
#ifdef HAVE_UCHAR_H
    do_putmbn_common(fmt.mut_buf(), ::mbrtoc16, s);
    return fmt;
#else
    noadl::sprintf_and_throw<domain_error>("u16tinyfmt: UTF-16 functions not available");
#endif
  }

basic_tinyfmt<char32_t>&
operator<<(basic_tinyfmt<char32_t>& fmt, const char* s)
  {
#ifdef HAVE_UCHAR_H
    do_putmbn_common(fmt.mut_buf(), ::mbrtoc32, s);
    return fmt;
#else
    noadl::sprintf_and_throw<domain_error>("u32tinyfmt: UTF-32 functions not available");
#endif
  }

template class basic_tinyfmt<char>;
template class basic_tinyfmt<wchar_t>;
template class basic_tinyfmt<char16_t>;
template class basic_tinyfmt<char32_t>;

template tinyfmt& operator<<(tinyfmt&, char);
template wtinyfmt& operator<<(wtinyfmt&, wchar_t);
template u16tinyfmt& operator<<(u16tinyfmt&, char16_t);
template u32tinyfmt& operator<<(u32tinyfmt&, char32_t);

template tinyfmt& operator<<(tinyfmt&, const char*);
template wtinyfmt& operator<<(wtinyfmt&, const wchar_t*);
template u16tinyfmt& operator<<(u16tinyfmt&, const char16_t*);
template u32tinyfmt& operator<<(u32tinyfmt&, const char32_t*);

template tinyfmt& operator<<(tinyfmt&, const ascii_numput&);
template wtinyfmt& operator<<(wtinyfmt&, const ascii_numput&);
template u16tinyfmt& operator<<(u16tinyfmt&, const ascii_numput&);
template u32tinyfmt& operator<<(u32tinyfmt&, const ascii_numput&);

template tinyfmt& operator<<(tinyfmt&, signed char);
template wtinyfmt& operator<<(wtinyfmt&, signed char);
template u16tinyfmt& operator<<(u16tinyfmt&, signed char);
template u32tinyfmt& operator<<(u32tinyfmt&, signed char);

template tinyfmt& operator<<(tinyfmt&, signed short);
template wtinyfmt& operator<<(wtinyfmt&, signed short);
template u16tinyfmt& operator<<(u16tinyfmt&, signed short);
template u32tinyfmt& operator<<(u32tinyfmt&, signed short);

template tinyfmt& operator<<(tinyfmt&, signed);
template wtinyfmt& operator<<(wtinyfmt&, signed);
template u16tinyfmt& operator<<(u16tinyfmt&, signed);
template u32tinyfmt& operator<<(u32tinyfmt&, signed);

template tinyfmt& operator<<(tinyfmt&, signed long);
template wtinyfmt& operator<<(wtinyfmt&, signed long);
template u16tinyfmt& operator<<(u16tinyfmt&, signed long);
template u32tinyfmt& operator<<(u32tinyfmt&, signed long);

template tinyfmt& operator<<(tinyfmt&, signed long long);
template wtinyfmt& operator<<(wtinyfmt&, signed long long);
template u16tinyfmt& operator<<(u16tinyfmt&, signed long long);
template u32tinyfmt& operator<<(u32tinyfmt&, signed long long);

template tinyfmt& operator<<(tinyfmt&, unsigned char);
template wtinyfmt& operator<<(wtinyfmt&, unsigned char);
template u16tinyfmt& operator<<(u16tinyfmt&, unsigned char);
template u32tinyfmt& operator<<(u32tinyfmt&, unsigned char);

template tinyfmt& operator<<(tinyfmt&, unsigned short);
template wtinyfmt& operator<<(wtinyfmt&, unsigned short);
template u16tinyfmt& operator<<(u16tinyfmt&, unsigned short);
template u32tinyfmt& operator<<(u32tinyfmt&, unsigned short);

template tinyfmt& operator<<(tinyfmt&, unsigned);
template wtinyfmt& operator<<(wtinyfmt&, unsigned);
template u16tinyfmt& operator<<(u16tinyfmt&, unsigned);
template u32tinyfmt& operator<<(u32tinyfmt&, unsigned);

template tinyfmt& operator<<(tinyfmt&, unsigned long);
template wtinyfmt& operator<<(wtinyfmt&, unsigned long);
template u16tinyfmt& operator<<(u16tinyfmt&, unsigned long);
template u32tinyfmt& operator<<(u32tinyfmt&, unsigned long);

template tinyfmt& operator<<(tinyfmt&, unsigned long long);
template wtinyfmt& operator<<(wtinyfmt&, unsigned long long);
template u16tinyfmt& operator<<(u16tinyfmt&, unsigned long long);
template u32tinyfmt& operator<<(u32tinyfmt&, unsigned long long);

template tinyfmt& operator<<(tinyfmt&, float);
template wtinyfmt& operator<<(wtinyfmt&, float);
template u16tinyfmt& operator<<(u16tinyfmt&, float);
template u32tinyfmt& operator<<(u32tinyfmt&, float);

template tinyfmt& operator<<(tinyfmt&, double);
template wtinyfmt& operator<<(wtinyfmt&, double);
template u16tinyfmt& operator<<(u16tinyfmt&, double);
template u32tinyfmt& operator<<(u32tinyfmt&, double);

template tinyfmt& operator<<(tinyfmt&, const void*);
template wtinyfmt& operator<<(wtinyfmt&, const void*);
template u16tinyfmt& operator<<(u16tinyfmt&, const void*);
template u32tinyfmt& operator<<(u32tinyfmt&, const void*);

template tinyfmt& operator<<(tinyfmt&, void*);
template wtinyfmt& operator<<(wtinyfmt&, void*);
template u16tinyfmt& operator<<(u16tinyfmt&, void*);
template u32tinyfmt& operator<<(u32tinyfmt&, void*);

template tinyfmt& operator<<(tinyfmt&, const type_info&);
template wtinyfmt& operator<<(wtinyfmt&, const type_info&);
template u16tinyfmt& operator<<(u16tinyfmt&, const type_info&);
template u32tinyfmt& operator<<(u32tinyfmt&, const type_info&);

template tinyfmt& operator<<(tinyfmt&, const exception&);
template wtinyfmt& operator<<(wtinyfmt&, const exception&);
template u16tinyfmt& operator<<(u16tinyfmt&, const exception&);
template u32tinyfmt& operator<<(u32tinyfmt&, const exception&);

template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<1, 1000000000>>&);
template wtinyfmt& operator<<(wtinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<1, 1000000000>>&);
template u16tinyfmt& operator<<(u16tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<1, 1000000000>>&);
template u32tinyfmt& operator<<(u32tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<1, 1000000000>>&);

template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<1, 1000000>>&);
template wtinyfmt& operator<<(wtinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<1, 1000000>>&);
template u16tinyfmt& operator<<(u16tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<1, 1000000>>&);
template u32tinyfmt& operator<<(u32tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<1, 1000000>>&);

template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<1, 1000>>&);
template wtinyfmt& operator<<(wtinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<1, 1000>>&);
template u16tinyfmt& operator<<(u16tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<1, 1000>>&);
template u32tinyfmt& operator<<(u32tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<1, 1000>>&);

template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<1>>&);
template wtinyfmt& operator<<(wtinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<1>>&);
template u16tinyfmt& operator<<(u16tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<1>>&);
template u32tinyfmt& operator<<(u32tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<1>>&);

template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<60>>&);
template wtinyfmt& operator<<(wtinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<60>>&);
template u16tinyfmt& operator<<(u16tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<60>>&);
template u32tinyfmt& operator<<(u32tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<60>>&);

template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<3600>>&);
template wtinyfmt& operator<<(wtinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<3600>>&);
template u16tinyfmt& operator<<(u16tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<3600>>&);
template u32tinyfmt& operator<<(u32tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<3600>>&);

template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<86400>>&);
template wtinyfmt& operator<<(wtinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<86400>>&);
template u16tinyfmt& operator<<(u16tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<86400>>&);
template u32tinyfmt& operator<<(u32tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<86400>>&);

template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<604800>>&);
template wtinyfmt& operator<<(wtinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<604800>>&);
template u16tinyfmt& operator<<(u16tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<604800>>&);
template u32tinyfmt& operator<<(u32tinyfmt&, const ::std::chrono::duration<int64_t, ::std::ratio<604800>>&);

template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<1, 1000000000>>&);
template wtinyfmt& operator<<(wtinyfmt&, const ::std::chrono::duration<double, ::std::ratio<1, 1000000000>>&);
template u16tinyfmt& operator<<(u16tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<1, 1000000000>>&);
template u32tinyfmt& operator<<(u32tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<1, 1000000000>>&);

template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<1, 1000000>>&);
template wtinyfmt& operator<<(wtinyfmt&, const ::std::chrono::duration<double, ::std::ratio<1, 1000000>>&);
template u16tinyfmt& operator<<(u16tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<1, 1000000>>&);
template u32tinyfmt& operator<<(u32tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<1, 1000000>>&);

template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<1, 1000>>&);
template wtinyfmt& operator<<(wtinyfmt&, const ::std::chrono::duration<double, ::std::ratio<1, 1000>>&);
template u16tinyfmt& operator<<(u16tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<1, 1000>>&);
template u32tinyfmt& operator<<(u32tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<1, 1000>>&);

template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<1>>&);
template wtinyfmt& operator<<(wtinyfmt&, const ::std::chrono::duration<double, ::std::ratio<1>>&);
template u16tinyfmt& operator<<(u16tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<1>>&);
template u32tinyfmt& operator<<(u32tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<1>>&);

template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<60>>&);
template wtinyfmt& operator<<(wtinyfmt&, const ::std::chrono::duration<double, ::std::ratio<60>>&);
template u16tinyfmt& operator<<(u16tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<60>>&);
template u32tinyfmt& operator<<(u32tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<60>>&);

template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<3600>>&);
template wtinyfmt& operator<<(wtinyfmt&, const ::std::chrono::duration<double, ::std::ratio<3600>>&);
template u16tinyfmt& operator<<(u16tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<3600>>&);
template u32tinyfmt& operator<<(u32tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<3600>>&);

template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<86400>>&);
template wtinyfmt& operator<<(wtinyfmt&, const ::std::chrono::duration<double, ::std::ratio<86400>>&);
template u16tinyfmt& operator<<(u16tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<86400>>&);
template u32tinyfmt& operator<<(u32tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<86400>>&);

template tinyfmt& operator<<(tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<604800>>&);
template wtinyfmt& operator<<(wtinyfmt&, const ::std::chrono::duration<double, ::std::ratio<604800>>&);
template u16tinyfmt& operator<<(u16tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<604800>>&);
template u32tinyfmt& operator<<(u32tinyfmt&, const ::std::chrono::duration<double, ::std::ratio<604800>>&);

template tinyfmt& vformat(tinyfmt&, const char*, const formatter*, size_t);
template wtinyfmt& vformat(wtinyfmt&, const wchar_t*, const wformatter*, size_t);
template u16tinyfmt& vformat(u16tinyfmt&, const char16_t*, const u16formatter*, size_t);
template u32tinyfmt& vformat(u32tinyfmt&, const char32_t*, const u32formatter*, size_t);

}  // namespace rocket
