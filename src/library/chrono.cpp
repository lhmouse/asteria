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

constexpr int64_t s_timestamp_min = -11644473600'000;
constexpr int64_t s_timestamp_max = 253370764800'000;

constexpr char s_strings_min[][24] = { "1601-01-01 00:00:00", "1601-01-01 00:00:00.000" };
constexpr char s_strings_max[][24] = { "9999-01-01 00:00:00", "9999-01-01 00:00:00.000" };

constexpr int64_t s_timestamp_1600_03_01 = -11670912000'000;
constexpr uint8_t s_month_days[] = { 31,30,31,30,31,31,30,31,30,31,31,29 };  // from March
constexpr char s_spaces[] = " \f\n\r\t\v";

constexpr char s_nums_00_99[100][2] =
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
do_put_00_99(cow_string::iterator wpos, uint64_t value)
  {
    ROCKET_ASSERT(value <= 99);
    for(long k = 0;  k != 2;  ++k)
      wpos[k] = s_nums_00_99[value][k];
  }

inline
bool
do_get_integer(cow_string::const_iterator& rpos, cow_string::const_iterator epos,
               uint64_t& value, uint64_t lim_lo, uint64_t lim_hi)
  {
    value = 0;
    ptrdiff_t ndigits = 0;

    // Parse an integer. Leading whitespace is ignored.
    while(rpos != epos) {
      uint32_t ch = *rpos & 0xFF;
      if((ch == ' ') && (ndigits == 0)) {
        ++rpos;
        continue;
      }

      // Check for a digit.
      uint32_t dval = ch - '0';
      if(dval > 9)
        break;

      value = value * 10 + dval;
      if(value > lim_hi)
        return false;

      ++ndigits;
      ++rpos;
    }
    if(ndigits == 0)
      return false;

    return value >= lim_lo;
  }

inline
bool
do_get_separator(cow_string::const_iterator& rpos, cow_string::const_iterator epos,
                 ::std::initializer_list<uint8_t> seps)
  {
    ptrdiff_t ndigits = 0;

    // Look for one of the separators. Leading whitespace is ignored.
    while(rpos != epos) {
      uint32_t ch = *rpos & 0xFF;
      if(::rocket::is_any_of(ch, seps)) {
        ++rpos;
        return true;
      }

      if((ch != ' ') || (ndigits != 0))
        break;

      ++ndigits;
      ++rpos;
    }
    return false;
  }

}  // namespace

V_integer
std_chrono_utc_now()
  {
    // Get UTC time from the system.
    ::timespec ts;
    ::clock_gettime(CLOCK_REALTIME, &ts);

    // We return the time in milliseconds rather than seconds.
    int64_t secs = static_cast<int64_t>(ts.tv_sec);
    int64_t msecs = static_cast<int64_t>(ts.tv_nsec / 1000'000);
    return secs * 1000 + msecs;
  }

V_integer
std_chrono_local_now()
  {
    // Get local time and GMT offset from the system.
    ::timespec ts;
    ::clock_gettime(CLOCK_REALTIME, &ts);
    ::tm tr;
    ::localtime_r(&(ts.tv_sec), &tr);

    // We return the time in milliseconds rather than seconds.
    int64_t secs = static_cast<int64_t>(ts.tv_sec) + tr.tm_gmtoff;
    int64_t msecs = static_cast<int64_t>(ts.tv_nsec / 1000'000);
    return secs * 1000 + msecs;
  }

V_real
std_chrono_hires_now()
  {
    // Get the time since the system was started.
    ::timespec ts;
    ::clock_gettime(CLOCK_MONOTONIC, &ts);

    // We return the time in milliseconds rather than seconds.
    // Add a random offset to the result to help debugging.
    double secs = static_cast<double>(ts.tv_sec);
    double msecs = static_cast<double>(ts.tv_nsec) / 1000'000.0;
    return secs * 1000 + msecs + 1234567890123;
  }

V_integer
std_chrono_steady_now()
  {
    // Get the time since the system was started.
    ::timespec ts;
    ::clock_gettime(CLOCK_MONOTONIC_COARSE, &ts);

    // We return the time in milliseconds rather than seconds.
    // Add a random offset to the result to help debugging.
    int64_t secs = static_cast<int64_t>(ts.tv_sec);
    int64_t msecs = static_cast<int64_t>(ts.tv_nsec / 1000'000);
    return secs * 1000 + msecs + 3210987654321;
  }

V_integer
std_chrono_local_from_utc(V_integer time_utc)
  {
    // Handle special time values.
    if(time_utc <= s_timestamp_min)
      return INT64_MIN;

    if(time_utc >= s_timestamp_max)
      return INT64_MAX;

    // Calculate the local time.
    ::time_t tp = 0;
    ::tm tr;
    ::localtime_r(&tp, &tr);
    int64_t time_local = time_utc + tr.tm_gmtoff * 1000;

    // Ensure the value is within the range of finite values.
    if(time_local <= s_timestamp_min)
      return INT64_MIN;

    if(time_local >= s_timestamp_max)
      return INT64_MAX;

    return time_local;
  }

V_integer
std_chrono_utc_from_local(V_integer time_local)
  {
    // Handle special time values.
    if(time_local <= s_timestamp_min)
      return INT64_MIN;

    if(time_local >= s_timestamp_max)
      return INT64_MAX;

    // Calculate the local time.
    ::time_t tp = 0;
    ::tm tr;
    ::localtime_r(&tp, &tr);
    int64_t time_utc = time_local - tr.tm_gmtoff * 1000;

    // Ensure the value is within the range of finite values.
    if(time_utc <= s_timestamp_min)
      return INT64_MIN;

    if(time_utc >= s_timestamp_max)
      return INT64_MAX;

    return time_utc;
  }

V_string
std_chrono_format(V_integer time_point, optV_boolean with_ms)
  {
    // No millisecond part is added by default.
    bool pms = with_ms.value_or(false);
    // Handle special time points.
    if(time_point <= s_timestamp_min)
      return sref(s_strings_min[pms]);

    if(time_point >= s_timestamp_max)
      return sref(s_strings_max[pms]);

    // Convert the timestamp to the number of milliseconds since 1600-03-01.
    uint64_t temp = static_cast<uint64_t>(time_point - s_timestamp_1600_03_01);

    // Get subday parts.
    uint64_t msec = temp % 1000;
    temp /= 1000;
    uint64_t sec = temp % 60;
    temp /= 60;
    uint64_t min = temp % 60;
    temp /= 60;
    uint64_t hour = temp % 24;
    temp /= 24;

    // There are 146097 days in every 400 years.
    uint64_t y400 = temp / 146097;
    temp %= 146097;
    // There are 36524 days in every 100 years.
    uint64_t y100 = temp / 36524;
    temp %= 36524;
    if(y100 == 4) {  // leap
      y100 = 3;
      temp = 36524;
    }
    // There are 1461 days in every 4 years.
    uint64_t y4 = temp / 1461;
    temp %= 1461;

    // There are 365 days in every year.
    // Note we count from 03-01. The extra day of a leap year will be appended to the end.
    uint64_t year = temp / 365;
    temp %= 365;
    if(year == 4) {  // leap
      year = 3;
      temp = 365;
    }
    // Sum them up to get the number of years.
    year += y400 * 400;
    year += y100 * 100;
    year += y4   *   4;
    year += 1600;

    // Calculate the shifted month index, which counts from March.
    uint32_t mday_max;
    size_t mon_sh = 0;
    while(temp >= (mday_max = s_month_days[mon_sh])) {
      temp -= mday_max;
      mon_sh += 1;
    }
    uint64_t month = (mon_sh + 2) % 12 + 1;
    // Note our 'years' start from March.
    if(month < 3) {
      year += 1;
    }
    if(year > 9999) {
      year = 9999;
    }
    // `temp` now contains the number of days in the last month.
    uint64_t mday = temp + 1;

    // Format it now.
    cow_string time_str;
    auto wpos = time_str.insert(time_str.begin(), s_strings_max[pms]);

    do_put_00_99(wpos +  0, year / 100);
    do_put_00_99(wpos +  2, year % 100);
    do_put_00_99(wpos +  5, month);
    do_put_00_99(wpos +  8, mday);
    do_put_00_99(wpos + 11, hour);
    do_put_00_99(wpos + 14, min);
    do_put_00_99(wpos + 17, sec);

    if(pms) {
      do_put_00_99(wpos + 20, msec / 10);
      wpos[22] = static_cast<char>('0' + msec % 10);
    }
    return time_str;
  }

V_integer
std_chrono_utc_parse(V_string time_str)
  {
    // Trim leading and trailing spaces. Fail if the string becomes empty.
    size_t off = time_str.find_first_not_of(s_spaces);
    if(off == V_string::npos)
      ASTERIA_THROW_RUNTIME_ERROR((
          "Blank time string"));

    // Get the start and end of the non-empty sequence.
    auto rpos = time_str.begin() + static_cast<ptrdiff_t>(off);
    off = time_str.find_last_not_of(s_spaces) + 1;
    const auto epos = time_str.begin() + static_cast<ptrdiff_t>(off);

    // Parse individual parts.
    uint64_t year = 0;
    uint64_t month = 0;
    uint64_t mday = 0;
    uint64_t hour = 0;
    uint64_t min = 0;
    uint64_t sec = 0;
    uint64_t msec = 0;

    // Parse the year.
    if(!do_get_integer(rpos, epos, year, 0, 9999))
      ASTERIA_THROW_RUNTIME_ERROR((
          "Invalid date-time string (expecting year in `$1`)"),
          time_str);

    if(!do_get_separator(rpos, epos, { '-', '/' }))
      ASTERIA_THROW_RUNTIME_ERROR((
          "Invalid date-time string (expecting year-month separator in `$1`)"),
          time_str);

    // Parse the month.
    if(!do_get_integer(rpos, epos, month, 1, 12))
      ASTERIA_THROW_RUNTIME_ERROR((
          "Invalid date-time string (expecting month in `$1`)"),
          time_str);

    if(!do_get_separator(rpos, epos, { '-', '/' }))
      ASTERIA_THROW_RUNTIME_ERROR((
          "Invalid date-time string (expecting month-day separator in `$1`)"),
          time_str);

    // Get the maximum value of the day of month.
    uint32_t mday_max;
    size_t mon_sh = static_cast<size_t>(month + 9) % 12;
    if(mon_sh != 11) {
      mday_max = s_month_days[mon_sh];
    }
    else {
      mday_max = 28;
      if((year % 100 == 0) ? (year % 400 == 0) : (year % 4 == 0))
        mday_max += 1;
    }
    // Parse the day of month as at most two digits.
    if(!do_get_integer(rpos, epos, mday, 1, mday_max))
      ASTERIA_THROW_RUNTIME_ERROR((
          "Invalid date-time string (expecting day of month in `$1`)"),
          time_str);

    // Parse the day-hour separator, which may be a space or the letter `T`.
    // The subday part is optional.
    if(do_get_separator(rpos, epos, { ' ', '\t', 'T' })) {
      // Parse the number of hours.
      if(!do_get_integer(rpos, epos, hour, 0, 23))
        ASTERIA_THROW_RUNTIME_ERROR((
            "Invalid date-time string (expecting hours in `$1`)"),
            time_str);

      if(!do_get_separator(rpos, epos, { ':' }))
        ASTERIA_THROW_RUNTIME_ERROR((
            "Invalid date-time string (expecting hour-minute separator in `$1`)"),
            time_str);

      // Parse the number of minutes.
      if(!do_get_integer(rpos, epos, min, 0, 59))
        ASTERIA_THROW_RUNTIME_ERROR((
            "Invalid date-time string (expecting minutes in `$1`)"),
            time_str);

      if(!do_get_separator(rpos, epos, { ':' }))
        ASTERIA_THROW_RUNTIME_ERROR((
            "Invalid date-time string (expecting minute-second separator in `$1`)"),
            time_str);

      // Parse the number of seconds.
      // Note leap seconds.
      if(!do_get_integer(rpos, epos, sec, 0, 60))
        ASTERIA_THROW_RUNTIME_ERROR((
            "Invalid date-time string (expecting seconds in `$1`)"),
            time_str);

      // Parse the second-subsecond separator, which may be a point or a comma.
      // The subsecond part is optional.
      if(do_get_separator(rpos, epos, { '.', ',' })) {
        // Parse at most three digits. Excess digits are ignored.
        uint32_t weight = 100;

        // Parse an integer. Leading whitespace is ignored.
        while(rpos != epos) {
          uint32_t ch = *rpos & 0xFF;
          if((ch == ' ') && (weight == 100)) {
            ++rpos;
            continue;
          }

          // Check for a digit.
          uint64_t dval = ch - '0';
          if(dval > 9)
            ASTERIA_THROW_RUNTIME_ERROR((
                "Invalid date-time string (invalid subsecond digit in `$1`)"),
                time_str);

          msec += dval * weight;
          weight /= 10;
          ++rpos;
        }
      }
    }

    // Ensure all characters have been consumed.
    if(rpos != epos)
      ASTERIA_THROW_RUNTIME_ERROR((
          "Invalid date-time string (excess characters in `$1`)"),
          time_str);

    // Handle special time values.
    if(year <= 1600)
      return INT64_MIN;

    if(year >= 9999)
      return INT64_MAX;

    // Calculate the number of days since 1600-03-01.
    // Note our 'years' start from March.
    if(month < 3)
      year -= 1;

    // There are 146097 days in every 400 years.
    uint64_t temp = (year - 1600) / 400 * 146097;
    year %= 400;

    // There are 36524 days in every 100 years.
    temp += year / 100 * 36524;
    year %= 100;

    // There are 1461 days in every 4 years.
    temp += year / 4 * 1461;
    year %= 4;

    // There are 365 days in every year.
    // Note we count from 03-01. The extra day of a leap year will be appended to the end.
    temp += year * 365;

    // Accumulate months.
    while(mon_sh != 0) {
      mon_sh -= 1;
      temp += s_month_days[mon_sh];
    }
    // Accumulate days in the last month.
    temp += mday - 1;

    // Convert all parts into milliseconds and sum them up.
    temp *= 24;
    temp += hour;
    temp *= 60;
    temp += min;
    temp *= 60;
    temp += sec;
    temp *= 1000;
    temp += msec;

    // Shift the timestamp back.
    return static_cast<int64_t>(temp) + s_timestamp_1600_03_01;
  }

void
create_bindings_chrono(V_object& result, API_Version /*version*/)
  {
    result.insert_or_assign(sref("utc_now"),
      ASTERIA_BINDING(
        "std.chrono.utc_now", "",
        Argument_Reader&& reader)
      {
        reader.start_overload();
        if(reader.end_overload())
          return (Value) std_chrono_utc_now();

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("local_now"),
      ASTERIA_BINDING(
        "std.chrono.local_now", "",
        Argument_Reader&& reader)
      {
        reader.start_overload();
        if(reader.end_overload())
          return (Value) std_chrono_local_now();

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

    result.insert_or_assign(sref("local_from_utc"),
      ASTERIA_BINDING(
        "std.chrono.local_from_utc", "time_utc",
        Argument_Reader&& reader)
      {
        V_integer time_utc;

        reader.start_overload();
        reader.required(time_utc);
        if(reader.end_overload())
          return (Value) std_chrono_local_from_utc(time_utc);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("utc_from_local"),
      ASTERIA_BINDING(
        "std.chrono.utc_from_local", "time_local",
        Argument_Reader&& reader)
      {
        V_integer time_local;

        reader.start_overload();
        reader.required(time_local);
        if(reader.end_overload())
          return (Value) std_chrono_utc_from_local(time_local);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("format"),
      ASTERIA_BINDING(
        "std.chrono.format", "time_point, [with_ms]",
        Argument_Reader&& reader)
      {
        V_integer time_point;
        optV_boolean with_ms;

        reader.start_overload();
        reader.required(time_point);
        reader.optional(with_ms);
        if(reader.end_overload())
          return (Value) std_chrono_format(time_point, with_ms);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("utc_parse"),
      ASTERIA_BINDING(
        "std.chrono.utc_parse", "time_str",
        Argument_Reader&& reader)
      {
        V_string time_str;

        reader.start_overload();
        reader.required(time_str);
        if(reader.end_overload())
          return (Value) std_chrono_utc_parse(time_str);

        reader.throw_no_matching_function_call();
      });
  }

}  // namespace asteria
