// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "chrono.hpp"
#include "../runtime/argument_reader.hpp"
#include "../runtime/runtime_error.hpp"
#include "../utils.hpp"
#include <time.h>  // ::clock_gettime(), ::timespec
namespace asteria {
namespace {

constexpr char s_2digit[100][2] =
  {
    '0','0','0','1','0','2','0','3','0','4','0','5','0','6','0','7','0','8','0','9',
    '1','0','1','1','1','2','1','3','1','4','1','5','1','6','1','7','1','8','1','9',
    '2','0','2','1','2','2','2','3','2','4','2','5','2','6','2','7','2','8','2','9',
    '3','0','3','1','3','2','3','3','3','4','3','5','3','6','3','7','3','8','3','9',
    '4','0','4','1','4','2','4','3','4','4','4','5','4','6','4','7','4','8','4','9',
    '5','0','5','1','5','2','5','3','5','4','5','5','5','6','5','7','5','8','5','9',
    '6','0','6','1','6','2','6','3','6','4','6','5','6','6','6','7','6','8','6','9',
    '7','0','7','1','7','2','7','3','7','4','7','5','7','6','7','7','7','8','7','9',
    '8','0','8','1','8','2','8','3','8','4','8','5','8','6','8','7','8','8','8','9',
    '9','0','9','1','9','2','9','3','9','4','9','5','9','6','9','7','9','8','9','9',
  };

inline
void
do_mempcpy(char*& wptr_out, const char* cstr, size_t len)
  {
    ::memcpy(wptr_out, cstr, len);
    wptr_out += len;
  }

inline
bool
do_match(const char*& rptr_out, const char* cstr, size_t len)
  {
    // If a previous match has failed, don't do anything.
    if(rptr_out == nullptr)
      return false;

    if(::memcmp(rptr_out, cstr, len) == 0) {
      // A match has been found, so move the read pointer past it.
      rptr_out += len;
      return true;
    }

    // No match has been found, so mark this as a failure.
    rptr_out = nullptr;
    return false;
  }

template<uint32_t N, uint32_t S>
inline
bool
do_match(const char*& rptr_out, uint32_t& add_to_value, const char (&cstrs)[N][S], int limit)
  {
    // If a previous match has failed, don't do anything.
    if(rptr_out == nullptr)
      return false;

    for(uint32_t k = 0;  k != N;  ++k) {
      size_t len = (limit >= 0) ? (uint32_t) limit : ::strlen(cstrs[k]);
      if(::memcmp(rptr_out, cstrs[k], len) == 0) {
        // A match has been found, so move the read pointer past it.
        rptr_out += len;
        add_to_value += k;
        return true;
      }
    }

    // No match has been found, so mark this as a failure.
    rptr_out = nullptr;
    return false;
  }

}  // namespace

V_integer
std_chrono_now()
  {
    ::timespec ts;
    ::clock_gettime(CLOCK_REALTIME, &ts);
    return (int64_t) ts.tv_sec * 1000 + (uint32_t) ts.tv_nsec / 1000000;
  }

V_string
std_chrono_format(V_integer time_point, optV_boolean with_ms, optV_integer utc_offset)
  {
    uint32_t year = 0, mon = 0, day = 0, hour = 0, min = 0, sec = 0, ms = 0;
    int gmtoff_sign = 0;
    uint32_t gmtoff_abs = 0;

    if(time_point < 0) {
      // min
      year = 0;
      mon = 1;
      day = 1;
    }
    else if(time_point >= 253370764800000) {
      // max
      year = 9999;
      mon = 1;
      day = 1;
    }
    else {
      // Break the time point down.
      ::tm tr;

      if(!utc_offset) {
        // Obtain GMT offset from the system.
        ::time_t tp = (::time_t) ((uint64_t) time_point / 1000);
        ::localtime_r(&tp, &tr);
        gmtoff_sign = (int) (tr.tm_gmtoff >> 31 | 1);
        gmtoff_abs = (uint32_t) ::std::abs(tr.tm_gmtoff) / 60;
      }
      else {
        // Use the given GMT offset.
        if((*utc_offset <= -1440) || (*utc_offset >= +1440))
          ASTERIA_THROW_RUNTIME_ERROR((
              "UTC time offset out of range (`$1` exceeds 1440 minutes)"),
              utc_offset);

        ::time_t tp = (::time_t) ((uint64_t) time_point / 1000) + (::time_t) *utc_offset * 60;
        ::gmtime_r(&tp, &tr);
        gmtoff_sign = (int) (*utc_offset >> 63 | 1);
        gmtoff_abs = (uint32_t) ::std::abs(*utc_offset);
      }

      year = (uint32_t) tr.tm_year + 1900;
      mon = (uint32_t) tr.tm_mon + 1;
      day = (uint32_t) tr.tm_mday;
      hour = (uint32_t) tr.tm_hour;
      min = (uint32_t) tr.tm_min;
      sec = (uint32_t) tr.tm_sec;
      ms = (uint32_t) ((uint64_t) time_point % 1000);
    }

    // Make the string from decomposed parts.
    char time_str[64];
    char* wptr = time_str;

    do_mempcpy(wptr, s_2digit[year / 100], 2);
    do_mempcpy(wptr, s_2digit[year % 100], 2);
    do_mempcpy(wptr, "-", 1);
    do_mempcpy(wptr, s_2digit[mon], 2);
    do_mempcpy(wptr, "-", 1);
    do_mempcpy(wptr, s_2digit[day], 2);
    do_mempcpy(wptr, " ", 1);
    do_mempcpy(wptr, s_2digit[hour], 2);
    do_mempcpy(wptr, ":", 1);
    do_mempcpy(wptr, s_2digit[min], 2);
    do_mempcpy(wptr, ":", 1);
    do_mempcpy(wptr, s_2digit[sec], 2);

    if(with_ms == true) {
      // Output four digits, then overwrite the first one with
      // a decimal point.
      do_mempcpy(wptr, s_2digit[ms / 100], 2);
      wptr[-2] = '.';
      do_mempcpy(wptr, s_2digit[ms % 100], 2);
    }

    if(gmtoff_abs == 0) {
      // UTC (or conventionally, GMT)
      do_mempcpy(wptr, " UTC", 4);
    }
    else {
      // numeric
      do_mempcpy(wptr, " +\0 -" + (gmtoff_sign >> 1 & 3), 2);
      do_mempcpy(wptr, s_2digit[gmtoff_abs / 60], 2);
      do_mempcpy(wptr, s_2digit[gmtoff_abs % 60], 2);
    }

    return cow_string(time_str, wptr);
  }

V_integer
std_chrono_parse(V_string time_str)
  {
    uint32_t year = 0, mon = 0, day = 0, hour = 0, min = 0, sec = 0, ms = 0;
    int gmtoff_sign = 0;
    uint32_t gmtoff_abs = 0;

    // Collect date/time parts.
    const char* rptr = time_str.c_str();
    const char* eptr = rptr + time_str.length();

    while(*rptr == ' ')
      rptr ++;

    do_match(rptr, year, s_2digit, 2);
    year *= 100;
    do_match(rptr, year, s_2digit, 2);
    do_match(rptr, "-", 1);
    do_match(rptr, mon, s_2digit, 2);
    do_match(rptr, "-", 1);
    do_match(rptr, day, s_2digit, 2);
    do_match(rptr, " ", 1);
    do_match(rptr, hour, s_2digit, 2);
    do_match(rptr, ":", 1);
    do_match(rptr, min, s_2digit, 2);
    do_match(rptr, ":", 1);
    do_match(rptr, sec, s_2digit, 2);

    if(rptr && (*rptr == '.')) {
      rptr ++;

      uint32_t weight = 100;
      while((*rptr >= '0') && (*rptr <= '9')) {
        ms += (uint8_t) (*rptr - '0') * weight;
        rptr ++;
        weight /= 10;
      }
    }

    if(rptr && (*rptr == ' ')) {
      rptr ++;

      if((eptr - rptr >= 3) && ((::memcmp(rptr, "GMT", 3) == 0) || (::memcmp(rptr, "UTC", 3) == 0))) {
        // UTC
        rptr += 3;
        gmtoff_sign = 1;
      }
      else if((eptr - rptr >= 1) && ((*rptr == '+') || (*rptr == '-'))) {
        // Use the given GMT offset.
        gmtoff_sign = -(uint8_t) (*rptr - '+') >> 8 | 1;
        rptr += 1;

        do_match(rptr, gmtoff_abs, s_2digit, 2);
        gmtoff_abs *= 60;
        do_match(rptr, gmtoff_abs, s_2digit, 2);
      }
    }

    while(rptr && (*rptr == ' '))
      rptr ++;

    if(rptr != eptr)
      ASTERIA_THROW_RUNTIME_ERROR((
          "Invalid date/time string `$1`"),
          time_str);

    // Assembly parts.
    int64_t time_point = ms;

    ::tm tr;
    tr.tm_year = (int) year - 1900;
    tr.tm_mon = (int) mon - 1;
    tr.tm_mday = (int) day;
    tr.tm_hour = (int) hour;
    tr.tm_min = (int) min;
    tr.tm_sec = (int) sec;

    if(gmtoff_sign == 0) {
      // Obtain GMT offset from the system.
      tr.tm_isdst = -1;
      time_point += (int64_t) ::mktime(&tr) * 1000;
    }
    else {
      // Use the given GMT offset.
      tr.tm_isdst = 0;
      time_point += (int64_t) ::timegm(&tr) * 1000;
      time_point -= gmtoff_sign * (int32_t) gmtoff_abs * 60000;
    }

    if(time_point < 0)
      return INT64_MIN;
    else if(time_point >= 253370764800000)
      return INT64_MAX;
    else
      return time_point;
  }

V_real
std_chrono_hires_now()
  {
    ::timespec ts;
    ::clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double) ts.tv_sec * 1000 + (double) ts.tv_nsec / 1000000 + 123456789;
  }

V_integer
std_chrono_steady_now()
  {
    ::timespec ts;
    ::clock_gettime(CLOCK_MONOTONIC_COARSE, &ts);
    return (int64_t) ts.tv_sec * 1000 + (uint32_t) ts.tv_nsec / 1000000 + 987654321;
  }

void
create_bindings_chrono(V_object& result, API_Version /*version*/)
  {
    result.insert_or_assign(sref("now"),
      ASTERIA_BINDING(
        "std.chrono.now", "",
        Argument_Reader&& reader)
      {
        reader.start_overload();
        if(reader.end_overload())
          return (Value) std_chrono_now();

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("format"),
      ASTERIA_BINDING(
        "std.chrono.format", "time_point, [with_ms], [utc_offset]",
        Argument_Reader&& reader)
      {
        V_integer time_point;
        optV_boolean with_ms;
        optV_integer utc_offset;

        reader.start_overload();
        reader.required(time_point);
        reader.optional(with_ms);
        reader.optional(utc_offset);
        if(reader.end_overload())
          return (Value) std_chrono_format(time_point, with_ms, utc_offset);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("parse"),
      ASTERIA_BINDING(
        "std.chrono.parse", "time_str",
        Argument_Reader&& reader)
      {
        V_string time_str;

        reader.start_overload();
        reader.required(time_str);
        if(reader.end_overload())
          return (Value) std_chrono_parse(time_str);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("hires_now"),
      ASTERIA_BINDING(
        "std.chrono.hires_now", "",
        Argument_Reader&& reader)
      {
        reader.start_overload();
        if(reader.end_overload())
          return (Value) std_chrono_hires_now();

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("steady_now"),
      ASTERIA_BINDING(
        "std.chrono.steady_now", "",
        Argument_Reader&& reader)
      {
        reader.start_overload();
        if(reader.end_overload())
          return (Value) std_chrono_steady_now();

        reader.throw_no_matching_function_call();
      });
  }

}  // namespace asteria
