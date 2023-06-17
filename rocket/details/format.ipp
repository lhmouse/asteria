// This file is part of Asteria.
// Copyleft 2018 - 2023, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_FORMAT_
#  error Please include <rocket/format.hpp> instead.
#endif
namespace details_format {

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
    ascii_numput nump;
    nump.put_DU(uint32_t(tm.tm_year) + 1900U, 4);
    fmt.putn_latin1(nump.data(), 4);

    fmt.putc(charT('-'));
    nump.put_DU(uint32_t(tm.tm_mon) + 1U, 2);
    fmt.putn_latin1(nump.data(), 2);

    fmt.putc(charT('-'));
    nump.put_DU(uint32_t(tm.tm_mday), 2);
    fmt.putn_latin1(nump.data(), 2);

    fmt.putc(charT(' '));
    nump.put_DU(uint32_t(tm.tm_hour), 2);
    fmt.putn_latin1(nump.data(), 2);

    fmt.putc(charT(':'));
    nump.put_DU(uint32_t(tm.tm_min), 2);
    fmt.putn_latin1(nump.data(), 2);

    fmt.putc(charT(':'));
    nump.put_DU(uint32_t(tm.tm_sec), 2);
    fmt.putn_latin1(nump.data(), 2);

    fmt.putc(charT('.'));
    nump.put_DU(uint64_t(nsec), 9);
    fmt.putn_latin1(nump.data(), 9);

    fmt.putc(charT(' '));
    fmt.putc((tm.tm_gmtoff >= 0) ? charT('+') : charT('-'));
    nump.put_DU(uint32_t(::std::abs(tm.tm_gmtoff)) / 3600U, 2);
    fmt.putn_latin1(nump.data(), 2);
    nump.put_DU(uint32_t(::std::abs(tm.tm_gmtoff)) % 3600U / 60U, 2);
    fmt.putn_latin1(nump.data(), 2);
  }

}  // namespace details_format
