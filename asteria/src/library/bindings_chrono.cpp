// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "bindings_chrono.hpp"
#include "argument_reader.hpp"
#include "simple_binding_wrapper.hpp"
#include "../utilities.hpp"

#ifdef _WIN32
#  include <windows.h>
#else
#  include <time.h>
#endif

namespace Asteria {

std::int64_t std_chrono_utc_now()
  {
#ifdef _WIN32
    // Get UTC time from the system.
    ::FILETIME ft_u;
    ::GetSystemTimeAsFileTime(&ft_u);
    ::ULARGE_INTEGER ti;
    ti.LowPart = ft_u.dwLowDateTime;
    ti.HighPart = ft_u.dwHighDateTime;
    // Convert it to the number of milliseconds.
    // `116444736000000000` = duration from `1601-01-01` to `1970-01-01` in 100 nanoseconds.
    return static_cast<std::int64_t>(ti.QuadPart - 116444736000000000) / 10000;
#else
    // Get UTC time from the system.
    ::timespec ts;
    ::clock_gettime(CLOCK_REALTIME, &ts);
    // Convert it to the number of milliseconds.
    return static_cast<std::int64_t>(ts.tv_sec) * 1000 + ts.tv_nsec / 1000000;
#endif
  }

std::int64_t std_chrono_local_now()
  {
#ifdef _WIN32
    // Get local time from the system.
    ::FILETIME ft_u;
    ::GetSystemTimeAsFileTime(&ft_u);
    ::FILETIME ft_l;
    ::FileTimeToLocalFileTime(&ft_u, &ft_l);
    ::ULARGE_INTEGER ti;
    ti.LowPart = ft_l.dwLowDateTime;
    ti.HighPart = ft_l.dwHighDateTime;
    // Convert it to the number of milliseconds.
    // `116444736000000000` = duration from `1601-01-01` to `1970-01-01` in 100 nanoseconds.
    return static_cast<std::int64_t>(ti.QuadPart - 116444736000000000) / 10000;
#else
    // Get local time and GMT offset from the system.
    ::timespec ts;
    ::clock_gettime(CLOCK_REALTIME, &ts);
    ::tm tr;
    ::localtime_r(&(ts.tv_sec), &tr);
    // Convert it to the number of milliseconds.
    return (static_cast<std::int64_t>(ts.tv_sec) + tr.tm_gmtoff) * 1000 + ts.tv_nsec / 1000000;
#endif
  }

double std_chrono_hires_now()
  {
#ifdef _WIN32
    // Read the performance counter.
    // The performance counter frequency has to be obtained only once.
    static std::atomic<double> s_freq_recip;
    ::LARGE_INTEGER ti;
    auto freq_recip = s_freq_recip.load(std::memory_order_relaxed);
    if(ROCKET_UNEXPECT(!(freq_recip > 0))) {
      ::QueryPerformanceFrequency(&ti);
      freq_recip = 1000 / static_cast<double>(ti.QuadPart);
      s_freq_recip.store(freq_recip, std::memory_order_relaxed);
    }
    ::QueryPerformanceCounter(&ti);
    // Convert it to the number of milliseconds.
    // Add a random offset to the result to help debugging.
    return static_cast<double>(ti.QuadPart) * freq_recip + 0x987654321;
#else
    // Get the time since the system was started.
    ::timespec ts;
    ::clock_gettime(CLOCK_MONOTONIC, &ts);
    // Convert it to the number of milliseconds.
    // Add a random offset to the result to help debugging.
    return static_cast<double>(ts.tv_sec) * 1000 + static_cast<double>(ts.tv_nsec) / 1000000 + 0x987654321;
#endif
  }

std::int64_t std_chrono_steady_now()
  {
#ifdef _WIN32
    // Get the system tick count.
    // Add a random offset to the result to help debugging.
    return static_cast<std::int64_t>(::GetTickCount64()) + 0x123456789;
#else
    // Get the time since the system was started.
    ::timespec ts;
    ::clock_gettime(CLOCK_MONOTONIC_COARSE, &ts);
    // Convert it to the number of milliseconds.
    // Add a random offset to the result to help debugging.
    return static_cast<std::int64_t>(ts.tv_sec) * 1000 + ts.tv_nsec / 1000000 + 0x123456789;
#endif
  }

std::int64_t std_chrono_local_from_utc(std::int64_t time_utc)
  {
    // Get the offset of the local time from UTC in milliseconds.
    std::int64_t offset;
#ifdef _WIN32
    ::DYNAMIC_TIME_ZONE_INFORMATION dtz;
    ::GetDynamicTimeZoneInformation(&dtz);
    offset = dtz.Bias * -60000;
#else
    ::time_t tp = 0;
    ::tm tr;
    ::localtime_r(&tp, &tr);
    offset = tr.tm_gmtoff * 1000;
#endif
    // Add the time zone offset to `time_local` to get the local time.
    // Watch out for integral overflows.
    if((offset >= 0) && (time_utc > INT64_MAX - offset)) {
      return INT64_MAX;
    }
    if((offset < 0) && (time_utc < INT64_MIN - offset)) {
      return INT64_MIN;
    }
    return time_utc + offset;
  }

std::int64_t std_chrono_utc_from_local(std::int64_t time_local)
  {
    // Get the offset of the local time from UTC in milliseconds.
    std::int64_t offset;
#ifdef _WIN32
    ::DYNAMIC_TIME_ZONE_INFORMATION dtz;
    ::GetDynamicTimeZoneInformation(&dtz);
    offset = dtz.Bias * -60000;
#else
    ::time_t tp = 0;
    ::tm tr;
    ::localtime_r(&tp, &tr);
    offset = tr.tm_gmtoff * 1000;
#endif
    // Subtract the time zone offset from `time_local` to get the UTC time.
    // Watch out for integral overflows.
    if((offset >= 0) && (time_local < INT64_MIN + offset)) {
      return INT64_MIN;
    }
    if((offset < 0) && (time_local > INT64_MAX + offset)) {
      return INT64_MAX;
    }
    return time_local - offset;
  }

bool std_chrono_parse_datetime(std::int64_t &time_point_out, const Cow_String &time_str)
  {
    // Characters are read forwards, unlike `format_datetime()`.
    auto rpos = time_str.begin();
    // Define functions to read each field.
    // Be adviced that these functions modify `rpos`.
    const auto read_int = [&](auto &out, int width)
      {
        // The first digit is required.
        if(rpos == time_str.end()) {
          return false;
        }
        int d = *rpos - '0';
        if((d < 0) || (9 < d)) {
          return false;
        }
        ++rpos;
        // Parse as many digits as possible.
        int r = d;
        for(int i = 1; i < width; ++i) {
          if(rpos == time_str.end()) {
            break;
          }
          d = *rpos - '0';
          if((d < 0) || (9 < d)) {
            break;
          }
          ++rpos;
          r = r * 10 + d;
        }
        out = static_cast<typename std::decay<decltype(out)>::type>(r);
        return true;
      };
    const auto read_sep = [&](auto &&...seps)
      {
        if(rpos == time_str.end()) {
          return false;
        }
        if(rocket::is_none_of(*rpos, { seps... })) {
          return false;
        }
        ++rpos;
        return true;
      };
    // The millisecond part is optional so we have to declare some intermediate results here.
    bool succ;
    std::int64_t time_point;
#ifdef _WIN32
    // Parse the shortest acceptable substring, i.e. the substring without milliseconds.
    ::SYSTEMTIME st;
    succ = read_int(st.wYear, 4) &&
           read_sep('-') &&
           read_int(st.wMonth, 2) &&
           read_sep('-') &&
           read_int(st.wDay, 2) &&
           read_sep(' ', 'T') &&
           read_int(st.wHour, 2) &&
           read_sep(':') &&
           read_int(st.wMinute, 2) &&
           read_sep(':') &&
           read_int(st.wSecond, 2);
    if(!succ) {
      return false;
    }
    // Assemble the parts, assuming the millisecond field is zero..
    st.wMilliseconds = 0;
    ::FILETIME ft;
    if(!::SystemTimeToFileTime(&st, &ft)) {
      return false;
    }
    ::ULARGE_INTEGER ti;
    ti.LowPart = ft.dwLowDateTime;
    ti.HighPart = ft.dwHighDateTime;
    // Convert it to the number of milliseconds.
    // `116444736000000000` = duration from `1601-01-01` to `1970-01-01` in 100 nanoseconds.
    time_point = static_cast<std::int64_t>(ti.QuadPart - 116444736000000000) / 10000;
#else
    // Parse the shortest acceptable substring, i.e. the substring without milliseconds.
    ::tm tr;
    succ = read_int(tr.tm_year, 4) &&
           read_sep('-') &&
           read_int(tr.tm_mon, 2) &&
           read_sep('-') &&
           read_int(tr.tm_mday, 2) &&
           (read_sep(' ') || read_sep('T')) &&
           read_int(tr.tm_hour, 2) &&
           read_sep(':') &&
           read_int(tr.tm_min, 2) &&
           read_sep(':') &&
           read_int(tr.tm_sec, 2);
    if(!succ) {
      return false;
    }
    // Assemble the parts without milliseconds.
    tr.tm_year -= 1900;
    tr.tm_mon -= 1;
    tr.tm_isdst = 0;
    ::time_t tp = ::timegm(&tr);
    if(tp == static_cast<::time_t>(-1)) {
      return false;
    }
    // Convert it to the number of milliseconds.
    time_point = static_cast<std::int64_t>(tp) * 1000;
#endif
    // Parse the subsecond part if any.
    if(read_sep('.')) {
      // Parse digits backwards.
      // Use `rpos` as the loop counter.
      double r = 0;
      for(auto kpos = time_str.rbegin(); rpos != time_str.end(); ++kpos) {
        int d = *kpos - '0';
        if((d < 0) || (9 < d)) {
          return false;
        }
        ++rpos;
        r = (r + d) / 10;
      }
      time_point += static_cast<std::int64_t>(r * 1000);
    }
    // Reject invalid characters in the end of `time_str`.
    if(rpos != time_str.end()) {
      return false;
    }
    // Handle special time values.
    if(time_point <= -11644473600000) {
      time_point_out = INT64_MIN;
      return true;
    }
    if(time_point >= 253370764800000) {
      time_point_out = INT64_MAX;
      return true;
    }
    time_point_out = time_point;
    return true;
  }

void std_chrono_format_datetime(Cow_String &time_str_out, std::int64_t time_point, bool with_ms)
  {
    // Return strings that are allocated statically for special time point values.
    if(time_point <= -11644473600000) {
      time_str_out = rocket::sref(with_ms ? "1601-01-01 00:00:00.000"
                                          : "1601-01-01 00:00:00");
      return;
    }
    if(time_point >= 253370764800000) {
      time_str_out = rocket::sref(with_ms ? "9999-01-01 00:00:00.000"
                                          : "9999-01-01 00:00:00");
      return;
    }
    // Notice that the length of the result string is fixed.
    time_str_out.resize(std::char_traits<char>::length(with_ms ? "1601-01-01 00:00:00.000"
                                                               : "1601-01-01 00:00:00"));
    // Characters are written backwards, unlike `parse_datetime()`.
    auto wpos = time_str_out.mut_rbegin();
    // Define functions to write each field.
    // Be adviced that these functions modify `wpos`.
    const auto write_int = [&](int value, int width)
      {
        int r = value;
        for(int i = 0; i < width; ++i) {
          int d = r % 10;
          r /= 10;
          *wpos = static_cast<char>('0' + d);
          ++wpos;
        }
        return true;
      };
    const auto write_sep = [&](char sep)
      {
        *wpos = sep;
        ++wpos;
        return true;
      };
    // Break the time point down.
#ifdef _WIN32
    // Convert the time point to Windows NT time.
    // `116444736000000000` = duration from `1601-01-01` to `1970-01-01` in 100 nanoseconds.
    ::ULARGE_INTEGER ti;
    ti.QuadPart = static_cast<std::uint64_t>(time_point) * 10000 + 116444736000000000;
    ::FILETIME ft;
    ft.dwLowDateTime = ti.LowPart;
    ft.dwHighDateTime = ti.HighPart;
    ::SYSTEMTIME st;
    ::FileTimeToSystemTime(&ft, &st);
    // Write fields backwards.
    if(with_ms) {
      write_int(st.wMilliseconds, 3);
      write_sep('.');
    }
    write_int(st.wSecond, 2);
    write_sep(':');
    write_int(st.wMinute, 2);
    write_sep(':');
    write_int(st.wHour, 2);
    write_sep(' ');
    write_int(st.wDay, 2);
    write_sep('-');
    write_int(st.wMonth, 2);
    write_sep('-');
    write_int(st,wYear, 4);
#else
    // Write fields backwards.
    // Be advised that POSIX APIs handle seconds only.
    if(with_ms) {
      write_int(static_cast<int>(time_point % 1000), 3);
      write_sep('.');
    }
    ::time_t tp = time_point / 1000;
    ::tm tr;
    ::gmtime_r(&tp, &tr);
    write_int(tr.tm_sec, 2);
    write_sep(':');
    write_int(tr.tm_min, 2);
    write_sep(':');
    write_int(tr.tm_hour, 2);
    write_sep(' ');
    write_int(tr.tm_mday, 2);
    write_sep('-');
    write_int(tr.tm_mon + 1, 2);
    write_sep('-');
    write_int(tr.tm_year + 1900, 4);
#endif
    ROCKET_ASSERT(wpos == time_str_out.rend());
  }

Cow_String std_chrono_min_datetime(bool with_ms)
  {
    Cow_String time_str;
    std_chrono_format_datetime(time_str, INT64_MIN, with_ms);
    return time_str;
  }

Cow_String std_chrono_max_datetime(bool with_ms)
  {
    Cow_String time_str;
    std_chrono_format_datetime(time_str, INT64_MAX, with_ms);
    return time_str;
  }

D_object create_bindings_chrono()
  {
    D_object chrono;
    //===================================================================
    // `std.chrono.utc_now()`
    //===================================================================
    chrono.try_emplace(rocket::sref("utc_now"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.chrono.utc_now()`"
                     "\n  * Retrieves the wall clock time in UTC.                          "
                     "\n  * Returns the number of milliseconds since the Unix epoch,       "
                     "\n    represented as an `integer`.                                   "),
        // Definition
        [](const Cow_Vector<Value> & /*opaque*/, const Global_Context & /*global*/, Cow_Vector<Reference> &&args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.chrono.utc_now"), args);
            // Parse arguments.
            if(reader.start().finish()) {
              // Call the binding function.
              D_integer time_utc = std_chrono_utc_now();
              // Return it.
              Reference_Root::S_temporary ref_c = { time_utc };
              return rocket::move(ref_c);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameters
        { }
      )));
    //===================================================================
    // `std.chrono.local_now()`
    //===================================================================
    chrono.try_emplace(rocket::sref("local_now"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.chrono.local_now()`"
                     "\n  * Retrieves the wall clock time in the local time zone.          "
                     "\n  * Returns the number of milliseconds since `1970-01-01 00:00:00` "
                     "\n    in the local time zone, represented as an `integer`.           "),
        // Definition
        [](const Cow_Vector<Value> & /*opaque*/, const Global_Context & /*global*/, Cow_Vector<Reference> &&args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.chrono.local_now"), args);
            // Parse arguments.
            if(reader.start().finish()) {
              // Call the binding function.
              D_integer time_local = std_chrono_local_now();
              // Return it.
              Reference_Root::S_temporary ref_c = { time_local };
              return rocket::move(ref_c);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameters
        { }
      )));
    //===================================================================
    // `std.chrono.hires_now()`
    //===================================================================
    chrono.try_emplace(rocket::sref("hires_now"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.chrono.hires_now()`"
                     "\n  * Retrieves a time point from a high resolution clock. The clock "
                     "\n    goes monotonically and cannot be adjusted, being suitable for  "
                     "\n    time measurement. This function provides accuracy and might be "
                     "\n    quite heavyweight.                                             "
                     "\n  * Returns the number of milliseconds since an unspecified time   "
                     "\n    point, represented as a `real`.                                "),
        // Definition
        [](const Cow_Vector<Value> & /*opaque*/, const Global_Context & /*global*/, Cow_Vector<Reference> &&args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.chrono.hires_now"), args);
            // Parse arguments.
            if(reader.start().finish()) {
              // Call the binding function.
              D_real time_hires = std_chrono_hires_now();
              // Return it.
              Reference_Root::S_temporary ref_c = { time_hires };
              return rocket::move(ref_c);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameters
        { }
      )));
    //===================================================================
    // `std.chrono.steady_now()`
    //===================================================================
    chrono.try_emplace(rocket::sref("steady_now"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.chrono.steady_now()`"
                     "\n  * Retrieves a time point from a steady clock. The clock goes     "
                     "\n    monotonically and cannot be adjusted, being suitable for time  "
                     "\n    measurement. This function is supposed to be fast and might    "
                     "\n    have poor accuracy.                                            "
                     "\n  * Returns the number of milliseconds since an unspecified time   "
                     "\n    point, represented as an `integer`.                            "),
        // Definition
        [](const Cow_Vector<Value> & /*opaque*/, const Global_Context & /*global*/, Cow_Vector<Reference> &&args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.chrono.steady_now"), args);
            // Parse arguments.
            if(reader.start().finish()) {
              // Call the binding function.
              D_integer time_steady = std_chrono_steady_now();
              // Return it.
              Reference_Root::S_temporary ref_c = { time_steady };
              return rocket::move(ref_c);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameters
        { }
      )));
    //===================================================================
    // `std.chrono.local_from_utc(time_utc)`
    //===================================================================
    chrono.try_emplace(rocket::sref("local_from_utc"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.chrono.local_from_utc(time_utc)`"
                     "\n  * Converts a UTC time point to a local one. `time_utc` shall be  "
                     "\n    the number of milliseconds since the Unix epoch.               "
                     "\n  * Returns the number of milliseconds since `1970-01-01 00:00:00` "
                     "\n    in the local time zone, represented as an `integer`.           "),
        // Definition
        [](const Cow_Vector<Value> & /*opaque*/, const Global_Context & /*global*/, Cow_Vector<Reference> &&args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.chrono.local_from_utc"), args);
            // Parse arguments.
            D_integer time_utc;
            if(reader.start().req(time_utc).finish()) {
              // Call the binding function.
              D_integer time_local = std_chrono_local_from_utc(time_utc);
              // Return it.
              Reference_Root::S_temporary ref_c = { time_local };
              return rocket::move(ref_c);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameters
        { }
      )));
    //===================================================================
    // `std.chrono.utc_from_local(time_local)`
    //===================================================================
    chrono.try_emplace(rocket::sref("utc_from_local"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.chrono.utc_from_local(time_local)`"
                     "\n  * Converts a local time point to a UTC one. `time_local` shall   "
                     "\n    be the number of milliseconds since `1970-01-01 00:00:00` in   "
                     "\n    the local time zone.                                           "
                     "\n  * Returns the number of milliseconds since the Unix epoch,       "
                     "\n    represented as an `integer`.                                   "),
        // Definition
        [](const Cow_Vector<Value> & /*opaque*/, const Global_Context & /*global*/, Cow_Vector<Reference> &&args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.chrono.utc_from_local"), args);
            // Parse arguments.
            D_integer time_local;
            if(reader.start().req(time_local).finish()) {
              // Call the binding function.
              D_integer time_utc = std_chrono_utc_from_local(time_local);
              // Return it.
              Reference_Root::S_temporary ref_c = { time_utc };
              return rocket::move(ref_c);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameters
        { }
      )));
    //===================================================================
    // `std.chrono.parse_datetime(time_str)`
    //===================================================================
    chrono.try_emplace(rocket::sref("parse_datetime"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.chrono.parse_datetime(time_str)`"
                     "\n  * Parses `time_str`, which is an ASCII string representing a time"
                     "\n    point in the format `1970-01-01 00:00:00.000`, according to the"
                     "\n    ISO 8601 standard; the subsecond part is optional and may have "
                     "\n    fewer or more digits. There shall be no leading or trailing    "
                     "\n    spaces.                                                        "
                     "\n  * Returns the number of milliseconds since `1970-01-01 00:00:00` "
                     "\n    if the time string has been parsed successfully; otherwise     "
                     "\n    `null`.                                                        "),
        // Definition
        [](const Cow_Vector<Value> & /*opaque*/, const Global_Context & /*global*/, Cow_Vector<Reference> &&args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.chrono.parse_datetime"), args);
            // Parse arguments.
            D_string time_str;
            if(reader.start().req(time_str).finish()) {
              // Call the binding function.
              D_integer time_point;
              if(!std_chrono_parse_datetime(time_point, time_str)) {
                // Fail.
                return Reference_Root::S_null();
              }
              // Return the time point.
              Reference_Root::S_temporary ref_c = { time_point };
              return rocket::move(ref_c);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameters
        { }
      )));
    //===================================================================
    // `std.chrono.format_datetime(time_point, [with_ms])`
    //===================================================================
    chrono.try_emplace(rocket::sref("format_datetime"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.chrono.format_datetime(time_point, [with_ms])`"
                     "\n  * Converts `time_point`, which represents the number of          "
                     "\n    milliseconds since `1970-01-01 00:00:00`, to an ASCII string in"
                     "\n    the aforementioned format, according to the ISO 8601 standard. "
                     "\n    If `with_ms` is set to `true`, the string will have a 3-digit  "
                     "\n    fractional part. By default, no fractional part is added.      "),
        // Definition
        [](const Cow_Vector<Value> & /*opaque*/, const Global_Context & /*global*/, Cow_Vector<Reference> &&args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.chrono.format_datetime"), args);
            // Parse arguments.
            D_integer time_point;
            D_boolean with_ms = false;
            if(reader.start().req(time_point).opt(with_ms).finish()) {
              // Call the binding function.
              D_string time_str;
              std_chrono_format_datetime(time_str, time_point, with_ms);
              // Forward the result.
              Reference_Root::S_temporary ref_c = { rocket::move(time_str) };
              return rocket::move(ref_c);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameters
        { }
      )));
    //===================================================================
    // `std.chrono.min_datetime([with_ms])`
    //===================================================================
    chrono.try_emplace(rocket::sref("min_datetime"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.chrono.min_datetime([with_ms])`"
                     "\n  * Gets the special string that denotes the negative infinity time"
                     "\n    point. Calling this function has the same effect with calling  "
                     "\n    `format_datetime()` with the `time_point` argument set to      "
                     "\n    `-0x8000000000000000`.                                         "
                     "\n  * Returns the string `'1601-01-01 00:00:00'` or the string       "
                     "\n    `'1601-01-01 00:00:00.000'` according to `with_ms`.            "),
        // Definition
        [](const Cow_Vector<Value> & /*opaque*/, const Global_Context & /*global*/, Cow_Vector<Reference> &&args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.chrono.min_datetime"), args);
            // Parse arguments.
            D_boolean with_ms = false;
            if(reader.start().opt(with_ms).finish()) {
              // Call the binding function.
              D_string time_str = std_chrono_min_datetime(with_ms);
              // Forward the result.
              Reference_Root::S_temporary ref_c = { rocket::move(time_str) };
              return rocket::move(ref_c);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameters
        { }
      )));
    //===================================================================
    // `std.chrono.max_datetime([with_ms])`
    //===================================================================
    chrono.try_emplace(rocket::sref("max_datetime"),
      D_function(make_simple_binding(
        // Description
        rocket::sref("`std.chrono.max_datetime([with_ms])`"
                     "\n  * Gets the special string that denotes the positive infinity time"
                     "\n    point. Calling this function has the same effect with calling  "
                     "\n    `format_datetime()` with the `time_point` argument set to      "
                     "\n    `0x7FFFFFFFFFFFFFFF`.                                          "
                     "\n  * Returns the string `'9999-01-01 00:00:00'` or the string       "
                     "\n    `'9999-01-01 00:00:00.000'` according to `with_ms`.            "),
        // Definition
        [](const Cow_Vector<Value> & /*opaque*/, const Global_Context & /*global*/, Cow_Vector<Reference> &&args) -> Reference
          {
            Argument_Reader reader(rocket::sref("std.chrono.max_datetime"), args);
            // Parse arguments.
            D_boolean with_ms = false;
            if(reader.start().opt(with_ms).finish()) {
              // Call the binding function.
              D_string time_str = std_chrono_max_datetime(with_ms);
              // Forward the result.
              Reference_Root::S_temporary ref_c = { rocket::move(time_str) };
              return rocket::move(ref_c);
            }
            // Fail.
            reader.throw_no_matching_function_call();
          },
        // Opaque parameters
        { }
      )));
    //===================================================================
    // End of `std.chrono`
    //===================================================================
    return chrono;
  }

}  // namespace Asteria
