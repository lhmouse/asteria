// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "bindings_chrono.hpp"
#include "argument_reader.hpp"
#include "simple_binding_wrapper.hpp"
#include "../utilities.hpp"
#include <time.h>

namespace Asteria {

G_integer std_chrono_utc_now()
  {
    // Get UTC time from the system.
    ::timespec ts;
    ::clock_gettime(CLOCK_REALTIME, &ts);
    // We return the time in milliseconds rather than seconds.
    auto secs = static_cast<int64_t>(ts.tv_sec);
    auto msecs = ts.tv_nsec / 1'000'000;
    return secs * 1000 + msecs;
  }

G_integer std_chrono_local_now()
  {
    // Get local time and GMT offset from the system.
    ::timespec ts;
    ::clock_gettime(CLOCK_REALTIME, &ts);
    ::tm tr;
    ::localtime_r(&(ts.tv_sec), &tr);
    // We return the time in milliseconds rather than seconds.
    auto secs = static_cast<int64_t>(ts.tv_sec) + tr.tm_gmtoff;
    auto msecs = ts.tv_nsec / 1'000'000;
    return secs * 1000 + msecs;
  }

G_real std_chrono_hires_now()
  {
    // Get the time since the system was started.
    ::timespec ts;
    ::clock_gettime(CLOCK_MONOTONIC, &ts);
    // We return the time in milliseconds rather than seconds.
    // Add a random offset to the result to help debugging.
    auto secs = static_cast<double>(ts.tv_sec);
    auto msecs = static_cast<double>(ts.tv_nsec) / 1'000'000.0;
    return secs * 1000 + msecs + 1234567890123456;
  }

G_integer std_chrono_steady_now()
  {
    // Get the time since the system was started.
    ::timespec ts;
    ::clock_gettime(CLOCK_MONOTONIC_COARSE, &ts);
    // We return the time in milliseconds rather than seconds.
    // Add a random offset to the result to help debugging.
    auto secs = static_cast<int64_t>(ts.tv_sec);
    auto msecs = ts.tv_nsec / 1'000'000;
    return secs * 1000 + msecs + 6543210987654321;
  }

G_integer std_chrono_local_from_utc(const G_integer& time_utc)
  {
    // Handle special time values.
    if(time_utc <= -11644473600000) {
      return INT64_MIN;
    }
    if(time_utc >= 253370764800000) {
      return INT64_MAX;
    }
    // Calculate the local time.
    ::time_t tp = 0;
    ::tm tr;
    ::localtime_r(&tp, &tr);
    G_integer time_local = time_utc + tr.tm_gmtoff * 1000;
    // Ensure the value is within the range of finite values.
    if(time_local <= -11644473600000) {
      return INT64_MIN;
    }
    if(time_local >= 253370764800000) {
      return INT64_MAX;
    }
    return time_local;
  }

G_integer std_chrono_utc_from_local(const G_integer& time_local)
  {
    // Handle special time values.
    if(time_local <= -11644473600000) {
      return INT64_MIN;
    }
    if(time_local >= 253370764800000) {
      return INT64_MAX;
    }
    // Calculate the local time.
    ::time_t tp = 0;
    ::tm tr;
    ::localtime_r(&tp, &tr);
    G_integer time_utc = time_local - tr.tm_gmtoff * 1000;
    // Ensure the value is within the range of finite values.
    if(time_utc <= -11644473600000) {
      return INT64_MIN;
    }
    if(time_utc >= 253370764800000) {
      return INT64_MAX;
    }
    return time_utc;
  }

G_string std_chrono_utc_format(const G_integer& time_point, const opt<G_boolean>& with_ms)
  {
    // No millisecond part is added by default.
    bool pms = with_ms.value_or(false);
    // Handle special time points.
    if(time_point <= -11644473600000) {
      static constexpr char s_min_str[][24] = { "1601-01-01 00:00:00",
                                                "1601-01-01 00:00:00.000" };
      return ::rocket::sref(s_min_str[pms]);
    }
    if(time_point >= 253370764800000) {
      static constexpr char s_max_str[][24] = { "9999-01-01 00:00:00",
                                                "9999-01-01 00:00:00.000" };
      return ::rocket::sref(s_max_str[pms]);
    }
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

opt<G_integer> std_chrono_utc_parse(const G_string& time_str)
  {
    // Trim leading and trailing spaces. Fail if the string becomes empty.
    static constexpr char s_spaces[] = " \f\n\r\t\v";
    size_t off = time_str.find_first_not_of(s_spaces);
    if(off == G_string::npos) {
      return ::rocket::clear;
    }
    // The shortest form is '1234-67-90 23:56:89' which contains 19 characters.
    const char* p = time_str.data() + off;
    off = time_str.find_last_not_of(s_spaces) + 1;
    if(time_str.data() + off - p < 19) {
      return ::rocket::clear;
    }
    // Declare the timepoint value as two parts: 'yyyy-mm-dd HH:MM:SS' and '.sss'.
    ::tm tr = { };
    double msecs = 0;
    // Parse individual parts.
    ::rocket::ascii_numget numg;
    uint64_t temp = 0;
    auto xget = [&](int& val, size_t w, unsigned min, unsigned max, unsigned sub)
      {
        const char* ep = p + w;
        if(!numg.parse_U(p, ep, 10) || !numg.cast_U(temp, min, max) || (p != ep))
          return false;
        val = static_cast<int>(temp - sub);
        return true;
      };
    // 'yyyy'
    if(!xget(tr.tm_year, 4, 0, 9999, 1900))
      return ::rocket::clear;
    // '-' or '/'
    if(!::rocket::is_any_of(*(p++), { '-', '/' }))
      return ::rocket::clear;
    // 'mm'
    if(!xget(tr.tm_mon, 2, 1, 12, 1))
      return ::rocket::clear;
    // '-' or '/'
    if(!::rocket::is_any_of(*(p++), { '-', '/' }))
      return ::rocket::clear;
    // 'dd'
    if(!xget(tr.tm_mday, 2, 1, 31, 0))
      return ::rocket::clear;
    // ' ' or 'T'
    if(!::rocket::is_any_of(*(p++), { ' ', 'T' }))
      return ::rocket::clear;
    // 'HH'
    if(!xget(tr.tm_hour, 2, 0, 23, 0))
      return ::rocket::clear;
    // ':'
    if(!::rocket::is_any_of(*(p++), { ':' }))
      return ::rocket::clear;
    // 'MM'
    if(!xget(tr.tm_min, 2, 0, 59, 0))
      return ::rocket::clear;
    // ':'
    if(!::rocket::is_any_of(*(p++), { ':' }))
      return ::rocket::clear;
    // 'SS'
    // Note leap seconds.
    if(!xget(tr.tm_sec, 2, 0, 60, 0))
      return ::rocket::clear;
    // Check for the millisecond part.
    const char* ep = time_str.data() + off;
    if(*p == '.') {
      // Parse the second and millisecond parts as a whole.
      p -= 2;
      // 'SS.sss'
      if(!numg.parse_F(p, ep, 10) || !numg.cast_F(msecs, 0, 60.9999) || (p != ep))
        return ::rocket::clear;
      msecs = (msecs - tr.tm_sec) * 1000 + 0.01;
    }
    // Ensure all characters have been consumed.
    if(p != ep) {
      return ::rocket::clear;
    }
    // Handle special time values.
    if(tr.tm_year + 1900 < 1600) {
      return INT64_MIN;
    }
    if(tr.tm_year + 1900 > 9998) {
      return INT64_MAX;
    }
    // Assemble all parts.
    ::time_t tp = ::timegm(&tr);
    if(tp == ::time_t(-1)) {
      return ::rocket::clear;
    }
    return static_cast<int64_t>(tp) * 1000 + static_cast<int64_t>(msecs);
  }

void create_bindings_chrono(G_object& result, API_Version /*version*/)
  {
    //===================================================================
    // `std.chrono.utc_now()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("utc_now"),
      G_function(::rocket::make_refcnt<Simple_Binding_Wrapper>(
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
        [](cow_vector<Reference>&& args) -> Reference {
          Argument_Reader reader(::rocket::sref("std.chrono.utc_now"), ::rocket::ref(args));
          // Parse arguments.
          if(reader.start().finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_chrono_utc_now() };
            return ::rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.chrono.local_now()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("local_now"),
      G_function(::rocket::make_refcnt<Simple_Binding_Wrapper>(
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
        [](cow_vector<Reference>&& args) -> Reference {
          Argument_Reader reader(::rocket::sref("std.chrono.local_now"), ::rocket::ref(args));
          // Parse arguments.
          if(reader.start().finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_chrono_local_now() };
            return ::rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.chrono.hires_now()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("hires_now"),
      G_function(::rocket::make_refcnt<Simple_Binding_Wrapper>(
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
        [](cow_vector<Reference>&& args) -> Reference {
          Argument_Reader reader(::rocket::sref("std.chrono.hires_now"), ::rocket::ref(args));
          // Parse arguments.
          if(reader.start().finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_chrono_hires_now() };
            return ::rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.chrono.steady_now()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("steady_now"),
      G_function(::rocket::make_refcnt<Simple_Binding_Wrapper>(
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
        [](cow_vector<Reference>&& args) -> Reference {
          Argument_Reader reader(::rocket::sref("std.chrono.steady_now"), ::rocket::ref(args));
          // Parse arguments.
          if(reader.start().finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_chrono_steady_now() };
            return ::rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.chrono.local_from_utc()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("local_from_utc"),
      G_function(::rocket::make_refcnt<Simple_Binding_Wrapper>(
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
        [](cow_vector<Reference>&& args) -> Reference {
          Argument_Reader reader(::rocket::sref("std.chrono.local_from_utc"), ::rocket::ref(args));
          // Parse arguments.
          G_integer time_utc;
          if(reader.start().g(time_utc).finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_chrono_local_from_utc(time_utc) };
            return ::rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.chrono.utc_from_local()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("utc_from_local"),
      G_function(::rocket::make_refcnt<Simple_Binding_Wrapper>(
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
        [](cow_vector<Reference>&& args) -> Reference {
          Argument_Reader reader(::rocket::sref("std.chrono.utc_from_local"), ::rocket::ref(args));
          // Parse arguments.
          G_integer time_local;
          if(reader.start().g(time_local).finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_chrono_utc_from_local(time_local) };
            return ::rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.chrono.utc_format()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("utc_format"),
      G_function(::rocket::make_refcnt<Simple_Binding_Wrapper>(
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
        [](cow_vector<Reference>&& args) -> Reference {
          Argument_Reader reader(::rocket::sref("std.chrono.utc_format"), ::rocket::ref(args));
          // Parse arguments.
          G_integer time_point;
          opt<G_boolean> with_ms;
          if(reader.start().g(time_point).g(with_ms).finish()) {
            // Call the binding function.
            Reference_Root::S_temporary xref = { std_chrono_utc_format(time_point, with_ms) };
            return ::rocket::move(xref);
          }
          // Fail.
          reader.throw_no_matching_function_call();
        })
      ));
    //===================================================================
    // `std.chrono.utc_parse()`
    //===================================================================
    result.insert_or_assign(::rocket::sref("utc_parse"),
      G_function(::rocket::make_refcnt<Simple_Binding_Wrapper>(
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
        [](cow_vector<Reference>&& args) -> Reference {
          Argument_Reader reader(::rocket::sref("std.chrono.utc_parse"), ::rocket::ref(args));
          // Parse arguments.
          G_string time_str;
          if(reader.start().g(time_str).finish()) {
            // Call the binding function.
            auto qres = std_chrono_utc_parse(time_str);
            if(!qres) {
              return Reference_Root::S_null();
            }
            Reference_Root::S_temporary xref = { ::rocket::move(*qres) };
            return ::rocket::move(xref);
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
