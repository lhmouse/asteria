// This file is part of Asteria.
// Copyleft 2018 - 2020, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "bindings_chrono.hpp"
#include "argument_reader.hpp"
#include "simple_binding_wrapper.hpp"
#include "../utilities.hpp"
#include <time.h>

namespace Asteria {
namespace {

constexpr int64_t s_timestamp_min = -11644473600'000;
constexpr int64_t s_timestamp_max = 253370764800'000;

constexpr char s_strings_min[][24] = { "1601-01-01 00:00:00", "1601-01-01 00:00:00.000" };
constexpr char s_strings_max[][24] = { "9999-01-01 00:00:00", "9999-01-01 00:00:00.000" };

constexpr int64_t s_timestamp_1600_03_01 = -11670912000'000;
constexpr uint8_t s_month_days[] = { 31, 30, 31, 30, 31, 31, 30, 31, 30, 31, 31, 29 };  // from March

constexpr char s_spaces[] = " \f\n\r\t\v";

}  // namespace

Ival std_chrono_utc_now()
  {
    // Get UTC time from the system.
    ::timespec ts;
    ::clock_gettime(CLOCK_REALTIME, &ts);
    // We return the time in milliseconds rather than seconds.
    int64_t secs = static_cast<int64_t>(ts.tv_sec);
    int64_t msecs = static_cast<int64_t>(ts.tv_nsec / 1000'000);
    return secs * 1000 + msecs;
  }

Ival std_chrono_local_now()
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

Rval std_chrono_hires_now()
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

Ival std_chrono_steady_now()
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

Ival std_chrono_local_from_utc(Ival time_utc)
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
    else
      return time_local;
  }

Ival std_chrono_utc_from_local(Ival time_local)
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
    else
      return time_utc;
  }

Sval std_chrono_utc_format(Ival time_point, Bopt with_ms)
  {
    // No millisecond part is added by default.
    bool pms = with_ms.value_or(false);
    // Handle special time points.
    if(time_point <= s_timestamp_min)
      return ::rocket::sref(s_strings_min[pms]);
    if(time_point >= s_timestamp_max)
      return ::rocket::sref(s_strings_max[pms]);

    // Split the timepoint into second and millisecond parts.
    double secs = (static_cast<double>(time_point) + 0.01) / 1000;
    double intg = ::std::floor(secs);
    // Note that the number of seconds shall be rounded towards negative infinity.
    ::time_t tp = static_cast<::time_t>(intg);
    ::tm tr;
    ::gmtime_r(&tp, &tr);
    // Combine all parts into a single number.
    ::rocket::ascii_numput nump;
    uint64_t temp = 0;
    // 'yyyy'
    temp += static_cast<uint64_t>(static_cast<unsigned>(tr.tm_year + 1900))
            * 1'00'00'00'00'00'000;
    // 'mm'
    temp += static_cast<uint64_t>(static_cast<unsigned>(tr.tm_mon + 1))
            *    1'00'00'00'00'000;
    // 'dd'
    temp += static_cast<uint64_t>(static_cast<unsigned>(tr.tm_mday))
            *       1'00'00'00'000;
    // 'HH'
    temp += static_cast<uint64_t>(static_cast<unsigned>(tr.tm_hour))
            *          1'00'00'000;
    // 'MM'
    temp += static_cast<uint64_t>(static_cast<unsigned>(tr.tm_min))
            *             1'00'000;
    // 'SS'
    temp += static_cast<uint64_t>(static_cast<unsigned>(tr.tm_sec))
            *                1'000;
    // 'sss'
    temp += static_cast<uint64_t>(static_cast<int64_t>((secs - intg) * 1000));
    // Compose the string.
    const char* p = nump.put_DU(temp, 17).data();
    char tstr[] =
      {
        *(p++), *(p++), *(p++), *(p++), '-', *(p++), *(p++), '-', *(p++), *(p++),  // 'yyyy-mm-dd'  10
        ' ',            *(p++), *(p++), ':', *(p++), *(p++), ':', *(p++), *(p++),  // ' HH:MM:SS'    9
        '.',            *(p++), *(p++), *(p++)                                     // '.sss'         4
      };
    return cow_string(tstr, pms ? 23 : 19);
  }

Ival std_chrono_utc_parse(Sval time_str)
  {
    // Trim leading and trailing spaces. Fail if the string becomes empty.
    size_t off = time_str.find_first_not_of(s_spaces);
    if(off == Sval::npos) {
      ASTERIA_THROW("blank time string");
    }
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
    if(!numg.parse_U(bp, ep, 10) || !numg.cast_U(year, 0, 9999)) {
      ASTERIA_THROW("invalid date-time string (expecting year in `$1`)", time_str);
    }
    // Parse the year-month separator, which may be a dash or slash.
    if((bp == ep) || !::rocket::is_any_of(*(bp++), { '-', '/' })) {
      ASTERIA_THROW("invalid date-time string (expecting year-month separator in `$1`)", time_str);
    }
    // Parse the month as at most two digits.
    if(!numg.parse_U(bp, ep, 10) || !numg.cast_U(month, 1, 12)) {
      ASTERIA_THROW("invalid date-time string (expecting month in `$1`)", time_str);
    }
    // Parse the month-day separator, which may be a dash or slash.
    if((bp == ep) || !::rocket::is_any_of(*(bp++), { '-', '/' })) {
      ASTERIA_THROW("invalid date-time string (expecting month-day separator in `$1`)", time_str);
    }
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
    if(!numg.parse_U(bp, ep, 10) || !numg.cast_U(mday, 1, mday_max)) {
      ASTERIA_THROW("invalid date-time string (expecting day of month in `$1`)", time_str);
    }
    // The subday part is optional.
    if(bp != ep) {
      // Parse the day-hour separator, which may be a space or the letter `T`.
      if(!::rocket::is_any_of(*(bp++), { ' ', '\t', 'T' })) {
        ASTERIA_THROW("invalid date-time string (expecting day-hour separator in `$1`)", time_str);
      }
      // Parse the number of hours as at most two digits.
      if(!numg.parse_U(bp, ep, 10) || !numg.cast_U(hour, 0, 59)) {
        ASTERIA_THROW("invalid date-time string (expecting hours in `$1`)", time_str);
      }
      // Parse the hour-minute separator, which shall by a colon.
      if(!::rocket::is_any_of(*(bp++), { ':' })) {
        ASTERIA_THROW("invalid date-time string (expecting hour-minute separator in `$1`)", time_str);
      }
      // Parse the number of minutes as at most two digits.
      if(!numg.parse_U(bp, ep, 10) || !numg.cast_U(min, 0, 59)) {
        ASTERIA_THROW("invalid date-time string (expecting minutes in `$1`)", time_str);
      }
      // Parse the minute-second separator, which shall by a colon.
      if(!::rocket::is_any_of(*(bp++), { ':' })) {
        ASTERIA_THROW("invalid date-time string (expecting minute-second separator in `$1`)", time_str);
      }
      // Parse the number of seconds as at most two digits. Note leap seconds.
      if(!numg.parse_U(bp, ep, 10) || !numg.cast_U(sec, 0, 60)) {
        ASTERIA_THROW("invalid date-time string (expecting seconds in `$1`)", time_str);
      }
      // The subsecond part is optional.
      if(bp != ep) {
        // Parse the second-subsecond separator, which shall by a dot.
        if(!::rocket::is_any_of(*(bp++), { '.' })) {
          ASTERIA_THROW("invalid date-time string (expecting second-subsecond separator in `$1`)", time_str);
        }
        // Parse at most three digits. Excess digits are ignored.
        uint32_t weight = 100;
        while(bp != ep) {
          uint32_t dval = static_cast<uint32_t>(*(bp++) - '0');
          if(dval > 9)
            ASTERIA_THROW("invalid date-time string (invalid subsecond digit in `$1`)", time_str);
          msec += dval * weight;
          weight /= 10;
        }
        if(weight == 100)
          ASTERIA_THROW("invalid date-time string (no digits after decimal point in `$1`)", time_str);
      }
    }

    // Ensure all characters have been consumed.
    if(bp != ep) {
      ASTERIA_THROW("invalid date-time string (excess characters in `$1`)", time_str);
    }
    // Handle special time values.
    if(year <= 1600)
      return INT64_MIN;
    if(year >= 9999)
      return INT64_MAX;

    // Calculate the number of days since 1600-03-01.
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
    for(size_t i = 0;  i < mon_sh;  ++i)
      temp += s_month_days[i];
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

void create_bindings_chrono(Oval& result, API_Version /*version*/)
  {
    //===================================================================
    // `std.chrono.utc_now()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("utc_now"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.chrono.utc_now()`\n"
          "\n"
          "  * Retrieves the wall clock time in UTC.\n"
          "\n"
          "  * Returns the number of milliseconds since the Unix epoch,\n"
          "    represented as an `integer`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.chrono.utc_now"), ::rocket::ref(args));
          // Parse arguments.
          if(reader.I().F()) {
            // Call the binding function.
            return std_chrono_utc_now();
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.chrono.local_now()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("local_now"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.chrono.local_now()`\n"
          "\n"
          "  * Retrieves the wall clock time in the local time zone.\n"
          "\n"
          "  * Returns the number of milliseconds since `1970-01-01 00:00:00`\n"
          "    in the local time zone, represented as an `integer`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.chrono.local_now"), ::rocket::ref(args));
          // Parse arguments.
          if(reader.I().F()) {
            // Call the binding function.
            return std_chrono_local_now();
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.chrono.hires_now()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("hires_now"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.chrono.hires_now()`\n"
          "\n"
          "  * Retrieves a time point from a high resolution clock. The clock\n"
          "    goes monotonically and cannot be adjusted, being suitable for\n"
          "    time measurement. This function provides accuracy and might be\n"
          "    quite heavyweight.\n"
          "\n"
          "  * Returns the number of milliseconds since an unspecified time\n"
          "    point, represented as a `real`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.chrono.hires_now"), ::rocket::ref(args));
          // Parse arguments.
          if(reader.I().F()) {
            // Call the binding function.
            return std_chrono_hires_now();
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.chrono.steady_now()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("steady_now"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.chrono.steady_now()`\n"
          "\n"
          "  * Retrieves a time point from a steady clock. The clock goes\n"
          "    monotonically and cannot be adjusted, being suitable for time\n"
          "    measurement. This function is supposed to be fast and might\n"
          "    have poor accuracy.\n"
          "\n"
          "  * Returns the number of milliseconds since an unspecified time\n"
          "    point, represented as an `integer`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.chrono.steady_now"), ::rocket::ref(args));
          // Parse arguments.
          if(reader.I().F()) {
            // Call the binding function.
            return std_chrono_steady_now();
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.chrono.local_from_utc()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("local_from_utc"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.chrono.local_from_utc(time_utc)`\n"
          "\n"
          "  * Converts a UTC time point to a local one. `time_utc` shall be\n"
          "    the number of milliseconds since the Unix epoch.\n"
          "\n"
          "  * Returns the number of milliseconds since `1970-01-01 00:00:00`\n"
          "    in the local time zone, represented as an `integer`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.chrono.local_from_utc"), ::rocket::ref(args));
          // Parse arguments.
          Ival time_utc;
          if(reader.I().g(time_utc).F()) {
            // Call the binding function.
            return std_chrono_local_from_utc(::rocket::move(time_utc));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.chrono.utc_from_local()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("utc_from_local"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.chrono.utc_from_local(time_local)`\n"
          "\n"
          "  * Converts a local time point to a UTC one. `time_local` shall\n"
          "    be the number of milliseconds since `1970-01-01 00:00:00` in\n"
          "    the local time zone.\n"
          "\n"
          "  * Returns the number of milliseconds since the Unix epoch,\n"
          "    represented as an `integer`.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.chrono.utc_from_local"), ::rocket::ref(args));
          // Parse arguments.
          Ival time_local;
          if(reader.I().g(time_local).F()) {
            // Call the binding function.
            return std_chrono_utc_from_local(::rocket::move(time_local));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.chrono.utc_format()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("utc_format"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.chrono.utc_format(time_point, [with_ms])`\n"
          "\n"
          "  * Converts `time_point`, which represents the number of\n"
          "    milliseconds since `1970-01-01 00:00:00`, to an ASCII string in\n"
          "    the aforementioned format, according to the ISO 8601 standard.\n"
          "    If `with_ms` is set to `true`, the string will have a 3-digit\n"
          "    fractional part. By default, no fractional part is added.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.chrono.utc_format"), ::rocket::ref(args));
          // Parse arguments.
          Ival time_point;
          Bopt with_ms;
          if(reader.I().g(time_point).g(with_ms).F()) {
            // Call the binding function.
            return std_chrono_utc_format(::rocket::move(time_point), ::rocket::move(with_ms));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.chrono.utc_parse()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("utc_parse"),
      Fval(::rocket::make_refcnt<Simple_Binding_Wrapper>(
        // Description
        ::rocket::sref(
          "\n"
          "`std.chrono.utc_parse(time_str)`\n"
          "\n"
          "  * Parses `time_str`, which is an ASCII string representing a time\n"
          "    point in the format `1970-01-01 00:00:00.000`, according to the\n"
          "    ISO 8601 standard. The subsecond part is optional and may have\n"
          "    fewer or more digits. There may be leading or trailing spaces.\n"
          "\n"
          "  * Returns the number of milliseconds since `1970-01-01 00:00:00`\n"
          "    if the time string has been parsed successfully, or `null`\n"
          "    otherwise.\n"
        ),
        // Definition
        [](cow_vector<Reference>&& args) -> Value  {
          Argument_Reader reader(::rocket::sref("std.chrono.utc_parse"), ::rocket::ref(args));
          // Parse arguments.
          Sval time_str;
          if(reader.I().g(time_str).F()) {
            // Call the binding function.
            return std_chrono_utc_parse(::rocket::move(time_str));
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // End of `std.chrono`
    //===================================================================
  }

}  // namespace Asteria
