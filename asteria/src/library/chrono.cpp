// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "chrono.hpp"
#include "../runtime/argument_reader.hpp"
#include "../utilities.hpp"
#include <time.h>  // ::clock_gettime(), ::timespec

namespace asteria {
namespace {

constexpr int64_t s_timestamp_min = -11644473600'000;
constexpr int64_t s_timestamp_max = 253370764800'000;

constexpr char s_strings_min[][24] = { "1601-01-01 00:00:00", "1601-01-01 00:00:00.000" };
constexpr char s_strings_max[][24] = { "9999-01-01 00:00:00", "9999-01-01 00:00:00.000" };

constexpr int64_t s_timestamp_1600_03_01 = -11670912000'000;
constexpr uint8_t s_month_days[] = { 31, 30, 31, 30, 31, 31, 30, 31, 30, 31, 31, 29 };  // from March

constexpr char s_spaces[] = " \f\n\r\t\v";

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
std_chrono_utc_format(V_integer time_point, optV_boolean with_ms)
  {
    // No millisecond part is added by default.
    bool pms = with_ms.value_or(false);
    // Handle special time points.
    if(time_point <= s_timestamp_min)
      return ::rocket::sref(s_strings_min[pms]);

    if(time_point >= s_timestamp_max)
      return ::rocket::sref(s_strings_max[pms]);

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
    uint8_t mday_max;
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
    // `temp` now contains the number of days in the last month.
    uint64_t mday = temp + 1;

    // Pack these parts into a BCD integer which looks like `20200215041314789`.
    temp = year;
    temp *= 100;
    temp += month;
    temp *= 100;
    temp += mday;
    temp *= 100;
    temp += hour;
    temp *= 100;
    temp += min;
    temp *= 100;
    temp += sec;
    temp *= 1000;
    temp += msec;

    // Format it now.
    ::rocket::ascii_numput nump;
    const char* bp = nump.put_DU(temp).data();

    // Copy individual parts out.
    char sbuf[sizeof(s_strings_max[0])];
    char* wp = sbuf;

    // `yyyy`
    *(wp++) = *(bp++);
    *(wp++) = *(bp++);
    *(wp++) = *(bp++);
    *(wp++) = *(bp++);
    // `-mm`
    *(wp++) = '-';
    *(wp++) = *(bp++);
    *(wp++) = *(bp++);
    // `-dd`
    *(wp++) = '-';
    *(wp++) = *(bp++);
    *(wp++) = *(bp++);
    // ` HH`
    *(wp++) = ' ';
    *(wp++) = *(bp++);
    *(wp++) = *(bp++);
    // `:MM`
    *(wp++) = ':';
    *(wp++) = *(bp++);
    *(wp++) = *(bp++);
    // `:SS`
    *(wp++) = ':';
    *(wp++) = *(bp++);
    *(wp++) = *(bp++);
    // `.sss`
    if(pms) {
      *(wp++) = '.';
      *(wp++) = *(bp++);
      *(wp++) = *(bp++);
      *(wp++) = *(bp++);
    }
    return cow_string(sbuf, wp);
  }

V_integer
std_chrono_utc_parse(V_string time_str)
  {
    // Trim leading and trailing spaces. Fail if the string becomes empty.
    size_t off = time_str.find_first_not_of(s_spaces);
    if(off == V_string::npos)
      ASTERIA_THROW("Blank time string");

    // Get the start and end of the non-empty sequence.
    const char* bp = time_str.data() + off;
    off = time_str.find_last_not_of(s_spaces) + 1;
    const char* ep = time_str.data() + off;

    // Parse individual parts.
    ::rocket::ascii_numget numg;
    uint64_t year = 0;
    uint64_t month = 0;
    uint64_t mday = 0;
    uint64_t hour = 0;
    uint64_t min = 0;
    uint64_t sec = 0;
    uint64_t msec = 0;

    // Parse the year as at most four digits.
    if(!numg.parse_U(bp, ep, 10) || !numg.cast_U(year, 0, 9999))
      ASTERIA_THROW("Invalid date-time string (expecting year in `$1`)", time_str);

    // Parse the year-month separator, which may be a dash or slash.
    if((bp == ep) || !::rocket::is_any_of(*(bp++), { '-', '/' }))
      ASTERIA_THROW("Invalid date-time string (expecting year-month separator in `$1`)", time_str);

    // Parse the month as at most two digits.
    if(!numg.parse_U(bp, ep, 10) || !numg.cast_U(month, 1, 12))
      ASTERIA_THROW("Invalid date-time string (expecting month in `$1`)", time_str);

    // Parse the month-day separator, which may be a dash or slash.
    if((bp == ep) || !::rocket::is_any_of(*(bp++), { '-', '/' }))
      ASTERIA_THROW("Invalid date-time string (expecting month-day separator in `$1`)", time_str);

    // Get the maximum value of the day of month.
    uint8_t mday_max;
    size_t mon_sh = static_cast<size_t>(month + 9) % 12;
    if(mon_sh != 11)
      mday_max = s_month_days[mon_sh];
    else if((year % 100 == 0) ? (year % 400 == 0) : (year % 4 == 0))
      mday_max = 29;
    else
      mday_max = 28;
    // Parse the day of month as at most two digits.
    if(!numg.parse_U(bp, ep, 10) || !numg.cast_U(mday, 1, mday_max))
      ASTERIA_THROW("Invalid date-time string (expecting day of month in `$1`)", time_str);

    // The subday part is optional.
    if(bp != ep) {
      // Parse the day-hour separator, which may be a space or the letter `T`.
      if(!::rocket::is_any_of(*(bp++), { ' ', '\t', 'T' }))
        ASTERIA_THROW("Invalid date-time string (expecting day-hour separator in `$1`)", time_str);

      // Parse the number of hours as at most two digits.
      if(!numg.parse_U(bp, ep, 10) || !numg.cast_U(hour, 0, 59))
        ASTERIA_THROW("Invalid date-time string (expecting hours in `$1`)", time_str);

      // Parse the hour-minute separator, which shall by a colon.
      if(!::rocket::is_any_of(*(bp++), { ':' }))
        ASTERIA_THROW("Invalid date-time string (expecting hour-minute separator in `$1`)", time_str);

      // Parse the number of minutes as at most two digits.
      if(!numg.parse_U(bp, ep, 10) || !numg.cast_U(min, 0, 59))
        ASTERIA_THROW("Invalid date-time string (expecting minutes in `$1`)", time_str);

      // Parse the minute-second separator, which shall by a colon.
      if(!::rocket::is_any_of(*(bp++), { ':' }))
        ASTERIA_THROW("Invalid date-time string (expecting minute-second separator in `$1`)", time_str);

      // Parse the number of seconds as at most two digits. Note leap seconds.
      if(!numg.parse_U(bp, ep, 10) || !numg.cast_U(sec, 0, 60))
        ASTERIA_THROW("Invalid date-time string (expecting seconds in `$1`)", time_str);

      // The subsecond part is optional.
      if(bp != ep) {
        // Parse the second-subsecond separator, which shall by a dot.
        if(!::rocket::is_any_of(*(bp++), { '.' }))
          ASTERIA_THROW("Invalid date-time string (expecting second-subsecond separator in `$1`)", time_str);

        // Parse at most three digits. Excess digits are ignored.
        uintptr_t weight = 100;
        while(bp != ep) {
          uint32_t dval = static_cast<uint32_t>(*(bp++) - '0');
          if(dval > 9)
            ASTERIA_THROW("Invalid date-time string (invalid subsecond digit in `$1`)", time_str);
          msec += dval * weight;
          weight /= 10;
        }
        if(weight == 100)
          ASTERIA_THROW("Invalid date-time string (no digits after decimal point in `$1`)", time_str);
      }
    }

    // Ensure all characters have been consumed.
    if(bp != ep)
      ASTERIA_THROW("Invalid date-time string (excess characters in `$1`)", time_str);

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
    //===================================================================
    // `std.chrono.utc_now()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("utc_now"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.chrono.utc_now()`

  * Retrieves the wall clock time in UTC.

  * Returns the number of milliseconds since the Unix epoch,
    represented as an integer.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.chrono.utc_now"));
    // Parse arguments.
    if(reader.I().F()) {
      Reference_root::S_temporary xref = { std_chrono_utc_now() };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.chrono.local_now()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("local_now"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.chrono.local_now()`

  * Retrieves the wall clock time in the local time zone.

  * Returns the number of milliseconds since `1970-01-01 00:00:00`
    in the local time zone, represented as an integer.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.chrono.local_now"));
    // Parse arguments.
    if(reader.I().F()) {
      Reference_root::S_temporary xref = { std_chrono_local_now() };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.chrono.hires_now()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("hires_now"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.chrono.hires_now()`

  * Retrieves a time point from a high resolution clock. The clock
    goes monotonically and cannot be adjusted, being suitable for
    time measurement. This function provides accuracy and might be
    quite heavyweight.

  * Returns the number of milliseconds since an unspecified time
    point, represented as a real.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.chrono.hires_now"));
    // Parse arguments.
    if(reader.I().F()) {
      Reference_root::S_temporary xref = { std_chrono_hires_now() };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.chrono.steady_now()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("steady_now"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.chrono.steady_now()`

  * Retrieves a time point from a steady clock. The clock goes
    monotonically and cannot be adjusted, being suitable for time
    measurement. This function is supposed to be fast and might
    have poor accuracy.

  * Returns the number of milliseconds since an unspecified time
    point, represented as an integer.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.chrono.steady_now"));
    // Parse arguments.
    if(reader.I().F()) {
      Reference_root::S_temporary xref = { std_chrono_steady_now() };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.chrono.local_from_utc()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("local_from_utc"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.chrono.local_from_utc(time_utc)`

  * Converts a UTC time point to a local one. `time_utc` shall be
    the number of milliseconds since the Unix epoch.

  * Returns the number of milliseconds since `1970-01-01 00:00:00`
    in the local time zone, represented as an integer.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.chrono.local_from_utc"));
    // Parse arguments.
    V_integer time_utc;
    if(reader.I().v(time_utc).F()) {
      Reference_root::S_temporary xref = { std_chrono_local_from_utc(::std::move(time_utc)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.chrono.utc_from_local()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("utc_from_local"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.chrono.utc_from_local(time_local)`

  * Converts a local time point to a UTC one. `time_local` shall
    be the number of milliseconds since `1970-01-01 00:00:00` in
    the local time zone.

  * Returns the number of milliseconds since the Unix epoch,
    represented as an integer.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.chrono.utc_from_local"));
    // Parse arguments.
    V_integer time_local;
    if(reader.I().v(time_local).F()) {
      Reference_root::S_temporary xref = { std_chrono_utc_from_local(::std::move(time_local)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.chrono.utc_format()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("utc_format"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.chrono.utc_format(time_point, [with_ms])`

  * Converts `time_point`, which represents the number of
    milliseconds since `1970-01-01 00:00:00`, to an ASCII string in
    the aforementioned format, according to the ISO 8601 standard.
    If `with_ms` is set to `true`, the string will have a 3-digit
    fractional part. By default, no fractional part is added.

  * Returns a string representing the time point.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.chrono.utc_format"));
    // Parse arguments.
    V_integer time_point;
    optV_boolean with_ms;
    if(reader.I().v(time_point).o(with_ms).F()) {
      Reference_root::S_temporary xref = { std_chrono_utc_format(::std::move(time_point), with_ms) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));

    //===================================================================
    // `std.chrono.utc_parse()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("utc_parse"),
      V_function(
"""""""""""""""""""""""""""""""""""""""""""""""" R"'''''''''''''''(
`std.chrono.utc_parse(time_str)`

  * Parses `time_str`, which is an ASCII string representing a time
    point in the format `1970-01-01 00:00:00.000`, according to the
    ISO 8601 standard. The subsecond part is optional and may have
    fewer or more digits. There may be leading or trailing spaces.

  * Returns the number of milliseconds since `1970-01-01 00:00:00`.

  * Throws an exception if the string is invalid.
)'''''''''''''''" """""""""""""""""""""""""""""""""""""""""""""""",
*[](Reference& self, cow_vector<Reference>&& args, Global_Context& /*global*/) -> Reference&
  {
    Argument_Reader reader(::rocket::cref(args), ::rocket::sref("std.chrono.utc_parse"));
    // Parse arguments.
    V_string time_str;
    if(reader.I().v(time_str).F()) {
      Reference_root::S_temporary xref = { std_chrono_utc_parse(::std::move(time_str)) };
      return self = ::std::move(xref);
    }
    // Fail.
    reader.throw_no_matching_function_call();
  }
      ));
  }

}  // namespace asteria
