// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "tinyfmt.hpp"
#include "static_vector.hpp"
#include <limits.h>  // MB_LEN_MAX
#include <wchar.h>
#ifdef HAVE_UCHAR_H
#  include <uchar.h>
#endif
namespace rocket {
namespace {

template<typename xwcharT, typename xmbrtowcT>
void
do_putmbn_common(basic_tinybuf<xwcharT>& buf, xmbrtowcT&& xmbrtowc, const char* s, size_t n)
  {
    ::mbstate_t mbst = { };
    static_vector<xwcharT, 128> wtemp;
    const char* sptr = s;
    while(sptr != s + n) {
      // Decode one multi-byte character.
      xwcharT wc;
      int mblen = (int) xmbrtowc(&wc, sptr, (size_t) (s + n - sptr), &mbst);

      switch(mblen) {
        case -3:
          // trailing surrogate written; no input consumed
          wtemp.push_back(wc);
          break;

        case -2:
        case -1:
          // input invalid
          mbst = { };
          wtemp.push_back((unsigned char) *sptr);
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
          sptr += (unsigned int) mblen;
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

template class basic_tinyfmt<char>;
template class basic_tinyfmt<wchar_t>;
template class basic_tinyfmt<char16_t>;
template class basic_tinyfmt<char32_t>;

template<>
tinyfmt&
tinyfmt::
putmbn(const char* s, size_t n)
  {
    this->do_get_tinybuf_nonconst().putn(s, n);
    return *this;
  }

template<>
tinyfmt&
tinyfmt::
putmbs(const char* s)
  {
    this->do_get_tinybuf_nonconst().puts(s);
    return *this;
  }

template<>
wtinyfmt&
wtinyfmt::
putmbn(const char* s, size_t n)
  {
    do_putmbn_common(this->do_get_tinybuf_nonconst(), ::mbrtowc, s, n);
    return *this;
  }

template<>
wtinyfmt&
wtinyfmt::
putmbs(const char* s)
  {
    do_putmbn_common(this->do_get_tinybuf_nonconst(), ::mbrtowc, s, ::strlen(s));
    return *this;
  }

template<>
u16tinyfmt&
u16tinyfmt::
putmbn(const char* s, size_t n)
  {
#ifdef HAVE_UCHAR_H
    do_putmbn_common(this->do_get_tinybuf_nonconst(), ::mbrtoc16, s, n);
    return *this;
#else
    noadl::sprintf_and_throw<domain_error>("u16tinyfmt: UTF-16 functions not available");
#endif
  }

template<>
u16tinyfmt&
u16tinyfmt::
putmbs(const char* s)
  {
#ifdef HAVE_UCHAR_H
    do_putmbn_common(this->do_get_tinybuf_nonconst(), ::mbrtoc16, s, ::strlen(s));
    return *this;
#else
    noadl::sprintf_and_throw<domain_error>("u16tinyfmt: UTF-16 functions not available");
#endif
  }

template<>
u32tinyfmt&
u32tinyfmt::
putmbn(const char* s, size_t n)
  {
#ifdef HAVE_UCHAR_H
    do_putmbn_common(this->do_get_tinybuf_nonconst(), ::mbrtoc32, s, n);
    return *this;
#else
    noadl::sprintf_and_throw<domain_error>("u32tinyfmt: UTF-32 functions not available");
#endif
  }

template<>
u32tinyfmt&
u32tinyfmt::
putmbs(const char* s)
  {
#ifdef HAVE_UCHAR_H
    do_putmbn_common(this->do_get_tinybuf_nonconst(), ::mbrtoc32, s, ::strlen(s));
    return *this;
#else
    noadl::sprintf_and_throw<domain_error>("u32tinyfmt: UTF-32 functions not available");
#endif
  }

template tinyfmt& operator<<(tinyfmt&, char);
template wtinyfmt& operator<<(wtinyfmt&, wchar_t);
template u16tinyfmt& operator<<(u16tinyfmt&, char16_t);
template u32tinyfmt& operator<<(u32tinyfmt&, char32_t);

template wtinyfmt& operator<<(wtinyfmt&, const wchar_t*);
template wtinyfmt& operator<<(wtinyfmt&, const char*);
template u16tinyfmt& operator<<(u16tinyfmt&, const char16_t*);
template u16tinyfmt& operator<<(u16tinyfmt&, const char*);
template u32tinyfmt& operator<<(u32tinyfmt&, const char32_t*);
template u32tinyfmt& operator<<(u32tinyfmt&, const char*);

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

}  // namespace rocket
