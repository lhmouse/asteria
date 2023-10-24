// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_TINYFMT_
#  error Please include <rocket/tinyfmt.hpp> instead.
#endif
namespace details_tinyfmt {

template<typename charT>
void
do_format_errno(basic_tinyfmt<charT>& fmt)
  {
    ascii_numput nump;
    nump.put_DI(errno);
    fmt.putn_latin1(nump.data(), nump.size());
  }

template<typename charT>
void
do_format_strerror_errno(basic_tinyfmt<charT>& fmt)
  {
    char storage[1024];
    const char* str;
#ifdef _GNU_SOURCE
    str = ::strerror_r(errno, storage, sizeof(storage));
#else
    ::strerror_r(errno, storage, sizeof(storage));
    str = storage;
#endif
    fmt.putn_latin1(str, ::strlen(str));
  }

template<typename charT>
void
do_format_time_iso(basic_tinyfmt<charT>& fmt, const ::tm& tm, long nsec)
  {
    char stemp[] = "2014-09-26 00:58:26.123456789";
    uint64_t date;
    ascii_numput nump;

    date = (uint32_t) tm.tm_year + 1900;
    date *= 1000;
    date += (uint32_t) tm.tm_mon + 1;
    date *= 1000;
    date += (uint32_t) tm.tm_mday;
    date *= 1000;
    date += (uint32_t) tm.tm_hour;
    date *= 1000;
    date += (uint32_t) tm.tm_min;
    date *= 1000;
    date += (uint32_t) tm.tm_sec;

    nump.put_DU(date, 19);
    ::memcpy(stemp, nump.data(), 19);

    stemp[4] = '-';
    stemp[7] = '-';
    stemp[10] = ' ';
    stemp[13] = ':';
    stemp[16] = ':';

    nump.put_DU((uint32_t) nsec, 9);
    ::memcpy(stemp + 20, nump.data(), 9);

    fmt.putn_latin1(stemp, 29);
  }

}  // namespace details_tinyfmt
