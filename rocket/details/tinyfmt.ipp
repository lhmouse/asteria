// This file is part of Asteria.
// Copyright (C) 2018-2025, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_TINYFMT_
#  error Please include <rocket/tinyfmt.hpp> instead.
#endif
namespace details_tinyfmt {

template<typename charT, typename valueT>
basic_tinyfmt<charT>&
do_format_number(basic_tinyfmt<charT>& fmt, const valueT& value)
  {
    ascii_numput nump;
    nump.put(value);
    return fmt.putn_latin1(nump.data(), nump.size());
  }

template<typename charT>
basic_tinyfmt<charT>&
do_format_errno(basic_tinyfmt<charT>& fmt)
  {
    ascii_numput nump;
    nump.put_DI(errno);
    return fmt.putn_latin1(nump.data(), nump.size());
  }

template<typename charT>
basic_tinyfmt<charT>&
do_format_strerror_errno(basic_tinyfmt<charT>& fmt)
  {
    char str_stor[128];
    const char* str = str_stor;
#if defined _GNU_SOURCE
    str = ::strerror_r(errno, str_stor, sizeof(str_stor));
#else
    ::strerror_r(errno, str_stor, sizeof(str_stor));
#endif
    return fmt.putn_latin1(str, ::strlen(str));
  }

template<typename charT>
basic_tinyfmt<charT>&
do_format_time_iso(basic_tinyfmt<charT>& fmt, const struct tm& tm, long nsec)
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
    return fmt.putn_latin1(stemp, 29);
  }

}  // namespace details_tinyfmt
